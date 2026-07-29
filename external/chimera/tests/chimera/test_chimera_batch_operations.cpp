/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_chimera_batch_operations.cpp                  ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:22:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     489                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 828426fff0  2026-03-12  fix(chimera): address batch operations code review feedback ║
    • f799e001ef  2026-03-12  feat(chimera): implement Batch Operation Enhancements (v1... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_chimera_batch_operations.cpp
 * @brief Tests for advanced batch operations (BatchOptions / BatchResult)
 *
 * @details
 * Validates the batch_insert_advanced() and batch_insert_documents_advanced()
 * default implementations provided by IRelationalAdapter and IDocumentAdapter
 * respectively.  Tests exercise:
 *   - Basic happy-path chunking and result aggregation
 *   - Progress callback invocation
 *   - Per-chunk batch callback invocation
 *   - stop_on_error semantics
 *   - Empty input edge cases
 *   - Batch size of 1 and batch size larger than the row count
 *   - BatchResult field correctness (total_processed, successful, failed,
 *     total_time, batch_results)
 *   - Document batch variants
 *
 * All tests run without a live database server; the adapters use their
 * built-in in-process simulation modes.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/postgresql_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

#include <atomic>
#include <chrono>

using namespace chimera;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static RelationalRow make_row(const std::string& name, int64_t value) {
    RelationalRow row;
    row.columns["name"]  = Scalar{name};
    row.columns["value"] = Scalar{value};
    return row;
}

static Document make_doc(const std::string& id, const std::string& name) {
    Document doc;
    doc.id = id;
    doc.fields["name"] = Scalar{name};
    return doc;
}

static std::vector<RelationalRow> make_rows(size_t n) {
    std::vector<RelationalRow> rows;
    rows.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        rows.push_back(make_row("r" + std::to_string(i), static_cast<int64_t>(i)));
    }
    return rows;
}

static std::vector<Document> make_docs(size_t n) {
    std::vector<Document> docs;
    docs.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        docs.push_back(make_doc("id_" + std::to_string(i), "doc" + std::to_string(i)));
    }
    return docs;
}

// ---------------------------------------------------------------------------
// Fixture – PostgreSQLAdapter (in-process simulation)
// ---------------------------------------------------------------------------

class BatchOperationsPostgresTest : public ::testing::Test {
protected:
    static constexpr const char* kTable      = "batch_test_table";
    static constexpr const char* kCollection = "batch_test_docs";

    void SetUp() override {
        ASSERT_TRUE(adapter.connect("postgresql://localhost:5432/testdb").is_ok());
    }

    PostgreSQLAdapter adapter;
};

// ---------------------------------------------------------------------------
// batch_insert_advanced – relational
// ---------------------------------------------------------------------------

TEST_F(BatchOperationsPostgresTest, BasicBatchAdvancedSucceeds) {
    auto rows   = make_rows(10);
    auto result = adapter.batch_insert_advanced(kTable, rows);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 10u);
    EXPECT_EQ(br.successful,      10u);
    EXPECT_EQ(br.failed,           0u);
    EXPECT_EQ(br.batch_results.size(), 1u); // single chunk (batch_size=1000 > 10)
}

TEST_F(BatchOperationsPostgresTest, BatchAdvancedEmpty) {
    auto result = adapter.batch_insert_advanced(kTable, {});
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 0u);
    EXPECT_EQ(br.successful,      0u);
    EXPECT_EQ(br.failed,          0u);
    EXPECT_TRUE(br.batch_results.empty());
}

TEST_F(BatchOperationsPostgresTest, BatchAdvancedChunksProducesMultipleBatchResults) {
    auto rows = make_rows(10);

    BatchOptions opts;
    opts.batch_size = 3; // yields ceil(10/3) = 4 chunks

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 10u);
    EXPECT_EQ(br.successful,      10u);
    EXPECT_EQ(br.failed,           0u);
    EXPECT_EQ(br.batch_results.size(), 4u);
}

TEST_F(BatchOperationsPostgresTest, BatchAdvancedBatchSizeOne) {
    const size_t N = 5;
    auto rows = make_rows(N);

    BatchOptions opts;
    opts.batch_size = 1;

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, N);
    EXPECT_EQ(br.successful,      N);
    EXPECT_EQ(br.batch_results.size(), N);
}

TEST_F(BatchOperationsPostgresTest, BatchAdvancedBatchSizeLargerThanRows) {
    auto rows = make_rows(3);

    BatchOptions opts;
    opts.batch_size = 1000;

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 3u);
    EXPECT_EQ(br.batch_results.size(), 1u);
}

