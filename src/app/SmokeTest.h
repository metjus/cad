#pragma once
#include <QString>
namespace hcad::app {
class MainWindow;
// Scripted UI run used by CI: drives real viewport picking, edits a
// parameter, verifies persistent re-selection, and writes screenshots.
void runSmokeTest(MainWindow* w, const QString& outDir, const QString& meshPath);
}
