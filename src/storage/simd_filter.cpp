/**
 * @file simd_filter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=29, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "storage/simd_filter.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>

// ── SIMD headers (same guard pattern as vec_knn.cpp) ────────────────────────
#if defined(__AVX512F__)
#  define THEMIS_SIMD_FILTER_AVX512 1
#  include <immintrin.h>
#elif defined(__AVX2__)
#  define THEMIS_SIMD_FILTER_AVX2 1
#  include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#  define THEMIS_SIMD_FILTER_NEON 1
#  include <arm_neon.h>
#endif

#if defined(_MSC_VER) && (defined(__AVX2__) || defined(__AVX512F__))
#  include <immintrin.h>
#  ifndef THEMIS_SIMD_FILTER_AVX2
#    define THEMIS_SIMD_FILTER_AVX2 1
#  endif
#  include <intrin.h>  // _BitScanForward, __cpuid
#endif

// cpuid.h provides __get_cpuid_count() for GCC/Clang (x86/x86_64).
// It handles PIC-safe EBX save/restore internally.
#if (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__x86_64__) || defined(_M_X64) || \
     defined(__i386__)   || defined(_M_IX86))
#  include <cpuid.h>
#endif

namespace themis {
namespace storage {

// ============================================================================
// SIMD level detection (memoised)
// ============================================================================

SIMDLevel detectSIMDLevel() noexcept {
    static std::atomic<int> cached{-1};
    int v = cached.load(std::memory_order_acquire);
    if (v >= 0) {
        return static_cast<SIMDLevel>(v);
    }

#if defined(THEMIS_SIMD_FILTER_AVX512)
    SIMDLevel level = SIMDLevel::AVX512;
#elif defined(THEMIS_SIMD_FILTER_AVX2)
    SIMDLevel level = SIMDLevel::AVX2;
#elif defined(THEMIS_SIMD_FILTER_NEON)
    // AArch64 NEON: 4×int32 / 2×int64 / 4×float32 / 2×float64 per iteration.
    // Kernels are implemented below and selected when level == NEON.
    SIMDLevel level = SIMDLevel::NEON;
#else
    SIMDLevel level = SIMDLevel::SCALAR;
#endif

    // Additional runtime x86 CPUID check for AVX2 (handles cases where the
    // compiler flags enable AVX2 but runtime detection is needed).
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (level < SIMDLevel::AVX2) {
        // eax=7, ecx=0 → ebx bit 5 = AVX2
        // Use __get_cpuid_count (from <cpuid.h>) which handles PIC-safe
        // EBX save/restore on both 32-bit and 64-bit GCC/Clang targets.
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
        unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
            if (ebx & (1u << 5)) {
                level = SIMDLevel::AVX2;
            }
        }
#elif defined(_MSC_VER)
        // __cpuidex is required to pass the sub-leaf (ECX=0) for leaf 7.
        int cpuInfo[4] = {};
        __cpuidex(cpuInfo, 7, 0);
        if (static_cast<unsigned int>(cpuInfo[1]) & (1u << 5)) {
            level = SIMDLevel::AVX2;
        }
#endif
    }
#endif

    cached.store(static_cast<int>(level), std::memory_order_release);
    return level;
}

// ============================================================================
// Portable bit-scan helper (replaces __builtin_ctz for MSVC)
// ============================================================================

namespace {

// Returns the index of the lowest set bit in x.  x must be non-zero.
inline int themis_ctz(unsigned int x) noexcept {
#if defined(_MSC_VER)
    unsigned long idx = 0;
    _BitScanForward(&idx, x);
    return static_cast<int>(idx);
#else
    return __builtin_ctz(x);
#endif
}

inline void reserve_filter_output(std::vector<uint32_t>& out, size_t n) {
    if (n <= (out.max_size() - out.size())) {
        out.reserve(out.size() + n);
    }
}

// ============================================================================
// Internal scalar kernels
// ============================================================================
template<typename T>
inline bool scalar_cmp(T a, FilterOp op, T b) noexcept {
    switch (op) {
        case FilterOp::EQ: return a == b;
        case FilterOp::NE: return a != b;
        case FilterOp::LT: return a <  b;
        case FilterOp::LE: return a <= b;
        case FilterOp::GT: return a >  b;
        case FilterOp::GE: return a >= b;
    }
    return false;
}

template<typename T>
size_t scalar_filter(const T* data, size_t n, FilterOp op, T thr,
                     std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    for (size_t i = 0; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
            out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

// ============================================================================
// AVX2 kernels for int32
// ============================================================================

#if defined(THEMIS_SIMD_FILTER_AVX2) || defined(THEMIS_SIMD_FILTER_AVX512)

/// Build an AVX2 comparison mask (8 × int32) for the given op.
/// Returns an 8-bit integer where bit i is set if lane i passes.
inline int avx2_cmp_i32(__m256i a, __m256i b, FilterOp op) noexcept {
    __m256i mask;
    switch (op) {
        case FilterOp::EQ:
            mask = _mm256_cmpeq_epi32(a, b);
            return _mm256_movemask_ps(_mm256_castsi256_ps(mask));
        case FilterOp::NE:
            mask = _mm256_cmpeq_epi32(a, b);
            return ~_mm256_movemask_ps(_mm256_castsi256_ps(mask)) & 0xFF;
        case FilterOp::LT:
            // a < b  ↔  b > a
            mask = _mm256_cmpgt_epi32(b, a);
            return _mm256_movemask_ps(_mm256_castsi256_ps(mask));
        case FilterOp::LE:
            // a <= b  ↔  !(a > b)
            mask = _mm256_cmpgt_epi32(a, b);
            return ~_mm256_movemask_ps(_mm256_castsi256_ps(mask)) & 0xFF;
        case FilterOp::GT:
            mask = _mm256_cmpgt_epi32(a, b);
            return _mm256_movemask_ps(_mm256_castsi256_ps(mask));
        case FilterOp::GE:
            // a >= b  ↔  !(b > a)
            mask = _mm256_cmpgt_epi32(b, a);
            return ~_mm256_movemask_ps(_mm256_castsi256_ps(mask)) & 0xFF;
    }
    return 0;
}

size_t avx2_filter_i32(const int32_t* data, size_t n, FilterOp op, int32_t thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const __m256i vt = _mm256_set1_epi32(thr);
    size_t i = 0;

    for (; i + 8 <= n; i += 8) {
        __m256i va = _mm256_loadu_si256(
            reinterpret_cast<const __m256i*>(data + i));
        int bits = avx2_cmp_i32(va, vt, op);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    // Scalar tail
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
            out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

/// AVX2 kernel for int64 (4 lanes per iteration).
inline int avx2_cmp_i64(__m256i a, __m256i b, FilterOp op) noexcept {
    __m256i mask;
    switch (op) {
        case FilterOp::EQ:
            mask = _mm256_cmpeq_epi64(a, b);
            return _mm256_movemask_pd(_mm256_castsi256_pd(mask));
        case FilterOp::NE:
            mask = _mm256_cmpeq_epi64(a, b);
            return ~_mm256_movemask_pd(_mm256_castsi256_pd(mask)) & 0xF;
        case FilterOp::LT:
            mask = _mm256_cmpgt_epi64(b, a);
            return _mm256_movemask_pd(_mm256_castsi256_pd(mask));
        case FilterOp::LE:
            mask = _mm256_cmpgt_epi64(a, b);
            return ~_mm256_movemask_pd(_mm256_castsi256_pd(mask)) & 0xF;
        case FilterOp::GT:
            mask = _mm256_cmpgt_epi64(a, b);
            return _mm256_movemask_pd(_mm256_castsi256_pd(mask));
        case FilterOp::GE:
            mask = _mm256_cmpgt_epi64(b, a);
            return ~_mm256_movemask_pd(_mm256_castsi256_pd(mask)) & 0xF;
    }
    return 0;
}

size_t avx2_filter_i64(const int64_t* data, size_t n, FilterOp op, int64_t thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const __m256i vt = _mm256_set1_epi64x(thr);
    size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        __m256i va = _mm256_loadu_si256(
            reinterpret_cast<const __m256i*>(data + i));
        int bits = avx2_cmp_i64(va, vt, op);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
            out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

/// AVX2 kernel for float32 (8 lanes per iteration).
size_t avx2_filter_f32(const float* data, size_t n, FilterOp op, float thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const __m256 vt = _mm256_set1_ps(thr);
    size_t i = 0;

    for (; i + 8 <= n; i += 8) {
        const __m256 va = _mm256_loadu_ps(data + i);
        __m256 cmp;
        switch (op) {
            case FilterOp::EQ: cmp = _mm256_cmp_ps(va, vt, _CMP_EQ_OQ);  break;
            case FilterOp::NE: cmp = _mm256_cmp_ps(va, vt, _CMP_NEQ_OQ); break;
            case FilterOp::LT: cmp = _mm256_cmp_ps(va, vt, _CMP_LT_OQ);  break;
            case FilterOp::LE: cmp = _mm256_cmp_ps(va, vt, _CMP_LE_OQ);  break;
            case FilterOp::GT: cmp = _mm256_cmp_ps(va, vt, _CMP_GT_OQ);  break;
            case FilterOp::GE: cmp = _mm256_cmp_ps(va, vt, _CMP_GE_OQ);  break;
            default:           cmp = _mm256_setzero_ps(); break;
        }
        int bits = _mm256_movemask_ps(cmp);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
            out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

/// AVX2 kernel for float64 (4 lanes per iteration).
size_t avx2_filter_f64(const double* data, size_t n, FilterOp op, double thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const __m256d vt = _mm256_set1_pd(thr);
    size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        const __m256d va = _mm256_loadu_pd(data + i);
        __m256d cmp;
        switch (op) {
            case FilterOp::EQ: cmp = _mm256_cmp_pd(va, vt, _CMP_EQ_OQ);  break;
            case FilterOp::NE: cmp = _mm256_cmp_pd(va, vt, _CMP_NEQ_OQ); break;
            case FilterOp::LT: cmp = _mm256_cmp_pd(va, vt, _CMP_LT_OQ);  break;
            case FilterOp::LE: cmp = _mm256_cmp_pd(va, vt, _CMP_LE_OQ);  break;
            case FilterOp::GT: cmp = _mm256_cmp_pd(va, vt, _CMP_GT_OQ);  break;
            case FilterOp::GE: cmp = _mm256_cmp_pd(va, vt, _CMP_GE_OQ);  break;
            default:           cmp = _mm256_setzero_pd(); break;
        }
        int bits = _mm256_movemask_pd(cmp);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
            out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

#endif // AVX2 / AVX512

// ============================================================================
// ARM NEON kernels (AArch64 / ARMv8)
// ============================================================================
// Lane widths per iteration:
//   int32  : 4 lanes  (128-bit vceqq_s32 / vcgtq_s32)
//   int64  : 2 lanes  (128-bit vceqq_s64 / vcgtq_s64)  – AArch64 only
//   float32: 4 lanes  (128-bit vceqq_f32 / vcgtq_f32)
//   float64: 2 lanes  (128-bit vceqq_f64 / vcgtq_f64)  – AArch64 only
//
// Mask extraction uses vmovemask equivalents built from vget_high/vget_low
// and vzip to produce a 4-bit (int32/float32) or 2-bit (int64/float64) mask.
// ============================================================================

#if defined(THEMIS_SIMD_FILTER_NEON)

// Helper: collapse a uint32x4 predicate mask into a 4-bit integer where
// bit i is set when lane i is all-ones (0xFFFFFFFF).
static inline int neon_movemask_u32([[maybe_unused]] uint32x4_t mask) noexcept {
    // Shift each lane to its sign bit position then OR together.
    const uint32x4_t shift = {0, 1, 2, 3};
    uint32x4_t bits = vshrq_n_u32(vshlq_u32(mask, vreinterpretq_s32_u32(
                           vsubq_u32(vdupq_n_u32(31), shift))), 31);
    // Horizontal add via pairwise
    uint32x2_t lo = vget_low_u32(bits);
    uint32x2_t hi = vget_high_u32(bits);
    uint32x2_t pair = vorr_u32(lo, vshl_u32(hi, vdup_n_s32(2)));
    return static_cast<int>(vget_lane_u32(vpadd_u32(pair, pair), 0));
}

// Helper: collapse a uint64x2 predicate mask into a 2-bit integer.
static inline int neon_movemask_u64([[maybe_unused]] uint64x2_t mask) noexcept {
    uint64_t lo = vgetq_lane_u64(mask, 0) & 1u;
    uint64_t hi = vgetq_lane_u64(mask, 1) & 1u;
    return static_cast<int>(lo | (hi << 1));
}

// ── int32 (4 lanes) ──────────────────────────────────────────────────────────
size_t neon_filter_i32(const int32_t* data, size_t n, FilterOp op, int32_t thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const int32x4_t vt = vdupq_n_s32(thr);
    size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        int32x4_t va = vld1q_s32(data + i);
        uint32x4_t pred;
        switch (op) {
            case FilterOp::EQ: pred = vceqq_s32(va, vt); break;
            case FilterOp::NE: pred = vmvnq_u32(vceqq_s32(va, vt)); break;
            case FilterOp::LT: pred = vcltq_s32(va, vt); break;
            case FilterOp::LE: pred = vcleq_s32(va, vt); break;
            case FilterOp::GT: pred = vcgtq_s32(va, vt); break;
            case FilterOp::GE: pred = vcgeq_s32(va, vt); break;
            default: pred = vdupq_n_u32(0); break;
        }
        int bits = neon_movemask_u32(pred);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
          out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

// ── int64 (2 lanes, AArch64) ─────────────────────────────────────────────────
size_t neon_filter_i64(const int64_t* data, size_t n, FilterOp op, int64_t thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const int64x2_t vt = vdupq_n_s64(thr);
    size_t i = 0;

    for (; i + 2 <= n; i += 2) {
        int64x2_t va = vld1q_s64(data + i);
        uint64x2_t pred;
        switch (op) {
            case FilterOp::EQ: pred = vceqq_s64(va, vt); break;
            case FilterOp::NE: pred = vmvnq_u64(vceqq_s64(va, vt)); break;
            case FilterOp::LT: pred = vcltq_s64(va, vt); break;
            case FilterOp::LE: pred = vcleq_s64(va, vt); break;
            case FilterOp::GT: pred = vcgtq_s64(va, vt); break;
            case FilterOp::GE: pred = vcgeq_s64(va, vt); break;
            default: pred = vdupq_n_u64(0); break;
        }
        int bits = neon_movemask_u64(pred);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
          out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

// ── float32 (4 lanes) ────────────────────────────────────────────────────────
size_t neon_filter_f32(const float* data, size_t n, FilterOp op, float thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const float32x4_t vt = vdupq_n_f32(thr);
    size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(data + i);
        uint32x4_t pred;
        switch (op) {
            case FilterOp::EQ: pred = vceqq_f32(va, vt); break;
            case FilterOp::NE: pred = vmvnq_u32(vceqq_f32(va, vt)); break;
            case FilterOp::LT: pred = vcltq_f32(va, vt); break;
            case FilterOp::LE: pred = vcleq_f32(va, vt); break;
            case FilterOp::GT: pred = vcgtq_f32(va, vt); break;
            case FilterOp::GE: pred = vcgeq_f32(va, vt); break;
            default: pred = vdupq_n_u32(0); break;
        }
        int bits = neon_movemask_u32(pred);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
          out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

// ── float64 (2 lanes, AArch64) ───────────────────────────────────────────────
size_t neon_filter_f64(const double* data, size_t n, FilterOp op, double thr,
                       std::vector<uint32_t>& out) {
    reserve_filter_output(out, n);
    size_t before = out.size();
    const float64x2_t vt = vdupq_n_f64(thr);
    size_t i = 0;

    for (; i + 2 <= n; i += 2) {
        float64x2_t va = vld1q_f64(data + i);
        uint64x2_t pred;
        switch (op) {
            case FilterOp::EQ: pred = vceqq_f64(va, vt); break;
            case FilterOp::NE: pred = vmvnq_u64(vceqq_f64(va, vt)); break;
            case FilterOp::LT: pred = vcltq_f64(va, vt); break;
            case FilterOp::LE: pred = vcleq_f64(va, vt); break;
            case FilterOp::GT: pred = vcgtq_f64(va, vt); break;
            case FilterOp::GE: pred = vcgeq_f64(va, vt); break;
            default: pred = vdupq_n_u64(0); break;
        }
        int bits = neon_movemask_u64(pred);
        while (bits) {
            int lane = themis_ctz(static_cast<unsigned>(bits));
            out.push_back(static_cast<uint32_t>(i + lane));
            bits &= bits - 1;
        }
    }
    for (; i < n; ++i) {
        if (scalar_cmp(data[i], op, thr)) {
          out.push_back(static_cast<uint32_t>(i));
        }
    }
    return out.size() - before;
}

#endif // THEMIS_SIMD_FILTER_NEON

} // namespace

// ============================================================================
// Public API implementations
// ============================================================================

size_t simd_filter_int32(const int32_t* data, size_t n, FilterOp op,
                         int32_t threshold, std::vector<uint32_t>& out) {
    if (!data || n == 0) {
      return 0;
    }
#if defined(THEMIS_SIMD_FILTER_AVX2) || defined(THEMIS_SIMD_FILTER_AVX512)
    if (detectSIMDLevel() >= SIMDLevel::AVX2) {
        return avx2_filter_i32(data, n, op, threshold, out);
    }
#endif
#if defined(THEMIS_SIMD_FILTER_NEON)
    if (detectSIMDLevel() == SIMDLevel::NEON) {
        return neon_filter_i32(data, n, op, threshold, out);
    }
#endif
    return scalar_filter(data, n, op, threshold, out);
}

size_t simd_filter_int64(const int64_t* data, size_t n, FilterOp op,
                         int64_t threshold, std::vector<uint32_t>& out) {
    if (!data || n == 0) {
      return 0;
    }
#if defined(THEMIS_SIMD_FILTER_AVX2) || defined(THEMIS_SIMD_FILTER_AVX512)
    if (detectSIMDLevel() >= SIMDLevel::AVX2) {
        return avx2_filter_i64(data, n, op, threshold, out);
    }
#endif
#if defined(THEMIS_SIMD_FILTER_NEON)
    if (detectSIMDLevel() == SIMDLevel::NEON) {
        return neon_filter_i64(data, n, op, threshold, out);
    }
#endif
    return scalar_filter(data, n, op, threshold, out);
}

size_t simd_filter_float(const float* data, size_t n, FilterOp op,
                         float threshold, std::vector<uint32_t>& out) {
    if (!data || n == 0) {
      return 0;
    }
#if defined(THEMIS_SIMD_FILTER_AVX2) || defined(THEMIS_SIMD_FILTER_AVX512)
    if (detectSIMDLevel() >= SIMDLevel::AVX2) {
        return avx2_filter_f32(data, n, op, threshold, out);
    }
#endif
#if defined(THEMIS_SIMD_FILTER_NEON)
    if (detectSIMDLevel() == SIMDLevel::NEON) {
        return neon_filter_f32(data, n, op, threshold, out);
    }
#endif
    return scalar_filter(data, n, op, threshold, out);
}

size_t simd_filter_double(const double* data, size_t n, FilterOp op,
                          double threshold, std::vector<uint32_t>& out) {
    if (!data || n == 0) {
      return 0;
    }
#if defined(THEMIS_SIMD_FILTER_AVX2) || defined(THEMIS_SIMD_FILTER_AVX512)
    if (detectSIMDLevel() >= SIMDLevel::AVX2) {
        return avx2_filter_f64(data, n, op, threshold, out);
    }
#endif
#if defined(THEMIS_SIMD_FILTER_NEON)
    if (detectSIMDLevel() == SIMDLevel::NEON) {
        return neon_filter_f64(data, n, op, threshold, out);
    }
#endif
    return scalar_filter(data, n, op, threshold, out);
}

// ============================================================================
// SIMDColumnFilter implementation
// ============================================================================

namespace {

/// Zone-map guard: returns true if the segment can be skipped entirely.
bool canSkipSegmentForPred(const ColumnSegment& seg,
                           const ColumnPredicate& pred) noexcept {
    const ZoneMap& zm = seg.metadata().zone_map;
    if (zm.row_count == 0) {
      return true;
    }

    switch (pred.column_type) {
        case ColumnType::INT32:
        [[fallthrough]];\n        case ColumnType::INT64: {
            int64_t thr = (pred.column_type == ColumnType::INT32)
                ? static_cast<int64_t>(pred.threshold.i32)
                : pred.threshold.i64;
            switch (pred.op) {
                [[fallthrough]];\n                case FilterOp::EQ: return zm.canSkipForInt(thr);
                case FilterOp::NE: return false; // may always match
                case FilterOp::LT: return thr <= zm.min_int; // all >= min, need < thr
                case FilterOp::LE: return thr <  zm.min_int;
                case FilterOp::GT: return thr >= zm.max_int;
                case FilterOp::GE: return thr >  zm.max_int;
            }
            break;
        }
        case ColumnType::FLOAT32:
        [[fallthrough]];\n        case ColumnType::FLOAT64: {
            double thr = (pred.column_type == ColumnType::FLOAT32)
                ? static_cast<double>(pred.threshold.f32)
                : pred.threshold.f64;
            switch (pred.op) {
                [[fallthrough]];\n                case FilterOp::EQ: return zm.canSkipForFloat(thr);
                case FilterOp::NE: return false;
                case FilterOp::LT: return thr <= zm.min_float;
                case FilterOp::LE: return thr <  zm.min_float;
                case FilterOp::GT: return thr >= zm.max_float;
                case FilterOp::GE: return thr >  zm.max_float;
            }
            break;
        }
        default:
            break;
    }
    return false;
}

} // namespace

std::vector<uint32_t> SIMDColumnFilter::scan(const ColumnSegment& segment,
                                              const ColumnPredicate& predicate) {
    auto t0 = std::chrono::steady_clock::now();

    stats_.simd_level  = detectSIMDLevel();
    size_t n           = segment.metadata().row_count;

    std::vector<uint32_t> result;

    // Zone-map early-out
    if (canSkipSegmentForPred(segment, predicate)) {
        stats_.rows_processed += n;
        stats_.rows_matched   += 0;
        auto t1 = std::chrono::steady_clock::now();
        stats_.elapsed_us +=
            std::chrono::duration<double, std::micro>(t1 - t0).count();
        return result;
    }

    const auto& raw = segment.rawData();
    if (raw.empty() || n == 0) {
        return result;
    }

    result.reserve(n / 4);

    switch (predicate.column_type) {
        case ColumnType::INT32: {
            const int32_t* p = reinterpret_cast<const int32_t*>(raw.data());
            simd_filter_int32(p, n, predicate.op, predicate.threshold.i32, result);
            break;
        }
        case ColumnType::INT64: {
            const int64_t* p = reinterpret_cast<const int64_t*>(raw.data());
            simd_filter_int64(p, n, predicate.op, predicate.threshold.i64, result);
            break;
        }
        case ColumnType::FLOAT32: {
            const float* p = reinterpret_cast<const float*>(raw.data());
            simd_filter_float(p, n, predicate.op, predicate.threshold.f32, result);
            break;
        }
        case ColumnType::FLOAT64: {
            const double* p = reinterpret_cast<const double*>(raw.data());
            simd_filter_double(p, n, predicate.op, predicate.threshold.f64, result);
            break;
        }
        default:
            // BOOL / STRING: fall through to scalar path using raw bytes as int8
            for (size_t i = 0; i < n; ++i) {
                if (raw[i] != 0) {  // BOOL: non-zero = true
                    result.push_back(static_cast<uint32_t>(i));
                }
            }
            break;
    }

    auto t1 = std::chrono::steady_clock::now();
    stats_.rows_processed += n;
    stats_.rows_matched   += result.size();
    stats_.elapsed_us     +=
        std::chrono::duration<double, std::micro>(t1 - t0).count();

    return result;
}

std::vector<uint32_t> SIMDColumnFilter::scanBatch(
    const std::vector<ColumnSegment>& segments,
    const ColumnPredicate& predicate) {

    std::vector<uint32_t> result;
    uint32_t row_offset = 0;
    size_t total_rows = 0;
    for (const auto& seg : segments) {
        total_rows += seg.metadata().row_count;
    }
    if (total_rows <= result.max_size()) {
        result.reserve(total_rows);
    }

    for (const auto& seg : segments) {
        auto local = scan(seg, predicate);
        for (uint32_t idx : local) {
            result.push_back(idx + row_offset);
        }
        row_offset += static_cast<uint32_t>(seg.metadata().row_count);
    }
    return result;
}

void SIMDColumnFilter::resetStats() noexcept {
    stats_ = SIMDFilterStats{};
}

bool SIMDColumnFilter::canSkipSegment(const ColumnSegment& segment,
                                      const ColumnPredicate& pred) noexcept {
    return canSkipSegmentForPred(segment, pred);
}

} // namespace storage
} // namespace themis
