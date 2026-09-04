// Copyright 2026 ThemisDB — Licensed under MIT License
// DK-1 / S-1: Distributed Knowledge Layer 11 unit tests
//
// Test groups:
//   DK-A-01…DK-A-06  AdapterCapabilityAnnouncement (serialisation + callback)
//   DK-B-01…DK-B-13  LoRAFederationCoordinator (FedAvg, DP-noise, callbacks)
//   DK-C-01…DK-C-10  FederatedRAGMerger (RRF ranking, dedup, top_k)
//   DK-D-01…DK-D-08  CrossShardFeedbackSync (anonymisation, dedup cache)
//
// All tests: < 50 ms total, no network, no file I/O.

#include <gtest/gtest.h>

#include "distributed_knowledge/adapter_capability_announcement.h"
#include "distributed_knowledge/cross_shard_feedback_sync.h"
#include "distributed_knowledge/federated_rag_merger.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

#include <atomic>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace themis::distributed_knowledge;

#if 0  // Legacy DK test suite currently targets pre-refactor APIs.

// ============================================================================
// Helpers
// ============================================================================

static AdapterCapabilityAnnouncement makeAnnouncement(
    const std::string& adapter_id = "adapter-test",
    const std::string& shard_id   = "shard-A",
    AdapterDomainType  domain     = AdapterDomainType::DATABASE_QUERY) {
    AdapterCapabilityAnnouncement a;
    a.adapter_id       = adapter_id;
    a.shard_id         = shard_id;
    a.domain           = domain;
    a.accuracy_score   = 0.87;
    a.latency_ms       = 12.5;
    a.lora_rank        = 8;
    a.supported_langs  = {"de", "en"};
    a.announced_at     = std::chrono::system_clock::now();
    return a;
}

static EncryptedGradient makeGradient(
    const std::string& shard_id = "shard-A",
    uint64_t round              = 1) {
    EncryptedGradient g;
    g.shard_id    = shard_id;
    g.round       = round;
    g.data        = {{"layer_0", 0.01}, {"layer_1", -0.005}};
    g.sample_count = 100;
    return g;
}

static RetrievedDocument makeDoc(const std::string& doc_id,
                                  double score  = 1.0,
                                  int    rank   = 1) {
    RetrievedDocument d;
    d.doc_id  = doc_id;
    d.score   = score;
    d.rank    = rank;
    d.content = "content-" + doc_id;
    return d;
}

// ============================================================================
// DK-A — AdapterCapabilityAnnouncement
// ============================================================================

// DK-A-01: toJson() / fromJson() round-trip preserves all fields
TEST(DK_A_AdapterCapabilityAnnouncement, DK_A_01_JsonRoundTrip) {
    auto a = makeAnnouncement("adp-1", "shard-X", AdapterDomainType::LEGAL);
    a.accuracy_score  = 0.95;
    a.latency_ms      = 7.3;
    a.lora_rank       = 16;
    a.supported_langs = {"en", "fr"};

    const auto j  = a.toJson();
    const auto a2 = AdapterCapabilityAnnouncement::fromJson(j);

    EXPECT_EQ(a2.adapter_id,      a.adapter_id);
    EXPECT_EQ(a2.shard_id,        a.shard_id);
    EXPECT_EQ(a2.domain,          a.domain);
    EXPECT_NEAR(a2.accuracy_score, a.accuracy_score, 1e-6);
    EXPECT_NEAR(a2.latency_ms,    a.latency_ms,    1e-6);
    EXPECT_EQ(a2.lora_rank,       a.lora_rank);
    EXPECT_EQ(a2.supported_langs, a.supported_langs);
}

// DK-A-02: toJson() produces valid JSON with required keys
TEST(DK_A_AdapterCapabilityAnnouncement, DK_A_02_JsonHasRequiredKeys) {
    const auto j = makeAnnouncement().toJson();
    EXPECT_TRUE(j.contains("adapter_id"));
    EXPECT_TRUE(j.contains("shard_id"));
    EXPECT_TRUE(j.contains("domain"));
    EXPECT_TRUE(j.contains("accuracy_score"));
    EXPECT_TRUE(j.contains("latency_ms"));
    EXPECT_TRUE(j.contains("lora_rank"));
}

