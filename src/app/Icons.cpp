#include "app/Icons.h"

#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <functional>

namespace hcad::app::Icons {
namespace {

using Draw = std::function<void(QPainter&, const QColor&)>;

QPen pen(const QColor& c, double w = 1.6) {
  QPen p(c, w);
  p.setCapStyle(Qt::RoundCap);
  p.setJoinStyle(Qt::RoundJoin);
  return p;
}

// Isometric cube corners on the 24 grid.
const QPointF T(12, 3.5), L(4.5, 7.8), R(19.5, 7.8), C(12, 12.1), BL(4.5, 16.4), BR(19.5, 16.4), B(12, 20.7);

void cubeOutline(QPainter& p, const QColor& c, double w = 1.6) {
  p.setPen(pen(c, w));
  p.setBrush(Qt::NoBrush);
  QPolygonF hex{T, R, BR, B, BL, L};
  p.drawPolygon(hex);
  p.drawLine(C, L);
  p.drawLine(C, R);
  p.drawLine(C, B);
}

const QHash<QString, Draw>& table() {
  static const QHash<QString, Draw> t = {
    {"select-object", [](QPainter& p, const QColor& c) {
       cubeOutline(p, c);
       QColor f = c; f.setAlphaF(0.25);
       p.setBrush(f); p.setPen(Qt::NoPen);
       p.drawPolygon(QPolygonF{T, R, BR, B, BL, L});
     }},
    {"select-vertex", [](QPainter& p, const QColor& c) {
       QColor d = c; d.setAlphaF(0.45);
       cubeOutline(p, d, 1.2);
       p.setPen(Qt::NoPen); p.setBrush(c);
       p.drawEllipse(C, 2.8, 2.8);
     }},
    {"select-edge", [](QPainter& p, const QColor& c) {
       QColor d = c; d.setAlphaF(0.45);
       cubeOutline(p, d, 1.2);
       p.setPen(pen(c, 2.6));
       p.drawLine(C, B);
     }},
    {"select-face", [](QPainter& p, const QColor& c) {
       QColor d = c; d.setAlphaF(0.45);
       cubeOutline(p, d, 1.2);
       p.setPen(pen(c, 1.6)); p.setBrush(c);
       p.drawPolygon(QPolygonF{T, R, C, L});
     }},
    {"fit", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       for (int sx : {-1, 1}) for (int sy : {-1, 1}) {
         QPointF k(12 + sx * 8, 12 + sy * 8);
         p.drawLine(k, k - QPointF(sx * 4.5, 0));
         p.drawLine(k, k - QPointF(0, sy * 4.5));
       }
       p.drawRect(QRectF(8.5, 8.5, 7, 7));
     }},
    {"view-iso", [](QPainter& p, const QColor& c) { cubeOutline(p, c); }},
    {"view-top", [](QPainter& p, const QColor& c) {
       cubeOutline(p, QColor(c.red(), c.green(), c.blue(), 110), 1.2);
       p.setPen(pen(c)); p.setBrush(c); p.drawPolygon(QPolygonF{T, R, C, L});
     }},
    {"view-front", [](QPainter& p, const QColor& c) {
       cubeOutline(p, QColor(c.red(), c.green(), c.blue(), 110), 1.2);
       p.setPen(pen(c)); p.setBrush(c); p.drawPolygon(QPolygonF{L, C, B, BL});
     }},
    {"view-right", [](QPainter& p, const QColor& c) {
       cubeOutline(p, QColor(c.red(), c.green(), c.blue(), 110), 1.2);
       p.setPen(pen(c)); p.setBrush(c); p.drawPolygon(QPolygonF{C, R, BR, B});
     }},
    {"import", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawLine(QPointF(12, 3.5), QPointF(12, 14));
       p.drawLine(QPointF(7.5, 9.5), QPointF(12, 14));
       p.drawLine(QPointF(16.5, 9.5), QPointF(12, 14));
       QPainterPath tray;
       tray.moveTo(4, 14); tray.lineTo(4, 19.5); tray.lineTo(20, 19.5); tray.lineTo(20, 14);
       p.drawPath(tray);
     }},
    {"search", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawEllipse(QPointF(10.5, 10.5), 6, 6);
       p.drawLine(QPointF(15, 15), QPointF(20, 20));
     }},
    {"settings", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       for (double y : {6.5, 12.0, 17.5}) p.drawLine(QPointF(4, y), QPointF(20, y));
       p.setBrush(c);
       p.drawEllipse(QPointF(15, 6.5), 2, 2);
       p.drawEllipse(QPointF(8, 12), 2, 2);
       p.drawEllipse(QPointF(13, 17.5), 2, 2);
     }},
    {"update", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawArc(QRectF(4.5, 4.5, 15, 15), 30 * 16, 280 * 16);
       p.drawLine(QPointF(18.5, 4.5), QPointF(18.7, 9.2));
       p.drawLine(QPointF(18.7, 9.2), QPointF(14.2, 8.6));
     }},
    {"help", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawEllipse(QPointF(12, 12), 8.5, 8.5);
       QPainterPath q;
       q.moveTo(9.3, 9.6); q.cubicTo(9.3, 6.4, 14.9, 6.4, 14.9, 9.6);
       q.cubicTo(14.9, 11.6, 12, 11.8, 12, 14);
       p.drawPath(q);
       p.setBrush(c); p.setPen(Qt::NoPen); p.drawEllipse(QPointF(12, 17), 1.1, 1.1);
     }},
    {"history", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawLine(QPointF(7, 5), QPointF(7, 19));
       for (double y : {6.0, 12.0, 18.0}) {
         p.setBrush(c); p.drawEllipse(QPointF(7, y), 1.8, 1.8); p.setBrush(Qt::NoBrush);
         p.drawLine(QPointF(11, y), QPointF(20, y));
       }
     }},
    {"parameters", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawText(QRectF(3, 3, 18, 18), Qt::AlignCenter, "x=");
     }},
    {"grid", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c, 1.2)); p.setBrush(Qt::NoBrush);
       for (double v : {5.0, 9.6, 14.3, 19.0}) { p.drawLine(QPointF(5, v), QPointF(19, v)); p.drawLine(QPointF(v, 5), QPointF(v, 19)); }
     }},
    {"welcome", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       QPainterPath h;
       h.moveTo(4, 11); h.lineTo(12, 4); h.lineTo(20, 11); h.moveTo(6.5, 9); h.lineTo(6.5, 20); h.lineTo(17.5, 20); h.lineTo(17.5, 9);
       p.drawPath(h);
     }},
    {"new", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       QPainterPath d;
       d.moveTo(6, 3.5); d.lineTo(14, 3.5); d.lineTo(18.5, 8); d.lineTo(18.5, 20.5); d.lineTo(6, 20.5); d.closeSubpath();
       p.drawPath(d);
       p.drawLine(QPointF(12.2, 10.5), QPointF(12.2, 17)); p.drawLine(QPointF(9, 13.8), QPointF(15.5, 13.8));
     }},
    {"example", [](QPainter& p, const QColor& c) {
       cubeOutline(p, c, 1.4);
       p.setPen(Qt::NoPen); p.setBrush(c);
       QPainterPath star;
       star.addEllipse(QPointF(19, 19), 3, 3);
       p.drawPath(star);
     }},
    {"perspective", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawPolygon(QPolygonF{QPointF(3.5, 6), QPointF(20.5, 9.5), QPointF(20.5, 14.5), QPointF(3.5, 18)});
       p.drawLine(QPointF(12, 7.8), QPointF(12, 16.2));
     }},
    {"axes", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       const QPointF o(9, 15);
       p.drawLine(o, QPointF(20, 15)); p.drawLine(o, QPointF(9, 3.5)); p.drawLine(o, QPointF(3.5, 20.5));
       p.setBrush(c); p.drawEllipse(o, 1.6, 1.6);
     }},
    {"cube", [](QPainter& p, const QColor& c) {
       cubeOutline(p, c, 1.4);
       p.setPen(Qt::NoPen); QColor f = c; f.setAlphaF(0.55); p.setBrush(f);
       p.drawPolygon(QPolygonF{L, C, B, BL});
     }},
    {"logs", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawRoundedRect(QRectF(4.5, 3.5, 15, 17), 2, 2);
       for (double y : {8.0, 12.0, 16.0}) p.drawLine(QPointF(8, y), QPointF(16, y));
     }},
    {"keyboard", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawRoundedRect(QRectF(2.5, 6.5, 19, 11), 2, 2);
       p.setPen(Qt::NoPen); p.setBrush(c);
       for (double x : {6.0, 9.5, 13.0, 16.5}) p.drawRect(QRectF(x, 9, 1.8, 1.8));
       p.drawRect(QRectF(7.5, 13.5, 9, 1.8));
     }},
    {"info", [](QPainter& p, const QColor& c) {
       p.setPen(pen(c)); p.setBrush(Qt::NoBrush);
       p.drawEllipse(QPointF(12, 12), 8.5, 8.5);
       p.drawLine(QPointF(12, 11), QPointF(12, 16.5));
       p.setPen(Qt::NoPen); p.setBrush(c); p.drawEllipse(QPointF(12, 7.8), 1.1, 1.1);
     }},
    {"app", [](QPainter& p, const QColor&) {
       // Application mark: amber isometric cube with one highlighted face.
       const QColor amber(255, 170, 40), dark(40, 42, 46);
       p.setPen(pen(amber, 1.8)); p.setBrush(dark);
       p.drawPolygon(QPolygonF{T, R, BR, B, BL, L});
       p.setBrush(amber); p.drawPolygon(QPolygonF{T, R, C, L});
       p.setBrush(Qt::NoBrush); p.drawLine(C, B);
     }},
  };
  return t;
}

