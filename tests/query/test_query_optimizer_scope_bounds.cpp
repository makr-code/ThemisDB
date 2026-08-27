/**
 * @file test_query_optimizer_scope_bounds.cpp
 * @brief Scope-bounds contract tests for QueryOptimizer::Plan
 */

#include <gtest/gtest.h>

#include "query/query_optimizer.h"

using namespace themis::query;

TEST(QueryOptimizerScopeBoundsContract, EmptyBoundsAreInvalid) {
    QueryOptimizer::Plan plan;
    EXPECT_FALSE(plan.has_valid_scope_bounds());
}

TEST(QueryOptimizerScopeBoundsContract, RowBoundMakesPlanValid) {
    QueryOptimizer::Plan plan;
    plan.scope_bounds.scope_id = "tenant_a/db_users";
    plan.scope_bounds.max_result_rows = 1000;
    EXPECT_TRUE(plan.has_valid_scope_bounds());
}

TEST(QueryOptimizerScopeBoundsContract, ByteBoundMakesPlanValid) {
    QueryOptimizer::Plan plan;
    plan.scope_bounds.scope_id = "tenant_a/db_users";
    plan.scope_bounds.max_result_bytes = 1 * 1024 * 1024;
    EXPECT_TRUE(plan.has_valid_scope_bounds());
}

TEST(QueryOptimizerScopeBoundsContract, EmptyScopeIdIsAlwaysInvalid) {
    QueryOptimizer::Plan plan;
    plan.scope_bounds.max_result_rows = 1000;
    plan.scope_bounds.max_result_bytes = 1024;
    EXPECT_FALSE(plan.has_valid_scope_bounds());
}
