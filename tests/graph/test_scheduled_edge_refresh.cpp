/**
 * @file test_scheduled_edge_refresh.cpp
 * @brief Unit and integration tests for ScheduledGraphEdgeRefreshEngine.
 *
 * Covers:
 *  - RefreshPolicy validation (invalid threshold values throw)
 *  - computeSimilarity() for COSINE, DOT_PRODUCT, EUCLIDEAN metrics
 *  - computeTemporalDecay() half-life calculation
 *  - scoreEdge() combined relevance scoring
 *  - Safety gate: max_removal_fraction abort
 *  - Full refresh cycle: edges removed when below threshold
 *  - Full refresh cycle: candidate edges added when embeddings available
 *  - Audit trail entries after removal and addition
 *  - getStats() after a completed cycle
 *  - start() / stop() scheduler lifecycle with zero interval (manual only)
 *  - setPolicy() runtime update
 *  - Empty graph: triggerRefresh() handles zero-edge graph gracefully
 */

#include <gtest/gtest.h>

#include "graph/scheduled_edge_refresh.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

using namespace themis;
using namespace themis::graph;

// ============================================================================
// Test fixture
// ============================================================================

class ScheduledEdgeRefreshTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_scheduled_edge_refresh_test";
        fs::remove_all(test_db_path_);

        RocksDBWrapper::Config config;
        config.db_path             = test_db_path_;
        config.memtable_size_mb    = 16;
        config.block_cache_size_mb = 32;
        config.max_background_jobs = 1;

        db_        = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        graph_mgr_ = std::make_unique<GraphIndexManager>(*db_);
    }

    void TearDown() override {
        graph_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    void addEdge(const std::string& id,
                 const std::string& from,
                 const std::string& to,
                 double weight = 1.0,
                 int64_t created_at_sec = 0)
    {
        BaseEntity e(id);
        e.setField("id",     id);
        e.setField("_from",  from);
        e.setField("_to",    to);
        e.setField("_weight", std::to_string(weight));
        if (created_at_sec != 0)
            e.setField("_created_at", created_at_sec);
        ASSERT_TRUE(graph_mgr_->addEdge(e).ok);
    }

    /// Build a simple graph:  A→B, B→C, C→D, A→D
    void buildSmallGraph() {
        addEdge("e_ab", "A", "B", 1.0);
        addEdge("e_bc", "B", "C", 1.0);
        addEdge("e_cd", "C", "D", 1.0);
        addEdge("e_ad", "A", "D", 1.0);
    }

    /// Minimal always-positive embedding provider.
    static NodeEmbeddingProvider makeEmbeddingProvider(
        const std::unordered_map<std::string, std::vector<float>>& embs)
    {
        return [embs](const std::string& node_id) -> std::vector<float> {
            auto it = embs.find(node_id);
            if (it == embs.end()) return {};
            return it->second;
        };
    }

    std::string                          test_db_path_;
    std::unique_ptr<RocksDBWrapper>      db_;
    std::unique_ptr<GraphIndexManager>   graph_mgr_;
};

// ============================================================================
// RefreshPolicy validation
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, PolicyValidation_InvalidRelevanceThreshold) {
    RefreshPolicy policy;
    policy.relevance_threshold = 1.5f; // out of range

    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy),
        std::invalid_argument);
}

TEST_F(ScheduledEdgeRefreshTest, PolicyValidation_NegativeRelevanceThreshold) {
    RefreshPolicy policy;
    policy.relevance_threshold = -0.1f;

    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy),
        std::invalid_argument);
}

TEST_F(ScheduledEdgeRefreshTest, PolicyValidation_InvalidAddThreshold) {
    RefreshPolicy policy;
    policy.add_threshold = 2.0f;

    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy),
        std::invalid_argument);
}

TEST_F(ScheduledEdgeRefreshTest, PolicyValidation_InvalidMaxRemovalFraction) {
    RefreshPolicy policy;
    policy.max_removal_fraction = -0.5f;

    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy),
        std::invalid_argument);
}

TEST_F(ScheduledEdgeRefreshTest, PolicyValidation_ZeroTopKCandidates) {
    RefreshPolicy policy;
    policy.top_k_candidates = 0;

    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy),
        std::invalid_argument);
}

