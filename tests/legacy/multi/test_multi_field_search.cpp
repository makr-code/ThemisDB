/**
 * @file test_multi_field_search.cpp
 * @brief Unit tests for MultiFieldBoostedSearch (v1.9.0)
 *
 * Tests cover: config validation, defaultFields(), normalizeScores (via
 * public API), null-index safety, empty-query/empty-fields short-circuits,
 * and the k-cap property. Integration with live backends is exercised in
 * test_hybrid_search_integration.cpp.
 */

#include <gtest/gtest.h>
#include "search/multi_field_search.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(MultiFieldBoostedSearchConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(MultiFieldBoostedSearch(nullptr));
}

TEST(MultiFieldBoostedSearchConfig, ZeroKThrows) {
    MultiFieldBoostedSearch::Config cfg;
    cfg.k = 0;
    EXPECT_THROW(MultiFieldBoostedSearch(nullptr, cfg), std::invalid_argument);
}

TEST(MultiFieldBoostedSearchConfig, ZeroCandidatesPerFieldThrows) {
    MultiFieldBoostedSearch::Config cfg;
    cfg.candidates_per_field = 0;
    EXPECT_THROW(MultiFieldBoostedSearch(nullptr, cfg), std::invalid_argument);
}

TEST(MultiFieldBoostedSearchConfig, ConfigRoundtrip) {
    MultiFieldBoostedSearch::Config cfg;
    cfg.k = 5;
    cfg.candidates_per_field = 50;
    MultiFieldBoostedSearch mfs(nullptr, cfg);
    EXPECT_EQ(mfs.getConfig().k, 5u);
    EXPECT_EQ(mfs.getConfig().candidates_per_field, 50u);
}

// ============================================================================
// defaultFields()
// ============================================================================

TEST(MultiFieldBoostedSearchDefaultFields, ReturnsThreeFields) {
    auto fields = MultiFieldBoostedSearch::defaultFields("articles");
    ASSERT_EQ(fields.size(), 3u);
}

TEST(MultiFieldBoostedSearchDefaultFields, CorrectTableNames) {
    auto fields = MultiFieldBoostedSearch::defaultFields("articles");
    for (const auto& f : fields) {
        EXPECT_EQ(f.table, "articles");
    }
}

TEST(MultiFieldBoostedSearchDefaultFields, CorrectColumnNames) {
    auto fields = MultiFieldBoostedSearch::defaultFields("posts");
    EXPECT_EQ(fields[0].column, "title");
    EXPECT_EQ(fields[1].column, "body");
    EXPECT_EQ(fields[2].column, "tags");
}

TEST(MultiFieldBoostedSearchDefaultFields, TitleHasHighestBoost) {
    auto fields = MultiFieldBoostedSearch::defaultFields("docs");
    // title > body > tags
    EXPECT_GT(fields[0].boost, fields[1].boost);   // title > body
    EXPECT_GT(fields[1].boost, fields[2].boost);   // body  > tags
    EXPECT_GT(fields[0].boost, 0.0);
    EXPECT_GT(fields[1].boost, 0.0);
    EXPECT_GT(fields[2].boost, 0.0);
}

TEST(MultiFieldBoostedSearchDefaultFields, DefaultBoostValues) {
    auto fields = MultiFieldBoostedSearch::defaultFields("t");
    EXPECT_DOUBLE_EQ(fields[0].boost, 3.0);  // title
    EXPECT_DOUBLE_EQ(fields[1].boost, 1.0);  // body
    EXPECT_DOUBLE_EQ(fields[2].boost, 0.5);  // tags
}

// ============================================================================
// FieldConfig defaults
// ============================================================================

TEST(MultiFieldBoostedSearchFieldConfig, DefaultBoostIsOne) {
    MultiFieldBoostedSearch::FieldConfig fc;
    EXPECT_DOUBLE_EQ(fc.boost, 1.0);
}

TEST(MultiFieldBoostedSearchFieldConfig, DefaultTableAndColumnAreEmpty) {
    MultiFieldBoostedSearch::FieldConfig fc;
    EXPECT_TRUE(fc.table.empty());
    EXPECT_TRUE(fc.column.empty());
}

// ============================================================================
// Result defaults
// ============================================================================

