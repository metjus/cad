#include "app/AppPaths.h"
#include "app/Logging.h"
#include "app/MainWindow.h"
#include "app/Preferences.h"
#include "app/SmokeTest.h"
#include "app/Theme.h"
#include "app/UpdateService.h"
#include "app/UpdateTest.h"
#include "app/Version.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSysInfo>

int main(int argc, char** argv) {
  // Installer/uninstaller/update hooks must run before anything else; they may
  // exit the process. No-op in builds without Velopack.
  hcad::app::UpdateService::runStartupHooks(argc, argv);

  QApplication app(argc, argv);
  QApplication::setOrganizationName("HybridCAD");
  QApplication::setApplicationName("HybridCAD");
  QApplication::setApplicationDisplayName("HybridCAD");
  QApplication::setApplicationVersion(HCAD_VERSION);

  QCommandLineParser cli;
  cli.setApplicationDescription("HybridCAD parametric CAD");
  cli.addHelpOption();
  cli.addVersionOption();
  QCommandLineOption smoke("smoke-test", "Run the scripted UI smoke test and write screenshots to <dir>.", "dir");
  QCommandLineOption mesh("mesh", "Mesh file to import during the smoke test.", "file");
  QCommandLineOption updTest("update-test", "Developer: check <feed>, download and apply an update, restart into --update-test-verify.", "feed");
  QCommandLineOption updVerify("update-test-verify", "Developer: write version and settings report to <file> and exit.", "file");
  QCommandLineOption scope("settings-scope", "Use a separate settings/data scope (tests).", "name");
  for (auto* o : {&smoke, &mesh, &updTest, &updVerify, &scope}) cli.addOption(*o);
  cli.process(app);

  // Tests run in their own scope so they never touch the user's settings.
  if (cli.isSet(scope)) QApplication::setApplicationName("HybridCAD-" + cli.value(scope));
  else if (cli.isSet(smoke)) QApplication::setApplicationName("HybridCAD-SmokeTest");

  hcad::app::installLogging(hcad::app::AppPaths::logsDir());
  qInfo().noquote() << "HybridCAD" << HCAD_VERSION << "starting on" << QSysInfo::prettyProductName()
                    << "| install dir" << hcad::app::AppPaths::installDir();

  if (cli.isSet(updVerify)) return hcad::app::runUpdateVerify(cli.value(updVerify));
  if (cli.isSet(updTest)) return hcad::app::runUpdateTest(app, cli.value(updTest));

  hcad::app::applyDarkTheme(app);
  hcad::app::Preferences prefs = hcad::app::Preferences::load();
  hcad::app::MainWindow w(prefs);
  w.show();
  if (cli.isSet(smoke)) {
    w.openExampleProject();
    hcad::app::runSmokeTest(&w, cli.value(smoke), cli.value(mesh));
  } else {
    QMetaObject::invokeMethod(&w, [&w] { w.maybeShowWelcome(); }, Qt::QueuedConnection);
  }
  const int rc = app.exec();
  qInfo() << "Exit code" << rc;
  return rc;
}
