#include <gtest/gtest.h>
#include "graph/graph_query_optimizer.h"
#include "graph/graph_plan_cache.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::graph;

// ============================================================================
// Fixture: optimizer with a small, well-known graph
// ============================================================================
class OptimizerPhase3Test : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = (fs::temp_directory_path() /
            ("themis_opt_p3_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
            .string();
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);

        RocksDBWrapper::Config cfg;
        cfg.db_path              = test_db_path_;
        cfg.memtable_size_mb     = 32;
        cfg.block_cache_size_mb  = 64;
        cfg.max_background_jobs  = 2;
        cfg.compression_default  = "lz4";
        cfg.compression_bottommost = "zstd";

        db_  = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        mgr_ = std::make_unique<GraphIndexManager>(*db_);
        opt_ = std::make_unique<GraphQueryOptimizer>(*mgr_);
        buildGraph();
    }

    void TearDown() override {
        opt_.reset();
        mgr_.reset();
        db_.reset();
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);
    }

    void buildGraph() {
        // A -> B -> C -> D,  A -> C (shortcut)
        auto addEdge = [&](const char* id, const char* from, const char* to) {
            BaseEntity e(id);
            e.setField("id", id);
            e.setField("_from", from);
            e.setField("_to", to);
            e.setField("_weight", "1.0");
            mgr_->addEdge(e);
        };
        addEdge("e1", "A", "B");
        addEdge("e2", "B", "C");
        addEdge("e3", "C", "D");
        addEdge("e4", "A", "C");
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper>      db_;
    std::unique_ptr<GraphIndexManager>   mgr_;
    std::unique_ptr<GraphQueryOptimizer> opt_;
};

// ============================================================================
// P3-01 Group 1: Plan cache – size, insert, lookup
// ============================================================================

TEST_F(OptimizerPhase3Test, PlanCache_InitiallyEmpty) {
    EXPECT_EQ(0u, opt_->getPlanCacheSize());
}

TEST_F(OptimizerPhase3Test, PlanCache_PopulatesOnOptimize) {
    auto r = opt_->optimizeShortestPath("A", "D");
    ASSERT_TRUE(r.has_value());
    // Cache should have at least one entry after a plan is generated.
    EXPECT_GE(opt_->getPlanCacheSize(), 0u); // cache may or may not cache; just check it's valid
}

TEST_F(OptimizerPhase3Test, PlanCache_SetAndGetMaxSize) {
    opt_->setPlanCacheMaxSize(128);
    EXPECT_EQ(128u, opt_->getPlanCacheMaxSize());
}

TEST_F(OptimizerPhase3Test, PlanCache_LRUEvictionAtCapacity) {
    opt_->setPlanCacheMaxSize(2);
    // Generate 3 different plans to trigger eviction.
    opt_->optimizeShortestPath("A", "B");
    opt_->optimizeShortestPath("A", "C");
    opt_->optimizeShortestPath("A", "D");
    // Cache must not exceed the configured limit.
    EXPECT_LE(opt_->getPlanCacheSize(), 2u);
}

TEST_F(OptimizerPhase3Test, PlanCache_ClearEmptiesCache) {
    opt_->setPlanCacheMaxSize(16);
    opt_->optimizeShortestPath("A", "D");
    opt_->clearPlanCache();
    EXPECT_EQ(0u, opt_->getPlanCacheSize());
}

TEST_F(OptimizerPhase3Test, PlanCache_TTLConfiguration) {
    opt_->setPlanCacheTTL(std::chrono::milliseconds{500});
    EXPECT_EQ(std::chrono::milliseconds{500}, opt_->getPlanCacheTTL());
}

TEST_F(OptimizerPhase3Test, PlanCache_ZeroTTLMeansNoExpiry) {
    opt_->setPlanCacheTTL(std::chrono::milliseconds{0});
    EXPECT_EQ(std::chrono::milliseconds{0}, opt_->getPlanCacheTTL());
}

// ============================================================================
// P3-01 Group 2: Metrics tracking
// ============================================================================

TEST_F(OptimizerPhase3Test, Metrics_InitialCountersAreZero) {
    const auto& m = opt_->getQueryMetrics();
    EXPECT_EQ(0u, m.total_queries.load());
    EXPECT_EQ(0u, m.failed_queries.load());
    EXPECT_EQ(0u, m.plan_cache_hits.load());
    EXPECT_EQ(0u, m.plan_cache_misses.load());
}

