#include <gtest/gtest.h>

#include "search/fuzzy_matcher.h"

namespace themis {
namespace {

TEST(FuzzyMatcherNgram, StringsShorterThanNgramSizeRemainComparable) {
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("x", "x", 2), 1.0);
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("x", "y", 2), 0.0);
}

TEST(FuzzyMatcherNgram, RepeatedNgramsAreCountedAsMultisets) {
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("aaaa", "aa", 2), 0.5);
}

} // namespace
} // namespace themis
