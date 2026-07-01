/*
 * ThemisDB | File: test_graph_phase3.cpp
 * Phase 3 focused tests — Error Handling, Performance & Distributed Betweenness Centrality
 *
 * BC-01  Empty graph  → all scores 0 (empty result set)
 * BC-02  Single vertex (self-loop topology) → score 0
 * BC-03  Linear chain A→B→C → B has strictly higher centrality than A and C
 * BC-04  Bidirectional star (center ↔ 5 leaves) → center has maximum centrality
 * BC-05  sample_fraction validation: 0.0 → error, 1.01 → error, 0.5 → success
 * BC-06  Multi-shard aggregation: results merged from 2 shards, qualified IDs present
 * BC-07  Timeout / configuration: call completes without hang; shards_queried ≤ shard count
 * BC-08  Normalization: all scores in [0, 1]
 * STR-01 explainPlan() returns non-empty string containing expected substrings
 */

#include <gtest/gtest.h>

#include "graph/distributed_graph.h"
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Create a unique temporary path for a test RocksDB instance.
static std::string tmpPath(const char* label) {
    return (fs::temp_directory_path() /
            (std::string("themis_phase3_") + label + "_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
        .string();
}

/// RAII wrapper: owns a RocksDB + GraphIndexManager + LocalShardGraphExecutor.
struct ShardFixture {
    std::string path;
    std::unique_ptr<themis::RocksDBWrapper> db;
    std::unique_ptr<themis::GraphIndexManager> mgr;
    std::shared_ptr<themis::graph::LocalShardGraphExecutor> exec;

    explicit ShardFixture(const char* label, const char* shard_id) {
        path = tmpPath(label);
        fs::remove_all(path);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path                  = path;
        cfg.memtable_size_mb         = 64;
        cfg.block_cache_size_mb      = 128;
        cfg.max_background_jobs      = 2;
        cfg.compression_default      = "lz4";
        cfg.compression_bottommost   = "zstd";

        db = std::make_unique<themis::RocksDBWrapper>(cfg);
        EXPECT_TRUE(db->open());
        mgr  = std::make_unique<themis::GraphIndexManager>(*db);
        exec = std::make_shared<themis::graph::LocalShardGraphExecutor>(shard_id, *mgr);
    }

    ~ShardFixture() {
        exec.reset();
        mgr.reset();
        db.reset();
        fs::remove_all(path);
    }

    /// Add a directed edge id:from→to with default weight 1.0.
    void addEdge(const std::string& id, const std::string& from, const std::string& to) {
        themis::BaseEntity e(id);
        e.setField("id",      id);
        e.setField("_from",   from);
        e.setField("_to",     to);
        e.setField("_weight", "1.0");
        mgr->addEdge(e);
    }
};

/// Build a single-shard DistributedGraphManager from a ShardFixture.
static std::unique_ptr<themis::graph::DistributedGraphManager>
makeSingleShardMgr(ShardFixture& shard) {
    themis::graph::DistributedGraphConfig cfg;
    cfg.partitioning = themis::graph::PartitionStrategy::HASH;
    auto dist_mgr = std::make_unique<themis::graph::DistributedGraphManager>(cfg);
    dist_mgr->addShard(shard.exec->shardId(), shard.exec);
    return dist_mgr;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// STR-01: explainPlan() uses ostringstream, returns non-empty string with
//         expected substrings — regression guard for Batch A.
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3StringOpt, ExplainPlanNonEmptyWithExpectedSubstrings) {
    ShardFixture shard("str01", "s0");
    shard.addEdge("e1", "A", "B");

    themis::graph::GraphQueryOptimizer optimizer(*shard.mgr);
    themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
    auto plan_res = optimizer.optimizeShortestPath("A", "B", constraints);
    ASSERT_TRUE(plan_res.has_value()) << "optimizePlan must succeed";

    const std::string explanation = optimizer.explainPlan(*plan_res);
    EXPECT_FALSE(explanation.empty()) << "explainPlan() must return a non-empty string";
    EXPECT_NE(explanation.find("Query Pattern"),   std::string::npos) << "missing 'Query Pattern'";
    EXPECT_NE(explanation.find("Algorithm"),       std::string::npos) << "missing 'Algorithm'";
    EXPECT_NE(explanation.find("Estimated Cost"),  std::string::npos) << "missing 'Estimated Cost'";
    EXPECT_NE(explanation.find("Use Index"),       std::string::npos) << "missing 'Use Index'";
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-01: Empty graph (no edges) → computeLocalBetweenness returns empty map;
//        computeBetweennessCentrality on a shard with no vertices returns
//        BetweennessResult with an empty scores map.
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC01_EmptyGraph_AllScoresZero) {
    ShardFixture shard("bc01", "s0");
    // No edges added — getAllVertices() returns empty.

    auto dist_mgr = makeSingleShardMgr(shard);
    auto result = dist_mgr->computeBetweennessCentrality(1.0);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result->scores.empty())
        << "Empty graph must produce no betweenness scores";
    EXPECT_EQ(result->shards_queried, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-02: Single vertex (self-loop) → betweenness score is 0.
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC02_SingleVertex_ScoreZero) {
    ShardFixture shard("bc02", "s0");
    shard.addEdge("e1", "A", "A"); // self-loop to register vertex A

    auto dist_mgr = makeSingleShardMgr(shard);
    auto result = dist_mgr->computeBetweennessCentrality(1.0);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    // Betweenness of a single-vertex graph is always 0 — no (s,v,t) triple exists.
    for (const auto& [vid, score] : result->scores) {
        EXPECT_DOUBLE_EQ(score, 0.0)
            << "Single-vertex graph: score of " << vid << " must be 0";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-03: Linear chain A→B→C — B is the only intermediary; B must have strictly
//        higher normalized betweenness than A and C (both leaves).
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC03_LinearChain_MiddleNodeHighest) {
    ShardFixture shard("bc03", "s0");
    shard.addEdge("e1", "A", "B");
    shard.addEdge("e2", "B", "C");

    auto dist_mgr = makeSingleShardMgr(shard);
    auto result = dist_mgr->computeBetweennessCentrality(1.0);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_FALSE(result->scores.empty());

    // Locate qualified vertex IDs for A, B, C.
    double score_A = -1.0, score_B = -1.0, score_C = -1.0;
    for (const auto& [vid, score] : result->scores) {
        if (vid.rfind("A@", 0) == 0) score_A = score;
        if (vid.rfind("B@", 0) == 0) score_B = score;
        if (vid.rfind("C@", 0) == 0) score_C = score;
    }

    EXPECT_GE(score_A, 0.0) << "A must be present in scores";
    EXPECT_GE(score_B, 0.0) << "B must be present in scores";
    EXPECT_GE(score_C, 0.0) << "C must be present in scores";

    EXPECT_GT(score_B, score_A)
        << "B (intermediary) must have higher betweenness than A (source leaf)";
    EXPECT_GT(score_B, score_C)
        << "B (intermediary) must have higher betweenness than C (target leaf)";
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-04: Bidirectional star — center ↔ 5 leaves — center must have the maximum
//        betweenness score (all inter-leaf paths pass through center).
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC04_StarGraph_CenterHasMaxCentrality) {
    ShardFixture shard("bc04", "s0");
    const std::vector<std::string> leaves = {"L1", "L2", "L3", "L4", "L5"};
    int eid = 1;
    for (const auto& leaf : leaves) {
        shard.addEdge("e" + std::to_string(eid++), "C", leaf);  // center → leaf
        shard.addEdge("e" + std::to_string(eid++), leaf, "C");  // leaf → center
    }

    auto dist_mgr = makeSingleShardMgr(shard);
    auto result = dist_mgr->computeBetweennessCentrality(1.0);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_FALSE(result->scores.empty());

    double center_score = -1.0;
    double max_score    =  0.0;
    for (const auto& [vid, score] : result->scores) {
        if (vid.rfind("C@", 0) == 0) center_score = score;
        max_score = std::max(max_score, score);
    }

    EXPECT_GE(center_score, 0.0) << "Center must appear in scores";
    EXPECT_DOUBLE_EQ(center_score, max_score)
        << "Center must have the globally maximum betweenness score in a star graph";
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-05: sample_fraction validation — boundary conditions.
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC05_SampleFractionValidation) {
    ShardFixture shard("bc05", "s0");
    shard.addEdge("e1", "A", "B");
    shard.addEdge("e2", "B", "C");

    auto dist_mgr = makeSingleShardMgr(shard);

    // 0.0 is below the [0.01, 1.0] range → must return an error.
    auto bad_low = dist_mgr->computeBetweennessCentrality(0.0);
    EXPECT_FALSE(bad_low.has_value())
        << "sample_fraction=0.0 must be rejected with an error";

    // 1.01 exceeds the upper bound → must return an error.
    auto bad_high = dist_mgr->computeBetweennessCentrality(1.01);
    EXPECT_FALSE(bad_high.has_value())
        << "sample_fraction=1.01 must be rejected with an error";

    // 0.5 is within range → must succeed.
    auto ok_half = dist_mgr->computeBetweennessCentrality(0.5);
    EXPECT_TRUE(ok_half.has_value())
        << "sample_fraction=0.5 must succeed: " << (ok_half ? "" : ok_half.error().message());

    // Boundary 0.01 → must succeed.
    auto ok_min = dist_mgr->computeBetweennessCentrality(0.01);
    EXPECT_TRUE(ok_min.has_value())
        << "sample_fraction=0.01 must succeed: " << (ok_min ? "" : ok_min.error().message());

    // Boundary 1.0 → must succeed.
    auto ok_full = dist_mgr->computeBetweennessCentrality(1.0);
    EXPECT_TRUE(ok_full.has_value())
        << "sample_fraction=1.0 must succeed: " << (ok_full ? "" : ok_full.error().message());
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-06: Multi-shard aggregation — results from 2 shards are merged; qualified
//        vertex IDs from both shards appear in the output.
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC06_MultiShardAggregation) {
    ShardFixture s1("bc06_s1", "shard1");
    ShardFixture s2("bc06_s2", "shard2");

    // Each shard has a linear chain A→B→C.
    for (auto* s : {&s1, &s2}) {
        s->addEdge("e1", "A", "B");
        s->addEdge("e2", "B", "C");
    }

    themis::graph::DistributedGraphConfig cfg;
    cfg.partitioning = themis::graph::PartitionStrategy::HASH;
    themis::graph::DistributedGraphManager dist_mgr(cfg);
    dist_mgr.addShard("shard1", s1.exec);
    dist_mgr.addShard("shard2", s2.exec);

    auto result = dist_mgr.computeBetweennessCentrality(1.0);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->shards_queried, 2u) << "Both shards must contribute";

    // Verify that qualified IDs from both shards are present.
    bool found_shard1 = false, found_shard2 = false;
    for (const auto& [vid, score] : result->scores) {
        if (vid.find("@shard1") != std::string::npos) found_shard1 = true;
        if (vid.find("@shard2") != std::string::npos) found_shard2 = true;
    }
    EXPECT_TRUE(found_shard1) << "Results must include shard1-qualified vertex IDs";
    EXPECT_TRUE(found_shard2) << "Results must include shard2-qualified vertex IDs";
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-07: Timeout / configuration — call completes without hang;
//        shards_queried is within [0, shard_count].
//        Uses config.timeout_ms = 0 (no timeout) to verify normal completion.
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC07_TimeoutConfigDoesNotHang) {
    ShardFixture shard("bc07", "s0");
    shard.addEdge("e1", "A", "B");
    shard.addEdge("e2", "B", "C");

    themis::graph::DistributedGraphConfig cfg;
    cfg.timeout_ms   = 0;  // 0 = no per-call timeout (unlimited)
    cfg.partitioning = themis::graph::PartitionStrategy::HASH;
    themis::graph::DistributedGraphManager dist_mgr(cfg);
    dist_mgr.addShard(shard.exec->shardId(), shard.exec);

    auto result = dist_mgr.computeBetweennessCentrality(1.0);
    ASSERT_TRUE(result.has_value()) << "No-timeout config must succeed";
    EXPECT_LE(result->shards_queried, dist_mgr.shardCount())
        << "shards_queried must not exceed total shard count";
    EXPECT_GE(result->shards_queried, 1u)
        << "At least one shard must be queried";
}

// ─────────────────────────────────────────────────────────────────────────────
// BC-08: Normalization — every score in the result must lie in [0, 1].
// ─────────────────────────────────────────────────────────────────────────────

TEST(GraphPhase3Betweenness, BC08_NormalizationAllScoresInRange) {
    ShardFixture shard("bc08", "s0");
    // Slightly richer topology to produce diverse betweenness values.
    // A→B→C, A→C (A can bypass B), D→B (extra path into B).
    shard.addEdge("e1", "A", "B");
    shard.addEdge("e2", "B", "C");
    shard.addEdge("e3", "A", "C");
    shard.addEdge("e4", "D", "B");
    shard.addEdge("e5", "B", "D");

    auto dist_mgr = makeSingleShardMgr(shard);
    auto result = dist_mgr->computeBetweennessCentrality(1.0);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_FALSE(result->scores.empty());

    for (const auto& [vid, score] : result->scores) {
        EXPECT_GE(score, 0.0)
            << "Score for vertex " << vid << " must be >= 0";
        EXPECT_LE(score, 1.0)
            << "Score for vertex " << vid << " must be <= 1";
        EXPECT_FALSE(std::isnan(score))
            << "Score for vertex " << vid << " must not be NaN";
    }

    // At least one vertex must have the maximum score of 1.0.
    bool has_max = false;
    for (const auto& [vid, score] : result->scores) {
        if (std::abs(score - 1.0) < 1e-9) { has_max = true; break; }
    }
    EXPECT_TRUE(has_max) << "Normalization must produce at least one score of 1.0";
}
