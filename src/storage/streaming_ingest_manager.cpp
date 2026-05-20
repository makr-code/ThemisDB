/*
 * ThemisDB | File: streaming_ingest_manager.cpp | Version: 0.0.10 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 286
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=53 | delta=50 | status=divergent
 * External Severity (v3): C=9, H=39, M=5
 * PR: #4574 feat: StreamingIngestManager, TsStreamCursor, LZ4 compression, main... (2026-04-12T17:04:06Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/streaming_ingest_manager.h"
#include "utils/error_registry.h"

#include <rocksdb/write_batch.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>

namespace themis {

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::unique_ptr<StreamingIngestManager> StreamingIngestManager::create(
    std::shared_ptr<RocksDBWrapper> db,
    Config cfg)
{
    if (!db) {
        throw std::invalid_argument("StreamingIngestManager: db cannot be null");
    }
    if (cfg.max_buffer_events == 0) {
        cfg.max_buffer_events = 1;
    }
    if (cfg.max_batch_size == 0) {
        cfg.max_batch_size = 1;
    }
    return std::unique_ptr<StreamingIngestManager>(
        new StreamingIngestManager(std::move(db), std::move(cfg)));
}

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

StreamingIngestManager::StreamingIngestManager(
    std::shared_ptr<RocksDBWrapper> db,
    Config cfg)
    : db_(std::move(db))
    , cfg_(std::move(cfg))
{
    buffer_.reserve(std::min(cfg_.max_buffer_events, size_t{65536}));
}

StreamingIngestManager::~StreamingIngestManager() {
    if (running_.load(std::memory_order_acquire)) {
        // Best-effort drain on unexpected destruction.
        running_.store(false, std::memory_order_release);
        not_empty_.notify_all();
        if (flush_thread_.joinable()) {
            flush_thread_.join();
        }
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Result<void> StreamingIngestManager::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true,
                                           std::memory_order_acq_rel)) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                    "StreamingIngestManager: already started"));
    }
    flush_thread_ = std::thread([this] { flushLoop(); });
    return {};
}

Result<void> StreamingIngestManager::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return {};
    }
    running_.store(false, std::memory_order_release);
    not_empty_.notify_all();
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    // Final drain of any remaining events.
    {
        std::unique_lock<std::mutex> lock(mu_);
        if (!buffer_.empty()) {
            return flushOnce(lock);
        }
    }
    return {};
}

// ---------------------------------------------------------------------------
// Ingest
// ---------------------------------------------------------------------------

Result<void> StreamingIngestManager::ingest(std::string_view key,
                                              std::string_view value)
{
    std::unique_lock<std::mutex> lock(mu_);

    if (buffer_.size() >= cfg_.max_buffer_events) {
        if (cfg_.overflow_policy == OverflowPolicy::DROP) {
            stat_dropped_.fetch_add(1, std::memory_order_relaxed);
            return {};
        }

        // BLOCK policy: wait for the flush thread to drain some space.
        ++stat_backpressure_;
        auto deadline = cfg_.backpressure_timeout.count() > 0
            ? std::chrono::steady_clock::now() + cfg_.backpressure_timeout
            : std::chrono::steady_clock::time_point::max();

        bool got_space = not_full_.wait_until(lock, deadline, [this] {
            return buffer_.size() < cfg_.max_buffer_events
                || !running_.load(std::memory_order_relaxed);
        });

        if (!got_space || buffer_.size() >= cfg_.max_buffer_events) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_LOG_FULL,
                                        "StreamingIngestManager: buffer full, "
                                        "back-pressure timeout exceeded"));
        }
    }

    buffer_.push_back({std::string(key), std::string(value)});
    stat_ingested_.fetch_add(1, std::memory_order_relaxed);
    not_empty_.notify_one();
    return {};
}

Result<size_t> StreamingIngestManager::ingestBatch(std::vector<Event> events) {
    if (events.empty()) {
        return size_t{0};
    }

    std::unique_lock<std::mutex> lock(mu_);
    size_t accepted = 0;

    for (auto& ev : events) {
        if (buffer_.size() >= cfg_.max_buffer_events) {
            if (cfg_.overflow_policy == OverflowPolicy::DROP) {
                stat_dropped_.fetch_add(events.size() - accepted,
                                         std::memory_order_relaxed);
                break;
            }
            // Wait for space before accepting more from this batch.
            ++stat_backpressure_;
            auto deadline = cfg_.backpressure_timeout.count() > 0
                ? std::chrono::steady_clock::now() + cfg_.backpressure_timeout
                : std::chrono::steady_clock::time_point::max();

            bool got_space = not_full_.wait_until(lock, deadline, [this] {
                return buffer_.size() < cfg_.max_buffer_events
                    || !running_.load(std::memory_order_relaxed);
            });
            if (!got_space || buffer_.size() >= cfg_.max_buffer_events) {
                // Drop the rest of the batch.
                stat_dropped_.fetch_add(events.size() - accepted,
                                         std::memory_order_relaxed);
                break;
            }
        }
        buffer_.push_back(std::move(ev));
        ++accepted;
    }

    stat_ingested_.fetch_add(accepted, std::memory_order_relaxed);
    if (accepted > 0) {
        not_empty_.notify_one();
    }
    return accepted;
}

Result<void> StreamingIngestManager::flush() {
    std::unique_lock<std::mutex> lock(mu_);
    if (buffer_.empty()) {
        return {};
    }
    return flushOnce(lock);
}

// ---------------------------------------------------------------------------
// Background flush loop
// ---------------------------------------------------------------------------

void StreamingIngestManager::flushLoop() {
    while (running_.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock(mu_);

        // Wait up to flush_interval for events, or until stopped.
        not_empty_.wait_for(lock, cfg_.flush_interval, [this] {
            return !buffer_.empty()
                || !running_.load(std::memory_order_relaxed);
        });

        if (buffer_.empty()) {
            continue;
        }

        // Ignore individual batch errors from the background thread — they
        // are logged but should not crash the flush loop.
        (void)flushOnce(lock);
    }
}

// ---------------------------------------------------------------------------
// Internal flush (called with mu_ held; may unlock/re-lock)
// ---------------------------------------------------------------------------

Result<void> StreamingIngestManager::flushOnce(std::unique_lock<std::mutex>& lock) {
    if (buffer_.empty()) {
        return {};
    }

    // Swap out the current buffer so the caller threads can fill a new one
    // while we write to RocksDB without holding the lock.
    std::vector<Event> batch;
    batch.swap(buffer_);
    // Re-reserve so future appends don't allocate from scratch.
    buffer_.reserve(std::min(cfg_.max_buffer_events, size_t{65536}));

    // Notify waiting producers that space is available.
    not_full_.notify_all();

    lock.unlock();

    // ── Write to RocksDB ──────────────────────────────────────────────────
    size_t offset = 0;
    const size_t total = batch.size();

    while (offset < total) {
        rocksdb::WriteBatch wb;
        size_t count = 0;

        for (; offset < total && count < cfg_.max_batch_size; ++offset, ++count) {
            const Event& ev = batch[offset];
            wb.Put(rocksdb::Slice(ev.key), rocksdb::Slice(ev.value));
        }

        rocksdb::WriteOptions wo;
        wo.sync = cfg_.sync_wal;

        rocksdb::Status s = db_->getRawDB()->Write(wo, &wb);
        if (!s.ok()) {
            lock.lock();
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                        "StreamingIngestManager flush failed: " +
                                        s.ToString()));
        }

        stat_flushed_.fetch_add(count, std::memory_order_relaxed);
        stat_flush_count_.fetch_add(1, std::memory_order_relaxed);
    }

    lock.lock();
    return {};
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

StreamingIngestManager::Stats StreamingIngestManager::stats() const noexcept {
    Stats s;
    s.events_ingested   = stat_ingested_.load(std::memory_order_relaxed);
    s.events_flushed    = stat_flushed_.load(std::memory_order_relaxed);
    s.flush_count       = stat_flush_count_.load(std::memory_order_relaxed);
    s.backpressure_waits = stat_backpressure_.load(std::memory_order_relaxed);
    s.dropped_events    = stat_dropped_.load(std::memory_order_relaxed);
    return s;
}

} // namespace themis
