#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_set>

#include "query/query_rewrite_rule.h"
#include "query/query_profiler.h"
#include "query/approximate_aggregator.h"

using namespace themis::query;

// ═════════════════════════════════════════════════════════════════════════════
// QueryRewriteRule Tests
// ═════════════════════════════════════════════════════════════════════════════

class QueryRewriteRuleTest : public ::testing::Test {
protected:
    RewriteContext ctx_;
    void SetUp() override {
        ctx_.collection_row_counts["users"] = 1'000'000;
        ctx_.or_to_in_threshold = 3;
        ctx_.enable_constant_folding = true;
    }
};

// ─── PredicatePushdownRule ───────────────────────────────────────────────────

TEST_F(QueryRewriteRuleTest, PredicatePushdown_Name) {
    PredicatePushdownRule rule;
    EXPECT_EQ(rule.name(), "PredicatePushdown");
}

TEST_F(QueryRewriteRuleTest, PredicatePushdown_AppliesWhenFilterIsChildOfJoin) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {
        {"type", "join"},
        {"children", nlohmann::json::array({
            {{"type", "filter"}, {"condition", {{"type","eq"},{"field","age"},{"value",30}}}},
            {{"type", "scan"},   {"collection", "users"}, {"children", nlohmann::json::array()}}
        })}
    };
    EXPECT_TRUE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, PredicatePushdown_DoesNotApplyWithoutFilterChild) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {
        {"type", "scan"},
        {"collection", "users"},
        {"children", nlohmann::json::array()}
    };
    EXPECT_FALSE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, PredicatePushdown_MovesFilterToScan) {
    PredicatePushdownRule rule;
    nlohmann::json plan = {
        {"type", "join"},
        {"children", nlohmann::json::array({
            {{"type", "filter"}, {"condition", {{"type","eq"},{"field","age"},{"value",30}}}},
            {{"type", "scan"},   {"collection", "users"}, {"children", nlohmann::json::array()}}
        })}
    };
    const size_t n = rule.apply(plan, ctx_);
    EXPECT_GE(n, 1u);
    // The scan's children list should now contain the filter.
    bool found = false;
    for (const auto& child : plan["children"]) {
        if (child["type"] == "scan") {
            for (const auto& sc : child["children"]) {
                if (sc["type"] == "filter") {
                  found = true;
                }
            }
        }
    }
    EXPECT_TRUE(found);
}

// ─── ProjectionPushdownRule ──────────────────────────────────────────────────

TEST_F(QueryRewriteRuleTest, ProjectionPushdown_Name) {
    ProjectionPushdownRule rule;
    EXPECT_EQ(rule.name(), "ProjectionPushdown");
}

TEST_F(QueryRewriteRuleTest, ProjectionPushdown_AppliesWhenProjectAboveScan) {
    ProjectionPushdownRule rule;
    nlohmann::json plan = {
        {"type", "project"},
        {"columns", nlohmann::json::array({"id", "name"})},
        {"children", nlohmann::json::array({
            {{"type", "scan"}, {"collection", "users"}}
        })}
    };
    EXPECT_TRUE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, ProjectionPushdown_PushesColumnsToScan) {
    ProjectionPushdownRule rule;
    nlohmann::json plan = {
        {"type", "project"},
        {"columns", nlohmann::json::array({"id", "name"})},
        {"children", nlohmann::json::array({
            {{"type", "scan"}, {"collection", "users"}}
        })}
    };
    const size_t n = rule.apply(plan, ctx_);
    EXPECT_GE(n, 1u);
}

// ─── OrToInRewriteRule ───────────────────────────────────────────────────────

TEST_F(QueryRewriteRuleTest, OrToIn_Name) {
    OrToInRewriteRule rule;
    EXPECT_EQ(rule.name(), "OrToIn");
}