QPixmap render(const Draw& draw, const QColor& color, double dpr) {
  const int px = static_cast<int>(24 * dpr);
  QPixmap pm(px, px);
  pm.fill(Qt::transparent);
  pm.setDevicePixelRatio(dpr);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  draw(p, color);
  return pm;
}

} // namespace

QIcon get(const QString& name) {
  static QHash<QString, QIcon> cache;
  if (auto it = cache.find(name); it != cache.end()) return *it;
  const auto& t = table();
  auto it = t.find(name);
  if (it == t.end()) return {};
  const QColor normal(200, 203, 208), active(255, 255, 255), on(255, 180, 60), disabled(90, 94, 100);
  QIcon icon;
  for (double dpr : {1.0, 2.0}) {
    icon.addPixmap(render(*it, normal, dpr), QIcon::Normal, QIcon::Off);
    icon.addPixmap(render(*it, active, dpr), QIcon::Active, QIcon::Off);
    icon.addPixmap(render(*it, on, dpr), QIcon::Normal, QIcon::On);
    icon.addPixmap(render(*it, on, dpr), QIcon::Active, QIcon::On);
    icon.addPixmap(render(*it, disabled, dpr), QIcon::Disabled, QIcon::Off);
    icon.addPixmap(render(*it, disabled, dpr), QIcon::Disabled, QIcon::On);
  }
  cache.insert(name, icon);
  return icon;
}

QStringList names() { return table().keys(); }

} // namespace hcad::app::Icons
