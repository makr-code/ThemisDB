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

// ============================================================================
// Eviction listener callback type (Wave D — Q4 2026 AccessCoordinator hooks)
//
// Implementations that call choose_victim() may register one or more
// EvictionListener callbacks to receive notification whenever a key is
// selected for eviction.  The callback is invoked with the victim key and
// the associated tenant_id (empty string for global-namespace entries).
//
// Usage:
//   policy.registerEvictionListener(
//       [](const std::string& key, const std::string& tid) {
//           storage_coordinator.demote(key, tid);
//       });
// ============================================================================

using EvictionListener = std::function<void(const std::string& key,
                                             const std::string& tenant_id)>;

struct CacheKeyDescriptor {
    std::string key;
    size_t access_count;
    int64_t last_access_ns;
    int64_t creation_time_ns;
};

class CacheEvictionPolicy {
public:
    struct EvictionDecision {
        bool should_evict = 0;      ///< true if key should be evicted
        std::string victim_key; ///< Selected victim key when should_evict is true
        std::string reason;     ///< Human-readable reason
    };

    /**
     * @brief Cache Eviction Policy.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    virtual ~CacheEvictionPolicy() noexcept = default;

    // Move semantics
    CacheEvictionPolicy(CacheEvictionPolicy&& other) noexcept = default;

    CacheEvictionPolicy& operator=(CacheEvictionPolicy&& other) noexcept = default;

    // No copy
    CacheEvictionPolicy(const CacheEvictionPolicy&) = delete;
    CacheEvictionPolicy& operator=(const CacheEvictionPolicy&) = delete;

    /**
     * @brief --- Policy interface ---
     * @param[in] key Input parameter.
     */

    virtual void record_hit(const std::string& key) = 0;

    /**
     * @brief Record miss.
     * @param[in] key Input parameter.
     */
    virtual void record_miss(const std::string& key) = 0;

    /**
     * @brief Record insert.
     * @param[in] key Input parameter.
     * @param[in] size Input parameter.
     */
    virtual void record_insert(const std::string& key, size_t size) = 0;

    /**
     * @brief Record delete.
     * @param[in] key Input parameter.
     */
    virtual void record_delete(const std::string& key) = 0;

    /**
     * @brief Choose victim.
     * @param[in] candidates Input parameter.
     * @return Return value.
     */
    virtual EvictionDecision choose_victim(const std::vector<CacheKeyDescriptor>& candidates) = 0;

    /**
     * @brief Policy name.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    virtual const char* policy_name() const noexcept = 0;

    virtual std::unique_ptr<CacheEvictionPolicy> clone() const {
        throw std::runtime_error(std::string(policy_name()) + " does not support cloning");
    }

    /**
     * @brief Is moved from.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    virtual bool is_moved_from() const noexcept = 0;

protected:
    CacheEvictionPolicy() = default;
};

class LRUEvictionPolicy : public CacheEvictionPolicy {
public:
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

class LFUEvictionPolicy : public CacheEvictionPolicy {
public:
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

class FIFOEvictionPolicy : public CacheEvictionPolicy {
public:
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

class ARCEvictionPolicy : public CacheEvictionPolicy {
public:
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

class WeightedTieredLRUEvictionPolicy : public CacheEvictionPolicy {
public:
    enum class Tier : uint8_t { L3 = 0, L2 = 1, L1 = 2 };

    struct Config {
        size_t l2_promotion_threshold = 2;
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
    /**
     * @brief Weighted Tiered LRUEviction Policy.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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
     * @brief Tier for key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    Tier tier_for_key(const std::string& key) const;

    double score_for_key(const std::string& key, int64_t now_ns = 0) const;

    double score_for_descriptor(const CacheKeyDescriptor& descriptor, int64_t now_ns = 0) const;

    void observe_capacity(size_t current_capacity_percent, int64_t now_ns = 0);

    /**
     * @brief Recommended batch size.
     * @param[in] current_capacity_percent Input parameter.
     * @param[in] candidate_count Input parameter.
     * @return Return value.
     */
    size_t recommended_batch_size(size_t current_capacity_percent,
                                  size_t candidate_count) const;

    size_t trigger_threshold_percent() const noexcept { return trigger_threshold_percent_; }
    size_t safe_threshold_percent() const noexcept { return safe_threshold_percent_; }
    size_t severe_threshold_percent() const noexcept { return config_.severe_threshold_percent; }

    /**
     * @brief Register Eviction Listener.
     * @param[in] listener Input parameter.
     */
    void registerEvictionListener(EvictionListener listener);

    std::array<size_t, 3> tier_distribution() const;

private:
    struct EntryState {
        size_t access_count = 0;
        int64_t last_access_ns = 0;
        int64_t creation_time_ns = 0;
        double decayed_frequency = 0.0;
    };

    /**
     * @brief Steady now ns.
     * @return Return value.
     */
    static int64_t steady_now_ns();
    /**
     * @brief Clamp percent.
     * @param[in] value Input parameter.
     * @param[in] min_value Input parameter.
     * @param[in] max_value Input parameter.
     * @return Return value.
     */
    static size_t clamp_percent(size_t value, size_t min_value, size_t max_value);

    /**
     * @brief Ensure operational.
     */
    void ensure_operational() const;
    /**
     * @brief Classify locked.
     * @param[in] access_count Input parameter.
     * @return Return value.
     */
    Tier classify_locked(size_t access_count) const;
    /**
     * @brief Score locked.
     * @param[in] state Input parameter.
     * @param[in] now_ns Input parameter.
     * @return Return value.
     */
    double score_locked(const EntryState& state, int64_t now_ns) const;

    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, EntryState> states_;
    size_t trigger_threshold_percent_;
    size_t safe_threshold_percent_;
    int64_t last_threshold_adjustment_ns_ = 0;
    bool is_moved_from_ = false;
    std::vector<EvictionListener> eviction_listeners_; ///< Storage-demotion callback hooks
};

class EvictionPolicyFactory {
public:
    /**
     * @brief Create.
     * @param[in] policy_name Name of the policy.
     * @return Return value.
     */
    static std::unique_ptr<CacheEvictionPolicy> create(const std::string& policy_name);
};

} // namespace cache
} // namespace themis