TEST_F(ScheduledEdgeRefreshTest, PolicyValidation_ValidPolicy) {
    RefreshPolicy policy;
    policy.relevance_threshold  = 0.5f;
    policy.add_threshold        = 0.7f;
    policy.max_removal_fraction = 0.1f;
    policy.top_k_candidates     = 5;

    EXPECT_NO_THROW(ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy));
}

// ============================================================================
// computeSimilarity()
// ============================================================================

class SimilarityTest : public ScheduledEdgeRefreshTest {};

TEST_F(SimilarityTest, Cosine_IdenticalVectors) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::COSINE;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    std::vector<float> v = {1.0f, 0.0f, 0.0f};
    float sim = engine.computeSimilarity(v, v);
    EXPECT_NEAR(sim, 1.0f, 1e-4f);
}

TEST_F(SimilarityTest, Cosine_OrthogonalVectors) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::COSINE;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    std::vector<float> a = {1.0f, 0.0f};
    std::vector<float> b = {0.0f, 1.0f};
    float sim = engine.computeSimilarity(a, b);
    EXPECT_NEAR(sim, 0.5f, 1e-4f); // cos(90°)=0 → mapped to 0.5
}

TEST_F(SimilarityTest, Cosine_EmptyVectors) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::COSINE;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    EXPECT_FLOAT_EQ(engine.computeSimilarity({}, {}), 0.0f);
}

TEST_F(SimilarityTest, Cosine_DimensionMismatch) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::COSINE;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    std::vector<float> a = {1.0f, 2.0f};
    std::vector<float> b = {1.0f};
    EXPECT_FLOAT_EQ(engine.computeSimilarity(a, b), 0.0f);
}

TEST_F(SimilarityTest, DotProduct_IdenticalVectors) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::DOT_PRODUCT;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    std::vector<float> v = {1.0f, 0.0f};
    float sim = engine.computeSimilarity(v, v);
    EXPECT_NEAR(sim, 1.0f, 1e-4f);
}

TEST_F(SimilarityTest, Euclidean_IdenticalVectors) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::EUCLIDEAN;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    std::vector<float> v = {1.0f, 2.0f, 3.0f};
    float sim = engine.computeSimilarity(v, v); // dist=0 → sim=1/(1+0)=1
    EXPECT_NEAR(sim, 1.0f, 1e-4f);
}

TEST_F(SimilarityTest, Euclidean_DistantVectors) {
    RefreshPolicy p;
    p.similarity_metric = SimilarityMetric::EUCLIDEAN;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    std::vector<float> a = {0.0f};
    std::vector<float> b = {100.0f};
    float sim = engine.computeSimilarity(a, b); // sim = 1/101 ≈ 0.0099
    EXPECT_LT(sim, 0.02f);
    EXPECT_GT(sim, 0.0f);
}

// ============================================================================
// computeTemporalDecay()
// ============================================================================

class TemporalDecayTest : public ScheduledEdgeRefreshTest {};

TEST_F(TemporalDecayTest, NoDecay_WhenHalfLifeIsZero) {
    RefreshPolicy p;
    p.decay_half_life_seconds = 0.0;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    BaseEntity e("edge1");
    e.setField("id",   std::string("edge1"));
    e.setField("_from", std::string("A"));
    e.setField("_to",   std::string("B"));
    e.setField("_created_at", static_cast<int64_t>(0));

    float decay = engine.computeTemporalDecay(e);
    EXPECT_FLOAT_EQ(decay, 1.0f);
}

TEST_F(TemporalDecayTest, NoDecay_WhenNoTimestamp) {
    RefreshPolicy p;
    p.decay_half_life_seconds = 3600.0;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    BaseEntity e("edge1");
    // no _created_at field set

    float decay = engine.computeTemporalDecay(e);
    EXPECT_FLOAT_EQ(decay, 1.0f);
}

TEST_F(TemporalDecayTest, HalfDecay_AtHalfLife) {
    RefreshPolicy p;
    p.decay_half_life_seconds = 3600.0; // 1 hour
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    // Created 1 hour ago
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t created_at = static_cast<int64_t>(now_sec) - 3600;

    BaseEntity e("edge1");
    e.setField("_created_at", created_at);

    float decay = engine.computeTemporalDecay(e);
    EXPECT_NEAR(decay, 0.5f, 0.01f); // 2^(-1) = 0.5
}

