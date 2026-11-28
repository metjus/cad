#include "app/UpdateService.h"
#include "app/Version.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QPointer>
#include <thread>

#ifdef HCAD_WITH_VELOPACK
#include "Velopack.h"
#endif

namespace hcad::app {

QString UpdateService::toString(State s) {
  switch (s) {
  case State::Unsupported: return "Updates unavailable in this build";
  case State::Idle: return "Not checked yet";
  case State::Checking: return "Checking for updates…";
  case State::UpToDate: return "HybridCAD is up to date";
  case State::Available: return "Update available";
  case State::Downloading: return "Downloading update…";
  case State::ReadyToRestart: return "Update ready — restart to install";
  case State::Error: return "Update failed";
  }
  return {};
}

void UpdateService::setState(State s, const QString& msg) {
  state_ = s;
  message_ = msg.isEmpty() ? toString(s) : msg;
  qInfo().noquote() << "[update]" << toString(s) << (msg.isEmpty() ? "" : "-") << msg;
  emit stateChanged(s);
}

namespace {

// ---------------------------------------------------------------- no updater
class NullUpdateService final : public UpdateService {
public:
  explicit NullUpdateService(QObject* p) : UpdateService(p) {
    current_ = HCAD_VERSION;
    state_ = State::Unsupported;
    message_ = "In-app updates are available in installed Windows builds only.";
  }
  void configure(const QString&, const QString&) override {}
  void check() override { setState(State::Unsupported, message_); }
  void download() override {}
  bool applyAndRestart(bool, const QStringList&) override { return false; }
};

#ifdef HCAD_WITH_VELOPACK
QString lastError() {
  char buf[2048] = {};
  vpkc_get_last_error(buf, sizeof buf);
  return QString::fromUtf8(buf);
}

// ---------------------------------------------------------------- Velopack
class VelopackUpdateService final : public UpdateService {
public:
  explicit VelopackUpdateService(QObject* p) : UpdateService(p) { current_ = HCAD_VERSION; }
  ~VelopackUpdateService() override {
    if (worker_.joinable()) worker_.join();
    if (info_) vpkc_free_update_info(info_);
    if (mgr_) vpkc_free_update_manager(mgr_);
    if (source_) vpkc_free_source(source_);
  }

  void configure(const QString& feed, const QString& channel) override {
    if (mgr_) { vpkc_free_update_manager(mgr_); mgr_ = nullptr; }
    // Velopack layout: <root>/Update.exe and <root>/current/HybridCAD.exe.
    // A build folder or portable copy cannot update itself; do not touch the
    // Velopack library at all in that case.
    if (!QFile::exists(QCoreApplication::applicationDirPath() + "/../Update.exe")) {
      setState(State::Unsupported, "Updates need an installed copy of HybridCAD (this copy runs from a build or portable folder).");
      return;
    }
    feed_ = feed.toUtf8();
    channel_ = channel.toUtf8();
    vpkc_update_options_t opt{};
    opt.AllowVersionDowngrade = false;
    opt.ExplicitChannel = channel_.isEmpty() ? nullptr : channel_.data();
    opt.MaximumDeltasBeforeFallback = 10;
    // A bare GitHub repository URL uses GitHub Releases (including
    // pre-releases, which all alpha builds are); anything else is an HTTPS
    // feed URL or a local folder.
    bool ok = false;
    static const QRegularExpression githubRepo("^https://github\\.com/[^/]+/[^/]+/?$");
    if (githubRepo.match(feed).hasMatch()) {
      if (!source_) source_ = vpkc_new_source_github(feed_.constData(), nullptr, true);
      ok = source_ && vpkc_new_update_manager_with_source(source_, &opt, nullptr, &mgr_);
    } else {
      ok = vpkc_new_update_manager(feed_.constData(), &opt, nullptr, &mgr_);
    }
    if (!ok) {
      mgr_ = nullptr;
      // Typical cause: running from a build folder, not an installed copy.
      setState(State::Unsupported, "Updates need an installed copy of HybridCAD (" + lastError() + ")");
      return;
    }
    char ver[128] = {};
    vpkc_get_current_version(mgr_, ver, sizeof ver);
    current_ = QString::fromUtf8(ver);
    vpkc_asset_t* pending = nullptr;
    if (vpkc_update_pending_restart(mgr_, &pending) && pending) {
      available_ = QString::fromUtf8(pending->Version);
      vpkc_free_asset(pending);
      setState(State::ReadyToRestart);
    } else {
      setState(State::Idle);
    }
  }

