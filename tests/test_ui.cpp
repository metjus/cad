// UI-layer tests that need no OpenGL and never show a window.
#include "app/AppPaths.h"
#include "app/CommandPalette.h"
#include "app/CommandRegistry.h"
#include "app/Icons.h"
#include "app/MainWindow.h"
#include "app/Preferences.h"
#include "app/UpdateService.h"
#include "app/Version.h"

#include <QApplication>
#include <QMenuBar>
#include <QSettings>
#include <QStandardPaths>
#include <QToolBar>
#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#include <cstdlib>
#endif

using namespace hcad::app;
using hcad::LengthUnit;

TEST(Version, IsSemVerAndMatchesComponents) {
  const QString v = HCAD_VERSION;
  EXPECT_TRUE(v.startsWith(QString("%1.%2.%3").arg(HCAD_VERSION_MAJOR).arg(HCAD_VERSION_MINOR).arg(HCAD_VERSION_PATCH)));
  EXPECT_STREQ(HCAD_PACK_ID, "HybridCAD.App");
}

TEST(CommandRegistry, TriggerRespectsEnabledAndEmits) {
  QWidget host;
  CommandRegistry reg(&host);
  int runs = 0;
  bool enabled = true;
  CommandDef d{"test.cmd", "Do Thing", "Does a thing", "Test", QKeySequence("Ctrl+Shift+T"), "fit", false, [&] { ++runs; }};
  d.enabled = [&] { return enabled; };
  QAction* a = reg.add(d);
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(reg.action("test.cmd"), a);
  EXPECT_FALSE(a->icon().isNull());
  EXPECT_TRUE(a->toolTip().contains("Does a thing"));
  int emitted = 0;
  QObject::connect(&reg, &CommandRegistry::triggered, [&](const QString& id) { if (id == "test.cmd") ++emitted; });
  EXPECT_TRUE(reg.trigger("test.cmd"));
  EXPECT_EQ(runs, 1);
  EXPECT_EQ(emitted, 1);
  a->trigger(); // menu/toolbar/shortcut path
  EXPECT_EQ(runs, 2);
  enabled = false;
  reg.refreshStates();
  EXPECT_FALSE(a->isEnabled());
  EXPECT_FALSE(reg.trigger("test.cmd"));
  EXPECT_EQ(runs, 2);
  EXPECT_FALSE(reg.trigger("no.such"));
}

TEST(CommandRegistry, CheckedStateFollowsModel) {
  QWidget host;
  CommandRegistry reg(&host);
  bool on = false;
  CommandDef d{"test.toggle", "Toggle", "", "Test", {}, {}, true, [&] { on = !on; }};
  d.checked = [&] { return on; };
  QAction* a = reg.add(d);
  reg.trigger("test.toggle");
  EXPECT_TRUE(a->isChecked());
  reg.trigger("test.toggle");
  EXPECT_FALSE(a->isChecked());
}

TEST(CommandRegistry, SearchRanksBestMatchFirstAndSkipsDisabled) {
  QWidget host;
  CommandRegistry reg(&host);
  auto add = [&](const char* id, const char* text, const char* cat, bool en = true) {
    CommandDef d{id, text, "", cat, {}, {}, false, [] {}};
    d.enabled = [en] { return en; };
    reg.add(d);
  };
  add("view.top", "Top View", "View");
  add("view.fitAll", "Fit All", "View");
  add("view.grid", "Show Grid", "View");
  add("x.hidden", "Top Secret", "Test", false);
  const QStringList r = reg.search("top");
  ASSERT_FALSE(r.isEmpty());
  EXPECT_EQ(r.front(), "view.top");
  EXPECT_FALSE(r.contains("x.hidden"));
  EXPECT_EQ(reg.search("grid").front(), "view.grid");
  EXPECT_TRUE(reg.search("qqq").isEmpty());
  EXPECT_EQ(reg.search("").size(), 3);
}

