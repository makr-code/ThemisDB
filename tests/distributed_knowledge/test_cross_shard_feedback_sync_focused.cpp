/**
 * @file test_cross_shard_feedback_sync_focused.cpp
 * @brief Focused unit tests for cross-shard feedback synchronization.
 *
 * Tests the core distributed knowledge feedback sync paths against the actual
 * production API surface:
 * - FeedbackSummary creation and JSON round-trip
 * - CrossShardFeedbackSync publish/receive lifecycle
 * - Deduplication by summary_id
 * - Inbound policy check and ZeroTrust enforcer
 * - Observability counters (publishedCount, receivedCount, deduplicatedCount)
 *
 * Target: Production readiness validation for feedback sync layer.
 * Q3 2026 Hardening: dedup semantics, policy-edge feedback filtering, determinism.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <chrono>

#include "distributed_knowledge/cross_shard_feedback_sync.h"

using namespace themis::distributed_knowledge;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class CrossShardFeedbackSyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.max_embedding_dim       = 4;   // Small dim for tests
        cfg_.dedup_cache_size        = 100;
        cfg_.validate_embedding_dim  = true;
        gossip_messages_.clear();
    }

    FeedbackSyncConfig cfg_;
    std::vector<json>  gossip_messages_;

    auto make_gossip_fn() {
        return [this](json msg) { gossip_messages_.push_back(std::move(msg)); };
    }

    /// Build a minimal FeedbackSummary with a 4-float embedding.
    static FeedbackSummary makeSummary(const std::string& id,
                                        const std::string& type_label = "USER_NEGATIVE") {
        FeedbackSummary s;
        s.summary_id          = id;
        s.feedback_type_label = type_label;
        s.reason_embedding    = {0.1f, 0.2f, 0.3f, 0.4f};
        s.rlaif_round         = 1;
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FeedbackSummary Tests (CSS-01..CSS-03)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test CSS-01: FeedbackSummary creation with required fields.
 *
 * Verifies that a FeedbackSummary can be constructed and its fields
 * are accessible as documented.
 */
TEST_F(CrossShardFeedbackSyncTest, CreateFeedbackSummary) {
    FeedbackSummary s = makeSummary("sum-001", "USER_NEGATIVE");

    EXPECT_EQ(s.summary_id,          "sum-001");
    EXPECT_EQ(s.feedback_type_label, "USER_NEGATIVE");
    EXPECT_EQ(s.reason_embedding.size(), 4u);
    EXPECT_EQ(s.rlaif_round, 1u);
}

/**
 * @test CSS-02: FeedbackSummary JSON serialization round-trip.
 *
 * Verifies that toJson() / fromJson() preserve all fields.
 */
TEST_F(CrossShardFeedbackSyncTest, FeedbackSummaryJsonRoundTrip) {
    FeedbackSummary original = makeSummary("sum-round-trip", "SECURITY_ISSUE");
    original.shard_origin = "ANON";
    original.rlaif_round  = 7;

    json j = original.toJson();
    FeedbackSummary restored = FeedbackSummary::fromJson(j);

    EXPECT_EQ(restored.summary_id,          original.summary_id);
    EXPECT_EQ(restored.feedback_type_label, original.feedback_type_label);
    EXPECT_EQ(restored.shard_origin,        "ANON");
    EXPECT_EQ(restored.rlaif_round,         7u);
    EXPECT_EQ(restored.reason_embedding.size(), original.reason_embedding.size());
}

/**
 * @test CSS-03: FeedbackSummary shard_origin defaults to ANON.
 *
 * Verifies that the default value for shard_origin is "ANON" as required
 * by the privacy contract.
 */
TEST_F(CrossShardFeedbackSyncTest, FeedbackSummaryDefaultShardOriginIsAnon) {
    FeedbackSummary s;
    EXPECT_EQ(s.shard_origin, "ANON");
}

// ─────────────────────────────────────────────────────────────────────────────
// Publish/Receive Tests (CSS-04..CSS-06)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test CSS-04: publishFeedback() dispatches a gossip message.
 *
 * Verifies that publishFeedback() calls the gossip function exactly once
 * and increments publishedCount().
 */
TEST_F(CrossShardFeedbackSyncTest, PublishFeedbackDispatchesGossipMessage) {
    CrossShardFeedbackSync sync(cfg_, "shard-001", make_gossip_fn());

    sync.publishFeedback(makeSummary("sum-001"));

    EXPECT_EQ(sync.publishedCount(), 1u);
    ASSERT_EQ(gossip_messages_.size(), 1u);
}

/**
 * @test CSS-05: handleInboundSummary() invokes the registered callback.
 *
 * Verifies that after setting a feedback callback, processing an inbound
 * summary payload calls the callback with the correct summary_id.
 */
TEST_F(CrossShardFeedbackSyncTest, HandleInboundSummaryInvokesCallback) {
    CrossShardFeedbackSync sync(cfg_, "shard-002", make_gossip_fn());

    std::string received_id;
    sync.setFeedbackCallback([&](const FeedbackSummary& s) {
        received_id = s.summary_id;
    });

    FeedbackSummary original = makeSummary("sum-inbound");
    sync.handleInboundSummary(original.toJson());

    EXPECT_EQ(received_id, "sum-inbound");
    EXPECT_EQ(sync.receivedCount(), 1u);
}

/**
 * @test CSS-06: publishFeedback() throws when embedding dimension is wrong.
 *
 * Verifies that the embedding dimension validator rejects a summary whose
 * reason_embedding length differs from max_embedding_dim.
 */
