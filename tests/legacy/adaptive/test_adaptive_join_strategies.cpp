/**
 * @file test_adaptive_join_strategies.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "query/adaptive_join.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Create a table with `n` rows, where each row has a single column "id"
/// whose value is the string representation of [0, n).
Table makeTable(const std::string& name, size_t n,
                bool sorted = false, bool has_index = false) {
    Table t;
    t.name      = name;
    t.is_sorted = sorted;
    t.has_index = has_index;
    t.rows.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        RowValue row;
        row["id"] = std::to_string(i);
        t.rows.push_back(std::move(row));
    }
    return t;
}

/// Create a table where ALL rows share the same key value.
Table makeTableSameKey(const std::string& name, size_t n, const std::string& key_val) {
    Table t;
    t.name = name;
    t.rows.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        RowValue row;
        row["id"]  = key_val;
        row["seq"] = std::to_string(i);
        t.rows.push_back(std::move(row));
    }
    return t;
}

/// Build a default equi-join spec on column "id".
JoinSpec makeSpec() {
    JoinSpec s;
    s.left_key  = "id";
    s.right_key = "id";
    return s;
}

/// Build a RuntimeStats with sensible defaults (large memory, non-distributed).
RuntimeStats defaultStats() {
    RuntimeStats rs;
    rs.memory_budget_bytes = 256ULL * 1024ULL * 1024ULL;  // 256 MiB
    rs.bytes_per_row       = 256;
    rs.is_distributed      = false;
    rs.grace_hash_threshold = 0.9;
    return rs;
}

} // namespace

// ============================================================================
// joinAlgorithmName
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameHashJoin) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::HASH_JOIN), "HASH_JOIN");
}

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameMergeJoin) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::MERGE_JOIN), "MERGE_JOIN");
}

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameNestedLoop) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::NESTED_LOOP_JOIN), "NESTED_LOOP_JOIN");
}

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameIndexNestedLoop) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::INDEX_NESTED_LOOP), "INDEX_NESTED_LOOP");
}

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameGraceHash) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::GRACE_HASH_JOIN), "GRACE_HASH_JOIN");
}

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameBroadcast) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::BROADCAST_JOIN), "BROADCAST_JOIN");
}

TEST(AdaptiveJoinStrategiesTest, AlgorithmNameShuffle) {
    EXPECT_STREQ(joinAlgorithmName(JoinAlgorithm::SHUFFLE_JOIN), "SHUFFLE_JOIN");
}

// ============================================================================
// estimateJoinCost
// ============================================================================

TEST(AdaptiveJoinCostModelTest, HashJoinCostIsLinear) {
    // HASH_JOIN cost = L + R
    double cost = estimateJoinCost(JoinAlgorithm::HASH_JOIN, 1000, 2000);
    EXPECT_DOUBLE_EQ(cost, 3000.0);
}

TEST(AdaptiveJoinCostModelTest, MergeJoinSortedCostIsLinear) {
    // Both sorted: cost = L + R only
    double cost = estimateJoinCost(JoinAlgorithm::MERGE_JOIN, 500, 500, /*left_sorted*/true, /*right_sorted*/true);
    EXPECT_DOUBLE_EQ(cost, 1000.0);
}

TEST(AdaptiveJoinCostModelTest, MergeJoinUnsortedAddsSortCost) {
    // Unsorted: cost = L + R + L*log2(L) + R*log2(R)
    double cost = estimateJoinCost(JoinAlgorithm::MERGE_JOIN, 1024, 1024, false, false);
    double expected = 1024.0 + 1024.0 + 1024.0 * 10.0 + 1024.0 * 10.0;  // log2(1024)=10
    EXPECT_NEAR(cost, expected, 0.01);
}

TEST(AdaptiveJoinCostModelTest, NestedLoopCostIsQuadratic) {
    double cost = estimateJoinCost(JoinAlgorithm::NESTED_LOOP_JOIN, 100, 200);
    EXPECT_DOUBLE_EQ(cost, 20000.0);
}

TEST(AdaptiveJoinCostModelTest, IndexNestedLoopCostIsNLogR) {
    // left × log2(right)
    double cost = estimateJoinCost(JoinAlgorithm::INDEX_NESTED_LOOP, 1000, 1024);
    EXPECT_NEAR(cost, 1000.0 * std::log2(1024.0), 0.01);
}

