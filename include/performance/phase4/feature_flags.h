/**
 * @file feature_flags.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Phase 4 Performance Optimizations Feature Flags
// Covers ML-based workload predictor, PMU integration, CI regression
// detection, DPDK/io_uring zero-copy I/O, and persistent memory storage layout.
//
// This header follows the same pattern as phase3/feature_flags.h.
// Expected gains (Phase 4): +50-200% for PMem-heavy workloads;
//                            10-50x latency improvement with io_uring.

#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace themis {
namespace performance {
namespace phase4 {

/// Thread-safe singleton for Phase 4 performance feature flags.
/// Allows runtime toggling of optimizations without recompilation.
class Phase4FeatureFlags {
public:
    static Phase4FeatureFlags& instance() {
        static Phase4FeatureFlags inst;
        return inst;
    }

    // Persistent Memory (Optane) aware storage layout
    // Expected gain: +50-200% write throughput for small random writes
    bool pmem_enabled() const { return pmem_enabled_.load(std::memory_order_relaxed); }
    void set_pmem_enabled(bool enabled) { pmem_enabled_.store(enabled, std::memory_order_relaxed); }

    // Hardware PMU counters for cache miss analysis
    // Expected overhead: <1 ns per measurement point when amortised
    bool pmu_enabled() const { return pmu_enabled_.load(std::memory_order_relaxed); }
    void set_pmu_enabled(bool enabled) { pmu_enabled_.store(enabled, std::memory_order_relaxed); }

    // io_uring zero-copy I/O path for network performance
    // Expected gain: 10-50x lower latency vs. epoll/read/write on Linux ≥ 5.1
    bool io_uring_enabled() const { return io_uring_enabled_.load(std::memory_order_relaxed); }
    void set_io_uring_enabled(bool enabled) { io_uring_enabled_.store(enabled, std::memory_order_relaxed); }

    // Load configuration from JSON file
    void load_from_config(const std::string& config_path);

private:
    Phase4FeatureFlags() = default;
    ~Phase4FeatureFlags() = default;
    Phase4FeatureFlags(const Phase4FeatureFlags&) = delete;
    Phase4FeatureFlags& operator=(const Phase4FeatureFlags&) = delete;

    std::atomic<bool> pmem_enabled_{false};
    std::atomic<bool> pmu_enabled_{false};
    std::atomic<bool> io_uring_enabled_{false};
};

// Macro helpers for compile-time + runtime checks
#ifdef THEMIS_ENABLE_PMEM
    #define THEMIS_PHASE4_PMEM_ENABLED() \
        (::themis::performance::phase4::Phase4FeatureFlags::instance().pmem_enabled())
#else
    #define THEMIS_PHASE4_PMEM_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_PMU_COUNTERS
    #define THEMIS_PHASE4_PMU_ENABLED() \
        (::themis::performance::phase4::Phase4FeatureFlags::instance().pmu_enabled())
#else
    #define THEMIS_PHASE4_PMU_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_IO_URING
    #define THEMIS_PHASE4_IO_URING_ENABLED() \
        (::themis::performance::phase4::Phase4FeatureFlags::instance().io_uring_enabled())
#else
    #define THEMIS_PHASE4_IO_URING_ENABLED() (false)
#endif

} // namespace phase4
} // namespace performance
} // namespace themis
