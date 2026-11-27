#include "app/MainWindow.h"
#include "app/AppPaths.h"
#include "app/CommandPalette.h"
#include "app/Icons.h"
#include "app/Logging.h"
#include "app/PreferencesDialog.h"
#include "app/UpdateDialog.h"
#include "app/Version.h"
#include "app/WelcomeDialog.h"
#include "document/Features.h"
#include "kernel/ShapeInfo.h"

#include <Standard_Version.hxx>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace hcad::app {

namespace {
QIcon statusDot(const QColor& c) {
  QPixmap pm(12, 12);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  p.setBrush(c);
  p.setPen(Qt::NoPen);
  p.drawEllipse(2, 2, 8, 8);
  return QIcon(pm);
}

QColor statusColor(doc::FeatureStatus s) {
  switch (s) {
  case doc::FeatureStatus::Ok: return QColor(110, 190, 120);
  case doc::FeatureStatus::Error: return QColor(230, 100, 90);
  case doc::FeatureStatus::Suppressed: return QColor(130, 134, 140);
  case doc::FeatureStatus::RolledBack: return QColor(80, 84, 90);
  default: return QColor(200, 180, 90);
  }
}

// "Edge(Box1.Front|Box1.Top)" -> "Box1"; "Chamfer1.Edge(...)" -> "Chamfer1".
std::string owningFeature(const std::string& ref) {
  std::string s = ref;
  for (const char* k : {"Edge(", "Vertex("}) if (s.rfind(k, 0) == 0) s = s.substr(std::string(k).size());
  return s.substr(0, s.find('.'));
}
} // namespace

MainWindow::MainWindow(const Preferences& prefs) : prefs_(prefs) {
  setWindowTitle(QString("HybridCAD %1").arg(HCAD_VERSION));
  setWindowIcon(Icons::get("app"));
  resize(1440, 900);
  unit_ = prefs_.units;
  commands_ = new CommandRegistry(this);
  updates_ = UpdateService::create(this);
  buildUi();
  registerCommands();
  buildMenus();
  buildToolbars();
  palette_ = new CommandPalette(commands_, this);
  applyPreferences(prefs_);

  connect(view_, &OcctViewWidget::viewReady, this, [this] {
    recomputeAndRefresh();
    commands_->refreshStates();
    QTimer::singleShot(0, view_, &OcctViewWidget::fitAll);
  });
  connect(view_, &OcctViewWidget::selectionChanged, this, &MainWindow::onViewportSelection);
  connect(view_, &OcctViewWidget::cursorModel, this, [this](double x, double y, double z) {
    coordLabel_->setText(QString("X %1  Y %2  Z %3").arg(fmtLength(x), fmtLength(y), fmtLength(z)));
  });
  connect(updates_.get(), &UpdateService::stateChanged, this, &MainWindow::syncUpdateIndicator);

  QSettings s;
  restoreGeometry(s.value("ui/geometry").toByteArray());
  restoreState(s.value("ui/state").toByteArray());
  connect(qApp, &QApplication::aboutToQuit, this, [this] {
    QSettings st;
    st.setValue("ui/geometry", saveGeometry());
    st.setValue("ui/state", saveState());
  });

  if (UpdateService::velopackEnabled()) {
    updates_->configure(prefs_.updateFeed.isEmpty() ? QString(HCAD_DEFAULT_UPDATE_FEED) : prefs_.updateFeed, prefs_.updateChannel);
    if (prefs_.checkUpdatesOnStartup && updates_->state() == UpdateService::State::Idle)
      QTimer::singleShot(4000, updates_.get(), &UpdateService::check);
  }
  syncUpdateIndicator();
  qInfo().noquote() << "Main window ready; data dir" << AppPaths::dataDir();
}

MainWindow::~MainWindow() = default;

void MainWindow::applyPreferences(const Preferences& p) {
  prefs_ = p;
  view_->setNavigationStyle(p.navigation);
  if (navHint_) navHint_->setText(OcctViewWidget::navigationHint(p.navigation) + " · F3 commands");
  setUnits(p.units);
}

void MainWindow::setUnits(LengthUnit u) {
  unit_ = u;
  unitLabel_->setText(u == LengthUnit::Inch ? "in" : "mm");
  refreshParameterTable();
  refreshProperties();
  commands_->refreshStates();
}

