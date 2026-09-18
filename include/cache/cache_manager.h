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

struct CacheManagerConfig {
    size_t default_cache_size = 1000;
    size_t default_max_bytes = 0;
    bool enable_stats = true;
    int num_shards = 4;
    bool enable_compression = false;
};

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

class CacheManager {
public:
    using EventHandler = std::function<void(const CacheEvent&)>;

    /**
     * @brief Cache Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CacheManager(const CacheManagerConfig& config);

    ~CacheManager() noexcept;

    // Move semantics
    CacheManager(CacheManager&& other) noexcept;

    CacheManager& operator=(CacheManager&& other) noexcept;

    // No copy
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    // --- Cache registration ---

    bool register_cache(const std::string& cache_name, size_t size = 0);

    /**
     * @brief Unregister cache.
     * @param[in] cache_name Name of the cache.
     * @return True when the operation succeeds.
     */
    bool unregister_cache(const std::string& cache_name);

    /**
     * @brief Get cache names.
     * @return Return value.
     */
    std::vector<std::string> get_cache_names() const;

    /**
     * @brief --- Policy management ---
     * @param[in] cache_name Name of the cache.
     * @param[in] policy Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    bool set_eviction_policy(const std::string& cache_name, 
                             CacheEvictionPolicy&& policy) noexcept;

    /**
     * @brief Get eviction policy.
     * @param[in] cache_name Name of the cache.
     * @return Pointer to the result.
     */
    const CacheEvictionPolicy* get_eviction_policy(const std::string& cache_name) const;

    /**
     * @brief --- Event handling ---
     * @param[in] handler Input parameter.
     * @return Return value.
     */

    uint32_t register_event_handler(EventHandler&& handler);

    /**
     * @brief Unregister event handler.
     * @param[in] handler_id Identifier of the handler.
     * @return True when the operation succeeds.
     */
    bool unregister_event_handler(uint32_t handler_id);

    /**
     * @brief Dispatch event.
     * @param[in] event Input parameter.
     * @note Exception safety: noexcept.
     */
    void dispatch_event(const CacheEvent& event) noexcept;

    // --- Statistics ---

    struct CacheStats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        size_t size = 0;
        size_t capacity = 0;
        double hit_rate = 0.0;
    };

    /**
     * @brief Get cache stats.
     * @param[in] cache_name Name of the cache.
     * @return Return value.
     */
    std::optional<CacheStats> get_cache_stats(const std::string& cache_name) const;

    /**
     * @brief Clear all.
     */
    void clear_all();

    const CacheManagerConfig& get_config() const noexcept { return config_; }

    bool is_moved_from() const noexcept { return is_moved_from_; }

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

    /**
     * @brief Cleanup.
     * @note Exception safety: noexcept.
     */
    void cleanup() noexcept;
};

} // namespace cache
} // namespace themis