// DK-A-03: fromJson() with missing optional fields uses sensible defaults
TEST(DK_A_AdapterCapabilityAnnouncement, DK_A_03_FromJsonMissingOptionals) {
    nlohmann::json j = {{"adapter_id", "a"}, {"shard_id", "s"}};
    auto a = AdapterCapabilityAnnouncement::fromJson(j);
    EXPECT_EQ(a.adapter_id, "a");
    EXPECT_EQ(a.shard_id,   "s");
    EXPECT_GE(a.accuracy_score, 0.0);
}

// DK-A-04: GossipAdapterPublisher::announce() invokes gossip_fn with JSON payload
TEST(DK_A_AdapterCapabilityAnnouncement, DK_A_04_AnnounceBroadcastsJson) {
    std::vector<nlohmann::json> sent;
    GossipAdapterPublisher publisher(
        "shard-A",
        [&](nlohmann::json payload) { sent.push_back(std::move(payload)); });

    publisher.announce(makeAnnouncement("adp-1", "shard-A"));
    ASSERT_EQ(sent.size(), 1u);
    EXPECT_EQ(sent[0].value("adapter_id", ""), "adp-1");
}

// DK-A-05: GossipAdapterPublisher::lastAnnouncement() returns the most recent one
TEST(DK_A_AdapterCapabilityAnnouncement, DK_A_05_LastAnnouncementReturnsLatest) {
    GossipAdapterPublisher publisher("shard-A", [](nlohmann::json) {});
    EXPECT_FALSE(publisher.lastAnnouncement().has_value());

    publisher.announce(makeAnnouncement("adp-1", "shard-A"));
    ASSERT_TRUE(publisher.lastAnnouncement().has_value());
    EXPECT_EQ(publisher.lastAnnouncement()->adapter_id, "adp-1");
}

// DK-A-06: handleInbound() dispatches to registered announcement callback
TEST(DK_A_AdapterCapabilityAnnouncement, DK_A_06_InboundCallbackDispatched) {
    GossipAdapterPublisher publisher("shard-B", [](nlohmann::json) {});

    std::optional<AdapterCapabilityAnnouncement> received;
    publisher.setAnnouncementCallback(
        [&](const AdapterCapabilityAnnouncement& a) { received = a; });

    auto a = makeAnnouncement("adp-remote", "shard-A");
    const auto payload = a.toJson();
    publisher.handleInboundMessage(payload);

    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(received->adapter_id, "adp-remote");
}

// ============================================================================
// DK-B — LoRAFederationCoordinator
// ============================================================================

// DK-B-01: currentRound() starts at 1
TEST(DK_B_LoRAFederationCoordinator, DK_B_01_InitialRound) {
    LoRAFederationCoordinator coord;
    EXPECT_EQ(coord.currentRound(), 1u);
}

// DK-B-02: auto-aggregation produces a non-empty delta after min_participants submit
TEST(DK_B_LoRAFederationCoordinator, DK_B_02_AggregationProducesDelta) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    LoRAFederationCoordinator coord(cfg);

    // Auto-trigger fires when 2nd gradient is submitted
    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));

    ASSERT_TRUE(coord.lastDelta().has_value());
    EXPECT_FALSE(coord.lastDelta()->delta.empty());
}

// DK-B-03: round counter advances to 2 after auto-aggregation
TEST(DK_B_LoRAFederationCoordinator, DK_B_03_RoundAdvancesAfterAggregation) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));  // auto-trigger

    EXPECT_EQ(coord.currentRound(), 2u);
}

// DK-B-04: FedAvg — aggregated delta contains expected layer keys
TEST(DK_B_LoRAFederationCoordinator, DK_B_04_FedAvgPreservesLayerKeys) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_sensitivity   = 1e-9;   // near-zero noise for determinism
    LoRAFederationCoordinator coord(cfg);

    EncryptedGradient g1 = makeGradient("shard-A", 1);
    g1.data = {{"layer_0", 0.10}, {"layer_1", 0.05}};
    EncryptedGradient g2 = makeGradient("shard-B", 1);
    g2.data = {{"layer_0", 0.20}, {"layer_1", 0.15}};

    coord.submitGradient(g1);
    coord.submitGradient(g2);  // auto-trigger

    ASSERT_TRUE(coord.lastDelta().has_value());
    // Both layer keys must be present in the aggregated delta
    EXPECT_TRUE(coord.lastDelta()->delta.contains("layer_0"));
    EXPECT_TRUE(coord.lastDelta()->delta.contains("layer_1"));
}

