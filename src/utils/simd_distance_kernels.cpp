/**
 * @file simd_distance_kernels.cpp
 * @brief Runtime kernel selection and CPUID detection for SIMD implementations
 */

#include "utils/simd_distance_kernels.h"
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
  #include <intrin.h>
#else
  #include <cpuid.h>
#endif

namespace themis {
namespace simd {

/**
 * @brief CPUID helper for x86/x64 detection
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
static void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx) {
#if defined(_WIN32) || defined(_WIN64)
    int regs[4];
    __cpuidex((int*)regs, (int)leaf, (int)subleaf);
    eax = regs[0]; ebx = regs[1]; ecx = regs[2]; edx = regs[3];
#else
    __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
#endif
}
#endif

/**
 * @brief Runtime detection of SIMD capability
 */
SIMDCapability detect_simd_capability() {
#if defined(__AVX512F__)
    // Compile-time AVX-512F available
    return SIMDCapability::AVX512F;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    // x86/x64: Check CPUID for AVX-512F, AVX2, AVX, SSE2
    uint32_t eax, ebx, ecx, edx;
    
    // Leaf 7, subleaf 0: Extended features
    cpuid(7, 0, eax, ebx, ecx, edx);
    
    // Check AVX-512F (bit 16 of EBX)
    if ((ebx & (1U << 16)) != 0) {
        return SIMDCapability::AVX512F;
    }
    
    // Check AVX2 (bit 5 of EBX)
    if ((ebx & (1U << 5)) != 0) {
        return SIMDCapability::AVX2;
    }
    
    // Leaf 1: Basic features
    cpuid(1, 0, eax, ebx, ecx, edx);
    
    // Check AVX (bit 28 of ECX)
    if ((ecx & (1U << 28)) != 0) {
        return SIMDCapability::AVX;
    }
    
    // Check SSE2 (bit 26 of EDX)
    if ((edx & (1U << 26)) != 0) {
        return SIMDCapability::SSE2;
    }
    
    return SIMDCapability::SCALAR;
    
#elif defined(__ARM_NEON) || defined(__aarch64__)
    // ARM: NEON support detection
#if defined(__aarch64__)
    return SIMDCapability::NEON64;
#else
    return SIMDCapability::NEON;
#endif
#elif defined(__EMSCRIPTEN__)
    // WebAssembly with SIMD support
    return SIMDCapability::WASM;
#else
    // Unknown architecture: use scalar
    return SIMDCapability::SCALAR;
#endif
}

/**
 * @brief Get kernel implementations for capability level
 */
SIMDKernels get_simd_kernels(SIMDCapability capability) {
    // For now, return scalar implementations as fallback
    // AVX2/AVX512 implementations will be added in Phase 2
    // NEON/WASM implementations will be added in Phase 3
    
    extern float scalar_l2_distance(const float* a, const float* b, std::size_t dim);
    extern float scalar_l2_distance_sq(const float* a, const float* b, std::size_t dim);
    extern void scalar_batch_l2_distance_sq(const float* query, const float* database,
                                             std::size_t n, std::size_t dim, float* distances);
    extern float scalar_inner_product(const float* a, const float* b, std::size_t dim);
    extern float scalar_cosine_distance(const float* a, const float* b, std::size_t dim);
    
    SIMDKernels scalar_kernels = {
        .l2_distance = scalar_l2_distance,
        .l2_distance_sq = scalar_l2_distance_sq,
        .batch_l2_distance_sq = scalar_batch_l2_distance_sq,
        .inner_product = scalar_inner_product,
        .cosine_distance = scalar_cosine_distance,
    };
    
    switch (capability) {
#if defined(__AVX512F__) || SIMD_HAS_AVX512F
        case SIMDCapability::AVX512F: {
            extern float avx512_l2_distance(const float* a, const float* b, std::size_t dim);
            extern float avx512_l2_distance_sq(const float* a, const float* b, std::size_t dim);
            extern void avx512_batch_l2_distance_sq(const float* query, const float* database,
                                                     std::size_t n, std::size_t dim, float* distances);
            extern float avx512_inner_product(const float* a, const float* b, std::size_t dim);
            extern float avx512_cosine_distance(const float* a, const float* b, std::size_t dim);
            return SIMDKernels{
                .l2_distance = avx512_l2_distance,
                .l2_distance_sq = avx512_l2_distance_sq,
                .batch_l2_distance_sq = avx512_batch_l2_distance_sq,
                .inner_product = avx512_inner_product,
                .cosine_distance = avx512_cosine_distance,
            };
        }
#endif
#if defined(__AVX2__) || SIMD_HAS_AVX2
        case SIMDCapability::AVX2: {
            extern float avx2_l2_distance(const float* a, const float* b, std::size_t dim);
            extern float avx2_l2_distance_sq(const float* a, const float* b, std::size_t dim);
            extern void avx2_batch_l2_distance_sq(const float* query, const float* database,
                                                   std::size_t n, std::size_t dim, float* distances);
            extern float avx2_inner_product(const float* a, const float* b, std::size_t dim);
            extern float avx2_cosine_distance(const float* a, const float* b, std::size_t dim);
            return SIMDKernels{
                .l2_distance = avx2_l2_distance,
                .l2_distance_sq = avx2_l2_distance_sq,
                .batch_l2_distance_sq = avx2_batch_l2_distance_sq,
                .inner_product = avx2_inner_product,
                .cosine_distance = avx2_cosine_distance,
            };
        }
#endif
#if defined(__ARM_NEON) || defined(__aarch64__)
        case SIMDCapability::NEON:
        case SIMDCapability::NEON64: {
            extern float neon_l2_distance(const float* a, const float* b, std::size_t dim);
            extern float neon_l2_distance_sq(const float* a, const float* b, std::size_t dim);
            extern void neon_batch_l2_distance_sq(const float* query, const float* database,
                                                   std::size_t n, std::size_t dim, float* distances);
            extern float neon_inner_product(const float* a, const float* b, std::size_t dim);
            extern float neon_cosine_distance(const float* a, const float* b, std::size_t dim);
            return SIMDKernels{
                .l2_distance = neon_l2_distance,
                .l2_distance_sq = neon_l2_distance_sq,
                .batch_l2_distance_sq = neon_batch_l2_distance_sq,
                .inner_product = neon_inner_product,
                .cosine_distance = neon_cosine_distance,
            };
        }
#endif
        case SIMDCapability::SCALAR:
        default:
            return scalar_kernels;
    }
}

/**
 * @brief Get human-readable SIMD capability name
 */
const char* simd_capability_name(SIMDCapability capability) {
    switch (capability) {
        case SIMDCapability::SCALAR:
            return "scalar";
        case SIMDCapability::SSE2:
            return "SSE2";
        case SIMDCapability::AVX:
            return "AVX";
        case SIMDCapability::AVX2:
            return "AVX2";
        case SIMDCapability::AVX512F:
            return "AVX-512F";
        case SIMDCapability::NEON:
            return "NEON";
        case SIMDCapability::NEON64:
            return "NEON64";
        case SIMDCapability::WASM:
            return "WASM SIMD";
        default:
            return "unknown";
    }
}

} // namespace simd
} // namespace themis
