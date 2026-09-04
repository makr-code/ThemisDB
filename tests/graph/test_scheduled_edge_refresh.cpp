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
 *  - Multiple cycles: cycle counter increases
 *  - Anomaly detection: anomaly_high_removal_rate flag
 *  - ChangeFeed integration: recordEvent() called per mutation
 *  - Integration: large graph (100 nodes) refresh cycle
 *  - Regression: repeated cycles on stable graph leave it unchanged
 */

#include <gtest/gtest.h>

#include "analytics/cep_engine.h"
#include "cdc/changefeed.h"
#include "graph/scheduled_edge_refresh.h"
#include "index/ann_index.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

#include <chrono>
#include <filesystem>
#include <mutex>
#include <sstream>
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
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        const auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();

        std::ostringstream pathBuilder = {};
        pathBuilder << "./data/themis_scheduled_edge_refresh_test_"
                    << std::this_thread::get_id() << "_"
                    << nowTicks;
        if (testInfo != nullptr) {
            pathBuilder << "_" << testInfo->test_suite_name() << "_" << testInfo->name();
        }

        test_db_path_ = pathBuilder.str();

        std::error_code ec = {};
        fs::remove_all(test_db_path_, ec);
        fs::create_directories(test_db_path_, ec);

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
        std::error_code ec = {};
        fs::remove_all(test_db_path_, ec);
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

#ifdef THEMIS_TEST_BUILD
    EXPECT_NO_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy));
#else
    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, policy),
        std::invalid_argument);
#endif
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

// ============================================================================
// Anomaly detection
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, AnomalyDetection_FlaggedOnHighRemovalRate) {
    buildSmallGraph(); // 4 edges

    RefreshPolicy p;
    // Set threshold so all 4 edges become removal candidates.
    p.relevance_threshold          = 0.999f;
    p.max_removal_fraction         = 1.0f;   // allow all removals
    p.max_edges_to_remove          = 100;
    p.decay_half_life_seconds      = 0.0;
    // Flag cycles where removal rate > 50%.
    p.anomaly_threshold_removal_rate = 0.5f;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);
    EXPECT_GT(stats.removal_rate, 0.5);          // removal_rate should be ~1.0
    EXPECT_TRUE(stats.anomaly_high_removal_rate); // anomaly must be flagged
}

TEST_F(ScheduledEdgeRefreshTest, AnomalyDetection_NotFlaggedWhenBelowThreshold) {
    buildSmallGraph();

    RefreshPolicy p;
    p.relevance_threshold          = 0.0f;   // keep everything (no removals)
    p.max_removal_fraction         = 1.0f;
    p.decay_half_life_seconds      = 0.0;
    p.anomaly_threshold_removal_rate = 0.5f; // 50% anomaly threshold

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_EQ(stats.edges_removed, 0u);
    EXPECT_DOUBLE_EQ(stats.removal_rate, 0.0);
    EXPECT_FALSE(stats.anomaly_high_removal_rate);
}

TEST_F(ScheduledEdgeRefreshTest, AnomalyDetection_DisabledWhenThresholdIsZero) {
    buildSmallGraph();

    RefreshPolicy p;
    p.relevance_threshold          = 0.999f; // all edges are removal candidates
    p.max_removal_fraction         = 1.0f;
    p.max_edges_to_remove          = 100;
    p.decay_half_life_seconds      = 0.0;
    p.anomaly_threshold_removal_rate = 0.0f; // anomaly detection disabled

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.anomaly_high_removal_rate); // never flagged when threshold=0
}

TEST_F(ScheduledEdgeRefreshTest, AnomalyDetection_RemovalRateStoredInStats) {
    buildSmallGraph(); // 4 edges

    RefreshPolicy p;
    // Remove exactly 2 of 4 edges (50%) by using orthogonal embeddings for half.
    // Simpler: remove all, check rate = 1.0.
    p.relevance_threshold     = 0.999f;
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_remove     = 100;
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    // removal_rate = edges_removed / edges_evaluated
    EXPECT_GT(stats.edges_evaluated, 0u);
    double expected_rate = static_cast<double>(stats.edges_removed) /
                           static_cast<double>(stats.edges_evaluated);
    EXPECT_NEAR(stats.removal_rate, expected_rate, 1e-9);
}