TEST_F(BatchOperationsPostgresTest, ProgressCallbackIsInvokedForEachChunk) {
    const size_t N = 7;
    auto rows = make_rows(N);

    std::vector<std::pair<size_t,size_t>> progress_calls;
    BatchOptions opts;
    opts.batch_size = 3;
    opts.progress_callback = [&](size_t processed, size_t total) {
        progress_calls.emplace_back(processed, total);
    };

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    // With batch_size=3 and N=7 we have 3 chunks: 3+3+1
    // progress_callback fires once per chunk, reporting cumulative processed items
    ASSERT_EQ(progress_calls.size(), 3u);
    EXPECT_EQ(progress_calls[0].first, 3u);
    EXPECT_EQ(progress_calls[0].second, N);
    EXPECT_EQ(progress_calls[1].first, 6u);
    EXPECT_EQ(progress_calls[2].first, 7u);
    EXPECT_EQ(progress_calls[2].second, N);
}

TEST_F(BatchOperationsPostgresTest, BatchCallbackIsInvokedForEachChunk) {
    auto rows = make_rows(9);

    std::vector<size_t> batch_indices;
    std::vector<bool>   batch_ok_flags;

    BatchOptions opts;
    opts.batch_size = 4; // 4+4+1 = 3 chunks
    opts.batch_callback = [&](size_t idx, const Result<size_t>& res) {
        batch_indices.push_back(idx);
        batch_ok_flags.push_back(res.is_ok());
    };

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    ASSERT_EQ(batch_indices.size(), 3u);
    EXPECT_EQ(batch_indices[0], 0u);
    EXPECT_EQ(batch_indices[1], 1u);
    EXPECT_EQ(batch_indices[2], 2u);
    for (bool ok : batch_ok_flags) {
        EXPECT_TRUE(ok);
    }
}

TEST_F(BatchOperationsPostgresTest, TotalTimeIsRecorded) {
    auto rows   = make_rows(5);
    auto result = adapter.batch_insert_advanced(kTable, rows);
    ASSERT_TRUE(result.is_ok());
    // total_time should be a non-negative duration (may be 0 in fast simulation)
    EXPECT_GE(result.value.value().total_time.count(), 0);
}

// ---------------------------------------------------------------------------
// batch_insert_advanced – connection error propagation
// ---------------------------------------------------------------------------

TEST_F(BatchOperationsPostgresTest, BatchAdvancedWhileDisconnected) {
    adapter.disconnect();
    auto rows = make_rows(5);

    BatchOptions opts;
    opts.stop_on_error = true;

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok()); // batch_insert_advanced itself succeeds ...

    const auto& br = result.value.value();
    EXPECT_GT(br.failed, 0u);   // ... but the chunk failed
    ASSERT_GE(br.batch_results.size(), 1u);
    EXPECT_TRUE(br.batch_results[0].is_err());
    EXPECT_EQ(br.batch_results[0].error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(BatchOperationsPostgresTest, StopOnErrorHaltsAfterFirstChunkFailure) {
    adapter.disconnect();
    auto rows = make_rows(9);

    BatchOptions opts;
    opts.batch_size    = 3;
    opts.stop_on_error = true;

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    // Should stop after the very first chunk
    EXPECT_EQ(br.batch_results.size(), 1u);
}

TEST_F(BatchOperationsPostgresTest, NoStopOnErrorContinuesAfterChunkFailure) {
    adapter.disconnect();
    auto rows = make_rows(9);

    BatchOptions opts;
    opts.batch_size    = 3;
    opts.stop_on_error = false; // default

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    // All 3 chunks are attempted even though every one fails
    EXPECT_EQ(br.batch_results.size(), 3u);
    EXPECT_EQ(br.failed,  9u);
    EXPECT_EQ(br.successful, 0u);
}

TEST_F(BatchOperationsPostgresTest, BatchCallbackIsInvokedForFailingChunks) {
    adapter.disconnect();
    auto rows = make_rows(6);

    std::vector<bool> cb_ok_flags;
    BatchOptions opts;
    opts.batch_size    = 3;
    opts.stop_on_error = false;
    opts.batch_callback = [&](size_t /*idx*/, const Result<size_t>& r) {
        cb_ok_flags.push_back(r.is_ok());
    };

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    // Both chunks fail, but the callback must still be invoked for each
    ASSERT_EQ(cb_ok_flags.size(), 2u);
    EXPECT_FALSE(cb_ok_flags[0]);
    EXPECT_FALSE(cb_ok_flags[1]);
}

TEST_F(BatchOperationsPostgresTest, CallbacksFiredBeforeStopOnErrorBreaks) {
    adapter.disconnect();
    auto rows = make_rows(9);

    size_t progress_fires = 0;
    size_t batch_fires    = 0;

    BatchOptions opts;
    opts.batch_size        = 3;
    opts.stop_on_error     = true;
    opts.progress_callback = [&](size_t, size_t) { ++progress_fires; };
    opts.batch_callback    = [&](size_t, const Result<size_t>&) { ++batch_fires; };

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    // Exactly one chunk attempted; both callbacks must fire for that chunk
    EXPECT_EQ(result.value.value().batch_results.size(), 1u);
    EXPECT_EQ(progress_fires, 1u);
    EXPECT_EQ(batch_fires,    1u);
}

TEST_F(BatchOperationsPostgresTest, BatchSizeZeroTreatedAsSingleChunk) {
    auto rows = make_rows(7);

    BatchOptions opts;
    opts.batch_size = 0; // should default to all rows in one chunk

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed,     7u);
    EXPECT_EQ(br.batch_results.size(), 1u);
}

