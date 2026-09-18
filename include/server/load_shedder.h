/**
 * @file load_shedder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>

namespace themis {
namespace server {

class LoadShedder {
public:
    struct Config {
        double cpu_threshold = 0.95;       ///< CPU usage threshold (0.0-1.0)
        double memory_threshold = 0.90;    ///< Memory usage threshold (0.0-1.0)
        size_t queue_depth_threshold = 1000; ///< Request queue depth limit
        bool enable_shedding = true;       ///< Enable/disable load shedding
    };
    
    enum class Priority { HIGH, NORMAL, LOW };
    
    /**
     * @brief Load Shedder.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoadShedder(const Config& config);
    
    /**
     * @brief Should Reject.
     * @param[in] prio Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldReject(Priority prio) const;
    
    /**
     * @brief Update Load.
     * @param[in] cpu_usage Input parameter.
     * @param[in] memory_usage Input parameter.
     * @param[in] queue_depth Input parameter.
     */
    void updateLoad(double cpu_usage, double memory_usage, size_t queue_depth);
    
    /**
     * @brief Get Current Load.
     * @return Return value.
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
