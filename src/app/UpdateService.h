#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace hcad::app {

// In-app updates (brief §15). Production builds on Windows use Velopack:
// the app checks a release feed over HTTPS, downloads a delta package when
// one exists (full package otherwise), verifies its SHA-256, and applies it on
// restart. The previous version keeps running if anything fails. Builds
// without Velopack (developer/Linux builds) report "Unsupported".
class UpdateService : public QObject {
  Q_OBJECT
public:
  enum class State { Unsupported, Idle, Checking, UpToDate, Available, Downloading, ReadyToRestart, Error };
  Q_ENUM(State)

  static std::unique_ptr<UpdateService> create(QObject* parent = nullptr);
  // Must run first thing in main(): handles installer/uninstaller hooks and
  // may exit or restart the process (no-op without Velopack).
  static void runStartupHooks(int argc, char** argv);
  static bool velopackEnabled();

  State state() const { return state_; }
  QString message() const { return message_; }
  QString currentVersion() const { return current_; }
  QString availableVersion() const { return available_; }
  bool deltaAvailable() const { return delta_; }
  quint64 downloadBytes() const { return bytes_; }
  QString releaseNotes() const { return notes_; }

  // Feed: https URL or local directory. Channel: "stable" or "beta".
  virtual void configure(const QString& feed, const QString& channel) = 0;
  virtual void check() = 0;          // async
  virtual void download() = 0;       // async, after Available
  // Launches the updater, which waits for this process to exit, applies the
  // update and (optionally) restarts with `restartArgs`. Returns false if it
  // could not be started; the caller must quit the app on success.
  virtual bool applyAndRestart(bool restart = true, const QStringList& restartArgs = {}) = 0;

  static QString toString(State s);

signals:
  void stateChanged(hcad::app::UpdateService::State state);
  void progressChanged(int percent);

protected:
  explicit UpdateService(QObject* parent) : QObject(parent) {}
  void setState(State s, const QString& msg = {});

  State state_ = State::Idle;
  QString message_, current_, available_, notes_;
  bool delta_ = false;
  quint64 bytes_ = 0;
};

} // namespace hcad::app