  void check() override {
    if (!mgr_ || busy()) return;
    setState(State::Checking);
    runAsync([this] {
      vpkc_update_info_t* info = nullptr;
      const auto r = vpkc_check_for_updates(mgr_, &info);
      QString err = r == UPDATE_ERROR ? lastError() : QString();
      post([this, r, info, err] {
        if (info_) vpkc_free_update_info(info_);
        info_ = info;
        if (r == UPDATE_AVAILABLE && info_) {
          available_ = QString::fromUtf8(info_->TargetFullRelease->Version);
          notes_ = QString::fromUtf8(info_->TargetFullRelease->NotesMarkdown ? info_->TargetFullRelease->NotesMarkdown : "");
          delta_ = info_->DeltasToTargetCount > 0;
          bytes_ = 0;
          if (delta_) for (size_t i = 0; i < info_->DeltasToTargetCount; ++i) bytes_ += info_->DeltasToTarget[i]->Size;
          else bytes_ = info_->TargetFullRelease->Size;
          setState(State::Available, QString("Version %1 is available (%2, %3 MB)")
                                       .arg(available_, delta_ ? "delta update" : "full package")
                                       .arg(bytes_ / 1048576.0, 0, 'f', 1));
        } else if (r == UPDATE_ERROR) {
          setState(State::Error, "Could not check for updates: " + err);
        } else {
          setState(State::UpToDate);
        }
      });
    });
  }

  void download() override {
    if (!mgr_ || !info_ || busy()) return;
    setState(State::Downloading);
    runAsync([this] {
      const bool ok = vpkc_download_updates(mgr_, info_, &VelopackUpdateService::onProgress, this);
      QString err = ok ? QString() : lastError();
      post([this, ok, err] {
        if (ok) setState(State::ReadyToRestart);
        else setState(State::Error, "Download failed, nothing was changed: " + err);
      });
    });
  }

  bool applyAndRestart(bool restart, const QStringList& args) override {
    if (!mgr_) return false;
    vpkc_asset_t* asset = nullptr;
    bool own = false;
    if (info_) asset = info_->TargetFullRelease;
    else if (vpkc_update_pending_restart(mgr_, &asset)) own = true;
    if (!asset) return false;
    std::vector<QByteArray> store;
    std::vector<char*> argv;
    for (const auto& a : args) { store.push_back(a.toUtf8()); }
    for (auto& s : store) argv.push_back(s.data());
    const bool ok = vpkc_wait_exit_then_apply_updates(mgr_, asset, true, restart, argv.empty() ? nullptr : argv.data(), argv.size());
    if (own) vpkc_free_asset(asset);
    if (!ok) setState(State::Error, "Could not start the updater: " + lastError());
    return ok;
  }

private:
  bool busy() const { return state_ == State::Checking || state_ == State::Downloading; }
  template <class F> void runAsync(F f) {
    if (worker_.joinable()) worker_.join();
    worker_ = std::thread(std::move(f));
  }
  template <class F> void post(F f) { QMetaObject::invokeMethod(this, std::move(f), Qt::QueuedConnection); }
  static void onProgress(void* self, size_t pct) {
    auto* s = static_cast<VelopackUpdateService*>(self);
    QMetaObject::invokeMethod(s, [s, pct] { emit s->progressChanged(static_cast<int>(pct)); }, Qt::QueuedConnection);
  }

  vpkc_update_manager_t* mgr_ = nullptr;
  vpkc_update_source_t* source_ = nullptr;
  vpkc_update_info_t* info_ = nullptr;
  QByteArray feed_, channel_;
  std::thread worker_;
};
#endif

} // namespace

std::unique_ptr<UpdateService> UpdateService::create(QObject* parent) {
#ifdef HCAD_WITH_VELOPACK
  return std::make_unique<VelopackUpdateService>(parent);
#else
  return std::make_unique<NullUpdateService>(parent);
#endif
}

bool UpdateService::velopackEnabled() {
#ifdef HCAD_WITH_VELOPACK
  return true;
#else
  return false;
#endif
}

void UpdateService::runStartupHooks(int argc, char** argv) {
#ifdef HCAD_WITH_VELOPACK
  vpkc_app_set_args(argv, static_cast<size_t>(argc));
  vpkc_app_run(nullptr);
#else
  (void)argc;
  (void)argv;
#endif
}

} // namespace hcad::app
