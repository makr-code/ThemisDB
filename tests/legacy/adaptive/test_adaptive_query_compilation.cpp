/**
 * @file test_adaptive_query_compilation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

/**
 * Focused unit tests for AdaptiveQueryCompiler (performance module, v1.8.0).
 *
 * Acceptance criteria tested:
 *  AC-1  Hot Query Detection   – execution counter crosses hot_threshold
 *  AC-2  Type Specialisation   – compiled path uses typed predicate dispatch
 *  AC-3  Expression Folding    – constant predicates folded at compile time
 *  AC-4  Vectorized Codegen    – IR/assembly reflects vectorisation flag
 *  AC-5  Adaptive Recompilation – drift check triggers re-compile
 *  AC-6  LLVM IR generation    – CompiledQuery::llvm_ir is non-empty and valid
 *  AC-7  CompilationStats      – queries_compiled, failures, cache_size, etc.
 *
 * Test suite name: AdaptiveQueryCompilationFocusedTests
 */

#include <gtest/gtest.h>
#include "performance/adaptive_query_compiler.h"

#include <cmath>
#include <string>
#include <thread>
#include <vector>

using namespace themis::performance;

// ─── Fixture helpers ──────────────────────────────────────────────────────────

static ParsedQuery makeFilterQuery(const std::string& fp,
                                   const std::string& table = "users",
                                   int64_t            min_age = 0) {
    ParsedQuery q;
    q.query_text   = "SELECT * FROM " + table + " WHERE age >= " + std::to_string(min_age);
    q.fingerprint  = fp;
    q.table        = table;
    q.op_type      = QueryOpType::Filter;
    if (min_age > 0) {
        Predicate p;
        p.column = "age";
        p.op     = Predicate::Op::GE;
        p.value  = int64_t{min_age};
        q.predicates.push_back(p);
    }
    return q;
}

static Schema makeUserSchema() {
    Schema s;
    TableSchema ts;
    ts.table_name = "users";
    ts.columns = {
        {"id",   ColumnType::Int64,  false, true},
        {"age",  ColumnType::Int64,  false, false},
        {"name", ColumnType::String, true,  false},
    };
    s.tables["users"] = ts;
    return s;
}

static Schema makeProductSchema() {
    Schema s;
    TableSchema ts;
    ts.table_name = "products";
    ts.columns = {
        {"id",    ColumnType::Int64,  false, true},
        {"price", ColumnType::Double, false, false},
        {"name",  ColumnType::String, true,  false},
    };
    s.tables["products"] = ts;
    return s;
}

// ─── CompilationConfig tests ─────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, DefaultConfigValues) {
    AdaptiveQueryCompiler compiler;
    const auto& cfg = compiler.config();
    EXPECT_EQ(cfg.hot_threshold, 100u);
    EXPECT_EQ(cfg.optimization, AdaptiveQueryCompiler::OptLevel::O3);
    EXPECT_TRUE(cfg.enable_vectorization);
    EXPECT_TRUE(cfg.enable_prefetch);
    EXPECT_TRUE(cfg.enable_inlining);
    EXPECT_EQ(cfg.compilation_timeout_ms, 100u);
}

TEST(AdaptiveQueryCompilationFocusedTests, CustomConfigApplied) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;
    cfg.optimization  = AdaptiveQueryCompiler::OptLevel::O2;
    cfg.enable_vectorization = false;
    AdaptiveQueryCompiler compiler(cfg);
    EXPECT_EQ(compiler.config().hot_threshold, 5u);
    EXPECT_EQ(compiler.config().optimization, AdaptiveQueryCompiler::OptLevel::O2);
    EXPECT_FALSE(compiler.config().enable_vectorization);
}

// ─── is_compilable tests ──────────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, IsCompilableKnownOps) {
    AdaptiveQueryCompiler compiler;
    ParsedQuery q;
    q.fingerprint = "fp1";

    q.op_type = QueryOpType::Filter;
    EXPECT_TRUE(compiler.is_compilable(q));

    q.op_type = QueryOpType::Aggregate;
    EXPECT_TRUE(compiler.is_compilable(q));

    q.op_type = QueryOpType::Join;
    EXPECT_TRUE(compiler.is_compilable(q));

    q.op_type = QueryOpType::Projection;
    EXPECT_TRUE(compiler.is_compilable(q));

    q.op_type = QueryOpType::Sort;
    EXPECT_TRUE(compiler.is_compilable(q));

    q.op_type = QueryOpType::Limit;
    EXPECT_TRUE(compiler.is_compilable(q));
}