void MainWindow::newEmptyProject(bool confirm) {
  if (confirm && !doc_.features().empty()) {
    const auto r = QMessageBox::question(this, "New empty project",
      "Close the current project and start an empty one?\n\nThis alpha cannot save projects yet, so unsaved changes are lost.");
    if (r != QMessageBox::Yes) return;
  }
  doc_ = doc::Document{};
  currentFeature_.clear();
  selection_.clear();
  recomputeAndRefresh();
  statusBar()->showMessage("Empty project. Open the example project with Ctrl+E, or import a mesh reference with Ctrl+I.", 8000);
  qInfo() << "New empty project";
}

void MainWindow::openExampleProject() {
  doc_ = doc::Document{};
  doc::buildDemoDocument(doc_);
  currentFeature_.clear();
  selection_.clear();
  recomputeAndRefresh();
  view_->fitAll();
  statusBar()->showMessage("Example project opened. Try editing Width in the Parameters table.", 8000);
  qInfo() << "Opened example project";
}

void MainWindow::maybeShowWelcome() {
  if (prefs_.showWelcome || !prefs_.welcomeCompleted) showWelcome();
  else openExampleProject();
}

void MainWindow::showWelcome() {
  WelcomeDialog dlg(prefs_, this);
  if (dlg.exec() != QDialog::Accepted) {
    if (doc_.features().empty()) openExampleProject();
    return;
  }
  Preferences p = dlg.result(prefs_);
  p.save();
  applyPreferences(p);
  if (dlg.start() == WelcomeDialog::Start::Example) openExampleProject();
  else newEmptyProject(false);
}

QString MainWindow::fmtLength(double mm) const {
  return QString::fromStdString(formatLength(mm, unit_, unit_ == LengthUnit::Inch ? 4 : 2));
}

void MainWindow::buildUi() {
  view_ = new OcctViewWidget(this);
  setCentralWidget(view_);
  scene_ = std::make_unique<SceneSync>(view_);

  // ---- Left dock: project browser + feature history
  auto* leftTabs = new QTabWidget;
  project_ = new QTreeWidget;
  project_->setHeaderHidden(true);
  leftTabs->addTab(project_, "Project");
  history_ = new QListWidget;
  history_->setContextMenuPolicy(Qt::CustomContextMenu);
  leftTabs->addTab(history_, "History");
  browserDock_ = new QDockWidget("Browser", this);
  browserDock_->setObjectName("browserDock");
  browserDock_->setWidget(leftTabs);
  addDockWidget(Qt::LeftDockWidgetArea, browserDock_);
  leftTabs->setCurrentIndex(1);

  connect(history_, &QListWidget::currentRowChanged, this, [this](int row) {
    if (updating_ || row < 0 || row >= static_cast<int>(doc_.features().size())) return;
    showFeature(doc_.features()[static_cast<size_t>(row)]->id());
  });
  connect(history_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
    int row = history_->indexAt(pos).row();
    if (row < 0) return;
    QMenu menu;
    auto* f = doc_.features()[static_cast<size_t>(row)].get();
    menu.addAction(f->suppressed ? "Unsuppress" : "Suppress", this, [this, f] { f->suppressed = !f->suppressed; recomputeAndRefresh(); });
    menu.addAction("Roll back to here", this, [this, row] { doc_.setRollbackIndex(row); recomputeAndRefresh(); });
    menu.addAction("Roll to end", this, [this] { doc_.setRollbackIndex(-1); recomputeAndRefresh(); });
    menu.exec(history_->viewport()->mapToGlobal(pos));
  });
  connect(project_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item) {
    const QString id = item->data(0, Qt::UserRole).toString();
    const QString kind = item->data(0, Qt::UserRole + 1).toString();
    if (kind == "mesh") showMesh(id.toStdString());
    else if (!id.isEmpty()) showFeature(id.toStdString());
  });

  // ---- Right dock: context-sensitive properties
  auto* props = new QWidget;
  auto* pv = new QVBoxLayout(props);
  pv->setContentsMargins(6, 6, 6, 6);

  selectionBox_ = new QGroupBox("Selection");
  auto* sl = new QVBoxLayout(selectionBox_);
  selectionInfo_ = new QLabel;
  selectionInfo_->setWordWrap(true);
  selectionInfo_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  sl->addWidget(selectionInfo_);
  pv->addWidget(selectionBox_);

  featureBox_ = new QGroupBox("Feature");
  featureForm_ = new QFormLayout(featureBox_);
  pv->addWidget(featureBox_);

  meshBox_ = new QGroupBox("Mesh reference (not CAD geometry)");
  auto* ml = new QVBoxLayout(meshBox_);
  meshInfo_ = new QPlainTextEdit;
  meshInfo_->setReadOnly(true);
  meshInfo_->setMinimumHeight(220);
  ml->addWidget(meshInfo_);
  pv->addWidget(meshBox_);
  meshBox_->hide();

  auto* paramBox = new QGroupBox("Parameters");
  auto* pl = new QVBoxLayout(paramBox);
  paramTable_ = new QTableWidget(0, 3);
  paramTable_->setHorizontalHeaderLabels({"Name", "Expression", "Value"});
  paramTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  paramTable_->verticalHeader()->hide();
  paramTable_->setMinimumHeight(170);
  pl->addWidget(paramTable_);
  auto* hint = new QLabel("Edit an expression, e.g. <i>Width * 0.5</i> or <i>2 in</i>, then press Enter.");
  hint->setObjectName("hint");
  hint->setWordWrap(true);
  pl->addWidget(hint);
  pv->addWidget(paramBox);
  pv->addStretch();
  connect(paramTable_, &QTableWidget::cellChanged, this, &MainWindow::onParameterEdited);

  auto* scroll = new QScrollArea;
  scroll->setWidget(props);
  scroll->setWidgetResizable(true);
  propsDock_ = new QDockWidget("Properties", this);
  propsDock_->setObjectName("propertiesDock");
  propsDock_->setWidget(scroll);
  propsDock_->setMinimumWidth(340);
  addDockWidget(Qt::RightDockWidgetArea, propsDock_);

  // ---- Status bar
  modeLabel_ = new QLabel;
  selLabel_ = new QLabel("Nothing selected");
  coordLabel_ = new QLabel;
  unitLabel_ = new QLabel("mm");
  recomputeLabel_ = new QLabel;
  navHint_ = new QLabel;
  navHint_->setObjectName("hint");
  updateBadge_ = new QToolButton;
  updateBadge_->setIcon(Icons::get("update"));
  updateBadge_->setAutoRaise(true);
  updateBadge_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  updateBadge_->hide();
  connect(updateBadge_, &QToolButton::clicked, this, &MainWindow::showUpdateDialog);
  statusBar()->addWidget(modeLabel_);
  statusBar()->addWidget(selLabel_, 1);
  statusBar()->addPermanentWidget(updateBadge_);
  statusBar()->addPermanentWidget(navHint_);
  statusBar()->addPermanentWidget(coordLabel_);
  statusBar()->addPermanentWidget(unitLabel_);
  statusBar()->addPermanentWidget(recomputeLabel_);
  modeLabel_->setText("Mode: Object");
}

