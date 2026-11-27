#include "app/SceneSync.h"
#include "app/OcctViewWidget.h"
#include "kernel/MeshBridge.h"
#include "kernel/Nurbs.h"

#include <AIS_PointCloud.hxx>
#include <AIS_Triangulation.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <SelectMgr_Selection.hxx>
#include <SelectMgr_SensitiveEntity.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopAbs.hxx>

namespace hcad::app {

static Quantity_Color rgb(int r, int g, int b) {
  return Quantity_Color(r / 255.0, g / 255.0, b / 255.0, Quantity_TOC_sRGB);
}

void SceneSync::rebuild(const doc::Document& doc) {
  auto ctx = view_->context();
  ctx->RemoveAll(false);
  view_->clearHelpers();
  owners_.clear();
  bodies_.clear();

  for (const doc::Feature* f : doc.tipFeatures()) {
    if (f->result.body.isNull()) continue;
    Handle(AIS_Shape) ais = new AIS_Shape(f->result.body.shape());
    const bool isSurface = f->result.nurbs.has_value();
    ais->SetMaterial(Graphic3d_MaterialAspect(Graphic3d_NameOfMaterial_Satin));
    ais->SetColor(isSurface ? rgb(110, 175, 150) : rgb(178, 182, 190));
    if (isSurface) {
      // Show both sides of an open sheet clearly.
      ais->Attributes()->SetFaceBoundaryDraw(true);
    }
    // Display with mode 0 so the object is registered with the selector;
    // displaying with -1 and activating later leaves sub-shape modes unpickable
    // (observed with OCCT 7.6).
    ctx->Display(ais, AIS_Shaded, 0, false);
    owners_[ais.get()] = f->id();
    bodies_[f->id()] = ais;
    view_->activateSelectionMode(ais);

    if (isSurface) {
      // Control cage (polygon) and control points: display helpers.
      const auto& def = *f->result.nurbs;
      Handle(AIS_Shape) cage = new AIS_Shape(kernel::controlCage(def));
      cage->SetColor(rgb(240, 200, 90));
      cage->SetWidth(1.0);
      cage->Attributes()->SetLineAspect(new Prs3d_LineAspect(rgb(240, 200, 90), Aspect_TOL_SOLID, 1.0));
      cage->Attributes()->SetWireAspect(new Prs3d_LineAspect(rgb(240, 200, 90), Aspect_TOL_SOLID, 1.0));
      cage->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_EMPTY, rgb(240, 200, 90), 0.1));
      view_->markHelper(cage);
      ctx->Display(cage, AIS_WireFrame, -1, false);
      ctx->Deactivate(cage);

      Handle(Graphic3d_ArrayOfPoints) pts = new Graphic3d_ArrayOfPoints(static_cast<int>(def.poles.size()));
      for (const auto& p : def.poles) pts->AddVertex(p);
      Handle(AIS_PointCloud) cloud = new AIS_PointCloud();
      cloud->SetPoints(pts);
      cloud->SetColor(rgb(255, 215, 110));
      cloud->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT, rgb(255, 215, 110), 2.0));
      view_->markHelper(cloud);
      ctx->Display(cloud, AIS_PointCloud::DM_Points, -1, false);
      ctx->Deactivate(cloud);
    }
  }

  for (const auto& m : doc.meshes()) {
    if (!m.visible) continue;
    Handle(AIS_Triangulation) tri = new AIS_Triangulation(kernel::toPolyTriangulation(m.mesh));
    // AIS_Triangulation reads the shading aspect directly; SetColor() alone
    // has no effect on it. Violet = reference mesh (never CAD geometry).
    Handle(Prs3d_ShadingAspect) shade = new Prs3d_ShadingAspect();
    Graphic3d_MaterialAspect meshMat(Graphic3d_NameOfMaterial_Plastified);
    meshMat.SetColor(rgb(150, 120, 210));
    shade->SetMaterial(meshMat);
    shade->SetColor(rgb(150, 120, 210));
    shade->SetTransparency(0.2);
    tri->Attributes()->SetShadingAspect(shade);
    view_->markHelper(tri); // reference meshes are not CAD-selectable in Phase 0
    ctx->Display(tri, 0, -1, false);
    ctx->Deactivate(tri);
    owners_[tri.get()] = m.id;
  }
  view_->redraw();
}

const std::string* SceneSync::featureOf(const Handle(AIS_InteractiveObject)& obj) const {
  auto it = owners_.find(obj.get());
  return it == owners_.end() ? nullptr : &it->second;
}

Handle(AIS_Shape) SceneSync::bodyPresentation(const std::string& id) const {
  auto it = bodies_.find(id);
  return it == bodies_.end() ? Handle(AIS_Shape)() : it->second;
}

std::vector<SelectionRef> SceneSync::currentSelection(const doc::Document& doc) const {
  std::vector<SelectionRef> out;
  auto ctx = view_->context();
  for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected()) {
    const std::string* fid = featureOf(ctx->SelectedInteractive());
    if (!fid) continue;
    SelectionRef ref{*fid, {}};
    if (view_->selectionMode() != SelectionMode::Object && ctx->HasSelectedShape()) {
      const doc::Feature* f = doc.feature(*fid);
      if (f) {
        auto name = f->result.body.nameOf(ctx->SelectedShape());
        ref.reference = name ? *name : std::string("<unnamed ") + TopAbs::ShapeTypeToString(ctx->SelectedShape().ShapeType()) + ">";
      }
    }
    out.push_back(ref);
  }
  return out;
}

int SceneSync::restoreSelection(const doc::Document& doc, const std::vector<SelectionRef>& refs) {
  auto ctx = view_->context();
  ctx->ClearSelected(false);
  int restored = 0;
  for (const auto& r : refs) {
    Handle(AIS_Shape) ais = bodyPresentation(r.featureId);
    const doc::Feature* f = doc.feature(r.featureId);
    if (ais.IsNull() || !f) continue;
    if (r.reference.empty()) {
      ctx->AddOrRemoveSelected(ais, false);
      ++restored;
      continue;
    }
    auto target = f->result.body.resolve(r.reference);
    if (!target) continue;
    const int mode = AIS_Shape::SelectionMode(target->ShapeType());
    if (!ais->HasSelection(mode)) continue;
    const Handle(SelectMgr_Selection)& sel = ais->Selection(mode);
    for (const auto& entity : sel->Entities()) {
      Handle(StdSelect_BRepOwner) owner = Handle(StdSelect_BRepOwner)::DownCast(entity->BaseSensitive()->OwnerId());
      if (!owner.IsNull() && owner->Shape().IsSame(*target)) {
        ctx->AddOrRemoveSelected(owner, false);
        ++restored;
        break;
      }
    }
  }
  ctx->UpdateCurrentViewer();
  return restored;
}

} // namespace hcad::app
