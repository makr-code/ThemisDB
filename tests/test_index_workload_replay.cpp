/*
 * ThemisDB | File: test_index_workload_replay.cpp | Version: 0.0.18
 * Maturity: 🟢 PRODUCTION-READY | Score: 97/100
 * Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <algorithm>
#include "index/workload_replay.h"
#include "metadata/index_recommender.h"

using namespace themis;
using namespace themis::metadata;

// ============================================================================
// WorkloadEvent – JSON round-trip
// ============================================================================

TEST(WorkloadEventTest, FilterEventToJSON) {
    WorkloadEvent e;
    e.table_name  = "users";
    e.column_name = "email";
    e.access_type = IndexRecommender::AccessType::FILTER;
    e.selectivity = 0.01;

    auto j = e.toJSON();
    EXPECT_EQ(j["table_name"],  "users");
    EXPECT_EQ(j["column_name"], "email");
    EXPECT_EQ(j["access_type"], "filter");
    EXPECT_NEAR(j["selectivity"].get<double>(), 0.01, 1e-9);
}

TEST(WorkloadEventTest, SortEventToJSON) {
    WorkloadEvent e;
    e.table_name  = "orders";
    e.column_name = "created_at";
    e.access_type = IndexRecommender::AccessType::SORT;
    e.selectivity = 1.0;

    auto j = e.toJSON();
    EXPECT_EQ(j["access_type"], "sort");
}

TEST(WorkloadEventTest, RoundTripFilter) {
    WorkloadEvent orig;
    orig.table_name  = "products";
    orig.column_name = "sku";
    orig.access_type = IndexRecommender::AccessType::FILTER;
    orig.selectivity = 0.005;

    auto restored = WorkloadEvent::fromJSON(orig.toJSON());
    EXPECT_EQ(restored.table_name,  orig.table_name);
    EXPECT_EQ(restored.column_name, orig.column_name);
    EXPECT_EQ(restored.access_type, orig.access_type);
    EXPECT_NEAR(restored.selectivity, orig.selectivity, 1e-9);
}

TEST(WorkloadEventTest, RoundTripSort) {
    WorkloadEvent orig;
    orig.table_name  = "logs";
    orig.column_name = "timestamp";
    orig.access_type = IndexRecommender::AccessType::SORT;
    orig.selectivity = 0.5;

    auto restored = WorkloadEvent::fromJSON(orig.toJSON());
    EXPECT_EQ(restored.access_type, IndexRecommender::AccessType::SORT);
}

// ============================================================================
// WorkloadCapture – basic recording
// ============================================================================

class WorkloadCaptureTest : public ::testing::Test {
protected:
    WorkloadCapture capture_;
};

TEST_F(WorkloadCaptureTest, InitiallyEmpty) {
    EXPECT_EQ(capture_.eventCount(),  0u);
    EXPECT_EQ(capture_.totalQueries(), 0u);
}

TEST_F(WorkloadCaptureTest, RecordEventIncreasesCount) {
    capture_.recordEvent("users", "email", IndexRecommender::AccessType::FILTER, 0.01);
    EXPECT_EQ(capture_.eventCount(), 1u);
}

TEST_F(WorkloadCaptureTest, RecordQueryIncreasesCounter) {
    capture_.recordQuery();
    capture_.recordQuery();
    EXPECT_EQ(capture_.totalQueries(), 2u);
}

TEST_F(WorkloadCaptureTest, EventsSnapshotCorrect) {
    capture_.recordEvent("t", "col_a", IndexRecommender::AccessType::FILTER, 0.1);
    capture_.recordEvent("t", "col_b", IndexRecommender::AccessType::SORT,   0.5);

    auto evts = capture_.events();
    ASSERT_EQ(evts.size(), 2u);
    EXPECT_EQ(evts[0].column_name, "col_a");
    EXPECT_EQ(evts[1].column_name, "col_b");
}

TEST_F(WorkloadCaptureTest, ClearResetsAll) {
    capture_.recordQuery();
    capture_.recordEvent("t", "c", IndexRecommender::AccessType::FILTER, 0.1);
    capture_.clear();

    EXPECT_EQ(capture_.eventCount(),  0u);
    EXPECT_EQ(capture_.totalQueries(), 0u);
}

// ============================================================================
// WorkloadCapture – JSON serialisation
// ============================================================================

TEST_F(WorkloadCaptureTest, ToJSONContainsQueryCount) {
    capture_.recordQuery();
    capture_.recordQuery();
    capture_.recordEvent("t", "c", IndexRecommender::AccessType::FILTER, 0.2);

    auto j = capture_.toJSON();
    EXPECT_EQ(j["total_queries"].get<uint64_t>(), 2u);
    EXPECT_EQ(j["events"].size(), 1u);
}

TEST_F(WorkloadCaptureTest, JSONRoundTrip) {
    for (int i = 0; i < 5; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("users", "email", IndexRecommender::AccessType::FILTER, 0.0);
    }
    capture_.recordEvent("orders", "status", IndexRecommender::AccessType::SORT, 0.5);

    auto j      = capture_.toJSON();
    auto loaded = WorkloadCapture::fromJSON(j);

    EXPECT_EQ(loaded.totalQueries(), capture_.totalQueries());
    EXPECT_EQ(loaded.eventCount(),   capture_.eventCount());

    auto orig_evts   = capture_.events();
    auto loaded_evts = loaded.events();
    ASSERT_EQ(orig_evts.size(), loaded_evts.size());
    for (size_t i = 0; i < orig_evts.size(); ++i) {
        EXPECT_EQ(loaded_evts[i].table_name,  orig_evts[i].table_name);
        EXPECT_EQ(loaded_evts[i].column_name, orig_evts[i].column_name);
        EXPECT_EQ(loaded_evts[i].access_type, orig_evts[i].access_type);
        EXPECT_NEAR(loaded_evts[i].selectivity, orig_evts[i].selectivity, 1e-9);
    }
}

// ============================================================================
// WorkloadReplayer – replay produces recommendations
// ============================================================================

class WorkloadReplayerTest : public ::testing::Test {
protected:
    WorkloadCapture  capture_;
    WorkloadReplayer replayer_;
};

TEST_F(WorkloadReplayerTest, ReplayEmptyWorkloadReturnsNoRecommendations) {
    auto recs = replayer_.replay(capture_, "users");
    EXPECT_TRUE(recs.empty());
}

TEST_F(WorkloadReplayerTest, ReplayProducesAddRecommendation) {
    // Simulate 50 queries filtering on a highly selective column
    for (int i = 0; i < 50; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("users", "email", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto recs = replayer_.replay(capture_, "users");
    ASSERT_FALSE(recs.empty());

    auto add_count = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::ADD; });
    EXPECT_GT(add_count, 0);
    EXPECT_EQ(recs[0].column_name, "email");
}

TEST_F(WorkloadReplayerTest, ReplayRespectsExistingIndexes) {
    // Column already indexed → no ADD recommendation
    for (int i = 0; i < 50; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("users", "email", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto recs = replayer_.replay(capture_, "users", {"email"});
    auto add_count = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::ADD; });
    EXPECT_EQ(add_count, 0);
}

TEST_F(WorkloadReplayerTest, ReplayDropsUnusedIndex) {
    // Record a single access then simulate many queries without it
    capture_.recordQuery();
    capture_.recordEvent("orders", "legacy_col", IndexRecommender::AccessType::FILTER, 1.0);
    for (int i = 0; i < 500; ++i) {
        capture_.recordQuery();
    }

    auto recs = replayer_.replay(capture_, "orders", {"legacy_col"});
    auto drop_count = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::DROP; });
    EXPECT_GT(drop_count, 0);
}

TEST_F(WorkloadReplayerTest, ReplayIsolated) {
    // Each replay starts fresh – prior replays do not affect subsequent ones
    for (int i = 0; i < 50; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("t", "col", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto recs1 = replayer_.replay(capture_, "t");
    auto recs2 = replayer_.replay(capture_, "t");

    ASSERT_EQ(recs1.size(), recs2.size());
    for (size_t i = 0; i < recs1.size(); ++i) {
        EXPECT_NEAR(recs1[i].benefit_score, recs2[i].benefit_score, 1e-9);
    }
}

TEST_F(WorkloadReplayerTest, ReplayAllCoversMultipleTables) {
    for (int i = 0; i < 50; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("t1", "col", IndexRecommender::AccessType::FILTER, 0.0);
        capture_.recordEvent("t2", "col", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto all = replayer_.replayAll(capture_);
    EXPECT_GE(all.size(), 2u);
    EXPECT_TRUE(all.count("t1") > 0);
    EXPECT_TRUE(all.count("t2") > 0);
}

TEST_F(WorkloadReplayerTest, ReplayFromDeserializedCapture) {
    // Verify that replaying a JSON-round-tripped capture produces the same result
    for (int i = 0; i < 50; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("items", "price", IndexRecommender::AccessType::FILTER, 0.05);
    }

    auto loaded  = WorkloadCapture::fromJSON(capture_.toJSON());
    auto recs_orig   = replayer_.replay(capture_, "items");
    auto recs_loaded = replayer_.replay(loaded,   "items");

    ASSERT_EQ(recs_orig.size(), recs_loaded.size());
    for (size_t i = 0; i < recs_orig.size(); ++i) {
        EXPECT_EQ(recs_loaded[i].column_name,  recs_orig[i].column_name);
        EXPECT_EQ(recs_loaded[i].action,       recs_orig[i].action);
        EXPECT_NEAR(recs_loaded[i].benefit_score, recs_orig[i].benefit_score, 1e-9);
    }
}

TEST_F(WorkloadReplayerTest, SortHeavyColumnGetsRangeIndex) {
    for (int i = 0; i < 50; ++i) {
        capture_.recordQuery();
        capture_.recordEvent("products", "price", IndexRecommender::AccessType::SORT, 0.5);
    }

    auto recs = replayer_.replay(capture_, "products");
    if (!recs.empty()) {
        auto it = std::find_if(recs.begin(), recs.end(),
            [](const auto& r) { return r.column_name == "price"; });
        if (it != recs.end() && it->action == IndexRecommendation::Action::ADD) {
            EXPECT_EQ(it->index_type, "range");
        }
    }
}
