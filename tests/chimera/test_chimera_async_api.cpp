/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_chimera_async_api.cpp                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:22:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     497                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 3760a281cf  2026-03-12  fix(chimera): move semantics for ScopedTokenRemover, id-r... ║
    • 16eb8c2a4c  2026-03-12  fix(chimera): address async API review comments (RAII cle... ║
    • 0701ac8f4d  2026-03-12  feat(chimera): implement async/promise-based API (IAsyncD... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_chimera_async_api.cpp
 * @brief Unit tests for IAsyncDatabaseAdapter / ThemisDBAdapter async API
 *
 * @details
 * Validates the Promise/Future-based async operations implemented in
 * ThemisDBAdapter:
 *   - execute_query_async: non-blocking query execution, concurrent fan-out
 *   - batch_insert_async: progress callback invocation, cancellation
 *   - search_vectors_async: concurrent vector searches
 *   - cancel_async: cancellation token lifecycle, NOT_FOUND error
 *   - ASYNC_OPERATIONS capability reported correctly
 *   - Concurrent execution of multiple async operations
 *
 * All tests run without a live ThemisDB server; the adapter uses its
 * built-in in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace chimera;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class AsyncAdapterTest : public ::testing::Test {
protected:
    ThemisDBAdapter adapter;

    void SetUp() override {
        auto r = adapter.connect("themisdb://localhost:7777");
        ASSERT_TRUE(r.is_ok()) << r.error_message;
    }

    static RelationalRow make_row(const std::string& name, int64_t value) {
        RelationalRow row;
        row.columns["name"]  = Scalar{name};
        row.columns["value"] = Scalar{value};
        return row;
    }

    static std::vector<RelationalRow> make_rows(size_t n) {
        std::vector<RelationalRow> rows;
        rows.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            rows.push_back(make_row("r" + std::to_string(i),
                                    static_cast<int64_t>(i)));
        }
        return rows;
    }

    static Vector make_vector(size_t dim, float fill) {
        Vector v;
        v.data.assign(dim, fill);
        return v;
    }
};

// ---------------------------------------------------------------------------
// ASYNC_OPERATIONS capability
// ---------------------------------------------------------------------------

TEST_F(AsyncAdapterTest, HasAsyncOperationsCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::ASYNC_OPERATIONS));

    const auto caps = adapter.get_capabilities();
    EXPECT_NE(std::find(caps.begin(), caps.end(), Capability::ASYNC_OPERATIONS),
              caps.end());
}

// ---------------------------------------------------------------------------
// execute_query_async
// ---------------------------------------------------------------------------

