#pragma once
#include "app/Preferences.h"
#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;

namespace hcad::app {

class PreferencesDialog : public QDialog {
  Q_OBJECT
public:
  PreferencesDialog(const Preferences& prefs, QWidget* parent);
  Preferences result(Preferences base) const;

private:
  QComboBox* nav_;
  QComboBox* units_;
  QCheckBox* welcome_;
  QCheckBox* checkUpdates_;
  QComboBox* channel_;
  QLineEdit* feed_;
};

} // namespace hcad::app