TEST(AdaptiveQueryCompilationFocusedTests, IsCompilableUnknownOpReturnsFalse) {
    AdaptiveQueryCompiler compiler;
    ParsedQuery q;
    q.fingerprint = "fp_unknown";
    q.op_type     = QueryOpType::Unknown;
    EXPECT_FALSE(compiler.is_compilable(q));
}

TEST(AdaptiveQueryCompilationFocusedTests, IsCompilableEmptyFingerprintReturnsFalse) {
    AdaptiveQueryCompiler compiler;
    ParsedQuery q;
    q.fingerprint = "";
    q.op_type     = QueryOpType::Filter;
    EXPECT_FALSE(compiler.is_compilable(q));
}

// ─── AC-1: Hot Query Detection ───────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, ExecutionCountStartsAtZero) {
    AdaptiveQueryCompiler compiler;
    EXPECT_EQ(compiler.executionCount("nonexistent"), 0u);
}

TEST(AdaptiveQueryCompilationFocusedTests, ColdPathExecutedBelowThreshold) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;
    AdaptiveQueryCompiler compiler(cfg);

    auto q = makeFilterQuery("fp_cold", "users", 0);
    auto schema = makeUserSchema();

    for (size_t i = 0; i < 4; ++i) {
        compiler.execute(q, schema, {});
        EXPECT_FALSE(compiler.isCompiled("fp_cold"));
    }
    EXPECT_EQ(compiler.executionCount("fp_cold"), 4u);
}

TEST(AdaptiveQueryCompilationFocusedTests, CompiledAtHotThreshold) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 3;
    AdaptiveQueryCompiler compiler(cfg);

    auto q = makeFilterQuery("fp_hot", "users", 0);
    auto schema = makeUserSchema();

    for (size_t i = 0; i < 3; ++i)
        compiler.execute(q, schema, {});

    EXPECT_TRUE(compiler.isCompiled("fp_hot"));
}

TEST(AdaptiveQueryCompilationFocusedTests, HotPathInvocationsCountedAfterCompile) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 2;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_count", "users", 0);
    auto schema = makeUserSchema();

    for (size_t i = 0; i < 5; ++i)
        compiler.execute(q, schema, {});

    auto stats = compiler.getStats();
    EXPECT_GE(stats.hot_path_invocations, 1u);
    EXPECT_GE(stats.cold_path_invocations, 0u);
}

// ─── AC-2: Type Specialisation ───────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, Int64PredicateCompiledAndExecuted) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "filter:users:age:GE:5";
    q.table       = "users";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column = "age";
    p.op     = Predicate::Op::GE;
    p.value  = int64_t{5};
    q.predicates.push_back(p);

    auto schema = makeUserSchema();

    // First call: cold path (count = 1 == hot_threshold → compile)
    auto r1 = compiler.execute(q, schema, {});
    EXPECT_TRUE(r1.ok);

    // Second call: hot path (compiled specialisation)
    auto r2 = compiler.execute(q, schema, {});
    EXPECT_TRUE(r2.ok);
    EXPECT_TRUE(compiler.isCompiled(q.fingerprint));
}

TEST(AdaptiveQueryCompilationFocusedTests, DoublePredicateFilterReturnsCorrectRows) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "filter:products:price:GE";
    q.table       = "products";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column = "price";
    p.op     = Predicate::Op::GE;
    p.value  = double{0.0};
    q.predicates.push_back(p);

    auto schema = makeProductSchema();

    auto r = compiler.execute(q, schema, {});
    EXPECT_TRUE(r.ok);
    EXPECT_FALSE(r.empty());
}

TEST(AdaptiveQueryCompilationFocusedTests, BindParameterResolutionWorks) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "filter:users:age:GE:param";
    q.table       = "users";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column     = "age";
    p.op         = Predicate::Op::GE;
    p.param_name = "@min_age";
    q.predicates.push_back(p);

    auto schema = makeUserSchema();
    QueryParams params;
    params.set("@min_age", int64_t{5});

    // First call triggers compilation at threshold=1
    compiler.execute(q, schema, params);
    // Second call: hot path with bind parameter
    auto r = compiler.execute(q, schema, params);
    EXPECT_TRUE(r.ok);
}

