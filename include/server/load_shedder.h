/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            load_shedder.h                                     ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     83                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>

namespace themis {
namespace server {

/**
 * @brief Load Shedder for adaptive request rejection under overload
 * 
 * Monitors system load and rejects low-priority requests when
 * resources are constrained.
 */
class LoadShedder {
public:
    struct Config {
        double cpu_threshold = 0.95;       ///< CPU usage threshold (0.0-1.0)
        double memory_threshold = 0.90;    ///< Memory usage threshold (0.0-1.0)
        size_t queue_depth_threshold = 1000; ///< Request queue depth limit
        bool enable_shedding = true;       ///< Enable/disable load shedding
    };
    
    enum class Priority { HIGH, NORMAL, LOW };
    
    explicit LoadShedder(const Config& config);
    
    /**
     * @brief Check if request should be rejected
     * @param prio Request priority
     * @return true if request should be rejected (rate limited)
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
     * @brief Get current load factor (0.0-1.0)
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
