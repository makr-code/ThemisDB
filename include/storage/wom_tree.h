/**
 * @file wom_tree.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "utils/expected.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {

/**
 * @brief Write-Optimized Merge (WOM) Tree
 *
 * The WOM Tree is a write-optimized data structure modelled after the
 * Bε-tree (B-epsilon tree). Unlike LSM trees, which batch writes into
 * immutable sorted runs and periodically compact them, the WOM tree
 * keeps a live tree whose internal nodes each carry a small *write buffer*.
 * Pending mutations are propagated downward lazily — only when a buffer is
 * full — so the effective write-amplification is determined by the buffer
 * size ε rather than by the compaction fan-in.
 *
 * ## Key properties
 *
 * | Property               | WOM Tree            | LSM Tree             |
 * |------------------------|---------------------|----------------------|
 * | Write amplification    | 2–5×                | 10–30×               |
 * | Update-heavy workloads | ✅ Excellent        | ⚠️  Moderate          |
 * | Compaction overhead    | Low (lazy)          | High (periodic)      |
 * | Space amplification    | Moderate–High       | Low–Moderate         |
 * | Point-read latency     | Higher (multi-level)| Lower (bloom filter) |
 *
 * ## Design
 *
 * - **Fanout** (B): number of children per internal node (default 16).
 * - **Buffer fraction** (ε): fraction of each node's capacity used for the
 *   write buffer.  The remaining capacity holds pivot keys and child
 *   pointers.  Larger ε → lower write amplification, larger node footprint.
 * - **Lazy flushing**: when a node's buffer exceeds `buffer_size_bytes`,
 *   the oldest / largest batch is pushed to the appropriate child.  Leaf
 *   nodes apply mutations directly to their sorted data.
 *
 * ## Thread safety
 *
 * All public methods are thread-safe.  Read-only operations (`get`,
 * `contains`, `stats`, `scan`, `scanRange`, `size`, `empty`, `config`)
 * acquire a shared lock so concurrent readers do not block each other.
 * Mutating operations (`put`, `remove`, `compact`, `flushOnce`, `clear`)
 * acquire an exclusive lock.
 *
 * ## References
 *
 * - M. A. Bender et al., "An Introduction to Bε-Trees and Write-Optimization,"
 *   *;login: The USENIX Magazine*, 40(5):22–28, 2015.
 * - src/storage/FUTURE_ENHANCEMENTS.md#write-optimized-merge-wom-tree
 */
class WomTree {
public:
    // ─────────────────────────────────────────────────────────────────────
    // Configuration
    // ─────────────────────────────────────────────────────────────────────

    /** Configuration knobs for the WOM tree. */
    struct Config {
        /**
         * Target size in bytes for the write buffer held at each internal
         * node.  Larger values → fewer propagation steps → lower write
         * amplification, but higher per-node memory usage.
         * Recommended: 64 KiB–512 KiB.
         */
        size_t buffer_size_bytes = 64 * 1024;  // 64 KiB

        /**
         * Fanout factor B: maximum number of children per internal node.
         * Must be ≥ 2.  Higher fanout reduces tree height (fewer read
         * amplification levels) at the cost of larger nodes.
         */
        uint32_t fanout = 16;

        /**
         * Maximum number of entries per leaf node before the leaf is split.
         * Must be ≥ 2.
         */
        uint32_t leaf_capacity = 256;

        /**
         * Maximum total number of entries held in all in-memory node buffers
         * before a synchronous flush-to-leaf pass is triggered.  Zero means
         * "no global limit" (rely solely on per-node thresholds).
         */
        size_t max_buffered_entries = 0;

        /**
         * If true (default), a `remove()` call is buffered as a tombstone and
         * propagated lazily down to the leaf during the next flush/compact
         * pass.  This keeps the write path fast at the cost of a slightly
         * higher read path (every buffer along the path must be scanned for
         * the most-recent tombstone).
         *
         * If false, `remove()` immediately descends to the leaf, clears any
         * buffered ops for that key in all internal nodes on the path, and
         * physically erases the entry from the leaf.  This makes reads
         * cheaper (no buffer scanning for deleted keys) but makes the write
         * path heavier: it must walk the tree top-to-bottom and purge the
         * key from every intermediate buffer.
         */
        bool lazy_deletes = true;
    };

    // ─────────────────────────────────────────────────────────────────────
    // Statistics
    // ─────────────────────────────────────────────────────────────────────

    /** Live statistics snapshot returned by stats(). */
    struct Stats {
        /** Total number of put() calls since construction. */
        uint64_t total_puts{0};
        /** Total number of remove() calls since construction. */
        uint64_t total_removes{0};
        /** Total number of get() calls since construction. */
        uint64_t total_gets{0};
        /** Number of get() calls that found a value. */
        uint64_t get_hits{0};
        /** Number of buffer-flush (propagation) passes performed. */
        uint64_t flush_passes{0};
        /** Cumulative bytes written by user put() calls. */
        uint64_t user_bytes_written{0};
        /** Cumulative bytes written during internal buffer propagation. */
        uint64_t internal_bytes_written{0};
        /** Total live entries currently in the tree (approximate). */
        uint64_t live_entries{0};
        /** Current tree height (root → leaf depth). */
        uint32_t tree_height{0};
        /** Number of leaf nodes. */
        uint64_t leaf_count{0};
        /** Number of internal nodes. */
        uint64_t internal_node_count{0};