TEST(AdaptiveJoinCostModelTest, GraceHashCostIsThreeTimesLinear) {
    double cost = estimateJoinCost(JoinAlgorithm::GRACE_HASH_JOIN, 1000, 2000);
    EXPECT_DOUBLE_EQ(cost, 3.0 * (1000.0 + 2000.0));
}

// ============================================================================
// AC-1: Hash Join – default for large equi-joins
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, AC1_SelectAlgo_LargeInputs_HashJoin) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    // left=50 000, right=100 000 → no special condition → HASH_JOIN
    JoinAlgorithm algo = exec.selectAlgorithm(
        50'000, 100'000,
        /*left_sorted*/false, /*right_sorted*/false,
        /*has_index*/false,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::HASH_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, AC1_ExecuteJoin_LargeInputs_HashJoinUsed) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 5000);
    Table right = makeTable("R", 5000);
    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::HASH_JOIN);
    // Each id 0..4999 matches exactly once
    EXPECT_EQ(result.rowCount(), 5000u);
}

TEST(AdaptiveJoinStrategiesTest, AC1_HashJoin_CorrectRows) {
    AdaptiveJoinExecutor exec;

    // Force hash join: use 2000 rows so nested-loop threshold (1000) is not triggered.
    // Ids 0..1999 on both sides → 2000 exact matches.
    Table big_left  = makeTable("L", 2000);
    Table big_right = makeTable("R", 2000);
    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, big_left, big_right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::HASH_JOIN);
    EXPECT_EQ(result.rowCount(), 2000u);
}

// ============================================================================
// AC-2: Merge Join – both inputs sorted on join key
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, AC2_SelectAlgo_BothSorted_MergeJoin) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    JoinAlgorithm algo = exec.selectAlgorithm(
        5000, 5000,
        /*left_sorted*/true, /*right_sorted*/true,
        /*has_index*/false,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::MERGE_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, AC2_SelectAlgo_OnlyLeftSorted_NotMergeJoin) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    JoinAlgorithm algo = exec.selectAlgorithm(
        5000, 5000,
        /*left_sorted*/true, /*right_sorted*/false,
        /*has_index*/false,
        stats);

    EXPECT_NE(algo, JoinAlgorithm::MERGE_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, AC2_ExecuteJoin_BothSorted_MergeJoinUsed) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 3000, /*sorted*/true);
    Table right = makeTable("R", 3000, /*sorted*/true);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::MERGE_JOIN);
    EXPECT_EQ(result.rowCount(), 3000u);  // id 0..2999 each match once
}

TEST(AdaptiveJoinStrategiesTest, AC2_MergeJoin_CorrectResultForDisjointKeys) {
    // Use a config with nested_loop_threshold=0 so merge join is reachable
    // even for small tables (both sides are sorted).
    AdaptiveJoinConfig cfg;
    cfg.nested_loop_threshold = 0;
    AdaptiveJoinExecutor exec(cfg);

    Table left;
    left.is_sorted = true;
    left.rows = {
        {{"id", "1"}, {"val", "a"}},
        {{"id", "3"}, {"val", "b"}},
    };

    Table right;
    right.is_sorted = true;
    right.rows = {
        {{"id", "2"}, {"val", "x"}},
        {{"id", "4"}, {"val", "y"}},
    };

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::MERGE_JOIN);
    // Keys {1,3} and {2,4} are disjoint → no matches
    EXPECT_EQ(result.rowCount(), 0u);
}

TEST(AdaptiveJoinStrategiesTest, AC2_MergeJoin_IgnoresRowsWithMissingJoinKey) {
    AdaptiveJoinConfig cfg;
    cfg.nested_loop_threshold = 0;
    AdaptiveJoinExecutor exec(cfg);

    Table left;
    left.is_sorted = true;
    left.rows = {
        {{"other", "left-only"}},
        {{"id", "2"}, {"val", "left-match"}},
    };

    Table right;
    right.is_sorted = true;
    right.rows = {
        {{"other", "right-only"}},
        {{"id", "2"}, {"val", "right-match"}},
    };

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::MERGE_JOIN);
    EXPECT_EQ(result.rowCount(), 1u);
}