TEST_F(CrossShardFeedbackSyncTest, PublishFeedbackRejectsWrongEmbeddingDim) {
    CrossShardFeedbackSync sync(cfg_, "shard-001", make_gossip_fn());

    FeedbackSummary bad = makeSummary("sum-bad");
    bad.reason_embedding = {0.1f, 0.2f};  // dim=2, expected 4

    EXPECT_THROW(sync.publishFeedback(bad), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// Deduplication Tests (CSS-07..CSS-08)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test CSS-07: Duplicate inbound summary_id is deduplicated.
 *
 * Verifies that sending the same payload twice results in receivedCount()==1
 * and deduplicatedCount()==1.
 */
TEST_F(CrossShardFeedbackSyncTest, DuplicateSummaryIsDeduped) {
    CrossShardFeedbackSync sync(cfg_, "shard-002", make_gossip_fn());

    FeedbackSummary s = makeSummary("sum-dup");
    json payload = s.toJson();

    sync.handleInboundSummary(payload);
    sync.handleInboundSummary(payload);  // duplicate

    EXPECT_EQ(sync.receivedCount(),      1u);
    EXPECT_EQ(sync.deduplicatedCount(),  1u);
}

/**
 * @test CSS-08: Multiple distinct summaries all received.
 *
 * Verifies that five distinct inbound summaries are all processed
 * (no false-positive deduplication).
 */
TEST_F(CrossShardFeedbackSyncTest, MultipleDistinctSummariesAllReceived) {
    CrossShardFeedbackSync sync(cfg_, "shard-002", make_gossip_fn());

    for (int i = 0; i < 5; ++i) {
        FeedbackSummary s = makeSummary("sum-" + std::to_string(i));
        sync.handleInboundSummary(s.toJson());
    }

    EXPECT_EQ(sync.receivedCount(),     5u);
    EXPECT_EQ(sync.deduplicatedCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Policy and ZeroTrust Tests (CSS-09..CSS-10)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test CSS-09: Inbound policy check rejects summaries.
 *
 * Verifies that a policy check returning false causes the summary to be
 * silently dropped and rejectedByPolicyCount() is incremented.
 */
TEST_F(CrossShardFeedbackSyncTest, InboundPolicyCheckRejectsSummary) {
    CrossShardFeedbackSync sync(cfg_, "shard-002", make_gossip_fn());

    sync.setInboundPolicyCheck([](const FeedbackSummary&) { return false; });

    bool callback_called = false;
    sync.setFeedbackCallback([&](const FeedbackSummary&) { callback_called = true; });

    sync.handleInboundSummary(makeSummary("sum-rejected").toJson());

    EXPECT_FALSE(callback_called);
    EXPECT_EQ(sync.rejectedByPolicyCount(), 1u);
}

/**
 * @test CSS-10: ZeroTrust enforcer returning false throws on high-risk summary.
 *
 * Verifies that when the ZeroTrust enforcer returns false, handleInboundSummary()
 * throws std::runtime_error for the high-risk summary.
 */
TEST_F(CrossShardFeedbackSyncTest, ZeroTrustEnforcerThrowsOnHighRisk) {
    CrossShardFeedbackSync sync(cfg_, "shard-002", make_gossip_fn());
    sync.setZeroTrustEnforcer([](const FeedbackSummary&) { return false; });

    EXPECT_THROW(sync.handleInboundSummary(makeSummary("sum-risky").toJson()),
                 std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Observability and Config Tests (CSS-11..CSS-12)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test CSS-11: getStats() returns a JSON object with expected counters.
 *
 * Verifies that getStats() returns a JSON object containing at minimum
 * "published_count" and "received_count" after some operations.
 */
TEST_F(CrossShardFeedbackSyncTest, GetStatsReturnsJsonWithCounters) {
    CrossShardFeedbackSync sync(cfg_, "shard-001", make_gossip_fn());
    sync.publishFeedback(makeSummary("sum-stats-1"));
    sync.publishFeedback(makeSummary("sum-stats-2"));

    json stats = sync.getStats();
    EXPECT_TRUE(stats.contains("published_count"));
    EXPECT_EQ(stats["published_count"].get<size_t>(), 2u);
}

/**
 * @test CSS-12: FeedbackSyncConfig validation.
 *
 * Verifies that a correctly constructed FeedbackSyncConfig reports
 * isValid() == true, and that degenerate configs (dim=0) are rejected.
 */
TEST_F(CrossShardFeedbackSyncTest, FeedbackSyncConfigValidation) {
    EXPECT_TRUE(cfg_.isValid());

    FeedbackSyncConfig bad_cfg;
    bad_cfg.max_embedding_dim = 0;
    EXPECT_FALSE(bad_cfg.isValid());
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Summary
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Test Coverage Summary (CSS-01..CSS-12):
 *
 * CSS-01: FeedbackSummary creation with required fields
 * CSS-02: FeedbackSummary JSON serialization round-trip
 * CSS-03: FeedbackSummary shard_origin defaults to ANON
 * CSS-04: publishFeedback() dispatches a gossip message
 * CSS-05: handleInboundSummary() invokes the registered callback
 * CSS-06: publishFeedback() throws on wrong embedding dimension
 * CSS-07: Duplicate inbound summary_id is deduplicated
 * CSS-08: Multiple distinct summaries all received
 * CSS-09: Inbound policy check rejects summaries
 * CSS-10: ZeroTrust enforcer throws on high-risk summary
 * CSS-11: getStats() returns JSON with expected counters
 * CSS-12: FeedbackSyncConfig validation
 *
 * Target: Q3 2026 Hardening - dedup semantics, replay prevention, privacy filtering.
 * Status: Focused unit test suite for cross-shard feedback sync layer.
 */