void MainWindow::registerCommands() {
  auto& c = *commands_;
  // ---- File
  c.add({"file.new", "New Empty Project", "Close the current project and start with an empty one", "File",
         QKeySequence::New, "new", false, [this] { newEmptyProject(); }});
  c.add({"file.openExample", "Open Example Project", "Open the parametric example: block, chamfer, fillet and a NURBS surface", "File",
         QKeySequence("Ctrl+E"), "example", false, [this] { openExampleProject(); }});
  c.add({"file.importMesh", "Import Mesh Reference…", "Import an STL or OBJ scan as reference data (kept separate from CAD geometry)", "File",
         QKeySequence("Ctrl+I"), "import", false, [this] { importMeshDialog(); }});
  c.add({"file.preferences", "Preferences…", "Navigation style, units, updates and data locations", "File",
         QKeySequence("Ctrl+,"), "settings", false, [this] { showPreferences(); }});
  c.add({"file.quit", "Quit", "Close HybridCAD", "File", QKeySequence::Quit, {}, false, [] { QApplication::quit(); }});

  // ---- Selection modes
  const struct { SelectionMode m; const char* id; const char* text; const char* key; const char* icon; const char* desc; } modes[] = {
    {SelectionMode::Object, "select.object", "Select Objects", "1", "select-object", "Pick whole bodies, surfaces and mesh references"},
    {SelectionMode::Vertex, "select.vertex", "Select Vertices", "2", "select-vertex", "Pick B-Rep vertices"},
    {SelectionMode::Edge, "select.edge", "Select Edges", "3", "select-edge", "Pick B-Rep edges"},
    {SelectionMode::Face, "select.face", "Select Faces", "4", "select-face", "Pick B-Rep faces"},
  };
  for (const auto& md : modes) {
    const SelectionMode m = md.m;
    CommandDef d{md.id, md.text, md.desc, "Selection", QKeySequence(md.key), md.icon, true, [this, m] { setSelectionMode(m); }};
    d.checked = [this, m] { return view_->selectionMode() == m; };
    c.add(std::move(d));
  }

  // ---- View
  c.add({"view.fitAll", "Fit All", "Frame all visible geometry", "View", QKeySequence(Qt::Key_Home), "fit", false, [this] { view_->fitAll(); }});
  const struct { const char* id; const char* text; const char* key; const char* icon; V3d_TypeOfOrientation o; } views[] = {
    {"view.iso", "Isometric View", "0", "view-iso", V3d_XposYnegZpos},
    {"view.top", "Top View", "7", "view-top", V3d_Zpos},
    {"view.bottom", "Bottom View", "Ctrl+7", "", V3d_Zneg},
    {"view.front", "Front View", "8", "view-front", V3d_Yneg},
    {"view.back", "Back View", "Ctrl+8", "", V3d_Ypos},
    {"view.right", "Right View", "9", "view-right", V3d_Xpos},
    {"view.left", "Left View", "Ctrl+9", "", V3d_Xneg},
  };
  for (const auto& vw : views) {
    const V3d_TypeOfOrientation o = vw.o;
    c.add({vw.id, vw.text, QString("Look at the model from the %1 and fit it to the window").arg(QString(vw.text).section(' ', 0, 0).toLower()),
           "View", QKeySequence(vw.key), vw.icon, false, [this, o] { view_->setStandardView(o); }});
  }
  CommandDef persp{"view.perspective", "Perspective Projection", "Switch between perspective and orthographic projection", "View",
                   QKeySequence("5"), "perspective", true, [this] { view_->setPerspective(!view_->perspective()); }};
  persp.checked = [this] { return view_->perspective(); };
  c.add(std::move(persp));
  CommandDef grid{"view.grid", "Show Grid", "Show the 10 mm construction grid on the XY plane", "View",
                  QKeySequence("Shift+G"), "grid", true, [this] { view_->setGridVisible(!view_->gridVisible()); }};
  grid.checked = [this] { return view_->gridVisible(); };
  c.add(std::move(grid));
  CommandDef axes{"view.axes", "Show Axes", "Show the X/Y/Z axis indicator in the lower-left corner", "View",
                  QKeySequence("Shift+A"), "axes", true, [this] { view_->setAxesVisible(!view_->axesVisible()); }};
  axes.checked = [this] { return view_->axesVisible(); };
  c.add(std::move(axes));
  CommandDef cube{"view.viewCube", "Show View Cube", "Show the orientation cube; click its faces, edges or corners to change the view", "View",
                  QKeySequence("Shift+C"), "cube", true, [this] { view_->setViewCubeVisible(!view_->viewCubeVisible()); }};
  cube.checked = [this] { return view_->viewCubeVisible(); };
  c.add(std::move(cube));
  CommandDef mm{"view.unitsMm", "Display Millimetres", "Show lengths in millimetres", "View", {}, {}, true, [this] { setUnits(LengthUnit::Millimetre); }};
  mm.checked = [this] { return unit_ == LengthUnit::Millimetre; };
  c.add(std::move(mm));
  CommandDef in{"view.unitsIn", "Display Inches", "Show lengths in inches", "View", {}, {}, true, [this] { setUnits(LengthUnit::Inch); }};
  in.checked = [this] { return unit_ == LengthUnit::Inch; };
  c.add(std::move(in));
  CommandDef pb{"view.browserPanel", "Browser Panel", "Show or hide the project browser and feature history", "Window",
                QKeySequence("Ctrl+1"), "history", true, [this] { browserDock_->setVisible(!browserDock_->isVisible()); }};
  pb.checked = [this] { return browserDock_->isVisible(); };
  c.add(std::move(pb));
  CommandDef pp{"view.propertiesPanel", "Properties Panel", "Show or hide feature properties and the parameter table", "Window",
                QKeySequence("Ctrl+2"), "parameters", true, [this] { propsDock_->setVisible(!propsDock_->isVisible()); }};
  pp.checked = [this] { return propsDock_->isVisible(); };
  c.add(std::move(pp));

  // ---- Help
  c.add({"help.commandSearch", "Search Commands…", "Find and run any command by typing part of its name", "Help",
         QKeySequence(Qt::Key_F3), "search", false, [this] { palette_->popup(); }});
  c.action("help.commandSearch")->setShortcuts({QKeySequence(Qt::Key_F3), QKeySequence("Ctrl+K")});
  c.add({"help.welcome", "Welcome Screen", "Show the welcome screen: navigation style, units and first steps", "Help",
         {}, "welcome", false, [this] { showWelcome(); }});
  c.add({"help.shortcuts", "Keyboard Shortcuts", "List every command with its shortcut", "Help",
         QKeySequence("Ctrl+F1"), "keyboard", false, [this] { showShortcuts(); }});
  c.add({"help.checkUpdates", "Check for Updates…", "Check for a newer HybridCAD version and install it in place", "Help",
         {}, "update", false, [this] { showUpdateDialog(); updates_->check(); }});
  c.add({"help.openLogs", "Open Log Folder", "Open the folder with HybridCAD's log files (useful for bug reports)", "Help",
         {}, "logs", false, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::logsDir())); }});
  c.add({"help.about", "About HybridCAD", "Version, geometry kernel and licence information", "Help", {}, "info", false, [this] {
    QMessageBox::about(this, "About HybridCAD",
      QString("<b>HybridCAD %1</b><br>Early alpha.<br><br>Geometry kernel: Open CASCADE Technology %2<br>UI: Qt %3<br><br>"
              "Third-party licences are listed in docs/LICENCES.md.").arg(HCAD_VERSION, OCC_VERSION_COMPLETE, QT_VERSION_STR));
  }});
}

