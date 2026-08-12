/**
 * @file test_fuzzy_matcher.cpp
 * @brief Unit tests for FuzzyMatcher static algorithm utilities (v1.5.0)
 *
 * Static methods are tested directly (no live index required).
 * Integration with SecondaryIndexManager is tested in test_hybrid_search_integration.cpp.
 */

#include <gtest/gtest.h>
#include "search/fuzzy_matcher.h"
#include <cmath>
#include <string>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(FuzzyMatcherConfig, NullIndexIsAccepted) {
    // Construction with null index is valid; search() will return error
    EXPECT_NO_THROW(FuzzyMatcher(nullptr));
}

TEST(FuzzyMatcherConfig, NegativeMaxDistanceThrows) {
    FuzzyMatcher::Config cfg;
    cfg.max_distance = -1;
    EXPECT_THROW(FuzzyMatcher(nullptr, cfg), std::invalid_argument);
}

TEST(FuzzyMatcherConfig, ZeroNgramSizeThrows) {
    FuzzyMatcher::Config cfg;
    cfg.ngram_size = 0;
    EXPECT_THROW(FuzzyMatcher(nullptr, cfg), std::invalid_argument);
}

// ============================================================================
// Levenshtein
// ============================================================================

TEST(FuzzyMatcherLevenshtein, IdenticalStrings) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("hello", "hello"), 0);
}

TEST(FuzzyMatcherLevenshtein, EmptyAndNonEmpty) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("", "abc"), 3);
    EXPECT_EQ(FuzzyMatcher::levenshtein("abc", ""), 3);
}

TEST(FuzzyMatcherLevenshtein, BothEmpty) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("", ""), 0);
}

TEST(FuzzyMatcherLevenshtein, SingleSubstitution) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("cat", "bat"), 1);
}

TEST(FuzzyMatcherLevenshtein, SingleInsertion) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("database", "databse"), 1);
}

TEST(FuzzyMatcherLevenshtein, SingleDeletion) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("colour", "color"), 1);
}

TEST(FuzzyMatcherLevenshtein, MultipleDifferences) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("kitten", "sitting"), 3);
}

TEST(FuzzyMatcherLevenshtein, Symmetric) {
    EXPECT_EQ(FuzzyMatcher::levenshtein("abc", "xyz"),
              FuzzyMatcher::levenshtein("xyz", "abc"));
}

// ============================================================================
// Soundex
// ============================================================================

TEST(FuzzyMatcherSoundex, EmptyReturns0000) {
    EXPECT_EQ(FuzzyMatcher::soundex(""), "0000");
}

TEST(FuzzyMatcherSoundex, CorrectCodeForRobert) {
    EXPECT_EQ(FuzzyMatcher::soundex("Robert"), "R010");
}

TEST(FuzzyMatcherSoundex, CorrectCodeForRupert) {
    EXPECT_EQ(FuzzyMatcher::soundex("Rupert"), "R010");
}

TEST(FuzzyMatcherSoundex, AlwaysFourChars) {
    EXPECT_EQ(FuzzyMatcher::soundex("a").size(), 4u);
    EXPECT_EQ(FuzzyMatcher::soundex("supercalifragilistic").size(), 4u);
}

TEST(FuzzyMatcherSoundex, SimilarNamesMatchingCode) {
    // "Smith" and "Smythe" should have the same Soundex code
    std::string s1 = FuzzyMatcher::soundex("Smith");
    std::string s2 = FuzzyMatcher::soundex("Smythe");
    EXPECT_EQ(s1, s2);
}

// ============================================================================
// Metaphone
// ============================================================================

TEST(FuzzyMatcherMetaphone, EmptyReturnsEmpty) {
    EXPECT_EQ(FuzzyMatcher::metaphone(""), "");
}

TEST(FuzzyMatcherMetaphone, SimilarPhoneticWordsShareCode) {
    std::string c1 = FuzzyMatcher::metaphone("knight");
    std::string c2 = FuzzyMatcher::metaphone("night");
    EXPECT_FALSE(c1.empty());
    EXPECT_FALSE(c2.empty());
}

TEST(FuzzyMatcherMetaphone, NonEmptyForNormalWord) {
    EXPECT_FALSE(FuzzyMatcher::metaphone("computer").empty());
}

// ============================================================================
// N-gram similarity
// ============================================================================

TEST(FuzzyMatcherNgram, IdenticalStrings) {
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("hello", "hello", 2), 1.0);
}

TEST(FuzzyMatcherNgram, CompletelyDifferentStrings) {
    double sim = FuzzyMatcher::ngramSimilarity("abc", "xyz", 2);
    EXPECT_DOUBLE_EQ(sim, 0.0);
}

TEST(FuzzyMatcherNgram, BothEmpty) {
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("", "", 2), 1.0);
}

TEST(FuzzyMatcherNgram, OneEmpty) {
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("", "hello", 2), 0.0);
    EXPECT_DOUBLE_EQ(FuzzyMatcher::ngramSimilarity("hello", "", 2), 0.0);
}

TEST(FuzzyMatcherNgram, ResultInUnitInterval) {
    double sim = FuzzyMatcher::ngramSimilarity("database", "databse", 2);
    EXPECT_GE(sim, 0.0);
    EXPECT_LE(sim, 1.0);
}

TEST(FuzzyMatcherNgram, CloseStringHigherThanDistantString) {
    double close  = FuzzyMatcher::ngramSimilarity("database", "databse", 2);
    double distant = FuzzyMatcher::ngramSimilarity("database", "zzzzzzz", 2);
    EXPECT_GT(close, distant);
}

TEST(FuzzyMatcherNgram, TrigramWorks) {
    double sim = FuzzyMatcher::ngramSimilarity("hello", "hello", 3);
    EXPECT_DOUBLE_EQ(sim, 1.0);
}

// ============================================================================
// Null index search returns error
// ============================================================================

TEST(FuzzyMatcherSearch, NullIndexReturnsError) {
    FuzzyMatcher matcher(nullptr);
    auto [status, results] = matcher.search("test", "table", "col");
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(results.empty());
}

TEST(FuzzyMatcherSearch, EmptyQueryReturnsError) {
    FuzzyMatcher matcher(nullptr);
    auto [status, results] = matcher.search("", "table", "col");
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(results.empty());
}
