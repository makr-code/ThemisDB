/**
 * @file simd_distance.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SIMD distance calculations for vector embeddings
// 
// CACHE OPTIMIZATION:
// - Prefetch hints for 1536D vectors (OpenAI ada-002 embeddings)
// - Prefetch distance: 64 floats (256 bytes) ahead into L2 cache
// - Reduces L2/L3 cache misses by ~10-20% for large batch operations
// - Optimized for typical cache line size (64 bytes) and L1/L2 cache hierarchy
//
// Performance Impact:
// - 1536D vector distance: ~96 iterations (AVX-512), ~192 iterations (AVX2)
// - Prefetching keeps pipeline full and reduces memory stalls
// - Estimated 10-20% speedup on embedding similarity searches

#include "utils/simd_distance.h"
#include <algorithm>
#include <cstddef>
#include <cmath>

#if defined(__AVX512F__)
  #include <immintrin.h>
#elif defined(__AVX2__)
  #include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
  #include <arm_neon.h>
#endif

namespace themis {
namespace simd {

// Cache line size constant for prefetching optimization
constexpr std::size_t CACHE_LINE_SIZE = 64;

// Prefetch distance in number of floats to prefetch ahead
// 64 floats = 256 bytes = 4 cache lines ahead
// This distance is optimal for hiding memory latency in streaming operations
constexpr std::size_t PREFETCH_DISTANCE = 64;

static inline float scalar_l2_sq(const float* a, const float* b, std::size_t dim) {
    float acc = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        acc += d * d;
    }
    return acc;
}

#if defined(__AVX512F__)
static inline float avx512_l2_sq(const float* a, const float* b, std::size_t dim) {
    std::size_t i = 0;
    // Use 4 accumulators to maximize pipeline utilization across 4 fused multiply-add units
    __m512 acc0 = _mm512_setzero_ps();
    __m512 acc1 = _mm512_setzero_ps();
    __m512 acc2 = _mm512_setzero_ps();
    __m512 acc3 = _mm512_setzero_ps();
    const std::size_t step = 64; // 4 × 16 floats per iteration

    for (; i + step <= dim; i += step) {
        // Prefetch next cache lines (256 bytes = 64 floats ahead)
        if (i + PREFETCH_DISTANCE < dim) {
            _mm_prefetch(reinterpret_cast<const char*>(a + i + PREFETCH_DISTANCE), _MM_HINT_T1);
            _mm_prefetch(reinterpret_cast<const char*>(b + i + PREFETCH_DISTANCE), _MM_HINT_T1);
        }
        __m512 d0 = _mm512_sub_ps(_mm512_loadu_ps(a + i),      _mm512_loadu_ps(b + i));
        __m512 d1 = _mm512_sub_ps(_mm512_loadu_ps(a + i + 16), _mm512_loadu_ps(b + i + 16));
        __m512 d2 = _mm512_sub_ps(_mm512_loadu_ps(a + i + 32), _mm512_loadu_ps(b + i + 32));
        __m512 d3 = _mm512_sub_ps(_mm512_loadu_ps(a + i + 48), _mm512_loadu_ps(b + i + 48));
        acc0 = _mm512_fmadd_ps(d0, d0, acc0);
        acc1 = _mm512_fmadd_ps(d1, d1, acc1);
        acc2 = _mm512_fmadd_ps(d2, d2, acc2);
        acc3 = _mm512_fmadd_ps(d3, d3, acc3);
    }
    // Handle remaining full 16-float chunks
    for (; i + 16 <= dim; i += 16) {
        __m512 d = _mm512_sub_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i));
        acc0 = _mm512_fmadd_ps(d, d, acc0);
    }
    // Combine accumulators and reduce to scalar
    __m512 acc = _mm512_add_ps(_mm512_add_ps(acc0, acc1), _mm512_add_ps(acc2, acc3));
    float res = _mm512_reduce_add_ps(acc);
    // Scalar tail for elements not covered by 16-wide SIMD
    if (i < dim) res += scalar_l2_sq(a + i, b + i, dim - i);
    return res;
}

static inline float avx512_inner_product(const float* a, const float* b, std::size_t dim) {
    std::size_t i = 0;
    __m512 acc0 = _mm512_setzero_ps();
    __m512 acc1 = _mm512_setzero_ps();
    __m512 acc2 = _mm512_setzero_ps();
    __m512 acc3 = _mm512_setzero_ps();
    const std::size_t step = 64;

    for (; i + step <= dim; i += step) {
        if (i + PREFETCH_DISTANCE < dim) {
            _mm_prefetch(reinterpret_cast<const char*>(a + i + PREFETCH_DISTANCE), _MM_HINT_T1);
            _mm_prefetch(reinterpret_cast<const char*>(b + i + PREFETCH_DISTANCE), _MM_HINT_T1);
        }
        acc0 = _mm512_fmadd_ps(_mm512_loadu_ps(a + i),      _mm512_loadu_ps(b + i),      acc0);
        acc1 = _mm512_fmadd_ps(_mm512_loadu_ps(a + i + 16), _mm512_loadu_ps(b + i + 16), acc1);
        acc2 = _mm512_fmadd_ps(_mm512_loadu_ps(a + i + 32), _mm512_loadu_ps(b + i + 32), acc2);
        acc3 = _mm512_fmadd_ps(_mm512_loadu_ps(a + i + 48), _mm512_loadu_ps(b + i + 48), acc3);
    }
    for (; i + 16 <= dim; i += 16) {
        acc0 = _mm512_fmadd_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i), acc0);
    }
    __m512 acc = _mm512_add_ps(_mm512_add_ps(acc0, acc1), _mm512_add_ps(acc2, acc3));
    float res = _mm512_reduce_add_ps(acc);
    for (; i < dim; ++i) res += a[i] * b[i];
    return res;
}

static inline float avx512_norm_sq(const float* a, std::size_t dim) {
    std::size_t i = 0;
    __m512 acc0 = _mm512_setzero_ps();
    __m512 acc1 = _mm512_setzero_ps();
    const std::size_t step = 32;

    for (; i + step <= dim; i += step) {
        __m512 v0 = _mm512_loadu_ps(a + i);
        __m512 v1 = _mm512_loadu_ps(a + i + 16);
        acc0 = _mm512_fmadd_ps(v0, v0, acc0);
        acc1 = _mm512_fmadd_ps(v1, v1, acc1);
    }
    for (; i + 16 <= dim; i += 16) {
        __m512 v = _mm512_loadu_ps(a + i);
        acc0 = _mm512_fmadd_ps(v, v, acc0);
    }
    __m512 acc = _mm512_add_ps(acc0, acc1);
    float res = _mm512_reduce_add_ps(acc);
    for (; i < dim; ++i) res += a[i] * a[i];
    return res;
}
#elif defined(__AVX2__)
static inline float avx2_inner_product(const float* a, const float* b, std::size_t dim) {
    std::size_t i = 0;
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    const std::size_t step = 16;

    for (; i + step <= dim; i += step) {
        acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(a + i),     _mm256_loadu_ps(b + i),     acc0);
        acc1 = _mm256_fmadd_ps(_mm256_loadu_ps(a + i + 8), _mm256_loadu_ps(b + i + 8), acc1);
    }
    __m256 acc = _mm256_add_ps(acc0, acc1);
    alignas(32) float tmp[8];
    _mm256_store_ps(tmp, acc);
    float res = 0.0f;
    for (int k = 0; k < 8; ++k) res += tmp[k];
    for (; i < dim; ++i) res += a[i] * b[i];
    return res;
}

static inline float avx2_norm_sq(const float* a, std::size_t dim) {
    std::size_t i = 0;
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    const std::size_t step = 16;

    for (; i + step <= dim; i += step) {
        __m256 v0 = _mm256_loadu_ps(a + i);
        __m256 v1 = _mm256_loadu_ps(a + i + 8);
        acc0 = _mm256_fmadd_ps(v0, v0, acc0);
        acc1 = _mm256_fmadd_ps(v1, v1, acc1);
    }
    __m256 acc = _mm256_add_ps(acc0, acc1);
    alignas(32) float tmp[8];
    _mm256_store_ps(tmp, acc);
    float res = 0.0f;
    for (int k = 0; k < 8; ++k) res += tmp[k];
    for (; i < dim; ++i) res += a[i] * a[i];
    return res;
}

static inline float avx2_l2_sq(const float* a, const float* b, std::size_t dim) {
    std::size_t i = 0;
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    const std::size_t step = 16;
    // Prefetch distance for 1536D vectors: prefetch 64 floats (256 bytes) ahead
    constexpr std::size_t prefetch_distance = 64;
    
    for (; i + step <= dim; i += step) {
        // Prefetch next cache line into L2 cache (optimized for 1536D embeddings)
        if (i + prefetch_distance < dim) {
            _mm_prefetch(reinterpret_cast<const char*>(a + i + prefetch_distance), _MM_HINT_T1);
            _mm_prefetch(reinterpret_cast<const char*>(b + i + prefetch_distance), _MM_HINT_T1);
        }
        
        __m256 va0 = _mm256_loadu_ps(a + i);
        __m256 vb0 = _mm256_loadu_ps(b + i);
        __m256 d0 = _mm256_sub_ps(va0, vb0);
        acc0 = _mm256_fmadd_ps(d0, d0, acc0);

        __m256 va1 = _mm256_loadu_ps(a + i + 8);
        __m256 vb1 = _mm256_loadu_ps(b + i + 8);
        __m256 d1 = _mm256_sub_ps(va1, vb1);
        acc1 = _mm256_fmadd_ps(d1, d1, acc1);
    }
    __m256 acc = _mm256_add_ps(acc0, acc1);
    alignas(32) float tmp[8];
    _mm256_store_ps(tmp, acc);
    float res = 0.0f;
    for (int k = 0; k < 8; ++k) res += tmp[k];
    if (i < dim) res += scalar_l2_sq(a + i, b + i, dim - i);
    return res;
}
#elif defined(__ARM_NEON) || defined(__aarch64__)
static inline float neon_l2_sq(const float* a, const float* b, std::size_t dim) {
    std::size_t i = 0;
    float32x4_t acc0 = vdupq_n_f32(0.0f);
    float32x4_t acc1 = vdupq_n_f32(0.0f);
    const std::size_t step = 8;
    // Prefetch distance for 1536D vectors: prefetch 64 floats (256 bytes) ahead
    // This keeps L2 cache warm for upcoming iterations
    constexpr std::size_t prefetch_distance = 64;
    
    // Process 8 floats at a time (2x NEON registers)
    for (; i + step <= dim; i += step) {
        // ARM prefetch intrinsic for cache optimization
        #if defined(__aarch64__)
        if (i + PREFETCH_DISTANCE < dim) {
            __builtin_prefetch(a + i + PREFETCH_DISTANCE, 0, 3);
            __builtin_prefetch(b + i + PREFETCH_DISTANCE, 0, 3);
        }
        #endif
        // Prefetch next cache line into L2 cache (optimized for 1536D embeddings)
        // ARM prefetch intrinsic: __builtin_prefetch
        if (i + prefetch_distance < dim) {
            __builtin_prefetch(a + i + prefetch_distance, 0, 1);  // read, moderate locality
            __builtin_prefetch(b + i + prefetch_distance, 0, 1);
        }
        
        float32x4_t va0 = vld1q_f32(a + i);
        float32x4_t vb0 = vld1q_f32(b + i);
        float32x4_t diff0 = vsubq_f32(va0, vb0);
        // Use FMA for better performance and accuracy (available on ARMv8+)
        // Falls back to vmlaq_f32 on older architectures
        #if defined(__ARM_FEATURE_FMA)
        acc0 = vfmaq_f32(acc0, diff0, diff0);  // Fused multiply-add
        #else
        acc0 = vmlaq_f32(acc0, diff0, diff0);  // Regular multiply-add
        #endif
        
        float32x4_t va1 = vld1q_f32(a + i + 4);
        float32x4_t vb1 = vld1q_f32(b + i + 4);
        float32x4_t diff1 = vsubq_f32(va1, vb1);
        #if defined(__ARM_FEATURE_FMA)
        acc1 = vfmaq_f32(acc1, diff1, diff1);
        #else
        acc1 = vmlaq_f32(acc1, diff1, diff1);
        #endif
    }
    
    // Combine accumulators
    float32x4_t acc = vaddq_f32(acc0, acc1);
    
    // Horizontal sum: reduce 4 lanes to scalar
    float32x2_t sum2 = vadd_f32(vget_low_f32(acc), vget_high_f32(acc));
    float32x2_t sum1 = vpadd_f32(sum2, sum2);
    float res = vget_lane_f32(sum1, 0);
    
    // Handle remaining elements (not enough for a full SIMD vector)
    if (i < dim) {
        res += scalar_l2_sq(a + i, b + i, dim - i);
    }
    return res;
}

static inline float neon_inner_product(const float* a, const float* b, std::size_t dim) {
    std::size_t i = 0;
    float32x4_t acc0 = vdupq_n_f32(0.0f);
    float32x4_t acc1 = vdupq_n_f32(0.0f);
    const std::size_t step = 8;

    for (; i + step <= dim; i += step) {
        float32x4_t va0 = vld1q_f32(a + i);
        float32x4_t vb0 = vld1q_f32(b + i);
        float32x4_t va1 = vld1q_f32(a + i + 4);
        float32x4_t vb1 = vld1q_f32(b + i + 4);
        #if defined(__ARM_FEATURE_FMA)
        acc0 = vfmaq_f32(acc0, va0, vb0);
        acc1 = vfmaq_f32(acc1, va1, vb1);
        #else
        acc0 = vmlaq_f32(acc0, va0, vb0);
        acc1 = vmlaq_f32(acc1, va1, vb1);
        #endif
    }
    float32x4_t acc = vaddq_f32(acc0, acc1);
    float32x2_t sum2 = vadd_f32(vget_low_f32(acc), vget_high_f32(acc));
    float32x2_t sum1 = vpadd_f32(sum2, sum2);
    float res = vget_lane_f32(sum1, 0);
    for (; i < dim; ++i) res += a[i] * b[i];
    return res;
}

static inline float neon_norm_sq(const float* a, std::size_t dim) {
    std::size_t i = 0;
    float32x4_t acc0 = vdupq_n_f32(0.0f);
    float32x4_t acc1 = vdupq_n_f32(0.0f);
    const std::size_t step = 8;

    for (; i + step <= dim; i += step) {
        float32x4_t v0 = vld1q_f32(a + i);
        float32x4_t v1 = vld1q_f32(a + i + 4);
        #if defined(__ARM_FEATURE_FMA)
        acc0 = vfmaq_f32(acc0, v0, v0);
        acc1 = vfmaq_f32(acc1, v1, v1);
        #else
        acc0 = vmlaq_f32(acc0, v0, v0);
        acc1 = vmlaq_f32(acc1, v1, v1);
        #endif
    }
    float32x4_t acc = vaddq_f32(acc0, acc1);
    float32x2_t sum2 = vadd_f32(vget_low_f32(acc), vget_high_f32(acc));
    float32x2_t sum1 = vpadd_f32(sum2, sum2);
    float res = vget_lane_f32(sum1, 0);
    for (; i < dim; ++i) res += a[i] * a[i];
    return res;
}
#endif // __ARM_NEON || __aarch64__

static inline float scalar_inner_product(const float* a, const float* b, std::size_t dim) {
    float acc = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) acc += a[i] * b[i];
    return acc;
}

static inline float scalar_norm_sq(const float* a, std::size_t dim) {
    float acc = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) acc += a[i] * a[i];
    return acc;
}

float l2_distance_sq(const float* a, const float* b, std::size_t dim) {
#if defined(__AVX512F__)
    return avx512_l2_sq(a, b, dim);
#elif defined(__AVX2__)
    return avx2_l2_sq(a, b, dim);
#elif defined(__ARM_NEON) || defined(__aarch64__)
    return neon_l2_sq(a, b, dim);
#else
    return scalar_l2_sq(a, b, dim);
#endif
}

float l2_distance(const float* a, const float* b, std::size_t dim) {
    float d2 = l2_distance_sq(a, b, dim);
    return std::sqrt(d2);
}

// Batch compute L2 squared distances - optimized with explicit prefetching
void batch_l2_distance_sq(const float* query, const float* database, 
                          std::size_t n, std::size_t dim, float* distances) {
    // Process multiple database vectors with explicit prefetching
    // Query vector will be hot in L1 cache after first iteration
    for (std::size_t i = 0; i < n; ++i) {
        // Prefetch next database vector while computing current one.
        // Keep the prefetch variables inside the feature-specific branches so the
        // scalar path does not carry unused locals that MSVC warns about.
        #if defined(__AVX2__) || defined(__AVX512F__) || defined(__aarch64__)
        if (i + 1 < n) {
            const float* next_vec = database + (i + 1) * dim;
            const std::size_t cache_lines_to_prefetch = (dim * sizeof(float) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE;
            #if defined(__AVX2__) || defined(__AVX512F__)
            for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {
                _mm_prefetch(reinterpret_cast<const char*>(next_vec + cl * (CACHE_LINE_SIZE / sizeof(float))), _MM_HINT_T0);
            }
            #elif defined(__aarch64__)
            for (std::size_t cl = 0; cl < std::min(cache_lines_to_prefetch, std::size_t(4)); ++cl) {
                __builtin_prefetch(next_vec + cl * (CACHE_LINE_SIZE / sizeof(float)), 0, 3);
            }
            #endif
        }
        #endif
        distances[i] = l2_distance_sq(query, database + i * dim, dim);
    }
}

float inner_product(const float* a, const float* b, std::size_t dim) {
#if defined(__AVX512F__)
    return avx512_inner_product(a, b, dim);
#elif defined(__AVX2__)
    return avx2_inner_product(a, b, dim);
#elif defined(__ARM_NEON) || defined(__aarch64__)
    return neon_inner_product(a, b, dim);
#else
    return scalar_inner_product(a, b, dim);
#endif
}

float cosine_distance(const float* a, const float* b, std::size_t dim) {
#if defined(__AVX512F__)
    float dot   = avx512_inner_product(a, b, dim);
    float norm_a = avx512_norm_sq(a, dim);
    float norm_b = avx512_norm_sq(b, dim);
#elif defined(__AVX2__)
    float dot    = avx2_inner_product(a, b, dim);
    float norm_a = avx2_norm_sq(a, dim);
    float norm_b = avx2_norm_sq(b, dim);
#elif defined(__ARM_NEON) || defined(__aarch64__)
    float dot    = neon_inner_product(a, b, dim);
    float norm_a = neon_norm_sq(a, dim);
    float norm_b = neon_norm_sq(b, dim);
#else
    float dot    = scalar_inner_product(a, b, dim);
    float norm_a = scalar_norm_sq(a, dim);
    float norm_b = scalar_norm_sq(b, dim);
#endif
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-10f) return 1.0f; // treat zero-norm vectors as maximally distant
    float cosine_sim = dot / denom;
    // Clamp to [-1, 1] to guard against floating-point rounding errors
    if (cosine_sim > 1.0f) cosine_sim = 1.0f;
    if (cosine_sim < -1.0f) cosine_sim = -1.0f;
    return 1.0f - cosine_sim;
}

// batch_cosine_similarity: compute cosine_similarity(query, db[i]) for all i.
// Implemented as 1.0f - cosine_distance per element; future SIMD optimisation
// can fuse the norm computation across the batch.
void batch_cosine_similarity(const float* query, const float* database,
                              std::size_t n, std::size_t dim, float* results) {
    for (std::size_t i = 0; i < n; ++i) {
        results[i] = 1.0f - cosine_distance(query, database + i * dim, dim);
    }
}

} // namespace simd
} // namespace themis
