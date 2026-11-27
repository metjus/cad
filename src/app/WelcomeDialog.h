#pragma once
#include "app/Preferences.h"
#include <QDialog>

class QCheckBox;
class QRadioButton;

namespace hcad::app {

// First-launch onboarding (brief §5): choose a navigation preset and units,
// learn the three things needed to get started, then open the example
// project or start empty. Can be reopened from Help > Welcome Screen.
class WelcomeDialog : public QDialog {
  Q_OBJECT
public:
  enum class Start { Example, Empty };
  WelcomeDialog(const Preferences& prefs, QWidget* parent);
  // Preferences with the user's choices applied (navigation, units, showWelcome).
  Preferences result(Preferences base) const;
  Start start() const { return start_; }

private:
  QRadioButton* blender_;
  QRadioButton* cad_;
  QRadioButton* mm_;
  QRadioButton* inch_;
  QCheckBox* showAgain_;
  Start start_ = Start::Example;
};

} // namespace hcad::app