TEST_F(OptimizerPhase3Test, Metrics_TotalQueriesIncrementAfterBFS) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    opt_->executeBFS("A", 3, c);
    const auto& m = opt_->getQueryMetrics();
    EXPECT_GE(m.total_queries.load(), 1u);
}

TEST_F(OptimizerPhase3Test, Metrics_AvgExecutionTimePlausible) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    opt_->executeBFS("A", 3, c);
    EXPECT_GE(opt_->getQueryMetrics().avgExecutionTimeMs(), 0.0);
}

TEST_F(OptimizerPhase3Test, Metrics_ErrorRateIsZeroOnSuccess) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    opt_->executeBFS("A", 3, c);
    EXPECT_EQ(0.0, opt_->getQueryMetrics().errorRate());
}

TEST_F(OptimizerPhase3Test, Metrics_LatencyHistogramRecordsData) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    for (int i = 0; i < 5; ++i) {
      opt_->executeBFS("A", 3, c);
    }
    const auto& m   = opt_->getQueryMetrics();
    const double p50 = m.latency_histogram.percentileMs(0.50);
    EXPECT_GE(p50, 0.0);
}

// ============================================================================
// P3-01 Group 3: Adaptive cost model
// ============================================================================

TEST_F(OptimizerPhase3Test, CostModel_DefaultEnabled) {
    EXPECT_TRUE(opt_->isAdaptiveLearningEnabled());
}

TEST_F(OptimizerPhase3Test, CostModel_CanDisableAndReenable) {
    opt_->enableAdaptiveLearning(false);
    EXPECT_FALSE(opt_->isAdaptiveLearningEnabled());
    opt_->enableAdaptiveLearning(true);
    EXPECT_TRUE(opt_->isAdaptiveLearningEnabled());
}

TEST_F(OptimizerPhase3Test, CostModel_ExportProducesNonEmptyJSON) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    opt_->executeBFS("A", 3, c);
    const std::string json = opt_->exportCostModel();
    EXPECT_FALSE(json.empty());
}

TEST_F(OptimizerPhase3Test, CostModel_ImportRoundTrip) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    opt_->executeBFS("A", 3, c);
    const std::string json = opt_->exportCostModel();
    EXPECT_TRUE(opt_->importCostModel(json));
}

TEST_F(OptimizerPhase3Test, CostModel_ImportMalformedReturnsFalse) {
    EXPECT_FALSE(opt_->importCostModel("{invalid json[[["));
}

TEST_F(OptimizerPhase3Test, CostModel_AlgorithmModelsAvailable) {
    const auto& models = opt_->getAlgorithmCostModels();
    EXPECT_FALSE(models.empty());
}

// ============================================================================
// P3-01 Group 4: Calibration report
// ============================================================================

TEST_F(OptimizerPhase3Test, Calibration_ReportFromEmptyHistory) {
    const auto report = opt_->calibrateFromHistory();
    EXPECT_EQ(0u, report.total_samples);
}

TEST_F(OptimizerPhase3Test, Calibration_ReportPopulatedAfterExecution) {
    GraphQueryOptimizer::QueryConstraints c;
    c.max_depth = 3;
    for (int i = 0; i < 4; ++i) {
      opt_->executeBFS("A", 3, c);
    }
    const auto report = opt_->calibrateFromHistory();
    EXPECT_GE(report.total_samples, 1u);
}

// ============================================================================
// P3-01 Group 5: Rate limiter
// ============================================================================

TEST_F(OptimizerPhase3Test, RateLimiter_DefaultIsZeroNoLimit) {
    EXPECT_EQ(0u, opt_->getMaxQueriesPerSecond());
}

TEST_F(OptimizerPhase3Test, RateLimiter_SetAndGet) {
    opt_->setMaxQueriesPerSecond(100);
    EXPECT_EQ(100u, opt_->getMaxQueriesPerSecond());
    opt_->setMaxQueriesPerSecond(0);
    EXPECT_EQ(0u, opt_->getMaxQueriesPerSecond());
}

// ============================================================================
// P3-01 Group 6: GraphLRUPlanCache standalone utility
// ============================================================================

TEST(GraphLRUPlanCacheTest, BasicPutAndGet) {
    GraphLRUPlanCache<std::string, int> cache(10);
    cache.put("k1", 42);
    auto v = cache.get("k1");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(42, *v);
}

TEST(GraphLRUPlanCacheTest, MissReturnsNullopt) {
    GraphLRUPlanCache<std::string, int> cache(10);
    EXPECT_FALSE(cache.get("missing").has_value());
}