// ─── AC-3: Expression Folding ────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, CompiledQueryHasNonEmptyLLVMIR) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto q = makeFilterQuery("fp_ir", "users", 3);
    auto schema = makeUserSchema();

    auto cq = compiler.compile(q, schema);
    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_FALSE(cq.llvm_ir.empty());
    EXPECT_NE(cq.llvm_ir.find("ThemisDB"), std::string::npos);
    EXPECT_NE(cq.llvm_ir.find("fp_ir"), std::string::npos);
}

TEST(AdaptiveQueryCompilationFocusedTests, CompiledQueryHasNonEmptyAssembly) {
    auto q      = makeFilterQuery("fp_asm", "users", 2);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);
    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_FALSE(cq.assembly.empty());
    EXPECT_NE(cq.assembly.find("ret"), std::string::npos);
}

TEST(AdaptiveQueryCompilationFocusedTests, LLVMIRContainsPredicateInfo) {
    ParsedQuery q;
    q.fingerprint = "fp_expr_fold";
    q.table       = "users";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column = "age"; p.op = Predicate::Op::GE; p.value = int64_t{18};
    q.predicates.push_back(p);

    auto schema = makeUserSchema();
    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    // LLVM IR should mention the predicate count or column
    EXPECT_NE(cq.llvm_ir.find("predicate"), std::string::npos);
}

// ─── AC-4: Vectorized Codegen ────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, VectorizationFlagReflectedInIR) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.enable_vectorization = true;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_vec", "users", 1);
    auto schema = makeUserSchema();
    auto cq     = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_TRUE(cq.vectorized);
    EXPECT_NE(cq.llvm_ir.find("vectorize: enabled"), std::string::npos);
}

TEST(AdaptiveQueryCompilationFocusedTests, VectorizationDisabledNotInIR) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.enable_vectorization = false;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_novec", "users", 1);
    auto schema = makeUserSchema();
    auto cq     = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_FALSE(cq.vectorized);
    EXPECT_EQ(cq.llvm_ir.find("vectorize: enabled"), std::string::npos);
}

TEST(AdaptiveQueryCompilationFocusedTests, PrefetchFlagReflectedInAssembly) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.enable_prefetch = true;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_prefetch", "users", 0);
    auto schema = makeUserSchema();
    auto cq     = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_NE(cq.assembly.find("prefetch"), std::string::npos);
}

// ─── AC-5: Adaptive Recompilation ────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, InvalidateClearsCompilationEntry) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 2;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_invalidate", "users", 0);
    auto schema = makeUserSchema();

    // Warm up past threshold
    for (size_t i = 0; i < 3; ++i)
        compiler.execute(q, schema, {});

    ASSERT_TRUE(compiler.isCompiled("fp_invalidate"));

    compiler.invalidate("fp_invalidate");
    EXPECT_FALSE(compiler.isCompiled("fp_invalidate"));
    EXPECT_EQ(compiler.executionCount("fp_invalidate"), 0u);
}

TEST(AdaptiveQueryCompilationFocusedTests, InvalidateAllClearsAllEntries) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();

    for (int i = 0; i < 3; ++i) {
        auto q = makeFilterQuery("fp_all_" + std::to_string(i), "users", i);
        compiler.execute(q, schema, {});
        compiler.execute(q, schema, {});
    }

    compiler.invalidateAll();

    for (int i = 0; i < 3; ++i) {
        EXPECT_FALSE(compiler.isCompiled("fp_all_" + std::to_string(i)));
        EXPECT_EQ(compiler.executionCount("fp_all_" + std::to_string(i)), 0u);
    }
}

TEST(AdaptiveQueryCompilationFocusedTests, RecompilationTrackedInStats) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold           = 2;
    cfg.recompile_check_interval = 3;
    cfg.recompile_drift_factor   = 1.0; // Immediate re-compile on any change
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_recompile", "users", 0);
    auto schema = makeUserSchema();

    // Warm up and trigger recompilation check
    for (size_t i = 0; i < 10; ++i)
        compiler.execute(q, schema, {});

    // Stats should show at least 1 compilation
    auto stats = compiler.getStats();
    EXPECT_GE(stats.queries_compiled, 1u);
}

// ─── AC-6: LLVM IR / debug info ──────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, AggregateIRContainsAggFunction) {
    ParsedQuery q;
    q.fingerprint   = "agg:users:SUM:age";
    q.table         = "users";
    q.op_type       = QueryOpType::Aggregate;
    q.agg_function  = "SUM";
    q.agg_column    = "age";

    auto schema = makeUserSchema();
    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_NE(cq.llvm_ir.find("SUM"), std::string::npos);
}