        /**
         * @brief Estimated write-amplification factor.
         *
         * Ratio of all bytes physically written to tree nodes (direct leaf
         * writes + buffer propagation hops) to bytes submitted by the user.
         * A value of 1.0 means each user byte was written exactly once
         * (ideal, single-leaf tree); typical multi-level WOM trees target
         * 2–5× compared to 10–30× for typical LSM trees.
         *
         * Returns 0.0 if no user bytes have been written yet.
         */
        double writeAmplification() const noexcept {
            if (user_bytes_written == 0) {
              return 0.0;
            }
            return static_cast<double>(internal_bytes_written) /
                   static_cast<double>(user_bytes_written);
        }

        /**
         * @brief Point-read hit ratio.
         *
         * Returns fraction of get() calls that found a value.  Values
         * below 1.0 include misses for absent keys; this is expected in
         * write-heavy workloads.
         */
        double readHitRatio() const noexcept {
            if (total_gets == 0) {
              return 0.0;
            }
            return static_cast<double>(get_hits) /
                   static_cast<double>(total_gets);
        }
    };

    // ─────────────────────────────────────────────────────────────────────
    // Construction / destruction
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Construct a WOM Tree with default configuration.
     */
    WomTree();

    /**
     * @brief Construct a WOM Tree with the supplied configuration.
     *
     * @throws std::invalid_argument if the configuration is invalid
     *         (e.g. fanout < 2, leaf_capacity < 2, buffer_size_bytes == 0).
     */
    explicit WomTree(const Config& config);

    ~WomTree();

    // Non-copyable, movable.
    WomTree(const WomTree&)            = delete;
    WomTree& operator=(const WomTree&) = delete;
    WomTree(WomTree&&)                 noexcept;
    WomTree& operator=(WomTree&&)      noexcept;

    // ─────────────────────────────────────────────────────────────────────
    // Core write API
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Insert or update a key-value pair.
     *
     * The mutation is appended to the root's write buffer.  If the buffer
     * exceeds `config.buffer_size_bytes` the affected subtree is flushed
     * synchronously before returning.
     *
     * @param key    Non-empty key.
     * @param value  Value to associate with the key.
     * @return OkVoid() on success.
     */
    Result<void> put(std::string_view key, std::string_view value);

    /**
     * @brief Remove a key from the tree.
     *
     * Depending on `config.lazy_deletes`, the key is either tombstoned
     * immediately (deferred purge) or removed synchronously from the leaf.
     *
     * @param key  Key to remove.
     * @return OkVoid() on success; Err(..., NOT_FOUND) if the key does not
     *         exist in the tree.
     */
    Result<void> remove(std::string_view key);

    // ─────────────────────────────────────────────────────────────────────
    // Core read API
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Look up the value for a key.
     *
     * The search traverses all in-memory node buffers from root to leaf,
     * applying any pending mutations in arrival order before returning the
     * effective value.  This is intentionally slower than in an LSM tree
     * (where bloom filters can short-circuit lookups) — a documented
     * WOM-tree trade-off.
     *
     * @param key  Key to look up.
     * @return Ok(value) if found; Err(..., NOT_FOUND) if absent or
     *         tombstoned.
     */
    Result<std::string> get(std::string_view key) const;

    /**
     * @brief Check whether a key is present.
     *
     * @param key  Key to test.
     * @return true if the key has a live value.
     */
    bool contains(std::string_view key) const;

    // ─────────────────────────────────────────────────────────────────────
    // Scan / iteration
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Scan all live key-value pairs in ascending key order.
     *
     * The callback is invoked for every live entry.  Returning false from
     * the callback stops the scan early.
     *
     * @param callback  Function called with (key, value) for each entry.
     *                  Return true to continue, false to stop.
     */
    void scan(const std::function<bool(std::string_view key,
                                       std::string_view value)>& callback) const;

    /**
     * @brief Scan a half-open key range [start_key, end_key) in ascending
     *        order.
     *
     * @param start_key  Inclusive lower bound (empty string = beginning).
     * @param end_key    Exclusive upper bound (empty string = end).
     * @param callback   Called with (key, value).  Return false to stop.
     */
    void scanRange(std::string_view start_key,
                   std::string_view end_key,
                   const std::function<bool(std::string_view key,
                                            std::string_view value)>& callback) const;

    // ─────────────────────────────────────────────────────────────────────
    // Maintenance
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Force a full flush of all node buffers down to the leaves.
     *
     * After this call every live mutation resides in a leaf node and all
     * tombstones have been purged.  This is the analogue of a full
     * compaction in an LSM tree, but incurs significantly less I/O because
     * each mutation has already been partially propagated.
     *
     * @return OkVoid() on success.
     */
    Result<void> compact();

    /**
     * @brief Flush buffered mutations from the root down one level.
     *
     * This is a lightweight, incremental version of compact() suitable for
     * background maintenance.  Call periodically to keep buffer occupancy
     * low without blocking writers for long.
     *
     * @return OkVoid() on success.
     */
    Result<void> flushOnce();

    /**
     * @brief Return the number of live entries.
     *
     * The count is exact in single-threaded usage.  Under concurrent writes
     * it may be momentarily stale by at most the number of in-flight writers,
     * but it is always consistent with what `get()`/`contains()` observe once
     * the same exclusive lock is acquired.
     */
    size_t size() const noexcept;

    /**
     * @brief Return true if the tree contains no live entries.
     */
    bool empty() const noexcept;

    /**
     * @brief Remove all entries from the tree, resetting all statistics.
     */
    void clear();

    // ─────────────────────────────────────────────────────────────────────
    // Observability
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Return a consistent snapshot of current statistics.
     */
    Stats stats() const;

    /**
     * @brief Return the active configuration.
     */
    const Config& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace themis