// ============================================================================
// AC-3: Nested Loop – left side < 1,000 rows
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, AC3_SelectAlgo_SmallLeft_NestedLoop) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    JoinAlgorithm algo = exec.selectAlgorithm(
        500, 100'000,
        false, false, false,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::NESTED_LOOP_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, AC3_SelectAlgo_ExactThreshold_NotNestedLoop) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    // left_rows == nested_loop_threshold (1000) → NOT nested loop (boundary)
    JoinAlgorithm algo = exec.selectAlgorithm(
        1000, 10'000,
        false, false, false,
        stats);

    EXPECT_NE(algo, JoinAlgorithm::NESTED_LOOP_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, AC3_ExecuteJoin_SmallLeft_NestedLoopUsed) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 10);   // < 1000
    Table right = makeTable("R", 5000);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::NESTED_LOOP_JOIN);
    // ids 0..9 all exist in the right table
    EXPECT_EQ(result.rowCount(), 10u);
}

TEST(AdaptiveJoinStrategiesTest, AC3_NestedLoop_CustomThreshold) {
    AdaptiveJoinConfig cfg;
    cfg.nested_loop_threshold = 50;
    AdaptiveJoinExecutor exec(cfg);

    RuntimeStats stats = defaultStats();

    // 49 rows < 50 threshold → NESTED_LOOP
    EXPECT_EQ(exec.selectAlgorithm(49, 1000, false, false, false, stats),
              JoinAlgorithm::NESTED_LOOP_JOIN);

    // 50 rows == threshold → NOT nested loop
    EXPECT_NE(exec.selectAlgorithm(50, 1000, false, false, false, stats),
              JoinAlgorithm::NESTED_LOOP_JOIN);
}

// ============================================================================
// AC-4: Index Nested Loop – right has index AND left < 10,000 rows
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, AC4_SelectAlgo_IndexAndSmallLeft_IndexNestedLoop) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    JoinAlgorithm algo = exec.selectAlgorithm(
        5000, 100'000,
        false, false,
        /*has_index*/true,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::INDEX_NESTED_LOOP);
}

TEST(AdaptiveJoinStrategiesTest, AC4_SelectAlgo_NoIndex_NotIndexNestedLoop) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    JoinAlgorithm algo = exec.selectAlgorithm(
        5000, 100'000,
        false, false,
        /*has_index*/false,
        stats);

    EXPECT_NE(algo, JoinAlgorithm::INDEX_NESTED_LOOP);
}

TEST(AdaptiveJoinStrategiesTest, AC4_SelectAlgo_IndexButLargeLeft_NotIndexNestedLoop) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();

    // left >= index_nested_loop_threshold (10,000)
    JoinAlgorithm algo = exec.selectAlgorithm(
        10'000, 100'000,
        false, false,
        /*has_index*/true,
        stats);

    EXPECT_NE(algo, JoinAlgorithm::INDEX_NESTED_LOOP);
}

TEST(AdaptiveJoinStrategiesTest, AC4_ExecuteJoin_IndexAndSmallLeft_IndexNestedLoopUsed) {
    Table left  = makeTable("L", 500);                           // < 1000 → would be NL
    Table right = makeTable("R", 50'000, false, /*has_index*/true);

    // Force past nested-loop threshold by raising it
    AdaptiveJoinConfig cfg;
    cfg.nested_loop_threshold       = 100;    // left=500 > 100  → skip NL
    cfg.index_nested_loop_threshold = 10'000; // left=500 < 10000 → INDEX_NL
    AdaptiveJoinExecutor exec2(cfg);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec2.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::INDEX_NESTED_LOOP);
    EXPECT_EQ(result.rowCount(), 500u);
}

