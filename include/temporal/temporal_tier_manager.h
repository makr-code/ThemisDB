/**
 * @file temporal_tier_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Tier Manager
 *
 * Three-tier LSM-style version storage following the RocksDB philosophy:
 *
 *   Hot  (MemTable)  — mutable sorted map, fully in RAM/VRAM.
 *                      Last N versions per key; O(log n) AS-OF queries.
 *
 *   Warm (L0 blocks) — immutable VersionBlocks in RAM.
 *                      Each block carries a Bloom filter for O(1) miss
 *                      detection and min/max timestamps for range pruning.
 *                      Values are RAM-resident but payload is optional-LZ4.
 *
 *   Cold (L1+ SST)   — only composite keys in RAM; JSON values on disk
 *                      via TemporalColdStore / FileSystemBackend.
 *
 * ── Tier transition (Abwägungsentscheidung) ──────────────────────────────
 *
 * Every insert evaluates TierPolicy::evaluate() to decide whether the hot
 * tier should be flushed to warm, or warm blocks compacted to cold.
 * TierPolicy ships with a threshold-based default implementation.
 *
 * ## Future plan — autonomous LLM / LoRA tier decisions
 *
 * The `TierPolicy::decision_fn` field is an optional hook that replaces the
 * built-in threshold logic with an arbitrary callable.  In a future release
 * this hook will be wired to the ThemisDB LoRA/ML stack so that an
 * in-process LoRA adapter can observe real-time workload signals
 * (access frequency, query patterns, RAM pressure, time-of-day, key
 * cardinality) and emit tier-migration decisions autonomously.
 *
 * Expected wiring (future, not yet implemented):
 *
 *   auto policy = TierPolicy::withLoraAdvisor(lora_router);
 *   tier_manager.setPolicy(policy);
 *
 * Until the LoRA advisor is available, the default threshold logic is used.
 * The hook contract is identical so the switch is fully backward-compatible.
 *
 * ── Complexity summary ────────────────────────────────────────────────────
 *
 * | Operation          | Hot tier      | Warm tier        | Cold tier      |
 * |--------------------|---------------|------------------|----------------|
 * | insert()           | O(log h)      | —                | —              |
 * | getAsOf()          | O(log h)      | O(b * bloomO(1)) | O(log N) + I/O |
 * | getHistory()       | O(h)          | O(b * k_w)       | O(k_c) + I/O   |
 * | flush hot→warm     | O(h log h)    | O(1) append      | —              |
 * | compact warm→cold  | —             | O(b * k_w)       | O(k_w log N)   |
 *
 * h = hot versions per key, b = warm blocks per key,
 * k_w/k_c = versions in warm/cold, N = total cold versions
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_cold_store.h"
#include "temporal/temporal_types.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace themisdb {
namespace temporal {

// ============================================================================
// BloomFilter — per-VersionBlock O(1) timestamp membership test
// ============================================================================

/**
 * @brief Simple Bloom filter over int64_t (Timestamp) values.
 *
 * Uses 3 independent hash functions derived from Murmur3-style finalizers.
 * Default sizing: 8 bits per expected element → ~2.4 % false-positive rate.
 *
 * Used in VersionBlock to short-circuit getAsOf() calls that cannot possibly
 * match (warm-tier miss detection).
 */
class BloomFilter {
public:
    /// Construct for `expected_elements` items with `bits_per_elem` bits.
    explicit BloomFilter(size_t expected_elements, size_t bits_per_elem = 8);

    BloomFilter() : BloomFilter(64) {}

    void add(int64_t value) noexcept;
    bool mightContain(int64_t value) const noexcept;

    size_t bitCount() const noexcept { return num_bits_; }

private:
    std::vector<uint64_t> bits_;
    size_t num_bits_;

    static uint64_t h1(uint64_t x) noexcept;
    static uint64_t h2(uint64_t x) noexcept;
    static uint64_t h3(uint64_t x) noexcept;

    void setBit(size_t idx) noexcept;
    bool testBit(size_t idx) const noexcept;
};

// ============================================================================
// VersionBlock — immutable warm-tier L0 block
// ============================================================================

/**
 * @brief Immutable block of historical VersionedDocument entries.
 *
 * Analogous to an L0 SST file in RocksDB:
 * - Created by flushing the oldest versions from the hot-tier MemTable.
 * - Immutable once written — never mutated, only read or evicted.
 * - Carries metadata (min_start, max_end) for O(1) range pruning.
 * - Carries a BloomFilter over sys_start timestamps for O(1) miss detection.
 *
 * Entries are stored as JSON strings sorted ascending by sys_start.
 */