void MainWindow::buildMenus() {
  auto& c = *commands_;
  auto addAll = [&c](QMenu* m, std::initializer_list<const char*> ids) {
    for (const char* id : ids) {
      if (!*id) m->addSeparator();
      else m->addAction(c.action(id));
    }
  };
  addAll(menuBar()->addMenu("&File"), {"file.new", "file.openExample", "", "file.importMesh", "", "file.preferences", "", "file.quit"});
  auto* select = menuBar()->addMenu("&Select");
  auto* modeGroup = new QActionGroup(this);
  for (const char* id : {"select.object", "select.vertex", "select.edge", "select.face"}) {
    modeGroup->addAction(c.action(id));
    select->addAction(c.action(id));
  }
  auto* view = menuBar()->addMenu("&View");
  addAll(view, {"view.fitAll", "", "view.iso", "view.top", "view.bottom", "view.front", "view.back", "view.right", "view.left", "",
                "view.perspective", "view.grid", "view.axes", "view.viewCube"});
  auto* units = view->addMenu("Display Units");
  auto* unitGroup = new QActionGroup(this);
  for (const char* id : {"view.unitsMm", "view.unitsIn"}) {
    unitGroup->addAction(c.action(id));
    units->addAction(c.action(id));
  }
  addAll(menuBar()->addMenu("&Window"), {"view.browserPanel", "view.propertiesPanel"});
  addAll(menuBar()->addMenu("&Help"), {"help.commandSearch", "help.shortcuts", "help.welcome", "", "help.checkUpdates", "help.openLogs", "",
                                       "help.about"});
  // Keep check marks in sync when docks are closed with their own buttons.
  for (auto* d : {browserDock_, propsDock_})
    connect(d, &QDockWidget::visibilityChanged, commands_, &CommandRegistry::refreshStates);
}

