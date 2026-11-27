#include "app/UpdateDialog.h"
#include "app/Icons.h"
#include "app/UpdateService.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace hcad::app {

UpdateDialog::UpdateDialog(UpdateService* svc, QWidget* parent) : QDialog(parent), svc_(svc) {
  setWindowTitle("Software Update");
  setMinimumWidth(480);
  auto* v = new QVBoxLayout(this);
  auto* head = new QHBoxLayout;
  auto* icon = new QLabel;
  icon->setPixmap(Icons::get("update").pixmap(32, 32));
  head->addWidget(icon);
  version_ = new QLabel;
  head->addWidget(version_, 1);
  v->addLayout(head);
  status_ = new QLabel;
  status_->setWordWrap(true);
  status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  v->addWidget(status_);
  progress_ = new QProgressBar;
  progress_->setRange(0, 100);
  v->addWidget(progress_);
  auto* note = new QLabel("Updates install over the existing program. Your projects, settings and logs are kept.");
  note->setObjectName("hint");
  note->setWordWrap(true);
  v->addWidget(note);

  auto* b = new QHBoxLayout;
  check_ = new QPushButton("Check Now");
  download_ = new QPushButton("Download");
  restart_ = new QPushButton("Restart and Install");
  auto* close = new QPushButton("Close");
  b->addWidget(check_);
  b->addStretch();
  b->addWidget(download_);
  b->addWidget(restart_);
  b->addWidget(close);
  v->addLayout(b);

  connect(check_, &QPushButton::clicked, svc_, &UpdateService::check);
  connect(download_, &QPushButton::clicked, svc_, &UpdateService::download);
  connect(restart_, &QPushButton::clicked, this, &UpdateDialog::restartRequested);
  connect(close, &QPushButton::clicked, this, &QDialog::accept);
  connect(svc_, &UpdateService::stateChanged, this, &UpdateDialog::sync);
  connect(svc_, &UpdateService::progressChanged, progress_, &QProgressBar::setValue);
  sync();
}

void UpdateDialog::sync() {
  using S = UpdateService::State;
  const S s = svc_->state();
  version_->setText(QString("<b>HybridCAD %1</b><br><span style='color:#8a8e94'>installed version</span>").arg(svc_->currentVersion()));
  status_->setText(svc_->message().toHtmlEscaped());
  status_->setObjectName(s == S::Error ? "error" : "");
  status_->style()->unpolish(status_);
  status_->style()->polish(status_);
  progress_->setVisible(s == S::Downloading || s == S::ReadyToRestart);
  if (s == S::ReadyToRestart) progress_->setValue(100);
  check_->setEnabled(s != S::Unsupported && s != S::Checking && s != S::Downloading);
  download_->setVisible(s == S::Available);
  restart_->setVisible(s == S::ReadyToRestart);
}

} // namespace hcad::app
