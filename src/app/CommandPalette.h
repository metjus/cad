#pragma once
#include <QDialog>

class QLineEdit;
class QListWidget;

namespace hcad::app {

class CommandRegistry;

// F3 / Ctrl+K command search. Shows name, description, category and
// shortcut; Enter runs the highlighted command. Recently used commands are
// listed first when the query is empty.
class CommandPalette : public QDialog {
  Q_OBJECT
public:
  CommandPalette(CommandRegistry* registry, QWidget* parent);
  void popup();
  QStringList visibleIds() const;           // for tests
  void setQuery(const QString& q);          // for tests
  bool runCurrent();

protected:
  bool eventFilter(QObject* obj, QEvent* e) override;

private:
  void refill();
  void remember(const QString& id);

  CommandRegistry* reg_;
  QLineEdit* edit_;
  QListWidget* list_;
};

} // namespace hcad::app