// ============================================================================
// AC-5: Grace Hash Join – memory budget exceeded
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, AC5_SelectAlgo_MemoryExceeded_GraceHash) {
    AdaptiveJoinExecutor exec;

    RuntimeStats stats = defaultStats();
    stats.memory_budget_bytes = 1024;  // Only 1 KiB budget
    stats.bytes_per_row       = 100;
    stats.grace_hash_threshold = 0.9;

    // smaller side = 10 rows × 100 bytes = 1000 bytes > 0.9 × 1024 = 921
    JoinAlgorithm algo = exec.selectAlgorithm(
        10'000, 10,          // smaller = 10
        false, false, false,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::GRACE_HASH_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, AC5_ExecuteJoin_MemoryExceeded_GraceHashUsed) {
    AdaptiveJoinExecutor exec;

    // Force left > NL threshold (1000) and right > INL threshold
    Table left  = makeTable("L", 2000);
    Table right = makeTable("R", 2000);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();
    // Set a very small budget: 2000 rows × 256 bytes = 512 KiB;
    // budget = 1 byte → grace hash triggered
    stats.memory_budget_bytes = 1;
    stats.bytes_per_row       = 256;

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::GRACE_HASH_JOIN);
    EXPECT_EQ(result.rowCount(), 2000u);
}

TEST(AdaptiveJoinStrategiesTest, AC5_GraceHashJoin_CorrectResults) {
    AdaptiveJoinExecutor exec;

    // Use rows > nested_loop_threshold (1000) so NESTED_LOOP is not chosen
    // and the memory constraint triggers GRACE_HASH_JOIN instead.
    Table left  = makeTable("L", 2000);
    Table right = makeTable("R", 2000);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();
    stats.memory_budget_bytes = 1;  // force grace hash

    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::GRACE_HASH_JOIN);
    EXPECT_EQ(result.rowCount(), 2000u);
}

TEST(AdaptiveJoinStrategiesTest, AC5_SelectAlgo_OverflowSafeMemoryEstimate_GraceHash) {
    AdaptiveJoinExecutor exec;

    RuntimeStats stats = defaultStats();
    stats.memory_budget_bytes = 4096;
    stats.bytes_per_row = 2;
    stats.grace_hash_threshold = 0.9;

    const size_t left_rows = std::numeric_limits<size_t>::max();
    const size_t right_rows = (std::numeric_limits<size_t>::max() / 2) + 1;

    JoinAlgorithm algo = exec.selectAlgorithm(left_rows,
                                              right_rows,
                                              false,
                                              false,
                                              false,
                                              stats);

    EXPECT_EQ(algo, JoinAlgorithm::GRACE_HASH_JOIN);
}

// ============================================================================
// Distributed mode: Broadcast and Shuffle Join
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, Distributed_SmallRight_BroadcastJoin) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();
    stats.is_distributed = true;

    // smaller side = 100 <= broadcast_threshold (10,000)
    JoinAlgorithm algo = exec.selectAlgorithm(
        100'000, 100,
        false, false, false,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::BROADCAST_JOIN);
}

TEST(AdaptiveJoinStrategiesTest, Distributed_BothLarge_ShuffleJoin) {
    AdaptiveJoinExecutor exec;
    RuntimeStats stats = defaultStats();
    stats.is_distributed = true;

    // Both sides > broadcast_threshold
    JoinAlgorithm algo = exec.selectAlgorithm(
        1'000'000, 500'000,
        false, false, false,
        stats);

    EXPECT_EQ(algo, JoinAlgorithm::SHUFFLE_JOIN);
}

// ============================================================================
// Filter predicate forwarding
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, Filter_ExcludesNonMatchingPairs_HashJoin) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 2000);  // ids 0..1999
    Table right = makeTable("R", 2000);  // ids 0..1999

    JoinSpec spec = makeSpec();
    // Only keep rows where id is even
    spec.filter = [](const RowValue& l, const RowValue&) {
        int id = std::stoi(l.at("id"));
        return id % 2 == 0;
    };

    RuntimeStats stats = defaultStats();
    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::HASH_JOIN);
    EXPECT_EQ(result.rowCount(), 1000u);  // 1000 even ids
}

TEST(AdaptiveJoinStrategiesTest, Filter_WorksWithNestedLoopJoin) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 10);   // < 1000
    Table right = makeTable("R", 10);

    JoinSpec spec = makeSpec();
    spec.filter = [](const RowValue& l, const RowValue&) {
        return std::stoi(l.at("id")) < 5;
    };

    RuntimeStats stats = defaultStats();
    JoinResult result = exec.executeJoin(spec, left, right, stats);

    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::NESTED_LOOP_JOIN);
    EXPECT_EQ(result.rowCount(), 5u);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, EdgeCase_EmptyLeftTable) {
    AdaptiveJoinExecutor exec;

    Table left;   // empty
    Table right = makeTable("R", 100);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);
    EXPECT_EQ(result.rowCount(), 0u);
}

