/**
 * @file simd_distance_kernels.h
 * @brief Multi-architecture SIMD distance kernel abstraction and dispatch
 * @version 0.1.0
 * @note This file provides architecture-agnostic SIMD kernel selection and fallback
 * @note Supported architectures: AVX-512, AVX2, NEON (ARM), WASM, scalar fallback
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace simd {

/**
 * @enum SIMDCapability
 * @brief Runtime-detected SIMD capability level
 */
enum class SIMDCapability : uint8_t {
    SCALAR = 0,      ///< Scalar fallback (no SIMD)
    SSE2 = 1,        ///< SSE2 (baseline x86_64)
    AVX = 2,         ///< AVX (Sandy Bridge+)
    AVX2 = 3,        ///< AVX2 (Haswell+)
    AVX512F = 4,     ///< AVX-512F (Skylake+)
    NEON = 5,        ///< ARM NEON (ARMv7+)
    NEON64 = 6,      ///< ARM NEON 64-bit (AArch64)
    WASM = 7,        ///< WebAssembly SIMD
};

/**
 * @struct SIMDKernels
 * @brief Function pointers for distance computation kernels
 * @details Contains function pointers for all distance metric implementations
 *          for a specific SIMD capability level
 */
struct SIMDKernels {
    /// L2 distance function pointer
    float (*l2_distance)(const float* a, const float* b, std::size_t dim);
    
    /// L2 squared distance function pointer
    float (*l2_distance_sq)(const float* a, const float* b, std::size_t dim);
    
    /// Batch L2 squared distance function pointer
    void (*batch_l2_distance_sq)(const float* query, const float* database,
                                  std::size_t n, std::size_t dim, float* distances);
    
    /// Inner product function pointer
    float (*inner_product)(const float* a, const float* b, std::size_t dim);
    
    /// Cosine distance function pointer
    float (*cosine_distance)(const float* a, const float* b, std::size_t dim);
};

/**
 * @brief Get runtime SIMD capability
 * @return Detected SIMD capability level
 * @details Uses CPUID detection on x86/x64, feature flags on ARM,
 *          and runtime checks on other platforms
 */
SIMDCapability detect_simd_capability();

/**
 * @brief Get kernel implementation for capability level
 * @param[in] capability Requested SIMD capability level
 * @return Kernel function pointers for the given capability
 * @details Automatically falls back to lower capability levels if requested
 *          level is not available
 */
SIMDKernels get_simd_kernels(SIMDCapability capability);

/**
 * @brief Get optimal kernels for this CPU
 * @return Kernel function pointers for best available SIMD implementation
 * @details Automatically detects CPU capability and selects optimal kernels
 */
inline SIMDKernels get_optimal_kernels() {
    return get_simd_kernels(detect_simd_capability());
}

/**
 * @brief Get human-readable name of SIMD capability
 * @param[in] capability SIMD capability level
 * @return String name (e.g., "AVX-512F", "NEON", "scalar")
 */
const char* simd_capability_name(SIMDCapability capability);

} // namespace simd
} // namespace themis
