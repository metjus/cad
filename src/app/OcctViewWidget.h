#pragma once
#include <AIS_InteractiveContext.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#include <QPoint>
#include <set>
#include <QWidget>

namespace hcad::app {

enum class SelectionMode { Object, Vertex, Edge, Face };
const char* toString(SelectionMode m);

// Native OCCT viewport hosted in a Qt widget. Presentation only: it knows
// nothing about features or parameters.
class OcctViewWidget : public QWidget {
  Q_OBJECT
public:
  explicit OcctViewWidget(QWidget* parent = nullptr);

  const Handle(AIS_InteractiveContext)& context() const { return context_; }
  const Handle(V3d_View)& view() const { return view_; }
  bool isReady() const { return !view_.IsNull(); }

  QPaintEngine* paintEngine() const override { return nullptr; }

  void setSelectionMode(SelectionMode m);
  SelectionMode selectionMode() const { return mode_; }
  void activateSelectionMode(const Handle(AIS_InteractiveObject)& obj);
  // Helper objects are displayed but never selectable (control cages...).
  void markHelper(const Handle(AIS_InteractiveObject)& obj) { helpers_.insert(obj.get()); }
  void clearHelpers() { helpers_.clear(); }

  void fitAll();
  void setStandardView(V3d_TypeOfOrientation o);
  void redraw();

  // Picks at a position in physical pixels (as V3d_View::Convert returns).
  bool pickAtPixel(int x, int y, bool extend);
  // Projects a 3D point to physical pixel coordinates.
  QPoint project(double x, double y, double z) const;
  bool dump(const QString& path);

signals:
  void viewReady();
  void selectionChanged();
  void cursorModel(double x, double y, double z);

protected:
  void showEvent(QShowEvent*) override;
  void paintEvent(QPaintEvent*) override;
  void resizeEvent(QResizeEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  void mouseMoveEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;

private:
  void initViewer();
  QPoint toPixels(const QPointF& logical) const;
  void syncSize();
  QSize lastSize_;

  Handle(V3d_Viewer) viewer_;
  Handle(V3d_View) view_;
  Handle(AIS_InteractiveContext) context_;
  SelectionMode mode_ = SelectionMode::Object;
  std::set<const AIS_InteractiveObject*> helpers_;

  enum class Drag { None, Orbit, Pan, Zoom } drag_ = Drag::None;
  QPoint pressPos_, lastPos_;
  bool moved_ = false;
};

} // namespace hcad::app