TEST(AdaptiveJoinStrategiesTest, EdgeCase_EmptyRightTable) {
    AdaptiveJoinExecutor exec;

    Table left = makeTable("L", 100);
    Table right;  // empty

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);
    EXPECT_EQ(result.rowCount(), 0u);
}

TEST(AdaptiveJoinStrategiesTest, EdgeCase_NoMatchingKeys) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 2000);       // ids 0..1999
    Table right = makeTableSameKey("R", 2000, "99999"); // all keys = 99999

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);
    EXPECT_EQ(result.rowCount(), 0u);
}

TEST(AdaptiveJoinStrategiesTest, EdgeCase_ThrowsOnEmptyLeftKey) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 10);
    Table right = makeTable("R", 10);

    JoinSpec spec;
    spec.left_key  = "";
    spec.right_key = "id";

    RuntimeStats stats = defaultStats();
    EXPECT_THROW({
        auto join_result = exec.executeJoin(spec, left, right, stats);
        static_cast<void>(join_result);
    }, std::invalid_argument);
}

TEST(AdaptiveJoinStrategiesTest, EdgeCase_ThrowsOnEmptyRightKey) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 10);
    Table right = makeTable("R", 10);

    JoinSpec spec;
    spec.left_key  = "id";
    spec.right_key = "";

    RuntimeStats stats = defaultStats();
    EXPECT_THROW({
        auto join_result = exec.executeJoin(spec, left, right, stats);
        static_cast<void>(join_result);
    }, std::invalid_argument);
}

TEST(AdaptiveJoinStrategiesTest, EdgeCase_MultiRowCrossProductInBucket) {
    // Both sides have multiple rows with the same key → cross product.
    // Use a custom config that raises nested_loop_threshold so hash join is
    // still selected for 50-row tables, keeping the test fast.
    AdaptiveJoinConfig cfg;
    cfg.nested_loop_threshold = 0;  // force through to hash join path
    AdaptiveJoinExecutor exec(cfg);

    Table left  = makeTable("L", 50);
    Table right = makeTable("R", 50);
    // Override all rows to share the same key → full cross product
    for (auto& r : left.rows) {
      r["id"] = "42";
    }
    for (auto& r : right.rows) {
      r["id"] = "42";
    }

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);
    EXPECT_EQ(result.algorithm_used, JoinAlgorithm::HASH_JOIN);
    // 50 × 50 cross product on the same key
    EXPECT_EQ(result.rowCount(), 50u * 50u);
}

// ============================================================================
// Config round-trip
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, Config_GetSetRoundTrip) {
    AdaptiveJoinConfig cfg;
    cfg.nested_loop_threshold       = 500;
    cfg.index_nested_loop_threshold = 5000;
    cfg.broadcast_threshold         = 2000;

    AdaptiveJoinExecutor exec(cfg);
    EXPECT_EQ(exec.config().nested_loop_threshold,       500u);
    EXPECT_EQ(exec.config().index_nested_loop_threshold, 5000u);
    EXPECT_EQ(exec.config().broadcast_threshold,         2000u);

    AdaptiveJoinConfig cfg2;
    cfg2.nested_loop_threshold = 9999;
    exec.setConfig(cfg2);
    EXPECT_EQ(exec.config().nested_loop_threshold, 9999u);
}

// ============================================================================
// Cost model: estimated_cost is populated in JoinResult
// ============================================================================

TEST(AdaptiveJoinStrategiesTest, JoinResult_HasPositiveEstimatedCost) {
    AdaptiveJoinExecutor exec;

    Table left  = makeTable("L", 2000);
    Table right = makeTable("R", 2000);

    JoinSpec spec = makeSpec();
    RuntimeStats stats = defaultStats();

    JoinResult result = exec.executeJoin(spec, left, right, stats);
    EXPECT_GT(result.estimated_cost, 0.0);
}
