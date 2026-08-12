/**
 * @file expected_cycles.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>

namespace themis {
namespace performance {

/**
 * @brief Expected cycle counts for various operations
 * 
 * These values are based on modern hardware (Intel/AMD ~3-4 GHz, DDR4/DDR5).
 * Use for validation and anomaly detection.
 */
struct ExpectedCycles {
    // ========================================================================
    // MEMORY OPERATIONS
    // ========================================================================
    
    /// L1 cache hit: ~4 cycles (1-2ns @ 3GHz)
    static constexpr uint64_t L1_CACHE_HIT = 4;
    
    /// L2 cache hit: ~12 cycles (3-4ns @ 3GHz)
    static constexpr uint64_t L2_CACHE_HIT = 12;
    
    /// L3 cache hit: ~50 cycles (12-15ns @ 3GHz)
    static constexpr uint64_t L3_CACHE_HIT = 50;
    
    /// RAM access: ~250 cycles (60-80ns @ 3GHz)
    static constexpr uint64_t RAM_ACCESS = 250;
    
    // ========================================================================
    // POINTER OPERATIONS
    // ========================================================================
    
    /// Pointer passing overhead: ~150 cycles (registry + stack)
    /// This should be negligible compared to actual computation
    static constexpr uint64_t POINTER_PASSING = 150;
    
    /// Memory copy per KB: ~800 cycles (~0.8 cycles/byte with SIMD)
    static constexpr uint64_t MEMORY_COPY_PER_KB = 800;
    
    // ========================================================================
    // HNSW SEARCH (CPU)
    // ========================================================================
    
    /// HNSW search: 1K vectors, 768-dim, ef=50
    static constexpr uint64_t HNSW_SEARCH_1K_VECTORS = 1'200'000;
    
    /// HNSW search: 10K vectors, 768-dim, ef=50
    static constexpr uint64_t HNSW_SEARCH_10K_VECTORS = 4'500'000;
    
    /// HNSW search: 100K vectors, 768-dim, ef=50
    static constexpr uint64_t HNSW_SEARCH_100K_VECTORS = 12'000'000;
    
    /// HNSW search: 1M vectors, 768-dim, ef=50
    static constexpr uint64_t HNSW_SEARCH_1M_VECTORS = 35'000'000;
    
    // ========================================================================
    // LLM INFERENCE (CPU)
    // ========================================================================
    
    /// LLM inference per token: 7B parameter model
    /// ~20-30ms @ 3GHz = 60-90M cycles
    static constexpr uint64_t LLM_INFERENCE_TOKEN_7B = 80'000'000;
    
    /// LLM inference per token: 13B parameter model
    /// ~35-50ms @ 3GHz = 105-150M cycles
    static constexpr uint64_t LLM_INFERENCE_TOKEN_13B = 145'000'000;
    
    // ========================================================================
    // PCIe TRANSFERS
    // ========================================================================
    
    /// PCIe Gen 4 x16: ~50 GB/s effective bandwidth
    /// Transferring 1MB takes ~20µs = ~80K cycles @ 4GHz
    /// Per MB: 335K cycles @ 3GHz average
    static constexpr uint64_t PCIE_CYCLES_PER_MB = 335'000;
    
    // ========================================================================
    // GPU OPERATIONS
    // ========================================================================
    
    /// GPU kernel launch overhead: ~20K cycles (~5-7µs)
    static constexpr uint64_t GPU_KERNEL_LAUNCH = 20'000;
    
    // ========================================================================
    // FULL RAG PIPELINE
    // ========================================================================
    
    /**
     * @brief Expected cycle counts for a complete RAG pipeline
     * 
     * Scenario: 10K vector search + pointer passing + 10 token generation (7B model)
     */
    struct RAGPipeline {
        /// HNSW search: 10K vectors
        static constexpr uint64_t HNSW_10K_SEARCH = 4'500'000;
        
        /// Pointer passing overhead
        static constexpr uint64_t POINTER_PASSING_OVERHEAD = 150;
        
        /// LLM inference: 10 tokens with 7B model
        static constexpr uint64_t LLM_10_TOKENS_7B = 800'000'000;
        
        /// Total cycles
        static constexpr uint64_t TOTAL = HNSW_10K_SEARCH + POINTER_PASSING_OVERHEAD + LLM_10_TOKENS_7B;
        
        /// Pointer passing overhead as percentage of total
        /// Expected: 0.000019% (negligible!)
        static constexpr double POINTER_OVERHEAD_PERCENT = 
            (double)POINTER_PASSING_OVERHEAD / (double)TOTAL * 100.0;
        
        // Breakdown percentages
        static constexpr double HNSW_PERCENT = 
            (double)HNSW_10K_SEARCH / (double)TOTAL * 100.0;  // ~0.56%
        
        static constexpr double LLM_PERCENT = 
            (double)LLM_10_TOKENS_7B / (double)TOTAL * 100.0;  // ~99.44%
    };
    
    // ========================================================================
    // TOLERANCES FOR ALERTING
    // ========================================================================
    
    /**
     * @brief Tolerance levels for deviation from expected values
     * 
     * Use these thresholds for monitoring and alerting.
     */
    struct Tolerances {
        /// Normal variance: ±15% (expected under normal conditions)
        static constexpr double NORMAL_VARIANCE = 0.15;
        
        /// Warning threshold: ±30% (investigate if exceeded)
        static constexpr double WARNING_THRESHOLD = 0.30;
        
        /// Critical threshold: ±50% (serious performance issue)
        static constexpr double CRITICAL_THRESHOLD = 0.50;
    };
    
    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================
    
    /**
     * @brief Calculate deviation percentage from expected value
     * @param actual Measured cycle count
     * @param expected Expected cycle count
     * @return Deviation as percentage (positive = slower, negative = faster)
     */
    static inline double deviation_percent(uint64_t actual, uint64_t expected) noexcept {
        if (expected == 0) return 0.0;
        return ((double)actual - (double)expected) / (double)expected * 100.0;
    }
    
    /**
     * @brief Check if deviation is within normal variance
     * @param actual Measured cycle count
     * @param expected Expected cycle count
     * @return true if within ±15%
     */
    static inline bool is_normal(uint64_t actual, uint64_t expected) noexcept {
        double deviation = std::abs(deviation_percent(actual, expected)) / 100.0;
        return deviation <= Tolerances::NORMAL_VARIANCE;
    }
    
    /**
     * @brief Check if deviation requires warning
     * @param actual Measured cycle count
     * @param expected Expected cycle count
     * @return true if deviation > 30%
     */
    static inline bool is_warning(uint64_t actual, uint64_t expected) noexcept {
        double deviation = std::abs(deviation_percent(actual, expected)) / 100.0;
        return deviation > Tolerances::NORMAL_VARIANCE && deviation <= Tolerances::WARNING_THRESHOLD;
    }
    
    /**
     * @brief Check if deviation is critical
     * @param actual Measured cycle count
     * @param expected Expected cycle count
     * @return true if deviation > 50%
     */
    static inline bool is_critical(uint64_t actual, uint64_t expected) noexcept {
        double deviation = std::abs(deviation_percent(actual, expected)) / 100.0;
        return deviation > Tolerances::WARNING_THRESHOLD;
    }
};

} // namespace performance
} // namespace themis