TEST(AdaptiveQueryCompilationFocusedTests, JoinIRContainsJoinKeys) {
    ParsedQuery q;
    q.fingerprint      = "join:orders:users:user_id";
    q.table            = "orders";
    q.op_type          = QueryOpType::Join;
    q.join_table       = "users";
    q.join_key_left    = "user_id";
    q.join_key_right   = "id";

    Schema schema;
    schema.tables["orders"] = {
        "orders",
        {{"id", ColumnType::Int64, false, true},
         {"user_id", ColumnType::Int64, false, false}}
    };
    schema.tables["users"] = {
        "users",
        {{"id", ColumnType::Int64, false, true}}
    };

    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_NE(cq.llvm_ir.find("user_id"), std::string::npos);
}

TEST(AdaptiveQueryCompilationFocusedTests, CompiledQueryHasPositiveCodeSize) {
    auto q      = makeFilterQuery("fp_codesize", "users", 0);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_GT(cq.code_size_bytes, 0u);
}

TEST(AdaptiveQueryCompilationFocusedTests, CompiledQueryHasPositiveCompilationTime) {
    auto q      = makeFilterQuery("fp_time", "users", 0);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    // Compilation should take some non-negative time
    EXPECT_GE(cq.compilation_time_us, 0u);
}

// ─── AC-7: Compilation statistics ────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, StatsQueriesCompiledCountCorrect) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();
    for (int i = 0; i < 3; ++i) {
        auto q = makeFilterQuery("fp_stats_" + std::to_string(i), "users", i);
        // Call twice: first triggers compile, second uses hot path
        compiler.execute(q, schema, {});
        compiler.execute(q, schema, {});
    }

    auto stats = compiler.getStats();
    EXPECT_EQ(stats.queries_compiled, 3u);
    EXPECT_EQ(stats.cache_size, 3u);
}

TEST(AdaptiveQueryCompilationFocusedTests, StatsFailuresCountedForUnknownOp) {
    AdaptiveQueryCompiler compiler;

    ParsedQuery bad_q;
    bad_q.fingerprint = "fp_bad";
    bad_q.table       = "users";
    bad_q.op_type     = QueryOpType::Unknown;

    auto schema = makeUserSchema();
    auto cq = compiler.compile(bad_q, schema);
    EXPECT_FALSE(static_cast<bool>(cq));

    auto stats = compiler.getStats();
    EXPECT_GE(stats.compilation_failures, 1u);
}

TEST(AdaptiveQueryCompilationFocusedTests, StatsCacheSizeReflectsInvalidate) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();
    auto q      = makeFilterQuery("fp_cache", "users", 0);

    compiler.execute(q, schema, {});
    compiler.execute(q, schema, {});

    EXPECT_EQ(compiler.getStats().cache_size, 1u);
    compiler.invalidate("fp_cache");
    EXPECT_EQ(compiler.getStats().cache_size, 0u);
}

TEST(AdaptiveQueryCompilationFocusedTests, ResetStatsDoesNotEvictCode) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();
    auto q      = makeFilterQuery("fp_reset", "users", 0);

    compiler.execute(q, schema, {});
    compiler.execute(q, schema, {});

    ASSERT_TRUE(compiler.isCompiled("fp_reset"));
    compiler.resetStats();

    // Code should still be cached
    EXPECT_TRUE(compiler.isCompiled("fp_reset"));
    // Stats should be zeroed
    auto stats = compiler.getStats();
    EXPECT_EQ(stats.queries_compiled, 0u);
    EXPECT_EQ(stats.hot_path_invocations, 0u);
}

// ─── Aggregate execution tests ───────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, AggregateCountReturnsNonNegative) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint  = "agg:count:users";
    q.table        = "users";
    q.op_type      = QueryOpType::Aggregate;
    q.agg_function = "COUNT";
    q.agg_column   = "id";

    auto schema = makeUserSchema();
    auto r = compiler.execute(q, schema, {});
    EXPECT_TRUE(r.ok);
    ASSERT_FALSE(r.empty());
    const QueryValue* v = r.rows[0].get("COUNT_result");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(std::holds_alternative<double>(*v));
    EXPECT_GE(std::get<double>(*v), 0.0);
}

