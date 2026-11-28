#pragma once
#include "app/CommandRegistry.h"
#include "app/OcctViewWidget.h"
#include "app/Preferences.h"
#include "app/UpdateService.h"
#include "app/SceneSync.h"
#include "core/Units.h"
#include "document/Document.h"

#include <QMainWindow>
#include <memory>

class QFormLayout;
class QGroupBox;
class QLabel;
class QListWidget;
class QTableWidget;
class QTreeWidget;
class QPlainTextEdit;
class QCheckBox;
class QDockWidget;
class QToolButton;

namespace hcad::app {

class CommandPalette;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(const Preferences& prefs);
  ~MainWindow() override;

  CommandRegistry& commands() { return *commands_; }
  CommandPalette* palette() { return palette_; }
  UpdateService& updates() { return *updates_; }
  const Preferences& preferences() const { return prefs_; }
  void applyPreferences(const Preferences& p);

  void newEmptyProject(bool confirm = true);
  void openExampleProject();
  // Shows the first-launch welcome if enabled; returns immediately otherwise.
  void maybeShowWelcome();
  void showWelcome();

  doc::Document& document() { return doc_; }
  OcctViewWidget* viewport() { return view_; }
  SceneSync& scene() { return *scene_; }

  // Recomputes the document, rebuilds the scene and restores the selection by
  // persistent reference. Returns the number of restored selection items.
  int recomputeAndRefresh();
  bool importMeshFile(const QString& path, double unitToMm = 1.0);
  void setSelectionMode(SelectionMode m);
  QString selectionText() const;

private:
  void buildUi();
  void registerCommands();
  void buildMenus();
  void buildToolbars();
  void showShortcuts();
  void showUpdateDialog();
  void showPreferences();
  void restartToUpdate();
  void syncUpdateIndicator();
  void setUnits(LengthUnit u);
  void refreshParameterTable();
  void refreshHistory();
  void refreshProjectTree();
  void refreshProperties();
  void showFeature(const std::string& id);
  void showMesh(const std::string& id);
  void onViewportSelection();
  void onParameterEdited(int row, int column);
  void importMeshDialog();
  QString fmtLength(double mm) const;

  Preferences prefs_;
  CommandRegistry* commands_ = nullptr;
  CommandPalette* palette_ = nullptr;
  std::unique_ptr<UpdateService> updates_;
  QToolButton* updateBadge_ = nullptr;
  QDockWidget* browserDock_ = nullptr;
  QDockWidget* propsDock_ = nullptr;
  QLabel* navHint_ = nullptr;

  doc::Document doc_;
  OcctViewWidget* view_ = nullptr;
  std::unique_ptr<SceneSync> scene_;
  LengthUnit unit_ = LengthUnit::Millimetre;
  std::string currentFeature_;
  std::vector<SelectionRef> selection_;
  bool updating_ = false;

  QTableWidget* paramTable_ = nullptr;
  QListWidget* history_ = nullptr;
  QTreeWidget* project_ = nullptr;
  QGroupBox* featureBox_ = nullptr;
  QFormLayout* featureForm_ = nullptr;
  QGroupBox* selectionBox_ = nullptr;
  QLabel* selectionInfo_ = nullptr;
  QGroupBox* meshBox_ = nullptr;
  QPlainTextEdit* meshInfo_ = nullptr;

  QLabel* modeLabel_ = nullptr;
  QLabel* selLabel_ = nullptr;
  QLabel* coordLabel_ = nullptr;
  QLabel* unitLabel_ = nullptr;
  QLabel* recomputeLabel_ = nullptr;
};

} // namespace hcad::app
