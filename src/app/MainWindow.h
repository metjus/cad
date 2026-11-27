#pragma once
#include "app/OcctViewWidget.h"
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

namespace hcad::app {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();

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
  void buildMenus();
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
  QList<QAction*> modeActions_;
};

} // namespace hcad::app
