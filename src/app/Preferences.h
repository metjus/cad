#pragma once
#include "core/Units.h"
#include <QString>

namespace hcad::app {

enum class NavigationStyle { Blender, Cad };

// Typed access to persisted user preferences (QSettings). Kept outside the
// install directory, preserved across updates.
struct Preferences {
  NavigationStyle navigation = NavigationStyle::Blender;
  LengthUnit units = LengthUnit::Millimetre;
  bool showWelcome = true;
  bool welcomeCompleted = false;
  bool checkUpdatesOnStartup = true;
  QString updateChannel = "stable"; // stable | beta
  QString updateFeed;               // empty = built-in default

  static Preferences load();
  void save() const;
};

} // namespace hcad::app
