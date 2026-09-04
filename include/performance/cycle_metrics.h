/**
 * @file cycle_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

#ifdef _MSC_VER
    #include <intrin.h>
    #ifdef _M_ARM64
        #include <arm64_neon.h>
    #endif
#endif

namespace themis {
namespace performance {

/**
 * @brief Hardware cycle counter for precise performance measurement
 * 
 * Provides platform-specific CPU cycle counting with sub-nanosecond precision.
 * Uses RDTSC on x86_64 and CNTVCT_EL0 on ARM64.
 */
class HardwareCycleCounter {
public:
    /**
     * @brief Read CPU cycle counter (non-serializing)
     * @return Current CPU cycle count
     */
    static inline uint64_t cpu_cycles() noexcept {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
        return __rdtsc();
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
        uint32_t lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__) || defined(_M_ARM64)
        uint64_t val = {};
#if defined(_MSC_VER)
        val = _ReadStatusReg(ARM64_CNTVCT);
#else
        __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
#endif
        return val;
#else
        // Fallback for unsupported architectures: steady_clock nanoseconds
        // (not CPU cycles, but provides monotonic timing for relative measurements)
        return static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
#endif
    }

    /**
     * @brief Read CPU cycle counter with serialization (precise measurement)
     * @return Current CPU cycle count
     * 
     * RDTSCP ensures all previous instructions complete before reading.
     * Use for precise measurement endpoints.
     */
    static inline uint64_t rdtscp() noexcept {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
        unsigned int aux;
        return __rdtscp(&aux);
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
        uint32_t lo, hi;
        __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
        return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__) || defined(_M_ARM64)
#if defined(_MSC_VER)
        __isb(_ARM64_BARRIER_SY);
        return _ReadStatusReg(ARM64_CNTVCT);
#else
        // ARM64: use ISB to serialize
        __asm__ __volatile__("isb" ::: "memory");
        uint64_t val = {};
        __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
        return val;
#endif
#else
        return static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
#endif
    }

    /**
     * @brief Get CPU frequency in Hz
     * @return CPU frequency or 0 if unavailable
     */
    static uint64_t cpu_frequency_hz() noexcept;

    /**
     * @brief Get CPU model string
     * @return CPU model name
     */
    static std::string cpu_model() noexcept;

#ifdef THEMIS_ENABLE_GPU_CYCLE_METRICS
    using GpuCyclesStartFn = std::function<void*()>;
    using GpuCyclesEndFn = std::function<uint64_t(void* event)>;

    static void setGpuCyclesStartFn(GpuCyclesStartFn fn);
    static void setGpuCyclesEndFn(GpuCyclesEndFn fn);

    /**
     * @brief Start GPU cycle measurement
     * @return GPU event handle
     */
    static void* gpu_cycles_start() noexcept;

    /**
     * @brief End GPU cycle measurement
     * @param event GPU event handle from gpu_cycles_start()
     * @return Elapsed GPU cycles
     */
    static uint64_t gpu_cycles_end(void* event) noexcept;
#endif
};

/**
 * @brief Cycle metrics for a single operation
 * 
 * Captures cycle counts for all major components of the ThemisDB pipeline.
 */
struct OperationCycleMetrics {
    // Core operations
    uint64_t hnsw_search_cycles = 0;
    uint64_t hnsw_index_traversal_cycles = 0;
    uint64_t hnsw_distance_computation_cycles = 0;
    
    uint64_t pointer_passing_cycles = 0;      // Critical: Should be ~100-500 cycles
    uint64_t memory_copy_cycles = 0;          // For comparison
    
    uint64_t llm_inference_cycles = 0;
    uint64_t llm_tokenization_cycles = 0;
    uint64_t llm_forward_pass_cycles = 0;
    uint64_t llm_sampling_cycles = 0;
    
    // Memory subsystem
    uint64_t l1_cache_cycles = 0;
    uint64_t l2_cache_cycles = 0;
    uint64_t l3_cache_cycles = 0;
    uint64_t ram_access_cycles = 0;
    uint64_t cache_miss_cycles = 0;
    
    // GPU operations
    uint64_t gpu_kernel_launch_cycles = 0;
    uint64_t gpu_compute_cycles = 0;
    uint64_t gpu_memory_cycles = 0;
    
    // Bus operations
    uint64_t pcie_host_to_device_cycles = 0;
    uint64_t pcie_device_to_host_cycles = 0;
    
    // Total
    uint64_t total_cycles = 0;
    uint64_t idle_cycles = 0;
    
    // Efficiency metrics
    double cpu_efficiency_ratio = 0.0;
    double memory_efficiency_ratio = 0.0;
    double pcie_efficiency_ratio = 0.0;
    
    // System info
    uint64_t cpu_frequency_hz = 0;
    std::string cpu_model;

    /**
     * @brief Calculate total cycles from components
     */
    void calculate_totals() noexcept {
        total_cycles = hnsw_search_cycles + pointer_passing_cycles + 
                      llm_inference_cycles + cache_miss_cycles +
                      pcie_host_to_device_cycles + pcie_device_to_host_cycles +
                      gpu_compute_cycles;
    }
};

/**
 * @brief RAII timer for automatic cycle measurement
 * 
 * Usage:
 *   uint64_t cycles;
 *   {
 *       ScopedCycleTimer timer(&cycles);
 *       // ... code to measure ...
 *   }
 *   // cycles now contains elapsed cycles
 */
class ScopedCycleTimer {
public:
    explicit ScopedCycleTimer(uint64_t* output) noexcept
        : output_(output), start_(HardwareCycleCounter::cpu_cycles()) {}

    ~ScopedCycleTimer() noexcept {
        if (output_) {
            *output_ = HardwareCycleCounter::rdtscp() - start_;
        }
    }

    // Non-copyable, non-movable
    ScopedCycleTimer(const ScopedCycleTimer&) = delete;
    ScopedCycleTimer& operator=(const ScopedCycleTimer&) = delete;

private:
    uint64_t* output_;
    uint64_t start_;
};

} // namespace performance
} // namespace themis
