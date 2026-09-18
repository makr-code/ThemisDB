// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file lock_free_linker_contract.h
 * @brief Lock-free data structures and performance scaling for process linking under high contention.
 * @version 2.1.0-beta
 *
 * @section purpose Purpose
 * Defines lock-free linking layer for process module to support >10,000 concurrent link operations/sec
 * with P99 latency < 1ms. Replaces contended mutex-based linking with atomic CAS operations,
 * lock-free hash tables, and epoch-based memory reclamation.
 *
 * @section motivation Motivation
 *
 * Current (Phase 1-6) ProcessLinker uses fine-grained per-link mutex locking:
 * - Latency: 1-10ms per link operation (acceptable for low churn)
 * - Throughput: ~100-500 links/sec per core
 * - Problem: Under >500 concurrent operations, lock contention becomes bottleneck
 *
 * Lock-free redesign:
 * - Target throughput: ≥10,000 links/sec per core
 * - Target latency: P99 < 1ms
 * - Backward compatible: single-shard path unaffected; opt-in for federated deployments
 *
 * @section architecture Architecture
 *
 * ### Layered Design
 * ```
 * Client Layer (ProcessLinker public API)
 *     ↓ (dispatcher: check lock_free flag)
 *     ├─ Lock-Free Path (opt-in, high throughput)
 *     │   ├─ Lock-Free Hash Table (link registry)
 *     │   ├─ Atomic CAS for link state transitions
 *     │   ├─ Epoch-based memory reclamation
 *     │   └─ Batch commit ring buffer
 *     │
 *     └─ Traditional Lock Path (fallback, low latency)
 *         ├─ Per-link mutex
 *         ├─ Snapshot isolation
 *         └─ Simple FIFO queue
 * ```
 *
 * ### Lock-Free Link Registry
 * - Hash table: `ConcurrentHashMap<LinkId, LinkState>`
 * - Per-bucket locking: minimal (only during resize, not on lookup)
 * - Link state includes: status (PENDING/ACTIVE/DELETED), version, references count
 *
 * ### Link State Machine (Atomic)
 * ```
 * PENDING --[commit]--> ACTIVE --[delete]--> DELETED
 *                          ↑                      ↓
 *                          └─────[mark_stale]────┘
 * ```
 * Transitions via atomic CAS; ABA problem mitigated via versioning.
 *
 * ### Memory Reclamation
 * - **Epoch-based:** Global epoch counter; threads announce current epoch
 * - **Deferred deletion:** Marked-deleted entries deferred until all threads past epoch
 * - **Simplicity:** No hazard pointers or RCU; trade complexity for latency predictability
 * - **Overhead:** ~epoch_duration (usually 1-10ms) additional memory before reclamation
 *
 * ### Batch Operation Queue
 * - Lock-free ring buffer for staging link commits
 * - Producer: link creation appends to buffer (atomic pointer CAS)
 * - Consumer: background thread batches commits (atomic swap)
 * - Backpressure: if buffer full, apply retry/backoff to producer
 *
 * @section progress_guarantees Progress Guarantees
 *
 * | Term | Definition | Scope | Guarantee |
 * |------|------------|-------|-----------|
 * | Wait-Free | Bounded operations; no waiting | Single link creation | Link create/delete in O(1) CAS attempts (99.9%+ first-try) |
 * | Lock-Free | ≥1 thread progresses | Batch operations | ≥1 batch commits despite contention |
 * | Obstruction-Free | Progresses if uncontended | All operations | Guaranteed progress if no contention |
 *
 * **Achieved levels:**
 * - Link creation: wait-free (single atomic CAS)
 * - Link deletion: obstruction-free (epoch-based cleanup)
 * - Batch commit: lock-free (ring buffer atomic swap)
 *
 * @section aba_mitigation ABA Problem Mitigation
 *
 * **Problem:** Thread A reads value V1, is preempted, reads value V1 again later.
 * Between reads, other threads may have changed V1 → V2 → V1 (identical value, different state).
 * CAS succeeds even though state changed.
 *
 * **Solution:** Version tagging
 * ```cpp
 * struct VersionedLinkState {
 *     LinkState state;      // PENDING, ACTIVE, DELETED
 *     uint32_t version;     // Incremented on every state change
 * };
 *
 * // CAS compares both state and version
 * compare_and_swap(link_state, {ACTIVE, v1}, {DELETED, v1+1})
 * // Fails if version changed, even if state coincidentally same
 * ```
 *
 * **Alternative:** Epoch-based prevention (simpler, lower overhead)
 * - Epoch advances periodically (every 1-10ms)
 * - Versioning happens only across epochs
 * - Within epoch, ABA probability negligible
 *
 * @section memory_ordering Memory Ordering
 *
 * All atomic operations use `std::memory_order_acq_rel` (acquire-release):
 * - Read: acquire semantics (prevent later ops from moving before read)
 * - Write: release semantics (ensure all prior ops complete before write visible)
 * - CAS: both (full synchronization)
 *
 * Stronger than necessary (seq_cst not needed) but simpler than hand-tuned ordering.
 *
 * @section contract_freeze Contract Freeze
 * This contract is frozen for ThemisDB v2.1; breaking changes require v3.0.
 */

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <optional>