TEST(CommandPalette, FiltersAndRunsCommand) {
  QWidget host;
  CommandRegistry reg(&host);
  int fit = 0;
  reg.add({"view.fitAll", "Fit All", "Frame everything", "View", {}, "fit", false, [&] { ++fit; }});
  reg.add({"view.top", "Top View", "Look from above", "View", {}, "view-top", false, [] {}});
  CommandPalette pal(&reg, &host);
  pal.setQuery("fit");
  ASSERT_EQ(pal.visibleIds().size(), 1);
  EXPECT_EQ(pal.visibleIds().front(), "view.fitAll");
  EXPECT_TRUE(pal.runCurrent());
  EXPECT_EQ(fit, 1);
  // Recently used commands come first for an empty query.
  pal.setQuery("");
  EXPECT_EQ(pal.visibleIds().front(), "view.fitAll");
}

TEST(Icons, EveryIconHasAllStates) {
  const QStringList names = Icons::names();
  EXPECT_GE(names.size(), 20);
  for (const auto& n : names) {
    const QIcon i = Icons::get(n);
    ASSERT_FALSE(i.isNull()) << n.toStdString();
    for (auto mode : {QIcon::Normal, QIcon::Active, QIcon::Disabled})
      EXPECT_FALSE(i.pixmap(24, 24, mode, QIcon::Off).isNull()) << n.toStdString();
    // Checked state uses the accent colour, so it must differ from normal
    // (except the fixed-colour application mark).
    if (n != "app") EXPECT_NE(i.pixmap(24, 24, QIcon::Normal, QIcon::On).toImage(), i.pixmap(24, 24, QIcon::Normal, QIcon::Off).toImage())
      << n.toStdString();
  }
  EXPECT_TRUE(Icons::get("does-not-exist").isNull());
}

TEST(AppPaths, UserDataIsOutsideInstallDir) {
  EXPECT_FALSE(AppPaths::isInside(AppPaths::dataDir(), AppPaths::installDir()));
  EXPECT_FALSE(AppPaths::isInside(AppPaths::logsDir(), AppPaths::installDir()));
  EXPECT_FALSE(AppPaths::isInside(AppPaths::projectsDir(), AppPaths::installDir()));
  EXPECT_TRUE(AppPaths::isInside(AppPaths::logsDir(), AppPaths::dataDir()));
  EXPECT_TRUE(AppPaths::isInside("/a/b/c", "/a/b"));
  EXPECT_FALSE(AppPaths::isInside("/a/bc", "/a/b"));
}

TEST(Preferences, RoundTrip) {
  Preferences p;
  p.navigation = NavigationStyle::Cad;
  p.units = LengthUnit::Inch;
  p.showWelcome = false;
  p.welcomeCompleted = true;
  p.checkUpdatesOnStartup = false;
  p.updateChannel = "beta";
  p.updateFeed = "C:/feed";
  p.save();
  const Preferences q = Preferences::load();
  EXPECT_EQ(q.navigation, NavigationStyle::Cad);
  EXPECT_EQ(q.units, LengthUnit::Inch);
  EXPECT_FALSE(q.showWelcome);
  EXPECT_TRUE(q.welcomeCompleted);
  EXPECT_FALSE(q.checkUpdatesOnStartup);
  EXPECT_EQ(q.updateChannel, "beta");
  EXPECT_EQ(q.updateFeed, "C:/feed");
  QSettings().clear();
  EXPECT_EQ(Preferences::load().navigation, NavigationStyle::Blender);
}

TEST(UpdateService, DeveloperBuildReportsUnsupported) {
  if (UpdateService::velopackEnabled()) GTEST_SKIP() << "Velopack build";
  auto s = UpdateService::create();
  EXPECT_EQ(s->state(), UpdateService::State::Unsupported);
  EXPECT_EQ(s->currentVersion(), HCAD_VERSION);
  s->check();
  EXPECT_EQ(s->state(), UpdateService::State::Unsupported);
  EXPECT_FALSE(s->applyAndRestart());
}