// ============================================================================
// scoreEdge()
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, ScoreEdge_RelevanceBelowThreshold) {
    RefreshPolicy p;
    p.relevance_threshold     = 0.9f; // high threshold
    p.decay_half_life_seconds = 0.0;  // no decay

    // Use orthogonal embeddings → low cosine similarity
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"A", {1.0f, 0.0f}},
        {"B", {0.0f, 1.0f}},
    };

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));

    BaseEntity e("e_ab");
    e.setField("id",    std::string("e_ab"));
    e.setField("_from", std::string("A"));
    e.setField("_to",   std::string("B"));

    auto score = engine.scoreEdge(e);
    EXPECT_EQ(score.edge_id, "e_ab");
    EXPECT_LT(score.relevance, 0.9f);
    EXPECT_TRUE(score.is_removal_candidate);
}

TEST_F(ScheduledEdgeRefreshTest, ScoreEdge_RelevanceAboveThreshold) {
    RefreshPolicy p;
    p.relevance_threshold     = 0.1f; // low threshold
    p.decay_half_life_seconds = 0.0;  // no decay

    // Use nearly identical embeddings → high cosine similarity
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"A", {1.0f, 0.01f}},
        {"B", {1.0f, 0.01f}},
    };

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));

    BaseEntity e("e_ab");
    e.setField("id",    std::string("e_ab"));
    e.setField("_from", std::string("A"));
    e.setField("_to",   std::string("B"));

    auto score = engine.scoreEdge(e);
    EXPECT_GT(score.relevance, 0.1f);
    EXPECT_FALSE(score.is_removal_candidate);
}

// ============================================================================
// triggerRefresh() – empty graph
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, TriggerRefresh_EmptyGraph) {
    RefreshPolicy p;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_EQ(stats.edges_evaluated, 0u);
    EXPECT_EQ(stats.edges_removed, 0u);
    EXPECT_EQ(stats.edges_added, 0u);
    EXPECT_FALSE(stats.aborted_safety_gate);
    EXPECT_EQ(stats.total_cycles_completed, 1u);
}

// ============================================================================
// triggerRefresh() – safety gate
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, SafetyGate_AbortWhenTooManyRemovals) {
    buildSmallGraph(); // 4 edges

    RefreshPolicy p;
    // With no embedding provider, similarity=1.0 and temporal decay disabled.
    // relevance = 1.0 * 1.0 * centrality_weight (< 1.0 for any vertex with out-edges).
    // Setting threshold=0.999 makes all 4 edges removal candidates.
    // With max_removal_fraction=0.1, removal of 4/4 = 100% > 10% triggers the gate.
    p.relevance_threshold     = 0.999f;
    p.max_removal_fraction    = 0.1f;  // only 10% allowed – with 4 edges, gate fires
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_TRUE(stats.aborted_safety_gate);
    EXPECT_EQ(stats.edges_removed, 0u);
    EXPECT_EQ(stats.edges_added, 0u);
}

// ============================================================================
// triggerRefresh() – removes low-relevance edges
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, TriggerRefresh_RemovesLowRelevanceEdges) {
    buildSmallGraph();

    RefreshPolicy p;
    // Use orthogonal embeddings → low cosine similarity → all edges removed candidates
    p.relevance_threshold     = 0.8f;
    p.max_removal_fraction    = 1.0f;  // allow all removals
    p.max_edges_to_remove     = 100;
    p.decay_half_life_seconds = 0.0;
    p.top_k_candidates        = 1;

    std::unordered_map<std::string, std::vector<float>> embs = {
        {"A", {1.0f, 0.0f}},
        {"B", {0.0f, 1.0f}},
        {"C", {-1.0f, 0.0f}},
        {"D", {0.0f, -1.0f}},
    };

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);
    EXPECT_GT(stats.edges_removed, 0u);
    EXPECT_EQ(stats.total_cycles_completed, 1u);
}