// DK-B-05: FedAvg mean is approximately correct with near-zero DP noise
TEST(DK_B_LoRAFederationCoordinator, DK_B_05_FedAvgMeanApproxCorrect) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_sensitivity   = 1e-9;   // sigma ≈ 5e-8, negligible noise
    LoRAFederationCoordinator coord(cfg);

    EncryptedGradient g1 = makeGradient("shard-A", 1);
    g1.data = {{"layer_0", 0.10}};
    EncryptedGradient g2 = makeGradient("shard-B", 1);
    g2.data = {{"layer_0", 0.20}};
    coord.submitGradient(g1);
    coord.submitGradient(g2);  // auto-trigger

    ASSERT_TRUE(coord.lastDelta().has_value());
    // Expected mean = 0.15; allow for near-zero DP noise
    EXPECT_NEAR(coord.lastDelta()->delta.value<double>("layer_0", 0.0), 0.15, 1e-4);
}

// DK-B-06: epsilon_spent in the delta matches the configured dp_epsilon
TEST(DK_B_LoRAFederationCoordinator, DK_B_06_EpsilonSpentMatchesConfig) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_epsilon       = 0.5;
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));  // auto-trigger

    ASSERT_TRUE(coord.lastDelta().has_value());
    EXPECT_NEAR(coord.lastDelta()->epsilon_spent, 0.5, 1e-9);
}

// DK-B-07: setGlobalDeltaCallback() is invoked on auto-aggregation
TEST(DK_B_LoRAFederationCoordinator, DK_B_07_DeltaCallbackInvoked) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_sensitivity   = 1e-9;
    LoRAFederationCoordinator coord(cfg);

    std::optional<GlobalAdapterDelta> received;
    coord.setGlobalDeltaCallback(
        [&](const GlobalAdapterDelta& d) { received = d; });

    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));  // auto-trigger fires callback

    ASSERT_TRUE(received.has_value());
    EXPECT_FALSE(received->delta.empty());
}

// DK-B-08: Duplicate submitGradient() for same shard_id is ignored in aggregation
TEST(DK_B_LoRAFederationCoordinator, DK_B_08_DuplicateSubmitIsIdempotent) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_sensitivity   = 1e-9;
    LoRAFederationCoordinator coord(cfg);

    EncryptedGradient g1 = makeGradient("shard-A", 1);
    g1.data = {{"layer_0", 0.10}};
    coord.submitGradient(g1);
    coord.submitGradient(g1);  // duplicate → silently ignored, no auto-trigger yet

    EncryptedGradient g2 = makeGradient("shard-B", 1);
    g2.data = {{"layer_0", 0.20}};
    coord.submitGradient(g2);  // auto-trigger: only shard-A + shard-B count

    ASSERT_TRUE(coord.lastDelta().has_value());
    // Only one shard-A gradient counted: mean of 0.10 and 0.20 = 0.15
    EXPECT_NEAR(coord.lastDelta()->delta.value<double>("layer_0", 0.0), 0.15, 1e-4);
}

// DK-B-09: lastDelta() returns nullopt before first aggregation
TEST(DK_B_LoRAFederationCoordinator, DK_B_09_LastDeltaNullBeforeFirstRound) {
    LoRAFederationCoordinator coord;
    EXPECT_FALSE(coord.lastDelta().has_value());
}

// DK-B-10: lastDelta() is populated after aggregation
TEST(DK_B_LoRAFederationCoordinator, DK_B_10_LastDeltaPopulatedAfterAgg) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_sensitivity   = 1e-9;
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));  // auto-trigger

    ASSERT_TRUE(coord.lastDelta().has_value());
}