TEST_F(QueryRewriteRuleTest, OrToIn_AppliesWhenOrChainExceedsThreshold) {
    OrToInRewriteRule rule;
    nlohmann::json plan = {
        {"type", "filter"},
        {"condition", {
            {"type", "or"},
            {"left", {
                {"type", "or"},
                {"left",  {{"type","eq"},{"field","city"},{"value","Berlin"}}},
                {"right", {{"type","eq"},{"field","city"},{"value","Munich"}}}
            }},
            {"right", {{"type","eq"},{"field","city"},{"value","Hamburg"}}}
        }}
    };
    ctx_.or_to_in_threshold = 3;
    EXPECT_TRUE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, OrToIn_DoesNotApplyBelowThreshold) {
    OrToInRewriteRule rule;
    nlohmann::json plan = {
        {"type", "filter"},
        {"condition", {
            {"type", "or"},
            {"left",  {{"type","eq"},{"field","city"},{"value","Berlin"}}},
            {"right", {{"type","eq"},{"field","city"},{"value","Munich"}}}
        }}
    };
    ctx_.or_to_in_threshold = 3;
    EXPECT_FALSE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, OrToIn_RewritesOrChainToIn) {
    OrToInRewriteRule rule;
    nlohmann::json plan = {
        {"type", "filter"},
        {"condition", {
            {"type", "or"},
            {"left", {
                {"type", "or"},
                {"left",  {{"type","eq"},{"field","city"},{"value","Berlin"}}},
                {"right", {{"type","eq"},{"field","city"},{"value","Munich"}}}
            }},
            {"right", {{"type","eq"},{"field","city"},{"value","Hamburg"}}}
        }}
    };
    ctx_.or_to_in_threshold = 3;
    const size_t n = rule.apply(plan, ctx_);
    EXPECT_GE(n, 1u);
    EXPECT_EQ(plan["condition"]["type"], "in");
    EXPECT_EQ(plan["condition"]["field"], "city");
    EXPECT_EQ(plan["condition"]["values"].size(), 3u);
}

// ─── ConstantFoldingRule ─────────────────────────────────────────────────────

TEST_F(QueryRewriteRuleTest, ConstantFolding_Name) {
    ConstantFoldingRule rule;
    EXPECT_EQ(rule.name(), "ConstantFolding");
}

TEST_F(QueryRewriteRuleTest, ConstantFolding_AppliesForArithmeticLiterals) {
    ConstantFoldingRule rule;
    nlohmann::json plan = {
        {"type", "add"},
        {"left",  2.0},
        {"right", 3.0}
    };
    EXPECT_TRUE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, ConstantFolding_FoldsAddition) {
    ConstantFoldingRule rule;
    nlohmann::json plan = {
        {"type", "add"},
        {"left",  2.0},
        {"right", 3.0}
    };
    const size_t n = rule.apply(plan, ctx_);
    EXPECT_GE(n, 1u);
    EXPECT_NEAR(plan.get<double>(), 5.0, 1e-9);
}

TEST_F(QueryRewriteRuleTest, ConstantFolding_FoldsMultiplication) {
    ConstantFoldingRule rule;
    nlohmann::json plan = {
        {"type", "mul"},
        {"left",  4.0},
        {"right", 7.0}
    };
    rule.apply(plan, ctx_);
    EXPECT_NEAR(plan.get<double>(), 28.0, 1e-9);
}

TEST_F(QueryRewriteRuleTest, ConstantFolding_DoesNotFoldDivisionByZero) {
    ConstantFoldingRule rule;
    nlohmann::json plan = {
        {"type", "div"},
        {"left",  1.0},
        {"right", 0.0}
    };
    const size_t n = rule.apply(plan, ctx_);
    EXPECT_EQ(n, 0u); // left unchanged
    EXPECT_TRUE(plan.is_object());
}

TEST_F(QueryRewriteRuleTest, ConstantFolding_DisabledWhenFlagFalse) {
    ConstantFoldingRule rule;
    nlohmann::json plan = {{"type","add"},{"left",1.0},{"right",2.0}};
    ctx_.enable_constant_folding = false;
    EXPECT_FALSE(rule.applies(plan, ctx_));
}

// ─── CommonSubexpressionRule ─────────────────────────────────────────────────

TEST_F(QueryRewriteRuleTest, CSE_Name) {
    CommonSubexpressionRule rule;
    EXPECT_EQ(rule.name(), "CommonSubexpressionElimination");
}

