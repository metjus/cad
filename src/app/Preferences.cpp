#include "app/Preferences.h"
#include <QSettings>

namespace hcad::app {

Preferences Preferences::load() {
  QSettings s;
  Preferences p;
  p.navigation = s.value("prefs/navigation", "blender").toString() == "cad" ? NavigationStyle::Cad : NavigationStyle::Blender;
  p.units = s.value("prefs/units", "mm").toString() == "in" ? LengthUnit::Inch : LengthUnit::Millimetre;
  p.showWelcome = s.value("prefs/showWelcome", true).toBool();
  p.welcomeCompleted = s.value("prefs/welcomeCompleted", false).toBool();
  p.checkUpdatesOnStartup = s.value("update/checkOnStartup", true).toBool();
  p.updateChannel = s.value("update/channel", "stable").toString();
  p.updateFeed = s.value("update/feed").toString();
  return p;
}

void Preferences::save() const {
  QSettings s;
  s.setValue("prefs/navigation", navigation == NavigationStyle::Cad ? "cad" : "blender");
  s.setValue("prefs/units", units == LengthUnit::Inch ? "in" : "mm");
  s.setValue("prefs/showWelcome", showWelcome);
  s.setValue("prefs/welcomeCompleted", welcomeCompleted);
  s.setValue("update/checkOnStartup", checkUpdatesOnStartup);
  s.setValue("update/channel", updateChannel);
  s.setValue("update/feed", updateFeed);
}

} // namespace hcad::app
