// Tests for ParallelExecutor (Intra-Query Parallel Execution, v1.7.0)
//
// Covers:
//   - ParallelConfig defaults and boundary values
//   - parallelScan: sequential fallback, parallel path, filter correctness
//   - parallelHashJoin: inner join correctness, multi-row matches, empty inputs
//   - parallelAggregate: Count, Sum, Avg, Min, Max; GROUP BY; sequential fallback
//   - Consistency: parallel and sequential results agree

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "storage/base_entity.h"
#include "query/parallel_executor.h"

using namespace themis;
using Table   = ParallelExecutor::Table;
using JoinSpec = ParallelExecutor::JoinSpec;
using AggSpec  = ParallelExecutor::AggregateSpec;
using AggFn    = ParallelExecutor::AggregateFunction;

// ── helpers ──────────────────────────────────────────────────────────────────

namespace {

BaseEntity makeEntity(std::string pk, BaseEntity::FieldMap fields) {
    return BaseEntity::fromFields(pk, std::move(fields));
}

// Build a table of N rows.  Each entity has:
//   "id"    → string(i)
//   "value" → int64_t(i)
//   "group" → "even" / "odd"
Table buildTable(size_t n) {
    Table t;
    t.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        BaseEntity::FieldMap f;
        f["id"]    = std::to_string(i);
        f["value"] = int64_t(i);
        f["group"] = (i % 2 == 0) ? std::string("even") : std::string("odd");
        t.push_back(makeEntity("pk" + std::to_string(i), std::move(f)));
    }
    return t;
}

} // namespace

// ============================================================================
// ParallelConfig
// ============================================================================

TEST(ParallelConfigTest, DefaultValues) {
    ParallelExecutor::ParallelConfig cfg;
    EXPECT_GT(cfg.max_threads, 0u);
    EXPECT_EQ(cfg.morsel_size, 1024u);
    EXPECT_TRUE(cfg.enable_parallel_scan);
    EXPECT_TRUE(cfg.enable_parallel_join);
    EXPECT_TRUE(cfg.enable_parallel_aggregate);
}

TEST(ParallelConfigTest, ZeroMaxThreadsClampedToOne) {
    ParallelExecutor::ParallelConfig cfg;
    cfg.max_threads = 0;
    ParallelExecutor exec(cfg);
    EXPECT_GE(exec.getConfig().max_threads, 1u);
}

TEST(ParallelConfigTest, ZeroMorselSizeClampedToOne) {
    ParallelExecutor::ParallelConfig cfg;
    cfg.morsel_size = 0;
    ParallelExecutor exec(cfg);
    EXPECT_GE(exec.getConfig().morsel_size, 1u);
}

TEST(ParallelConfigTest, SetConfigClampsZeroValues) {
    ParallelExecutor exec;
    ParallelExecutor::ParallelConfig bad;
    bad.max_threads = 0;
    bad.morsel_size = 0;
    exec.setConfig(bad);
    EXPECT_GE(exec.getConfig().max_threads, 1u);
    EXPECT_GE(exec.getConfig().morsel_size, 1u);
}

// ============================================================================
// parallelScan
// ============================================================================

TEST(ParallelScanTest, EmptyInput) {
    ParallelExecutor exec;
    Table input;
    auto res = exec.parallelScan(input, [](const BaseEntity&) { return true; });
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelScanTest, NullFilterReturnsError) {
    ParallelExecutor exec;
    Table input = buildTable(10);
    auto res = exec.parallelScan(input, nullptr);
    EXPECT_FALSE(res.has_value());
}

TEST(ParallelScanTest, AcceptAllFilter) {
    ParallelExecutor exec;
    const size_t N = 50;
    Table input = buildTable(N);
    auto res = exec.parallelScan(input, [](const BaseEntity&) { return true; });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), N);
}

TEST(ParallelScanTest, RejectAllFilter) {
    ParallelExecutor exec;
    Table input = buildTable(50);
    auto res = exec.parallelScan(input, [](const BaseEntity&) { return false; });
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelScanTest, EvenRowsFilter_SmallInput) {
    // Small input – exercises sequential fallback (single morsel).
    ParallelExecutor::ParallelConfig cfg;
    cfg.morsel_size = 1024; // large morsel → sequential path for N=20
    ParallelExecutor exec(cfg);

    const size_t N = 20;
    Table input = buildTable(N);
    auto res = exec.parallelScan(input, [](const BaseEntity& e) {
        auto g = e.getFieldAsString("group");
        return g && *g == "even";
    });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), N / 2);
}

