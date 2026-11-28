#pragma once
#include <QString>

namespace hcad::app {

// Separation of application files and user data (brief §15):
//  * installDir   - program binaries, replaced on update (never written to)
//  * dataDir      - per-user data: logs, recovery, caches (%LOCALAPPDATA%)
//  * settings     - QSettings (registry HKCU on Windows)
//  * projectsDir  - default folder for user projects (Documents)
// None of the user locations is inside the install directory, so an update
// or uninstall can never delete user data.
namespace AppPaths {
QString installDir();
QString dataDir();
QString logsDir();
QString recoveryDir();
QString projectsDir();
bool isInside(const QString& path, const QString& dir);
} // namespace AppPaths

} // namespace hcad::app
