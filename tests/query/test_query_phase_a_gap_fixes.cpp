/**
 * @file test_phase_a_gap_fixes.cpp
 * @brief Phase A gap-fix regression tests.
 *
 * Covers three CRITICAL gaps fixed for Hybrid Retrieval Rollout Phase A:
 *
 * TAOPT-01 … TAOPT-05  TensorAwareQueryOptimizer::estimateTTCost —
 *                       multiplication-overflow guard (MODULE_GAPS.md lines 113/118/123)
 * QCANCEL-01 … QCANCEL-03  QueryCanceller timed-mutex timeout resilience
 *                            (blocking_no_timeout gap)
 * REWRT-01 … REWRT-04   PredicatePushdownRule::apply — safe index-based
 *                         child iteration (iterator_invalidation gap)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include "query/tensor_aware_query_optimizer.h"
#include "query/query_canceller.h"
#include "query/query_rewrite_rule.h"

#include <cmath>
#include <limits>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::query;

// ============================================================================
// TensorAwareQueryOptimizer::estimateTTCost — overflow guard tests
// ============================================================================

class TensorTTCostTest : public ::testing::Test {};

/// TAOPT-01: Normal inputs must produce a finite positive cost.
TEST_F(TensorTTCostTest, NormalInputsAreFiniteAndPositive) {
    const double cost = TensorAwareQueryOptimizer::estimateTTCost(
        "TENSOR_SIMILARITY", 4, 16, 8);
    EXPECT_TRUE(std::isfinite(cost));
    EXPECT_GT(cost, 0.0);
}

/// TAOPT-02: All known function names must produce finite results for
///           extreme-but-representable size_t inputs.
TEST_F(TensorTTCostTest, ExtremeInputsRemainFinite) {
    static const std::vector<std::string> kFunctions = {
        "TENSOR_SIMILARITY", "TENSOR_NORM", "TENSOR_CONTRACT",
        "TENSOR_SLICE",      "TENSOR_PROJECT",
        "TENSOR_COMPRESS",   "TENSOR_DECOMPOSE",
        "TENSOR_INFO",
        "UNKNOWN_FUNCTION",
    };
    constexpr std::size_t kBig = std::numeric_limits<std::size_t>::max();
    for (const auto& fn : kFunctions) {
        const double cost =
            TensorAwareQueryOptimizer::estimateTTCost(fn, kBig, kBig, kBig);
        EXPECT_TRUE(std::isfinite(cost))
            << "non-finite result for function: " << fn;
        EXPECT_GT(cost, 0.0) << "non-positive result for function: " << fn;
    }
}

/// TAOPT-03: Zero inputs fall back to default dimension values (no div-by-zero).
TEST_F(TensorTTCostTest, ZeroInputsUseDefaults) {
    const double cost =
        TensorAwareQueryOptimizer::estimateTTCost("TENSOR_SIMILARITY", 0, 0, 0);
    EXPECT_TRUE(std::isfinite(cost));
    EXPECT_GT(cost, 0.0);
}

/// TAOPT-04: O(d·n·r³) path (TENSOR_SIMILARITY) is more expensive than
///           O(d·n·r²) path (TENSOR_SLICE) for r > 1.
TEST_F(TensorTTCostTest, SimilarityCostExceedsSliceCost) {
    const double sim   = TensorAwareQueryOptimizer::estimateTTCost("TENSOR_SIMILARITY", 4, 16, 8);
    const double slice = TensorAwareQueryOptimizer::estimateTTCost("TENSOR_SLICE", 4, 16, 8);
    EXPECT_GT(sim, slice);
}

/// TAOPT-05: TENSOR_INFO (O(d·n)) produces the smallest cost for equal inputs.
TEST_F(TensorTTCostTest, InfoCostIsSmallest) {
    constexpr std::size_t d = 4, n = 16, r = 8;
    const double info      = TensorAwareQueryOptimizer::estimateTTCost("TENSOR_INFO", d, n, r);
    const double generic   = TensorAwareQueryOptimizer::estimateTTCost("UNKNOWN",     d, n, r);
    const double similarity= TensorAwareQueryOptimizer::estimateTTCost("TENSOR_SIMILARITY", d, n, r);
    EXPECT_LT(info, generic);
    EXPECT_LT(info, similarity);
}

// ============================================================================
// QueryCanceller — timed-mutex timeout resilience tests
// ============================================================================

class QueryCancellerTimedMutexTest : public ::testing::Test {
protected:
    QueryCanceller canceller_;
};

/// QCANCEL-01: register + cancel + unregister round-trip succeeds.
TEST_F(QueryCancellerTimedMutexTest, BasicRoundTrip) {
    auto token = canceller_.registerQuery("rc-1");
    ASSERT_NE(token, nullptr);
    EXPECT_FALSE(token->isCancelled());

    EXPECT_TRUE(canceller_.cancel("rc-1"));
    EXPECT_TRUE(token->isCancelled());

    canceller_.unregisterQuery("rc-1");
    EXPECT_FALSE(canceller_.cancel("rc-1")); // already unregistered
}

/// QCANCEL-02: cancel() returns false for an unknown request ID.
TEST_F(QueryCancellerTimedMutexTest, CancelUnknownIdReturnsFalse) {
    EXPECT_FALSE(canceller_.cancel("nonexistent-req"));
}

/// QCANCEL-03: Multiple concurrent registrations and cancellations do not
///             deadlock and complete within a generous timeout.
TEST_F(QueryCancellerTimedMutexTest, ConcurrentAccessDoesNotDeadlock) {
    constexpr int kWorkers = 8;
    std::vector<std::thread> threads;
    threads.reserve(kWorkers);

    for (int i = 0; i < kWorkers; ++i) {
        threads.emplace_back([this, i] {
            const std::string id = "rc-conc-" + std::to_string(i);
            auto token = canceller_.registerQuery(id);
            canceller_.cancel(id);
            canceller_.unregisterQuery(id);
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    // Reaching here without deadlock means the test passed.
    SUCCEED();
}

// ============================================================================
// PredicatePushdownRule — safe index-based child iteration tests
// ============================================================================

class PredicatePushdownTest : public ::testing::Test {};

/// REWRT-01: applies() returns false for a plan without JOIN nodes.
TEST_F(PredicatePushdownTest, AppliesReturnsFalseForNonJoinPlan) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {{"type", "scan"}, {"table", "orders"}};
    RewriteContext ctx;
    EXPECT_FALSE(rule.applies(plan, ctx));
}

/// REWRT-02: applies() returns true when a JOIN has both FILTER and SCAN children.
TEST_F(PredicatePushdownTest, AppliesReturnsTrueForJoinWithFilterAndScan) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {
        {"type", "join"},
        {"children", nlohmann::json::array({
            {{"type", "filter"}, {"condition", "x > 1"}},
            {{"type", "scan"},   {"table", "orders"}},
        })}
    };
    RewriteContext ctx;
    EXPECT_TRUE(rule.applies(plan, ctx));
}

/// REWRT-03: apply() pushes FILTER into SCAN children and returns change count 1.
TEST_F(PredicatePushdownTest, ApplyPushesFilterIntoScan) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {
        {"type", "join"},
        {"children", nlohmann::json::array({
            {{"type", "filter"}, {"condition", "status = 'active'"}},
            {{"type", "scan"},   {"table", "users"}},
        })}
    };
    RewriteContext ctx;
    const std::size_t changes = rule.apply(plan, ctx);
    EXPECT_EQ(changes, 1u);

    // After push: JOIN children should only contain the SCAN.
    const auto& children = plan["children"];
    ASSERT_EQ(children.size(), 1u);
    EXPECT_EQ(children[0]["type"].get<std::string>(), "scan");

    // SCAN should now carry the pushed filter as a child.
    ASSERT_TRUE(children[0].contains("children"));
    ASSERT_EQ(children[0]["children"].size(), 1u);
    EXPECT_EQ(children[0]["children"][0]["type"].get<std::string>(), "filter");
}

/// REWRT-04: Multiple FILTER nodes are all pushed; non-SCAN, non-FILTER children
///           are left in place.
TEST_F(PredicatePushdownTest, MultipleFiltersAllPushed) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {
        {"type", "join"},
        {"children", nlohmann::json::array({
            {{"type", "filter"}, {"condition", "a = 1"}},
            {{"type", "filter"}, {"condition", "b = 2"}},
            {{"type", "scan"},   {"table", "events"}},
            {{"type", "limit"},  {"count", 100}},
        })}
    };
    RewriteContext ctx;
    const std::size_t changes = rule.apply(plan, ctx);
    EXPECT_EQ(changes, 2u);

    // Two non-filter children remain at top level: SCAN + LIMIT.
    const auto& children = plan["children"];
    EXPECT_EQ(children.size(), 2u);

    // Both filters are inside the SCAN.
    const auto scan_it = std::find_if(children.begin(), children.end(),
        [](const nlohmann::json& n) {
            return n.contains("type") && n["type"] == "scan";
        });
    ASSERT_NE(scan_it, children.end());
    EXPECT_EQ((*scan_it)["children"].size(), 2u);
}