void MainWindow::buildToolbars() {
  auto& c = *commands_;
  auto* tb = new QToolBar("Selection and View", this);
  tb->setObjectName("toolsBar");
  tb->setOrientation(Qt::Vertical);
  tb->setMovable(false);
  tb->setIconSize(QSize(24, 24));
  tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
  addToolBar(Qt::LeftToolBarArea, tb);
  for (const char* id : {"select.object", "select.vertex", "select.edge", "select.face", "", "view.fitAll", "view.iso", "view.top",
                         "view.front", "view.right", "", "view.grid", "view.perspective", "", "help.commandSearch"}) {
    if (!*id) tb->addSeparator();
    else tb->addAction(c.action(id));
  }
}

void MainWindow::showShortcuts() {
  QDialog dlg(this);
  dlg.setWindowTitle("Keyboard Shortcuts");
  dlg.resize(640, 560);
  auto* v = new QVBoxLayout(&dlg);
  auto* table = new QTableWidget(0, 3);
  table->setHorizontalHeaderLabels({"Command", "Shortcut", "Category"});
  table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table->verticalHeader()->hide();
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  for (const auto& d : commands_->all()) {
    QStringList keys;
    for (const auto& k : commands_->action(d.id)->shortcuts()) keys << k.toString(QKeySequence::NativeText);
    const int r = table->rowCount();
    table->insertRow(r);
    auto* name = new QTableWidgetItem(commands_->action(d.id)->icon(), d.text);
    name->setToolTip(d.description);
    table->setItem(r, 0, name);
    table->setItem(r, 1, new QTableWidgetItem(keys.join("  or  ")));
    table->setItem(r, 2, new QTableWidgetItem(d.category));
  }
  table->resizeColumnsToContents();
  table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  v->addWidget(table);
  auto* nav = new QLabel("Viewport: " + OcctViewWidget::navigationHint(prefs_.navigation) + ". Alt + left mouse emulates the middle button.");
  nav->setWordWrap(true);
  nav->setObjectName("hint");
  v->addWidget(nav);
  auto* bb = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  v->addWidget(bb);
  dlg.exec();
}

