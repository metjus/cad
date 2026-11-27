#include "app/PreferencesDialog.h"
#include "app/AppPaths.h"
#include "app/Logging.h"
#include "app/Version.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

namespace hcad::app {

PreferencesDialog::PreferencesDialog(const Preferences& p, QWidget* parent) : QDialog(parent) {
  setWindowTitle("Preferences");
  setMinimumWidth(560);
  auto* v = new QVBoxLayout(this);

  auto* general = new QGroupBox("General");
  auto* gf = new QFormLayout(general);
  nav_ = new QComboBox;
  nav_->addItem("Blender-style (MMB orbit, Shift+MMB pan)");
  nav_->addItem("CAD-style (MMB pan, Shift+MMB orbit)");
  nav_->setCurrentIndex(p.navigation == NavigationStyle::Cad ? 1 : 0);
  gf->addRow("Navigation", nav_);
  units_ = new QComboBox;
  units_->addItems({"Millimetres", "Inches"});
  units_->setCurrentIndex(p.units == LengthUnit::Inch ? 1 : 0);
  gf->addRow("Display units", units_);
  welcome_ = new QCheckBox("Show the welcome screen at startup");
  welcome_->setChecked(p.showWelcome);
  gf->addRow(welcome_);
  v->addWidget(general);

  auto* upd = new QGroupBox("Updates");
  auto* uf = new QFormLayout(upd);
  checkUpdates_ = new QCheckBox("Check for updates when HybridCAD starts");
  checkUpdates_->setChecked(p.checkUpdatesOnStartup);
  uf->addRow(checkUpdates_);
  channel_ = new QComboBox;
  channel_->addItem("Stable", "stable");
  channel_->addItem("Beta", "beta");
  channel_->setCurrentIndex(p.updateChannel == "beta" ? 1 : 0);
  uf->addRow("Channel", channel_);
  feed_ = new QLineEdit(p.updateFeed);
  feed_->setPlaceholderText(QString("Default: %1").arg(HCAD_DEFAULT_UPDATE_FEED));
  feed_->setToolTip("Advanced: an HTTPS release feed or a local folder. Leave empty for the default.");
  uf->addRow("Release feed", feed_);
  v->addWidget(upd);

  auto* where = new QGroupBox("Where HybridCAD keeps things");
  auto* wf = new QFormLayout(where);
  auto row = [&](const QString& label, const QString& path) {
    auto* l = new QLabel(path);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->setObjectName("hint");
    wf->addRow(label, l);
  };
  row("Program files", AppPaths::installDir());
  row("User data", AppPaths::dataDir());
  row("Log file", currentLogFile());
  row("Settings", QSettings().fileName());
  auto* note = new QLabel("Updates replace only the program files. User data, settings and projects are never touched.");
  note->setWordWrap(true);
  wf->addRow(note);
  v->addWidget(where);

  auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
  v->addWidget(bb);
}

Preferences PreferencesDialog::result(Preferences p) const {
  p.navigation = nav_->currentIndex() == 1 ? NavigationStyle::Cad : NavigationStyle::Blender;
  p.units = units_->currentIndex() == 1 ? LengthUnit::Inch : LengthUnit::Millimetre;
  p.showWelcome = welcome_->isChecked();
  p.checkUpdatesOnStartup = checkUpdates_->isChecked();
  p.updateChannel = channel_->currentData().toString();
  p.updateFeed = feed_->text().trimmed();
  return p;
}

} // namespace hcad::app
