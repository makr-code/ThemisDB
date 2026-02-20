// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include "metadata/index_recommender.h"

using namespace themis;

class IndexRecommenderTest : public ::testing::Test {
protected:
    IndexRecommender rec_;
};

// ============================================================================
// ColumnAccess / IndexRecommendation JSON
// ============================================================================

TEST(IndexRecommenderStructTest, ColumnAccessToJSON) {
    ColumnAccess ca;
    ca.table_name       = "users";
    ca.column_name      = "email";
    ca.filter_count     = 10;
    ca.sort_count       = 2;
    ca.avg_selectivity  = 0.01;

    auto j = ca.toJSON();
    EXPECT_EQ(j["table_name"],      "users");
    EXPECT_EQ(j["column_name"],     "email");
    EXPECT_EQ(j["filter_count"],    10u);
    EXPECT_EQ(j["sort_count"],      2u);
    EXPECT_NEAR(j["avg_selectivity"].get<double>(), 0.01, 1e-9);
}

TEST(IndexRecommenderStructTest, IndexRecommendationToJSONAdd) {
    IndexRecommendation r;
    r.table_name    = "orders";
    r.column_name   = "status";
    r.index_type    = "regular";
    r.action        = IndexRecommendation::Action::ADD;
    r.benefit_score = 75.0;
    r.rationale     = "High filter frequency";

    auto j = r.toJSON();
    EXPECT_EQ(j["action"],        "ADD");
    EXPECT_EQ(j["column_name"],   "status");
    EXPECT_NEAR(j["benefit_score"].get<double>(), 75.0, 1e-6);
}

TEST(IndexRecommenderStructTest, IndexRecommendationToJSONDrop) {
    IndexRecommendation r;
    r.action = IndexRecommendation::Action::DROP;
    auto j = r.toJSON();
    EXPECT_EQ(j["action"], "DROP");
}

// ============================================================================
// recordAccess + recordQuery
// ============================================================================

TEST_F(IndexRecommenderTest, RecordAccessUpdatesCounts) {
    rec_.recordQuery();
    rec_.recordAccess("users", "email", IndexRecommender::AccessType::FILTER, 0.05);

    auto stats = rec_.getAccessStats("users");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].column_name,  "email");
    EXPECT_EQ(stats[0].filter_count, 1u);
    EXPECT_EQ(stats[0].sort_count,   0u);
}

TEST_F(IndexRecommenderTest, RecordSortAccess) {
    rec_.recordQuery();
    rec_.recordAccess("orders", "created_at", IndexRecommender::AccessType::SORT, 1.0);

    auto stats = rec_.getAccessStats("orders");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].sort_count,   1u);
    EXPECT_EQ(stats[0].filter_count, 0u);
}

TEST_F(IndexRecommenderTest, MultipleAccessesAccumulateCounts) {
    for (int i = 0; i < 5; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("items", "price", IndexRecommender::AccessType::FILTER, 0.1);
    }
    auto stats = rec_.getAccessStats("items");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].filter_count, 5u);
}

TEST_F(IndexRecommenderTest, RecordAccessMultipleColumns) {
    rec_.recordQuery();
    rec_.recordAccess("t", "col_a", IndexRecommender::AccessType::FILTER, 0.1);
    rec_.recordAccess("t", "col_b", IndexRecommender::AccessType::FILTER, 0.5);

    auto stats = rec_.getAccessStats("t");
    EXPECT_EQ(stats.size(), 2u);
}

// ============================================================================
// getAccessStats
// ============================================================================

TEST_F(IndexRecommenderTest, GetAccessStatsUnknownTable) {
    auto stats = rec_.getAccessStats("nonexistent");
    EXPECT_TRUE(stats.empty());
}

// ============================================================================
// recommend
// ============================================================================

TEST_F(IndexRecommenderTest, RecommendEmptyStats) {
    auto recs = rec_.recommend("no_table");
    EXPECT_TRUE(recs.empty());
}

