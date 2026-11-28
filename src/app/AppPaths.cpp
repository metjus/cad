#include "app/AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace hcad::app::AppPaths {

static QString ensure(const QString& p) {
  QDir().mkpath(p);
  return QDir::cleanPath(p);
}

QString installDir() { return QDir::cleanPath(QCoreApplication::applicationDirPath()); }
QString dataDir() { return ensure(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)); }
QString logsDir() { return ensure(dataDir() + "/logs"); }
QString recoveryDir() { return ensure(dataDir() + "/recovery"); }
QString projectsDir() {
  return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/HybridCAD Projects");
}

bool isInside(const QString& path, const QString& dir) {
  const QString p = QDir::cleanPath(QDir(path).absolutePath()) + "/";
  const QString d = QDir::cleanPath(QDir(dir).absolutePath()) + "/";
  return p.startsWith(d, Qt::CaseInsensitive);
}

} // namespace hcad::app::AppPaths
