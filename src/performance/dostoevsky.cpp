/*
 * ThemisDB | File: dostoevsky.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 85
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #469 Fix 11 compilation errors: ... (2026-03-11) | #160 Implement Phase 2 and Phase... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