TEST(ParallelScanTest, EvenRowsFilter_LargeInput) {
    // Large input – exercises the morsel-parallel path.
    ParallelExecutor::ParallelConfig cfg;
    cfg.morsel_size = 32; // small morsel to force parallelism on medium input
    cfg.max_threads = 4;
    ParallelExecutor exec(cfg);

    const size_t N = 512;
    Table input = buildTable(N);
    auto res = exec.parallelScan(input, [](const BaseEntity& e) {
        auto g = e.getFieldAsString("group");
        return g && *g == "even";
    });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), N / 2);
}

TEST(ParallelExecutorScanTest, ParallelAndSequentialResultsAgree) {
    // Both paths must produce the same set of entities.
    const size_t N = 300;
    Table input = buildTable(N);

    ParallelExecutor::ParallelConfig seq_cfg;
    seq_cfg.enable_parallel_scan = false;
    ParallelExecutor seq_exec(seq_cfg);

    ParallelExecutor::ParallelConfig par_cfg;
    par_cfg.morsel_size = 16;
    par_cfg.max_threads = 4;
    ParallelExecutor par_exec(par_cfg);

    auto filter = [](const BaseEntity& e) {
        auto v = e.getFieldAsInt("value");
        return v && *v % 3 == 0;
    };

    auto seq_res = seq_exec.parallelScan(input, filter);
    auto par_res = par_exec.parallelScan(input, filter);
    ASSERT_TRUE(seq_res.has_value());
    ASSERT_TRUE(par_res.has_value());

    // Sort both by PK and compare.
    auto sortByPk = [](std::vector<BaseEntity>& v) {
        std::sort(v.begin(), v.end(), [](const BaseEntity& a, const BaseEntity& b) {
            return a.getPrimaryKey() < b.getPrimaryKey();
        });
    };
    sortByPk(*seq_res);
    sortByPk(*par_res);

    ASSERT_EQ(seq_res->size(), par_res->size());
    for (size_t i = 0; i < seq_res->size(); ++i) {
        EXPECT_EQ((*seq_res)[i].getPrimaryKey(), (*par_res)[i].getPrimaryKey());
    }
}

TEST(ParallelScanTest, DisabledParallelScanFallsBackToSequential) {
    ParallelExecutor::ParallelConfig cfg;
    cfg.enable_parallel_scan = false;
    cfg.morsel_size = 1; // would trigger parallel otherwise
    ParallelExecutor exec(cfg);

    Table input = buildTable(100);
    auto res = exec.parallelScan(input, [](const BaseEntity&) { return true; });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), 100u);
}

// ============================================================================
// parallelHashJoin
// ============================================================================