namespace themis::process {

// ============================================================================
// Lock-Free Link State Machine
// ============================================================================

/**
 * @brief State of a link in lock-free registry.
 */
enum class LinkState : uint32_t {
    /// Link created, not yet committed
    PENDING = 6400,
    /// Link committed, active, accessible
    ACTIVE = 6401,
    /// Link marked for deletion (epoch-based cleanup pending)
    MARKED_FOR_DELETION = 6402,
    /// Link deleted, reclaimed
    DELETED = 6403,
};

/**
 * @brief Versioned link state (for ABA prevention).
 *
 * Combines state and version; version incremented on every state change.
 * Prevents ABA problem: even if state cycles V1→V2→V1, version differs.
 */
struct VersionedLinkState {
    /// Current link state
    LinkState state = LinkState::PENDING;

    /// Version number (incremented on state change)
    uint32_t version = 0;

    /// Reference count (number of active references to this link)
    std::atomic<uint32_t> ref_count{0};

    /**
     * @brief Attempt state transition via CAS.
     *
     * @param expected_state Expected current state
     * @param expected_version Expected current version
     * @param new_state New state to transition to
     * @return true if transition succeeded; false if state/version mismatch
     */
    bool tryTransition(LinkState expected_state, uint32_t expected_version,
                       LinkState new_state) noexcept;
};

/**
 * @brief Entry in lock-free link registry.
 *
 * Stores link metadata; actual link data stored elsewhere (unchanged from Phase 1-6).
 */
struct LockFreeLinkEntry {
    /// Link identifier (unique per shard)
    std::string link_id;

    /// Source model ID
    std::string source_model_id;

    /// Target entity ID
    std::string target_id;

    /// Link type (e.g., "document", "instance")
    std::string link_type;

    /// Versioned state machine (atomic updates)
    VersionedLinkState versioned_state;

    /// Timestamp of link creation (UTC, nanoseconds since epoch)
    int64_t created_at_ns = 0;

    /// Timestamp of last modification
    int64_t modified_at_ns = 0;

    /// Principal (user/service) ID that created this link
    std::string creator_principal;

    /// true if this link is stale (source model deleted)
    std::atomic<bool> is_stale{false};

    /**
     * @brief Increment reference count (prevents reclamation while referenced).
     */
    void acquireRef() noexcept {
        versioned_state.ref_count.fetch_add(1, std::memory_order_acq_rel);
    }

    /**
     * @brief Decrement reference count; may trigger reclamation.
     */
    void releaseRef() noexcept {
        versioned_state.ref_count.fetch_sub(1, std::memory_order_acq_rel);
    }

    /**
     * @brief Get current reference count.
     * @return Reference count
     */
    uint32_t getRefCount() const noexcept {
        return versioned_state.ref_count.load(std::memory_order_acquire);
    }
};

// ============================================================================
// Epoch-Based Memory Reclamation
// ============================================================================

/**
 * @brief Global epoch counter for memory reclamation.
 *
 * Each thread announces the epoch it's in; memory is reclaimed when all threads
 * have advanced past a given epoch.
 */
class EpochManager {
public:
    /**
     * @brief Get singleton instance.
     * @return Reference to global epoch manager
     */
    static EpochManager& instance();

