/**
 * @file ts_stream_cursor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "timeseries/ts_stream_cursor.h"
#include "utils/error_registry.h"
#include "utils/expected.h"

#include <algorithm>
#include <climits>
#include <stdexcept>

namespace themis {
namespace timeseries {

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

Result<std::unique_ptr<TsStreamCursor>> TsStreamCursor::open(
    TSStore& store,
    TSStore::QueryOptions options)
{
    return open(store, std::move(options), Config{});
}

Result<std::unique_ptr<TsStreamCursor>> TsStreamCursor::open(
    TSStore& store,
    TSStore::QueryOptions options,
    Config cfg)
{
    if (cfg.page_size == 0) {
        cfg.page_size = kDefaultPageSize;
    }

    // Use private constructor via new so make_unique can't bypass it.
    auto cursor = std::unique_ptr<TsStreamCursor>(
        new TsStreamCursor(store, std::move(options), cfg));

    // Eagerly fetch first page so valid() is immediately usable.
    if (auto res = cursor->fetchNextPage(); !res) {
        return tl::unexpected(res.error());
    }
    return cursor;
}

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

TsStreamCursor::TsStreamCursor(TSStore& store,
                                 TSStore::QueryOptions options,
                                 Config cfg)
    : store_(&store)
    , options_(std::move(options))
    , cfg_(cfg)
{}

TsStreamCursor::~TsStreamCursor() = default;

// ---------------------------------------------------------------------------
// Core iteration
// ---------------------------------------------------------------------------

bool TsStreamCursor::valid() const noexcept {
    return static_cast<bool>(!exhausted_ && page_pos_  < static_cast<int>(page_.size()));
}

const TSStore::DataPoint& TsStreamCursor::current() const noexcept {
    return page_[page_pos_];
}

Result<void> TsStreamCursor::advance() {
    if (exhausted_) {
        return {};
    }

    ++page_pos_;
    ++rows_consumed_;

    if (static_cast<int>(page_.size()) > page_pos_) {
        // Still within the current page.
        return {};
    }

    // Page exhausted — fetch the next one.
    return fetchNextPage();
}

void TsStreamCursor::close() noexcept {
    exhausted_ = true;
    page_.clear();
    page_pos_ = 0;
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

uint64_t TsStreamCursor::rowsConsumed() const noexcept {
    return rows_consumed_;
}

uint64_t TsStreamCursor::pagesFetched() const noexcept {
    return pages_fetched_;
}

// ---------------------------------------------------------------------------
// Internal: paginated fetch
// ---------------------------------------------------------------------------

Result<void> TsStreamCursor::fetchNextPage() {
    if (!store_) {
        exhausted_ = true;
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                    "TsStreamCursor: underlying TSStore was destroyed"));
    }

    // Build a page-sized query starting just after the last consumed timestamp.
    TSStore::QueryOptions page_opts = options_;
    page_opts.limit = cfg_.page_size;

    if (last_timestamp_ms_ >= 0) {
        // Advance the window by 1 ms to avoid re-delivering the last point.
        page_opts.from_timestamp_ms = last_timestamp_ms_ + 1;
    }

    // Bail out early if the window is already empty.
    if (page_opts.from_timestamp_ms > page_opts.to_timestamp_ms) {
        exhausted_ = true;
        page_.clear();
        page_pos_ = 0;
        return {};
    }

    auto result = store_->query(page_opts);
    if (!result) {
        exhausted_ = true;
        return tl::unexpected(result.error());
    }

    ++pages_fetched_;
    page_     = std::move(*result);
    page_pos_ = 0;

    if (page_.empty()) {
        exhausted_ = true;
        return {};
    }

    // Track the timestamp of the last point in this page for the next fetch.
    last_timestamp_ms_ = page_.back().timestamp_ms;

    // If the page returned fewer rows than requested, the scan is complete.
    if (page_.size() < cfg_.page_size) {
        // We'll exhaust naturally — no need to set exhausted_ here; once
        // advance() goes past page_.size() and fetchNextPage() returns an
        // empty page, exhausted_ will be set above.
    }

    return {};
}

} // namespace timeseries
} // namespace themis