struct VersionBlock {
    std::string doc_key;
    Timestamp   min_start{kMaxTimestamp};  ///< Earliest sys_start in block
    Timestamp   max_end{kMinTimestamp};    ///< Latest sys_end   in block
    size_t      version_count{0};

    /// Serialised VersionedDocument entries, sorted ascending by sys_start.
    std::vector<std::string> entries;

    /// Bloom filter over sys_start values — fast miss detection.
    BloomFilter bloom;

    /// Approximate RAM cost of this block (sum of entry string sizes + overhead).
    uint64_t approx_bytes{0};

    VersionBlock() : bloom(64) {}
};

// ============================================================================
// TierDecision
// ============================================================================

/** Result returned by TierPolicy::evaluate(). */
enum class TierDecision {
    KEEP,               ///< No tier change needed
    FLUSH_HOT_TO_WARM,  ///< Move oldest (hot - hot_max) versions to warm
    FLUSH_WARM_TO_COLD  ///< Move oldest warm block(s) to cold
};

/**
 * @brief Input snapshot passed to the tier-decision function.
 *
 * Contains all observable signals that a threshold-based rule or an ML
 * model needs to emit a TierDecision.
 */
struct TierDecisionContext {
    std::string table_name;
    std::string doc_key;

    size_t   hot_version_count{0};   ///< Current hot-tier version count for key
    size_t   warm_block_count{0};    ///< Current warm-tier block count for key
    size_t   warm_versions_for_key{0};
    uint64_t warm_bytes_for_key{0};  ///< RAM used by warm blocks for this key
    uint64_t total_warm_bytes{0};    ///< Total RAM used by ALL warm blocks

    Timestamp oldest_hot_sys_start{kMaxTimestamp};
    Timestamp now_ts{0};

    /// Ratio of `total_warm_bytes` to the configured `max_warm_bytes` (0.0–1.0+)
    double warm_pressure{0.0};
};

// ============================================================================
// TierPolicy
// ============================================================================

/**
 * @brief Configuration and decision logic for the three-tier manager.
 *
 * ## Default behaviour (threshold-based)
 *
 * - `hot_max_versions_per_key`:  max closed versions kept in the hot tier.
 *   When exceeded → FLUSH_HOT_TO_WARM.
 * - `warm_max_blocks_per_key`:   max VersionBlocks kept in the warm tier.
 *   When exceeded → FLUSH_WARM_TO_COLD.
 * - `max_warm_bytes`:            global RAM budget for all warm blocks.
 *   When exceeded → FLUSH_WARM_TO_COLD (evict oldest block, any key).
 * - `cold_after_age`:            versions older than this are ineligible for
 *   the hot tier regardless of count → FLUSH_HOT_TO_WARM.
 * - `warm_block_size`:           target number of versions per VersionBlock.
 *
 * ## Future hook — LLM / LoRA autonomous decisions
 *
 * Set `decision_fn` to replace the threshold logic with a custom callable.
 * The callable receives a fully-populated TierDecisionContext and must
 * return a TierDecision synchronously (it will be called while the caller
 * holds a write lock, so it must be fast — no blocking I/O).
 *
 * Planned wiring with ThemisDB LoRA advisor:
 *
 *   policy.decision_fn = [&router](const TierDecisionContext& ctx) {
 *       return router.adviseTierDecision(ctx);  // future API
 *   };
 *
 * Until the LoRA advisor is integrated, leave `decision_fn` empty (nullptr)
 * and the built-in evaluate() is used automatically.
 */
struct TierPolicy {
    // ── Hot tier ──────────────────────────────────────────────────────────
    /// Max closed (historical) versions kept in the hot tier per key.
    size_t hot_max_versions_per_key = 100;

    // ── Warm tier ─────────────────────────────────────────────────────────
    /// Max VersionBlocks kept in the warm tier per key before cold flush.
    size_t warm_max_blocks_per_key = 8;

    /// Target number of versions per VersionBlock (flush granularity).
    size_t warm_block_size = 50;

    /// Global RAM budget for all warm-tier blocks combined.
    uint64_t max_warm_bytes = 64ULL * 1024 * 1024;  // 64 MB

    // ── Age-based demotion ────────────────────────────────────────────────
    /// Versions whose sys_end is older than this are flush-eligible
    /// regardless of the hot version count.  0 = disabled.
    std::chrono::milliseconds cold_after_age{
        std::chrono::hours(24 * 30)};  // 30 days

