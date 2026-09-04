/**
 * @file cache_eviction_policy.h
 * @brief Polymorphic eviction policy base and implementations with move semantics
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Gap Categories: CWE-672 (use-after-free), CWE-457 (uninitialized variable)
 * 
 * Provides:
 * - Polymorphic eviction strategy interface
 * - Move-enabled policy objects
 * - Policy state tracking and moved-from detection
 * - Multiple concrete strategies (LRU, LFU, FIFO, ARC)
 * 
 * @see ThemisDB Cache Module Roadmap: src/cache/ROADMAP.md
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace cache {

/**
 * @brief Policy-agnostic cache key descriptor
 */
struct CacheKeyDescriptor {
    std::string key;
    size_t access_count;
    int64_t last_access_ns;
    int64_t creation_time_ns;
};

/**
 * @brief Abstract eviction policy base class with move semantics
 * 
 * Defines interface for cache eviction policies with:
 * - Virtual destructor for polymorphic cleanup
 * - Move-only semantics (prevents aliasing bugs with virtual calls)
 * - Moved-from state validation
 * - Pure virtual interface for subclasses
 */
class CacheEvictionPolicy {
public:
    /**
     * @brief Eviction decision result
     */
    struct EvictionDecision {
        bool should_evict = 0;      ///< true if key should be evicted
        std::string victim_key; ///< Selected victim key when should_evict is true
        std::string reason;     ///< Human-readable reason
    };

    /**
     * @brief Virtual destructor for polymorphic cleanup
     */
    virtual ~CacheEvictionPolicy() noexcept = default;

    // Move semantics
    /**
     * @brief Move constructor
     * 
     * @param other Policy to move from
     * 
     * Transfers policy state. `other` becomes moved-from state.
     * Note: This is the default implementation; subclasses may override.
     */
    CacheEvictionPolicy(CacheEvictionPolicy&& other) noexcept = default;

    /**
     * @brief Move assignment operator
     * 
     * @param other Policy to move from
     * @return Reference to this policy
     * 
     * Transfers policy state. `other` becomes moved-from state.
     */
    CacheEvictionPolicy& operator=(CacheEvictionPolicy&& other) noexcept = default;

    // No copy
    CacheEvictionPolicy(const CacheEvictionPolicy&) = delete;
    CacheEvictionPolicy& operator=(const CacheEvictionPolicy&) = delete;

    // --- Policy interface ---

    /**
     * @brief Record a cache hit
     * 
     * Update policy statistics when a key is accessed successfully.
     * 
     * @param key Accessed key
     * @throws std::logic_error If called on moved-from policy
     */
    virtual void record_hit(const std::string& key) = 0;

    /**
     * @brief Record a cache miss
     * 
     * Update policy statistics when a key is not found.
     * 
     * @param key Missing key
     * @throws std::logic_error If called on moved-from policy
     */
    virtual void record_miss(const std::string& key) = 0;

    /**
     * @brief Record cache insertion
     * 
     * Update policy when new item is added.
     * 
     * @param key Inserted key
     * @param size Item size in bytes
     * @throws std::logic_error If called on moved-from policy
     */
    virtual void record_insert(const std::string& key, size_t size) = 0;

    /**
     * @brief Record cache deletion
     * 
     * Update policy when item is removed.
     * 
     * @param key Deleted key
     * @throws std::logic_error If called on moved-from policy
     */
    virtual void record_delete(const std::string& key) = 0;

