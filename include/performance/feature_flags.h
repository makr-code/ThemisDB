/**
 * @file feature_flags.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Performance Optimization Feature Flags
// Based on research documentation in docs/de/research/
// 
// This header provides compile-time and runtime feature flags for
// performance optimizations identified in scientific research papers.
//
// See: docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md
// See: docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md

#pragma once

#include <atomic>
#include <string>
#include <unordered_map>

namespace themis {
namespace performance {

/**
 * @brief Feature flags for research-based performance optimizations
 * 
 * Each optimization can be enabled at compile-time (CMake) and/or runtime (config).
 * This allows for gradual rollout, A/B testing, and quick rollback if needed.
 */
class PerformanceFeatureFlags {
public:
    // Singleton instance
    static PerformanceFeatureFlags& instance() {
        static PerformanceFeatureFlags instance;
        return instance;
    }

    // Phase 1: Quick Wins (1-3 months, +50-100% read-heavy workloads)
    
    /**
     * @brief Use mimalloc allocator (+10-20% overall performance)
     * Paper: "Mimalloc: Free List Sharding in Action" (ISMM'19)
     * Compile: THEMIS_ENABLE_MIMALLOC
     * Runtime: performance.enable_mimalloc
     */
    bool mimalloc_enabled() const { return mimalloc_enabled_.load(); }
    void set_mimalloc_enabled(bool enabled) { mimalloc_enabled_.store(enabled); }

    /**
     * @brief Enable huge pages support (+15-30% memory-intensive workloads)
     * Paper: "Optimizing Database Performance using Huge Pages" (FAST'14)
     * Compile: THEMIS_ENABLE_HUGE_PAGES
     * Runtime: performance.enable_huge_pages
     */
    bool huge_pages_enabled() const { return huge_pages_enabled_.load(); }
    void set_huge_pages_enabled(bool enabled) { huge_pages_enabled_.store(enabled); }

    /**
     * @brief Use RCU for read-heavy index paths (+200-500% read performance)
     * Paper: "Scalable Read-Mostly Synchronization Using RCU" (ASPLOS'10)
     * Compile: THEMIS_ENABLE_RCU_INDEX
     * Runtime: performance.enable_rcu_index
     */
    bool rcu_index_enabled() const { return rcu_index_enabled_.load(); }
    void set_rcu_index_enabled(bool enabled) { rcu_index_enabled_.store(enabled); }

    /**
     * @brief Use LIRS cache replacement policy (+30-40% cache hit rate)
     * Paper: "LIRS: An Efficient Low Inter-reference Recency Set" (SIGMETRICS'02)
     * Compile: THEMIS_ENABLE_LIRS_CACHE
     * Runtime: performance.enable_lirs_cache
     */
    bool lirs_cache_enabled() const { return lirs_cache_enabled_.load(); }
    void set_lirs_cache_enabled(bool enabled) { lirs_cache_enabled_.store(enabled); }

    // Phase 2: Medium-Term (3-6 months, +100-200% overall)
    
    /**
     * @brief Enable WiscKey value separation (+40-60% write throughput for >1KB values)
     * Paper: "WiscKey: Separating Keys from Values in SSD-conscious Storage" (FAST'16)
     * Compile: THEMIS_ENABLE_WISCKEY
     * Runtime: performance.enable_wisckey
     */
    bool wisckey_enabled() const { return wisckey_enabled_.load(); }
    void set_wisckey_enabled(bool enabled) { wisckey_enabled_.store(enabled); }

    /**
     * @brief Use Cicada optimistic concurrency control (+100-150% TX throughput)
     * Paper: "Cicada: Dependably Fast Multi-Core In-Memory Transactions" (SIGMOD'17)
     * Compile: THEMIS_ENABLE_CICADA_CC
     * Runtime: performance.enable_cicada_cc
     */
    bool cicada_cc_enabled() const { return cicada_cc_enabled_.load(); }
    void set_cicada_cc_enabled(bool enabled) { cicada_cc_enabled_.store(enabled); }

    // Phase 3: Long-Term (6-12 months, +200-500% domain-specific)
    
    /**
     * @brief Use DiskANN for billion-scale vector search (+300-400% throughput)
     * Paper: "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search" (NeurIPS'19)
     * Compile: THEMIS_ENABLE_DISKANN
     * Runtime: performance.enable_diskann
     */
    bool diskann_enabled() const { return diskann_enabled_.load(); }
    void set_diskann_enabled(bool enabled) { diskann_enabled_.store(enabled); }

    /**
     * @brief Use Bw-Tree lock-free index (+100-200% index update throughput)
     * Paper: "The Bw-Tree: A Lock-Free B-Tree for New Hardware" (ICDE'18)
     * Compile: THEMIS_ENABLE_BW_TREE
     * Runtime: performance.enable_bw_tree
     */
    bool bw_tree_enabled() const { return bw_tree_enabled_.load(); }
    void set_bw_tree_enabled(bool enabled) { bw_tree_enabled_.store(enabled); }

    // Phase 4: ML-Based Optimization & CI Integration (6-12 months, Issue #2424)