TEST(MultiFieldBoostedSearchResult, DefaultInitialization) {
    MultiFieldBoostedSearch::Result r;
    EXPECT_TRUE(r.document_id.empty());
    EXPECT_DOUBLE_EQ(r.score, 0.0);
    EXPECT_TRUE(r.field_scores.empty());
}

// ============================================================================
// search() — null index and empty input short-circuits
// ============================================================================

TEST(MultiFieldBoostedSearchSearch, NullIndexReturnsEmpty) {
    MultiFieldBoostedSearch mfs(nullptr);
    auto fields = MultiFieldBoostedSearch::defaultFields("articles");
    auto results = mfs.search("hello", fields);
    EXPECT_TRUE(results.empty());
}

TEST(MultiFieldBoostedSearchSearch, EmptyQueryReturnsEmpty) {
    MultiFieldBoostedSearch mfs(nullptr);
    auto fields = MultiFieldBoostedSearch::defaultFields("articles");
    auto results = mfs.search("", fields);
    EXPECT_TRUE(results.empty());
}

TEST(MultiFieldBoostedSearchSearch, EmptyFieldsReturnsEmpty) {
    MultiFieldBoostedSearch mfs(nullptr);
    auto results = mfs.search("hello", {});
    EXPECT_TRUE(results.empty());
}

TEST(MultiFieldBoostedSearchSearch, NullIndexNullStatsDoesNotThrow) {
    MultiFieldBoostedSearch mfs(nullptr);
    EXPECT_NO_THROW(mfs.search("hello", {}));
}

// ============================================================================
// k cap
// ============================================================================

