/*
 * Tests for QueryCompiler — Query Compilation & JIT (v1.8.0, Issue #89)
 *
 * Validates all acceptance criteria:
 *   AC-1  Hot-path detection: query transitions from cold to compiled at
 *         hot_threshold executions.
 *   AC-2  Compiled path produces results identical to the interpreted path.
 *   AC-3  Fallback to interpreted execution on compilation errors.
 *   AC-4  Cache key is deterministic and distinct per query text.
 *   AC-5  invalidate() resets a single entry without affecting others.
 *   AC-6  invalidateAll() resets all entries.
 *   AC-7  Statistics counters (total_calls, hot_hits, cold_hits,
 *         compilations, cache_size) are accurate.
 *   AC-8  Default Config values match the specification.
 *   AC-9  JIT disabled (enable_jit=false): always uses cold path.
 *   AC-10 Re-registration (compile() called twice) reuses the entry.
 */

#include "query/query_compiler.h"

#include <gtest/gtest.h>
#include <atomic>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::query;

// =============================================================================
// Helpers
// =============================================================================

/// Build a simple interpreted executor that returns the query text as a
/// single-row result so callers can verify round-trip identity.
static QueryCompiler::ExecuteFn makeEcho() {
    return [](const std::string& q, const QueryParams& /*p*/) -> Result<QueryResult> {
        QueryResult r;
        r.rows = {nlohmann::json{{"query", q}}};
        r.rows_examined    = 1;
        r.used_compiled_path = false;
        r.execution_time_us  = 0;
        return r;
    };
}

/// Build an executor that counts how many times it has been called.
static QueryCompiler::ExecuteFn makeCountingExecutor(std::atomic<int>& counter) {
    return [&counter](const std::string& /*q*/,
                      const QueryParams& /*p*/) -> Result<QueryResult> {
        ++counter;
        QueryResult r;
        r.rows = {nlohmann::json{{"count", counter.load()}}};
        return r;
    };
}

/// Build an executor that always returns an error.
static QueryCompiler::ExecuteFn makeFailingExecutor() {
    return [](const std::string& /*q*/,
              const QueryParams& /*p*/) -> Result<QueryResult> {
        return Err<QueryResult>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, "forced failure");
    };
}

// =============================================================================
// Fixture
// =============================================================================

class QueryJITCompilationFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        QueryCompiler::Config cfg;
        cfg.hot_threshold       = 5;   // low threshold for fast tests
        cfg.enable_jit          = true;
        cfg.opt_level           = QueryCompiler::OptimizationLevel::O2;
        cfg.max_cache_entries   = 64;
        cfg.compilation_timeout_ms = 100;
        compiler_ = std::make_unique<QueryCompiler>(cfg);
    }

    std::unique_ptr<QueryCompiler> compiler_;
};

// =============================================================================
// AC-1  Hot-path detection
// =============================================================================

