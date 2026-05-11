/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ts_stream_cursor.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:47:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     190                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 040083b025  2026-04-12  feat: StreamingIngestManager, TsStreamCursor, LZ4 compres... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "timeseries/tsstore.h"
#include "utils/expected.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include &lt;optional&gt;
#include <string>
#include <vector>

namespace themis {
namespace timeseries {

/**
 * @file ts_stream_cursor.h
 * @brief Lazy streaming cursor for large TSStore result sets.
 *
 * TsStreamCursor wraps a TSStore::QueryOptions query and exposes a lazy,
 * paginated row-iterator that avoids materialising the entire result in
 * memory.  Results are fetched in page-sized chunks on demand, providing
 * natural back-pressure: callers control the scan rate simply by calling
 * next() at their own pace.
 *
 * ## Design constraints
 * - Zero-copy: internal pages are reused between fetches.
 * - Caller owns result memory only for the duration of the current page.
 * - The cursor is invalidated if the underlying TSStore is destroyed.
 * - Concurrent chunk rotation may render a cursor stale; in that case
 *   next() returns a CursorInvalidated error.
 *
 * ## Usage
 * @code
 * TSStore::QueryOptions opts;
 * opts.metric = "cpu_usage";
 * opts.from_timestamp_ms = start;
 * opts.to_timestamp_ms   = end;
 *
 * auto cursor = TsStreamCursor::open(store, opts);
 * while (cursor->valid()) {
 *     const TSStore::DataPoint& dp = cursor->current();
 *     process(dp);
 *     if (auto err = cursor->advance(); !err) {
 *         handleError(err.error());
 *         break;
 *     }
 * }
 * @endcode
 *
 * ## Performance target
 * ≥ 500 MB/s sustained scan throughput on NVMe storage (see ROADMAP).
 */
class TsStreamCursor {
public:
    /** Page size: number of DataPoints fetched per backend round-trip. */
    static constexpr size_t kDefaultPageSize = 4096;

    /**
     * @brief Configuration for a cursor scan.
     */
    struct Config {
        /** How many DataPoints to fetch per internal page request. */
        size_t page_size = kDefaultPageSize;
    };

    // ── Factory ────────────────────────────────────────────────────────────

    /**
     * @brief Open a streaming cursor over @p store.
     *
     * The first page is fetched eagerly during open() so that valid() is
     * immediately usable.
     *
     * @param store   The TSStore to scan (must outlive the cursor).
     * @param options Query filter (metric, time range, entity, tags).
     * @param cfg     Tuning parameters (page size).
     * @return A ready cursor, or an error if the initial fetch failed.
     */
    static Result<std::unique_ptr<TsStreamCursor>> open(
        TSStore& store,
      TSStore::QueryOptions options);

    static Result<std::unique_ptr<TsStreamCursor>> open(
      TSStore& store,
      TSStore::QueryOptions options,
      Config cfg);

    // ── Iterator interface ─────────────────────────────────────────────────

    /**
     * @brief Return true while the cursor has at least one more DataPoint.
     *
     * Returns false once the last DataPoint has been consumed or after
     * close() has been called.
     */
    bool valid() const noexcept;

    /**
     * @brief Reference to the current DataPoint.
     *
     * @pre valid() == true.  Behaviour is undefined otherwise.
     *
     * The reference is valid until the next advance() call that crosses a
     * page boundary (i.e. until the internal page buffer is refilled).
     * Copy the DataPoint if you need to retain it across advance() calls.
     */
    const TSStore::DataPoint& current() const noexcept;

    /**
     * @brief Move to the next DataPoint.
     *
     * Fetches the next page from the store when the current page is
     * exhausted.  Calling advance() when valid() == false is a no-op and
     * returns success.
     *
     * @return Success, or an error (e.g. storage I/O failure, cursor
     *         invalidated by concurrent schema change).
     */
    Result<void> advance();

    /**
     * @brief Release all resources and mark the cursor as exhausted.
     *
     * After close(), valid() returns false.  The cursor may not be re-opened.
     */
    void close() noexcept;

    // ── Observability ─────────────────────────────────────────────────────

    /** Total number of DataPoints returned by current() so far. */
    uint64_t rowsConsumed() const noexcept;

    /** Total number of backend fetch operations issued. */
    uint64_t pagesFetched() const noexcept;

    ~TsStreamCursor();

    // Non-copyable; movable.
    TsStreamCursor(const TsStreamCursor&)            = delete;
    TsStreamCursor& operator=(const TsStreamCursor&) = delete;
    TsStreamCursor(TsStreamCursor&&)                 = default;
    TsStreamCursor& operator=(TsStreamCursor&&)      = default;

private:
    explicit TsStreamCursor(TSStore& store,
                             TSStore::QueryOptions options,
                             Config cfg);

    Result<void> fetchNextPage();

    TSStore*                        store_;
    TSStore::QueryOptions           options_;
    Config                          cfg_;

    // Current page buffer and position within it
    std::vector<TSStore::DataPoint> page_;
    size_t                          page_pos_{0};

    // Timestamp of the last DataPoint consumed; used to paginate
    int64_t                         last_timestamp_ms_{-1};

    bool                            exhausted_{false};
    uint64_t                        rows_consumed_{0};
    uint64_t                        pages_fetched_{0};
};

} // namespace timeseries
} // namespace themis
