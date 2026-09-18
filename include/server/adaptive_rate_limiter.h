/**
 * @file adaptive_rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace server {

struct BackendHealthSample {
    std::chrono::milliseconds latency_ms{0};

    bool is_error = false;
};

class AdaptiveRateLimiter {
public:
    struct Config {
        size_t base_capacity = 1000;

        uint64_t high_latency_threshold_ms = 500;

        uint64_t low_latency_threshold_ms = 100;

        double high_error_rate = 0.05;

        double low_error_rate = 0.01;

        double recovery_step = 0.1;

        uint32_t window_seconds = 60;

        size_t min_samples_to_adapt = 10;
    };

    /**
     * @brief Adaptive Rate Limiter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AdaptiveRateLimiter(const Config& config);

    /**
     * @brief Record Sample.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] sample Input parameter.
     */
    void recordSample(const std::string& tenant_id,
                      const BackendHealthSample& sample);

    bool allowRequest(const std::string& tenant_id = "");

    size_t getCurrentCapacity(const std::string& tenant_id = "") const;

    uint64_t getTotalRequests() const {
        return total_requests_.load(std::memory_order_relaxed);
    }

    uint64_t getTotalRejections() const {
        return total_rejections_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:
    // ── Internal types ───────────────────────────────────────────────────────

    struct TimedSample {
        std::chrono::steady_clock::time_point ts;
        std::chrono::milliseconds latency_ms;
        bool is_error;
    };

    struct TenantState {
        std::vector<TimedSample> window;

        size_t current_capacity = {};

        size_t available_tokens = {};

        std::chrono::steady_clock::time_point window_start;

        /**
         * @brief Tenant State.
         * @param[in] base_cap Input parameter.
         * @return Return value.
         */
        explicit TenantState(size_t base_cap)
            : current_capacity(base_cap)
            , available_tokens(base_cap)
            , window_start(std::chrono::steady_clock::now())
        {}
    };


    /**
     * @brief Prune And Adapt.
     * @param[in,out] state Input/output parameter.
     */
    void pruneAndAdapt(TenantState& state);

    /**
     * @brief Compute P99.
     * @param[in] samples Input parameter.
     * @return Return value.
     */
    static std::chrono::milliseconds computeP99(
        const std::vector<TimedSample>& samples);

    /**
     * @brief Compute Error Rate.
     * @param[in] samples Input parameter.
     * @return Return value.
     */
    static double computeErrorRate(const std::vector<TimedSample>& samples);

    // ── State ─────────────────────────────────────────────────────────────────

    Config config_;

    mutable std::shared_mutex tenants_mutex_;
    std::unordered_map<std::string, TenantState> tenants_;

    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_rejections_{0};
};

} // namespace server
} // namespace themis
