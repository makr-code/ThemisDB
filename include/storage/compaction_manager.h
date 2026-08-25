/**
 * @file compaction_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.46
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/rocksdb_wrapper.h"
#include "utils/expected.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace themis {

/**
 * @brief Compaction and Garbage-Collection manager for the storage engine.
 *
 * CompactionManager wraps RocksDB's built-in compaction API and adds:
 *
 *   • **On-demand compaction** – compact a key range or all data immediately.
 *   • **Background GC loop** – periodic background thread that triggers
 *     compaction and tombstone cleanup automatically.
 *   • **Tombstone accounting** – tracks an approximate tombstone count so
 *     the caller can decide when to run GC.
 *   • **Write-amplification metrics** – exposes the raw RocksDB statistics
 *     string for monitoring and alerting.
 *
 * ## Thread safety
 *
 * All public methods are thread-safe.  The background thread is started by
 * startBackgroundGC() and stopped (with a join) by stopBackgroundGC() or
 * the destructor.
 */
class CompactionManager {
public:
    /** Configuration options. */
    struct Config {
        /** Minimum number of tombstone deletions before triggering a GC pass. */
        uint64_t tombstone_gc_threshold = 10'000;

        /** Interval between automatic background GC passes. */
        std::chrono::seconds bg_gc_interval{300};  // 5 minutes default

        /** If true, enable full range compaction during background GC. */
        bool enable_full_compaction = false;
    };

    /** Statistics snapshot returned by stats(). */
    struct Stats {
        uint64_t tombstones_tracked{0};  ///< Approximate live tombstone count
        uint64_t gc_runs{0};             ///< Total number of GC passes completed
        uint64_t manual_compactions{0};  ///< Total manual compaction calls
        std::string rocksdb_stats;       ///< Raw RocksDB statistics string

        // ── Write-amplification ──────────────────────────────────────────
        /// Bytes written by user (logical writes); from rocksdb::BYTES_WRITTEN.
        uint64_t user_bytes_written{0};
        /// Bytes written by L1+ compaction; parsed from the Write(GB) column in
        /// the per-level Compaction Stats table inside the `rocksdb.stats` property
        /// string.  Zero if the stats string is unavailable or format changed.
        uint64_t compact_bytes_written{0};
        /// Bytes written by memtable flush (L0 output); parsed from the L0 row
        /// of the Compaction Stats table in `rocksdb.stats`.  Zero if unavailable.
        uint64_t flush_bytes_written{0};

        /**
         * @brief Estimated write-amplification factor.
         *
         * Defined as (compact_bytes_written + flush_bytes_written) /
         * user_bytes_written.  Returns 0.0 if no user writes have occurred.
         * A value close to 1.0 is ideal; values above 2.0 indicate excessive
         * compaction overhead relative to the user workload.
         */
        double writeAmplification() const noexcept {
            if (user_bytes_written == 0) return 0.0;
            return static_cast<double>(compact_bytes_written + flush_bytes_written)
                   / static_cast<double>(user_bytes_written);
        }
    };

    /**
     * @brief Construct a CompactionManager with default configuration.
     *
     * @param db  Shared ownership of the underlying RocksDB instance.
     *            Must already be open.
     */
    explicit CompactionManager(std::shared_ptr<RocksDBWrapper> db);

    /**
     * @brief Construct a CompactionManager.
     *
     * @param db     Shared ownership of the underlying RocksDB instance.
     *               Must already be open.
     * @param config Configuration options.
     */
    CompactionManager(std::shared_ptr<RocksDBWrapper> db, const Config& config);

    /// @brief Destructor — noexcept; stops background GC thread and swallows exceptions.
    ~CompactionManager() noexcept;

    // Not copyable or movable after construction.
    CompactionManager(const CompactionManager&) = delete;
    CompactionManager& operator=(const CompactionManager&) = delete;

    // ── Manual compaction ─────────────────────────────────────────────────

    /**
     * @brief Compact a specific key range [start_key, end_key).
     *
     * Blocks until RocksDB finishes the compaction.
     */
    Result<void> compactRange(std::string_view start_key, std::string_view end_key);

    /**
     * @brief Compact the entire keyspace.
     *
     * This is an expensive operation – prefer compactRange() in production.
     */
    Result<void> compactAll();

    // ── Tombstone tracking ────────────────────────────────────────────────

    /**
     * @brief Notify the manager that @p count DEL entries were written.
     *
     * When the tombstone count reaches @c config.tombstone_gc_threshold,
     * a compaction is triggered on the next background GC pass.
     */
    void recordDeletions(uint64_t count = 1);

    /**
     * @brief Run a GC pass immediately (synchronous).
     *
     * Triggers a full compaction if the tombstone threshold has been
     * reached, or if @p force is true.
     *
     * @return Result<void> – ok on success, error on compaction failure.
     */
    Result<void> runGC(bool force = false);

    // ── Background GC ─────────────────────────────────────────────────────

    /**
     * @brief Start the background GC thread.
     *
     * Does nothing if the thread is already running.
     */
    void startBackgroundGC();

    /**
     * @brief Stop and join the background GC thread.
     *
     * Blocks until the background thread has exited.
     */
    void stopBackgroundGC();

    /** Return true if the background GC thread is currently running. */
    bool isBackgroundGCRunning() const;

    // ── Dynamic configuration ─────────────────────────────────────────────

    /**
     * @brief Replace the current configuration.
     *
     * If the background GC thread is running, it is stopped, the new config
     * is applied, and the thread is restarted so the new @c bg_gc_interval
     * takes effect immediately.
     *
     * @param config  New configuration to apply.
     */
    void setConfig(const Config& config);

    /** Return the active configuration. */
    Config getConfig() const;

    // ── Metrics ───────────────────────────────────────────────────────────

    /** Return a snapshot of current compaction statistics. */
    Stats stats() const;

private:
    void backgroundLoop();

    std::shared_ptr<RocksDBWrapper> db_;
    Config config_;

    std::atomic<uint64_t> tombstones_{0};
    std::atomic<uint64_t> gc_runs_{0};
    std::atomic<uint64_t> manual_compactions_{0};

    // Background GC thread
    std::thread       bg_thread_;
    std::atomic<bool> bg_stop_{false};
    mutable std::mutex bg_mutex_;
    std::condition_variable bg_cv_;
};

} // namespace themis