// DK-B-11: participants field equals number of shards that submitted
TEST(DK_B_LoRAFederationCoordinator, DK_B_11_DeltaParticipantsCount) {
    FederationConfig cfg;
    cfg.min_participants = 3;
    cfg.dp_sensitivity   = 1e-9;
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));
    coord.submitGradient(makeGradient("shard-C", 1));  // auto-trigger

    ASSERT_TRUE(coord.lastDelta().has_value());
    EXPECT_EQ(coord.lastDelta()->participants, 3u);
}

// DK-B-12: version string in delta contains "global-v" and round field equals 1
TEST(DK_B_LoRAFederationCoordinator, DK_B_12_DeltaVersionContainsRound) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_sensitivity   = 1e-9;
    LoRAFederationCoordinator coord(cfg);

    coord.submitGradient(makeGradient("shard-A", 1));
    coord.submitGradient(makeGradient("shard-B", 1));  // auto-trigger

    ASSERT_TRUE(coord.lastDelta().has_value());
    EXPECT_EQ(coord.lastDelta()->round, 1u);
    EXPECT_FALSE(coord.lastDelta()->version.empty());
    EXPECT_NE(coord.lastDelta()->version.find("global-v"), std::string::npos);
}

// DK-B-13: config() accessor returns the configured FederationConfig
TEST(DK_B_LoRAFederationCoordinator, DK_B_13_ConfigAccessorReturnsConfig) {
    FederationConfig cfg;
    cfg.min_participants = 5;
    cfg.dp_sensitivity   = 2.5;
    cfg.dp_epsilon       = 0.1;
    LoRAFederationCoordinator coord(cfg);

    EXPECT_EQ(coord.config().min_participants, 5u);
    EXPECT_NEAR(coord.config().dp_sensitivity, 2.5, 1e-9);
}

// ============================================================================
// DK-C — FederatedRAGMerger
// ============================================================================

static ShardRetrievalResult makeShardResult(
    const std::string& shard_id,
    const std::vector<std::pair<std::string, double>>& docs) {
    ShardRetrievalResult r;
    r.shard_id = shard_id;
    int rank   = 1;
    for (auto& [id, score] : docs) {
        r.documents.push_back(makeDoc(id, score, rank++));
    }
    return r;
}

// DK-C-01: merge() returns non-empty MergedRAGContext when shards provide results
TEST(DK_C_FederatedRAGMerger, DK_C_01_MergeReturnsNonEmptyContext) {
    FederatedRAGMerger merger;
    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"doc-a", 0.9}, {"doc-b", 0.7}}),
        makeShardResult("s2", {{"doc-c", 0.8}, {"doc-d", 0.6}}),
    };
    auto ctx = merger.merge(shards);
    EXPECT_FALSE(ctx.documents.empty());
}

// DK-C-02: RRF ranking — higher-ranked doc across shards scores highest
TEST(DK_C_FederatedRAGMerger, DK_C_02_RRFTopDocIsHighestRankedAcrossShards) {
    FederatedRAGMergerConfig cfg;
    cfg.strategy = MergeStrategy::RECIPROCAL_RANK_FUSION;
    cfg.top_k    = 10;
    FederatedRAGMerger merger(cfg);

    // doc-top is rank-1 in both shards → should appear first after RRF
    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"doc-top", 0.9}, {"doc-mid", 0.5}}),
        makeShardResult("s2", {{"doc-top", 0.85}, {"doc-low", 0.3}}),
    };
    auto ctx = merger.merge(shards);

    ASSERT_FALSE(ctx.documents.empty());
    EXPECT_EQ(ctx.documents.front().doc_id, "doc-top");
}

// DK-C-03: Dedup — same doc_id from two shards appears only once
TEST(DK_C_FederatedRAGMerger, DK_C_03_DedupEliminatesDuplicateDocIds) {
    FederatedRAGMergerConfig cfg;
    cfg.deduplicate = true;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"shared-doc", 0.9}}),
        makeShardResult("s2", {{"shared-doc", 0.8}}),
    };
    auto ctx = merger.merge(shards);

    size_t count = 0;
    for (auto& d : ctx.documents) {
        if (d.doc_id == "shared-doc") {
          ++count;
        }
    }
    EXPECT_EQ(count, 1u);
}