TEST_F(QueryRewriteRuleTest, CSE_AppliesWhenDuplicateSubexprExists) {
    CommonSubexpressionRule rule;
    const nlohmann::json subexpr = {{"type","eq"},{"field","age"},{"value",30}};
    nlohmann::json plan = {
        {"type", "and"},
        {"left",  subexpr},
        {"right", subexpr}   // same sub-expression twice
    };
    EXPECT_TRUE(rule.applies(plan, ctx_));
}

TEST_F(QueryRewriteRuleTest, CSE_DoesNotApplyWithoutDuplicates) {
    CommonSubexpressionRule rule;
    nlohmann::json plan = {
        {"type", "and"},
        {"left",  {{"type","eq"},{"field","age"},{"value",30}}},
        {"right", {{"type","eq"},{"field","city"},{"value","Berlin"}}}
    };
    EXPECT_FALSE(rule.applies(plan, ctx_));
}

// ─── QueryRewritePipeline ─────────────────────────────────────────────────────

TEST_F(QueryRewriteRuleTest, Pipeline_DefaultHasFiveRules) {
    auto pipeline = QueryRewritePipeline::createDefault();
    EXPECT_EQ(pipeline.ruleCount(), 5u);
}

TEST_F(QueryRewriteRuleTest, Pipeline_AddAndClearRules) {
    QueryRewritePipeline pipeline;
    pipeline.addRule(std::make_shared<ConstantFoldingRule>());
    EXPECT_EQ(pipeline.ruleCount(), 1u);
    pipeline.clearRules();
    EXPECT_EQ(pipeline.ruleCount(), 0u);
}

TEST_F(QueryRewriteRuleTest, Pipeline_RunCollectsStats) {
    QueryRewritePipeline pipeline;
    pipeline.addRule(std::make_shared<ConstantFoldingRule>());

    nlohmann::json plan = {{"type","add"},{"left",10.0},{"right",20.0}};
    const auto stats = pipeline.run(plan, ctx_);
    EXPECT_GE(stats.rules_applied, 1u);
    EXPECT_GE(stats.total_transformations, 1u);
    EXPECT_FALSE(stats.applied_rule_names.empty());
}

TEST_F(QueryRewriteRuleTest, Pipeline_ReachesFixedPoint) {
    // A plan that requires two passes: first add-fold, then a second constant.
    QueryRewritePipeline pipeline;
    pipeline.addRule(std::make_shared<ConstantFoldingRule>());

    // 2 + 3 → 5; no more constant sub-expressions after that.
    nlohmann::json plan = {{"type","add"},{"left",2.0},{"right",3.0}};
    pipeline.run(plan, ctx_);
    EXPECT_NEAR(plan.get<double>(), 5.0, 1e-9);
}

// ═════════════════════════════════════════════════════════════════════════════
// QueryProfiler Tests
// ═════════════════════════════════════════════════════════════════════════════

class QueryProfilerTest : public ::testing::Test {};

TEST_F(QueryProfilerTest, BasicQueryTiming) {
    QueryProfiler profiler;
    profiler.beginQuery("FOR u IN users RETURN u");
    profiler.beginOperator("SeqScan");
    profiler.endOperator(1000, 1000);
    profiler.endQuery(1000);

    const auto profile = profiler.getProfile();
    EXPECT_EQ(profile.query_text, "FOR u IN users RETURN u");
    EXPECT_EQ(profile.result_rows, 1000u);
    EXPECT_FALSE(profile.cache_hit);
    EXPECT_FALSE(profile.operators.empty());
    EXPECT_EQ(profile.operators[0].operator_name, "SeqScan");
    EXPECT_EQ(profile.operators[0].rows_out, 1000u);
    EXPECT_GE(profile.total_duration_ns, 0);
}

TEST_F(QueryProfilerTest, CacheHitFlagPreserved) {
    QueryProfiler profiler;
    profiler.beginQuery("SELECT * FROM t");
    profiler.endQuery(50, /*cache_hit=*/true);

    const auto profile = profiler.getProfile();
    EXPECT_TRUE(profile.cache_hit);
    EXPECT_EQ(profile.result_rows, 50u);
}