// ============================================================================
// triggerRefresh() – adds new candidate edges
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, TriggerRefresh_AddsCandidateEdges) {
    // Start with only A→B; B→A should be discovered as a new candidate since
    // the embeddings are identical (cosine similarity = 1.0 > add_threshold).
    addEdge("e_ab", "A", "B");

    RefreshPolicy p;
    p.relevance_threshold     = 0.0f;  // keep all existing edges
    p.add_threshold           = 0.5f;  // low threshold: cos(0°)=1 → mapped 1.0 > 0.5
    p.max_removal_fraction    = 0.0f;  // no removals allowed
    p.max_edges_to_remove     = 0;
    p.max_edges_to_add        = 100;
    p.decay_half_life_seconds = 0.0;
    p.top_k_candidates        = 5;
    p.similarity_metric       = SimilarityMetric::COSINE;

    // A and B have identical embeddings → cosine similarity = 1.0,
    // mapped to [0,1] space gives 1.0 > add_threshold=0.5.
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"A", {1.0f, 0.0f}},
        {"B", {1.0f, 0.0f}},  // identical to A → sim = 1.0
    };

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);
    // B→A has identical embeddings as A→B but B→A does not yet exist; it must be added.
    EXPECT_GE(stats.edges_added, 1u);
    EXPECT_EQ(stats.edges_removed, 0u);
}

// ============================================================================
// Audit trail
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, AuditTrail_RecordsRemovals) {
    buildSmallGraph();

    RefreshPolicy p;
    p.relevance_threshold     = 0.99f;  // make all edges removal candidates
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_remove     = 100;
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    engine.triggerRefresh();

    auto trail = engine.getAuditTrail();
    EXPECT_GT(trail.size(), 0u);

    for (const auto& entry : trail) {
        EXPECT_EQ(entry.action, RefreshAuditEntry::Action::REMOVE);
        EXPECT_EQ(entry.cycle_number, 1u);
        EXPECT_FALSE(entry.edge_id.empty());
    }
}

TEST_F(ScheduledEdgeRefreshTest, AuditTrail_EmptyBeforeFirstCycle) {
    RefreshPolicy p;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    EXPECT_TRUE(engine.getAuditTrail().empty());
}

// ============================================================================
// getStats()
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, GetStats_AfterCycle) {
    buildSmallGraph();

    RefreshPolicy p;
    p.relevance_threshold  = 0.0f; // keep everything
    p.max_removal_fraction = 1.0f;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    engine.triggerRefresh();

    auto stats = engine.getStats();
    EXPECT_EQ(stats.total_cycles_completed, 1u);
    EXPECT_EQ(stats.edges_evaluated, 4u);
}

// ============================================================================
// setPolicy() at runtime
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, SetPolicy_UpdatesPolicy) {
    RefreshPolicy p;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshPolicy p2;
    p2.relevance_threshold = 0.3f;
    EXPECT_NO_THROW(engine.setPolicy(p2));

    EXPECT_FLOAT_EQ(engine.getPolicy().relevance_threshold, 0.3f);
}

TEST_F(ScheduledEdgeRefreshTest, SetPolicy_InvalidThrowsException) {
    RefreshPolicy p;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshPolicy bad;
    bad.relevance_threshold = 5.0f;
    EXPECT_THROW(engine.setPolicy(bad), std::invalid_argument);
}

// ============================================================================
// Scheduler lifecycle (zero interval = manual only)
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, Scheduler_ZeroIntervalNoBackgroundThread) {
    RefreshPolicy p;
    p.refresh_interval = std::chrono::seconds(0); // manual only

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);
    EXPECT_NO_THROW(engine.start());
    EXPECT_NO_THROW(engine.stop());
}

TEST_F(ScheduledEdgeRefreshTest, Scheduler_StartStopIdempotent) {
    RefreshPolicy p;
    p.refresh_interval = std::chrono::seconds(0);

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    // Multiple start/stop calls must not crash.
    engine.start();
    engine.start(); // second call is a no-op
    engine.stop();
    engine.stop(); // second call is a no-op
}

// ============================================================================
// Multiple cycles
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, MultipleCycles_CycleCounterIncreases) {
    RefreshPolicy p;
    p.relevance_threshold = 0.0f;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    engine.triggerRefresh();
    engine.triggerRefresh();
    engine.triggerRefresh();

    auto stats = engine.getStats();
    EXPECT_EQ(stats.total_cycles_completed, 3u);
}
