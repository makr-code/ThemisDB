/**
 * @file interval_tree_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Augmented Interval Tree Index
 *
 * An augmented BST-based interval tree with per-node max-end tracking,
 * providing O(log n) insert/remove and O(log n + k) overlap-query
 * performance where k is the number of matching intervals.
 *
 * Suitable for high-throughput valid-time overlap detection in bi-temporal
 * tables and period-predicate foreign-key enforcement.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * A single entry stored in the interval tree.
 *
 * Each entry represents a half-open interval [range.start, range.end)
 * associated with a named key and an optional JSON payload.
 */
struct IntervalEntry {
    std::string key;
    TimeRange   range;
    nlohmann::json payload; ///< Optional denormalised columns for covering scans
};

/**
 * Snapshot statistics for an IntervalTreeIndex instance.
 */
struct IntervalTreeStats {
    size_t    total_entries{0};
    Timestamp min_start{kMaxTimestamp};
    Timestamp max_end{kMinTimestamp};
    size_t    point_queries{0};
    size_t    overlap_queries{0};
    size_t    height{0};   ///< Current tree height (informational)

    nlohmann::json toJson() const {
        return {{"total_entries",   total_entries},
                {"min_start",       min_start},
                {"max_end",         max_end},
                {"point_queries",   point_queries},
                {"overlap_queries", overlap_queries},
                {"height",          height}};
    }
};

/**
 * @brief Augmented interval tree index for O(log n + k) overlap detection.
 *
 * Implements a classic centre-key BST where each node additionally stores
 * the maximum `range.end` value in the subtree rooted at that node.  This
 * augmentation lets the query algorithm prune entire subtrees when their
 * max-end cannot overlap the query interval, yielding O(log n + k) time for
 * stab / overlap queries instead of the O(n) linear scan used by the
 * simpler `TemporalIndex` class.
 *
 * ## Key operations
 * | Operation         | Time complexity                          |
 * |-------------------|------------------------------------------|
 * | insert()          | O(log n) — AVL-balanced                  |
 * | remove()          | O(log n) — AVL-balanced                  |
 * | queryPoint()      | O(log n + k)                             |
 * | queryOverlap()    | O(log n + k)                             |
 * | queryKey()        | O(k) via secondary key index             |
 * | size()            | O(1) — atomic                            |
 *
 * Thread-safety: reads use a shared lock (std::shared_mutex) so concurrent
 * point/overlap/key queries do not block one another.  Writes acquire an
 * exclusive lock.
 *
 * @note The BST is kept balanced via AVL rotations (LL, RR, LR, RL cases).
 *       The `subtree_max_end` augmentation field is updated atomically during
 *       each rotation so the invariant is preserved at all times.  A secondary
 *       hash index (key → entries) provides O(k) queryKey() without a full
 *       tree traversal.
 */
class IntervalTreeIndex {
public:
    explicit IntervalTreeIndex(std::string name);

    // Non-copyable; movable
    IntervalTreeIndex(const IntervalTreeIndex&)            = delete;
    IntervalTreeIndex& operator=(const IntervalTreeIndex&) = delete;
    IntervalTreeIndex(IntervalTreeIndex&&)                 = default;
    IntervalTreeIndex& operator=(IntervalTreeIndex&&)      = default;

    ~IntervalTreeIndex() = default;

    // ── Mutation ──────────────────────────────────────────────────────────────

    /**
     * Insert an entry into the tree.
     * Amortised O(log n).
     */
    void insert(const IntervalEntry& entry);

    /**
     * Remove all entries matching both key and range exactly.
     * Returns the number of entries removed.  O(log n) expected.
     */
    size_t remove(const std::string& key, const TimeRange& range);

    /**
     * Remove all entries for the given key regardless of range.
     * O(n) worst-case due to multiple hits with the same key.
     */
    size_t removeKey(const std::string& key);

    /**
     * @brief STL-style erase: remove all entries for the given key.
     *
     * This is an alias for `removeKey()` that follows STL container naming
     * conventions (`erase` vs. `remove`).  It removes every interval entry
     * associated with @p key from the tree while maintaining the AVL-balance
     * invariant through tree rotations.
     *
     * Complexity:
     *   - O(k·log n) where k is the number of intervals stored for @p key and
     *     n is the total number of entries in the tree.
     *   - For the common case of a unique-key index (k = 1) this degenerates
     *     to O(log n).
     *
     * @par Thread-safety
     * `erase()` acquires an **exclusive** (`std::unique_lock`) on the internal
     * `std::shared_mutex`.  All concurrent readers — including those holding a
     * `std::shared_lock` via `queryPoint()`, `queryRange()`, or `queryKey()` —
     * are **blocked** until the erase completes.  Callers that received results
     * from a previous query call must not retain raw pointers or references into
     * those result vectors after `erase()` returns; the returned
     * `std::vector<IntervalEntry>` copies are safe to use independently.
     *
     * @param key  Logical key whose entries should be removed.
     * @return     Number of entries actually removed (0 if the key was absent).
     */
    size_t erase(const std::string& key);

