#include "app/CommandPalette.h"
#include "app/CommandRegistry.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QVBoxLayout>

namespace hcad::app {
namespace {

enum Roles { IdRole = Qt::UserRole, DescRole, CatRole, KeyRole };

class PaletteDelegate : public QStyledItemDelegate {
public:
  using QStyledItemDelegate::QStyledItemDelegate;
  QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex&) const override { return {o.rect.width(), 46}; }
  void paint(QPainter* p, const QStyleOptionViewItem& o, const QModelIndex& idx) const override {
    p->save();
    const bool sel = o.state & QStyle::State_Selected;
    p->fillRect(o.rect, sel ? QColor(58, 62, 68) : QColor(37, 38, 41));
    if (sel) p->fillRect(QRect(o.rect.left(), o.rect.top(), 3, o.rect.height()), QColor(255, 170, 40));
    const QIcon icon = idx.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) icon.paint(p, QRect(o.rect.left() + 12, o.rect.top() + 11, 24, 24));
    QFont f = o.font;
    f.setBold(true);
    p->setFont(f);
    p->setPen(QColor(230, 232, 236));
    const QRect textR = o.rect.adjusted(46, 5, -150, -24);
    p->drawText(textR, Qt::AlignLeft | Qt::AlignVCenter, idx.data(Qt::DisplayRole).toString());
    p->setFont(o.font);
    p->setPen(QColor(138, 142, 148));
    p->drawText(o.rect.adjusted(46, 23, -150, -4), Qt::AlignLeft | Qt::AlignVCenter,
                p->fontMetrics().elidedText(idx.data(DescRole).toString(), Qt::ElideRight, o.rect.width() - 200));
    p->drawText(o.rect.adjusted(0, 5, -12, -24), Qt::AlignRight | Qt::AlignVCenter, idx.data(KeyRole).toString());
    p->drawText(o.rect.adjusted(0, 23, -12, -4), Qt::AlignRight | Qt::AlignVCenter, idx.data(CatRole).toString());
    p->restore();
  }
};

} // namespace

CommandPalette::CommandPalette(CommandRegistry* reg, QWidget* parent) : QDialog(parent, Qt::Popup), reg_(reg) {
  setObjectName("commandPalette");
  setStyleSheet("#commandPalette { background: #25262a; border: 1px solid #45484e; border-radius: 6px; }"
                "QLineEdit { background: #1d1e21; border: 1px solid #45484e; border-radius: 4px; padding: 8px; font-size: 11pt; }"
                "QListWidget { background: #25262a; border: none; }");
  auto* v = new QVBoxLayout(this);
  v->setContentsMargins(8, 8, 8, 8);
  edit_ = new QLineEdit;
  edit_->setPlaceholderText("Search commands…   (Enter to run, Esc to close)");
  list_ = new QListWidget;
  list_->setItemDelegate(new PaletteDelegate(list_));
  list_->setUniformItemSizes(true);
  v->addWidget(edit_);
  v->addWidget(list_);
  resize(620, 420);
  edit_->installEventFilter(this);
  connect(edit_, &QLineEdit::textChanged, this, &CommandPalette::refill);
  connect(list_, &QListWidget::itemActivated, this, [this] { runCurrent(); });
}

void CommandPalette::popup() {
  if (QWidget* w = parentWidget()) {
    const QPoint topCenter = w->mapToGlobal(QPoint(w->width() / 2, 60));
    move(topCenter.x() - width() / 2, topCenter.y());
  }
  edit_->clear();
  refill();
  show();
  raise();
  activateWindow();
  edit_->setFocus();
}

void CommandPalette::setQuery(const QString& q) { edit_->setText(q); refill(); }

QStringList CommandPalette::visibleIds() const {
  QStringList ids;
  for (int i = 0; i < list_->count(); ++i) ids << list_->item(i)->data(IdRole).toString();
  return ids;
}

void CommandPalette::refill() {
  list_->clear();
  QStringList ids = reg_->search(edit_->text());
  if (edit_->text().trimmed().isEmpty()) {
    // Recent first, then the rest in registration order.
    const QStringList recent = QSettings().value("palette/recent").toStringList();
    QStringList ordered;
    for (const auto& r : recent) if (ids.contains(r)) ordered << r;
    for (const auto& id : ids) if (!ordered.contains(id)) ordered << id;
    ids = ordered;
  }
  for (const auto& id : ids) {
    const CommandDef* d = reg_->def(id);
    auto* it = new QListWidgetItem(d->text);
    it->setData(IdRole, id);
    it->setData(DescRole, d->description);
    it->setData(CatRole, d->category);
    it->setData(KeyRole, d->shortcut.toString(QKeySequence::NativeText));
    if (QAction* a = reg_->action(id)) it->setIcon(a->icon());
    list_->addItem(it);
  }
  if (list_->count() > 0) list_->setCurrentRow(0);
}

void CommandPalette::remember(const QString& id) {
  QSettings s;
  QStringList recent = s.value("palette/recent").toStringList();
  recent.removeAll(id);
  recent.prepend(id);
  while (recent.size() > 8) recent.removeLast();
  s.setValue("palette/recent", recent);
}

bool CommandPalette::runCurrent() {
  QListWidgetItem* it = list_->currentItem();
  if (!it) return false;
  const QString id = it->data(IdRole).toString();
  hide();
  remember(id);
  return reg_->trigger(id);
}

bool CommandPalette::eventFilter(QObject* obj, QEvent* e) {
  if (obj == edit_ && e->type() == QEvent::KeyPress) {
    auto* k = static_cast<QKeyEvent*>(e);
    switch (k->key()) {
    case Qt::Key_Down: list_->setCurrentRow(std::min(list_->currentRow() + 1, list_->count() - 1)); return true;
    case Qt::Key_Up: list_->setCurrentRow(std::max(list_->currentRow() - 1, 0)); return true;
    case Qt::Key_Return:
    case Qt::Key_Enter: runCurrent(); return true;
    case Qt::Key_Escape: hide(); return true;
    default: break;
    }
  }
  return QDialog::eventFilter(obj, e);
}

} // namespace hcad::app