// No decorative UI (brief §4): every menu and toolbar entry is a registered,
// described command, and every command is reachable from a menu.
TEST(MainWindow, MenusAndToolbarsAreGeneratedFromCommands) {
  MainWindow w(Preferences{});
  auto& reg = w.commands();
  ASSERT_GE(reg.all().size(), 30u);
  QSet<QString> inMenus;
  for (QAction* top : w.menuBar()->actions()) {
    ASSERT_NE(top->menu(), nullptr);
    std::function<void(QMenu*)> walk = [&](QMenu* m) {
      for (QAction* a : m->actions()) {
        if (a->isSeparator()) continue;
        if (a->menu()) { walk(a->menu()); continue; }
        ASSERT_NE(reg.def(a->objectName()), nullptr) << "menu item without command: " << a->text().toStdString();
        inMenus.insert(a->objectName());
      }
    };
    walk(top->menu());
  }
  for (const auto& d : reg.all()) {
    EXPECT_TRUE(inMenus.contains(d.id)) << "command not in any menu: " << d.id.toStdString();
    EXPECT_FALSE(d.description.isEmpty()) << d.id.toStdString();
    EXPECT_FALSE(d.category.isEmpty()) << d.id.toStdString();
  }
  for (QToolBar* tb : w.findChildren<QToolBar*>())
    for (QAction* a : tb->actions())
      if (!a->isSeparator()) {
        EXPECT_NE(reg.def(a->objectName()), nullptr) << a->text().toStdString();
        EXPECT_FALSE(a->icon().isNull()) << "toolbar action without icon: " << a->objectName().toStdString();
      }
  // Shortcuts are unique.
  QHash<QString, QString> seen;
  for (const auto& d : reg.all())
    for (const auto& k : reg.action(d.id)->shortcuts()) {
      const QString key = k.toString();
      EXPECT_FALSE(seen.contains(key)) << key.toStdString() << " used by " << seen.value(key).toStdString() << " and "
                                       << d.id.toStdString();
      seen.insert(key, d.id);
    }
}

TEST(MainWindow, ExampleAndEmptyProjects) {
  MainWindow w(Preferences{});
  w.openExampleProject();
  EXPECT_FALSE(w.document().features().empty());
  EXPECT_NE(w.document().feature("Fillet1"), nullptr);
  w.newEmptyProject(false);
  EXPECT_TRUE(w.document().features().empty());
  EXPECT_TRUE(w.commands().trigger("file.openExample"));
  EXPECT_NE(w.document().feature("Box1"), nullptr);
}

TEST(MainWindow, SelectionModeCommandsAreExclusive) {
  MainWindow w(Preferences{});
  w.commands().trigger("select.edge");
  EXPECT_EQ(w.viewport()->selectionMode(), SelectionMode::Edge);
  EXPECT_TRUE(w.commands().action("select.edge")->isChecked());
  EXPECT_FALSE(w.commands().action("select.object")->isChecked());
  w.commands().trigger("select.face");
  EXPECT_FALSE(w.commands().action("select.edge")->isChecked());
}

TEST(MainWindow, NavigationPreferenceApplies) {
  Preferences p;
  p.navigation = NavigationStyle::Cad;
  MainWindow w(p);
  EXPECT_EQ(w.viewport()->navigationStyle(), NavigationStyle::Cad);
  p.navigation = NavigationStyle::Blender;
  w.applyPreferences(p);
  EXPECT_EQ(w.viewport()->navigationStyle(), NavigationStyle::Blender);
}

int main(int argc, char** argv) {
#ifdef _WIN32
  // Never block a CI run on an error dialog (crash, abort, missing plugin).
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
  ::testing::InitGoogleTest(&argc, argv);
  if (::testing::GTEST_FLAG(list_tests)) return RUN_ALL_TESTS(); // test discovery: no Qt start-up
#ifndef _WIN32
  qputenv("QT_QPA_PLATFORM", "offscreen"); // no X server needed
#endif
  // (On Windows the native platform plugin works without showing windows.)
  QStandardPaths::setTestModeEnabled(true);
  QApplication app(argc, argv);
  QApplication::setOrganizationName("HybridCAD");
  QApplication::setApplicationName("HybridCAD-UnitTest");
  QSettings().clear();
  const int rc = RUN_ALL_TESTS();
  QSettings().clear();
  return rc;
}
