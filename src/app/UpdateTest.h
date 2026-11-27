#pragma once
#include <QString>

class QApplication;

namespace hcad::app {
// Headless update round trip used by CI on an installed copy:
//  1. runUpdateTest: stores a settings marker, checks <feed>, downloads the
//     update (delta when available), launches the updater with
//     --update-test-verify <dataDir>/update-test.json and exits.
//  2. runUpdateVerify (in the new version after restart): writes the running
//     version, the marker and paths as JSON so CI can assert on them.
int runUpdateTest(QApplication& app, const QString& feed);
int runUpdateVerify(const QString& reportFile);
} // namespace hcad::app
