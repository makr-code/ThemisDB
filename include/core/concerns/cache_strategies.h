/**
 * @file cache_strategies.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <memory>
#include <optional>

namespace themis {
namespace core {
namespace concerns {

class IEvictionStrategy {
public:
    /**
     * @brief IEviction Strategy.
     * @return Return value.
     */
    virtual ~IEvictionStrategy() = default;

    /**
     * @brief On Access.
     * @param[in] key Input parameter.
     */
    virtual void onAccess(std::string_view key) = 0;

    /**
     * @brief On Insert.
     * @param[in] key Input parameter.
     * @param[in] timestamp_ms Input parameter.
     */
    virtual void onInsert(std::string_view key, uint64_t timestamp_ms) = 0;

    /**
     * @brief On Remove.
     * @param[in] key Input parameter.
     */
    virtual void onRemove(std::string_view key) = 0;

    /**
     * @brief Select Victim.
     * @return Return value.
     */
    virtual std::optional<std::string> selectVictim() = 0;

    /**
     * @brief Clear.
     */
    virtual void clear() = 0;

    /**
     * @brief Size.
     * @return Return value.
     */
    virtual size_t size() const = 0;

    /**
     * @brief Get Name.
     * @return Return value.
     */
    virtual std::string_view getName() const = 0;
};

struct CacheMetrics {
    uint64_t hit_count{0};
    uint64_t miss_count{0};
    uint64_t eviction_count{0};
    uint64_t insertion_count{0};
    size_t current_size{0};
    size_t max_size{0};
    uint64_t total_latency_ns{0};  // Cumulative operation latency
    
    double hitRate() const {
        uint64_t total = hit_count + miss_count;
        return total > 0 ? static_cast<double>(hit_count) / total : 0.0;
    }
    
    double avgLatencyNs() const {
        uint64_t total_ops = hit_count + miss_count;
        return total_ops > 0 ? static_cast<double>(total_latency_ns) / total_ops : 0.0;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
