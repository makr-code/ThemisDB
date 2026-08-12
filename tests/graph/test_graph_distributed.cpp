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