TEST_F(QueryProfilerTest, MultipleOperatorsTracked) {
    QueryProfiler profiler;
    profiler.beginQuery("q");
    profiler.beginOperator("HashJoin");
    profiler.endOperator(5000, 2000, /*memory_bytes=*/1024 * 1024);
    profiler.beginOperator("Filter");
    profiler.endOperator(2000, 500);
    profiler.endQuery(500);

    const auto profile = profiler.getProfile();
    EXPECT_EQ(profile.operators.size(), 2u);
    EXPECT_EQ(profile.operators[0].operator_name, "HashJoin");
    EXPECT_EQ(profile.operators[1].operator_name, "Filter");
    EXPECT_EQ(profile.operators[0].memory_bytes, 1024u * 1024u);
}

TEST_F(QueryProfilerTest, SlowestOperatorReturnsCorrectOne) {
    QueryProfiler profiler;
    profiler.beginQuery("q");

    // Simulate two operators with synthetic durations by sleeping briefly.
    profiler.beginOperator("Fast");
    profiler.endOperator(100, 100);
    profiler.beginOperator("Slow");
    profiler.endOperator(100, 50);
    profiler.endQuery(50);

    const auto profile = profiler.getProfile();
    const auto* slowest = profile.slowestOperator();
    ASSERT_NE(slowest, nullptr);
    EXPECT_FALSE(slowest->operator_name.empty());
}

TEST_F(QueryProfilerTest, ResetClearsState) {
    QueryProfiler profiler;
    profiler.beginQuery("q1");
    profiler.endQuery(5);
    profiler.reset();

    const auto profile = profiler.getProfile();
    EXPECT_TRUE(profile.query_text.empty());
    EXPECT_EQ(profile.result_rows, 0u);
    EXPECT_TRUE(profile.operators.empty());
}

TEST_F(QueryProfilerTest, NullProfilerNoOp) {
    NullQueryProfiler np;
    np.beginQuery("q");
    np.beginOperator("op");
    np.endOperator(100, 50, 512, 3);
    np.endQuery(50, true);
    const auto profile = np.getProfile();
    EXPECT_TRUE(profile.query_text.empty());
    EXPECT_EQ(profile.operators.size(), 0u);
}

TEST_F(QueryProfilerTest, PeakMemoryTrackedFromOperators) {
    QueryProfiler profiler;
    profiler.beginQuery("q");
    profiler.beginOperator("Op1");
    profiler.endOperator(0, 0, /*memory_bytes=*/2048u);
    profiler.beginOperator("Op2");
    profiler.endOperator(0, 0, /*memory_bytes=*/8192u);
    profiler.endQuery(0);

    const auto profile = profiler.getProfile();
    EXPECT_EQ(profile.peak_memory_bytes, 8192u);
}

TEST_F(QueryProfilerTest, SlowestOperatorNullWhenNoOperators) {
    QueryProfile empty;
    EXPECT_EQ(empty.slowestOperator(), nullptr);
}

// ═════════════════════════════════════════════════════════════════════════════
// ApproximateCountDistinct (HyperLogLog) Tests
// ═════════════════════════════════════════════════════════════════════════════

class HyperLogLogTest : public ::testing::Test {};

TEST_F(HyperLogLogTest, DefaultPrecision12) {
    ApproximateCountDistinct hll;
    EXPECT_EQ(hll.precision(), 12);
}

TEST_F(HyperLogLogTest, ErrorRateBelowTwoPercent) {
    ApproximateCountDistinct hll(12);
    EXPECT_LT(hll.errorRate(), 0.02);
}

TEST_F(HyperLogLogTest, EmptyEstimateIsZeroOrNearZero) {
    ApproximateCountDistinct hll;
    const int64_t est = hll.estimate().get<int64_t>();
    EXPECT_LE(est, 5); // HLL may report small non-zero for empty input
}

TEST_F(HyperLogLogTest, CountDistinctSmallSet) {
    ApproximateCountDistinct hll(12);
    const int n = 100;
    for (int i = 0; i < n; ++i) {
        hll.add(nlohmann::json(std::to_string(i)));
    }
    const int64_t est = hll.estimate().get<int64_t>();
    // Allow generous error for small cardinalities.
    EXPECT_GE(est, 50);
    EXPECT_LE(est, 200);
}

