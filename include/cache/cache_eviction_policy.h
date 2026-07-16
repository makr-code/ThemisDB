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
 * @see ThemisDB Remediation Roadmap: Sprint 8 Phase 1C
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <stdexcept>

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
        bool should_evict;      ///< true if key should be evicted
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

    // Move semantics (inherited, but explicitly defined for clarity)
    LRUEvictionPolicy(LRUEvictionPolicy&& other) noexcept = default;
    LRUEvictionPolicy& operator=(LRUEvictionPolicy&& other) noexcept = default;

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

    FIFOEvictionPolicy(FIFOEvictionPolicy&& other) noexcept = default;
    FIFOEvictionPolicy& operator=(FIFOEvictionPolicy&& other) noexcept = default;

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
 * @brief Policy factory for creating eviction policies
 */
class EvictionPolicyFactory {
public:
    /**
     * @brief Create eviction policy by name
     * 
     * @param policy_name Name of policy ("LRU", "LFU", "FIFO", "ARC")
     * @return Unique pointer to newly created policy
     * @throws std::invalid_argument If policy name not recognized
     */
    static std::unique_ptr<CacheEvictionPolicy> create(const std::string& policy_name);
};

} // namespace cache
} // namespace themis