TEST_F(AsyncAdapterTest, ExecuteQueryAsyncReturnsTable) {
    auto future = adapter.execute_query_async("SELECT 1");
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

TEST_F(AsyncAdapterTest, ExecuteQueryAsyncWithParams) {
    auto future = adapter.execute_query_async(
        "SELECT * FROM users WHERE id = ?",
        {Scalar{int64_t{42}}}
    );
    ASSERT_TRUE(future.valid());
    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

TEST_F(AsyncAdapterTest, ExecuteQueryAsyncConcurrentFanOut) {
    // Insert some rows first
    auto rows = make_rows(10);
    auto insert = adapter.batch_insert("concurrent_table", rows);
    ASSERT_TRUE(insert.is_ok());

    // Fan out 5 concurrent queries
    constexpr int kQueries = 5;
    std::vector<std::future<Result<RelationalTable>>> futures;
    futures.reserve(kQueries);
    for (int i = 0; i < kQueries; ++i) {
        futures.push_back(adapter.execute_query_async("SELECT * FROM concurrent_table"));
    }

    int ok_count = 0;
    for (auto& f : futures) {
        ASSERT_TRUE(f.valid());
        auto result = f.get();
        if (result.is_ok()) {
            ++ok_count;
        }
    }
    EXPECT_EQ(ok_count, kQueries);
}

TEST_F(AsyncAdapterTest, ExecuteQueryAsyncWithOperationId) {
    AsyncQueryOptions opts;
    opts.operation_id = "test-query-op-1";

    auto future = adapter.execute_query_async("SELECT 1", {}, opts);
    ASSERT_TRUE(future.valid());
    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

// ---------------------------------------------------------------------------
// batch_insert_async
// ---------------------------------------------------------------------------

TEST_F(AsyncAdapterTest, BatchInsertAsyncBasic) {
    auto rows = make_rows(50);
    auto future = adapter.batch_insert_async("async_table", rows);
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
    EXPECT_EQ(result.value.value_or(0), 50u);
}

TEST_F(AsyncAdapterTest, BatchInsertAsyncEmptyRows) {
    auto future = adapter.batch_insert_async("empty_table", {});
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
    EXPECT_EQ(result.value.value_or(0), 0u);
}

TEST_F(AsyncAdapterTest, BatchInsertAsyncProgressCallback) {
    auto rows = make_rows(1200); // > one chunk (chunk size = 500)

    std::atomic<size_t> callback_count{0};
    std::atomic<size_t> last_processed{0};

    auto future = adapter.batch_insert_async(
        "progress_table",
        rows,
        [&](size_t processed) {
            ++callback_count;
            last_processed.store(processed, std::memory_order_relaxed);
        }
    );
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
    EXPECT_EQ(result.value.value_or(0), 1200u);

    // With 1200 rows and chunk size 500: 3 chunks → 3 callbacks
    EXPECT_EQ(callback_count.load(), 3u);
    EXPECT_EQ(last_processed.load(), 1200u);
}

TEST_F(AsyncAdapterTest, BatchInsertAsyncProgressCallbackExactChunk) {
    auto rows = make_rows(500); // exactly one chunk

    std::atomic<int> callback_count{0};

    auto future = adapter.batch_insert_async(
        "exact_chunk_table",
        rows,
        [&](size_t /*processed*/) { ++callback_count; }
    );
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(callback_count.load(), 1);
}

TEST_F(AsyncAdapterTest, MultipleBatchInsertAsyncConcurrent) {
    constexpr int kConcurrent = 4;
    std::vector<std::future<Result<size_t>>> futures;
    futures.reserve(kConcurrent);

    for (int i = 0; i < kConcurrent; ++i) {
        auto rows = make_rows(20);
        futures.push_back(adapter.batch_insert_async(
            "concurrent_batch_" + std::to_string(i), rows));
    }

    for (auto& f : futures) {
        ASSERT_TRUE(f.valid());
        auto result = f.get();
        EXPECT_TRUE(result.is_ok()) << result.error_message;
        EXPECT_EQ(result.value.value_or(0), 20u);
    }
}

// ---------------------------------------------------------------------------
// search_vectors_async
// ---------------------------------------------------------------------------

TEST_F(AsyncAdapterTest, SearchVectorsAsyncBasic) {
    // Insert some vectors first
    const std::string collection = "async_vec_collection";
    auto idx = adapter.create_index(collection, 4);
    ASSERT_TRUE(idx.is_ok());

    for (int i = 0; i < 5; ++i) {
        adapter.insert_vector(collection, make_vector(4, static_cast<float>(i)));
    }

    auto future = adapter.search_vectors_async(
        collection, make_vector(4, 1.0f), 3);
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
    EXPECT_LE(result.value.value().size(), 3u);
}

TEST_F(AsyncAdapterTest, SearchVectorsAsyncWithFilters) {
    const std::string collection = "async_vec_filtered";
    adapter.create_index(collection, 4);
    adapter.insert_vector(collection, make_vector(4, 0.5f));

    std::map<std::string, Scalar> filters;
    auto future = adapter.search_vectors_async(
        collection, make_vector(4, 0.5f), 2, filters);
    ASSERT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

TEST_F(AsyncAdapterTest, SearchVectorsAsyncConcurrent) {
    const std::string collection = "async_vec_concurrent";
    adapter.create_index(collection, 4);
    for (int i = 0; i < 10; ++i) {
        adapter.insert_vector(collection, make_vector(4, static_cast<float>(i)));
    }

    constexpr int kSearches = 6;
    std::vector<std::future<Result<std::vector<std::pair<Vector, double>>>>> futures;
    futures.reserve(kSearches);
    for (int i = 0; i < kSearches; ++i) {
        futures.push_back(adapter.search_vectors_async(
            collection, make_vector(4, static_cast<float>(i)), 3));
    }

    for (auto& f : futures) {
        ASSERT_TRUE(f.valid());
        auto result = f.get();
        EXPECT_TRUE(result.is_ok()) << result.error_message;
    }
}

TEST_F(AsyncAdapterTest, SearchVectorsAsyncWithOperationId) {
    const std::string collection = "async_vec_op_id";
    adapter.create_index(collection, 4);
    adapter.insert_vector(collection, make_vector(4, 1.0f));

    AsyncQueryOptions opts;
    opts.operation_id = "vec-search-op-1";
    auto future = adapter.search_vectors_async(
        collection, make_vector(4, 1.0f), 1, {}, opts);
    ASSERT_TRUE(future.valid());
    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

// ---------------------------------------------------------------------------
// cancel_async
// ---------------------------------------------------------------------------

TEST_F(AsyncAdapterTest, CancelAsyncEmptyIdReturnsInvalidArgument) {
    auto result = adapter.cancel_async("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(AsyncAdapterTest, CancelAsyncUnknownIdReturnsNotFound) {
    auto result = adapter.cancel_async("nonexistent-op-id");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(AsyncAdapterTest, CancelAsyncTokenCleanedUpAfterCompletion) {
    // Launch a fast operation with an ID, let it complete, then try to cancel.
    AsyncQueryOptions opts;
    opts.operation_id = "completed-op";

    auto future = adapter.execute_query_async("SELECT 1", {}, opts);
    future.get(); // wait for completion

    // After completion the token should be gone → NOT_FOUND
    auto cancel_result = adapter.cancel_async("completed-op");
    EXPECT_TRUE(cancel_result.is_err());
    EXPECT_EQ(cancel_result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(AsyncAdapterTest, DuplicateOperationIdReturnsAlreadyExists) {
    // Launching two operations with the same operation_id while the first is
    // still in flight must be rejected with ALREADY_EXISTS for the second.
    // Use a callback gate to deterministically keep the first operation active.
    auto rows = make_rows(2000);
    AsyncQueryOptions opts;
    opts.operation_id = "dup-op-id";

    std::promise<void> first_chunk_seen;
    auto first_chunk_seen_fut = first_chunk_seen.get_future();
    std::promise<void> release_first;
    auto release_first_fut = release_first.get_future().share();
    std::atomic<bool> gate_entered{false};

    auto first = adapter.batch_insert_async(
        "dup_table_1",
        rows,
        [&](size_t /*processed*/) {
            bool expected = false;
            if (gate_entered.compare_exchange_strong(expected, true)) {
                first_chunk_seen.set_value();
                release_first_fut.wait();
            }
        },
        opts);
    ASSERT_TRUE(first.valid());

    ASSERT_EQ(first_chunk_seen_fut.wait_for(std::chrono::seconds(2)),
              std::future_status::ready)
        << "First async batch did not reach callback gate in time";

    // Attempt to launch a second operation with the same id immediately.
    auto second = adapter.batch_insert_async("dup_table_2", rows, nullptr, opts);
    ASSERT_TRUE(second.valid());

    // The second future must resolve with ALREADY_EXISTS without waiting on
    // the first.
    auto second_result = second.get();
    EXPECT_TRUE(second_result.is_err());
    EXPECT_EQ(second_result.error_code, ErrorCode::ALREADY_EXISTS);

    release_first.set_value();

    // The first must still succeed.
    auto first_result = first.get();
    EXPECT_TRUE(first_result.is_ok()) << first_result.error_message;

    // After the first completes its token is removed by ScopedTokenRemover;
    // a third operation with the same id must therefore succeed.
    auto rows_small = make_rows(5);
    auto reuse = adapter.batch_insert_async("dup_table_reuse", rows_small, nullptr, opts);
    ASSERT_TRUE(reuse.valid());
    auto reuse_result = reuse.get();
    EXPECT_TRUE(reuse_result.is_ok()) << reuse_result.error_message;
}

TEST_F(AsyncAdapterTest, DuplicateOperationIdQueryReturnsAlreadyExists) {
    // Same check for execute_query_async.
    // Keep the first operation deterministically in-flight so the duplicate-id
    // rejection does not depend on scheduler timing.
    AsyncQueryOptions opts;
    opts.operation_id = "dup-query-op";

    auto rows = make_rows(2000);
    std::promise<void> first_chunk_seen;
    auto first_chunk_seen_fut = first_chunk_seen.get_future();
    std::promise<void> release_first;
    auto release_first_fut = release_first.get_future().share();
    std::atomic<bool> gate_entered{false};

    // Keep a batch running to hold the token.
    auto busy = adapter.batch_insert_async(
        "dup_busy_table",
        rows,
        [&](size_t /*processed*/) {
            bool expected = false;
            if (gate_entered.compare_exchange_strong(expected, true)) {
                first_chunk_seen.set_value();
                release_first_fut.wait();
            }
        },
        opts);
    ASSERT_TRUE(busy.valid());

    ASSERT_EQ(first_chunk_seen_fut.wait_for(std::chrono::seconds(2)),
              std::future_status::ready)
        << "First async batch did not reach callback gate in time";

    // A second async query with the same id should be rejected immediately.
    auto dup = adapter.execute_query_async("SELECT 1", {}, opts);
    ASSERT_TRUE(dup.valid());
    auto dup_result = dup.get();
    EXPECT_TRUE(dup_result.is_err());
    EXPECT_EQ(dup_result.error_code, ErrorCode::ALREADY_EXISTS);

    // Wait for the first to finish so the token is released.
    release_first.set_value();
    busy.get();

    // Now the id must be available again.
    auto reuse = adapter.execute_query_async("SELECT 1", {}, opts);
    ASSERT_TRUE(reuse.valid());
    auto reuse_result = reuse.get();
    EXPECT_TRUE(reuse_result.is_ok()) << reuse_result.error_message;
}

// ---------------------------------------------------------------------------
// IAsyncDatabaseAdapter pointer-based usage
// ---------------------------------------------------------------------------

TEST_F(AsyncAdapterTest, UsableViaInterfacePointer) {
    // Verify ThemisDBAdapter can be used through IAsyncDatabaseAdapter*.
    IAsyncDatabaseAdapter* async_iface = &adapter;

    auto future = async_iface->execute_query_async("SELECT 1");
    ASSERT_TRUE(future.valid());
    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

TEST_F(AsyncAdapterTest, BatchInsertAsyncViaInterfacePointer) {
    IAsyncDatabaseAdapter* async_iface = &adapter;

    auto rows = make_rows(10);
    auto future = async_iface->batch_insert_async("iface_table", rows);
    ASSERT_TRUE(future.valid());
    auto result = future.get();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
    EXPECT_EQ(result.value.value_or(0), 10u);
}

// ---------------------------------------------------------------------------
// capability_to_string round-trip
// ---------------------------------------------------------------------------

TEST(CapabilityStringTest, AsyncOperationsStringRoundTrip) {
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::ASYNC_OPERATIONS),
              "ASYNC_OPERATIONS");
}

TEST(CapabilityStringTest, AllCapabilitiesIncludesAsync) {
    const auto all = AdapterCapabilityMatrix::all_capabilities();
    EXPECT_NE(std::find(all.begin(), all.end(), Capability::ASYNC_OPERATIONS),
              all.end());
}