TEST(ParallelHashJoinTest, EmptyInputsProduceEmptyResult) {
    ParallelExecutor exec;
    Table left, right;
    JoinSpec spec{"id", "id"};
    auto res = exec.parallelHashJoin(left, right, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelHashJoinTest, EmptyLeftProducesEmptyResult) {
    ParallelExecutor exec;
    Table left;
    Table right = buildTable(10);
    JoinSpec spec{"id", "id"};
    auto res = exec.parallelHashJoin(left, right, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelHashJoinTest, EmptyRightProducesEmptyResult) {
    ParallelExecutor exec;
    Table left = buildTable(10);
    Table right;
    JoinSpec spec{"id", "id"};
    auto res = exec.parallelHashJoin(left, right, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelHashJoinTest, EmptyJoinKeyReturnsError) {
    ParallelExecutor exec;
    Table left = buildTable(5);
    Table right = buildTable(5);
    JoinSpec bad_spec{"", "id"};
    auto res = exec.parallelHashJoin(left, right, bad_spec);
    EXPECT_FALSE(res.has_value());
}

TEST(ParallelHashJoinTest, SelfJoinOnId) {
    // Join table with itself on "id" → each row matches exactly one row.
    ParallelExecutor::ParallelConfig cfg;
    cfg.morsel_size = 8;
    cfg.max_threads = 4;
    ParallelExecutor exec(cfg);

    const size_t N = 40;
    Table tbl = buildTable(N);
    JoinSpec spec{"id", "id"};
    auto res = exec.parallelHashJoin(tbl, tbl, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), N); // each row joins with exactly itself
}

TEST(ParallelHashJoinTest, CrossProductExcludedByMismatch) {
    // Left "id" is in [0,9]; right "id" is in [10,19] → no match.
    ParallelExecutor exec;

    Table left = buildTable(10); // ids 0..9
    Table right;
    for (size_t i = 10; i < 20; ++i) {
        BaseEntity::FieldMap f;
        f["id"] = std::to_string(i);
        right.push_back(makeEntity("r" + std::to_string(i), std::move(f)));
    }
    JoinSpec spec{"id", "id"};
    auto res = exec.parallelHashJoin(left, right, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelHashJoinTest, MultiRowMatchOnBuildSide) {
    // Two right-side rows share the same key → both appear in the result.
    ParallelExecutor exec;

    Table left;
    {
        BaseEntity::FieldMap f;
        f["dept_id"] = std::string("eng");
        left.push_back(makeEntity("emp1", std::move(f)));
    }

    Table right;
    for (int i = 0; i < 3; ++i) {
        BaseEntity::FieldMap f;
        f["id"]   = std::string("eng");
        f["name"] = std::string("room") + std::to_string(i);
        right.push_back(makeEntity("room" + std::to_string(i), std::move(f)));
    }

    JoinSpec spec{"dept_id", "id"};
    auto res = exec.parallelHashJoin(left, right, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), 3u);
}

TEST(ParallelHashJoinTest, ParallelAndSequentialResultsAgree) {
    const size_t N = 200;
    Table tbl = buildTable(N);

    ParallelExecutor::ParallelConfig seq_cfg;
    seq_cfg.enable_parallel_join = false;
    ParallelExecutor seq_exec(seq_cfg);

    ParallelExecutor::ParallelConfig par_cfg;
    par_cfg.max_threads = 4;
    ParallelExecutor par_exec(par_cfg);

    JoinSpec spec{"id", "id"};
    auto seq_res = seq_exec.parallelHashJoin(tbl, tbl, spec);
    auto par_res = par_exec.parallelHashJoin(tbl, tbl, spec);
    ASSERT_TRUE(seq_res.has_value());
    ASSERT_TRUE(par_res.has_value());
    EXPECT_EQ(seq_res->size(), par_res->size());
}

// ============================================================================
// parallelAggregate
// ============================================================================

TEST(ParallelAggregateTest, CountAll) {
    ParallelExecutor exec;
    const size_t N = 100;
    Table input = buildTable(N);
    AggSpec spec{"value", AggFn::Count, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 1u);  // no GROUP BY → single entry
    EXPECT_DOUBLE_EQ(res->at(""), static_cast<double>(N));
}

TEST(ParallelAggregateTest, SumValues) {
    ParallelExecutor exec;
    const size_t N = 10;
    Table input = buildTable(N); // values 0..9
    AggSpec spec{"value", AggFn::Sum, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 1u);
    // 0+1+...+9 = 45
    EXPECT_DOUBLE_EQ(res->at(""), 45.0);
}

TEST(ParallelAggregateTest, AvgValues) {
    ParallelExecutor exec;
    const size_t N = 11;
    Table input = buildTable(N); // values 0..10, avg = 5.0
    AggSpec spec{"value", AggFn::Avg, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res->at(""), 5.0);
}

TEST(ParallelAggregateTest, MinValues) {
    ParallelExecutor exec;
    const size_t N = 50;
    Table input = buildTable(N);
    AggSpec spec{"value", AggFn::Min, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res->at(""), 0.0);
}

TEST(ParallelAggregateTest, MaxValues) {
    ParallelExecutor exec;
    const size_t N = 50;
    Table input = buildTable(N);
    AggSpec spec{"value", AggFn::Max, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res->at(""), static_cast<double>(N - 1));
}

TEST(ParallelAggregateTest, CountByGroup) {
    ParallelExecutor exec;
    const size_t N = 100; // 50 even + 50 odd
    Table input = buildTable(N);
    AggSpec spec{"value", AggFn::Count, {"group"}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2u);
    // Group keys use length-prefix encoding: "4:even" and "3:odd".
    EXPECT_DOUBLE_EQ(res->at("4:even"), 50.0);
    EXPECT_DOUBLE_EQ(res->at("3:odd"),  50.0);
}

TEST(ParallelAggregateTest, SumByGroup) {
    ParallelExecutor exec;
    const size_t N = 6; // values 0,1,2,3,4,5
    Table input = buildTable(N);
    // even: 0+2+4=6, odd: 1+3+5=9
    AggSpec spec{"value", AggFn::Sum, {"group"}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2u);
    EXPECT_DOUBLE_EQ(res->at("4:even"), 6.0);
    EXPECT_DOUBLE_EQ(res->at("3:odd"),  9.0);
}

TEST(ParallelAggregateTest, EmptyInputReturnsEmptyResult) {
    ParallelExecutor exec;
    Table input;
    AggSpec spec{"value", AggFn::Sum, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
}

TEST(ParallelAggregateTest, DisabledParallelAggregateFallsBackToSequential) {
    ParallelExecutor::ParallelConfig cfg;
    cfg.enable_parallel_aggregate = false;
    ParallelExecutor exec(cfg);

    const size_t N = 200;
    Table input = buildTable(N);
    AggSpec spec{"value", AggFn::Count, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res->at(""), static_cast<double>(N));
}

TEST(ParallelAggregateTest, ParallelAndSequentialResultsAgree) {
    const size_t N = 500;
    Table input = buildTable(N);
    AggSpec spec{"value", AggFn::Sum, {"group"}};

    ParallelExecutor::ParallelConfig seq_cfg;
    seq_cfg.enable_parallel_aggregate = false;
    ParallelExecutor seq_exec(seq_cfg);

    ParallelExecutor::ParallelConfig par_cfg;
    par_cfg.morsel_size = 16;
    par_cfg.max_threads = 4;
    ParallelExecutor par_exec(par_cfg);

    auto seq_res = seq_exec.parallelAggregate(input, spec);
    auto par_res = par_exec.parallelAggregate(input, spec);
    ASSERT_TRUE(seq_res.has_value());
    ASSERT_TRUE(par_res.has_value());

    ASSERT_EQ(seq_res->size(), par_res->size());
    for (const auto& [k, v] : *seq_res) {
        ASSERT_TRUE(par_res->count(k)) << "missing group key: " << k;
        EXPECT_NEAR((*par_res)[k], v, 1e-9) << "mismatch for group: " << k;
    }
}

TEST(ParallelAggregateTest, MorselBoundaryEdgeCase) {
    // Exactly 1 row – always sequential path.
    ParallelExecutor exec;
    BaseEntity::FieldMap f;
    f["value"] = int64_t(42);
    Table input{makeEntity("only", std::move(f))};

    AggSpec spec{"value", AggFn::Sum, {}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res->at(""), 42.0);
}

TEST(ParallelAggregateTest, GroupKeyPipeCharacterNoCollision) {
    // Ensure that field values containing '|' do not cause two different
    // group values to encode to the same group key.
    // We create two groups: g="foo|bar" (value contains the separator) and
    // g="foo", verifying they are counted independently.
    ParallelExecutor exec;

    Table input;
    for (int i = 0; i < 5; ++i) {
        BaseEntity::FieldMap f;
        f["g"]     = std::string("foo|bar"); // value contains '|'
        f["value"] = int64_t(1);
        input.push_back(makeEntity("a" + std::to_string(i), std::move(f)));
    }
    for (int i = 0; i < 3; ++i) {
        BaseEntity::FieldMap f;
        f["g"]     = std::string("foo");
        f["value"] = int64_t(1);
        input.push_back(makeEntity("b" + std::to_string(i), std::move(f)));
    }

    AggSpec spec{"value", AggFn::Count, {"g"}};
    auto res = exec.parallelAggregate(input, spec);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2u);
    EXPECT_DOUBLE_EQ(res->at("7:foo|bar"), 5.0);
    EXPECT_DOUBLE_EQ(res->at("3:foo"),     3.0);
}
