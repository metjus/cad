#include "app/OcctViewWidget.h"

#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_RenderingParams.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Image_AlienPixMap.hxx>
#include <Quantity_Color.hxx>

#ifdef _WIN32
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

#include <QMouseEvent>
#include <cmath>
#include <cstdio>
#include <Graphic3d_Camera.hxx>
#include <QWheelEvent>

namespace hcad::app {

const char* toString(SelectionMode m) {
  switch (m) {
  case SelectionMode::Object: return "Object";
  case SelectionMode::Vertex: return "Vertex";
  case SelectionMode::Edge: return "Edge";
  case SelectionMode::Face: return "Face";
  }
  return "?";
}

OcctViewWidget::OcctViewWidget(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_NativeWindow);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
  setMinimumSize(320, 240);
}

static Quantity_Color rgb(int r, int g, int b) {
  return Quantity_Color(r / 255.0, g / 255.0, b / 255.0, Quantity_TOC_sRGB);
}

void OcctViewWidget::initViewer() {
  Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(display, false);
  viewer_ = new V3d_Viewer(driver);
  viewer_->SetDefaultLights();
  viewer_->SetLightOn();
  viewer_->SetDefaultViewProj(V3d_XposYnegZpos);

  view_ = viewer_->CreateView();
#ifdef _WIN32
  Handle(WNT_Window) wnd = new WNT_Window(reinterpret_cast<Aspect_Handle>(winId()));
#else
  Handle(Xw_Window) wnd = new Xw_Window(display, static_cast<Aspect_Drawable>(winId()));
#endif
  view_->SetWindow(wnd);
  if (!wnd->IsMapped()) wnd->Map();

  // Neutral charcoal background with a very subtle vertical gradient.
  view_->SetBgGradientColors(rgb(52, 54, 58), rgb(30, 31, 34), Aspect_GradientFillMethod_Vertical);
  bool msaaSet = false;
  const int msaa = qEnvironmentVariableIntValue("HCAD_MSAA", &msaaSet);
  view_->ChangeRenderingParams().NbMsaaSamples = msaaSet ? msaa : 4;
  view_->ChangeRenderingParams().IsAntialiasingEnabled = true;
  view_->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.09, V3d_ZBUFFER);

  viewer_->SetRectangularGridValues(0, 0, 10, 10, 0);
  viewer_->SetRectangularGridGraphicValues(200, 200, 0);
  viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

  context_ = new AIS_InteractiveContext(viewer_);
  context_->SetPixelTolerance(4);
  // Clear selection/hover colours (amber = selected, light blue = hover).
  context_->HighlightStyle(Prs3d_TypeOfHighlight_Selected)->SetColor(rgb(255, 170, 40));
  context_->HighlightStyle(Prs3d_TypeOfHighlight_LocalSelected)->SetColor(rgb(255, 170, 40));
  context_->HighlightStyle(Prs3d_TypeOfHighlight_Dynamic)->SetColor(rgb(110, 180, 255));
  context_->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic)->SetColor(rgb(110, 180, 255));
  context_->HighlightStyle(Prs3d_TypeOfHighlight_LocalSelected)->SetDisplayMode(AIS_Shaded);
  context_->DefaultDrawer()->SetFaceBoundaryDraw(true);
  context_->DefaultDrawer()->SetFaceBoundaryAspect(new Prs3d_LineAspect(rgb(20, 20, 22), Aspect_TOL_SOLID, 1.0));
  context_->SetDisplayMode(AIS_Shaded, false);

  view_->MustBeResized();
  emit viewReady();
}

void OcctViewWidget::showEvent(QShowEvent*) {
  if (view_.IsNull()) initViewer();
}

void OcctViewWidget::syncSize() {
  if (view_.IsNull()) return;
  // Compare against the size OCCT believes the native window has; the
  // native window may be resized by Qt after our resize event fired.
  const QSize physical = size() * devicePixelRatioF();
  int w = 0, h = 0;
  view_->Window()->Size(w, h);
  const double aspect = physical.height() > 0 ? double(physical.width()) / physical.height() : 1.0;
  if (physical != QSize(w, h) || physical != lastSize_ || std::abs(view_->Camera()->Aspect() - aspect) > 1e-6) {
    lastSize_ = physical;
    view_->MustBeResized();
  }
}

void OcctViewWidget::paintEvent(QPaintEvent*) {
  syncSize();
  if (!view_.IsNull()) view_->Redraw();
}

void OcctViewWidget::resizeEvent(QResizeEvent*) {
  syncSize();
}

void OcctViewWidget::redraw() {
  syncSize();
  if (!view_.IsNull()) view_->Redraw();
}

QPoint OcctViewWidget::toPixels(const QPointF& p) const {
  const double r = devicePixelRatioF();
  return QPoint(static_cast<int>(p.x() * r), static_cast<int>(p.y() * r));
}

