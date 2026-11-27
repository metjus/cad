#include "app/UpdateTest.h"
#include "app/AppPaths.h"
#include "app/UpdateService.h"
#include "app/Version.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>
#include <cstdio>

namespace hcad::app {

int runUpdateTest(QApplication& app, const QString& feed) {
  QSettings s;
  const QString marker = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  s.setValue("test/updateMarker", marker);
  s.sync();
  QFile::remove(AppPaths::dataDir() + "/update-test.json");
  std::printf("update-test: running %s, marker %s, feed %s\n", HCAD_VERSION, qPrintable(marker), qPrintable(feed));

  auto svc = UpdateService::create();
  int rc = 2;
  using S = UpdateService::State;
  QObject::connect(svc.get(), &UpdateService::stateChanged, &app, [&](S st) {
    std::printf("update-test: %s\n", qPrintable(svc->message()));
    std::fflush(stdout);
    switch (st) {
    case S::Available:
      std::printf("update-test: %s -> %s, delta=%d, bytes=%llu\n", qPrintable(svc->currentVersion()), qPrintable(svc->availableVersion()),
                  svc->deltaAvailable() ? 1 : 0, static_cast<unsigned long long>(svc->downloadBytes()));
      svc->download();
      break;
    case S::ReadyToRestart: {
      const QString report = AppPaths::dataDir() + "/update-test.json";
      rc = svc->applyAndRestart(true, {"--update-test-verify", report}) ? 0 : 3;
      app.quit();
      break;
    }
    case S::UpToDate:
    case S::Error:
    case S::Unsupported:
      rc = 4;
      app.quit();
      break;
    default: break;
    }
  });
  svc->configure(feed, "stable");
  if (svc->state() == S::Unsupported) return 5;
  QTimer::singleShot(0, svc.get(), &UpdateService::check);
  QTimer::singleShot(5 * 60 * 1000, &app, [&] { std::printf("update-test: timeout\n"); rc = 6; app.quit(); });
  app.exec();
  std::printf("update-test: exit %d\n", rc);
  return rc;
}

int runUpdateVerify(const QString& reportFile) {
  QSettings s;
  QJsonObject o;
  o["version"] = HCAD_VERSION;
  o["marker"] = s.value("test/updateMarker").toString();
  o["installDir"] = AppPaths::installDir();
  o["dataDir"] = AppPaths::dataDir();
  o["dataOutsideInstall"] = !AppPaths::isInside(AppPaths::dataDir(), AppPaths::installDir());
  QDir().mkpath(QFileInfo(reportFile).absolutePath());
  QFile f(reportFile);
  if (!f.open(QIODevice::WriteOnly)) return 1;
  f.write(QJsonDocument(o).toJson());
  std::printf("update-test-verify: wrote %s\n", qPrintable(reportFile));
  return 0;
}

} // namespace hcad::app
