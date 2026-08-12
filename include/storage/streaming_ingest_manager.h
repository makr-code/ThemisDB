/**
 * @file streaming_ingest_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "utils/expected.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {

/** @brief Streaming ingest manager component. */
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
        std::chrono::milliseconds flush_interval;

        /**
         * Maximum number of buffered events before back-pressure kicks in.
         * Must be ≥ 1.
         */
        size_t max_buffer_events;

        /**
         * Maximum number of events per RocksDB WriteBatch.
         * Larger batches improve throughput; smaller batches reduce latency.
         */
        size_t max_batch_size;

        /**
         * When BLOCK: maximum time ingest() waits for buffer space.
         * When the timeout expires, ERR_STORAGE_LOG_FULL is returned.
         * Zero means "wait forever".
         */
        std::chrono::milliseconds backpressure_timeout;

        /** Policy when the ring buffer is full. */
        OverflowPolicy overflow_policy;

        /**
         * When true (default), each WriteBatch is flushed to the WAL
         * synchronously before the flush thread moves on.  Set to false
         * for maximum throughput at the cost of durability.
         */
        bool sync_wal;

        Config()
            : flush_interval(10)
            , max_buffer_events(1'000'000)
            , max_batch_size(65'536)
            , backpressure_timeout(0)
            , overflow_policy(OverflowPolicy::BLOCK)
            , sync_wal(true)
        {}
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