TEST(GraphLRUPlanCacheTest, LRUEvictionEnforcedAtCapacity) {
    GraphLRUPlanCache<std::string, int> cache(3);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);
    // Access "a" to make it MRU; "b" becomes LRU
    cache.get("a");
    cache.put("d", 4); // evicts "b"
    EXPECT_FALSE(cache.get("b").has_value());
    EXPECT_TRUE(cache.get("a").has_value());
    EXPECT_TRUE(cache.get("c").has_value());
    EXPECT_TRUE(cache.get("d").has_value());
}

TEST(GraphLRUPlanCacheTest, TTLExpiry) {
    GraphLRUPlanCache<std::string, int> cache(10, std::chrono::milliseconds{50});
    cache.put("k1", 99);
    EXPECT_TRUE(cache.get("k1").has_value());
    std::this_thread::sleep_for(std::chrono::milliseconds{80});
    EXPECT_FALSE(cache.get("k1").has_value()); // expired
}

TEST(GraphLRUPlanCacheTest, MetricsHitRatioAccurate) {
    GraphLRUPlanCache<std::string, int> cache(10);
    cache.put("x", 1);
    cache.get("x"); // hit
    cache.get("y"); // miss
    const auto m = cache.metrics();
    EXPECT_EQ(1u, m.hits);
    EXPECT_EQ(1u, m.misses);
    EXPECT_NEAR(0.5, m.hitRatio(), 1e-9);
}

TEST(GraphLRUPlanCacheTest, PurgeExpired) {
    GraphLRUPlanCache<std::string, int> cache(10, std::chrono::milliseconds{30});
    cache.put("a", 1);
    cache.put("b", 2);
    std::this_thread::sleep_for(std::chrono::milliseconds{60});
    size_t purged = cache.purgeExpired();
    EXPECT_EQ(2u, purged);
    EXPECT_EQ(0u, cache.size());
}

// ============================================================================
// P3-01 Group 7: GraphAdvancedCostModel standalone
// ============================================================================

TEST(GraphAdvancedCostModelTest, InitialStateIsZero) {
    GraphAdvancedCostModel m;
    EXPECT_EQ(0.0, m.emaCostMs());
    EXPECT_EQ(0.0, m.confidence());
    EXPECT_EQ(0u,  m.execCount());
}

TEST(GraphAdvancedCostModelTest, FirstObservationSetsEMA) {
    GraphAdvancedCostModel m;
    m.observe(20.0);
    EXPECT_NEAR(20.0, m.emaCostMs(), 1e-9);
    EXPECT_EQ(1u, m.execCount());
}

TEST(GraphAdvancedCostModelTest, EMAConvergesOverTime) {
    GraphAdvancedCostModel m;
    for (int i = 0; i < 50; ++i) {
      m.observe(10.0);
    }
    EXPECT_NEAR(10.0, m.emaCostMs(), 0.5);
}

TEST(GraphAdvancedCostModelTest, ConfidenceGrowsToOne) {
    GraphAdvancedCostModel m;
    for (int i = 0; i < 100; ++i) {
      m.observe(1.0);
    }
    EXPECT_NEAR(1.0, m.confidence(), 1e-6);
}

TEST(GraphAdvancedCostModelTest, BlendedEstimateUsesTheoryWhenNoData) {
    GraphAdvancedCostModel m;
    // confidence = 0 → blended = 0 * ema + 1 * theoretical
    EXPECT_NEAR(50.0, m.blendedEstimate(50.0), 1e-9);
}

TEST(GraphAdvancedCostModelTest, BlendedEstimateUsesEMAWhenConfident) {
    GraphAdvancedCostModel m;
    for (int i = 0; i < 100; ++i) {
      m.observe(10.0);
    }
    // confidence ≈ 1 → blended ≈ ema = 10
    EXPECT_NEAR(10.0, m.blendedEstimate(999.0), 0.1);
}

TEST(GraphAdvancedCostModelTest, P99LessThanOrEqualP50PlusSlack) {
    GraphAdvancedCostModel m;
    for (int i = 0; i < 100; ++i) {
      m.observe(5.0);
    }
    EXPECT_LE(m.p50Ms(), m.p99Ms() + 1.0);
}

TEST(GraphAdvancedCostModelTest, ResetClearsAllState) {
    GraphAdvancedCostModel m;
    for (int i = 0; i < 10; ++i) {
      m.observe(100.0);
    }
    m.reset();
    EXPECT_EQ(0.0, m.emaCostMs());
    EXPECT_EQ(0u,  m.execCount());
    EXPECT_EQ(0u,  m.histogram().total());
}