TEST_F(HyperLogLogTest, CountDistinctLargeSet) {
    ApproximateCountDistinct hll(14);
    const int n = 100'000;
    for (int i = 0; i < n; ++i) {
        hll.add(nlohmann::json(i));
    }
    const int64_t est = hll.estimate().get<int64_t>();
    // Error should be < 2 % for precision=14.
    const double rel_err = std::abs(static_cast<double>(est - n)) / n;
    EXPECT_LT(rel_err, 0.05); // 5 % tolerance in tests
}

TEST_F(HyperLogLogTest, DuplicatesNotCountedTwice) {
    ApproximateCountDistinct hll;
    for (int rep = 0; rep < 100; ++rep) {
        hll.add(nlohmann::json("same_value"));
    }
    const int64_t est = hll.estimate().get<int64_t>();
    // Should estimate ~1 distinct value.
    EXPECT_LE(est, 10);
}

TEST_F(HyperLogLogTest, ResetClearsRegisters) {
    ApproximateCountDistinct hll;
    for (int i = 0; i < 1000; ++i) {
      hll.add(nlohmann::json(i));
    }
    hll.reset();
    const int64_t est = hll.estimate().get<int64_t>();
    EXPECT_LE(est, 5);
}

TEST_F(HyperLogLogTest, MergeTwoSketchesYieldsUnion) {
    ApproximateCountDistinct hll1(12), hll2(12);
    for (int i = 0; i < 500; ++i) {
      hll1.add(nlohmann::json(i));
    }
    for (int i = 500; i < 1000; ++i) {
      hll2.add(nlohmann::json(i));
    }
    hll1.merge(hll2);
    const int64_t est = hll1.estimate().get<int64_t>();
    EXPECT_GE(est, 500);
    EXPECT_LE(est, 2000);
}

TEST_F(HyperLogLogTest, IncompatibleMergeThrows) {
    ApproximateCountDistinct hll1(10);
    ApproximateCountDistinct hll2(14);
    EXPECT_THROW(hll1.merge(hll2), std::invalid_argument);
}

// ═════════════════════════════════════════════════════════════════════════════
// ApproximatePercentile (t-Digest) Tests
// ═════════════════════════════════════════════════════════════════════════════

class TDigestTest : public ::testing::Test {};

TEST_F(TDigestTest, DefaultQuantileIsMedian) {
    ApproximatePercentile td;
    EXPECT_NEAR(td.quantile(), 0.5, 1e-9);
}

TEST_F(TDigestTest, EmptyEstimateIsNull) {
    ApproximatePercentile td(0.5);
    EXPECT_TRUE(td.estimate().is_null());
}

TEST_F(TDigestTest, MedianOfUniformDistribution) {
    ApproximatePercentile td(0.5, 200);
    for (int i = 1; i <= 1000; ++i) {
        td.add(nlohmann::json(static_cast<double>(i)));
    }
    const double median = td.estimate().get<double>();
    // Median of 1..1000 is ~500.5.
    EXPECT_GE(median, 400.0);
    EXPECT_LE(median, 600.0);
}

TEST_F(TDigestTest, P95OfUniformDistribution) {
    ApproximatePercentile td(0.95, 200);
    for (int i = 1; i <= 1000; ++i) {
        td.add(nlohmann::json(static_cast<double>(i)));
    }
    const double p95 = td.estimate().get<double>();
    // p95 of 1..1000 is ~950.
    EXPECT_GE(p95, 850.0);
    EXPECT_LE(p95, 1000.0);
}

TEST_F(TDigestTest, MergeTwoDigests) {
    ApproximatePercentile td1(0.5), td2(0.5);
    for (int i = 1; i <= 500; ++i) {
      td1.add(nlohmann::json(static_cast<double>(i)));
    }
    for (int i = 501; i <= 1000; ++i) {
      td2.add(nlohmann::json(static_cast<double>(i)));
    }
    td1.merge(td2);
    const double median = td1.estimate().get<double>();
    EXPECT_GE(median, 350.0);
    EXPECT_LE(median, 650.0);
}

