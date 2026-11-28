#pragma once
#include <QString>

namespace hcad::app {
// Installs a Qt message handler that writes timestamped lines to
// <logsDir>/hybridcad.log (rotated at 2 MB, 3 files kept) and to stderr.
void installLogging(const QString& logsDir);
QString currentLogFile();
} // namespace hcad::app
