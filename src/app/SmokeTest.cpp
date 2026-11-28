#include "app/SmokeTest.h"
#include "app/MainWindow.h"
#include "app/CommandPalette.h"
#include "app/PreferencesDialog.h"
#include "app/WelcomeDialog.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QTimer>
#include <cmath>
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
  // Screenshot of a top-level dialog on its own.
  void shotDialog(QWidget* dlg, const QString& name) {
    dlg->show();
    settle();
    check(dlg->grab().save(dir + "/" + name + ".png"), "screenshot " + name);
    dlg->hide();
  }
  void shot(const QString& name, QWidget* overlay = nullptr) {
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
    if (overlay && overlay->isVisible()) {
      QPainter p(&pm);
      p.drawPixmap(overlay->mapToGlobal(QPoint(0, 0)) - w->mapToGlobal(QPoint(0, 0)), overlay->grab());
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

    // ---- Phase 1: view cube, command palette, onboarding, preferences
    {
      r.check(w->commands().trigger("view.front"), "front view command");
      r.settle();
      double dx, dy, dz;
      w->viewport()->view()->Proj(dx, dy, dz);
      r.check(std::abs(dy + 1.0) < 1e-6, "front view looks along +Y");
      // The cube sits 85 px from the top-right corner. Click along its
      // vertical centre line until a cube part other than Front is hit.
      const double dpr = w->viewport()->devicePixelRatioF();
      const int cx = static_cast<int>((w->viewport()->width() - 85) * dpr);
      w->setSelectionMode(SelectionMode::Object);
      bool changed = false;
      for (int off = -40; off <= 40 && !changed; off += 4) {
        w->viewport()->pickAtPixel(cx, static_cast<int>((85 + off) * dpr), false);
        w->viewport()->view()->Proj(dx, dy, dz);
        changed = std::abs(dy + 1.0) > 1e-3;
      }
      r.settle();
      r.check(changed, QString("view cube click changes the view (proj %1 %2 %3)").arg(dx).arg(dy).arg(dz));
      r.check(w->selectionText().isEmpty(), "view cube click does not select model geometry");
      r.check(w->commands().trigger("view.iso"), "isometric view command");
      r.shot("07_view_cube");

      const bool gridBefore = w->viewport()->gridVisible();
      w->commands().trigger("view.grid");
      r.check(w->viewport()->gridVisible() != gridBefore && w->commands().action("view.grid")->isChecked() == !gridBefore,
              "grid toggle command updates viewport and check state");
      w->commands().trigger("view.grid");

      w->palette()->popup();
      w->palette()->setQuery("view");
      r.check(w->palette()->visibleIds().size() >= 7, QString("palette lists view commands (%1)").arg(w->palette()->visibleIds().size()));
      r.shot("08_command_palette", w->palette());
      w->palette()->setQuery("fit al");
      r.check(!w->palette()->visibleIds().isEmpty() && w->palette()->visibleIds().front() == "view.fitAll", "palette: 'fit al' -> Fit All");
      r.check(w->palette()->runCurrent(), "palette runs the highlighted command");

      WelcomeDialog welcome(w->preferences(), w);
      r.shotDialog(&welcome, "09_welcome");
      PreferencesDialog prefs(w->preferences(), w);
      r.shotDialog(&prefs, "10_preferences");
    }

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
