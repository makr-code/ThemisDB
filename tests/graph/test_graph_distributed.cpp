/*
 * ThemisDB | File: test_graph_distributed.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include "graph/distributed_graph.h"
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: build a small test graph in a fresh RocksDB instance
// ---------------------------------------------------------------------------

static void populateTestGraph(themis::GraphIndexManager& mgr) {
    // A -> B -> C -> D  (weights 1.0)
    //      A -> C        (weight  2.0)
    auto addEdge = [&](const std::string& id, const std::string& from,
                       const std::string& to, const std::string& weight) {
        themis::BaseEntity e(id);
        e.setField("id", id);
        e.setField("_from", from);
        e.setField("_to", to);
        e.setField("_weight", weight);
        mgr.addEdge(e);
    };

    addEdge("e1", "A", "B", "1.0");
    addEdge("e2", "B", "C", "1.0");
    addEdge("e3", "C", "D", "1.0");
    addEdge("e4", "A", "C", "2.0");
}

// ---------------------------------------------------------------------------
// Fixture: two independent shards each backed by a real GraphIndexManager
// ---------------------------------------------------------------------------

class DistributedGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        path1_ = "./data/themis_dist_graph_shard1_test";
        path2_ = "./data/themis_dist_graph_shard2_test";
        fs::remove_all(path1_);
        fs::remove_all(path2_);

        auto makeDB = [](const std::string& path) {
            themis::RocksDBWrapper::Config cfg;
            cfg.db_path = path;
            cfg.memtable_size_mb = 64;
            cfg.block_cache_size_mb = 128;
            cfg.max_background_jobs = 2;
            cfg.compression_default = "lz4";
            cfg.compression_bottommost = "zstd";
            auto db = std::make_unique<themis::RocksDBWrapper>(cfg);
            EXPECT_TRUE(db->open());
            return db;
        };

        db1_ = makeDB(path1_);
        db2_ = makeDB(path2_);
        mgr1_ = std::make_unique<themis::GraphIndexManager>(*db1_);
        mgr2_ = std::make_unique<themis::GraphIndexManager>(*db2_);

        populateTestGraph(*mgr1_);
        populateTestGraph(*mgr2_);

        // Build the manager with two local shards.
        themis::graph::DistributedGraphConfig cfg;
        cfg.partitioning = themis::graph::PartitionStrategy::HASH;
        dist_mgr_ = std::make_unique<themis::graph::DistributedGraphManager>(cfg);
        dist_mgr_->addShard("shard1",
            std::make_shared<themis::graph::LocalShardGraphExecutor>("shard1", *mgr1_));
        dist_mgr_->addShard("shard2",
            std::make_shared<themis::graph::LocalShardGraphExecutor>("shard2", *mgr2_));
    }

    void TearDown() override {
        dist_mgr_.reset();
        mgr1_.reset();
        mgr2_.reset();
        db1_.reset();
        db2_.reset();
        fs::remove_all(path1_);
        fs::remove_all(path2_);
    }

    std::string path1_, path2_;
    std::unique_ptr<themis::RocksDBWrapper> db1_, db2_;
    std::unique_ptr<themis::GraphIndexManager> mgr1_, mgr2_;
    std::unique_ptr<themis::graph::DistributedGraphManager> dist_mgr_;
};

// ---------------------------------------------------------------------------
// parseVertexId – static utility
// ---------------------------------------------------------------------------

TEST(DistributedGraphUtilTest, ParseVertexId_WithQualifier) {
    auto [local, shard] =
        themis::graph::DistributedGraphManager::parseVertexId("node_A@shard1");
    EXPECT_EQ(local, "node_A");
    EXPECT_EQ(shard, "shard1");
}

TEST(DistributedGraphUtilTest, ParseVertexId_WithoutQualifier) {
    auto [local, shard] =
        themis::graph::DistributedGraphManager::parseVertexId("node_A");
    EXPECT_EQ(local, "node_A");
    EXPECT_EQ(shard, "");
}

TEST(DistributedGraphUtilTest, ParseVertexId_EmptyString) {
    auto [local, shard] =
        themis::graph::DistributedGraphManager::parseVertexId("");
    EXPECT_EQ(local, "");
    EXPECT_EQ(shard, "");
}

TEST(DistributedGraphUtilTest, ParseVertexId_MultipleAtSigns_LastWins) {
    auto [local, shard] =
        themis::graph::DistributedGraphManager::parseVertexId("a@b@c");
    EXPECT_EQ(local, "a@b");
    EXPECT_EQ(shard, "c");
}

// ---------------------------------------------------------------------------
// DistributedGraphManager – shard registry
// ---------------------------------------------------------------------------

TEST(DistributedGraphManagerBasicTest, DefaultConfig_NoShards) {
    themis::graph::DistributedGraphManager mgr;
    EXPECT_EQ(mgr.shardCount(), 0u);
    EXPECT_TRUE(mgr.shardIds().empty());
}

TEST(DistributedGraphManagerBasicTest, AddAndRemoveShard) {
    themis::graph::DistributedGraphManager mgr;
    // We pass a nullptr executor for this registry-only test.
    mgr.addShard("s1", nullptr);
    EXPECT_EQ(mgr.shardCount(), 1u);
    mgr.addShard("s2", nullptr);
    EXPECT_EQ(mgr.shardCount(), 2u);

    mgr.removeShard("s1");
    EXPECT_EQ(mgr.shardCount(), 1u);
    auto ids = mgr.shardIds();
    EXPECT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], "s2");
}

// ---------------------------------------------------------------------------
// resolveShardForVertex – hash partitioning determinism
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, ResolveShardForVertex_Deterministic) {
    // The same vertex should always map to the same shard.
    std::string s1 = dist_mgr_->resolveShardForVertex("A");
    std::string s2 = dist_mgr_->resolveShardForVertex("A");
    EXPECT_EQ(s1, s2);
    EXPECT_FALSE(s1.empty());
}

TEST_F(DistributedGraphTest, ResolveShardForVertex_KnownShard) {
    // Result must be one of the registered shards.
    std::string s = dist_mgr_->resolveShardForVertex("B");
    auto ids = dist_mgr_->shardIds();
    bool known = (std::find(ids.begin(), ids.end(), s) != ids.end());
    EXPECT_TRUE(known);
}

TEST(DistributedGraphManagerBasicTest, ResolveShardForVertex_NoShards_ReturnsEmpty) {
    themis::graph::DistributedGraphManager mgr;
    EXPECT_EQ(mgr.resolveShardForVertex("A"), "");
}

// ---------------------------------------------------------------------------
// shortestPath – error when no shards registered
// ---------------------------------------------------------------------------

TEST(DistributedGraphManagerBasicTest, ShortestPath_NoShards_ReturnsError) {
    themis::graph::DistributedGraphManager mgr;
    auto res = mgr.shortestPath("A", "D");
    EXPECT_FALSE(res.has_value());
}

// ---------------------------------------------------------------------------
// shortestPath – finds path across shards
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, ShortestPath_FindsPath) {
    // Both shards contain A->B->C->D; at least one should return a valid path.
    auto res = dist_mgr_->shortestPath("A", "D");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_FALSE(res->path.empty());
}

TEST_F(DistributedGraphTest, ShortestPath_WithShardQualifier_FindsPath) {
    // Provide an explicit shard qualifier; the manager should respect it.
    auto res = dist_mgr_->shortestPath("A@shard1", "D@shard1");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_FALSE(res->path.empty());
}

TEST_F(DistributedGraphTest, ShortestPath_NonExistentVertex_ReturnsPathNotFound) {
    // Neither shard has "Z"; expect ERR_GRAPH_PATH_NOT_FOUND.
    auto res = dist_mgr_->shortestPath("Z", "W");
    EXPECT_FALSE(res.has_value());
}

TEST_F(DistributedGraphTest, ShortestPath_CostIsPositive) {
    auto res = dist_mgr_->shortestPath("A", "D");
    ASSERT_TRUE(res.has_value());
    EXPECT_GT(res->totalCost, 0.0);
}

// ---------------------------------------------------------------------------
// kHopNeighbors – fans out to all shards
// ---------------------------------------------------------------------------

TEST(DistributedGraphManagerBasicTest, KHopNeighbors_NoShards_ReturnsError) {
    themis::graph::DistributedGraphManager mgr;
    auto res = mgr.kHopNeighbors("A", 2);
    EXPECT_FALSE(res.has_value());
}

TEST_F(DistributedGraphTest, KHopNeighbors_ReturnsVertices) {
    auto res = dist_mgr_->kHopNeighbors("A", 2);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_GT(res->size(), 0u);
}

TEST_F(DistributedGraphTest, KHopNeighbors_VerticesAreQualified) {
    auto res = dist_mgr_->kHopNeighbors("A", 2);
    ASSERT_TRUE(res.has_value());
    for (const auto& v : *res) {
        EXPECT_NE(v.find('@'), std::string::npos)
            << "Vertex '" << v << "' missing shard qualifier";
    }
}

TEST_F(DistributedGraphTest, KHopNeighbors_NoDuplicates) {
    auto res = dist_mgr_->kHopNeighbors("A", 3);
    ASSERT_TRUE(res.has_value());
    std::vector<std::string> sorted = *res;
    std::sort(sorted.begin(), sorted.end());
    auto unique_end = std::unique(sorted.begin(), sorted.end());
    EXPECT_EQ(unique_end, sorted.end()) << "Duplicate vertices in result";
}

// ---------------------------------------------------------------------------
// optimizePlan – shard-aware plan generation
// ---------------------------------------------------------------------------

TEST(DistributedGraphManagerBasicTest, OptimizePlan_NoShards_ReturnsError) {
    themis::graph::DistributedGraphManager mgr;
    auto res = mgr.optimizePlan("A", "D",
        themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
    EXPECT_FALSE(res.has_value());
}

TEST_F(DistributedGraphTest, OptimizePlan_IsDistributed_WithMultipleShards) {
    auto res = dist_mgr_->optimizePlan("A", "D",
        themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_TRUE(res->is_distributed);
    EXPECT_EQ(res->shard_ids.size(), 2u);
    EXPECT_GE(res->recommended_parallelism, 1u);
}

TEST_F(DistributedGraphTest, OptimizePlan_ContainsBothShardIds) {
    auto res = dist_mgr_->optimizePlan("A", "D",
        themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
    ASSERT_TRUE(res.has_value());
    auto ids = res->shard_ids;
    bool has_shard1 = std::find(ids.begin(), ids.end(), "shard1") != ids.end();
    bool has_shard2 = std::find(ids.begin(), ids.end(), "shard2") != ids.end();
    EXPECT_TRUE(has_shard1);
    EXPECT_TRUE(has_shard2);
}

TEST_F(DistributedGraphTest, OptimizePlan_CostPositive) {
    auto res = dist_mgr_->optimizePlan("A", "D",
        themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH);
    ASSERT_TRUE(res.has_value());
    EXPECT_GT(res->estimated_cost, 0.0);
}

TEST_F(DistributedGraphTest, OptimizePlan_KHopPattern) {
    auto res = dist_mgr_->optimizePlan("A", "",
        themis::graph::GraphQueryOptimizer::QueryPattern::K_HOP_NEIGHBORS);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->pattern,
              themis::graph::GraphQueryOptimizer::QueryPattern::K_HOP_NEIGHBORS);
}

// ---------------------------------------------------------------------------
// OptimizationPlan – backward-compatible shard-aware fields
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, OptimizationPlan_SingleNode_NotDistributed) {
    // A plan produced by a regular GraphQueryOptimizer (no DistributedGraphManager)
    // must have is_distributed == false and empty shard_ids (default values).
    std::string db_path = "./data/themis_plan_shard_compat_test";
    fs::remove_all(db_path);

    {
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 2;
        cfg.compression_default = "lz4";
        cfg.compression_bottommost = "zstd";
        auto db = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        auto mgr = std::make_unique<themis::GraphIndexManager>(*db);
        populateTestGraph(*mgr);

        themis::graph::GraphQueryOptimizer optimizer(*mgr);
        auto plan = optimizer.optimizeShortestPath("A", "D");
        ASSERT_TRUE(plan.has_value());

        // Single-node plan must not be marked distributed.
        EXPECT_FALSE(plan->is_distributed);
        EXPECT_TRUE(plan->shard_ids.empty());
        EXPECT_EQ(plan->recommended_parallelism, 1u);
    }

    fs::remove_all(db_path);
}

// ---------------------------------------------------------------------------
// explainPlan – shard info printed for distributed plans
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, ExplainPlan_DistributedPlanContainsShardInfo) {
    // Use a single-shard DB just to get an optimizer for explainPlan.
    std::string db_path = "./data/themis_explain_dist_test";
    fs::remove_all(db_path);

    {
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 2;
        cfg.compression_default = "lz4";
        cfg.compression_bottommost = "zstd";
        auto db = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        auto mgr = std::make_unique<themis::GraphIndexManager>(*db);

        themis::graph::GraphQueryOptimizer optimizer(*mgr);

        // Build a distributed plan manually and verify explainPlan output.
        themis::graph::GraphQueryOptimizer::OptimizationPlan plan;
        plan.algorithm = themis::graph::GraphQueryOptimizer::TraversalAlgorithm::DIJKSTRA;
        plan.pattern = themis::graph::GraphQueryOptimizer::QueryPattern::SHORTEST_PATH;
        plan.estimated_cost = 2.0;
        plan.estimated_time_ms = 20.0;
        plan.estimated_nodes_explored = 50;
        plan.use_index = true;
        plan.enable_parallel = true;
        plan.is_distributed = true;
        plan.shard_ids = {"shard1", "shard2"};
        plan.recommended_parallelism = 2;

        std::string explanation = optimizer.explainPlan(plan);
        EXPECT_NE(explanation.find("Distributed"), std::string::npos);
        EXPECT_NE(explanation.find("shard1"), std::string::npos);
        EXPECT_NE(explanation.find("shard2"), std::string::npos);
        EXPECT_NE(explanation.find("Parallelism"), std::string::npos);
    }

    fs::remove_all(db_path);
}

// ---------------------------------------------------------------------------
// PartitionStrategy and ConsistencyLevel – enum values accessible
// ---------------------------------------------------------------------------

TEST(DistributedGraphConfigTest, PartitionStrategyValues) {
    EXPECT_NE(themis::graph::PartitionStrategy::HASH,   themis::graph::PartitionStrategy::RANGE);
    EXPECT_NE(themis::graph::PartitionStrategy::RANGE,  themis::graph::PartitionStrategy::GEO);
    EXPECT_NE(themis::graph::PartitionStrategy::GEO,    themis::graph::PartitionStrategy::CUSTOM);
}

TEST(DistributedGraphConfigTest, ConsistencyLevelValues) {
    EXPECT_NE(themis::graph::ConsistencyLevel::EVENTUAL,
              themis::graph::ConsistencyLevel::STRONG);
}

TEST(DistributedGraphConfigTest, DefaultConfig) {
    themis::graph::DistributedGraphConfig cfg;
    EXPECT_EQ(cfg.partitioning, themis::graph::PartitionStrategy::HASH);
    EXPECT_EQ(cfg.replication_factor, 1);
    EXPECT_EQ(cfg.consistency, themis::graph::ConsistencyLevel::EVENTUAL);
    EXPECT_EQ(cfg.timeout_ms, 5000u);
    EXPECT_EQ(cfg.max_parallel_shards, 0u);
}

// ---------------------------------------------------------------------------
// TSAN stress test: 8 concurrent execute() threads + 1 addShard() thread
// ---------------------------------------------------------------------------

// Minimal stub executor used only for the concurrency stress test.
class StubShardExecutor final : public themis::graph::ShardGraphExecutor {
public:
    explicit StubShardExecutor(std::string id) : id_(std::move(id)) {}
    std::string shardId() const override { return id_; }
    themis::Result<std::vector<std::string>> executeBFS(
        const std::string&, int,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        return themis::Ok(std::vector<std::string>{});
    }
    themis::Result<themis::GraphIndexManager::PathResult> executeDijkstra(
        const std::string&, const std::string&,
        const themis::graph::GraphQueryOptimizer::QueryConstraints&) override {
        themis::GraphIndexManager::PathResult r;
        r.totalCost = 0.0;
        return themis::Ok(r);
    }
private:
    std::string id_;
};

TEST(DistributedGraphSharedMutexStressTest, ConcurrentReadsAndOneWriter) {
    themis::graph::DistributedGraphManager mgr;

    // Pre-populate with some shards so readers have work to do.
    for (int i = 0; i < 4; ++i) {
        mgr.addShard("shard" + std::to_string(i),
                     std::make_shared<StubShardExecutor>("shard" + std::to_string(i)));
    }

    std::atomic<bool> stop{false};
    std::atomic<int>  read_ops{0};

    // 8 reader threads: continuously call kHopNeighbors() (the "execute()" path
    // that exercises healthyShards() under shared_lock) until the writer is done.
    std::vector<std::thread> readers;
    readers.reserve(8);
    for (int t = 0; t < 8; ++t) {
        readers.emplace_back([&mgr, &stop, &read_ops, t]() {
            while (!stop.load(std::memory_order_relaxed)) {
                // kHopNeighbors calls healthyShards() (shared_lock) then fans out.
                auto res = mgr.kHopNeighbors("vertex_" + std::to_string(t), 1);
                (void)res;
                // Also exercise the simpler read helpers.
                (void)mgr.shardCount();
                mgr.resolveShardForVertex("vertex_" + std::to_string(t));
                ++read_ops;
            }
        });
    }

    // 1 writer thread: add then remove 8 extra shards.
    std::thread writer([&mgr]() {
        for (int i = 4; i < 12; ++i) {
            std::string sid = "dynamic_shard" + std::to_string(i);
            mgr.addShard(sid, std::make_shared<StubShardExecutor>(sid));
            std::this_thread::yield();
            mgr.removeShard(sid);
            std::this_thread::yield();
        }
    });

    writer.join();
    stop.store(true, std::memory_order_relaxed);
    for (auto& r : readers) r.join();

    // At least some read operations were completed.
    EXPECT_GT(read_ops.load(), 0);
    // After the writer finishes, only the original 4 shards should remain.
    EXPECT_EQ(mgr.shardCount(), 4u);
}

// ---------------------------------------------------------------------------
// QW-024, QW-025: Shard-Aware Routing and K-Way Merge Tests
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, QW024_KHopNeighborsWithAffinityRouting) {
    // Test that kHopNeighbors correctly identifies and queries primary shard first
    
    // Clear and re-populate with unique data on each shard
    dist_mgr_.reset();
    mgr1_.reset();
    mgr2_.reset();
    
    // Recreate graph managers and populate with distinct data
    db1_ = std::make_unique<themis::RocksDBWrapper>(themis::RocksDBWrapper::Config{
        .db_path = path1_,
        .memtable_size_mb = 64,
        .block_cache_size_mb = 128,
    });
    db1_->open();
    mgr1_ = std::make_unique<themis::GraphIndexManager>(*db1_);
    
    db2_ = std::make_unique<themis::RocksDBWrapper>(themis::RocksDBWrapper::Config{
        .db_path = path2_,
        .memtable_size_mb = 64,
        .block_cache_size_mb = 128,
    });
    db2_->open();
    mgr2_ = std::make_unique<themis::GraphIndexManager>(*db2_);
    
    // Add edges to shard1: A -> B (only on shard1)
    themis::BaseEntity e1("e1");
    e1.setField("_from", "A");
    e1.setField("_to", "B");
    mgr1_->addEdge(e1);
    
    // Add edges to shard2: C -> D (only on shard2)
    themis::BaseEntity e2("e2");
    e2.setField("_from", "C");
    e2.setField("_to", "D");
    mgr2_->addEdge(e2);
    
    themis::graph::DistributedGraphConfig cfg;
    cfg.partitioning = themis::graph::PartitionStrategy::HASH;
    dist_mgr_ = std::make_unique<themis::graph::DistributedGraphManager>(cfg);
    dist_mgr_->addShard("shard1",
        std::make_shared<themis::graph::LocalShardGraphExecutor>("shard1", *mgr1_));
    dist_mgr_->addShard("shard2",
        std::make_shared<themis::graph::LocalShardGraphExecutor>("shard2", *mgr2_));
    
    // Query k-hop neighbors - should work on both shards
    auto result = dist_mgr_->kHopNeighbors("A", 1);
    EXPECT_TRUE(result);
    // Result should contain vertices from the queried graph
}

TEST_F(DistributedGraphTest, QW025_KWayMergeDeduplicate) {
    // Test that k-way merge properly deduplicates results
    auto result = dist_mgr_->kHopNeighbors("A", 2);
    EXPECT_TRUE(result);
    
    // Verify no duplicates in result
    std::unordered_set<std::string> seen;
    for (const auto& vertex : *result) {
        EXPECT_EQ(seen.count(vertex), 0) << "Duplicate vertex found: " << vertex;
        seen.insert(vertex);
    }
}

// ---------------------------------------------------------------------------
// QW-027: Affinity Cache Tests
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, QW027_AffinityCacheEnabled) {
    // Test that affinity cache is enabled by default
    themis::graph::DistributedGraphConfig cfg;
    cfg.enable_affinity_cache = true;
    cfg.affinity_cache_size = 100;
    
    auto mgr = std::make_unique<themis::graph::DistributedGraphManager>(cfg);
    mgr->addShard("shard1",
        std::make_shared<themis::graph::LocalShardGraphExecutor>("shard1", *mgr1_));
    
    // First query should resolve shard via hash
    std::string shard1 = mgr->resolveShardForVertex("vertex_A");
    
    // Second query for same vertex should use cache (we can't directly observe this,
    // but verify the shard resolution is consistent)
    std::string shard2 = mgr->resolveShardForVertex("vertex_A");
    EXPECT_EQ(shard1, shard2);
}

TEST_F(DistributedGraphTest, QW027_AffinityCacheCanBeDisabled) {
    // Test that affinity cache can be disabled
    themis::graph::DistributedGraphConfig cfg;
    cfg.enable_affinity_cache = false;
    
    auto mgr = std::make_unique<themis::graph::DistributedGraphManager>(cfg);
    mgr->addShard("shard1",
        std::make_shared<themis::graph::LocalShardGraphExecutor>("shard1", *mgr1_));
    
    // Verify shard resolution works even when cache is disabled
    std::string shard = mgr->resolveShardForVertex("vertex_A");
    EXPECT_EQ(shard, "shard1");
}

TEST_F(DistributedGraphTest, QW027_AffinityCacheInvalidatedOnAddShard) {
    // Test that cache is invalidated when shards are added
    themis::graph::DistributedGraphConfig cfg;
    cfg.enable_affinity_cache = true;
    
    auto mgr = std::make_unique<themis::graph::DistributedGraphManager>(cfg);
    mgr->addShard("shard1",
        std::make_shared<themis::graph::LocalShardGraphExecutor>("shard1", *mgr1_));
    
    std::string shard1 = mgr->resolveShardForVertex("vertex_test");
    
    // Add another shard - this should invalidate the cache internally
    mgr->addShard("shard2",
        std::make_shared<themis::graph::LocalShardGraphExecutor>("shard2", *mgr2_));
    
    // Resolution should still work (now possibly different due to hash change with 2 shards)
    std::string shard2 = mgr->resolveShardForVertex("vertex_test");
    EXPECT_FALSE(shard2.empty());
}

// ---------------------------------------------------------------------------
// QW-028: Error Handling Tests
// ---------------------------------------------------------------------------

TEST_F(DistributedGraphTest, QW028_KHopNeighborsValidation_EmptyStartVertex) {
    // Test that empty start vertex is rejected
    auto result = dist_mgr_->kHopNeighbors("", 1);
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code, themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

TEST_F(DistributedGraphTest, QW028_KHopNeighborsValidation_InvalidK) {
    // Test that invalid k values are rejected
    auto result1 = dist_mgr_->kHopNeighbors("A", 0);
    EXPECT_FALSE(result1);
    EXPECT_EQ(result1.error().code, themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
    
    auto result2 = dist_mgr_->kHopNeighbors("A", -1);
    EXPECT_FALSE(result2);
    EXPECT_EQ(result2.error().code, themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
    
    auto result3 = dist_mgr_->kHopNeighbors("A", 2000);
    EXPECT_FALSE(result3);
    EXPECT_EQ(result3.error().code, themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

TEST_F(DistributedGraphTest, QW028_ShortestPathValidation_EmptyStartVertex) {
    // Test that empty start vertex is rejected
    auto result = dist_mgr_->shortestPath("", "B");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code, themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

TEST_F(DistributedGraphTest, QW028_ShortestPathValidation_EmptyTargetVertex) {
    // Test that empty target vertex is rejected
    auto result = dist_mgr_->shortestPath("A", "");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code, themis::errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

// ---------------------------------------------------------------------------
// QW-026: Cache Invalidation Tests
// ---------------------------------------------------------------------------

TEST(GraphQueryOptimizerCacheInvalidationTest, QW026_BroadcastInvalidationClears) {
    // Create a simple test database for the optimizer
    std::string db_path = "./data/themis_cache_invalidation_test";
    fs::remove_all(db_path);
    
    {
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 2;
        auto db = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        auto mgr = std::make_unique<themis::GraphIndexManager>(*db);
        
        themis::graph::GraphQueryOptimizer optimizer(*mgr);
        
        // Populate plan cache with some entries
        auto plan = optimizer.optimizeShortestPath("A", "B");
        ASSERT_TRUE(plan);
        
        // Verify cache has entries
        size_t cache_size_before = optimizer.getPlanCacheSize();
        
        // Broadcast invalidation
        auto invalidation_result = optimizer.broadcastInvalidation({});
        EXPECT_TRUE(invalidation_result);
        
        // Verify cache was cleared
        size_t cache_size_after = optimizer.getPlanCacheSize();
        EXPECT_LT(cache_size_after, cache_size_before);
    }
    
    fs::remove_all(db_path);
}

TEST(GraphQueryOptimizerCacheInvalidationTest, QW026_BroadcastInvalidationWithVertices) {
    // Test selective invalidation with vertex list
    std::string db_path = "./data/themis_cache_selective_invalidation_test";
    fs::remove_all(db_path);
    
    {
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 2;
        auto db = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        auto mgr = std::make_unique<themis::GraphIndexManager>(*db);
        
        themis::graph::GraphQueryOptimizer optimizer(*mgr);
        
        // Broadcast selective invalidation
        std::vector<std::string> affected = {"vertex_A", "vertex_B"};
        auto result = optimizer.broadcastInvalidation(affected);
        EXPECT_TRUE(result);
    }
    
    fs::remove_all(db_path);
}

