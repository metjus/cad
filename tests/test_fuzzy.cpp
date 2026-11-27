#include "core/FuzzyMatch.h"
#include <gtest/gtest.h>

using hcad::fuzzyScore;

TEST(FuzzyMatch, EmptyQueryMatchesEverything) {
  EXPECT_GE(fuzzyScore("", "Fit All"), 0);
}

TEST(FuzzyMatch, RequiresCharactersInOrder) {
  EXPECT_LT(fuzzyScore("xyz", "Fit All"), 0);
  EXPECT_LT(fuzzyScore("lla", "Fit All"), 0);
  EXPECT_GE(fuzzyScore("fa", "Fit All"), 0);
}

TEST(FuzzyMatch, CaseInsensitive) {
  EXPECT_GE(fuzzyScore("FIT", "fit all"), 0);
  EXPECT_EQ(fuzzyScore("fit", "Fit All"), fuzzyScore("FIT", "Fit All"));
}

TEST(FuzzyMatch, PrefixBeatsMiddle) {
  EXPECT_GT(fuzzyScore("top", "Top View"), fuzzyScore("top", "Stop Viewing"));
}

TEST(FuzzyMatch, WordStartsBeatScatteredLetters) {
  // "iv" = Isometric View (word starts) vs. "prevIew pIVot"-like scatter.
  EXPECT_GT(fuzzyScore("iv", "Isometric View"), fuzzyScore("iv", "Preview Divider"));
}

TEST(FuzzyMatch, ConsecutiveBeatsGaps) {
  EXPECT_GT(fuzzyScore("grid", "Show Grid"), fuzzyScore("grid", "Bigger Fraction Hold"));
}
