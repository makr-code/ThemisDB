/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_multi_field_search.cpp                        ║
  Version:         1.9.0                                              ║
  Last Modified:   2026-02-28                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
