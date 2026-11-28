#include "app/WelcomeDialog.h"
#include "app/Icons.h"
#include "app/Version.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace hcad::app {

namespace {
QRadioButton* option(const QString& title, const QString& detail, QVBoxLayout* into) {
  auto* rb = new QRadioButton(title);
  QFont f = rb->font();
  f.setBold(true);
  rb->setFont(f);
  into->addWidget(rb);
  auto* d = new QLabel(detail);
  d->setObjectName("hint");
  d->setWordWrap(true);
  d->setContentsMargins(24, 0, 0, 6);
  into->addWidget(d);
  return rb;
}
} // namespace

WelcomeDialog::WelcomeDialog(const Preferences& prefs, QWidget* parent) : QDialog(parent) {
  setObjectName("welcomeDialog");
  setWindowTitle("Welcome to HybridCAD");
  setMinimumWidth(640);
  auto* v = new QVBoxLayout(this);
  v->setContentsMargins(24, 20, 24, 18);
  v->setSpacing(12);

  auto* head = new QHBoxLayout;
  auto* logo = new QLabel;
  logo->setPixmap(Icons::get("app").pixmap(48, 48));
  head->addWidget(logo);
  auto* title = new QLabel(QString("<div style='font-size:17pt;font-weight:600'>Welcome to HybridCAD</div>"
                                   "<div style='color:#8a8e94'>Version %1 · early alpha</div>").arg(HCAD_VERSION));
  head->addWidget(title, 1);
  v->addLayout(head);

  auto* nav = new QGroupBox("How do you want to move around the 3D view?");
  auto* nl = new QVBoxLayout(nav);
  blender_ = option("Blender-style", "Middle mouse orbits, Shift + middle mouse pans, wheel zooms.", nl);
  cad_ = option("CAD-style", "Middle mouse pans, Shift + middle mouse orbits, wheel zooms.", nl);
  (prefs.navigation == NavigationStyle::Cad ? cad_ : blender_)->setChecked(true);
  auto* ng = new QButtonGroup(this);
  ng->addButton(blender_);
  ng->addButton(cad_);
  v->addWidget(nav);

  auto* units = new QGroupBox("Display units");
  auto* ul = new QHBoxLayout(units);
  mm_ = new QRadioButton("Millimetres");
  inch_ = new QRadioButton("Inches");
  (prefs.units == LengthUnit::Inch ? inch_ : mm_)->setChecked(true);
  ul->addWidget(mm_);
  ul->addWidget(inch_);
  ul->addStretch();
  auto* ug = new QButtonGroup(this);
  ug->addButton(mm_);
  ug->addButton(inch_);
  v->addWidget(units);

  auto* tips = new QLabel(
    "<b>Three things to know</b><ul style='margin-left:-20px'>"
    "<li><b>F3</b> (or Ctrl+K) searches every command, so you never have to hunt through menus.</li>"
    "<li><b>1 2 3 4</b> switch between selecting objects, vertices, edges and faces.</li>"
    "<li>Click the <b>cube</b> in the top-right corner to snap to a standard view.</li></ul>"
    "<span style='color:#8a8e94'>This alpha can open the example project and import STL/OBJ mesh references. "
    "Saving projects is not available yet.</span>");
  tips->setWordWrap(true);
  v->addWidget(tips);

  showAgain_ = new QCheckBox("Show this screen at startup");
  showAgain_->setChecked(prefs.showWelcome);
  v->addWidget(showAgain_);

  auto* buttons = new QHBoxLayout;
  buttons->addStretch();
  auto* empty = new QPushButton("Start with an empty project");
  auto* example = new QPushButton("Open the example project");
  example->setDefault(true);
  example->setObjectName("primary");
  buttons->addWidget(empty);
  buttons->addWidget(example);
  v->addLayout(buttons);
  connect(empty, &QPushButton::clicked, this, [this] { start_ = Start::Empty; accept(); });
  connect(example, &QPushButton::clicked, this, [this] { start_ = Start::Example; accept(); });
}

Preferences WelcomeDialog::result(Preferences p) const {
  p.navigation = cad_->isChecked() ? NavigationStyle::Cad : NavigationStyle::Blender;
  p.units = inch_->isChecked() ? LengthUnit::Inch : LengthUnit::Millimetre;
  p.showWelcome = showAgain_->isChecked();
  p.welcomeCompleted = true;
  return p;
}

} // namespace hcad::app
