#include "app/MainWindow.h"
#include "document/Features.h"
#include "kernel/ShapeInfo.h"

#include <Standard_Version.hxx>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
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

MainWindow::MainWindow() {
  setWindowTitle("HybridCAD — Phase 0 proof of concept");
  resize(1440, 900);
  buildUi();
  buildMenus();
  doc::buildDemoDocument(doc_);

  connect(view_, &OcctViewWidget::viewReady, this, [this] {
    recomputeAndRefresh();
    QTimer::singleShot(0, view_, &OcctViewWidget::fitAll);
  });
  connect(view_, &OcctViewWidget::selectionChanged, this, &MainWindow::onViewportSelection);
  connect(view_, &OcctViewWidget::cursorModel, this, [this](double x, double y, double z) {
    coordLabel_->setText(QString("X %1  Y %2  Z %3").arg(fmtLength(x), fmtLength(y), fmtLength(z)));
  });

  QSettings s;
  restoreGeometry(s.value("ui/geometry").toByteArray());
  restoreState(s.value("ui/state").toByteArray());
  connect(qApp, &QApplication::aboutToQuit, this, [this] {
    QSettings st;
    st.setValue("ui/geometry", saveGeometry());
    st.setValue("ui/state", saveState());
  });
}

QString MainWindow::fmtLength(double mm) const {
  return QString::fromStdString(formatLength(mm, unit_, unit_ == LengthUnit::Inch ? 4 : 2));
}

void MainWindow::buildUi() {
  view_ = new OcctViewWidget(this);
  setCentralWidget(view_);
  scene_ = std::make_unique<SceneSync>(view_);

  // ---- Left toolbar: selection modes + view
  auto* tb = new QToolBar("Tools", this);
  tb->setObjectName("toolsBar");
  tb->setOrientation(Qt::Vertical);
  tb->setMovable(false);
  tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
  addToolBar(Qt::LeftToolBarArea, tb);
  auto* group = new QActionGroup(this);
  const struct { SelectionMode m; const char* label; const char* key; const char* tip; } modes[] = {
    {SelectionMode::Object, "Object", "1", "Select whole bodies and surfaces"},
    {SelectionMode::Vertex, "Vertex", "2", "Select B-Rep vertices"},
    {SelectionMode::Edge, "Edge", "3", "Select B-Rep edges"},
    {SelectionMode::Face, "Face", "4", "Select B-Rep faces"},
  };
  for (const auto& md : modes) {
    auto* a = tb->addAction(md.label);
    a->setCheckable(true);
    a->setShortcut(QKeySequence(md.key));
    a->setToolTip(QString("<b>%1 selection</b> (%2)<br>%3").arg(md.label, md.key, md.tip));
    group->addAction(a);
    const SelectionMode m = md.m;
    connect(a, &QAction::triggered, this, [this, m] { setSelectionMode(m); });
    modeActions_ << a;
  }
  modeActions_.front()->setChecked(true);
  tb->addSeparator();
  auto* fit = tb->addAction("Fit");
  fit->setShortcut(QKeySequence(Qt::Key_Home));
  fit->setToolTip("<b>Fit all</b> (Home)<br>Frame all visible geometry");
  connect(fit, &QAction::triggered, view_, &OcctViewWidget::fitAll);

  // ---- Left dock: project browser + feature history
  auto* leftTabs = new QTabWidget;
  project_ = new QTreeWidget;
  project_->setHeaderHidden(true);
  leftTabs->addTab(project_, "Project");
  history_ = new QListWidget;
  history_->setContextMenuPolicy(Qt::CustomContextMenu);
  leftTabs->addTab(history_, "History");
  auto* leftDock = new QDockWidget("Browser", this);
  leftDock->setObjectName("browserDock");
  leftDock->setWidget(leftTabs);
  addDockWidget(Qt::LeftDockWidgetArea, leftDock);
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
  auto* rightDock = new QDockWidget("Properties", this);
  rightDock->setObjectName("propertiesDock");
  rightDock->setWidget(scroll);
  rightDock->setMinimumWidth(340);
  addDockWidget(Qt::RightDockWidgetArea, rightDock);

  // ---- Status bar
  modeLabel_ = new QLabel;
  selLabel_ = new QLabel("Nothing selected");
  coordLabel_ = new QLabel;
  unitLabel_ = new QLabel("mm");
  recomputeLabel_ = new QLabel;
  auto* keys = new QLabel("MMB orbit · Shift+MMB pan · Wheel zoom · 1-4 selection mode · Home fit");
  keys->setObjectName("hint");
  statusBar()->addWidget(modeLabel_);
  statusBar()->addWidget(selLabel_, 1);
  statusBar()->addPermanentWidget(keys);
  statusBar()->addPermanentWidget(coordLabel_);
  statusBar()->addPermanentWidget(unitLabel_);
  statusBar()->addPermanentWidget(recomputeLabel_);
  modeLabel_->setText("Mode: Object");
}

void MainWindow::buildMenus() {
  auto* file = menuBar()->addMenu("&File");
  auto* imp = file->addAction("Import Mesh Reference (STL, OBJ)...", this, &MainWindow::importMeshDialog);
  imp->setShortcut(QKeySequence("Ctrl+I"));
  file->addSeparator();
  file->addAction("Quit", QKeySequence::Quit, qApp, &QApplication::quit);

  auto* view = menuBar()->addMenu("&View");
  view->addAction("Fit All", QKeySequence(Qt::Key_Home), view_, &OcctViewWidget::fitAll);
  view->addAction("Isometric", QKeySequence(Qt::Key_0), this, [this] { view_->setStandardView(V3d_XposYnegZpos); });
  view->addAction("Top", QKeySequence(Qt::Key_7), this, [this] { view_->setStandardView(V3d_Zpos); });
  view->addAction("Front", QKeySequence(Qt::Key_8), this, [this] { view_->setStandardView(V3d_Yneg); });
  view->addAction("Right", QKeySequence(Qt::Key_9), this, [this] { view_->setStandardView(V3d_Xpos); });
  view->addSeparator();
  auto* units = view->addMenu("Display Units");
  auto* ug = new QActionGroup(this);
  auto* mm = units->addAction("Millimetres");
  auto* in = units->addAction("Inches");
  for (auto* a : {mm, in}) { a->setCheckable(true); ug->addAction(a); }
  mm->setChecked(true);
  connect(mm, &QAction::triggered, this, [this] { unit_ = LengthUnit::Millimetre; unitLabel_->setText("mm"); refreshParameterTable(); refreshProperties(); });
  connect(in, &QAction::triggered, this, [this] { unit_ = LengthUnit::Inch; unitLabel_->setText("in"); refreshParameterTable(); refreshProperties(); });

  auto* help = menuBar()->addMenu("&Help");
  help->addAction("About", this, [this] {
    QMessageBox::about(this, "About", "HybridCAD — Phase 0 technical proof of concept.\n\n"
                                      "Geometry: Open CASCADE Technology " OCC_VERSION_COMPLETE "\nUI: Qt " QT_VERSION_STR);
  });
}

void MainWindow::setSelectionMode(SelectionMode m) {
  view_->setSelectionMode(m);
  modeActions_[static_cast<int>(m)]->setChecked(true);
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