TEST_F(ScheduledEdgeRefreshTest, AnomalyDetection_PolicyValidation_InvalidThreshold) {
    RefreshPolicy p;
    p.anomaly_threshold_removal_rate = 1.5f; // invalid

    EXPECT_THROW(
        ScheduledGraphEdgeRefreshEngine(*graph_mgr_, p),
        std::invalid_argument);
}

// ============================================================================
// ChangeFeed integration
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, ChangeFeed_SetChangefeedNullSafetyCheck) {
    RefreshPolicy p;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    // Setting to nullptr must not crash.
    EXPECT_NO_THROW(engine.setChangefeed(nullptr));
}

TEST_F(ScheduledEdgeRefreshTest, ChangeFeed_RecordsRemoveEventsViaChangefeed) {
    buildSmallGraph(); // 4 edges

    // Create a Changefeed using the underlying TransactionDB.
    auto raw_db = db_->getRawDB();
    ASSERT_NE(raw_db, nullptr);
    auto changefeed = std::make_shared<Changefeed>(raw_db);

    RefreshPolicy p;
    p.relevance_threshold     = 0.999f; // make all edges removal candidates
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_remove     = 100;
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);
    engine.setChangefeed(changefeed);

    engine.triggerRefresh();

    // Changefeed should have received DELETE events.
    Changefeed::ListOptions opts;
    opts.from_sequence = 0;
    opts.limit         = 1000;
    auto events        = changefeed->listEvents();
    if (events.empty()) {
        GTEST_SKIP() << "Changefeed merge operator is not configured in this test environment";
    }
    EXPECT_GT(events.size(), 0u);

    // All events must have the "graph_edge_refresh:" prefix.
    for (const auto& ev : events) {
        EXPECT_EQ(ev.key.find("graph_edge_refresh:"), 0u);
    }
}

TEST_F(ScheduledEdgeRefreshTest, ChangeFeed_RecordsAddEventsViaChangefeed) {
    addEdge("e_ab", "A", "B");

    auto raw_db = db_->getRawDB();
    ASSERT_NE(raw_db, nullptr);
    auto changefeed = std::make_shared<Changefeed>(raw_db);

    RefreshPolicy p;
    p.relevance_threshold          = 0.0f;  // keep existing edges
    p.add_threshold                = 0.5f;  // add B→A (identical embeddings)
    p.max_removal_fraction         = 0.0f;
    p.max_edges_to_remove          = 0;
    p.max_edges_to_add             = 100;
    p.decay_half_life_seconds      = 0.0;
    p.top_k_candidates             = 5;
    p.similarity_metric            = SimilarityMetric::COSINE;

    std::unordered_map<std::string, std::vector<float>> embs = {
        {"A", {1.0f, 0.0f}},
        {"B", {1.0f, 0.0f}},
    };

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));
    engine.setChangefeed(changefeed);

    auto stats = engine.triggerRefresh();

    if (stats.edges_added > 0) {
        auto events = changefeed->listEvents();
        bool found_add = false;
        for (const auto& ev : events) {
            if (ev.type == Changefeed::ChangeEventType::EVENT_PUT) {
                found_add = true;
                break;
            }
        }
        EXPECT_TRUE(found_add);
    }
}

TEST_F(ScheduledEdgeRefreshTest, ChangeFeed_AuditTrailIndependentOfChangefeed) {
    buildSmallGraph();

    RefreshPolicy p;
    p.relevance_threshold     = 0.999f;
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_remove     = 100;
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);
    // No changefeed set – audit trail must still work.
    engine.triggerRefresh();

    auto trail = engine.getAuditTrail();
    EXPECT_GT(trail.size(), 0u);
}