void MainWindow::showPreferences() {
  PreferencesDialog dlg(prefs_, this);
  if (dlg.exec() != QDialog::Accepted) return;
  Preferences p = dlg.result(prefs_);
  const bool feedChanged = p.updateFeed != prefs_.updateFeed || p.updateChannel != prefs_.updateChannel;
  p.save();
  applyPreferences(p);
  if (feedChanged && UpdateService::velopackEnabled())
    updates_->configure(p.updateFeed.isEmpty() ? QString(HCAD_DEFAULT_UPDATE_FEED) : p.updateFeed, p.updateChannel);
}

void MainWindow::showUpdateDialog() {
  auto* dlg = findChild<UpdateDialog*>();
  if (!dlg) {
    dlg = new UpdateDialog(updates_.get(), this);
    connect(dlg, &UpdateDialog::restartRequested, this, &MainWindow::restartToUpdate);
  }
  dlg->show();
  dlg->raise();
}

void MainWindow::restartToUpdate() {
  qInfo() << "Restarting to apply update" << updates_->availableVersion();
  if (updates_->applyAndRestart(true)) QApplication::quit();
}

void MainWindow::syncUpdateIndicator() {
  using S = UpdateService::State;
  const S s = updates_->state();
  if (s == S::Available) updateBadge_->setText(QString("Update %1 available").arg(updates_->availableVersion()));
  else if (s == S::ReadyToRestart) updateBadge_->setText("Restart to update");
  updateBadge_->setVisible(s == S::Available || s == S::ReadyToRestart);
}

void MainWindow::setSelectionMode(SelectionMode m) {
  view_->setSelectionMode(m);
  commands_->refreshStates();
  modeLabel_->setText(QString("Mode: %1").arg(toString(m)));
}

int MainWindow::recomputeAndRefresh() {
  if (view_->isReady()) selection_ = scene_->currentSelection(doc_);
  auto rep = doc_.recompute();
  int restored = 0;
  if (view_->isReady()) {
    scene_->rebuild(doc_);
    restored = scene_->restoreSelection(doc_, selection_);
  }
  recomputeLabel_->setText(rep.failed == 0
    ? QString("Recomputed %1 feature(s) in %2 ms").arg(rep.computed).arg(rep.totalMs, 0, 'f', 1)
    : QString("<span style='color:#ff7a6a'>%1 feature(s) failed</span>").arg(rep.failed));
  refreshParameterTable();
  refreshHistory();
  refreshProjectTree();
  onViewportSelection();
  return restored;
}

void MainWindow::refreshParameterTable() {
  updating_ = true;
  const auto& params = doc_.parameters().parameters();
  paramTable_->setRowCount(static_cast<int>(params.size()));
  for (int i = 0; i < static_cast<int>(params.size()); ++i) {
    const auto& p = params[static_cast<size_t>(i)];
    auto* name = new QTableWidgetItem(QString::fromStdString(p.name));
    name->setFlags(name->flags() & ~Qt::ItemIsEditable);
    name->setToolTip(QString::fromStdString(p.comment));
    auto* expr = new QTableWidgetItem(QString::fromStdString(p.expression));
    auto v = doc_.parameters().value(p.name);
    auto* val = new QTableWidgetItem(v ? fmtLength(*v) : QString::fromStdString(doc_.parameters().error(p.name)));
    val->setFlags(val->flags() & ~Qt::ItemIsEditable);
    if (!v) val->setForeground(QColor(255, 122, 106));
    paramTable_->setItem(i, 0, name);
    paramTable_->setItem(i, 1, expr);
    paramTable_->setItem(i, 2, val);
  }
  updating_ = false;
}