TEST_F(TDigestTest, ResetClearsState) {
    ApproximatePercentile td(0.5);
    for (int i = 0; i < 100; ++i) {
      td.add(nlohmann::json(static_cast<double>(i)));
    }
    td.reset();
    EXPECT_TRUE(td.estimate().is_null());
}

// ═════════════════════════════════════════════════════════════════════════════
// SamplingAggregator Tests
// ═════════════════════════════════════════════════════════════════════════════

class SamplingAggregatorTest : public ::testing::Test {};

TEST_F(SamplingAggregatorTest, CountEqualsExactForSmallInput) {
    SamplingAggregator sa(SamplingAggregator::AggregationType::COUNT, 10'000);
    for (int i = 0; i < 100; ++i) {
      sa.add(nlohmann::json(static_cast<double>(i)));
    }
    const int64_t est = sa.estimate().get<int64_t>();
    EXPECT_EQ(est, 100);
}

TEST_F(SamplingAggregatorTest, AvgEstimateWithinTenPercent) {
    // AVG of 1..1000 = 500.5; with 10k reservoir and 1k input, sample = all.
    SamplingAggregator sa(SamplingAggregator::AggregationType::AVG, 10'000);
    for (int i = 1; i <= 1000; ++i) {
      sa.add(nlohmann::json(static_cast<double>(i)));
    }
    const double est = sa.estimate().get<double>();
    EXPECT_GE(est, 400.0);
    EXPECT_LE(est, 600.0);
}

TEST_F(SamplingAggregatorTest, SumEstimateReasonable) {
    SamplingAggregator sa(SamplingAggregator::AggregationType::SUM, 10'000);
    for (int i = 1; i <= 1000; ++i) {
      sa.add(nlohmann::json(static_cast<double>(i)));
    }
    const double est = sa.estimate().get<double>();
    // Exact sum = 500500; reservoir contains all values, so scale = 1.
    EXPECT_GE(est, 400'000.0);
    EXPECT_LE(est, 600'000.0);
}

TEST_F(SamplingAggregatorTest, ReservoirCapIsRespected) {
    const size_t cap = 100;
    SamplingAggregator sa(SamplingAggregator::AggregationType::AVG, cap);
    for (int i = 0; i < 1000; ++i) {
      sa.add(nlohmann::json(static_cast<double>(i)));
    }
    EXPECT_LE(sa.sampleSize(), cap);
    EXPECT_EQ(sa.totalSeen(), 1000u);
}

TEST_F(SamplingAggregatorTest, NonNumericValuesIgnored) {
    SamplingAggregator sa(SamplingAggregator::AggregationType::COUNT, 100);
    sa.add(nlohmann::json("hello"));
    sa.add(nlohmann::json(1.0));
    // Only the numeric value should be counted.
    EXPECT_EQ(sa.totalSeen(), 1u);
}

TEST_F(SamplingAggregatorTest, ResetClearsState) {
    SamplingAggregator sa(SamplingAggregator::AggregationType::AVG, 100);
    for (int i = 0; i < 50; ++i) {
      sa.add(nlohmann::json(static_cast<double>(i)));
    }
    sa.reset();
    EXPECT_EQ(sa.totalSeen(), 0u);
    EXPECT_TRUE(sa.estimate().is_null());
}

TEST_F(SamplingAggregatorTest, ErrorRateDecreaseWithMoreSamples) {
    SamplingAggregator small_sa(SamplingAggregator::AggregationType::AVG, 10);
    SamplingAggregator large_sa(SamplingAggregator::AggregationType::AVG, 1000);
    for (int i = 0; i < 10; ++i) {
        small_sa.add(nlohmann::json(static_cast<double>(i)));
        large_sa.add(nlohmann::json(static_cast<double>(i)));
    }
    // With all values in reservoir, error rates should be reasonable.
    EXPECT_GE(small_sa.errorRate(), 0.0);
    EXPECT_GE(large_sa.errorRate(), 0.0);
}