/// Before hot_threshold the query is NOT compiled.
TEST_F(QueryJITCompilationFocusedTests, AC1_NotCompiledBeforeThreshold) {
    const std::string q = "FOR u IN users RETURN u";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // Execute threshold-1 times
    for (int i = 0; i < 4; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    EXPECT_FALSE(compiler_->isCompiled(handle.key));
    EXPECT_EQ(compiler_->callCount(handle.key), 4u);
}

/// At exactly hot_threshold the query becomes compiled.
TEST_F(QueryJITCompilationFocusedTests, AC1_CompiledAtThreshold) {
    const std::string q = "FOR u IN users FILTER u.age > 30 RETURN u";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // Execute exactly hot_threshold times (threshold == 5 in fixture)
    for (int i = 0; i < 5; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    EXPECT_TRUE(compiler_->isCompiled(handle.key));
}

/// Executions after threshold use the hot path (used_compiled_path = true).
TEST_F(QueryJITCompilationFocusedTests, AC1_HotPathUsedAfterThreshold) {
    const std::string q = "FOR o IN orders RETURN o";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // Warm up past threshold
    for (int i = 0; i < 6; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    // This call should use the compiled path
    auto result = compiler_->execute(handle, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->used_compiled_path);
}

/// On the threshold call itself the hot path is also active.
TEST_F(QueryJITCompilationFocusedTests, AC1_ThresholdCallUsesHotPath) {
    const std::string q = "FOR x IN col RETURN x._key";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // Execute threshold-1 cold calls
    for (int i = 0; i < 4; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    // The 5th call triggers compilation AND uses the hot path
    auto result = compiler_->execute(handle, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->used_compiled_path);
}

// =============================================================================
// AC-2  Result identity: compiled path produces the same rows as interpreter
// =============================================================================

/// Compiled and interpreted executions return identical rows.
TEST_F(QueryJITCompilationFocusedTests, AC2_ResultIdentity) {
    const std::string q = "FOR doc IN docs RETURN doc";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // Capture a cold-path result
    auto cold = compiler_->execute(handle, {});
    ASSERT_TRUE(cold.has_value());

    // Warm up to compiled path
    for (int i = 1; i < 5; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    // Compiled-path result
    auto hot = compiler_->execute(handle, {});
    ASSERT_TRUE(hot.has_value());

    // Row content must be identical
    ASSERT_EQ(cold->rows.size(), hot->rows.size());
    for (size_t i = 0; i < cold->rows.size(); ++i)
        EXPECT_EQ(cold->rows[i], hot->rows[i]);
}

/// Bind-parameter values are forwarded correctly on both paths.
TEST_F(QueryJITCompilationFocusedTests, AC2_BindParamsForwarded) {
    // Executor echoes the first bind param value
    auto paramEcho = [](const std::string& /*q*/, const QueryParams& p)
        -> Result<QueryResult>
    {
        QueryResult r;
        nlohmann::json val = p.count("@v") ? p.at("@v") : "none";
        r.rows = {nlohmann::json{{"v", val}}};
        return r;
    };

    const std::string q = "FOR d IN data FILTER d.v == @v RETURN d";
    auto handle = compiler_->compile(q, {"@v"}, paramEcho);

    QueryParams cold_params{{"@v", "hello"}};
    auto cold = compiler_->execute(handle, cold_params);
    ASSERT_TRUE(cold.has_value());
    EXPECT_EQ(cold->rows[0]["v"], "hello");

    // Warm to hot path
    for (int i = 1; i < 5; ++i)
        static_cast<void>(compiler_->execute(handle, cold_params));

    QueryParams hot_params{{"@v", "world"}};
    auto hot = compiler_->execute(handle, hot_params);
    ASSERT_TRUE(hot.has_value());
    EXPECT_EQ(hot->rows[0]["v"], "world");
}

// =============================================================================
// AC-3  Fallback on compilation error
// =============================================================================

/// If the executor always returns an error, execute() propagates that error.
TEST_F(QueryJITCompilationFocusedTests, AC3_FailingExecutorReturnsError) {
    const std::string q = "FOR bad IN nonexistent RETURN bad";
    auto handle = compiler_->compile(q, {}, makeFailingExecutor());

    for (int i = 0; i < 3; ++i) {
        auto r = compiler_->execute(handle, {});
        EXPECT_FALSE(r.has_value());
    }
}

/// Executing an unknown key returns an error, not a crash.
TEST_F(QueryJITCompilationFocusedTests, AC3_UnknownKeyReturnsError) {
    QueryCompiler::CompiledQuery fake;
    fake.key       = "deadbeef00000000";
    fake.query_text = "unknown query";
    fake.is_compiled = false;

    auto r = compiler_->execute(fake, {});
    EXPECT_FALSE(r.has_value());
}

// =============================================================================
// AC-4  Cache key determinism and uniqueness
// =============================================================================

/// Identical queries produce the same cache key.
TEST_F(QueryJITCompilationFocusedTests, AC4_KeyDeterministic) {
    std::string k1 = QueryCompiler::makeKey("FOR u IN users RETURN u");
    std::string k2 = QueryCompiler::makeKey("FOR u IN users RETURN u");
    EXPECT_EQ(k1, k2);
}

/// Different queries produce different cache keys.
TEST_F(QueryJITCompilationFocusedTests, AC4_KeyDistinct) {
    std::string k1 = QueryCompiler::makeKey("FOR u IN users RETURN u");
    std::string k2 = QueryCompiler::makeKey("FOR o IN orders RETURN o");
    EXPECT_NE(k1, k2);
}

/// Cache key is a 16-character hex string.
TEST_F(QueryJITCompilationFocusedTests, AC4_KeyFormat) {
    std::string key = QueryCompiler::makeKey("SELECT 1");
    EXPECT_EQ(key.size(), 16u);
    for (char c : key) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex char: " << c;
    }
}

/// Empty-string query has a well-defined key.
TEST_F(QueryJITCompilationFocusedTests, AC4_KeyEmptyQuery) {
    std::string key = QueryCompiler::makeKey("");
    EXPECT_EQ(key.size(), 16u);
}

// =============================================================================
// AC-5  Single-entry invalidation
// =============================================================================

/// invalidate() resets a compiled entry back to cold state.
TEST_F(QueryJITCompilationFocusedTests, AC5_InvalidateSingleEntry) {
    const std::string q = "FOR a IN A RETURN a";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // Compile it
    for (int i = 0; i < 5; ++i)
        static_cast<void>(compiler_->execute(handle, {}));
    ASSERT_TRUE(compiler_->isCompiled(handle.key));

    // Invalidate
    compiler_->invalidate(handle.key);
    EXPECT_FALSE(compiler_->isCompiled(handle.key));
    EXPECT_EQ(compiler_->callCount(handle.key), 0u);
}

/// Invalidating one entry does not affect another.
TEST_F(QueryJITCompilationFocusedTests, AC5_InvalidateDoesNotAffectOtherEntry) {
    const std::string q1 = "FOR a IN A RETURN a";
    const std::string q2 = "FOR b IN B RETURN b";

    auto h1 = compiler_->compile(q1, {}, makeEcho());
    auto h2 = compiler_->compile(q2, {}, makeEcho());

    for (int i = 0; i < 5; ++i) {
        static_cast<void>(compiler_->execute(h1, {}));
        static_cast<void>(compiler_->execute(h2, {}));
    }
    ASSERT_TRUE(compiler_->isCompiled(h1.key));
    ASSERT_TRUE(compiler_->isCompiled(h2.key));

    compiler_->invalidate(h1.key);
    EXPECT_FALSE(compiler_->isCompiled(h1.key));
    EXPECT_TRUE(compiler_->isCompiled(h2.key));
}

/// Invalidating an unknown key is a no-op (no crash).
TEST_F(QueryJITCompilationFocusedTests, AC5_InvalidateUnknownKeyIsNoop) {
    EXPECT_NO_THROW(compiler_->invalidate("nonexistentkey00"));
}

// =============================================================================
// AC-6  invalidateAll
// =============================================================================

/// invalidateAll() resets all compiled entries.
TEST_F(QueryJITCompilationFocusedTests, AC6_InvalidateAllResetsEverything) {
    const std::string q1 = "FOR a IN A RETURN a";
    const std::string q2 = "FOR b IN B RETURN b";
    const std::string q3 = "FOR c IN C RETURN c";

    auto h1 = compiler_->compile(q1, {}, makeEcho());
    auto h2 = compiler_->compile(q2, {}, makeEcho());
    auto h3 = compiler_->compile(q3, {}, makeEcho());

    for (int i = 0; i < 5; ++i) {
        static_cast<void>(compiler_->execute(h1, {}));
        static_cast<void>(compiler_->execute(h2, {}));
        static_cast<void>(compiler_->execute(h3, {}));
    }

    ASSERT_TRUE(compiler_->isCompiled(h1.key));
    ASSERT_TRUE(compiler_->isCompiled(h2.key));
    ASSERT_TRUE(compiler_->isCompiled(h3.key));

    compiler_->invalidateAll();

    EXPECT_FALSE(compiler_->isCompiled(h1.key));
    EXPECT_FALSE(compiler_->isCompiled(h2.key));
    EXPECT_FALSE(compiler_->isCompiled(h3.key));
    EXPECT_EQ(compiler_->callCount(h1.key), 0u);
    EXPECT_EQ(compiler_->callCount(h2.key), 0u);
    EXPECT_EQ(compiler_->callCount(h3.key), 0u);
}

/// After invalidateAll() queries can be re-warmed and re-compiled.
TEST_F(QueryJITCompilationFocusedTests, AC6_CanRewarmAfterInvalidateAll) {
    const std::string q = "FOR x IN X RETURN x";
    auto handle = compiler_->compile(q, {}, makeEcho());

    for (int i = 0; i < 5; ++i)
        compiler_->execute(handle, {});
    ASSERT_TRUE(compiler_->isCompiled(handle.key));

    compiler_->invalidateAll();
    ASSERT_FALSE(compiler_->isCompiled(handle.key));

    // Re-warm
    for (int i = 0; i < 5; ++i)
        static_cast<void>(compiler_->execute(handle, {}));
    EXPECT_TRUE(compiler_->isCompiled(handle.key));
}

// =============================================================================
// AC-7  Statistics accuracy
// =============================================================================

/// total_calls increments on every execute().
TEST_F(QueryJITCompilationFocusedTests, AC7_TotalCallsAccurate) {
    const std::string q = "FOR z IN Z RETURN z";
    auto handle = compiler_->compile(q, {}, makeEcho());

    for (int i = 0; i < 8; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    EXPECT_EQ(compiler_->stats().total_calls, 8u);
}

/// cold_hits counts calls on the cold path.
TEST_F(QueryJITCompilationFocusedTests, AC7_ColdHitsBeforeThreshold) {
    const std::string q = "FOR w IN W RETURN w";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // 4 cold calls (threshold is 5)
    for (int i = 0; i < 4; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    EXPECT_EQ(compiler_->stats().cold_hits, 4u);
    EXPECT_EQ(compiler_->stats().hot_hits,  0u);
}

/// hot_hits counts calls on the compiled path.
TEST_F(QueryJITCompilationFocusedTests, AC7_HotHitsAfterThreshold) {
    const std::string q = "FOR h IN H RETURN h";
    auto handle = compiler_->compile(q, {}, makeEcho());

    // 8 total: 4 cold + 1 compile-trigger (hot) + 3 hot
    for (int i = 0; i < 8; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    // At call 5 compilation fires and hot_fn is used; calls 6,7,8 also hot.
    EXPECT_EQ(compiler_->stats().cold_hits, 4u);
    EXPECT_EQ(compiler_->stats().hot_hits,  4u);
}

/// compilations counter tracks the number of specialisations generated.
TEST_F(QueryJITCompilationFocusedTests, AC7_CompilationsCounter) {
    const std::string q1 = "FOR q1 IN C1 RETURN q1";
    const std::string q2 = "FOR q2 IN C2 RETURN q2";

    auto h1 = compiler_->compile(q1, {}, makeEcho());
    auto h2 = compiler_->compile(q2, {}, makeEcho());

    for (int i = 0; i < 5; ++i) {
        static_cast<void>(compiler_->execute(h1, {}));
        static_cast<void>(compiler_->execute(h2, {}));
    }

    EXPECT_EQ(compiler_->stats().compilations, 2u);
}

/// resetStats() zeroes counters without evicting compiled code.
TEST_F(QueryJITCompilationFocusedTests, AC7_ResetStatsPreservesCompiled) {
    const std::string q = "FOR r IN R RETURN r";
    auto handle = compiler_->compile(q, {}, makeEcho());

    for (int i = 0; i < 5; ++i)
        static_cast<void>(compiler_->execute(handle, {}));

    ASSERT_TRUE(compiler_->isCompiled(handle.key));
    ASSERT_GT(compiler_->stats().total_calls, 0u);

    compiler_->resetStats();

    EXPECT_EQ(compiler_->stats().total_calls, 0u);
    EXPECT_EQ(compiler_->stats().hot_hits,    0u);
    EXPECT_EQ(compiler_->stats().cold_hits,   0u);
    // Compiled code must still be present
    EXPECT_TRUE(compiler_->isCompiled(handle.key));
}

// =============================================================================
// AC-8  Default Config values
// =============================================================================

TEST_F(QueryJITCompilationFocusedTests, AC8_DefaultConfigValues) {
    QueryCompiler compiler_with_defaults;
    const auto& cfg = compiler_with_defaults.config();
    EXPECT_EQ(cfg.hot_threshold,        100u);
    EXPECT_TRUE(cfg.enable_jit);
    EXPECT_EQ(cfg.opt_level, QueryCompiler::OptimizationLevel::O2);
    EXPECT_EQ(cfg.max_cache_entries,    512u);
    EXPECT_EQ(cfg.compilation_timeout_ms, 100u);
}

// =============================================================================
// AC-9  JIT disabled
// =============================================================================

/// When enable_jit=false, queries are never compiled regardless of call count.
TEST_F(QueryJITCompilationFocusedTests, AC9_JITDisabledNeverCompiles) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 3;
    cfg.enable_jit    = false;
    QueryCompiler no_jit(cfg);

    std::atomic<int> counter{0};
    auto h = no_jit.compile("FOR d IN D RETURN d", {},
                             makeCountingExecutor(counter));

    for (int i = 0; i < 10; ++i)
        static_cast<void>(no_jit.execute(h, {}));

    EXPECT_FALSE(no_jit.isCompiled(h.key));
    EXPECT_EQ(no_jit.stats().hot_hits, 0u);
    // All calls went through the cold path → executor called 10 times
    EXPECT_EQ(counter.load(), 10);
}

/// With enable_jit=false, used_compiled_path is always false.
TEST_F(QueryJITCompilationFocusedTests, AC9_JITDisabledUsedCompiledPathFalse) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 2;
    cfg.enable_jit    = false;
    QueryCompiler no_jit(cfg);

    auto h = no_jit.compile("FOR e IN E RETURN e", {}, makeEcho());

    for (int i = 0; i < 5; ++i) {
        auto r = no_jit.execute(h, {});
        ASSERT_TRUE(r.has_value());
        EXPECT_FALSE(r->used_compiled_path);
    }
}

// =============================================================================
// AC-10  Re-registration reuses existing entry
// =============================================================================

/// Calling compile() twice for the same query text returns handles with
/// the same key and does not reset the call count.
TEST_F(QueryJITCompilationFocusedTests, AC10_ReregistrationReusesEntry) {
    const std::string q = "FOR r IN reuse RETURN r";

    auto h1 = compiler_->compile(q, {}, makeEcho());

    // Warm up half-way
    for (int i = 0; i < 3; ++i)
        static_cast<void>(compiler_->execute(h1, {}));
    EXPECT_EQ(compiler_->callCount(h1.key), 3u);

    // Re-register with the same query text
    auto h2 = compiler_->compile(q, {}, makeEcho());
    EXPECT_EQ(h1.key, h2.key);

    // Call count is preserved (not reset to zero)
    EXPECT_EQ(compiler_->callCount(h2.key), 3u);
}

// =============================================================================
// Edge cases
// =============================================================================

/// Constructing with O0 disables opt-level-specific code paths, hot path
/// still works correctly.
TEST_F(QueryJITCompilationFocusedTests, Edge_O0OptLevel) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 3;
    cfg.enable_jit    = true;
    cfg.opt_level     = QueryCompiler::OptimizationLevel::O0;
    QueryCompiler compiler_o0(cfg);

    auto h = compiler_o0.compile("FOR x IN X0 RETURN x", {}, makeEcho());
    for (int i = 0; i < 3; ++i)
        static_cast<void>(compiler_o0.execute(h, {}));

    EXPECT_TRUE(compiler_o0.isCompiled(h.key));
    auto r = compiler_o0.execute(h, {});
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->used_compiled_path);
}

/// A threshold of 1 compiles on the very first execute() call.
TEST_F(QueryJITCompilationFocusedTests, Edge_ThresholdOne) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 1;
    cfg.enable_jit    = true;
    QueryCompiler imm(cfg);

    auto h = imm.compile("FOR i IN I1 RETURN i", {}, makeEcho());
    auto r = imm.execute(h, {});
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->used_compiled_path);
    EXPECT_TRUE(imm.isCompiled(h.key));
}

/// Multiple independent QueryCompiler instances do not share state.
TEST_F(QueryJITCompilationFocusedTests, Edge_IndependentInstances) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 3;
    QueryCompiler c1(cfg), c2(cfg);

    const std::string q = "FOR x IN SHARED RETURN x";
    auto h1 = c1.compile(q, {}, makeEcho());
    auto h2 = c2.compile(q, {}, makeEcho());

    // Warm c1 past threshold, leave c2 cold
    for (int i = 0; i < 3; ++i)
        static_cast<void>(c1.execute(h1, {}));

    EXPECT_TRUE(c1.isCompiled(h1.key));
    EXPECT_FALSE(c2.isCompiled(h2.key));
}
