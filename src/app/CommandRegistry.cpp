#include "app/CommandRegistry.h"
#include "app/Icons.h"
#include "core/FuzzyMatch.h"

#include <QWidget>
#include <algorithm>

namespace hcad::app {

CommandRegistry::CommandRegistry(QWidget* parent) : QObject(parent), parent_(parent) {}

QString CommandRegistry::tooltipFor(const CommandDef& d) {
  QString t = QString("<b>%1</b>").arg(d.text.toHtmlEscaped());
  if (!d.shortcut.isEmpty())
    t += QString(" <span style='color:#8a8e94'>%1</span>").arg(d.shortcut.toString(QKeySequence::NativeText).toHtmlEscaped());
  if (!d.description.isEmpty()) t += "<br>" + d.description.toHtmlEscaped();
  return t;
}

QAction* CommandRegistry::add(CommandDef d) {
  Q_ASSERT(!actions_.contains(d.id));
  auto* a = new QAction(d.text, parent_);
  a->setObjectName(d.id);
  a->setShortcut(d.shortcut);
  a->setShortcutContext(Qt::WindowShortcut);
  a->setToolTip(tooltipFor(d));
  a->setStatusTip(d.description);
  a->setCheckable(d.checkable);
  if (!d.icon.isEmpty()) a->setIcon(Icons::get(d.icon));
  parent_->addAction(a); // shortcuts work even when the command is in no menu
  const QString id = d.id;
  connect(a, &QAction::triggered, this, [this, id] { trigger(id); });
  actions_.insert(d.id, a);
  defs_.push_back(std::move(d));
  return a;
}

QAction* CommandRegistry::action(const QString& id) const { return actions_.value(id, nullptr); }

const CommandDef* CommandRegistry::def(const QString& id) const {
  for (const auto& d : defs_) if (d.id == id) return &d;
  return nullptr;
}

bool CommandRegistry::trigger(const QString& id) {
  const CommandDef* d = def(id);
  if (!d || !d->run) return false;
  if (d->enabled && !d->enabled()) return false;
  d->run();
  emit triggered(id);
  refreshStates();
  return true;
}

void CommandRegistry::refreshStates() {
  for (const auto& d : defs_) {
    QAction* a = actions_.value(d.id);
    if (!a) continue;
    a->setEnabled(!d.enabled || d.enabled());
    if (d.checkable && d.checked) {
      const QSignalBlocker block(a);
      a->setChecked(d.checked());
    }
  }
}

QStringList CommandRegistry::search(const QString& query) const {
  struct Hit { int score; size_t order; QString id; };
  std::vector<Hit> hits;
  const std::string q = query.trimmed().toStdString();
  for (size_t i = 0; i < defs_.size(); ++i) {
    const auto& d = defs_[i];
    if (d.enabled && !d.enabled()) continue;
    int s = fuzzyScore(q, d.text.toStdString());
    // Also match against "Category: Name" and the description, weaker.
    s = std::max(s, fuzzyScore(q, (d.category + " " + d.text).toStdString()) - 30);
    if (s < 0) {
      const int ds = fuzzyScore(q, d.description.toStdString());
      if (ds >= 0 && q.size() >= 3) s = ds / 4;
    }
    if (s >= 0) hits.push_back({s, i, d.id});
  }
  std::stable_sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.score > b.score; });
  QStringList out;
  for (const auto& h : hits) out << h.id;
  return out;
}

} // namespace hcad::app