// ---------------------------------------------------------------------------
// batch_insert_documents_advanced – document adapter
// ---------------------------------------------------------------------------

class BatchOperationsDocumentTest : public ::testing::Test {
protected:
    static constexpr const char* kColl = "batch_doc_test";

    void SetUp() override {
        ASSERT_TRUE(adapter.connect("postgresql://localhost:5432/testdb").is_ok());
    }

    PostgreSQLAdapter adapter;
};

TEST_F(BatchOperationsDocumentTest, BasicDocumentBatchAdvancedSucceeds) {
    auto docs   = make_docs(8);
    auto result = adapter.batch_insert_documents_advanced(kColl, docs);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 8u);
    EXPECT_EQ(br.successful,      8u);
    EXPECT_EQ(br.failed,          0u);
    EXPECT_EQ(br.batch_results.size(), 1u);
}

TEST_F(BatchOperationsDocumentTest, DocumentBatchChunksCorrectly) {
    auto docs = make_docs(10);

    BatchOptions opts;
    opts.batch_size = 4; // 4+4+2 = 3 chunks

    auto result = adapter.batch_insert_documents_advanced(kColl, docs, opts);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 10u);
    EXPECT_EQ(br.successful,      10u);
    EXPECT_EQ(br.batch_results.size(), 3u);
}

TEST_F(BatchOperationsDocumentTest, DocumentProgressCallback) {
    auto docs = make_docs(6);

    size_t last_progress = 0;
    size_t call_count    = 0;

    BatchOptions opts;
    opts.batch_size = 2;
    opts.progress_callback = [&](size_t processed, size_t /*total*/) {
        last_progress = processed;
        ++call_count;
    };

    auto result = adapter.batch_insert_documents_advanced(kColl, docs, opts);
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(call_count,    3u);
    EXPECT_EQ(last_progress, 6u);
}

TEST_F(BatchOperationsDocumentTest, DocumentStopOnError) {
    adapter.disconnect();
    auto docs = make_docs(9);

    BatchOptions opts;
    opts.batch_size    = 3;
    opts.stop_on_error = true;

    auto result = adapter.batch_insert_documents_advanced(kColl, docs, opts);
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(result.value.value().batch_results.size(), 1u);
}

// ---------------------------------------------------------------------------
// ThemisDBAdapter – same default implementation via the base interface
// ---------------------------------------------------------------------------

class BatchOperationsThemisDBTest : public ::testing::Test {
protected:
    static constexpr const char* kTable = "themis_batch_table";

    void SetUp() override {
        ASSERT_TRUE(adapter.connect("themisdb://localhost:7474/testdb").is_ok());
    }

    ThemisDBAdapter adapter;
};

TEST_F(BatchOperationsThemisDBTest, BasicBatchAdvanced) {
    auto rows   = make_rows(5);
    auto result = adapter.batch_insert_advanced(kTable, rows);
    ASSERT_TRUE(result.is_ok());

    const auto& br = result.value.value();
    EXPECT_EQ(br.total_processed, 5u);
    EXPECT_EQ(br.failed,          0u);
}

TEST_F(BatchOperationsThemisDBTest, ProgressCallbackFires) {
    auto rows = make_rows(4);

    std::atomic<size_t> fires{0};
    BatchOptions opts;
    opts.batch_size        = 2;
    opts.progress_callback = [&](size_t /*p*/, size_t /*t*/) { ++fires; };

    auto result = adapter.batch_insert_advanced(kTable, rows, opts);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(fires.load(), 2u); // 2 chunks of 2
}

// ---------------------------------------------------------------------------
// BatchOptions default values
// ---------------------------------------------------------------------------

TEST(BatchOptionsDefaults, DefaultValues) {
    BatchOptions opts;
    EXPECT_EQ(opts.batch_size, 1000u);
    EXPECT_FALSE(opts.stop_on_error);
    EXPECT_FALSE(opts.progress_callback);
    EXPECT_FALSE(opts.batch_callback);
}

// ---------------------------------------------------------------------------
// BatchResult zero-initialisation
// ---------------------------------------------------------------------------

TEST(BatchResultDefaults, ZeroInit) {
    BatchResult br;
    EXPECT_EQ(br.total_processed, 0u);
    EXPECT_EQ(br.successful,      0u);
    EXPECT_EQ(br.failed,          0u);
    EXPECT_TRUE(br.batch_results.empty());
    EXPECT_EQ(br.total_time.count(), 0);
}
