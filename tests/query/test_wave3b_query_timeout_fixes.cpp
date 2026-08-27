/**
 * @file test_wave3b_query_timeout_fixes.cpp
 * @brief Contract tests for query compiler timeout/error hardening.
 */

#include <gtest/gtest.h>

#include "query/query_compiler.h"
#include "utils/expected.h"

using namespace themis;
using namespace themis::query;

TEST(QueryCompilerWave3BContract, ExecuteReturnsErrorWhenExecutorThrows) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 2;
    cfg.enable_jit = true;
    cfg.compilation_timeout_ms = 100;

    QueryCompiler compiler(cfg);
    QueryCompiler::ExecuteFn throwing_executor =
        [](const std::string&, const QueryParams&) -> Result<QueryResult> {
            throw std::runtime_error("wave3b test throw");
        };

    auto compiled = compiler.compile("SELECT 1", {}, throwing_executor);
    auto result = compiler.execute(compiled, {});
    EXPECT_FALSE(result.has_value());
    EXPECT_GT(compiler.stats().total_calls, 0u);
}

TEST(QueryCompilerWave3BContract, ColdPathWorksWithDeterministicExecutor) {
    QueryCompiler::Config cfg;
    cfg.hot_threshold = 100;
    cfg.enable_jit = true;

    QueryCompiler compiler(cfg);
    QueryCompiler::ExecuteFn executor =
        [](const std::string&, const QueryParams&) -> Result<QueryResult> {
            QueryResult r;
            r.rows.push_back(nlohmann::json{{"ok", true}});
            r.rows_examined = 1;
            r.used_compiled_path = false;
            return r;
        };

    auto compiled = compiler.compile("SELECT 1", {}, executor);
    auto result = compiler.execute(compiled, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->rows.size(), 1u);
    EXPECT_FALSE(result->used_compiled_path);
}