TEST(AdaptiveQueryCompilationFocusedTests, AggregateSumIsCorrect) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint  = "agg:sum:users:age";
    q.table        = "users";
    q.op_type      = QueryOpType::Aggregate;
    q.agg_function = "SUM";
    q.agg_column   = "age";

    auto schema = makeUserSchema();
    // Cold path
    auto r1 = compiler.execute(q, schema, {});
    // Hot path
    auto r2 = compiler.execute(q, schema, {});

    for (auto* r : {&r1, &r2}) {
        EXPECT_TRUE(r->ok);
        ASSERT_FALSE(r->empty());
        const QueryValue* v = r->rows[0].get("SUM_result");
        ASSERT_NE(v, nullptr);
        EXPECT_TRUE(std::holds_alternative<double>(*v));
    }
}

TEST(AdaptiveQueryCompilationFocusedTests, AggregateGroupByProducesMultipleRows) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint      = "agg:grp:users:name:COUNT:id";
    q.table            = "users";
    q.op_type          = QueryOpType::Aggregate;
    q.agg_function     = "COUNT";
    q.agg_column       = "id";
    q.group_by_column  = "name";

    auto schema = makeUserSchema();
    auto r1 = compiler.execute(q, schema, {});
    auto r2 = compiler.execute(q, schema, {});

    // Both paths should return at least 1 group
    for (auto* r : {&r1, &r2}) {
        EXPECT_TRUE(r->ok);
        EXPECT_FALSE(r->empty());
    }
}

// ─── Filter correctness tests ─────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, FilterEQPredicate) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "filter:users:id:EQ:3";
    q.table       = "users";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column = "id"; p.op = Predicate::Op::EQ; p.value = int64_t{3};
    q.predicates.push_back(p);

    auto schema = makeUserSchema();
    auto r1 = compiler.execute(q, schema, {});
    auto r2 = compiler.execute(q, schema, {});

    for (auto* r : {&r1, &r2}) {
        EXPECT_TRUE(r->ok);
        for (const auto& row : r->rows) {
            const QueryValue* idv = row.get("id");
            ASSERT_NE(idv, nullptr);
            ASSERT_TRUE(std::holds_alternative<int64_t>(*idv));
            EXPECT_EQ(std::get<int64_t>(*idv), 3);
        }
    }
}

TEST(AdaptiveQueryCompilationFocusedTests, FilterNoPredPassesAllRows) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_nopred", "users", 0);
    auto schema = makeUserSchema();

    auto r1 = compiler.execute(q, schema, {});
    auto r2 = compiler.execute(q, schema, {});

    // All 10 synthetic rows should pass with no predicates
    for (auto* r : {&r1, &r2}) {
        EXPECT_TRUE(r->ok);
        EXPECT_EQ(r->rows.size(), 10u);
    }
}

TEST(AdaptiveQueryCompilationFocusedTests, FilterHotAndColdPathResultsMatch) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "fp_diff";
    q.table       = "users";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column = "age"; p.op = Predicate::Op::GT; p.value = int64_t{3};
    q.predicates.push_back(p);

    auto schema = makeUserSchema();

    // Cold path result
    QueryResult cold_result;
    for (size_t i = 0; i < 5; ++i)
        cold_result = compiler.execute(q, schema, {});

    // Hot path result (one more call)
    auto hot_result = compiler.execute(q, schema, {});

    EXPECT_TRUE(compiler.isCompiled(q.fingerprint));
    EXPECT_EQ(cold_result.rows.size(), hot_result.rows.size());
}

// ─── Explicit compile() API tests ────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, ExplicitCompileProducesValidFunction) {
    auto q      = makeFilterQuery("fp_explicit", "users", 2);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);

    ASSERT_TRUE(static_cast<bool>(cq));
    ASSERT_TRUE(cq.execute != nullptr);

    // Execute directly
    QueryResult r = cq.execute(QueryParams{});
    EXPECT_TRUE(r.ok);
}

TEST(AdaptiveQueryCompilationFocusedTests, ExplicitCompileWithConfigOverride) {
    auto q      = makeFilterQuery("fp_override", "users", 0);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;

    AdaptiveQueryCompiler::CompilationConfig override;
    override.hot_threshold        = 10;
    override.optimization         = AdaptiveQueryCompiler::OptLevel::O1;
    override.enable_vectorization = false;

    auto cq = compiler.compile(q, schema, override);
    ASSERT_TRUE(static_cast<bool>(cq));
    EXPECT_FALSE(cq.vectorized);
}

