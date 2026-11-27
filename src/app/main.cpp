#include "app/MainWindow.h"
#include "app/SmokeTest.h"
#include "app/Theme.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setOrganizationName("HybridCAD");
  QApplication::setApplicationName("HybridCAD-PoC");
  QApplication::setApplicationVersion("0.0.1-phase0");
  hcad::app::applyDarkTheme(app);

  QCommandLineParser cli;
  cli.addHelpOption();
  cli.addVersionOption();
  QCommandLineOption smoke("smoke-test", "Run the scripted UI smoke test and write screenshots to <dir>.", "dir");
  QCommandLineOption mesh("mesh", "Mesh file to import during the smoke test.", "file");
  cli.addOption(smoke);
  cli.addOption(mesh);
  cli.process(app);

  hcad::app::MainWindow w;
  w.show();
  if (cli.isSet(smoke)) hcad::app::runSmokeTest(&w, cli.value(smoke), cli.value(mesh));
  return app.exec();
}
