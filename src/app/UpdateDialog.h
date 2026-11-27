#pragma once
#include <QDialog>

class QLabel;
class QProgressBar;
class QPushButton;

namespace hcad::app {

class UpdateService;

// Help > Check for Updates. Shows the installed version, the result of the
// check (delta or full package, size), download progress and a restart
// button. Nothing is installed without the user's click.
class UpdateDialog : public QDialog {
  Q_OBJECT
public:
  UpdateDialog(UpdateService* service, QWidget* parent);

signals:
  void restartRequested();

private:
  void sync();
  UpdateService* svc_;
  QLabel* version_;
  QLabel* status_;
  QProgressBar* progress_;
  QPushButton* check_;
  QPushButton* download_;
  QPushButton* restart_;
};

} // namespace hcad::app