    /**
     * @brief Persistent Memory (Optane) aware storage layout (+50-200% write throughput)
     * Paper: "NOVA: A Log-structured File System for Hybrid Volatile/Non-volatile Main
     *         Memories" (FAST'16) – Jian Xu & Steven Swanson, UCSD
     * Compile: THEMIS_ENABLE_PMEM
     * Runtime: performance.enable_pmem
     */
    bool pmem_enabled() const { return pmem_enabled_.load(); }
    void set_pmem_enabled(bool enabled) { pmem_enabled_.store(enabled); }

    /**
     * @brief ML-based workload predictor for proactive resource scaling
     * Technique: EMA smoothing + OLS linear regression over a sliding observation window.
     * Forecasts QPS, CPU/memory utilization, and query latency; recommends
     * thread-pool and cache-size adjustments before resource pressure occurs.
     * Compile: THEMIS_ENABLE_ML_WORKLOAD_PREDICTOR
     * Runtime: performance.enable_ml_workload_predictor
     */
    bool ml_workload_predictor_enabled() const { return ml_workload_predictor_enabled_.load(); }
    void set_ml_workload_predictor_enabled(bool enabled) { ml_workload_predictor_enabled_.store(enabled); }

    // Configuration loading
    void load_from_config(const std::unordered_map<std::string, bool>& config) {
        for (const auto& [key, value] : config) {
            if (key == "enable_mimalloc") {
              set_mimalloc_enabled(value);
            }
            else if (key == "enable_huge_pages") set_huge_pages_enabled(value);
            else if (key == "enable_rcu_index") set_rcu_index_enabled(value);
            else if (key == "enable_lirs_cache") set_lirs_cache_enabled(value);
            else if (key == "enable_wisckey") set_wisckey_enabled(value);
            else if (key == "enable_cicada_cc") set_cicada_cc_enabled(value);
            else if (key == "enable_diskann") set_diskann_enabled(value);
            else if (key == "enable_bw_tree") set_bw_tree_enabled(value);
            else if (key == "enable_pmem") set_pmem_enabled(value);
            else if (key == "enable_ml_workload_predictor") set_ml_workload_predictor_enabled(value);
        }
    }

    // Get all feature flags as a map (for debugging/monitoring)
    std::unordered_map<std::string, bool> get_all_flags() const {
        return {
            {"mimalloc", mimalloc_enabled()},
            {"huge_pages", huge_pages_enabled()},
            {"rcu_index", rcu_index_enabled()},
            {"lirs_cache", lirs_cache_enabled()},
            {"wisckey", wisckey_enabled()},
            {"cicada_cc", cicada_cc_enabled()},
            {"diskann", diskann_enabled()},
            {"bw_tree", bw_tree_enabled()},
            {"pmem", pmem_enabled()},
            {"ml_workload_predictor", ml_workload_predictor_enabled()}
        };
    }

private:
    PerformanceFeatureFlags() {
        // Initialize from compile-time flags
        #ifdef THEMIS_ENABLE_MIMALLOC
        mimalloc_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_HUGE_PAGES
        huge_pages_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_RCU_INDEX
        rcu_index_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_LIRS_CACHE
        lirs_cache_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_WISCKEY
        wisckey_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_CICADA_CC
        cicada_cc_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_DISKANN
        diskann_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_BW_TREE
        bw_tree_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_PMEM
        pmem_enabled_.store(true);
        #endif
        #ifdef THEMIS_ENABLE_ML_WORKLOAD_PREDICTOR
        ml_workload_predictor_enabled_.store(true);
        #endif
    }

    // Atomic flags for thread-safe runtime toggling
    std::atomic<bool> mimalloc_enabled_{false};
    std::atomic<bool> huge_pages_enabled_{false};
    std::atomic<bool> rcu_index_enabled_{false};
    std::atomic<bool> lirs_cache_enabled_{false};
    std::atomic<bool> wisckey_enabled_{false};
    std::atomic<bool> cicada_cc_enabled_{false};
    std::atomic<bool> diskann_enabled_{false};
    std::atomic<bool> bw_tree_enabled_{false};
    std::atomic<bool> pmem_enabled_{false};
    std::atomic<bool> ml_workload_predictor_enabled_{false};
};

// Convenience macros for checking feature flags
#define THEMIS_PERF_MIMALLOC_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().mimalloc_enabled())
#define THEMIS_PERF_HUGE_PAGES_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().huge_pages_enabled())
#define THEMIS_PERF_RCU_INDEX_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().rcu_index_enabled())
#define THEMIS_PERF_LIRS_CACHE_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().lirs_cache_enabled())
#define THEMIS_PERF_WISCKEY_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().wisckey_enabled())
#define THEMIS_PERF_CICADA_CC_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().cicada_cc_enabled())
#define THEMIS_PERF_DISKANN_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().diskann_enabled())
#define THEMIS_PERF_BW_TREE_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().bw_tree_enabled())
#define THEMIS_PERF_PMEM_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().pmem_enabled())
#define THEMIS_PERF_ML_WORKLOAD_PREDICTOR_ENABLED() \
    (::themis::performance::PerformanceFeatureFlags::instance().ml_workload_predictor_enabled())

} // namespace performance
} // namespace themis
