/**
 * @file test_chimera_streaming.cpp
 * @brief Tests for IResultStream / IStreamingAdapter (v1.9.0)
 *
 * @details
 * Validates the streaming result-set interface and its ThemisDB reference
 * implementation.  Tests cover:
 *   - STREAMING_RESULTS capability is reported
 *   - Dynamic cast to IStreamingAdapter succeeds
 *   - execute_query_stream on an empty table returns a closed-immediately cursor
 *   - has_more() / next_batch() / position() / total_size() contract
 *   - next_batch() with explicit batch_size returns the requested number of rows
 *   - Consuming all rows via batches yields the same row count as execute_query
 *   - next_batch() returns an empty vector when the cursor is exhausted
 *   - close() succeeds; next_batch() after close() returns INTERNAL_ERROR
 *   - has_more() returns false after close()
 *   - set_stream_config() changes the default batch size
 *   - Explicit batch_size overrides the configured default
 *   - Streaming via IDatabaseAdapter base pointer
 *
 * All tests run without a live ThemisDB server (simulation mode).
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/themisdb_adapter.hpp"

#include <algorithm>
#include <string>
#include <vector>

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

/// Insert n rows into `table` through the adapter and assert success.
static void populate_table(
    IDatabaseAdapter& adapter,
    const std::string& table,
    size_t n
) {
    for (size_t i = 0; i < n; ++i) {
        ASSERT_TRUE(adapter.insert_row(
            table,
            make_row("r" + std::to_string(i), static_cast<int64_t>(i))
        ).is_ok());
    }
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class ChimeraStreamingTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(
            adapter_.connect("themisdb://localhost:7777/testdb").is_ok());
    }

    ThemisDBAdapter adapter_;
};

// ---------------------------------------------------------------------------
// Capability detection
// ---------------------------------------------------------------------------

TEST_F(ChimeraStreamingTest, HasStreamingResultsCapability) {
    EXPECT_TRUE(adapter_.has_capability(Capability::STREAMING_RESULTS));
}

TEST_F(ChimeraStreamingTest, GetCapabilitiesIncludesStreamingResults) {
    const auto caps = adapter_.get_capabilities();
    EXPECT_NE(std::find(caps.begin(), caps.end(),
                        Capability::STREAMING_RESULTS),
              caps.end());
}

TEST_F(ChimeraStreamingTest, DynamicCastToIStreamingAdapterSucceeds) {
    auto* sa = dynamic_cast<IStreamingAdapter*>(&adapter_);
    EXPECT_NE(sa, nullptr);
}

// ---------------------------------------------------------------------------
// Basic streaming contract
// ---------------------------------------------------------------------------

TEST_F(ChimeraStreamingTest, StreamEmptyTable) {
    auto qr = adapter_.execute_query_stream("FOR r IN stream_empty RETURN r");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    EXPECT_FALSE(stream->has_more());
    EXPECT_EQ(stream->position(), 0u);
    EXPECT_EQ(stream->total_size().value_or(999u), 0u);
    EXPECT_TRUE(stream->close().is_ok());
}

TEST_F(ChimeraStreamingTest, StreamSingleBatch) {
    constexpr size_t kRows = 5;
    populate_table(adapter_, "sb_table", kRows);

    auto qr = adapter_.execute_query_stream("SELECT * FROM sb_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    EXPECT_TRUE(stream->has_more());
    EXPECT_EQ(stream->total_size().value_or(0u), kRows);

    auto batch = stream->next_batch(kRows + 10); // request more than available
    ASSERT_TRUE(batch.is_ok());
    EXPECT_EQ(batch.value->size(), kRows);
    EXPECT_EQ(stream->position(), kRows);

    EXPECT_FALSE(stream->has_more());
    EXPECT_TRUE(stream->close().is_ok());
}

TEST_F(ChimeraStreamingTest, StreamMultipleBatches) {
    constexpr size_t kRows = 10;
    populate_table(adapter_, "mb_table", kRows);

    auto qr = adapter_.execute_query_stream("SELECT * FROM mb_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    size_t total_fetched = 0;
    while (stream->has_more()) {
        auto batch = stream->next_batch(3);
        ASSERT_TRUE(batch.is_ok());
        total_fetched += batch.value->size();
    }
    EXPECT_EQ(total_fetched, kRows);
    EXPECT_EQ(stream->position(), kRows);
    EXPECT_TRUE(stream->close().is_ok());
}

// ---------------------------------------------------------------------------
// IResultStream invariants
// ---------------------------------------------------------------------------

TEST_F(ChimeraStreamingTest, PositionStartsAtZero) {
    auto qr = adapter_.execute_query_stream("FOR r IN pos_test RETURN r");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());
    EXPECT_EQ(stream->position(), 0u);
    EXPECT_TRUE(stream->close().is_ok());
}

TEST_F(ChimeraStreamingTest, PositionAdvancesWithNextBatch) {
    constexpr size_t kRows = 6;
    populate_table(adapter_, "adv_table", kRows);

    auto qr = adapter_.execute_query_stream("SELECT * FROM adv_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    auto b1 = stream->next_batch(2);
    ASSERT_TRUE(b1.is_ok());
    EXPECT_EQ(b1.value->size(), 2u);
    EXPECT_EQ(stream->position(), 2u);

    auto b2 = stream->next_batch(2);
    ASSERT_TRUE(b2.is_ok());
    EXPECT_EQ(b2.value->size(), 2u);
    EXPECT_EQ(stream->position(), 4u);

    stream->close();
}

TEST_F(ChimeraStreamingTest, NextBatchAfterExhaustionReturnsEmpty) {
    constexpr size_t kRows = 3;
    populate_table(adapter_, "exh_table", kRows);

    auto qr = adapter_.execute_query_stream("SELECT * FROM exh_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    // Consume all rows
    while (stream->has_more()) {
        auto b = stream->next_batch(100);
        ASSERT_TRUE(b.is_ok());
    }
    EXPECT_FALSE(stream->has_more());

    // next_batch on exhausted (not closed) stream returns empty vector
    auto empty_batch = stream->next_batch(10);
    ASSERT_TRUE(empty_batch.is_ok());
    EXPECT_TRUE(empty_batch.value->empty());

    stream->close();
}

TEST_F(ChimeraStreamingTest, CloseIsIdempotent) {
    auto qr = adapter_.execute_query_stream("FOR r IN idem RETURN r");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());
    EXPECT_TRUE(stream->close().is_ok());
    EXPECT_TRUE(stream->close().is_ok()); // second close must not crash
}

TEST_F(ChimeraStreamingTest, NextBatchAfterCloseReturnsError) {
    auto qr = adapter_.execute_query_stream("FOR r IN closed_test RETURN r");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());
    EXPECT_TRUE(stream->close().is_ok());

    auto batch = stream->next_batch(10);
    EXPECT_FALSE(batch.is_ok());
    EXPECT_EQ(batch.error_code, ErrorCode::INTERNAL_ERROR);
}

TEST_F(ChimeraStreamingTest, HasMoreReturnsFalseAfterClose) {
    constexpr size_t kRows = 4;
    populate_table(adapter_, "hm_table", kRows);

    auto qr = adapter_.execute_query_stream("SELECT * FROM hm_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());
    EXPECT_TRUE(stream->has_more());
    stream->close();
    EXPECT_FALSE(stream->has_more());
}

// ---------------------------------------------------------------------------
// StreamConfig / set_stream_config
// ---------------------------------------------------------------------------

TEST_F(ChimeraStreamingTest, SetStreamConfigChangesDefaultBatchSize) {
    constexpr size_t kRows = 12;
    populate_table(adapter_, "cfg_table", kRows);

    StreamConfig cfg;
    cfg.default_batch_size = 4;
    ASSERT_TRUE(adapter_.set_stream_config(cfg).is_ok());

    auto qr = adapter_.execute_query_stream("SELECT * FROM cfg_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    // batch_size == 0 → uses the configured default (4)
    auto b = stream->next_batch(0);
    ASSERT_TRUE(b.is_ok());
    EXPECT_EQ(b.value->size(), 4u);

    stream->close();
}

TEST_F(ChimeraStreamingTest, ExplicitBatchSizeOverridesDefault) {
    constexpr size_t kRows = 10;
    populate_table(adapter_, "bs_table", kRows);

    StreamConfig cfg;
    cfg.default_batch_size = 100; // large default
    ASSERT_TRUE(adapter_.set_stream_config(cfg).is_ok());

    auto qr = adapter_.execute_query_stream("SELECT * FROM bs_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    // Explicit batch_size = 3 overrides the default 100
    auto b = stream->next_batch(3);
    ASSERT_TRUE(b.is_ok());
    EXPECT_EQ(b.value->size(), 3u);

    stream->close();
}

// ---------------------------------------------------------------------------
// Streaming result matches synchronous result
// ---------------------------------------------------------------------------

TEST_F(ChimeraStreamingTest, StreamResultMatchesSyncResult) {
    constexpr size_t kRows = 7;
    populate_table(adapter_, "match_table", kRows);

    // Synchronous count
    auto sync_res = adapter_.execute_query("SELECT * FROM match_table");
    ASSERT_TRUE(sync_res.is_ok());
    const size_t sync_count = sync_res.value->rows.size();

    // Streaming count
    auto qr = adapter_.execute_query_stream("SELECT * FROM match_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());

    size_t stream_count = 0;
    while (stream->has_more()) {
        auto batch = stream->next_batch(10);
        ASSERT_TRUE(batch.is_ok());
        stream_count += batch.value->size();
    }
    stream->close();

    EXPECT_EQ(stream_count, sync_count);
}

// ---------------------------------------------------------------------------
// Via IDatabaseAdapter base pointer
// ---------------------------------------------------------------------------

TEST_F(ChimeraStreamingTest, StreamingViaBasePointer) {
    populate_table(adapter_, "base_table", 5);

    IDatabaseAdapter* base = &adapter_;
    ASSERT_TRUE(base->has_capability(Capability::STREAMING_RESULTS));

    auto* sa = dynamic_cast<IStreamingAdapter*>(base);
    ASSERT_NE(sa, nullptr);

    auto qr = sa->execute_query_stream("SELECT * FROM base_table");
    ASSERT_TRUE(qr.is_ok());
    auto stream = std::move(qr.value.value());
    EXPECT_EQ(stream->total_size().value_or(0u), 5u);
    stream->close();
}