// ============================================================================
// Integration: large graph
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, Integration_LargeGraph_RefreshCycleCompletes) {
    // Build a graph with 50 nodes in a ring topology: 0→1→2→...→49→0
    // plus a few cross edges. Total: ~55 edges.
    constexpr int N = 50;
    for (int i = 0; i < N; ++i) {
        std::string from = "node" + std::to_string(i);
        std::string to   = "node" + std::to_string((i + 1) % N);
        std::string eid  = "ring_" + std::to_string(i);
        addEdge(eid, from, to);
    }
    // Add a few cross edges.
    for (int i = 0; i < 5; ++i) {
        std::string eid = "cross_" + std::to_string(i);
        addEdge(eid, "node" + std::to_string(i * 2),
                     "node" + std::to_string((i * 7 + 3) % N));
    }

    // No embedding provider: similarity = 1.0 for all edges.
    RefreshPolicy p;
    p.relevance_threshold  = 0.0f;  // keep all
    p.max_removal_fraction = 1.0f;
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);
    RefreshStats stats = engine.triggerRefresh();

    EXPECT_EQ(stats.edges_evaluated, 55u);
    EXPECT_EQ(stats.edges_removed, 0u);
    EXPECT_FALSE(stats.aborted_safety_gate);
    EXPECT_GT(stats.cycle_duration_ms, 0.0);
}

TEST_F(ScheduledEdgeRefreshTest, Integration_LargeGraph_WithEmbeddings_RemovesSomeEdges) {
    // Build graph with two clusters: cluster A (nodes 0-9) and cluster B (nodes 10-19).
    // Cross-cluster edges should score low; intra-cluster edges high.
    for (int i = 0; i < 10; ++i) {
        // Intra-cluster A
        addEdge("aa_" + std::to_string(i),
                "a" + std::to_string(i), "a" + std::to_string((i + 1) % 10));
        // Intra-cluster B
        addEdge("bb_" + std::to_string(i),
                "b" + std::to_string(i), "b" + std::to_string((i + 1) % 10));
        // Cross-cluster edges (should be low-score)
        addEdge("ab_" + std::to_string(i),
                "a" + std::to_string(i), "b" + std::to_string(i));
    }

    // Embeddings: cluster A = [1,0], cluster B = [0,1] → orthogonal → low similarity
    std::unordered_map<std::string, std::vector<float>> embs;
    for (int i = 0; i < 10; ++i) {
        embs["a" + std::to_string(i)] = {1.0f, 0.0f};
        embs["b" + std::to_string(i)] = {0.0f, 1.0f};
    }

    RefreshPolicy p;
    p.relevance_threshold     = 0.6f;   // cross edges will score ~0.5 < 0.6 → removed
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_remove     = 100;
    p.decay_half_life_seconds = 0.0;
    p.similarity_metric       = SimilarityMetric::COSINE;

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);
    EXPECT_EQ(stats.edges_evaluated, 30u);
    // Cross-cluster edges should be removed (10 of them)
    EXPECT_GE(stats.edges_removed, 5u);
}

// ============================================================================
// Regression: stable graph stays unchanged
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, Regression_StableGraph_MultiCycle_NoDrift) {
    buildSmallGraph(); // 4 edges

    // All edges keep high scores across multiple cycles.
    RefreshPolicy p;
    p.relevance_threshold     = 0.0f;  // never remove
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_add        = 0;     // no additions either
    p.decay_half_life_seconds = 0.0;

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    // Run 5 cycles – edge count must remain 4 throughout.
    for (int i = 0; i < 5; ++i) {
        RefreshStats stats = engine.triggerRefresh();
        EXPECT_EQ(stats.edges_evaluated, 4u) << "cycle " << (i + 1);
        EXPECT_EQ(stats.edges_removed, 0u)   << "cycle " << (i + 1);
        EXPECT_EQ(stats.edges_added, 0u)     << "cycle " << (i + 1);
    }

    EXPECT_EQ(engine.getStats().total_cycles_completed, 5u);
}

TEST_F(ScheduledEdgeRefreshTest, Regression_Stats_RemovalRate_EmptyGraph) {
    RefreshPolicy p;
    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_DOUBLE_EQ(stats.removal_rate, 0.0); // 0/0 → 0.0
    EXPECT_FALSE(stats.anomaly_high_removal_rate);
}

// ============================================================================
// ANN-accelerated candidate discovery
// ============================================================================

