/**
 * @file dostoevsky.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/dostoevsky.h"
#include "performance/phase2_feature_flags.h"
#include <algorithm>

namespace themis {
namespace performance {

// Hardware validation
static bool is_dostoevsky_hardware_supported() {
    return Phase2FeatureFlags::instance().dostoevsky_hardware_supported();
}

DostoevskeyLSM::DostoevskeyLSM(int num_levels) 
    : num_levels_(num_levels), level_policies_(num_levels, MergePolicy::LAZY_LEVELING) {
    
    // Validate input: positive levels required
    if (num_levels <= 0) {
        throw std::runtime_error("Dostoevsky: num_levels must be positive");
    }
    
    // Fail-closed: hardware must support multi-core operations
    if (!is_dostoevsky_hardware_supported()) {
        throw std::runtime_error(
            "Dostoevsky: Hardware does not support multi-core operations required for adaptive merging. "
            "Use standard LSM tree configuration instead."
        );
    }
    
    // Clamp num_levels to reasonable bounds (1-32)
    if (num_levels > 32) {
        throw std::runtime_error("Dostoevsky: num_levels exceeds maximum (32)");
    }
}

MergePolicy DostoevskeyLSM::compute_optimal_policy(int level, const WorkloadStats& stats) const {
    // Validate level bounds
    if (level < 0 || level >= num_levels_) {
        throw std::runtime_error("Dostoevsky: level out of bounds");
    }
    
    double read_ratio = stats.get_read_ratio();
    
    // Validate read_ratio: must be in [0, 1]
    if (read_ratio < 0.0 || read_ratio > 1.0) {
        throw std::runtime_error("Dostoevsky: read_ratio out of bounds [0, 1]");
    }
    
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
    if (level < 0 || level >= num_levels_) {
        throw std::runtime_error("Dostoevsky: level out of bounds");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    return level_policies_[level];
}

void DostoevskeyLSM::update_policy(int level, const WorkloadStats& stats) {
    if (level < 0 || level >= num_levels_) {
        throw std::runtime_error("Dostoevsky: level out of bounds");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    level_policies_[level] = compute_optimal_policy(level, stats);
}

DostoevskeyLSM::MergeCost DostoevskeyLSM::estimate_cost(int level, MergePolicy policy) const {
    if (level < 0 || level >= num_levels_) {
        throw std::runtime_error("Dostoevsky: level out of bounds");
    }
    
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
    
    // Validate costs are non-negative
    if (cost.read_amplification < 0.0 || cost.write_amplification < 0.0 || 
        cost.space_amplification < 0.0) {
        throw std::runtime_error("Dostoevsky: computed cost is negative");
    }
    
    return cost;
}

WorkloadMonitor::WorkloadMonitor(std::chrono::seconds window_duration)
    : window_duration_(window_duration) {
    
    // Validate window duration
    if (window_duration_.count() <= 0) {
        throw std::runtime_error("WorkloadMonitor: window_duration must be positive");
    }
    
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