TEST_F(IndexRecommenderTest, RecommendAddIndex) {
    // Record many filter accesses on a very selective column
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("users", "email", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto recs = rec_.recommend("users");
    ASSERT_FALSE(recs.empty());

    auto add_recs = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::ADD; });
    EXPECT_GT(add_recs, 0);

    EXPECT_EQ(recs[0].column_name, "email");
    EXPECT_GE(recs[0].benefit_score, IndexRecommender::kAddThreshold);
}

TEST_F(IndexRecommenderTest, RecommendDropUnusedIndex) {
    // Record a very small number of accesses
    rec_.recordQuery();
    rec_.recordAccess("orders", "legacy_col", IndexRecommender::AccessType::FILTER, 1.0);

    // Add many more queries without using legacy_col
    for (int i = 0; i < 500; ++i) {
        rec_.recordQuery();
    }

    auto recs = rec_.recommend("orders", {"legacy_col"});
    auto drop_recs = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::DROP; });
    EXPECT_GT(drop_recs, 0);
}

TEST_F(IndexRecommenderTest, NoRecommendForAlreadyIndexedHighBenefitColumn) {
    // Column with high benefit, already indexed → no ADD
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.0);
    }
    auto recs = rec_.recommend("t", {"col"});  // col already indexed
    auto add_recs = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::ADD; });
    EXPECT_EQ(add_recs, 0);
}

TEST_F(IndexRecommenderTest, RecommendSortPreferRangeIndex) {
    // Sort-heavy column should get "range" index type recommended
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("products", "price", IndexRecommender::AccessType::SORT, 0.5);
    }

    auto recs = rec_.recommend("products");
    // If a recommendation exists, sort-heavy should suggest range
    if (!recs.empty()) {
        auto it = std::find_if(recs.begin(), recs.end(),
            [](const auto& r) { return r.column_name == "price"; });
        if (it != recs.end() && it->action == IndexRecommendation::Action::ADD) {
            EXPECT_EQ(it->index_type, "range");
        }
    }
}

TEST_F(IndexRecommenderTest, RecommendSortedByBenefitDesc) {
    for (int i = 0; i < 10; ++i) rec_.recordQuery();
    rec_.recordAccess("t", "high_col", IndexRecommender::AccessType::FILTER, 0.0);
    rec_.recordAccess("t", "high_col", IndexRecommender::AccessType::FILTER, 0.0);
    rec_.recordAccess("t", "low_col",  IndexRecommender::AccessType::FILTER, 0.9);

    // Add a lot more queries so benefit normalises differently
    for (int i = 0; i < 10; ++i) rec_.recordQuery();
    rec_.recordAccess("t", "high_col", IndexRecommender::AccessType::FILTER, 0.0);

    auto recs = rec_.recommend("t");
    for (size_t i = 1; i < recs.size(); ++i) {
        EXPECT_GE(recs[i-1].benefit_score, recs[i].benefit_score);
    }
}

// ============================================================================
// recommendAll
// ============================================================================

TEST_F(IndexRecommenderTest, RecommendAllMultipleTables) {
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("t1", "col", IndexRecommender::AccessType::FILTER, 0.0);
        rec_.recordAccess("t2", "col", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto all = rec_.recommendAll();
    EXPECT_GE(all.size(), 2u);
    EXPECT_TRUE(all.count("t1"));
    EXPECT_TRUE(all.count("t2"));
}

// ============================================================================
// reset
// ============================================================================

TEST_F(IndexRecommenderTest, ResetClearsStats) {
    rec_.recordQuery();
    rec_.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.1);
    EXPECT_FALSE(rec_.getAccessStats("t").empty());

    rec_.reset();
    EXPECT_TRUE(rec_.getAccessStats("t").empty());
}

// ============================================================================
// toJSON
// ============================================================================

TEST_F(IndexRecommenderTest, ToJSONStructure) {
    rec_.recordQuery();
    rec_.recordAccess("t", "a", IndexRecommender::AccessType::FILTER, 0.1);
    rec_.recordAccess("t", "b", IndexRecommender::AccessType::SORT,   0.5);

    auto j = rec_.toJSON();
    EXPECT_TRUE(j.contains("t"));
    EXPECT_TRUE(j["t"].is_array());
    EXPECT_EQ(j["t"].size(), 2u);
}
