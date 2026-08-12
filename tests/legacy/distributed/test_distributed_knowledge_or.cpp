// Copyright 2026 ThemisDB — Licensed under MIT License
// DK-OR / S-15: Operational Resilience unit tests
// DK-8  / S-14: Memory-leak regression test
//
// Test groups:
//   DK-OR-B-01..02  Backpressure (round timeout, non-blocking publish)
//   DK-OR-T-01..03  Timeout / Circuit Breaker (shard timeout in RAG merger)
//   DK-OR-E-01..02  Error Signal Paths (NaN guard, audit record on round)
//   DK-OR-S-01..02  Security Integration (ZeroTrust enforcer)
//   DK-OR-H-01..03  GDPR Hardening (erase on all four components)
//   DK-8-ML-01      Memory-Leak regression (50 federation rounds)
//
// All tests: no network, no file I/O.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "distributed_knowledge/adapter_capability_announcement.h"
#include "distributed_knowledge/cross_shard_feedback_sync.h"
#include "distributed_knowledge/federated_rag_merger.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::distributed_knowledge;
using namespace std::chrono_literals;

// ============================================================================
// Helpers
// ============================================================================

namespace {

FederationConfig makeFedConfig(size_t min_participants = 2) {
    FederationConfig cfg;
    cfg.min_participants = min_participants;
    cfg.dp_epsilon       = 0.5;
    cfg.dp_delta         = 1e-5;
    cfg.dp_sensitivity   = 1.0;
    return cfg;
}

EncryptedGradient makeGrad(const std::string& shard_id,
                            uint64_t round    = 1,
                            size_t   num_keys = 2) {
    EncryptedGradient g;
    g.shard_id     = shard_id;
    g.round        = round;
    g.sample_count = 100;
    for (size_t i = 0; i < num_keys; ++i) {
        g.data["key_" + std::to_string(i)] = 0.01 * static_cast<double>(i + 1);
    }
    return g;
}

ShardRetrievalResult makeShardResult(const std::string& shard_id,
                                      size_t num_docs = 5) {
    ShardRetrievalResult sr;
    sr.shard_id  = shard_id;
    sr.ok        = true;
    sr.timed_out = false;
    for (size_t i = 0; i < num_docs; ++i) {
        RetrievedDocument doc;
        doc.doc_id          = shard_id + "-doc-" + std::to_string(i);
        doc.content         = "content";
        doc.shard_id        = shard_id;
        doc.relevance_score = 1.0 / static_cast<double>(i + 1);
        doc.rank_in_shard   = i + 1;
        sr.documents.push_back(std::move(doc));
    }
    return sr;
}

FeedbackSummary makeFS(const std::string& summary_id,
                        size_t embed_dim = 4) {
    FeedbackSummary fs;
    fs.summary_id          = summary_id;
    fs.feedback_type_label = "USER_NEGATIVE";
    fs.shard_origin        = "ANON";
    fs.reason_embedding.assign(embed_dim, 0.1f);
    return fs;
}

CrossShardFeedbackSync makeSyncWithSink(
    std::vector<nlohmann::json>& sent,
    const std::string& shard_id = "test-shard",
    size_t embed_dim = 4)
{
    FeedbackSyncConfig cfg;
    cfg.max_embedding_dim      = embed_dim;
    cfg.validate_embedding_dim = true;
    cfg.dedup_cache_size       = 10000;
    return CrossShardFeedbackSync(
        cfg, shard_id,
        [&sent](nlohmann::json j) { sent.push_back(std::move(j)); });
}

/// Read current RSS from /proc/self/status in bytes (Linux only).
size_t getCurrentRSS() {
#ifdef __linux__
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            size_t kb = 0;
            std::istringstream iss(line.substr(6));
            iss >> kb;
            return kb * 1024;
        }
    }
#endif
    return 0;
}

} // anonymous namespace

// ============================================================================
// DK-OR-B — Backpressure
// ============================================================================

// DK-OR-B-1: triggerAggregation(timeout_ms=0) → throws std::runtime_error
// (timeout_ms=0 is the immediate-timeout sentinel value)
TEST(DK_OR_Backpressure, DK_OR_B_01_TriggerAggregationTimesOut) {
    auto cfg = makeFedConfig(2);
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGrad("s1", 1));
    coord.submitGradient(makeGrad("s2", 1));

    // timeout_ms=0 always throws immediately
    EXPECT_THROW(coord.triggerAggregation(0), std::runtime_error);
}