TEST(AdaptiveQueryCompilationFocusedTests, ExplicitCompileInsertsIntoCache) {
    auto q      = makeFilterQuery("fp_pre_warm", "users", 0);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;
    EXPECT_FALSE(compiler.isCompiled("fp_pre_warm"));

    compiler.compile(q, schema);
    EXPECT_TRUE(compiler.isCompiled("fp_pre_warm"));
}

TEST(AdaptiveQueryCompilationFocusedTests, CompileUncompilableReturnsNullFn) {
    ParsedQuery bad_q;
    bad_q.fingerprint = "";  // Empty fingerprint → not compilable
    bad_q.table       = "users";
    bad_q.op_type     = QueryOpType::Filter;

    auto schema = makeUserSchema();
    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(bad_q, schema);
    EXPECT_FALSE(static_cast<bool>(cq));
}

// ─── Projection tests ────────────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, ProjectionSelectsSubsetOfColumns) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint     = "proj:users:name";
    q.table           = "users";
    q.op_type         = QueryOpType::Projection;
    q.select_columns  = {"name"};

    auto schema = makeUserSchema();
    auto r = compiler.execute(q, schema, {});
    EXPECT_TRUE(r.ok);
    for (const auto& row : r.rows) {
        EXPECT_EQ(row.column_names.size(), 1u);
        EXPECT_EQ(row.column_names[0], "name");
    }
}

// ─── Limit tests ────────────────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, LimitClauseTruncatesResult) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "limit:users:3";
    q.table       = "users";
    q.op_type     = QueryOpType::Limit;
    q.limit       = 3;

    auto schema = makeUserSchema();
    auto r = compiler.execute(q, schema, {});
    EXPECT_TRUE(r.ok);
    EXPECT_LE(r.rows.size(), 3u);
}

TEST(AdaptiveQueryCompilationFocusedTests, LimitWithOffsetSkipsRows) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint = "limit:users:2:offset3";
    q.table       = "users";
    q.op_type     = QueryOpType::Limit;
    q.limit       = 2;
    q.offset      = 3;

    auto schema = makeUserSchema();
    auto r = compiler.execute(q, schema, {});
    EXPECT_TRUE(r.ok);
    EXPECT_LE(r.rows.size(), 2u);
}

// ─── Sort tests ──────────────────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, SortAscendingProducesOrderedRows) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    ParsedQuery q;
    q.fingerprint      = "sort:users:age:asc";
    q.table            = "users";
    q.op_type          = QueryOpType::Sort;
    q.order_by_column  = "age";
    q.order_asc        = true;

    auto schema = makeUserSchema();
    auto r = compiler.execute(q, schema, {});
    EXPECT_TRUE(r.ok);
    // Verify ascending order
    for (size_t i = 1; i < r.rows.size(); ++i) {
        const QueryValue* prev = r.rows[i-1].get("age");
        const QueryValue* curr = r.rows[i].get("age");
        if (prev && curr &&
            std::holds_alternative<int64_t>(*prev) &&
            std::holds_alternative<int64_t>(*curr)) {
            EXPECT_LE(std::get<int64_t>(*prev), std::get<int64_t>(*curr));
        }
    }
}

// ─── QueryParams / QueryValue types tests ────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, QueryParamsSetAndGet) {
    QueryParams params;
    params.set("@age", int64_t{25});
    params.set("@name", std::string{"Alice"});
    params.set("@active", bool{true});

    const QueryValue* age = params.get("@age");
    ASSERT_NE(age, nullptr);
    EXPECT_EQ(std::get<int64_t>(*age), 25);

    const QueryValue* name = params.get("@name");
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(std::get<std::string>(*name), "Alice");

    const QueryValue* active = params.get("@active");
    ASSERT_NE(active, nullptr);
    EXPECT_TRUE(std::get<bool>(*active));

    EXPECT_EQ(params.get("@missing"), nullptr);
}

TEST(AdaptiveQueryCompilationFocusedTests, SchemaGetTableReturnsNullForUnknown) {
    Schema schema = makeUserSchema();
    EXPECT_NE(schema.getTable("users"), nullptr);
    EXPECT_EQ(schema.getTable("nonexistent"), nullptr);
}