    /**
     * @brief Decide which key to evict
     * 
     * @param candidates Vector of candidate keys for eviction
     * @return EvictionDecision indicating which key to evict (if any)
     * @throws std::logic_error If called on moved-from policy
     * 
     * Must return empty/false decision if candidates is empty.
     */
    virtual EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) = 0;

    /**
     * @brief Get policy type name
     * 
     * @return Human-readable policy name (e.g., "LRU", "LFU")
     */
    virtual const char* policy_name() const noexcept = 0;

    /**
     * @brief Clone this policy (if needed for transfer)
     * 
     * @return Unique pointer to new policy with same configuration
     * 
     * Default implementation throws; subclasses override if cloning needed.
     */
    virtual std::unique_ptr<CacheEvictionPolicy> clone() const {
        throw std::runtime_error(std::string(policy_name()) + " does not support cloning");
    }

    /**
     * @brief Check if policy is in moved-from state
     * 
     * @return true if resources have been moved out
     */
    virtual bool is_moved_from() const noexcept = 0;

protected:
    CacheEvictionPolicy() = default;
};

/**
 * @brief Least Recently Used (LRU) eviction policy
 * 
 * Evicts the entry that was accessed longest time ago.
 */
class LRUEvictionPolicy : public CacheEvictionPolicy {
public:
    /**
     * @brief Create LRU eviction policy
     */
    LRUEvictionPolicy() = default;

    // Move semantics (mark source as moved-from so runtime guards remain correct)
    LRUEvictionPolicy(LRUEvictionPolicy&& other) noexcept;
    LRUEvictionPolicy& operator=(LRUEvictionPolicy&& other) noexcept;

    // No copy
    LRUEvictionPolicy(const LRUEvictionPolicy&) = delete;
    LRUEvictionPolicy& operator=(const LRUEvictionPolicy&) = delete;

    void record_hit(const std::string& key) override;
    void record_miss(const std::string& key) override;
    void record_insert(const std::string& key, size_t size) override;
    void record_delete(const std::string& key) override;
    EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) override;
    const char* policy_name() const noexcept override { return "LRU"; }
    std::unique_ptr<CacheEvictionPolicy> clone() const override;
    bool is_moved_from() const noexcept override { return is_moved_from_; }

private:
    bool is_moved_from_ = false;
};

/**
 * @brief Least Frequently Used (LFU) eviction policy
 * 
 * Evicts the entry with lowest access frequency.
 */
class LFUEvictionPolicy : public CacheEvictionPolicy {
public:
    /**
     * @brief Create LFU eviction policy
     * 
     * @param aging_factor How quickly old accesses decay (0.0-1.0)
     */
    explicit LFUEvictionPolicy(double aging_factor = 0.5);

    LFUEvictionPolicy(LFUEvictionPolicy&& other) noexcept;
    LFUEvictionPolicy& operator=(LFUEvictionPolicy&& other) noexcept;

    LFUEvictionPolicy(const LFUEvictionPolicy&) = delete;
    LFUEvictionPolicy& operator=(const LFUEvictionPolicy&) = delete;

    void record_hit(const std::string& key) override;
    void record_miss(const std::string& key) override;
    void record_insert(const std::string& key, size_t size) override;
    void record_delete(const std::string& key) override;
    EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) override;
    const char* policy_name() const noexcept override { return "LFU"; }
    std::unique_ptr<CacheEvictionPolicy> clone() const override;
    bool is_moved_from() const noexcept override { return is_moved_from_; }

private:
    double aging_factor_;
    bool is_moved_from_ = false;
};

/**
 * @brief FIFO (First-In-First-Out) eviction policy
 * 
 * Evicts the oldest entry (by creation time).
 */
class FIFOEvictionPolicy : public CacheEvictionPolicy {
public:
    /**
     * @brief Create FIFO eviction policy
     */
    FIFOEvictionPolicy() = default;

    FIFOEvictionPolicy(FIFOEvictionPolicy&& other) noexcept;
    FIFOEvictionPolicy& operator=(FIFOEvictionPolicy&& other) noexcept;

    FIFOEvictionPolicy(const FIFOEvictionPolicy&) = delete;
    FIFOEvictionPolicy& operator=(const FIFOEvictionPolicy&) = delete;