TEST(MultiFieldBoostedSearchSearch, KCapPreserved) {
    MultiFieldBoostedSearch::Config cfg;
    cfg.k = 3;
    MultiFieldBoostedSearch mfs(nullptr, cfg);
    EXPECT_EQ(mfs.getConfig().k, 3u);
    // With null index → empty result, still doesn't throw
    auto results = mfs.search("test", MultiFieldBoostedSearch::defaultFields("t"));
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// normalizeScores — tested indirectly via FieldConfig with zero boost
// ============================================================================

TEST(MultiFieldBoostedSearchSearch, ZeroBoostFieldIsSkippedGracefully) {
    // A field with boost=0.0 is valid; it contributes 0 to the combined score
    // but should not crash.
    MultiFieldBoostedSearch mfs(nullptr);
    std::vector<MultiFieldBoostedSearch::FieldConfig> fields = {
        {"t", "col", 0.0}
    };
    EXPECT_NO_THROW(mfs.search("hello", fields));
}

TEST(MultiFieldBoostedSearchSearch, NegativeBoostFieldSkippedGracefully) {
    // Negative boost is invalid per field; should be skipped without crashing.
    MultiFieldBoostedSearch mfs(nullptr);
    std::vector<MultiFieldBoostedSearch::FieldConfig> fields = {
        {"t", "col", -1.0}
    };
    EXPECT_NO_THROW(mfs.search("hello", fields));
    auto results = mfs.search("hello", fields);
    EXPECT_TRUE(results.empty());
}

TEST(MultiFieldBoostedSearchSearch, EmptyTableInFieldSkippedGracefully) {
    MultiFieldBoostedSearch mfs(nullptr);
    std::vector<MultiFieldBoostedSearch::FieldConfig> fields = {
        {"", "col", 1.0}
    };
    EXPECT_NO_THROW(mfs.search("hello", fields));
}

TEST(MultiFieldBoostedSearchSearch, EmptyColumnInFieldSkippedGracefully) {
    MultiFieldBoostedSearch mfs(nullptr);
    std::vector<MultiFieldBoostedSearch::FieldConfig> fields = {
        {"table", "", 1.0}
    };
    EXPECT_NO_THROW(mfs.search("hello", fields));
}

// ============================================================================
// defaultFields() — different table names
// ============================================================================

TEST(MultiFieldBoostedSearchDefaultFields, EmptyTableName) {
    // defaultFields should work even with an empty table name
    auto fields = MultiFieldBoostedSearch::defaultFields("");
    ASSERT_EQ(fields.size(), 3u);
    for (const auto& f : fields) {
        EXPECT_TRUE(f.table.empty());
    }
}

TEST(MultiFieldBoostedSearchDefaultFields, UniqueColumnNames) {
    auto fields = MultiFieldBoostedSearch::defaultFields("t");
    // All column names should be distinct
    EXPECT_NE(fields[0].column, fields[1].column);
    EXPECT_NE(fields[1].column, fields[2].column);
    EXPECT_NE(fields[0].column, fields[2].column);
}

// ============================================================================
// setConfig()
// ============================================================================

TEST(MultiFieldBoostedSearchConfig, SetConfigUpdatesK) {
    MultiFieldBoostedSearch mfs(nullptr);
    MultiFieldBoostedSearch::Config cfg;
    cfg.k = 42;
    cfg.candidates_per_field = 200;
    mfs.setConfig(cfg);
    EXPECT_EQ(mfs.getConfig().k, 42u);
    EXPECT_EQ(mfs.getConfig().candidates_per_field, 200u);
}

// ============================================================================
// normalizeScores() — public static, tested directly
// ============================================================================

// Helper: build a pair list
static std::vector<std::pair<std::string, double>> makePairs(
    std::initializer_list<std::pair<std::string, double>> items) {
    return {items};
}

TEST(MultiFieldNormalizeScores, EmptyIsNoOp) {
    std::vector<std::pair<std::string, double>> empty;
    EXPECT_NO_THROW(MultiFieldBoostedSearch::normalizeScores(empty));
    EXPECT_TRUE(empty.empty());
}

TEST(MultiFieldNormalizeScores, SinglePositiveScoreBecomesOne) {
    auto v = makePairs({{"doc1", 3.7}});
    MultiFieldBoostedSearch::normalizeScores(v);
    EXPECT_DOUBLE_EQ(v[0].second, 1.0);
}

TEST(MultiFieldNormalizeScores, SingleZeroScoreBecomesZero) {
    auto v = makePairs({{"doc1", 0.0}});
    MultiFieldBoostedSearch::normalizeScores(v);
    EXPECT_DOUBLE_EQ(v[0].second, 0.0);
}

TEST(MultiFieldNormalizeScores, AllEqualPositiveBecomesOne) {
    auto v = makePairs({{"a", 5.0}, {"b", 5.0}, {"c", 5.0}});
    MultiFieldBoostedSearch::normalizeScores(v);
    for (const auto& p : v) {
        EXPECT_DOUBLE_EQ(p.second, 1.0);
    }
}

TEST(MultiFieldNormalizeScores, AllEqualZeroBecomesZero) {
    auto v = makePairs({{"a", 0.0}, {"b", 0.0}});
    MultiFieldBoostedSearch::normalizeScores(v);
    for (const auto& p : v) {
        EXPECT_DOUBLE_EQ(p.second, 0.0);
    }
}

TEST(MultiFieldNormalizeScores, RangeNormalizationIsCorrect) {
    // min=1, max=5 -> range=4; doc "a"->(1-1)/4=0.0; "b"->(3-1)/4=0.5; "c"->(5-1)/4=1.0
    auto v = makePairs({{"a", 1.0}, {"b", 3.0}, {"c", 5.0}});
    MultiFieldBoostedSearch::normalizeScores(v);
    EXPECT_DOUBLE_EQ(v[0].second, 0.0);
    EXPECT_DOUBLE_EQ(v[1].second, 0.5);
    EXPECT_DOUBLE_EQ(v[2].second, 1.0);
}

TEST(MultiFieldNormalizeScores, DocIdsPreservedAfterNormalization) {
    auto v = makePairs({{"alpha", 10.0}, {"beta", 20.0}});
    MultiFieldBoostedSearch::normalizeScores(v);
    EXPECT_EQ(v[0].first, "alpha");
    EXPECT_EQ(v[1].first, "beta");
}

TEST(MultiFieldNormalizeScores, NormalizedBoundsAreZeroToOne) {
    auto v = makePairs({{"a", 2.0}, {"b", 4.0}, {"c", 8.0}});
    MultiFieldBoostedSearch::normalizeScores(v);
    for (const auto& p : v) {
        EXPECT_GE(p.second, 0.0);
        EXPECT_LE(p.second, 1.0);
    }
    // Min becomes 0.0 and max becomes 1.0
    double min_s = std::min_element(v.begin(), v.end(),
        [](const auto& a, const auto& b){ return a.second < b.second; })->second;
    double max_s = std::max_element(v.begin(), v.end(),
        [](const auto& a, const auto& b){ return a.second < b.second; })->second;
    EXPECT_DOUBLE_EQ(min_s, 0.0);
    EXPECT_DOUBLE_EQ(max_s, 1.0);
}