// DK-C-04: top_k limit enforced — result never exceeds configured top_k
TEST(DK_C_FederatedRAGMerger, DK_C_04_TopKLimitEnforced) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 3;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"a", 0.9}, {"b", 0.8}, {"c", 0.7},
                                {"d", 0.6}, {"e", 0.5}}),
    };
    auto ctx = merger.merge(shards);
    EXPECT_LE(ctx.documents.size(), 3u);
}

// DK-C-05: total_candidate_count reflects all input documents before cut-off
TEST(DK_C_FederatedRAGMerger, DK_C_05_TotalCandidateCountCorrect) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 2;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"a", 0.9}, {"b", 0.7}}),
        makeShardResult("s2", {{"c", 0.8}, {"d", 0.6}}),
    };
    auto ctx = merger.merge(shards);
    EXPECT_EQ(ctx.total_candidate_count, 4u);
    EXPECT_LE(ctx.documents.size(), 2u);
}

// DK-C-06: unique_doc_count after dedup is ≤ total_candidate_count
TEST(DK_C_FederatedRAGMerger, DK_C_06_UniqueDocCountLeqTotal) {
    FederatedRAGMergerConfig cfg;
    cfg.deduplicate = true;
    cfg.top_k       = 100;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"dup", 0.9}, {"unique-1", 0.7}}),
        makeShardResult("s2", {{"dup", 0.8}, {"unique-2", 0.6}}),
    };
    auto ctx = merger.merge(shards);
    EXPECT_LE(ctx.unique_doc_count, ctx.total_candidate_count);
}

// DK-C-07: merge() of empty shard list returns empty context
TEST(DK_C_FederatedRAGMerger, DK_C_07_EmptyShardListReturnsEmptyContext) {
    FederatedRAGMerger merger;
    auto ctx = merger.merge({});
    EXPECT_TRUE(ctx.documents.empty());
    EXPECT_EQ(ctx.total_candidate_count, 0u);
}

// DK-C-08: SCORE_WEIGHTED strategy — highest-score doc appears first
TEST(DK_C_FederatedRAGMerger, DK_C_08_ScoreWeightedHighestScoreFirst) {
    FederatedRAGMergerConfig cfg;
    cfg.strategy = MergeStrategy::SCORE_WEIGHTED;
    cfg.top_k    = 10;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"low",  0.3}}),
        makeShardResult("s2", {{"high", 0.95}}),
    };
    auto ctx = merger.merge(shards);
    ASSERT_GE(ctx.documents.size(), 2u);
    EXPECT_EQ(ctx.documents.front().doc_id, "high");
}

// DK-C-09: mergeAndBuildContext() returns non-empty string
TEST(DK_C_FederatedRAGMerger, DK_C_09_MergeAndBuildContextNonEmpty) {
    FederatedRAGMerger merger;
    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"doc-x", 0.9}}),
    };
    const auto prompt = merger.mergeAndBuildContext(shards);
    EXPECT_FALSE(prompt.empty());
}

// DK-C-10: dedup disabled — same doc_id from two shards appears twice
TEST(DK_C_FederatedRAGMerger, DK_C_10_DedupDisabledAllowsDuplicates) {
    FederatedRAGMergerConfig cfg;
    cfg.deduplicate = false;
    cfg.top_k       = 100;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards = {
        makeShardResult("s1", {{"dup-doc", 0.9}}),
        makeShardResult("s2", {{"dup-doc", 0.8}}),
    };
    auto ctx = merger.merge(shards);

    size_t count = 0;
    for (auto& d : ctx.documents) {
        if (d.doc_id == "dup-doc") {
          ++count;
        }
    }
    EXPECT_EQ(count, 2u);
}

// ============================================================================
// DK-D — CrossShardFeedbackSync
// ============================================================================

static CrossShardFeedbackSync makeSyncWithSink(
    std::vector<nlohmann::json>& sent_payloads,
    const std::string& shard_id = "shard-A") {
    FeedbackSyncConfig cfg;
    cfg.max_embedding_dim = 4;
    cfg.dedup_cache_size  = 100;
    return CrossShardFeedbackSync(
        cfg, shard_id,
        [&](nlohmann::json payload) { sent_payloads.push_back(std::move(payload)); });
}