    // ── Background compaction ─────────────────────────────────────────────
    bool     auto_compact = true;
    std::chrono::milliseconds compact_interval{30'000};  // 30 s

    // ── Future LLM / LoRA decision hook ───────────────────────────────────
    /**
     * Optional autonomous tier-decision function.
     *
     * When set, this callable completely replaces the built-in threshold
     * logic in evaluate().  It receives a fully-populated
     * TierDecisionContext and must return a TierDecision **synchronously**.
     *
     * Planned future use: wired to the ThemisDB LoRA adapter router so
     * that a fine-tuned model observes workload signals (access frequency,
     * RAM pressure, query patterns) and makes per-key tier decisions
     * autonomously without hard-coded thresholds.
     *
     * Leave nullptr to use the built-in threshold policy (current default).
     */
    std::function<TierDecision(const TierDecisionContext&)> decision_fn;

    /// Built-in threshold-based evaluation.  Called by evaluate() when
    /// decision_fn is nullptr.
    TierDecision evaluate(const TierDecisionContext& ctx) const;
};

// ============================================================================
// TemporalTierManager
// ============================================================================

/**
 * @brief Three-tier LSM-style manager for VersionedDocument history.
 *
 * Implements the RocksDB philosophy for temporal data:
 *
 *   Write path:
 *     insert() → hot (MemTable) → [flush trigger] → warm (L0 blocks) →
 *     [compaction trigger] → cold (disk SST via TemporalColdStore)
 *
 *   Read path (getAsOf):
 *     1. Hot tier:  O(log h)  — upper_bound on sorted map
 *     2. Warm tier: O(b)      — per-block: Bloom check → range check → scan
 *     3. Cold tier: O(log N)  — RAM index upper_bound + 1 disk read
 *
 * ## Concurrency model
 *
 * Uses `std::shared_mutex`:
 * - Reads (getAsOf, getHistory) hold shared lock → parallel readers.
 * - Writes (insert, flush, compact) hold exclusive lock.
 * - Background compaction thread acquires exclusive lock per key batch.
 *
 * ## Usage
 *
 *   auto cold = std::make_shared<TemporalColdStore>(
 *       std::make_unique<FileSystemBackend>("/data/cold"));
 *
 *   TierPolicy policy;
 *   policy.hot_max_versions_per_key = 200;
 *   policy.max_warm_bytes           = 128 * 1024 * 1024;
 *
 *   TemporalTierManager mgr(policy, cold);
 *   mgr.startCompactionWorker();
 *
 *   mgr.insert("orders", doc);
 *   auto v = mgr.getAsOf("orders", "k1", t);
 *
 * Thread-safety: all public methods are thread-safe.
 */
class TemporalTierManager {
public:
    explicit TemporalTierManager(
        TierPolicy                         policy     = {},
        std::shared_ptr<TemporalColdStore> cold_store = nullptr);

    ~TemporalTierManager();

    // Non-copyable, non-movable (owns mutex + thread)
    TemporalTierManager(const TemporalTierManager&)            = delete;
    TemporalTierManager& operator=(const TemporalTierManager&) = delete;

    // ── Write path ────────────────────────────────────────────────────────

    /**
     * @brief Insert a VersionedDocument into the hot tier.
     *
     * The version is inserted into the sorted hot-tier map for its key.
     * After insertion the tier-decision is evaluated; if FLUSH_HOT_TO_WARM
     * or FLUSH_WARM_TO_COLD is returned the corresponding operation is
     * performed synchronously before returning.
     *
     * @return false if doc.isCurrent() is true (current versions belong
     *         to the live table, not the history tiers).
     */
    bool insert(const std::string& table_name, const VersionedDocument& doc);

    // ── Read path ─────────────────────────────────────────────────────────

    /**
     * @brief Return the version valid at timestamp as_of.
     *
     * Queries hot → warm → cold in order, returning the first match.
     */
    std::optional<VersionedDocument> getAsOf(const std::string& table_name,
                                             const std::string& doc_key,
                                             Timestamp as_of) const;

    /**
     * @brief Return all stored historical versions, sorted by sys_start.
     *
     * Merges hot + warm + cold tiers.
     */
    std::vector<VersionedDocument> getHistory(const std::string& table_name,
                                              const std::string& doc_key) const;

    /**
     * @brief Return versions whose sys_time overlaps range,
     *        sorted by sys_start.
     */
    std::vector<VersionedDocument> getHistoryInRange(
        const std::string& table_name,
        const std::string& doc_key,
        const TimeRange&   range) const;

    // ── Compaction ────────────────────────────────────────────────────────

