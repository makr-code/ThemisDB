/**
 * @file dostoevsky.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Dostoevsky: Better Space-Time Trade-offs for LSM Trees
// Paper: "Dostoevsky: Better Space-Time Trade-Offs for LSM-Trees via Adaptive Removal of Superfluous Merging" (SIGMOD'18)
// Authors: Niv Dayan, Stratos Idreos (Harvard)
//
// Key idea: Adaptive lazy leveling for optimal write/read trade-offs based on workload
// Expected gain: +25-35% for mixed workloads
// Reference: https://dl.acm.org/doi/10.1145/3183713.3196927

#pragma once

#include <cstdint>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>

namespace themis {
namespace performance {

/// Merge policy for LSM tree levels
enum class MergePolicy {
    LEVELING,       // Best for read-heavy workloads (minimize # of runs)
    TIERING,        // Best for write-heavy workloads (minimize writes)
    LAZY_LEVELING   // Hybrid approach (Dostoevsky)
};

/// Workload statistics tracker
class WorkloadStats {
public:
    void record_read() { 
        reads_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void record_write() {
        writes_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Get read ratio (0.0 = all writes, 1.0 = all reads)
    double get_read_ratio() const {
        uint64_t r = reads_.load(std::memory_order_relaxed);
        uint64_t w = writes_.load(std::memory_order_relaxed);
        if (r + w == 0) return 0.5; // Default to balanced
        return static_cast<double>(r) / (r + w);
    }
    
    // Reset statistics (for sliding window)
    void reset() {
        reads_.store(0, std::memory_order_relaxed);
        writes_.store(0, std::memory_order_relaxed);
    }
    
    uint64_t get_reads() const { return reads_.load(std::memory_order_relaxed); }
    uint64_t get_writes() const { return writes_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> reads_{0};
    std::atomic<uint64_t> writes_{0};
};

/// Adaptive LSM tree configuration
class DostoevskeyLSM {
public:
    // Thresholds for workload classification (from paper)
    static constexpr double READ_HEAVY_THRESHOLD = 0.7;   // >70% reads
    static constexpr double WRITE_HEAVY_THRESHOLD = 0.3;  // <30% reads
    
    DostoevskeyLSM(int num_levels);
    
    // Compute optimal merge policy for a level based on workload
    MergePolicy compute_optimal_policy(int level, const WorkloadStats& stats) const;
    
    // Get current policy for level
    MergePolicy get_policy(int level) const;
    
    // Update policy for level (adaptive)
    void update_policy(int level, const WorkloadStats& stats);
    
    // Get merge cost estimation
    struct MergeCost {
        double read_amplification = 0;
        double write_amplification;
        double space_amplification;
    };
    MergeCost estimate_cost(int level, MergePolicy policy) const;

private:
    int num_levels_;
    std::vector<MergePolicy> level_policies_;
    mutable std::mutex mutex_;
};

/// Workload monitor with sliding window
class WorkloadMonitor {
public:
    explicit WorkloadMonitor(std::chrono::seconds window_duration = std::chrono::seconds(60));
    
    void record_read() { current_stats_.record_read(); }
    void record_write() { current_stats_.record_write(); }
    
    // Get current workload statistics
    const WorkloadStats& get_stats() const { return current_stats_; }
    
    // Check if it's time to update policies (sliding window expired)
    bool should_update_policies() const;
    
    // Reset window
    void reset_window();

private:
    WorkloadStats current_stats_;
    std::chrono::steady_clock::time_point window_start_;
    std::chrono::seconds window_duration_;
};

} // namespace performance
} // namespace themis