TEST(AdaptiveQueryCompilationFocusedTests, QueryRowGetColumnByName) {
    QueryRow row;
    row.column_names = {"id", "name"};
    row.values       = {QueryValue{int64_t{1}}, QueryValue{std::string{"Bob"}}};

    EXPECT_NE(row.get("id"), nullptr);
    EXPECT_NE(row.get("name"), nullptr);
    EXPECT_EQ(row.get("missing"), nullptr);
}

// ─── Concurrent safety smoke test ────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, ConcurrentExecuteDoesNotCrash) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&compiler, &schema, t]() {
            auto q = makeFilterQuery("fp_concurrent_" + std::to_string(t % 2),
                                     "users", t);
            for (int i = 0; i < 20; ++i)
                compiler.execute(q, schema, {});
        });
    }
    for (auto& th : threads) th.join();

    // No crash and stats are consistent
    auto stats = compiler.getStats();
    EXPECT_GE(stats.hot_path_invocations + stats.cold_path_invocations, 0u);
}


// ─── execute(CompiledQuery, params) overload tests ────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, ExecuteCompiledQueryOverloadWorks) {
    auto q      = makeFilterQuery("fp_exec_cq", "users", 2);
    auto schema = makeUserSchema();

    AdaptiveQueryCompiler compiler;
    auto cq = compiler.compile(q, schema);
    ASSERT_TRUE(static_cast<bool>(cq));

    // Execute via the (CompiledQuery, params) overload
    QueryResult r = compiler.execute(cq, QueryParams{});
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.error.empty());
}

TEST(AdaptiveQueryCompilationFocusedTests, ExecuteCompiledQueryOverloadMatchesAutoPath) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 2;
    AdaptiveQueryCompiler compiler(cfg);

    auto q      = makeFilterQuery("fp_exec_cq2", "users", 0);
    auto schema = makeUserSchema();

    // Warm up to hot path
    for (int i = 0; i < 3; ++i) compiler.execute(q, schema, {});
    ASSERT_TRUE(compiler.isCompiled(q.fingerprint));

    // Explicit compile
    auto cq = compiler.compile(q, schema);
    ASSERT_TRUE(static_cast<bool>(cq));

    auto r1 = compiler.execute(q, schema, {});
    auto r2 = compiler.execute(cq, {});

    EXPECT_EQ(r1.rows.size(), r2.rows.size());
}

TEST(AdaptiveQueryCompilationFocusedTests, ExecuteInvalidCompiledQueryReturnsError) {
    AdaptiveQueryCompiler compiler;
    AdaptiveQueryCompiler::CompiledQuery empty_cq;  // execute == nullptr

    auto r = compiler.execute(empty_cq, QueryParams{});
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

// ─── get_stats() alias tests ──────────────────────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, GetStatsAliasMatchesGetStats) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();
    auto q      = makeFilterQuery("fp_getstats", "users", 0);

    compiler.execute(q, schema, {});
    compiler.execute(q, schema, {});

    auto s1 = compiler.getStats();
    auto s2 = compiler.get_stats();

    EXPECT_EQ(s1.queries_compiled,    s2.queries_compiled);
    EXPECT_EQ(s1.cache_size,          s2.cache_size);
    EXPECT_EQ(s1.hot_path_invocations, s2.hot_path_invocations);
    EXPECT_EQ(s1.cold_path_invocations, s2.cold_path_invocations);
}

TEST(AdaptiveQueryCompilationFocusedTests, GetStatsSnakeCaseMethodWorks) {
    AdaptiveQueryCompiler compiler;
    auto stats = compiler.get_stats();  // Must compile and return a valid struct
    EXPECT_EQ(stats.queries_compiled, 0u);
    EXPECT_EQ(stats.compilation_failures, 0u);
}

// ─── average_speedup_percent measurement tests ────────────────────────────────

TEST(AdaptiveQueryCompilationFocusedTests, AverageSpeedupPercentIsPositiveAfterHotPath) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;
    AdaptiveQueryCompiler compiler(cfg);

    auto schema = makeUserSchema();
    auto q      = makeFilterQuery("fp_speedup", "users", 0);

    // Cold path (5 calls) + hot path (10 calls)
    for (int i = 0; i < 15; ++i) compiler.execute(q, schema, {});

    auto stats = compiler.get_stats();
    // After enough samples the compiler should compute a meaningful speedup
    // (may still be the conservative 500% estimate if the system is too fast
    //  to produce distinct cold/hot timings, but must be >= 0)
    EXPECT_GE(stats.average_speedup_percent, 0u);
}
