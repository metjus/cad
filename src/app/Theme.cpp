#include "app/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace hcad::app {

void applyDarkTheme(QApplication& app) {
  app.setStyle(QStyleFactory::create("Fusion"));
  QPalette p;
  const QColor window(43, 45, 48), base(33, 34, 37), text(220, 222, 226), dim(130, 134, 140);
  const QColor accent(255, 170, 40); // same amber as viewport selection
  p.setColor(QPalette::Window, window);
  p.setColor(QPalette::WindowText, text);
  p.setColor(QPalette::Base, base);
  p.setColor(QPalette::AlternateBase, window);
  p.setColor(QPalette::ToolTipBase, QColor(25, 26, 28));
  p.setColor(QPalette::ToolTipText, text);
  p.setColor(QPalette::Text, text);
  p.setColor(QPalette::Button, window);
  p.setColor(QPalette::ButtonText, text);
  p.setColor(QPalette::Highlight, QColor(70, 110, 160));
  p.setColor(QPalette::HighlightedText, Qt::white);
  p.setColor(QPalette::Link, accent);
  p.setColor(QPalette::PlaceholderText, dim);
  p.setColor(QPalette::Disabled, QPalette::Text, dim);
  p.setColor(QPalette::Disabled, QPalette::ButtonText, dim);
  p.setColor(QPalette::Disabled, QPalette::WindowText, dim);
  app.setPalette(p);
  app.setStyleSheet(R"(
    QMainWindow::separator { background: #232427; width: 3px; height: 3px; }
    QDockWidget::title { background: #313337; padding: 5px 8px; font-weight: 600; }
    QToolBar { background: #2b2d30; border: none; spacing: 2px; padding: 4px; }
    QToolButton { padding: 6px 8px; border-radius: 4px; color: #dcdee2; }
    QToolButton:hover { background: #3a3d42; }
    QToolButton:checked { background: #4a3a1c; color: #ffb43c; }
    QGroupBox { border: 1px solid #3a3d42; border-radius: 4px; margin-top: 14px; padding-top: 6px; }
    QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #a6aab0; }
    QHeaderView::section { background: #313337; color: #a6aab0; border: none; padding: 4px; }
    QTabBar::tab { background: #2b2d30; padding: 6px 12px; color: #a6aab0; }
    QTabBar::tab:selected { background: #36383c; color: #dcdee2; }
    QStatusBar { background: #232427; color: #a6aab0; }
    QStatusBar QLabel { padding: 0 8px; }
    QLineEdit, QTableWidget, QListWidget, QTreeWidget { border: 1px solid #3a3d42; border-radius: 3px; }
    QLabel#error { color: #ff7a6a; }
    QLabel#hint { color: #8a8e94; }
    QDialogButtonBox { dialogbuttonbox-buttons-have-icons: 0; }
  )");
}

} // namespace hcad::app
