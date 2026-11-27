#include "app/SmokeTest.h"
#include "app/MainWindow.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QTimer>
#include <cstdio>

namespace hcad::app {
namespace {

struct Runner {
  MainWindow* w;
  QString dir;
  QString mesh;
  int failures = 0;

  void check(bool ok, const QString& what) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", what.toUtf8().constData());
    std::fflush(stdout);
    if (!ok) ++failures;
  }
  void settle() {
    for (int i = 0; i < 10; ++i) QApplication::processEvents();
    w->viewport()->redraw();
    QApplication::processEvents();
  }
  void shot(const QString& name) {
    settle();
    // Widgets are grabbed by Qt; the native OpenGL viewport is rendered by
    // OCCT (V3d_View::Dump) and composited into its rectangle. Screen grabs of
    // GL windows are unreliable on virtual X servers.
    const QString vp = dir + "/" + name + "_viewport.png";
    check(w->viewport()->dump(vp), "viewport render " + name);
    QPixmap pm = w->grab();
    {
      QPainter p(&pm);
      const QRect rect(w->viewport()->mapTo(w, QPoint(0, 0)), w->viewport()->size());
      p.drawImage(rect, QImage(vp));
    }
    QFile::remove(vp);
    check(pm.save(dir + "/" + name + ".png"), "screenshot " + name);
  }
  // Picks the centroid of a persistently named sub-shape of a feature body.
  bool pickRef(const std::string& featureId, const std::string& ref) {
    const auto* f = w->document().feature(featureId);
    if (!f) return false;
    auto s = f->result.body.resolve(ref);
    if (!s) return false;
    GProp_GProps p;
    if (s->ShapeType() == TopAbs_FACE) BRepGProp::SurfaceProperties(*s, p);
    else if (s->ShapeType() == TopAbs_EDGE) BRepGProp::LinearProperties(*s, p);
    else { BRepGProp::LinearProperties(*s, p); }
    gp_Pnt c = s->ShapeType() == TopAbs_VERTEX ? BRep_Tool_Pnt(*s) : p.CentreOfMass();
    QPoint px = w->viewport()->project(c.X(), c.Y(), c.Z());
    w->viewport()->pickAtPixel(px.x(), px.y(), false);
    settle();
    return true;
  }
  static gp_Pnt BRep_Tool_Pnt(const TopoDS_Shape& v);
};

} // namespace
} // namespace hcad::app

#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
gp_Pnt hcad::app::Runner::BRep_Tool_Pnt(const TopoDS_Shape& v) { return BRep_Tool::Pnt(TopoDS::Vertex(v)); }

namespace hcad::app {

void runSmokeTest(MainWindow* w, const QString& outDir, const QString& meshPath) {
  QDir().mkpath(outDir);
  QTimer::singleShot(1500, w, [w, outDir, meshPath] {
    Runner r{w, outDir, meshPath};
    r.settle();
    w->viewport()->fitAll();
    r.settle();
    const auto& doc = w->document();
    r.check(doc.feature("Fillet1") && doc.feature("Fillet1")->status == doc::FeatureStatus::Ok, "demo history computed");
    r.shot("01_overview");

    w->setSelectionMode(SelectionMode::Face);
    r.pickRef("Fillet1", "Box1.Top");
    r.check(w->selectionText() == "Fillet1 : Box1.Top", "face pick resolves to persistent name: " + w->selectionText());
    r.shot("02_face_selected");

    w->document().parameters().setExpression("Width", "90 mm");
    int restored = w->recomputeAndRefresh();
    r.settle();
    r.check(restored == 1 && w->selectionText() == "Fillet1 : Box1.Top",
            "after Width=90 the same face is re-selected: " + w->selectionText());
    const auto* ch = doc.feature("Chamfer1");
    r.check(ch && ch->status == doc::FeatureStatus::Ok, "chamfer still valid after upstream edit");
    w->viewport()->fitAll();
    r.shot("03_after_width_edit");

    w->setSelectionMode(SelectionMode::Edge);
    r.pickRef("Fillet1", "Edge(Box1.Front|Box1.Left)");
    r.check(w->selectionText() == "Fillet1 : Edge(Box1.Front|Box1.Left)", "edge pick: " + w->selectionText());
    r.shot("04_edge_selected");

    w->setSelectionMode(SelectionMode::Vertex);
    r.pickRef("Fillet1", "Vertex(Box1.Bottom|Box1.Front|Box1.Left)");
    r.check(w->selectionText() == "Fillet1 : Vertex(Box1.Bottom|Box1.Front|Box1.Left)", "vertex pick: " + w->selectionText());

    w->setSelectionMode(SelectionMode::Face);
    w->viewport()->setStandardView(V3d_XnegYposZpos); // look at the surface side
    r.pickRef("CPEdit2", "Surface1.Surface");
    r.check(w->selectionText() == "CPEdit2 : Surface1.Surface", "NURBS face pick: " + w->selectionText());
    r.shot("05_nurbs_selected");

    if (!meshPath.isEmpty()) {
      r.check(w->importMeshFile(meshPath), "mesh import " + meshPath);
      r.check(!w->document().meshes().empty() && w->document().meshes().back().report.closed, "imported mesh is watertight");
      w->viewport()->fitAll();
      r.shot("06_mesh_reference");
    }
    std::printf("SMOKE TEST %s (%d failure(s))\n", r.failures ? "FAILED" : "PASSED", r.failures);
    QApplication::exit(r.failures ? 1 : 0);
  });
}

} // namespace hcad::app