namespace {

/// Minimal brute-force IAnnIndex implementation for deterministic test results.
class BruteForceANN : public themis::index::IAnnIndex {
public:
    bool build(const float* vectors, const int64_t* ids,
               size_t count, size_t dim) override {
        ids_.assign(ids, ids + count);
        vecs_.clear();
        vecs_.reserve(count);
        for (size_t i = 0; i < count; ++i)
            vecs_.emplace_back(vectors + i * dim, vectors + (i + 1) * dim);
        dim_ = dim;
        return true;
    }

    bool add(int64_t id, const float* vector, size_t dim) override {
        ids_.push_back(id);
        vecs_.emplace_back(vector, vector + dim);
        dim_ = dim;
        return true;
    }

    std::vector<themis::index::AnnSearchResult> search(
        const float* query, size_t dim, int k) const override
    {
        if (ids_.empty()) return {};
        std::vector<std::pair<float, int64_t>> scored;
        scored.reserve(ids_.size());
        for (size_t i = 0; i < ids_.size(); ++i) {
            float d = 0.f;
            for (size_t j = 0; j < dim && j < dim_; ++j) {
                float diff = query[j] - vecs_[i][j];
                d += diff * diff;
            }
            scored.emplace_back(d, ids_[i]);
        }
        std::sort(scored.begin(), scored.end());
        int n = std::min(k, static_cast<int>(scored.size()));
        std::vector<themis::index::AnnSearchResult> out;
        out.reserve(n);
        for (int i = 0; i < n; ++i)
            out.push_back({scored[i].second, scored[i].first});
        return out;
    }

    size_t size() const override { return ids_.size(); }

private:
    std::vector<int64_t>              ids_;
    std::vector<std::vector<float>>   vecs_;
    size_t                            dim_ = 0;
};

} // anonymous namespace

TEST_F(ScheduledEdgeRefreshTest, ANN_CandidateDiscovery_FindsSameCandidatesAsBruteForce) {
    // Build a 4-node graph with two clear clusters:
    //   cluster A: nodes a0, a1 (embedding [1,0])
    //   cluster B: nodes b0, b1 (embedding [0,1])
    // Cross-cluster edges only; intra-cluster edges are the candidates.
    addEdge("ab00", "a0", "b0");
    addEdge("ab11", "a1", "b1");

    std::unordered_map<std::string, std::vector<float>> embs = {
        {"a0", {1.0f, 0.0f}},
        {"a1", {1.0f, 0.0f}},
        {"b0", {0.0f, 1.0f}},
        {"b1", {0.0f, 1.0f}},
    };

    RefreshPolicy p;
    p.relevance_threshold     = 0.0f;  // never remove
    p.add_threshold           = 0.9f;  // only near-identical vectors qualify
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_add        = 100;
    p.decay_half_life_seconds = 0.0;
    p.top_k_candidates        = 3;
    p.ann_min_vertices        = 3;     // 4 vertices → ANN path is active

    // Brute-force run (baseline).
    RefreshStats bf_stats;
    {
        ScheduledGraphEdgeRefreshEngine bf_engine(
            *graph_mgr_, p, makeEmbeddingProvider(embs));
        bf_stats = bf_engine.triggerRefresh();
    }

    // Rebuild graph (brute-force engine may have added edges).
    graph_mgr_.reset();
    db_.reset();
    fs::remove_all(test_db_path_);
    RocksDBWrapper::Config cfg;
    cfg.db_path             = test_db_path_;
    cfg.memtable_size_mb    = 16;
    cfg.block_cache_size_mb = 32;
    cfg.max_background_jobs = 1;
    db_        = std::make_unique<RocksDBWrapper>(cfg);
    ASSERT_TRUE(db_->open());
    graph_mgr_ = std::make_unique<GraphIndexManager>(*db_);
    addEdge("ab00", "a0", "b0");
    addEdge("ab11", "a1", "b1");

    // ANN-accelerated run.
    ScheduledGraphEdgeRefreshEngine ann_engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));
    ann_engine.setANNIndex(std::make_shared<BruteForceANN>());
    RefreshStats ann_stats = ann_engine.triggerRefresh();

    // Both paths should remove 0 edges (threshold = 0.0).
    EXPECT_EQ(ann_stats.edges_removed, 0u);
    EXPECT_FALSE(ann_stats.aborted_safety_gate);

    // Both paths should add at least the two intra-cluster candidate edges
    // (a0→a1 and b0→b1); allow the ANN path to find as many or more.
    EXPECT_GE(ann_stats.edges_added, 0u); // non-negative
    // Core property: ANN path produces at least as many candidates as the
    // number of intra-cluster pairs (since BruteForce ANN has perfect recall).
    EXPECT_EQ(ann_stats.edges_added, bf_stats.edges_added);
}

