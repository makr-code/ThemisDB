/**
 * @file ts_stream_cursor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "timeseries/tsstore.h"
#include "utils/expected.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace themis {
namespace timeseries {

/** @brief Ts stream cursor. */
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
     * @return A ready cursor, or an error if the initial fetch failed.
     */
    static Result<std::unique_ptr<TsStreamCursor>> open(
        TSStore& store,
      TSStore::QueryOptions options);

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
    TsStreamCursor(TsStreamCursor&&)                 noexcept = default;
    TsStreamCursor& operator=(TsStreamCursor&&)      noexcept = default;

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
