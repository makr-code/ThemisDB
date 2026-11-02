#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include "query/query_canceller.h"
#include "query/vectorized_execution.h"
#include "utils/error_registry.h"

using themis::errors::ErrorCode;
using themis::query::QueryCancellationToken;
using themis::query::VectorizedExecutionEngine;
using themis::query::VectorizedQueryPlan;
using themis::query::VectorizedSortKey;
using json = nlohmann::json;

TEST(VectorizedBoundedExecution, UnsortedLimitStopsRequestingRows) {
    size_t rows_requested = 0;
    const VectorizedExecutionEngine::RowBatchSource source =
        [&rows_requested](size_t offset, size_t count) {
            std::vector<json> batch;
            batch.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                batch.push_back({{"id", offset + i}});
            }
            rows_requested += batch.size();
            return batch;
        };
    VectorizedExecutionEngine::Config config;
    config.batch_size = 3;
    VectorizedExecutionEngine engine(config);
    VectorizedQueryPlan plan;
    plan.setLimit(10);

    auto result = engine.execute(10000000, source, plan);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 10u);
    EXPECT_EQ(rows_requested, 10u);
    EXPECT_EQ(engine.lastStats().rows_in, 10u);
    EXPECT_EQ(engine.lastStats().batches_processed, 4u);
}

TEST(VectorizedBoundedExecution, SortedLimitReturnsGlobalTopK) {
    std::vector<json> rows;
    rows.reserve(100);
    for (int id = 99; id >= 0; --id) {
        rows.push_back({{"id", id}});
    }

    VectorizedExecutionEngine::Config config;
    config.batch_size = 7;
    VectorizedExecutionEngine engine(config);
    VectorizedQueryPlan plan;
    plan.addSort({VectorizedSortKey{"id", true}}).setLimit(3);

    auto result = engine.execute(rows, plan);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 3u);
    EXPECT_EQ((*result)[0]["id"], 0);
    EXPECT_EQ((*result)[1]["id"], 1);
    EXPECT_EQ((*result)[2]["id"], 2);
    EXPECT_EQ(engine.lastStats().rows_in, rows.size());
}

TEST(VectorizedBoundedExecution, SortedTopKMatchesNullOrdering) {
    const std::vector<json> rows = {
        {{"id", nullptr}},
        {{"id", 1}},
        {{"id", 3}},
    };
    VectorizedExecutionEngine engine;

    VectorizedQueryPlan ascending;
    ascending.addSort({VectorizedSortKey{"id", true}}).setLimit(2);
    auto ascending_result = engine.execute(rows, ascending);
    ASSERT_TRUE(ascending_result.has_value())
        << ascending_result.error().message();
    ASSERT_EQ(ascending_result->size(), 2u);
    EXPECT_EQ((*ascending_result)[0]["id"], 1);
    EXPECT_EQ((*ascending_result)[1]["id"], 3);

    VectorizedQueryPlan descending;
    descending.addSort({VectorizedSortKey{"id", false}}).setLimit(2);
    auto descending_result = engine.execute(rows, descending);
    ASSERT_TRUE(descending_result.has_value())
        << descending_result.error().message();
    ASSERT_EQ(descending_result->size(), 2u);
    EXPECT_TRUE((*descending_result)[0]["id"].is_null());
    EXPECT_EQ((*descending_result)[1]["id"], 3);
}

TEST(VectorizedBoundedExecution, LimitDoesNotTruncateAggregateInput) {
    std::vector<json> rows(100, json{{"value", 1}});
    VectorizedExecutionEngine::Config config;
    config.batch_size = rows.size();
    VectorizedExecutionEngine engine(config);
    VectorizedQueryPlan plan;
    plan.addAggregate({{
        .result_field = "count",
        .input_field = "",
        .function = themis::query::VectorizedAggregation::Function::Count,
    }}).setLimit(1);

    auto result = engine.execute(rows, plan);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ((*result)[0]["count"], 100.0);
    EXPECT_EQ(engine.lastStats().rows_in, rows.size());
}

TEST(VectorizedBoundedExecution, MemoryBudgetReturnsTypedError) {
    VectorizedExecutionEngine::Config config;
    config.max_memory_bytes = 1;
    VectorizedExecutionEngine engine(config);
    VectorizedQueryPlan plan;
    const std::vector<json> rows = {{{"id", 1}}};

    auto result = engine.execute(rows, plan);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_QUERY_MEMORY_LIMIT_EXCEEDED);
    EXPECT_NE(result.error().message().find("QueryMemoryLimitExceeded"),
              std::string::npos);
}

TEST(VectorizedBoundedExecution, CancellationStopsBeforeRequestingRows) {
    QueryCancellationToken cancellation;
    cancellation.cancel();
    size_t source_calls = 0;
    const VectorizedExecutionEngine::RowBatchSource source =
        [&source_calls](size_t, size_t count) {
            ++source_calls;
            return std::vector<json>(count, json{{"id", 1}});
        };
    VectorizedExecutionEngine engine;
    VectorizedQueryPlan plan;

    auto result = engine.execute(100, source, plan, &cancellation);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_QUERY_CANCELLED);
    EXPECT_EQ(source_calls, 0u);
}

TEST(VectorizedBoundedExecution, RejectsIncompleteSourceBatch) {
    const VectorizedExecutionEngine::RowBatchSource source =
        [](size_t, size_t) { return std::vector<json>{}; };
    VectorizedExecutionEngine engine;
    VectorizedQueryPlan plan;

    auto result = engine.execute(1, source, plan);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_QUERY_INVALID_INPUT);
}