// DK-OR-B-2: publishFeedback() with gossip sink that throws → no re-throw,
//             getSkippedPublishCount() == 1
TEST(DK_OR_Backpressure, DK_OR_B_02_PublishFeedbackNonBlocking) {
    FeedbackSyncConfig cfg;
    cfg.max_embedding_dim      = 4;
    cfg.validate_embedding_dim = true;

    // Gossip sink that throws (simulates full queue / backpressure)
    CrossShardFeedbackSync sync(cfg, "shard-pub",
        [](nlohmann::json) { throw std::runtime_error("queue full"); });

    // Must NOT propagate the gossip exception
    EXPECT_NO_THROW(sync.publishFeedback(makeFS("skp-1")));
    EXPECT_EQ(sync.getSkippedPublishCount(), 1u);
}

// ============================================================================
// DK-OR-T — Timeout / Circuit Breaker (FederatedRAGMerger)
// ============================================================================

// DK-OR-T-1: One of three shards has timed_out=true → merge returns docs from
//             the remaining two shards only; shards_responded == 2
TEST(DK_OR_Timeout, DK_OR_T_01_OneShardTimedOutIsSkipped) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k            = 20;
    cfg.shard_timeout_ms = 1000;
    FederatedRAGMerger merger(cfg);

    auto s1 = makeShardResult("s1", 5);
    auto s2 = makeShardResult("s2", 5);
    auto s3 = makeShardResult("s3", 5);
    s3.timed_out = true;  // this shard timed out

    auto ctx = merger.merge({s1, s2, s3});

    for (const auto& doc : ctx.documents) {
        EXPECT_NE(doc.shard_id, "s3") << "timed-out shard should be excluded";
    }
    EXPECT_EQ(ctx.shards_queried, 3u);
    EXPECT_EQ(ctx.shards_responded, 2u);
}

// DK-OR-T-2: All shards have timed_out=true → throws "all shards timed out"
TEST(DK_OR_Timeout, DK_OR_T_02_AllShardsTimedOutThrows) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k            = 10;
    cfg.shard_timeout_ms = 1000;
    FederatedRAGMerger merger(cfg);

    auto s1 = makeShardResult("s1", 3);
    auto s2 = makeShardResult("s2", 3);
    s1.timed_out = true;
    s2.timed_out = true;

    try {
        merger.merge({s1, s2});
        FAIL() << "Expected std::runtime_error for all shards timed out";
    } catch (const std::runtime_error& e) {
        EXPECT_THAT(std::string(e.what()),
                    ::testing::HasSubstr("timed out"));
    }
}

// DK-OR-T-3: shard_timeout_ms=0 → immediate std::runtime_error even with data
TEST(DK_OR_Timeout, DK_OR_T_03_ZeroTimeoutThrowsImmediately) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k            = 10;
    cfg.shard_timeout_ms = 0;
    FederatedRAGMerger merger(cfg);

    auto s1 = makeShardResult("s1", 3);

    EXPECT_THROW(merger.merge({s1}), std::runtime_error);
}

// ============================================================================
// DK-OR-E — Error Signal Paths
// ============================================================================

// DK-OR-E-1: doAggregation() with NaN in gradient data
//             → triggerAggregation() throws std::runtime_error
TEST(DK_OR_ErrorSignal, DK_OR_E_01_NaNInGradientThrows) {
    auto cfg = makeFedConfig(2);
    LoRAFederationCoordinator coord(cfg);

    EncryptedGradient g1;
    g1.shard_id     = "s1";
    g1.round        = 1;
    g1.sample_count = 100;
    g1.data["key_0"] = std::numeric_limits<double>::quiet_NaN();

    coord.submitGradient(g1);
    coord.submitGradient(makeGrad("s2", 1));

    EXPECT_THROW(coord.triggerAggregation(), std::runtime_error);
}

// DK-OR-E-2: Successful round → setAuditRecordCallback() is invoked with
//             decision_type="FEDERATED_ROUND"
TEST(DK_OR_ErrorSignal, DK_OR_E_02_AuditCallbackInvokedOnSuccess) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping audit callback focused test on Windows due to unstable callback invocation in current focused configuration.";
#endif
    auto cfg = makeFedConfig(2);
    LoRAFederationCoordinator coord(cfg);

    std::vector<nlohmann::json> audit_records;
    coord.setAuditRecordCallback([&](const nlohmann::json& rec) {
        audit_records.push_back(rec);
    });

    coord.submitGradient(makeGrad("s1", 1));
    coord.submitGradient(makeGrad("s2", 1));
    coord.triggerAggregation();

    ASSERT_EQ(audit_records.size(), 1u);
    EXPECT_EQ(audit_records[0].value("decision_type", ""), "FEDERATED_ROUND");
}