    /** Remove all entries. O(n). */
    void clear();

    // ── Queries ───────────────────────────────────────────────────────────────

    /**
     * Return all entries whose interval contains timestamp t,
     * i.e. entry.range.start <= t < entry.range.end.
     * O(log n + k).
     */
    std::vector<IntervalEntry> queryPoint(Timestamp t) const;

    /**
     * Return all entries whose interval overlaps [from, to).
     * An interval [a, b) overlaps [from, to) iff a < to && from < b.
     * O(log n + k).
     */
    std::vector<IntervalEntry> queryOverlap(Timestamp from, Timestamp to) const;

    /**
     * Return all entries whose interval overlaps the given range.
     * Convenience overload for queryOverlap(range.start, range.end).
     */
    std::vector<IntervalEntry> queryOverlap(const TimeRange& range) const;

    /**
     * Return all entries for a specific key, optionally filtered to those
     * whose interval overlaps the given range.  O(n) worst-case.
     */
    std::vector<IntervalEntry> queryKey(
        const std::string& key,
        std::optional<TimeRange> range = std::nullopt) const;

    // ── Metadata ─────────────────────────────────────────────────────────────

    const std::string& name() const noexcept { return name_; }

    /** O(1) — maintained as an atomic counter. */
    size_t size() const noexcept;

    IntervalTreeStats stats() const;

private:
    // ── Internal BST node ────────────────────────────────────────────────────

    struct Node {
        IntervalEntry entry;

        /// Maximum `range.end` in this subtree (augmentation field).
        Timestamp subtree_max_end{kMinTimestamp};

        /// AVL balance height (1 for a leaf node).
        int height{1};

        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        explicit Node(IntervalEntry e)
            : entry(std::move(e)),
              subtree_max_end(entry.range.end) {}
    };

    // ── Internal helpers ─────────────────────────────────────────────────────

    static Timestamp nodeMaxEnd(const Node* n) noexcept;
    static void      updateSubtreeMax(Node* n) noexcept;

    // AVL helpers
    static int  nodeHeight(const Node* n) noexcept;
    static int  balanceFactor(const Node* n) noexcept;
    static void updateHeight(Node* n) noexcept;
    static std::unique_ptr<Node> rotateRight(std::unique_ptr<Node> y);
    static std::unique_ptr<Node> rotateLeft(std::unique_ptr<Node> x);
    static std::unique_ptr<Node> balance(std::unique_ptr<Node> n);

    static std::unique_ptr<Node> insertNode(std::unique_ptr<Node> root,
                                            const IntervalEntry& entry);

    static std::unique_ptr<Node> removeNode(std::unique_ptr<Node> root,
                                            const std::string& key,
                                            const TimeRange& range,
                                            size_t& removed_count);

    static std::unique_ptr<Node> removeKeyNode(std::unique_ptr<Node> root,
                                               const std::string& key,
                                               size_t& removed_count);

    /// Extract the in-order successor (leftmost node of a subtree).
    static Node* findMin(Node* n) noexcept;

    /// Detach and return the leftmost node, repairing the subtree.
    static std::unique_ptr<Node> detachMin(std::unique_ptr<Node> root,
                                           std::unique_ptr<Node>& out_node);

    void collectOverlap(const Node* n, Timestamp from, Timestamp to,
                        std::vector<IntervalEntry>& out) const;

    void collectKey(const Node* n, const std::string& key,
                    std::optional<TimeRange> range,
                    std::vector<IntervalEntry>& out) const;

    static size_t treeHeight(const Node* n) noexcept;

    // ── State ─────────────────────────────────────────────────────────────────

    std::string               name_;
    std::unique_ptr<Node>     root_;
    std::atomic<size_t>       size_{0};

    /// Secondary index: key → copies of all entries with that key.
    /// Provides O(k) queryKey() without a full BST traversal.
    std::unordered_map<std::string, std::vector<IntervalEntry>> key_index_;

    mutable std::shared_mutex mutex_;
    mutable IntervalTreeStats stats_;
};

} // namespace temporal
} // namespace themisdb
