#pragma once
#include <QIcon>
#include <QStringList>

namespace hcad::app {

// Original vector icon set, drawn in code on a 24x24 grid with a 1.6 px
// stroke. Every icon is generated for the four states the brief requires:
// normal, hover/active, checked (amber accent) and disabled, at 1x and 2x.
namespace Icons {
QIcon get(const QString& name);
QStringList names();
} // namespace Icons

} // namespace hcad::app