    /**
     * @brief Get current global epoch.
     * @return Monotonically increasing epoch number
     */
    uint64_t currentEpoch() const noexcept {
        return global_epoch_.load(std::memory_order_acquire);
    }

    /**
     * @brief Announce that current thread is using this epoch.
     *
     * Thread must call this before dereferencing any lock-free data.
     * Call `leaveEpoch()` when done.
     *
     * @param epoch Epoch to enter
     */
    void enterEpoch(uint64_t epoch) noexcept;

    /**
     * @brief Announce that current thread is done with current epoch.
     *
     * Thread may now advance to next epoch.
     */
    void leaveEpoch() noexcept;

    /**
     * @brief Wait for all threads to leave a given epoch.
     *
     * Blocks until all threads have called `leaveEpoch()` for epoch.
     * Used before reclaiming memory associated with epoch.
     *
     * @param epoch Epoch to synchronize on
     * @param timeout_ms Maximum time to wait (0 = no limit)
     * @return true if all threads left epoch; false if timeout
     */
    bool waitForEpoch(uint64_t epoch, uint32_t timeout_ms = 0) noexcept;

    /**
     * @brief Advance global epoch and trigger reclamation if safe.
     *
     * Called periodically (every 10-100ms); checks if all threads past an epoch.
     * If safe, reclaims memory deferred from that epoch.
     */
    void advanceEpoch() noexcept;

    /**
     * @brief Defer object deletion until epoch is safe.
     *
     * Object is not deleted immediately; instead queued for deletion when all threads
     * have left the current epoch.
     *
     * @param epoch Epoch to defer deletion until
     * @param deleter Callable to delete object (e.g., lambda or std::function)
     */
    template<typename Deleter>
    void deferDeletion(uint64_t epoch, Deleter deleter);

private:
    EpochManager() = default;
    ~EpochManager() = default;

    std::atomic<uint64_t> global_epoch_{0};
    // Additional internal state for thread tracking (implementation-specific)
};

// ============================================================================
// Lock-Free Hash Table
// ============================================================================

/**
 * @brief Lock-free concurrent hash table for link registry.
 *
 * Supports lockless reads and minimal-contention writes.
 */
template<typename Key, typename Value>
class LockFreeHashTable {
public:
    /**
     * @brief Insert a key-value pair (or update if exists).
     *
     * Atomic operation; concurrent calls are serialized at key level.
     *
     * @param key Key to insert
     * @param value Value to insert
     * @return true if inserted; false if key already existed (value updated)
     */
    bool insert(const Key& key, const Value& value);

    /**
     * @brief Lookup a value by key.
     *
     * Lock-free read; may return stale value briefly during concurrent writes.
     *
     * @param key Key to lookup
     * @return Value if found; std::nullopt otherwise
     */
    std::optional<Value> lookup(const Key& key) const;

    /**
     * @brief Delete a key-value pair.
     *
     * Marks entry for deletion; actual reclamation deferred to epoch-safe point.
     *
     * @param key Key to delete
     * @return true if deleted; false if not found
     */
    bool erase(const Key& key);

    /**
     * @brief Get number of entries (approximate; may be stale).
     * @return Approximate entry count
     */
    size_t size() const noexcept;

    /**
     * @brief Iterate over all entries (snapshot).
     *
     * Note: concurrent modifications may not be visible.
     *
     * @param visitor Callable(const Key&, const Value&) called for each entry
     */
    template<typename Visitor>
    void forEach(Visitor visitor) const;
};

// ============================================================================
// Lock-Free Batch Queue
// ============================================================================

/**
 * @brief Lock-free ring buffer for batching link commits.
 *
 * Producers (link creators) append to buffer; consumer (background thread) batches commits.
 * Allows amortizing commit overhead across multiple links.
 */
struct BatchQueueEntry {
    /// Link entry to commit
    std::shared_ptr<LockFreeLinkEntry> link_entry;