void OcctViewWidget::setSelectionMode(SelectionMode m) {
  mode_ = m;
  if (context_.IsNull()) return;
  context_->ClearSelected(false);
  AIS_ListOfInteractive objs;
  context_->DisplayedObjects(objs);
  for (AIS_ListOfInteractive::Iterator it(objs); it.More(); it.Next()) activateSelectionMode(it.Value());
  redraw();
  emit selectionChanged();
}

void OcctViewWidget::activateSelectionMode(const Handle(AIS_InteractiveObject)& obj) {
  if (context_.IsNull() || obj.IsNull()) return;
  context_->Deactivate(obj);
  if (!obj->IsKind(STANDARD_TYPE(AIS_Shape))) {
    if (mode_ == SelectionMode::Object) context_->Activate(obj, 0);
    return;
  }
  if (helpers_.count(obj.get())) return; // helper geometry, e.g. control cage
  switch (mode_) {
  case SelectionMode::Object: context_->Activate(obj, 0); break;
  case SelectionMode::Vertex: context_->Activate(obj, AIS_Shape::SelectionMode(TopAbs_VERTEX)); break;
  case SelectionMode::Edge: context_->Activate(obj, AIS_Shape::SelectionMode(TopAbs_EDGE)); break;
  case SelectionMode::Face: context_->Activate(obj, AIS_Shape::SelectionMode(TopAbs_FACE)); break;
  }
}

void OcctViewWidget::fitAll() {
  if (view_.IsNull()) return;
  syncSize();
  view_->FitAll(0.15, false);
  view_->ZFitAll();
  view_->Redraw();
}

void OcctViewWidget::setStandardView(V3d_TypeOfOrientation o) {
  if (view_.IsNull()) return;
  view_->SetProj(o);
  fitAll();
}

bool OcctViewWidget::pickAtPixel(int x, int y, bool extend) {
  if (context_.IsNull()) return false;
  syncSize();
  context_->MoveTo(x, y, view_, false);
  if (extend) context_->SelectDetected(AIS_SelectionScheme_XOR);
  else context_->SelectDetected(AIS_SelectionScheme_Replace);
  view_->Redraw();
  emit selectionChanged();
  return context_->NbSelected() > 0;
}

QPoint OcctViewWidget::project(double x, double y, double z) const {
  int px = 0, py = 0;
  if (!view_.IsNull()) view_->Convert(x, y, z, px, py);
  return {px, py};
}

bool OcctViewWidget::dump(const QString& path) {
  if (view_.IsNull()) return false;
  view_->Redraw();
  return view_->Dump(path.toUtf8().constData());
}

void OcctViewWidget::mousePressEvent(QMouseEvent* e) {
  if (view_.IsNull()) return;
  const QPoint p = toPixels(e->position());
  pressPos_ = lastPos_ = p;
  moved_ = false;
  const bool alt = e->modifiers() & Qt::AltModifier;
  if (e->button() == Qt::MiddleButton || (alt && e->button() == Qt::LeftButton)) {
    if (e->modifiers() & Qt::ShiftModifier) drag_ = Drag::Pan;
    else if (e->modifiers() & Qt::ControlModifier) drag_ = Drag::Zoom;
    else { drag_ = Drag::Orbit; view_->StartRotation(p.x(), p.y()); }
  } else if (e->button() == Qt::RightButton) {
    drag_ = Drag::Pan;
  }
}

void OcctViewWidget::mouseMoveEvent(QMouseEvent* e) {
  if (view_.IsNull()) return;
  const QPoint p = toPixels(e->position());
  if ((p - pressPos_).manhattanLength() > 3) moved_ = true;
  switch (drag_) {
  case Drag::Orbit: view_->Rotation(p.x(), p.y()); break;
  case Drag::Pan: view_->Pan(p.x() - lastPos_.x(), lastPos_.y() - p.y()); break;
  case Drag::Zoom: view_->Zoom(lastPos_.x(), lastPos_.y(), p.x(), p.y()); break;
  case Drag::None:
    context_->MoveTo(p.x(), p.y(), view_, true);
    {
      double x, y, z;
      view_->Convert(p.x(), p.y(), x, y, z);
      emit cursorModel(x, y, z);
    }
    break;
  }
  lastPos_ = p;
}

void OcctViewWidget::mouseReleaseEvent(QMouseEvent* e) {
  if (view_.IsNull()) return;
  const bool wasDrag = drag_ != Drag::None;
  drag_ = Drag::None;
  if (e->button() == Qt::LeftButton && !wasDrag && !moved_) {
    const QPoint p = toPixels(e->position());
    pickAtPixel(p.x(), p.y(), e->modifiers() & Qt::ShiftModifier);
  }
}

void OcctViewWidget::wheelEvent(QWheelEvent* e) {
  if (view_.IsNull()) return;
  const QPoint p = toPixels(e->position());
  const int delta = e->angleDelta().y() / 8;
  view_->StartZoomAtPoint(p.x(), p.y());
  view_->ZoomAtPoint(0, 0, delta, delta);
}

} // namespace hcad::app