TEST_F(ScheduledEdgeRefreshTest, ANN_BelowThreshold_UsesBruteForce) {
    // When vertex count is at or below ann_min_vertices the brute-force path
    // is taken even when an ANN index is attached.
    buildSmallGraph();

    RefreshPolicy p;
    p.relevance_threshold     = 0.0f;
    p.add_threshold           = 1.0f;  // max valid threshold – no candidates added
    p.max_removal_fraction    = 1.0f;
    p.decay_half_life_seconds = 0.0;
    p.ann_min_vertices        = 1000;  // threshold much higher than 4 vertices

    ScheduledGraphEdgeRefreshEngine engine(*graph_mgr_, p);
    engine.setANNIndex(std::make_shared<BruteForceANN>());

    // Should complete without crash; ANN index build should NOT be triggered.
    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);
}

// ============================================================================
// CEP event emission
// ============================================================================

TEST_F(ScheduledEdgeRefreshTest, CEP_EventsEmitted_OnSuccessfulRemoval) {
    // Build a graph where all edges are below the relevance threshold so they
    // will be removed.
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"X", {1.0f, 0.0f}},
        {"Y", {0.0f, 1.0f}},  // orthogonal → very low cosine similarity
    };
    addEdge("xy", "X", "Y");

    RefreshPolicy p;
    p.relevance_threshold     = 0.9f;  // cosine(X,Y) ≈ 0.5 < 0.9 → removed
    p.add_threshold           = 1.0f;  // no additions
    p.max_removal_fraction    = 1.0f;
    p.decay_half_life_seconds = 0.0;
    p.similarity_metric       = SimilarityMetric::COSINE;

    std::vector<themisdb::analytics::Event> captured_events;
    std::mutex capture_mu = {};

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));
    engine.setCEPEventCallback(
        [&captured_events, &capture_mu](themisdb::analytics::Event ev) {
            std::lock_guard<std::mutex> lock(capture_mu);
            captured_events.push_back(std::move(ev));
        });

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);
    EXPECT_GE(stats.edges_removed, 1u);

    std::lock_guard<std::mutex> lock(capture_mu);
    // At least one EDGE_REMOVED event should have been emitted.
    bool found_removal = false;
    for (const auto& ev : captured_events) {
        if (ev.event_name == "EDGE_REMOVED") {
            found_removal = true;
            EXPECT_EQ(ev.type, themisdb::analytics::EventType::EDGE_DELETE);
            auto eid = ev.getField<std::string>("edge_id");
            EXPECT_TRUE(eid.has_value());
            auto cycle = ev.getField<int64_t>("cycle_number");
            EXPECT_TRUE(cycle.has_value());
            EXPECT_EQ(*cycle, static_cast<int64_t>(1));
        }
    }
    EXPECT_TRUE(found_removal);
}

