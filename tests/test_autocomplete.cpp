/**
 * @file test_autocomplete.cpp
 * @brief Unit tests for AutocompleteEngine (v1.5.0)
 */

#include <gtest/gtest.h>
#include "search/autocomplete.h"
#include "search/search_analytics.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(AutocompleteConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(AutocompleteEngine(nullptr, nullptr));
}

TEST(AutocompleteConfig, ZeroMaxSuggestionsThrows) {
    AutocompleteEngine::Config cfg;
    cfg.max_suggestions = 0;
    EXPECT_THROW(AutocompleteEngine(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(AutocompleteConfig, ZeroMinPrefixLengthThrows) {
    AutocompleteEngine::Config cfg;
    cfg.min_prefix_length = 0;
    EXPECT_THROW(AutocompleteEngine(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(AutocompleteConfig, ConfigRoundtrip) {
    AutocompleteEngine::Config cfg;
    cfg.max_suggestions = 5;
    cfg.min_prefix_length = 2;
    AutocompleteEngine ac(nullptr, nullptr, cfg);
    EXPECT_EQ(ac.getConfig().max_suggestions, 5u);
    EXPECT_EQ(ac.getConfig().min_prefix_length, 2u);
}

// ============================================================================
// suggest() — prefix too short
// ============================================================================

TEST(AutocompleteSuggest, PrefixBelowMinLengthReturnsEmpty) {
    AutocompleteEngine::Config cfg;
    cfg.min_prefix_length = 3;
    AutocompleteEngine ac(nullptr, nullptr, cfg);
    auto suggestions = ac.suggest("ab");
    EXPECT_TRUE(suggestions.empty());
}

TEST(AutocompleteSuggest, EmptyPrefixReturnsEmpty) {
    AutocompleteEngine ac(nullptr, nullptr);
    auto suggestions = ac.suggest("");
    EXPECT_TRUE(suggestions.empty());
}

// ============================================================================
// suggestPopular() — backed by SearchAnalytics
// ============================================================================

TEST(AutocompletePopular, NullAnalyticsReturnsEmpty) {
    AutocompleteEngine ac(nullptr, nullptr);
    auto suggestions = ac.suggestPopular("data", 10);
    EXPECT_TRUE(suggestions.empty());
}

TEST(AutocompletePopular, PopularQueriesMatchedByPrefix) {
    SearchAnalytics analytics;
    // Record several queries
    for (int i = 0; i < 5; ++i) {
      analytics.record("database query", 3, 10.0);
    }
    for (int i = 0; i < 3; ++i) {
      analytics.record("data migration", 2, 8.0);
    }
    analytics.record("graph search", 1, 5.0);

    AutocompleteEngine ac(nullptr, &analytics);
    auto suggestions = ac.suggestPopular("data", 10);

    // Should have "database query" and "data migration" but not "graph search"
    EXPECT_GE(suggestions.size(), 1u);
    for (const auto& s : suggestions) {
        EXPECT_EQ(s.text.rfind("data", 0), 0u); // starts with "data"
        EXPECT_TRUE(s.is_popular);
    }
}

TEST(AutocompletePopular, MostFrequentQueryRankedFirst) {
    SearchAnalytics analytics;
    for (int i = 0; i < 10; ++i) {
      analytics.record("database index", 5, 5.0);
    }
    for (int i = 0; i < 2; ++i) {
      analytics.record("data warehouse", 3, 8.0);
    }

    AutocompleteEngine ac(nullptr, &analytics);
    auto suggestions = ac.suggestPopular("data", 10);

    ASSERT_GE(suggestions.size(), 2u);
    // "database index" (10 occurrences) should rank above "data warehouse" (2)
    EXPECT_EQ(suggestions[0].text, "database index");
}

TEST(AutocompletePopular, MaxLimitHonored) {
    SearchAnalytics analytics;
    for (int i = 0; i < 10; ++i) {
      analytics.record("data" + std::to_string(i), 1, 1.0);
    }

    AutocompleteEngine ac(nullptr, &analytics);
    auto suggestions = ac.suggestPopular("data", 3);
    EXPECT_LE(suggestions.size(), 3u);
}

// ============================================================================
// suggestByPrefix() — null index
// ============================================================================

TEST(AutocompleteByPrefix, NullIndexReturnsEmpty) {
    AutocompleteEngine ac(nullptr, nullptr);
    auto suggestions = ac.suggestByPrefix("data", "table", "col", 10);
    EXPECT_TRUE(suggestions.empty());
}

TEST(AutocompleteByPrefix, EmptyPrefixReturnsEmpty) {
    AutocompleteEngine ac(nullptr, nullptr);
    auto suggestions = ac.suggestByPrefix("", "table", "col", 10);
    EXPECT_TRUE(suggestions.empty());
}

TEST(AutocompleteByPrefix, EmptyTableReturnsEmpty) {
    AutocompleteEngine ac(nullptr, nullptr);
    auto suggestions = ac.suggestByPrefix("dat", "", "col", 10);
    EXPECT_TRUE(suggestions.empty());
}

// ============================================================================
// suggest() — combined with analytics
// ============================================================================

TEST(AutocompleteSuggest, CombinesPopularAndPrefix) {
    SearchAnalytics analytics;
    for (int i = 0; i < 3; ++i) {
      analytics.record("dataset analysis", 4, 6.0);
    }

    AutocompleteEngine::Config cfg;
    cfg.include_prefix = false; // disable prefix scan (no index)
    AutocompleteEngine ac(nullptr, &analytics, cfg);

    auto suggestions = ac.suggest("data");
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0].text, "dataset analysis");
}

TEST(AutocompleteSuggest, DeduplicatesResults) {
    SearchAnalytics analytics;
    analytics.record("database", 5, 3.0);

    AutocompleteEngine::Config cfg;
    cfg.include_prefix = false;
    cfg.deduplicate = true;
    AutocompleteEngine ac(nullptr, &analytics, cfg);

    // "database" should appear exactly once even if it comes from multiple sources
    auto suggestions = ac.suggest("data");
    size_t count = 0;
    for (const auto& s : suggestions) {
        if (s.text == "database") {
          ++count;
        }
    }
    EXPECT_LE(count, 1u);
}

TEST(AutocompleteSuggest, MaxSuggestionsHonored) {
    SearchAnalytics analytics;
    for (int i = 0; i < 20; ++i) {
      analytics.record("data" + std::to_string(i), 1, 1.0);
    }

    AutocompleteEngine::Config cfg;
    cfg.max_suggestions = 5;
    cfg.include_prefix = false;
    AutocompleteEngine ac(nullptr, &analytics, cfg);

    auto suggestions = ac.suggest("data");
    EXPECT_LE(suggestions.size(), 5u);
}

TEST(AutocompleteSuggest, DisabledPopularAndNoIndexReturnsEmpty) {
    AutocompleteEngine::Config cfg;
    cfg.include_popular = false;
    cfg.include_prefix  = false;
    AutocompleteEngine ac(nullptr, nullptr, cfg);
    auto suggestions = ac.suggest("data");
    EXPECT_TRUE(suggestions.empty());
}

// ============================================================================
// Suggestion struct
// ============================================================================

TEST(SuggestionStruct, DefaultInitialization) {
    Suggestion s;
    EXPECT_TRUE(s.text.empty());
    EXPECT_DOUBLE_EQ(s.score, 0.0);
    EXPECT_FALSE(s.is_popular);
}
