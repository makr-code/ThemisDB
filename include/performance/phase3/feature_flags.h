/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feature_flags.h                                    ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Phase 3 Performance Optimizations Feature Flags
// Based on scientific research from PR #156 (45+ peer-reviewed papers)
// Implementation framework from PR #157 (Phase 1) and current PR (Phase 2)
//
// Expected gains (Phase 3): +200-500% for specialized workloads
// Timeframe: 6-12 months implementation effort

#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace themis {
namespace performance {
namespace phase3 {

/// Thread-safe singleton for Phase 3 performance feature flags
/// Allows runtime toggling of optimizations without recompilation
class Phase3FeatureFlags {
public:
    static Phase3FeatureFlags& instance() {
        static Phase3FeatureFlags instance;
        return instance;
    }

    // DiskANN: Billion-scale Vector Search (NeurIPS'19)
    // Expected gain: +300-400% vector search throughput for >100M vectors
    bool diskann_enabled() const { return diskann_enabled_.load(std::memory_order_relaxed); }
    void set_diskann_enabled(bool enabled) { diskann_enabled_.store(enabled, std::memory_order_relaxed); }

    // Bw-Tree: Lock-Free Index (ICDE'13)
    // Expected gain: +100-200% index update throughput
    bool bwtree_enabled() const { return bwtree_enabled_.load(std::memory_order_relaxed); }
    void set_bwtree_enabled(bool enabled) { bwtree_enabled_.store(enabled, std::memory_order_relaxed); }

    // SplinterDB: Concurrent Compaction (OSDI'20)
    // Expected gain: -70% P99 latency
    bool splinterdb_enabled() const { return splinterdb_enabled_.load(std::memory_order_relaxed); }
    void set_splinterdb_enabled(bool enabled) { splinterdb_enabled_.store(enabled, std::memory_order_relaxed); }

    // Gunrock: GPU Graph Analytics (PPoPP'16)
    // Expected gain: +1000-3000% graph analytics on GPU
    bool gunrock_enabled() const { return gunrock_enabled_.load(std::memory_order_relaxed); }
    void set_gunrock_enabled(bool enabled) { gunrock_enabled_.store(enabled, std::memory_order_relaxed); }

    // Bao: ML-based Query Optimizer (SIGMOD'21)
    // Expected gain: +30-70% query performance
    bool bao_enabled() const { return bao_enabled_.load(std::memory_order_relaxed); }
    void set_bao_enabled(bool enabled) { bao_enabled_.store(enabled, std::memory_order_relaxed); }

    // Load configuration from JSON file
    void load_from_config(const std::string& config_path);

private:
    Phase3FeatureFlags() = default;
    ~Phase3FeatureFlags() = default;
    Phase3FeatureFlags(const Phase3FeatureFlags&) = delete;
    Phase3FeatureFlags& operator=(const Phase3FeatureFlags&) = delete;

    std::atomic<bool> diskann_enabled_{false};
    std::atomic<bool> bwtree_enabled_{false};
    std::atomic<bool> splinterdb_enabled_{false};
    std::atomic<bool> gunrock_enabled_{false};
    std::atomic<bool> bao_enabled_{false};
};

// Macro helpers for compile-time + runtime checks
#ifdef THEMIS_ENABLE_DISKANN
    #define THEMIS_PHASE3_DISKANN_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().diskann_enabled())
#else
    #define THEMIS_PHASE3_DISKANN_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_BWTREE
    #define THEMIS_PHASE3_BWTREE_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().bwtree_enabled())
#else
    #define THEMIS_PHASE3_BWTREE_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_SPLINTERDB
    #define THEMIS_PHASE3_SPLINTERDB_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().splinterdb_enabled())
#else
    #define THEMIS_PHASE3_SPLINTERDB_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_GUNROCK
    #define THEMIS_PHASE3_GUNROCK_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().gunrock_enabled())
#else
    #define THEMIS_PHASE3_GUNROCK_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_BAO
    #define THEMIS_PHASE3_BAO_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().bao_enabled())
#else
    #define THEMIS_PHASE3_BAO_ENABLED() (false)
#endif

} // namespace phase3
} // namespace performance
} // namespace themis