    /// Timestamp of entry creation
    int64_t created_at_ns = 0;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Ring buffer for batch queue (multi-producer, single-consumer).
 *
 * MPMC-friendly: multiple producers can append concurrently;
 * single consumer drains buffer.
 */
class LockFreeBatchQueue {
public:
    /**
     * @brief Create batch queue with capacity.
     * @param capacity Maximum entries before blocking (or backoff)
     */
    explicit LockFreeBatchQueue(size_t capacity);

    /**
     * @brief Try to enqueue an entry (non-blocking).
     *
     * @param entry Entry to enqueue
     * @return true if enqueued; false if queue full (caller should retry/backoff)
     */
    bool tryEnqueue(const BatchQueueEntry& entry) noexcept;

    /**
     * @brief Dequeue all buffered entries (atomic swap).
     *
     * Called by consumer thread; atomically swaps current buffer with new empty buffer.
     * Returns snapshot of entries; consumer can process without blocking producers.
     *
     * @return Vector of entries to process
     */
    std::vector<BatchQueueEntry> drain() noexcept;

    /**
     * @brief Get approximate queue depth (may be stale).
     * @return Approximate number of buffered entries
     */
    size_t depth() const noexcept;

    /**
     * @brief Get queue capacity.
     * @return Maximum entries before blocking
     */
    size_t capacity() const noexcept;
};

// ============================================================================
// Lock-Free Linker Configuration
// ============================================================================

/**
 * @brief Configuration for lock-free linking layer.
 */
struct LockFreeLinkerConfig {
    /// true to enable lock-free mode; false to use traditional locking
    bool enable_lock_free = true;

    /// Capacity of batch queue (entries)
    uint32_t batch_queue_capacity = 10000;

    /// Batch commit interval (ms); background thread commits every N ms
    uint32_t batch_commit_interval_ms = 10;

    /// Epoch duration (ms); global epoch advances every N ms
    uint32_t epoch_duration_ms = 100;

    /// Timeout for epoch synchronization before forcing reclamation (ms)
    uint32_t epoch_sync_timeout_ms = 1000;

    /// Initial hash table size (grows automatically)
    uint32_t hash_table_initial_size = 16384;

    /// Maximum load factor before hash table resizes (0.0 - 1.0)
    double hash_table_max_load_factor = 0.75;

    /// true to enable profiling (measures latencies, throughput)
    bool enable_profiling = false;

    /// Correlation ID for tracing lock-free operations
    std::string trace_id;
};

// ============================================================================
// Performance Monitoring
// ============================================================================

/**
 * @brief Metrics for lock-free linker performance.
 */
struct LockFreeLinkerMetrics {
    /// Total link create operations
    std::atomic<uint64_t> total_creates{0};

    /// Successful link creates
    std::atomic<uint64_t> successful_creates{0};

    /// Failed link creates (retried by caller)
    std::atomic<uint64_t> failed_creates{0};

    /// Total CAS attempts (first-attempt + retries)
    std::atomic<uint64_t> cas_attempts{0};

    /// First-try CAS success rate (higher = less contention)
    double cas_first_try_rate = 0.0;

    /// Average link create latency (microseconds)
    double avg_create_latency_us = 0.0;

    /// P99 link create latency (microseconds)
    double p99_create_latency_us = 0.0;

    /// Batch queue overflow events (queue full)
    std::atomic<uint64_t> batch_queue_overflows{0};

    /// Epoch sync timeouts (forced reclamation)
    std::atomic<uint64_t> epoch_sync_timeouts{0};

    /// Memory deferred for reclamation (bytes)
    std::atomic<uint64_t> deferred_memory_bytes{0};

    /// Hash table resize operations
    std::atomic<uint64_t> hash_table_resizes{0};

    /**
     * @brief Get metrics snapshot (for reporting).
     * @return JSON representation of metrics
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// Fallback and Safety
// ============================================================================

/**
 * @brief Build-time flag to enable/disable lock-free code.
 *
 * Can be set to LOCK_FREE_DISABLED for safety/testing to force traditional locking
 * even when lock_free config is enabled.
 */
enum class LockFreeMode : int32_t {
    /// Lock-free mode enabled (use atomic operations)
    LOCK_FREE_ENABLED = 6410,
    /// Lock-free mode disabled (use traditional mutex locking for safety)
    LOCK_FREE_DISABLED = 6411,
};

} // namespace themis::process