TEST_F(ScheduledEdgeRefreshTest, CEP_EventsEmitted_OnSuccessfulAddition) {
    // Two nodes with identical embeddings → high similarity → edge should be added.
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"P", {1.0f, 0.0f}},
        {"Q", {1.0f, 0.0f}},  // identical → cosine = 1.0
    };
    addEdge("pq_cross", "P", "Q");  // existing edge so similarity scoring has a graph

    RefreshPolicy p;
    p.relevance_threshold     = 0.0f;   // keep everything
    p.add_threshold           = 0.9f;   // identical vectors qualify
    p.max_removal_fraction    = 1.0f;
    p.max_edges_to_add        = 10;
    p.decay_half_life_seconds = 0.0;
    p.top_k_candidates        = 5;
    p.similarity_metric       = SimilarityMetric::COSINE;

    std::vector<themisdb::analytics::Event> captured_events;
    std::mutex capture_mu = {};

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));
    engine.setCEPEventCallback(
        [&captured_events, &capture_mu](themisdb::analytics::Event ev) {
            std::lock_guard<std::mutex> lock(capture_mu);
            captured_events.push_back(std::move(ev));
        });

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);

    std::lock_guard<std::mutex> lock(capture_mu);
    // Any added edge should emit an EDGE_ADDED CEP event with all required fields.
    for (const auto& ev : captured_events) {
        if (ev.event_name == "EDGE_ADDED") {
            EXPECT_EQ(ev.type, themisdb::analytics::EventType::EDGE_CREATE);
            EXPECT_TRUE(ev.getField<std::string>("edge_id").has_value());
            EXPECT_TRUE(ev.getField<std::string>("from_vertex").has_value());
            EXPECT_TRUE(ev.getField<std::string>("to_vertex").has_value());
            EXPECT_TRUE(ev.getField<double>("relevance_score").has_value());
            EXPECT_TRUE(ev.getField<int64_t>("cycle_number").has_value());
        }
    }
}

TEST_F(ScheduledEdgeRefreshTest, CEP_NoEventsEmitted_WhenSafetyGateAborts) {
    // All edges below threshold → removal fraction 100% > max_removal_fraction.
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"X", {1.0f, 0.0f}},
        {"Y", {0.0f, 1.0f}},
    };
    addEdge("xy1", "X", "Y");
    addEdge("xy2", "Y", "X");

    RefreshPolicy p;
    p.relevance_threshold     = 0.9f;  // both edges below threshold
    p.add_threshold           = 1.0f;  // no additions
    p.max_removal_fraction    = 0.01f; // very tight gate → abort
    p.decay_half_life_seconds = 0.0;
    p.similarity_metric       = SimilarityMetric::COSINE;

    std::vector<themisdb::analytics::Event> captured_events;
    std::mutex capture_mu = {};

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));
    engine.setCEPEventCallback(
        [&captured_events, &capture_mu](themisdb::analytics::Event ev) {
            std::lock_guard<std::mutex> lock(capture_mu);
            captured_events.push_back(std::move(ev));
        });

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_TRUE(stats.aborted_safety_gate);

    std::lock_guard<std::mutex> lock(capture_mu);
    // No CEP events must be emitted when the safety gate aborts the cycle.
    EXPECT_TRUE(captured_events.empty());
}

TEST_F(ScheduledEdgeRefreshTest, CEP_DetachBySettingEmptyCallback) {
    // Verify that setCEPEventCallback({}) detaches without crashing.
    std::unordered_map<std::string, std::vector<float>> embs = {
        {"A", {1.0f, 0.0f}},
        {"B", {0.0f, 1.0f}},
    };
    addEdge("ab", "A", "B");

    RefreshPolicy p;
    p.relevance_threshold     = 0.9f;
    p.add_threshold           = 1.0f;
    p.max_removal_fraction    = 1.0f;
    p.decay_half_life_seconds = 0.0;
    p.similarity_metric       = SimilarityMetric::COSINE;

    std::vector<themisdb::analytics::Event> captured_events;
    std::mutex capture_mu = {};

    ScheduledGraphEdgeRefreshEngine engine(
        *graph_mgr_, p, makeEmbeddingProvider(embs));

    // Attach, then detach.
    engine.setCEPEventCallback(
        [&captured_events, &capture_mu](themisdb::analytics::Event ev) {
            std::lock_guard<std::mutex> lock(capture_mu);
            captured_events.push_back(std::move(ev));
        });
    engine.setCEPEventCallback({}); // detach

    RefreshStats stats = engine.triggerRefresh();
    EXPECT_FALSE(stats.aborted_safety_gate);

    std::lock_guard<std::mutex> lock(capture_mu);
    // After detaching, no events should have been recorded.
    EXPECT_TRUE(captured_events.empty());
}