    void record_hit(const std::string& key) override;
    void record_miss(const std::string& key) override;
    void record_insert(const std::string& key, size_t size) override;
    void record_delete(const std::string& key) override;
    EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) override;
    const char* policy_name() const noexcept override { return "FIFO"; }
    std::unique_ptr<CacheEvictionPolicy> clone() const override;
    bool is_moved_from() const noexcept override { return is_moved_from_; }

private:
    bool is_moved_from_ = false;
};

/**
 * @brief Adaptive Replacement Cache (ARC) policy
 * 
 * Self-tuning eviction strategy combining LRU and LFU benefits.
 */
class ARCEvictionPolicy : public CacheEvictionPolicy {
public:
    /**
     * @brief Create ARC eviction policy
     */
    ARCEvictionPolicy() = default;

    ARCEvictionPolicy(ARCEvictionPolicy&& other) noexcept;
    ARCEvictionPolicy& operator=(ARCEvictionPolicy&& other) noexcept;

    ARCEvictionPolicy(const ARCEvictionPolicy&) = delete;
    ARCEvictionPolicy& operator=(const ARCEvictionPolicy&) = delete;

    void record_hit(const std::string& key) override;
    void record_miss(const std::string& key) override;
    void record_insert(const std::string& key, size_t size) override;
    void record_delete(const std::string& key) override;
    EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) override;
    const char* policy_name() const noexcept override { return "ARC"; }
    std::unique_ptr<CacheEvictionPolicy> clone() const override;
    bool is_moved_from() const noexcept override { return is_moved_from_; }

private:
    size_t arc_p = 0;  // Target size of recent list
    bool is_moved_from_ = false;
};

/**
 * @brief Temperature-aware weighted LRU policy for Phase 3 cache efficiency work.
 *
 * The policy classifies entries into L3, L2, and L1 tiers based on access
 * counts, then prefers evicting the lowest-tier entry with the lowest weighted
 * frequency/recency score. All mutating operations are thread-safe.
 * 
 * Phase 3 Integration (Q4 2026):
 * - Renamed tiers from cold/warm/hot to L3/L2/L1 (canonical access model naming)
 * - Threshold naming: l2_promotion_threshold, l1_promotion_threshold
 * - EvictionListener callbacks integrated via AdaptiveQueryCache
 * - Coordinator signal emission: eviction events → AccessCoordinator
 */
class WeightedTieredLRUEvictionPolicy : public CacheEvictionPolicy {
public:
    /// Cache storage tiers in ascending retention priority (L3=cold → L1=hot).
    enum class Tier : uint8_t { L3 = 0, L2 = 1, L1 = 2 };

    /**
     * @brief Policy tuning parameters (Phase 3: renamed thresholds).
     *
     * Thresholds are access counts that determine tier promotion:
     * - entries with < l2_promotion_threshold accesses stay in L3
     * - entries with >= l2_promotion_threshold accesses promote to L2
     * - entries with >= l1_promotion_threshold accesses promote to L1
     * 
     * Capacity percentages are clamped to sane ranges. Adaptive threshold tuning
     * changes trigger/safe levels at most once per @p threshold_adjustment_interval_ns
     * to avoid oscillation.
     */
    struct Config {
        /// Phase 3: renamed from warm_access_threshold
        size_t l2_promotion_threshold = 2;
        /// Phase 3: renamed from hot_access_threshold
        size_t l1_promotion_threshold = 10;
        double frequency_weight = 0.3;
        double recency_weight = 0.7;
        double frequency_decay_factor = 0.95;
        size_t trigger_threshold_percent = 70;
        size_t safe_threshold_percent = 50;
        size_t severe_threshold_percent = 85;
        bool adaptive_thresholds = true;
        int64_t threshold_adjustment_interval_ns = 600000000000LL;  // 10 minutes
    };

    WeightedTieredLRUEvictionPolicy();
    explicit WeightedTieredLRUEvictionPolicy(Config config);

