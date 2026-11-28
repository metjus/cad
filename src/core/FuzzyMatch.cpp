#include "core/FuzzyMatch.h"

#include <cctype>

namespace hcad {

int fuzzyScore(const std::string& query, const std::string& text) {
  if (query.empty()) return 0;
  auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
  int score = 0;
  size_t ti = 0;
  int run = 0;
  size_t firstMatch = std::string::npos;
  for (char qc : query) {
    if (qc == ' ') { run = 0; continue; }
    const char q = lower(qc);
    bool found = false;
    while (ti < text.size()) {
      const char t = lower(text[ti]);
      const bool wordStart = ti == 0 || text[ti - 1] == ' ' || text[ti - 1] == '-' || text[ti - 1] == '/' ||
                             (std::isupper(static_cast<unsigned char>(text[ti])) && std::islower(static_cast<unsigned char>(text[ti - 1])));
      if (t == q) {
        if (firstMatch == std::string::npos) firstMatch = ti;
        score += 10;
        if (wordStart) score += 15;
        if (run > 0) score += 8 * run;
        ++run;
        ++ti;
        found = true;
        break;
      }
      run = 0;
      score -= 1;
      ++ti;
    }
    if (!found) return -1;
  }
  if (firstMatch == 0) score += 25;
  score -= static_cast<int>(text.size()) / 4; // prefer shorter names on ties
  return score < 0 ? 0 : score;
}

} // namespace hcad
