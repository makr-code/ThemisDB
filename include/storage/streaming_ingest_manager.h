/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            streaming_ingest_manager.h                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-14 18:43:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     252                                            ║
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

#include "storage/rocksdb_wrapper.h"
#include "storage/wal_storage.h"
#include "utils/expected.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace themis {

/**
 * @file streaming_ingest_manager.h
 * @brief High-throughput streaming ingest manager for ThemisDB.
 *
 * StreamingIngestManager accepts a high-velocity stream of key-value events
 * and persists them durably with end-to-end latency ≤ 50 ms at
 * ≥ 1 M events/s on an 8-core node.
 *
 * ## Architecture
 *
 * Events are appended to an in-memory ring buffer by the calling thread(s).
 * A dedicated flush thread drains the ring buffer every `flush_interval`
 * into a single `rocksdb::WriteBatch`, flushing the WAL once per batch.
 * This amortises fsync/WAL overhead across many events.
 *
 * Back-pressure: when the ring buffer reaches `max_buffer_events`,
 * `ingest()` blocks until capacity is available or the timeout expires.
 *
 * ## Durability guarantee
 *
 * Every event is written to the WAL before `ingest()` returns to the caller
 * (when `Config::sync_wal == true`, the default).  The RocksDB write is
 * committed asynchronously by the flush thread.
 *
 * ## Thread safety
 *
 * All public methods are thread-safe.
 *
 * ## Usage
 * @code
 * StreamingIngestManager::Config cfg;
 * cfg.flush_interval = std::chrono::milliseconds(10);
 * cfg.max_buffer_events = 1'000'000;
 *
 * auto mgr = StreamingIngestManager::create(rocksdb_wrapper, std::move(cfg));
 * mgr->start();
 *
 * mgr->ingest("metrics:cpu:server01", "0.72");
 *
 * mgr->stop();
 * @endcode
 */
class StreamingIngestManager {
public:
    // ── Event type ────────────────────────────────────────────────────────

    /**
     * @brief A single ingest event.
     *
     * `key` and `value` are owned by the event once enqueued.
     */
    struct Event {
        std::string key;
        std::string value;
    };

    // ── Result types ──────────────────────────────────────────────────────

    /**
     * @brief Statistics snapshot from stats().
     */
    struct Stats {
        uint64_t events_ingested{0};  ///< Total events accepted since start()
        uint64_t events_flushed{0};   ///< Total events written to RocksDB
        uint64_t flush_count{0};      ///< Total flush-thread iterations
        uint64_t backpressure_waits{0}; ///< Times ingest() blocked on back-pressure
        uint64_t dropped_events{0};   ///< Events dropped due to overflow (overflow_policy=DROP)
    };

    // ── Configuration ─────────────────────────────────────────────────────

    /** What to do when the ring buffer is full. */
    enum class OverflowPolicy {
        BLOCK, ///< Block the caller until space is available (default).
        DROP,  ///< Silently discard the event and increment dropped_events.
    };

    struct Config {
        /** How often the flush thread drains the ring buffer to RocksDB. */
        std::chrono::milliseconds flush_interval{10};

        /**
         * Maximum number of buffered events before back-pressure kicks in.
         * Must be ≥ 1.
         */
        size_t max_buffer_events = 1'000'000;

        /**
         * Maximum number of events per RocksDB WriteBatch.
         * Larger batches improve throughput; smaller batches reduce latency.
         */
        size_t max_batch_size = 65'536;

        /**
         * When BLOCK: maximum time ingest() waits for buffer space.
         * When the timeout expires, ERR_STORAGE_LOG_FULL is returned.
         * Zero means "wait forever".
         */
        std::chrono::milliseconds backpressure_timeout{0};

        /** Policy when the ring buffer is full. */
        OverflowPolicy overflow_policy{OverflowPolicy::BLOCK};

        /**
         * When true (default), each WriteBatch is flushed to the WAL
         * synchronously before the flush thread moves on.  Set to false
         * for maximum throughput at the cost of durability.
         */
        bool sync_wal{true};
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────

    /**
     * @brief Create a StreamingIngestManager backed by @p db.
     *
     * @param db   RocksDB wrapper (must outlive the manager).
     * @param cfg  Tuning parameters.
     * @return A stopped manager; call start() before ingesting events.
     */
    static std::unique_ptr<StreamingIngestManager> create(
        std::shared_ptr<RocksDBWrapper> db,
        Config cfg = {});

    /** Start the background flush thread. */
    Result<void> start();

    /**
     * @brief Stop the background flush thread and flush all pending events.
     *
     * Blocks until the queue is drained.  After stop() the manager may not
     * be restarted.
     */
    Result<void> stop();

    // ── Ingest ────────────────────────────────────────────────────────────

    /**
     * @brief Enqueue a single event.
     *
     * When `Config::overflow_policy == BLOCK`, this method blocks until
     * buffer space is available (subject to `backpressure_timeout`).
     *
     * When `Config::overflow_policy == DROP`, returns success immediately
     * even if the event was discarded.
     *
     * @return Success, or ERR_STORAGE_LOG_FULL if the buffer is full and
     *         the backpressure timeout expired.
     */
    Result<void> ingest(std::string_view key, std::string_view value);

    /**
     * @brief Enqueue a batch of pre-built events.
     *
     * More efficient than calling ingest() in a loop because the mutex is
     * taken only once.  Events are appended in order.
     *
     * @return Number of events successfully enqueued.  If back-pressure
     *         triggers, the returned count may be less than `events.size()`.
     */
    Result<size_t> ingestBatch(std::vector<Event> events);

    /**
     * @brief Immediately flush all buffered events to RocksDB.
     *
     * Blocks until the flush is complete.  Useful for testing and graceful
     * shutdown scenarios.
     */
    Result<void> flush();

    // ── Observability ─────────────────────────────────────────────────────

    /** Return a snapshot of current statistics. */
    Stats stats() const noexcept;

    ~StreamingIngestManager();

    StreamingIngestManager(const StreamingIngestManager&)            = delete;
    StreamingIngestManager& operator=(const StreamingIngestManager&) = delete;

private:
    explicit StreamingIngestManager(std::shared_ptr<RocksDBWrapper> db, Config cfg);

    void flushLoop();
    Result<void> flushOnce(std::unique_lock<std::mutex>& lock);

    std::shared_ptr<RocksDBWrapper> db_;
    Config                          cfg_;

    mutable std::mutex              mu_;
    std::condition_variable         not_full_;
    std::condition_variable         not_empty_;

    std::vector<Event>              buffer_;
    std::atomic<bool>               running_{false};
    std::thread                     flush_thread_;

    // Stats counters (updated under mu_ or atomically).
    std::atomic<uint64_t>           stat_ingested_{0};
    std::atomic<uint64_t>           stat_flushed_{0};
    std::atomic<uint64_t>           stat_flush_count_{0};
    std::atomic<uint64_t>           stat_backpressure_{0};
    std::atomic<uint64_t>           stat_dropped_{0};
};

} // namespace themis
