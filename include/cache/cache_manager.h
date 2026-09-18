/**
 * @file cache_manager.h
 * @brief Cache manager with policy objects and move semantics
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Gap Categories: CWE-457 (uninitialized variable), CWE-672 (use-after-free)
 * 
 * Provides:
 * - Centralized cache policy management
 * - Move-enabled policy objects
 * - Event handler callbacks with move semantics
 * - Multi-cache coordination with moved-from state tracking
 * 
 * @see ThemisDB Cache Module Roadmap: src/cache/ROADMAP.md
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <optional>

namespace themis {
namespace cache {

// Forward declarations
class CacheEvictionPolicy;

/**
 * @brief Cache manager configuration
 */
struct CacheManagerConfig {
    size_t default_cache_size = 1000;
    size_t default_max_bytes = 0;
    bool enable_stats = true;
    int num_shards = 4;
    bool enable_compression = false;
};

/**
 * @brief Event raised when cache operation occurs
 */
struct CacheEvent {
    enum Type {
        MISS,           ///< Cache miss occurred
        HIT,            ///< Cache hit occurred
        EVICTION,       ///< Entry evicted
        CLEAR,          ///< Cache cleared
        POLICY_CHANGE,  ///< Eviction policy changed
    };

    Type type;
    std::string cache_name;
    std::string key;
    int64_t timestamp_us = 0;
};

/**
 * @brief Cache manager with policy coordination
 * 
 * Central management of multiple caches with:
 * - Policy object registration and move semantics
 * - Event handling with callback chaining
 * - Moved-from state validation
 * - Coordinated eviction across shards
 */
class CacheManager {
public:
    /**
     * @brief Event handler callback type
     */
    using EventHandler = std::function<void(const CacheEvent&)>;

    /**
     * @brief Create cache manager with configuration
     * 
     * @param config Manager configuration
     * @throws std::invalid_argument If config is invalid
     */
    explicit CacheManager(const CacheManagerConfig& config);

    /**
     * @brief Destructor - releases all managed caches and policies
     */
    ~CacheManager() noexcept;

    // Move semantics
    /**
     * @brief Move constructor
     * 
     * @param other Manager to move from
     * 
     * Transfers all caches, policies, and event handlers.
     * `other` becomes moved-from state (safe for destruction/reassignment).
     * 
     * @post other.is_moved_from() == true
     */
    CacheManager(CacheManager&& other) noexcept;

    /**
     * @brief Move assignment operator
     * 
     * @param other Manager to move from
     * @return Reference to this manager
     * 
     * Release-and-acquire:
     * - Clears current manager state
     * - Acquires all caches and policies from `other`
     * - `other` becomes moved-from state
     * 
     * @post other.is_moved_from() == true
     */
    CacheManager& operator=(CacheManager&& other) noexcept;

    // No copy
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    // --- Cache registration ---

    /**
     * @brief Register new cache with manager
     * 
     * @param cache_name Name for this cache instance
     * @param size Maximum entries (0 = use default)
     * @return true if registered, false if name already exists
     * @throws std::logic_error If called on moved-from manager
     */
    bool register_cache(const std::string& cache_name, size_t size = 0);

    /**
     * @brief Unregister cache
     * 
     * @param cache_name Cache name
     * @return true if unregistered, false if not found
     * @throws std::logic_error If called on moved-from manager
     */
    bool unregister_cache(const std::string& cache_name);

    /**
     * @brief Get registered cache names
     * 
     * @return Vector of cache names
     */
    std::vector<std::string> get_cache_names() const;

    // --- Policy management ---

    /**
     * @brief Set eviction policy for cache
     * 
     * @param cache_name Target cache name
     * @param policy Eviction policy (moved into manager)
     * @return true if policy set, false if cache not found
     * @throws std::logic_error If called on moved-from manager
     * 
     * Policies are moved to manager for ownership.
     */
    bool set_eviction_policy(const std::string& cache_name, 
                             CacheEvictionPolicy&& policy) noexcept;

    /**
     * @brief Get eviction policy for cache
     * 
     * @param cache_name Cache name
     * @return Pointer to policy, or nullptr if not found
     */
    const CacheEvictionPolicy* get_eviction_policy(const std::string& cache_name) const;

    // --- Event handling ---

    /**
     * @brief Register event handler
     * 
     * @param handler Callback to invoke on cache events
     * @return Handler ID (for later unregistration)
     * @throws std::logic_error If called on moved-from manager
     */
    uint32_t register_event_handler(EventHandler&& handler);

    /**
     * @brief Unregister event handler
     * 
     * @param handler_id ID returned from register_event_handler
     * @return true if unregistered, false if ID not found
     */
    bool unregister_event_handler(uint32_t handler_id);

    /**
     * @brief Dispatch event to all registered handlers
     * 
     * @param event Event to dispatch
     * @throws std::logic_error If called on moved-from manager
     */
    void dispatch_event(const CacheEvent& event) noexcept;

    // --- Statistics ---

    /**
     * @brief Get statistics for cache
     * 
     * @param cache_name Cache name
     * @return Statistics object if cache found, std::nullopt otherwise
     */
    struct CacheStats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        size_t size = 0;
        size_t capacity = 0;
        double hit_rate = 0.0;
    };

    std::optional<CacheStats> get_cache_stats(const std::string& cache_name) const;

    /**
     * @brief Clear all caches
     * 
     * @throws std::logic_error If called on moved-from manager
     */
    void clear_all();

    /**
     * @brief Get manager configuration
     * 
     * @return Current config
     */
    const CacheManagerConfig& get_config() const noexcept { return config_; }

    /**
     * @brief Check if manager is in moved-from state
     * 
     * @return true if resources have been moved out
     */
    bool is_moved_from() const noexcept { return is_moved_from_; }

    /**
     * @brief Check if manager is valid (not moved-from)
     * 
     * @return true if manager is operational
     */
    bool is_valid() const noexcept { return !is_moved_from_; }

private:
    struct CacheEntry {
        std::string name;
        size_t size;
        std::unique_ptr<CacheEvictionPolicy> policy;
    };

    struct EventHandlerEntry {
        uint32_t id = 0;
        EventHandler handler;
    };

    CacheManagerConfig config_;
    std::unordered_map<std::string, CacheEntry> caches_;
    std::vector<EventHandlerEntry> event_handlers_;
    uint32_t next_handler_id_;
    bool is_moved_from_;

    void cleanup() noexcept;
};

} // namespace cache
} // namespace themis