    WeightedTieredLRUEvictionPolicy(WeightedTieredLRUEvictionPolicy&& other) noexcept;
    WeightedTieredLRUEvictionPolicy& operator=(WeightedTieredLRUEvictionPolicy&& other) noexcept;

    WeightedTieredLRUEvictionPolicy(const WeightedTieredLRUEvictionPolicy&) = delete;
    WeightedTieredLRUEvictionPolicy& operator=(const WeightedTieredLRUEvictionPolicy&) = delete;

    void record_hit(const std::string& key) override;
    void record_miss(const std::string& key) override;
    void record_insert(const std::string& key, size_t size) override;
    void record_delete(const std::string& key) override;
    EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) override;
    const char* policy_name() const noexcept override { return "TIERED_LRU"; }
    std::unique_ptr<CacheEvictionPolicy> clone() const override;
    bool is_moved_from() const noexcept override { return is_moved_from_; }

    /**
     * @brief Return the currently assigned tier for @p key.
     *
     * Unknown keys are treated as cold because they have no retention history.
     */
    Tier tier_for_key(const std::string& key) const;

    /**
     * @brief Compute the weighted score for a tracked key.
     *
     * Higher scores mean the entry is more valuable to keep. Unknown keys return
     * 0.0. When @p now_ns is zero the current steady-clock timestamp is used.
     */
    double score_for_key(const std::string& key, int64_t now_ns = 0) const;

    /**
     * @brief Compute the weighted score for an arbitrary descriptor.
     *
     * Used by tests and by choose_victim() to validate score ordering without
     * mutating policy state.
     */
    double score_for_descriptor(const CacheKeyDescriptor& descriptor, int64_t now_ns = 0) const;

    /**
     * @brief Observe cache fullness and adapt thresholds if enabled.
     *
     * High sustained pressure lowers the trigger threshold; persistently low
     * pressure raises it. Threshold updates are rate-limited to avoid thrashing.
     */
    void observe_capacity(size_t current_capacity_percent, int64_t now_ns = 0);

    /**
     * @brief Recommend how many entries to evict for the current pressure level.
     *
     * Returns 0 below the trigger threshold, 1 between trigger and severe
     * thresholds, and a bounded batch size once severe pressure is reached.
     */
    size_t recommended_batch_size(size_t current_capacity_percent,
                                  size_t candidate_count) const;

    size_t trigger_threshold_percent() const noexcept { return trigger_threshold_percent_; }
    size_t safe_threshold_percent() const noexcept { return safe_threshold_percent_; }
    size_t severe_threshold_percent() const noexcept { return config_.severe_threshold_percent; }

    /**
     * @brief Return tracked entry counts for {cold, warm, hot} tiers.
     */
    std::array<size_t, 3> tier_distribution() const;

private:
    struct EntryState {
        size_t access_count = 0;
        int64_t last_access_ns = 0;
        int64_t creation_time_ns = 0;
        double decayed_frequency = 0.0;
    };

    static int64_t steady_now_ns();
    static size_t clamp_percent(size_t value, size_t min_value, size_t max_value);

    void ensure_operational() const;
    Tier classify_locked(size_t access_count) const;
    double score_locked(const EntryState& state, int64_t now_ns) const;

    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, EntryState> states_;
    size_t trigger_threshold_percent_;
    size_t safe_threshold_percent_;
    int64_t last_threshold_adjustment_ns_ = 0;
    bool is_moved_from_ = false;
};

/**
 * @brief Policy factory for creating eviction policies
 */
class EvictionPolicyFactory {
public:
    /**
     * @brief Create eviction policy by name
     * 
     * @param policy_name Name of policy ("LRU", "LFU", "FIFO", "ARC", "TIERED_LRU")
     * @return Unique pointer to newly created policy
     * @throws std::invalid_argument If policy name not recognized
     */
    static std::unique_ptr<CacheEvictionPolicy> create(const std::string& policy_name);
};

} // namespace cache
} // namespace themis
