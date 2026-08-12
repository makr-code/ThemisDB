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

    // Per-query cost model integration with query optimizer (Phase 3, Issue #2419)
    // Calibrates OptimizerCostModel constants from actual hardware cycle measurements.
    // Expected gain: ~10-30% better plan selection accuracy on repeat queries.
    bool per_query_cost_model_enabled() const { return per_query_cost_model_enabled_.load(std::memory_order_relaxed); }
    void set_per_query_cost_model_enabled(bool enabled) { per_query_cost_model_enabled_.store(enabled, std::memory_order_relaxed); }
    // Memory Pressure Monitoring with Automatic Cache Eviction
    // Monitors system RAM usage and triggers registered eviction callbacks
    // when configurable thresholds are exceeded.
    bool memory_pressure_enabled() const { return memory_pressure_enabled_.load(std::memory_order_relaxed); }
    void set_memory_pressure_enabled(bool enabled) { memory_pressure_enabled_.store(enabled, std::memory_order_relaxed); }

    // AVX-512 SIMD path for vector distance computations (Phase 3, Issue #1964)
    // Enables 4-wide unrolled AVX-512 kernels for L2, inner-product, and cosine
    // distance calculations. Falls back to AVX2/NEON/scalar at compile time when
    // the target CPU does not support AVX-512.
    // Expected gain: +200-400% throughput for high-dimensional (>=256D) vector search.
    bool avx512_distance_enabled() const { return avx512_distance_enabled_.load(std::memory_order_relaxed); }
    void set_avx512_distance_enabled(bool enabled) { avx512_distance_enabled_.store(enabled, std::memory_order_relaxed); }

    // Adaptive batch size tuning for LLM inference (Phase 3, Issue #1996)
    // Dynamically adjusts the LLM inference batch size based on measured
    // throughput and latency using hardware cycle counters.
    // Expected gain: +15-40% throughput for variable-length LLM inference workloads.
    bool adaptive_batch_tuner_enabled() const { return adaptive_batch_tuner_enabled_.load(std::memory_order_relaxed); }
    void set_adaptive_batch_tuner_enabled(bool enabled) { adaptive_batch_tuner_enabled_.store(enabled, std::memory_order_relaxed); }

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
    std::atomic<bool> per_query_cost_model_enabled_{false};
    std::atomic<bool> memory_pressure_enabled_{false};
    std::atomic<bool> avx512_distance_enabled_{
#if defined(__AVX512F__)
        true
#else
        false
#endif
    };
    std::atomic<bool> adaptive_batch_tuner_enabled_{true};
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

#ifdef THEMIS_ENABLE_PER_QUERY_COST_MODEL
    #define THEMIS_PHASE3_PER_QUERY_COST_MODEL_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().per_query_cost_model_enabled())
#else
    #define THEMIS_PHASE3_PER_QUERY_COST_MODEL_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_MEMORY_PRESSURE
    #define THEMIS_PHASE3_MEMORY_PRESSURE_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().memory_pressure_enabled())
#else
    #define THEMIS_PHASE3_MEMORY_PRESSURE_ENABLED() (false)
#endif

// AVX-512 SIMD distance flag: enabled at compile time when __AVX512F__ is set,
// and can be toggled at runtime for testing/diagnostics.
#ifdef __AVX512F__
    #define THEMIS_PHASE3_AVX512_DISTANCE_ENABLED() (::themis::performance::phase3::Phase3FeatureFlags::instance().avx512_distance_enabled())
#else
    #define THEMIS_PHASE3_AVX512_DISTANCE_ENABLED() (false)
#endif

// Adaptive batch size tuning for LLM inference (Phase 3, Issue #1996).
// Enabled by default; disable at runtime for A/B testing or fixed-batch mode.
#define THEMIS_PHASE3_ADAPTIVE_BATCH_TUNER_ENABLED() \
    (::themis::performance::phase3::Phase3FeatureFlags::instance().adaptive_batch_tuner_enabled())

} // namespace phase3
} // namespace performance
} // namespace themis
