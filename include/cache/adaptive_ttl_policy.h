/**
 * @file adaptive_ttl_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace cache {

// ============================================================================
// AdaptiveTTLPolicyConfig — policy configuration
// ============================================================================

struct AdaptiveTTLPolicyConfig {
    std::chrono::milliseconds minTTL{1'000};

    std::chrono::milliseconds maxTTL{3'600'000};

    uint32_t access_window_size = 64;

    double aggressiveness = 2.0;

    double decay_factor = 0.9;

    int64_t max_history_age_ms = 86'400'000LL;
};

// ============================================================================
// AccessRecord — single recorded access event
// ============================================================================

struct AccessRecord {
    int64_t timestamp_ms = 0; ///< Wall-clock time of the access (ms since epoch).
    bool    is_hit       = true;  ///< true = cache hit; false = cache miss (new put).
};

// ============================================================================
// AdaptiveTTLSuggestion — output from computeTTL
// ============================================================================

struct AdaptiveTTLSuggestion {
    std::chrono::milliseconds ttl{0};

    std::chrono::milliseconds mean_access_interval{0};

    uint32_t sample_count = 0;

    double confidence = 0.0;
};

// ============================================================================
// IAdaptiveTTLPolicy — stateful adaptive TTL policy interface
// ============================================================================

struct IAdaptiveTTLPolicy {
    /**
     * @brief IAdaptive TTLPolicy.
     * @return Return value.
     */
    virtual ~IAdaptiveTTLPolicy() = default;

    // -----------------------------------------------------------------------
    // Access recording
    // -----------------------------------------------------------------------

    virtual void recordAccess(const std::string& key,
                              int64_t            timestamp_ms,
                              bool               is_hit = true) noexcept = 0;

    // -----------------------------------------------------------------------
    // TTL computation
    // -----------------------------------------------------------------------

    /**
     * @brief Compute TTL.
     * @param[in] key Input parameter.
     * @param[in] now_ms Input parameter.
     * @return Return value.
     */
    virtual AdaptiveTTLSuggestion computeTTL(const std::string& key,
                                             int64_t            now_ms) const = 0;

    // -----------------------------------------------------------------------
    // History management
    // -----------------------------------------------------------------------

    /**
     * @brief Return a bounded slice of the recent retention action history.
     * @param[in] key Input parameter.
     * @return Most recent actions, or the full history when the limit is zero or oversized.
     */
    virtual std::vector<AccessRecord> getHistory(
        const std::string& key) const = 0;

    /**
     * @brief Evict.
     * @param[in] key Input parameter.
     */
    virtual void evict(const std::string& key) = 0;

    /**
     * @brief Prune History.
     * @param[in] now_ms Input parameter.
     * @param[in] max_age_ms Input parameter.
     * @return Return value.
     */
    virtual uint64_t pruneHistory(int64_t now_ms, int64_t max_age_ms) = 0;

    /**
     * @brief Flush History.
     */
    virtual void flushHistory() = 0;

    // -----------------------------------------------------------------------
    // Configuration & observability
    // -----------------------------------------------------------------------

    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     */
    virtual void configure(const AdaptiveTTLPolicyConfig& config) = 0;

    /**
     * @brief Get Config.
     * @return Return value.
     */
    virtual AdaptiveTTLPolicyConfig getConfig() const = 0;

    /**
     * @brief Tracked Key Count.
     * @return Return value.
     */
    virtual size_t trackedKeyCount() const = 0;
};

} // namespace cache
} // namespace themis