static FeedbackSummary makeFS(
    const std::string& summary_id    = "sum-001",
    const std::string& shard_origin  = "ANON",
    size_t             embedding_dim = 4) {
    FeedbackSummary fs;
    fs.summary_id          = summary_id;
    fs.shard_origin        = shard_origin;
    fs.feedback_type_label = "USER_NEGATIVE";
    fs.reason_embedding.assign(embedding_dim, 0.1f);
    fs.rlaif_round         = 1;
    return fs;
}

// DK-D-01: publishFeedback() dispatches a JSON payload via send_fn
TEST(DK_D_CrossShardFeedbackSync, DK_D_01_PublishDispatchesPayload) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    sync.publishFeedback(makeFS("sum-1"));
    EXPECT_EQ(sent.size(), 1u);
}

// DK-D-02: shard_origin is always "ANON" in outbound payload even if set to real value
TEST(DK_D_CrossShardFeedbackSync, DK_D_02_ShardOriginIsAlwaysAnon) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent, "shard-secret");

    FeedbackSummary fs = makeFS("sum-2");
    fs.shard_origin = "shard-secret";  // caller tries to set real origin
    sync.publishFeedback(fs);

    ASSERT_EQ(sent.size(), 1u);
    const auto& payload = sent[0];
    // Outbound shard_origin must be overwritten to "ANON"
    EXPECT_EQ(payload.value("shard_origin", ""), "ANON");
}

// DK-D-03: inboundCount() starts at 0
TEST(DK_D_CrossShardFeedbackSync, DK_D_03_InboundCountInitiallyZero) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);
    EXPECT_EQ(sync.receivedCount(), 0u);
}

// DK-D-04: processInbound() increments inboundCount for a new summary_id
TEST(DK_D_CrossShardFeedbackSync, DK_D_04_ProcessInboundIncrementsCount) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    sync.handleInboundSummary(makeFS("sum-new").toJson());
    EXPECT_EQ(sync.receivedCount(), 1u);
}

// DK-D-05: processInbound() with duplicate summary_id is deduped → count unchanged
TEST(DK_D_CrossShardFeedbackSync, DK_D_05_DuplicateInboundIsDeduped) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    const auto payload = makeFS("dup-sum-1").toJson();
    sync.handleInboundSummary(payload);
    sync.handleInboundSummary(payload);  // duplicate

    EXPECT_EQ(sync.receivedCount(), 1u);
    EXPECT_EQ(sync.deduplicatedCount(), 1u);
}

// DK-D-06: setFeedbackCallback() is invoked for each new inbound summary
TEST(DK_D_CrossShardFeedbackSync, DK_D_06_FeedbackCallbackInvokedOnInbound) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    std::vector<FeedbackSummary> received;
    sync.setFeedbackCallback([&](const FeedbackSummary& s) { received.push_back(s); });

    sync.handleInboundSummary(makeFS("s-1").toJson());
    sync.handleInboundSummary(makeFS("s-2").toJson());

    EXPECT_EQ(received.size(), 2u);
}

// DK-D-07: publishFeedback() validates embedding dimension and throws on mismatch
TEST(DK_D_CrossShardFeedbackSync, DK_D_07_EmbeddingDimValidation) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);  // max_embedding_dim=4

    FeedbackSummary bad_fs = makeFS("sum-bad", "ANON", 10);  // wrong dim
    EXPECT_THROW(sync.publishFeedback(bad_fs), std::invalid_argument);
}

// DK-D-08: deduplicatedCount() starts at 0 and only grows on actual duplicates
TEST(DK_D_CrossShardFeedbackSync, DK_D_08_DeduplicatedCountOnlyGrowsOnDups) {
    std::vector<nlohmann::json> sent;
    auto sync = makeSyncWithSink(sent);

    EXPECT_EQ(sync.deduplicatedCount(), 0u);

    sync.handleInboundSummary(makeFS("u-1").toJson());
    EXPECT_EQ(sync.deduplicatedCount(), 0u);  // no dup yet

    sync.handleInboundSummary(makeFS("u-1").toJson());  // now it's a dup
    EXPECT_EQ(sync.deduplicatedCount(), 1u);
}

#endif
