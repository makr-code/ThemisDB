/**
 * @file load_shedder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>

namespace themis {
namespace server {

/**
 * @brief Adaptive load shedding for server overload protection.
 * 
 * Monitors CPU, memory, and queue-depth pressure and rejects lower-priority
 * requests once configured overload thresholds are crossed.
 * 
 * ### Shedding Policy
 * - HIGH priority: always admitted while shedding is enabled
 * - LOW priority: rejected when computed load is > 0.80
 * - NORMAL priority: rejected when computed load is > 0.95
 * - Shedding can be disabled globally via Config::enable_shedding
 * 
 * ### Health Metrics
 * Current load is a weighted score:
 * - CPU utilization (50%)
 * - Memory utilization (30%)
 * - Queue depth factor normalized by Config::queue_depth_threshold (20%)
 *
 * updateLoad() clamps CPU/memory inputs to [0.0, 1.0] before evaluation.
 * queue_depth_threshold also controls how quickly queue pressure reaches full weight.
 * 
 * ### Operational Notes
 * - Queue depth contributes via Config::queue_depth_threshold normalization
 * - Policy decisions are deterministic for a given metric snapshot
 * - No randomized/LRU/FIFO eviction strategy is currently implemented
 * 
 * @note Decision-making is best-effort; no guarantee that all high-priority requests succeed
 * @note Thresholds are fixed in shouldReject(); metric normalization uses Config values
 * @note Enables graceful degradation under extreme load
 * 
 * @see RateLimitingMiddleware for request-level rate limiting
 */
class LoadShedder {
public:
    /**
     * @brief Runtime thresholds and feature toggle for load shedding.
     */
    struct Config {
        double cpu_threshold = 0.95;       ///< CPU usage threshold (0.0-1.0)
        double memory_threshold = 0.90;    ///< Memory usage threshold (0.0-1.0)
        size_t queue_depth_threshold = 1000; ///< Request queue depth limit
        bool enable_shedding = true;       ///< Enable/disable load shedding
    };
    
    /**
     * @brief Request priority classes used by shedding policy.
     */
    enum class Priority { HIGH, NORMAL, LOW };
    
    /**
     * @brief Construct shedder with static policy configuration.
     * @param config Threshold and toggle configuration.
     */
    explicit LoadShedder(const Config& config);
    
    /**
     * @brief Check if request should be rejected.
     * @param prio Request priority.
     * @return true if request should be rejected (shed), false otherwise.
     * @note HIGH priority is always admitted by policy.
     */
    bool shouldReject(Priority prio) const;
    
    /**
     * @brief Update system load metrics
     * @param cpu_usage CPU usage (0.0-1.0)
     * @param memory_usage Memory usage (0.0-1.0)
     * @param queue_depth Current request queue depth
     */
    void updateLoad(double cpu_usage, double memory_usage, size_t queue_depth);
    
    /**
     * @brief Get current normalized load factor.
     * @return Weighted load in [0.0, 1.0] based on CPU, memory, and queue depth.
     */
    double getCurrentLoad() const;
    
private:
    Config config_;
    std::atomic<double> cpu_usage_{0.0};
    std::atomic<double> memory_usage_{0.0};
    std::atomic<size_t> queue_depth_{0};
};

} // namespace server
} // namespace themis
