#pragma once
#include <string>

namespace hcad {

// Scores how well `query` matches `text` for the command palette.
// Returns a negative value when the query characters do not all appear in
// order (case-insensitive). Higher is better: prefix and word-start matches
// and consecutive characters are rewarded, gaps are penalised.
int fuzzyScore(const std::string& query, const std::string& text);

} // namespace hcad