void MainWindow::onParameterEdited(int row, int column) {
  if (updating_ || column != 1) return;
  const auto& p = doc_.parameters().parameters()[static_cast<size_t>(row)];
  doc_.parameters().setExpression(p.name, paramTable_->item(row, 1)->text().trimmed().toStdString());
  recomputeAndRefresh();
}

void MainWindow::refreshHistory() {
  updating_ = true;
  const int keep = history_->currentRow();
  history_->clear();
  for (const auto& f : doc_.features()) {
    auto* it = new QListWidgetItem(statusDot(statusColor(f->status)),
                                   QString("%1   %2").arg(QString::fromStdString(f->id()), QString::fromStdString(f->summary())));
    QString tip = QString("<b>%1</b> — %2").arg(QString::fromStdString(f->type()), toString(f->status));
    if (!f->error.empty()) tip += "<br><span style='color:#ff7a6a'>" + QString::fromStdString(f->error).toHtmlEscaped() + "</span>";
    it->setToolTip(tip);
    if (f->status == doc::FeatureStatus::RolledBack || f->status == doc::FeatureStatus::Suppressed) it->setForeground(QColor(120, 124, 130));
    history_->addItem(it);
  }
  if (keep >= 0 && keep < history_->count()) history_->setCurrentRow(keep);
  updating_ = false;
}

void MainWindow::refreshProjectTree() {
  project_->clear();
  auto* solids = new QTreeWidgetItem(project_, {"Solids"});
  auto* surfaces = new QTreeWidgetItem(project_, {"Surfaces"});
  auto* meshes = new QTreeWidgetItem(project_, {"Mesh References"});
  for (const doc::Feature* f : doc_.tipFeatures()) {
    if (f->result.body.isNull()) continue;
    auto* parent = f->result.nurbs ? surfaces : solids;
    auto* it = new QTreeWidgetItem(parent, {QString::fromStdString(f->id())});
    it->setData(0, Qt::UserRole, QString::fromStdString(f->id()));
    it->setToolTip(0, QString::fromStdString(f->summary()));
  }
  for (const auto& m : doc_.meshes()) {
    auto* it = new QTreeWidgetItem(meshes, {QString("%1 (%2)").arg(QString::fromStdString(m.mesh.name), QString::fromStdString(m.id))});
    it->setData(0, Qt::UserRole, QString::fromStdString(m.id));
    it->setData(0, Qt::UserRole + 1, "mesh");
  }
  project_->expandAll();
}

void MainWindow::showFeature(const std::string& id) {
  currentFeature_ = id;
  meshBox_->hide();
  featureBox_->show();
  refreshProperties();
}

void MainWindow::showMesh(const std::string& id) {
  for (const auto& m : doc_.meshes()) {
    if (m.id != id) continue;
    featureBox_->hide();
    meshBox_->show();
    meshInfo_->setPlainText(QString("Source: %1\nFormat: %2\n\n%3\n\nThis is polygon reference data. It is kept unchanged and is not an "
                                    "editable CAD solid. Reconstruction to CAD is planned for Phase 6.")
                              .arg(QFileInfo(QString::fromStdString(m.sourcePath)).fileName(), QString::fromStdString(m.format),
                                   QString::fromStdString(m.report.summary())));
  }
}