// ============================================================================
// DK-OR-S — Security Integration (ZeroTrust enforcer)
// ============================================================================

// DK-OR-S-1: handleInboundSummary() with ZeroTrust enforcer returning false
//             (risk=HIGH) → throws std::runtime_error
TEST(DK_OR_Security, DK_OR_S_01_ZeroTrustHighRiskRejects) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    sync.setZeroTrustEnforcer([](const FeedbackSummary&) -> bool { return false; });

    EXPECT_THROW(sync.handleInboundSummary(makeFS("zt-1").toJson()),
                 std::runtime_error);
}

// DK-OR-S-2: handleInboundSummary() with ZeroTrust enforcer returning true
//             (risk=LOW) → callback is invoked normally
TEST(DK_OR_Security, DK_OR_S_02_ZeroTrustLowRiskAllows) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    sync.setZeroTrustEnforcer([](const FeedbackSummary&) -> bool { return true; });

    std::vector<FeedbackSummary> received;
    sync.setFeedbackCallback([&](const FeedbackSummary& s) { received.push_back(s); });

    EXPECT_NO_THROW(sync.handleInboundSummary(makeFS("zt-2").toJson()));
    EXPECT_EQ(received.size(), 1u);
}

// ============================================================================
// DK-OR-H — GDPR Hardening (erase operations)
// ============================================================================

// DK-OR-H-1: LoRAFederationCoordinator::erase() clears pending gradients and
//             resets round to 0; eraseCount() increments
TEST(DK_OR_Hardening, DK_OR_H_01_FederationCoordinatorErase) {
    auto cfg = makeFedConfig(2);
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGrad("s1", 1));
    EXPECT_EQ(coord.submittedCount(), 1u);
    EXPECT_GT(coord.currentRound(), 0u);

    auto result = coord.erase("subject-1");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(coord.submittedCount(), 0u);
    EXPECT_EQ(coord.currentRound(), 0u);
    EXPECT_EQ(coord.eraseCount(), 1u);
}

// DK-OR-H-2: FederatedRAGMerger::erase() increments eraseCount(); success
TEST(DK_OR_Hardening, DK_OR_H_02_FederatedRAGMergerErase) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 10;
    FederatedRAGMerger merger(cfg);

    EXPECT_EQ(merger.eraseCount(), 0u);
    auto result = merger.erase("subject-1");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(merger.eraseCount(), 1u);
}

// DK-OR-H-3: CrossShardFeedbackSync::erase() clears dedup cache;
//             previously-seen summary_id is accepted again after erase
TEST(DK_OR_Hardening, DK_OR_H_03_CrossShardFeedbackSyncErase) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    sync.handleInboundSummary(makeFS("e-1").toJson());
    sync.handleInboundSummary(makeFS("e-2").toJson());
    EXPECT_EQ(sync.receivedCount(), 2u);

    auto result = sync.erase("subject-x");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sync.eraseCount(), 1u);

    // After erase, the same summary_id should be accepted again
    sync.handleInboundSummary(makeFS("e-1").toJson());
    EXPECT_EQ(sync.receivedCount(), 3u) << "erase should have cleared dedup cache";
}

// ============================================================================
// DK-8 Memory-Leak regression
// ============================================================================

// DK-8-ML-01: Run 50 federation rounds and verify RSS growth <= 5 MB.
// If /proc/self/status is unavailable (non-Linux), the test still exercises
// the code paths but skips the RSS assertion.
TEST(DK_8_MemoryLeak, DK_8_ML_01_NoLeakOver50Rounds) {
    auto cfg = makeFedConfig(3);
    LoRAFederationCoordinator coord(cfg);

    // Warm up to avoid first-allocation noise
    for (int s = 0; s < 3; ++s) {
        coord.submitGradient(makeGrad("shard-" + std::to_string(s), 1, 10));
    }
    EXPECT_NO_THROW(coord.triggerAggregation());

    const size_t rss_before = getCurrentRSS();

    for (int round = 0; round < 50; ++round) {
        const uint64_t r = coord.currentRound();
        for (int s = 0; s < 3; ++s) {
            coord.submitGradient(makeGrad("shard-" + std::to_string(s), r, 100));
        }
        EXPECT_NO_THROW(coord.triggerAggregation());
    }

    const size_t rss_after = getCurrentRSS();

    if (rss_before > 0 && rss_after > 0) {
        const size_t growth = (rss_after > rss_before) ? (rss_after - rss_before) : 0;
        EXPECT_LE(growth, 5u * 1024u * 1024u)
            << "RSS grew by " << growth / 1024 << " KB over 50 rounds";
    }
}
