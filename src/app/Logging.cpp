#include "app/Logging.h"

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QTextStream>
#include <cstdio>

namespace hcad::app {
namespace {
QString g_logFile;
QMutex g_mutex;
constexpr qint64 kMaxBytes = 2 * 1024 * 1024;

void rotate() {
  QFile f(g_logFile);
  if (f.size() < kMaxBytes) return;
  QFile::remove(g_logFile + ".3");
  for (int i = 2; i >= 1; --i) QFile::rename(g_logFile + "." + QString::number(i), g_logFile + "." + QString::number(i + 1));
  QFile::rename(g_logFile, g_logFile + ".1");
}

void handler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
  static const char* names[] = {"DEBUG", "WARN", "CRIT", "FATAL", "INFO"};
  const char* level = names[type <= QtInfoMsg ? type : 0];
  const QString line = QString("%1 [%2] %3\n").arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), level, msg);
  QMutexLocker lock(&g_mutex);
  std::fputs(line.toLocal8Bit().constData(), stderr);
  if (g_logFile.isEmpty()) return;
  rotate();
  QFile f(g_logFile);
  if (f.open(QIODevice::Append | QIODevice::Text)) f.write(line.toUtf8());
}
} // namespace

void installLogging(const QString& logsDir) {
  g_logFile = logsDir + "/hybridcad.log";
  qInstallMessageHandler(handler);
}

QString currentLogFile() { return g_logFile; }

} // namespace hcad::app
