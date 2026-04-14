/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            dostoevsky.cpp                                     ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:03:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/dostoevsky.h"
#include <algorithm>

namespace themis {
namespace performance {

DostoevskeyLSM::DostoevskeyLSM(int num_levels) 
    : num_levels_(num_levels), level_policies_(num_levels, MergePolicy::LAZY_LEVELING) {}

MergePolicy DostoevskeyLSM::compute_optimal_policy(int level, const WorkloadStats& stats) const {
    double read_ratio = stats.get_read_ratio();
    
    // Dostoevsky adaptive policy selection (from paper)
    if (read_ratio > READ_HEAVY_THRESHOLD) {
        return MergePolicy::LEVELING; // Minimize # of runs for reads
    } else if (read_ratio < WRITE_HEAVY_THRESHOLD) {
        return MergePolicy::TIERING;  // Minimize write amplification
    } else {
        return MergePolicy::LAZY_LEVELING; // Hybrid (best of both)
    }
}

MergePolicy DostoevskeyLSM::get_policy(int level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_policies_[level];
}

void DostoevskeyLSM::update_policy(int level, const WorkloadStats& stats) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_policies_[level] = compute_optimal_policy(level, stats);
}

DostoevskeyLSM::MergeCost DostoevskeyLSM::estimate_cost(int level, MergePolicy policy) const {
    MergeCost cost;
    
    // Simplified cost model (from Dostoevsky paper)
    switch (policy) {
        case MergePolicy::LEVELING:
            cost.read_amplification = 1.0;
            cost.write_amplification = static_cast<double>(num_levels_);
            cost.space_amplification = 1.1;
            break;
        case MergePolicy::TIERING:
            cost.read_amplification = static_cast<double>(num_levels_);
            cost.write_amplification = 1.0;
            cost.space_amplification = 1.5;
            break;
        case MergePolicy::LAZY_LEVELING:
            // Hybrid: better than both extremes
            cost.read_amplification = num_levels_ * 0.5;
            cost.write_amplification = num_levels_ * 0.5;
            cost.space_amplification = 1.2;
            break;
    }
    
    return cost;
}

WorkloadMonitor::WorkloadMonitor(std::chrono::seconds window_duration)
    : window_duration_(window_duration) {
    reset_window();
}

bool WorkloadMonitor::should_update_policies() const {
    auto now = std::chrono::steady_clock::now();
    return (now - window_start_) >= window_duration_;
}

void WorkloadMonitor::reset_window() {
    window_start_ = std::chrono::steady_clock::now();
    // Thread-safe reset of statistics by calling reset() method
    current_stats_.reset();
}

} // namespace performance
} // namespace themis