void MainWindow::refreshProperties() {
  while (featureForm_->rowCount() > 0) featureForm_->removeRow(0);
  doc::Feature* f = doc_.feature(currentFeature_);
  if (!f) {
    featureBox_->setTitle("Feature");
    featureForm_->addRow(new QLabel("Select a feature in History, or a face/edge in the viewport."));
    return;
  }
  featureBox_->setTitle(QString("%1 — %2").arg(QString::fromStdString(f->id()), QString::fromStdString(f->type())));
  for (auto& p : f->params()) {
    auto* edit = new QLineEdit(QString::fromStdString(p.expression));
    edit->setToolTip(p.error.empty() ? QString("= %1").arg(p.kind == doc::ParamKind::Length ? fmtLength(p.value) : QString::number(p.value))
                                     : QString::fromStdString(p.error));
    const std::string key = p.key;
    connect(edit, &QLineEdit::editingFinished, this, [this, f, key, edit] {
      auto* prm = f->param(key);
      const std::string e = edit->text().trimmed().toStdString();
      if (!prm || prm->expression == e) return;
      prm->expression = e;
      QMetaObject::invokeMethod(this, [this] { recomputeAndRefresh(); refreshProperties(); }, Qt::QueuedConnection);
    });
    auto* row = new QWidget;
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->addWidget(edit, 1);
    auto* val = new QLabel(p.kind == doc::ParamKind::Length ? fmtLength(p.value) : QString::number(p.value));
    val->setObjectName("hint");
    val->setMinimumWidth(80);
    h->addWidget(val);
    featureForm_->addRow(QString::fromStdString(p.label), row);
  }
  if (auto* blend = dynamic_cast<doc::EdgeBlendFeature*>(f)) {
    QStringList refs;
    for (const auto& r : blend->edgeRefs) refs << QString::fromStdString(r);
    auto* l = new QLabel(refs.join("\n"));
    l->setWordWrap(true);
    l->setToolTip("Persistent edge references (named by adjacent faces, not by index)");
    featureForm_->addRow("Edges", l);
  }
  auto* sup = new QCheckBox("Suppressed");
  sup->setChecked(f->suppressed);
  connect(sup, &QCheckBox::toggled, this, [this, f](bool on) {
    f->suppressed = on;
    QMetaObject::invokeMethod(this, [this] { recomputeAndRefresh(); refreshProperties(); }, Qt::QueuedConnection);
  });
  featureForm_->addRow(sup);
  auto* st = new QLabel(QString("Status: %1  (%2 ms)").arg(toString(f->status)).arg(f->lastComputeMs, 0, 'f', 1));
  featureForm_->addRow(st);
  if (!f->error.empty()) {
    auto* err = new QLabel(QString::fromStdString(f->error));
    err->setObjectName("error");
    err->setWordWrap(true);
    featureForm_->addRow(err);
  }
  if (!f->result.body.isNull()) {
    auto* info = new QLabel(QString::fromStdString(kernel::describe(kernel::shapeStats(f->result.body.shape()))));
    info->setObjectName("hint");
    featureForm_->addRow(info);
  }
}

QString MainWindow::selectionText() const {
  QStringList parts;
  for (const auto& r : selection_)
    parts << QString::fromStdString(r.reference.empty() ? r.featureId : r.featureId + " : " + r.reference);
  return parts.join(", ");
}

void MainWindow::onViewportSelection() {
  if (!view_->isReady()) return;
  selection_ = scene_->currentSelection(doc_);
  if (selection_.empty()) {
    selLabel_->setText("Nothing selected");
    selectionInfo_->setText(QString("<span style='color:#8a8e94'>Click geometry in the viewport. Current mode: <b>%1</b>.</span>")
                              .arg(toString(view_->selectionMode())));
    refreshProperties();
    return;
  }
  selLabel_->setText(QString("Selected: %1").arg(selectionText()));
  QString html;
  for (const auto& r : selection_) {
    html += QString("<b>%1</b><br>").arg(QString::fromStdString(r.featureId));
    if (!r.reference.empty())
      html += QString("Persistent reference:<br><code>%1</code><br>").arg(QString::fromStdString(r.reference).toHtmlEscaped());
  }
  selectionInfo_->setText(html);
  // Context-sensitive: show the feature that created the selected element.
  const auto& r = selection_.front();
  std::string owner = r.reference.empty() ? r.featureId : owningFeature(r.reference);
  if (!doc_.feature(owner)) owner = r.featureId;
  currentFeature_ = owner;
  meshBox_->hide();
  featureBox_->show();
  refreshProperties();
}

bool MainWindow::importMeshFile(const QString& path, double unitToMm) {
  std::string err = doc_.importMesh(path.toStdString(), unitToMm);
  if (!err.empty()) {
    QMessageBox::warning(this, "Mesh import failed", QString::fromStdString(err));
    return false;
  }
  scene_->rebuild(doc_);
  view_->setSelectionMode(view_->selectionMode());
  refreshProjectTree();
  showMesh(doc_.meshes().back().id);
  statusBar()->showMessage(QString("Imported %1 as mesh reference").arg(QFileInfo(path).fileName()), 5000);
  return true;
}

void MainWindow::importMeshDialog() {
  const QString path = QFileDialog::getOpenFileName(this, "Import mesh reference", {}, "Meshes (*.stl *.obj);;All files (*)");
  if (path.isEmpty()) return;
  bool ok = false;
  const QStringList units{"Millimetres", "Centimetres", "Metres", "Inches"};
  const QString u = QInputDialog::getItem(this, "Mesh units", "STL/OBJ files carry no units. The file is in:", units, 0, false, &ok);
  if (!ok) return;
  const double f[] = {1.0, 10.0, 1000.0, 25.4};
  importMeshFile(path, f[units.indexOf(u)]);
}

} // namespace hcad::app
