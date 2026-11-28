#pragma once
#include <QAction>
#include <QHash>
#include <QIcon>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <vector>

namespace hcad::app {

// A user-invocable command. Menus, toolbars, the command palette, tooltips and
// the shortcut sheet are all generated from these definitions, so every
// visible control maps to an implemented command.
struct CommandDef {
  QString id;           // stable identifier, e.g. "view.fitAll"
  QString text;         // menu/palette label
  QString description;  // one sentence for tooltips and the palette
  QString category;     // "View", "Selection", "File", ...
  QKeySequence shortcut;
  QString icon;         // Icons::get() name, may be empty
  bool checkable = false;
  std::function<void()> run;
  std::function<bool()> enabled;  // optional; default always enabled
  std::function<bool()> checked;  // optional for checkable commands
};

class CommandRegistry : public QObject {
  Q_OBJECT
public:
  explicit CommandRegistry(QWidget* actionParent);

  QAction* add(CommandDef def);
  QAction* action(const QString& id) const;
  const CommandDef* def(const QString& id) const;
  const std::vector<CommandDef>& all() const { return defs_; }

  bool trigger(const QString& id);
  // Re-evaluates enabled/checked state of every command (cheap; call after
  // document or selection changes).
  void refreshStates();

  // Palette search: ids sorted by match quality, only enabled commands.
  QStringList search(const QString& query) const;

  static QString tooltipFor(const CommandDef& d);

signals:
  void triggered(const QString& id);

private:
  QWidget* parent_;
  std::vector<CommandDef> defs_;
  QHash<QString, QAction*> actions_;
};

} // namespace hcad::app