    /**
     * @brief Flush oldest hot-tier versions to a new warm VersionBlock.
     *
     * Moves the oldest `(hot_count - policy.hot_max_versions_per_key)`
     * closed versions from the hot map into a new VersionBlock appended to
     * the warm tier.
     *
     * @return Number of versions moved.
     */
    size_t flushHotToWarm(const std::string& table_name,
                          const std::string& doc_key);

    /**
     * @brief Flush the oldest warm VersionBlock(s) to cold.
     *
     * Moves versions from the oldest warm block(s) into TemporalColdStore.
     *
     * @return Number of versions moved.
     */
    size_t flushWarmToCold(const std::string& table_name,
                           const std::string& doc_key);

    /**
     * @brief Compact all keys in table_name according to current policy.
     * @return Total versions moved across all tiers.
     */
    size_t compactTable(const std::string& table_name);

    // ── Policy ────────────────────────────────────────────────────────────

    void setPolicy(const TierPolicy& policy);
    const TierPolicy& policy() const noexcept { return policy_; }

    // ── Background worker ─────────────────────────────────────────────────

    /// Start the periodic background compaction thread (idempotent).
    void startCompactionWorker();

    /// Stop the background compaction thread and wait for it to exit.
    void stopCompactionWorker();

    // ── Observability ─────────────────────────────────────────────────────

    /** Tier counts for a single (table, key) pair. */
    struct KeyTierStats {
        size_t   hot_versions{0};
        size_t   warm_blocks{0};
        size_t   warm_versions{0};
        uint64_t warm_bytes{0};
        size_t   cold_versions{0};
    };

    KeyTierStats keyStats(const std::string& table_name,
                          const std::string& doc_key) const;

    /** Aggregate tier stats for table_name. */
    struct TableTierStats {
        size_t   total_hot_versions{0};
        size_t   total_warm_blocks{0};
        size_t   total_warm_versions{0};
        uint64_t total_warm_bytes{0};
        size_t   total_cold_versions{0};
        size_t   flush_hot_to_warm_count{0};
        size_t   flush_warm_to_cold_count{0};
    };

    TableTierStats tableStats(const std::string& table_name) const;

    nlohmann::json statsJson(const std::string& table_name) const;

private:
    TierPolicy                         policy_;
    std::shared_ptr<TemporalColdStore> cold_;

    // Hot tier: table → doc_key → sys_start-sorted map of closed versions
    using HotMap  = std::map<Timestamp, VersionedDocument>;
    using KeyHotMap = std::map<std::string, HotMap>;
    std::map<std::string, KeyHotMap> hot_;

    // Warm tier: table → doc_key → list of VersionBlocks (ascending min_start)
    using WarmBlocks = std::vector<VersionBlock>;
    using KeyWarmMap = std::map<std::string, WarmBlocks>;
    std::map<std::string, KeyWarmMap> warm_;

    std::atomic<uint64_t> total_warm_bytes_{0};

    // Cumulative stats
    std::atomic<size_t> stat_flush_hot_warm_{0};
    std::atomic<size_t> stat_flush_warm_cold_{0};

    mutable std::shared_mutex mutex_;

    // Background compaction
    std::thread             compact_thread_;
    std::atomic<bool>       compact_stop_{false};
    std::mutex              compact_cv_mutex_;
    std::condition_variable compact_cv_;

    void compactionLoop();

    // ── Internal helpers (caller must hold appropriate lock) ──────────────

    /// Build a TierDecisionContext for (table, key).  Lock must be held.
    TierDecisionContext makeContext(const std::string& table_name,
                                    const std::string& doc_key) const;

    /// Flush hot → warm for key.  Exclusive lock must be held.
    size_t flushHotToWarmLocked(const std::string& table_name,
                                 const std::string& doc_key,
                                 HotMap&            hot_map,
                                 WarmBlocks&        warm_blocks);

    /// Flush oldest warm block → cold for key.  Exclusive lock must be held.
    size_t flushWarmToColdLocked(const std::string& table_name,
                                  const std::string& doc_key,
                                  WarmBlocks&        warm_blocks);

    /// Build an immutable VersionBlock from a sorted vector of documents.
    static VersionBlock makeBlock(const std::string& doc_key,
                                   std::vector<VersionedDocument> versions);

    /// Search a single VersionBlock for the version containing as_of.
    static std::optional<VersionedDocument>
    searchBlock(const VersionBlock& block, Timestamp as_of);

    /// Collect all versions from a VersionBlock.
    static std::vector<VersionedDocument>
    allFromBlock(const VersionBlock& block);

    /// Collect overlapping versions from a VersionBlock.
    static std::vector<VersionedDocument>
    rangeFromBlock(const VersionBlock& block, const TimeRange& range);
};

} // namespace temporal
} // namespace themisdb
