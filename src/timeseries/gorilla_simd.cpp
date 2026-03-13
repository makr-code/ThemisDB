#include "timeseries/gorilla_simd.h"
#include "timeseries/gorilla.h"

#include <cstring>
#include <cstdint>
#include <vector>

#if defined(__AVX2__)
#  include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#  include <arm_neon.h>
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#  if defined(_MSC_VER)
#    include <intrin.h>
#  else
#    include <cpuid.h>
#  endif
#endif

namespace themis {

// ──────────────────────────────────────────────────────────────────────────
// Runtime CPU feature detection
// ──────────────────────────────────────────────────────────────────────────

bool gorilla_simd_has_avx2() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#  if defined(_MSC_VER)
    int info[4] = {};
    __cpuidex(info, 7, 0);
    return (info[1] & (1 << 5)) != 0;  // EBX bit 5 = AVX2
#  else
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    return (ebx & (1u << 5)) != 0;     // EBX bit 5 = AVX2
#  endif
#else
    return false;
#endif
}

bool gorilla_simd_has_neon() noexcept {
#if defined(__ARM_NEON) || defined(__aarch64__)
    return true;
#else
    return false;
#endif
}

// ──────────────────────────────────────────────────────────────────────────
// Helpers shared across paths
// ──────────────────────────────────────────────────────────────────────────

static inline double bits_to_dbl_simd(uint64_t b) {
    double v;
    std::memcpy(&v, &b, sizeof(v));
    return v;
}

// ──────────────────────────────────────────────────────────────────────────
// Phase 1: scalar bit-stream parser into intermediate arrays
//
// The output arrays are:
//   dods[i]     delta-of-delta for point (i+1), i in [0, n-2]
//   xorvals[i]  XOR pattern for point (i+1) (0 when value == previous)
// ──────────────────────────────────────────────────────────────────────────

struct GorillaParsed {
    bool        error{false};
    int64_t     first_ts{0};
    uint64_t    first_vbits{0};
    std::vector<int64_t>  dods;
    std::vector<uint64_t> xorvals;
};

static GorillaParsed parse_gorilla_chunk(const std::vector<uint8_t>& data) {
    GorillaParsed out;

    if (data.empty()) return out;

    BitReader br(data);

    // ── First point ──────────────────────────────────────────────────────
    br.alignToByte();
    if (br.eof()) return out;

    out.first_ts    = br.readZigZag64();
    if (br.eof()) { out.error = true; return out; }
    out.first_vbits = br.readBits(64);

    // ── Subsequent points ─────────────────────────────────────────────────
    uint64_t prev_vbits   = out.first_vbits;
    int      prev_leading = 64;
    int      prev_trailing = 64;

    while (true) {
        br.alignToByte();
        if (br.eof()) break;

        // Timestamp delta-of-delta
        int64_t dod = br.readZigZag64();

        // Detect truncation: EOF immediately after the ZigZag means the value
        // bits were never written — this is a corrupt/truncated chunk.
        if (br.eof()) { out.error = true; break; }
        bool different = br.readBit();

        uint64_t xorv = 0;
        if (different) {
            if (br.eof()) { out.error = true; break; }
            int leading    = static_cast<int>(br.readBits(6));
            if (br.eof()) { out.error = true; break; }
            int significant = static_cast<int>(br.readBits(6));
            if (significant == 0) significant = 64;
            if (leading + significant > 64) { out.error = true; break; }
            if (br.eof() && significant > 0) { out.error = true; break; }
            uint64_t payload = br.readBits(significant);
            int trailing     = 64 - leading - significant;
            xorv = payload << trailing;
            prev_leading  = leading;
            prev_trailing = trailing;
            prev_vbits ^= xorv;
        }
        // Only commit this point once we have successfully decoded both
        // the timestamp dod and the value field.
        out.dods.push_back(dod);
        out.xorvals.push_back(xorv);
    }

    return out;
}

// ──────────────────────────────────────────────────────────────────────────
// Phase 2a: in-place prefix-sum on int64_t array (for timestamps)
//
//   On exit: arr[i] = seed + arr[0] + arr[1] + … + arr[i]
//
// AVX2 path processes 4 × int64_t per iteration using an in-register
// Kogge-Stone prefix scan.  Falls back to scalar on non-AVX2 targets.
// ──────────────────────────────────────────────────────────────────────────

static void prefix_sum_i64(int64_t* arr, size_t n, int64_t seed) {
    size_t i = 0;

#if defined(__AVX2__)
    // In-register prefix scan for groups of 4 × int64_t.
    //
    // For input [a, b, c, d] the scan produces [a, a+b, a+b+c, a+b+c+d],
    // then adds `carry` to every element.
    //
    // Step 1  shift-right-by-1:  [0, a, b, c]
    //         _mm256_permute4x64_epi64(v, 0x93) → [d, a, b, c]
    //         _mm256_blend_epi32(..., 0x03)      → [0, a, b, c]
    // Step 2  add:               [a, a+b, b+c, c+d]
    // Step 3  shift-right-by-2:  [0, 0, a, a+b]
    //         _mm256_permute2x128_si256(zero, v, 0x20)
    // Step 4  add:               [a, a+b, a+b+c, a+b+c+d]
    // Step 5  add carry from previous batch.

    int64_t carry = seed;
    for (; i + 4 <= n; i += 4) {
        __m256i v    = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(arr + i));
        __m256i zero = _mm256_setzero_si256();

        // Shift right by 1 element with 0-fill
        __m256i s1 = _mm256_permute4x64_epi64(v, 0x93);          // [d, a, b, c]
        s1 = _mm256_blend_epi32(s1, zero, 0x03);                  // [0, a, b, c]
        __m256i v2 = _mm256_add_epi64(v, s1);                     // [a, a+b, b+c, c+d]

        // Shift right by 2 elements with 0-fill
        __m256i s2 = _mm256_permute2x128_si256(zero, v2, 0x20);   // [0, 0, a, a+b]
        v2 = _mm256_add_epi64(v2, s2);                            // prefix scan

        // Add carry from previous batch
        __m256i cv = _mm256_set1_epi64x(carry);
        v2 = _mm256_add_epi64(v2, cv);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(arr + i), v2);
        carry = arr[i + 3];
    }
    // Scalar tail
    for (; i < n; ++i) {
        carry += arr[i];
        arr[i] = carry;
    }

#elif defined(__ARM_NEON) || defined(__aarch64__)
    // NEON path: 2 × int64_t per register.
    int64_t carry = seed;
    for (; i + 2 <= n; i += 2) {
        int64x2_t v       = vld1q_s64(arr + i);
        int64x2_t zero128 = vdupq_n_s64(0);
        // shift right by 1: [0, v[0]] from [v[0], v[1]]
        int64x2_t shifted = vextq_s64(zero128, v, 1);  // [zero[1], v[0]] = [0, v[0]]
        v = vaddq_s64(v, shifted);                      // [v[0], v[0]+v[1]]
        int64x2_t cv = vdupq_n_s64(carry);
        v = vaddq_s64(v, cv);
        vst1q_s64(arr + i, v);
        carry = arr[i + 1];
    }
    for (; i < n; ++i) {
        carry += arr[i];
        arr[i] = carry;
    }

#else
    // Scalar fallback
    int64_t carry = seed;
    for (; i < n; ++i) {
        carry += arr[i];
        arr[i] = carry;
    }
#endif
}

// ──────────────────────────────────────────────────────────────────────────
// Phase 2b: in-place prefix-XOR on uint64_t array (for double bit-patterns)
//
//   On exit: arr[i] = seed XOR arr[0] XOR arr[1] XOR … XOR arr[i]
//
// The XOR operation is associative and commutative; the in-register prefix
// scan uses the same shift structure as the prefix sum above.
// ──────────────────────────────────────────────────────────────────────────

static void prefix_xor_u64(uint64_t* arr, size_t n, uint64_t seed) {
    size_t i = 0;

#if defined(__AVX2__)
    uint64_t carry = seed;
    for (; i + 4 <= n; i += 4) {
        __m256i v    = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(arr + i));
        __m256i zero = _mm256_setzero_si256();

        __m256i s1 = _mm256_permute4x64_epi64(v, 0x93);
        s1 = _mm256_blend_epi32(s1, zero, 0x03);          // [0, a, b, c]
        __m256i v2 = _mm256_xor_si256(v, s1);             // [a, a^b, b^c, c^d]

        __m256i s2 = _mm256_permute2x128_si256(zero, v2, 0x20); // [0,0,a,a^b]
        v2 = _mm256_xor_si256(v2, s2);                    // prefix XOR

        __m256i cv = _mm256_set1_epi64x(static_cast<int64_t>(carry));
        v2 = _mm256_xor_si256(v2, cv);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(arr + i), v2);
        carry = arr[i + 3];
    }
    for (; i < n; ++i) {
        carry ^= arr[i];
        arr[i] = carry;
    }

#elif defined(__ARM_NEON) || defined(__aarch64__)
    uint64_t carry = seed;
    for (; i + 2 <= n; i += 2) {
        uint64x2_t v       = vld1q_u64(arr + i);
        uint64x2_t zero128 = vdupq_n_u64(0);
        uint64x2_t shifted = vextq_u64(zero128, v, 1);  // [0, v[0]]
        v = veorq_u64(v, shifted);                       // [v[0], v[0]^v[1]]
        uint64x2_t cv = vdupq_n_u64(carry);
        v = veorq_u64(v, cv);
        vst1q_u64(arr + i, v);
        carry = arr[i + 1];
    }
    for (; i < n; ++i) {
        carry ^= arr[i];
        arr[i] = carry;
    }

#else
    uint64_t carry = seed;
    for (; i < n; ++i) {
        carry ^= arr[i];
        arr[i] = carry;
    }
#endif
}

// ──────────────────────────────────────────────────────────────────────────
// Scalar fallback path using existing GorillaDecoder
// ──────────────────────────────────────────────────────────────────────────

static size_t decode_scalar(const std::vector<uint8_t>& data,
                            std::vector<std::pair<int64_t, double>>& out,
                            bool& error_out) {
    GorillaDecoder dec(data);
    size_t count = 0;
    while (auto p = dec.next()) {
        out.push_back(*p);
        ++count;
    }
    error_out = dec.hasError();
    return count;
}

// ──────────────────────────────────────────────────────────────────────────
// GorillaSIMDDecoder implementation
// ──────────────────────────────────────────────────────────────────────────

GorillaSIMDDecoder::GorillaSIMDDecoder(std::vector<uint8_t> data)
    : data_(std::move(data)) {}

size_t GorillaSIMDDecoder::decodeAll(std::vector<std::pair<int64_t, double>>& out) {
    if (data_.empty()) return 0;

    // Phase 1: parse the compressed bit-stream into intermediate arrays.
    GorillaParsed parsed = parse_gorilla_chunk(data_);
    error_ = parsed.error;

    const size_t subsequent = parsed.dods.size();  // == parsed.xorvals.size()
    const size_t total      = 1 + subsequent;

    // Allocate output storage and set the first point.
    out.reserve(out.size() + total);

    if (subsequent == 0) {
        // Only one point was in the stream (or parse failed on the first point).
        if (!parsed.error || total >= 1) {
            out.emplace_back(parsed.first_ts, bits_to_dbl_simd(parsed.first_vbits));
            decoded_count_ += 1;
        }
        return 1;
    }

    // ── Phase 2a: reconstruct timestamps ─────────────────────────────────
    //
    // dods[] contains delta-of-deltas for points [1 .. n-1].
    // We apply two successive in-place prefix sums:
    //
    //   Pass 1 (seed = 0):            dods[i] → dt[i+1]
    //                                 (cumulative sum of dods gives Δt)
    //
    //   Pass 2 (seed = first_ts):     dt[i+1] → ts[i+1]
    //                                 (cumulative sum of Δt with first_ts)

    prefix_sum_i64(parsed.dods.data(), subsequent, 0);
    prefix_sum_i64(parsed.dods.data(), subsequent, parsed.first_ts);

    // ── Phase 2b: reconstruct double bit-patterns ─────────────────────────
    //
    // xorvals[i] contains the XOR pattern for point (i+1); 0 means "same value".
    // A single prefix-XOR pass with seed = first_vbits reconstructs all vbits:
    //
    //   vbits[i+1] = first_vbits XOR xorvals[0] XOR … XOR xorvals[i]

    prefix_xor_u64(parsed.xorvals.data(), subsequent, parsed.first_vbits);

    // ── Assemble output ───────────────────────────────────────────────────

    out.emplace_back(parsed.first_ts, bits_to_dbl_simd(parsed.first_vbits));
    for (size_t i = 0; i < subsequent; ++i) {
        out.emplace_back(parsed.dods[i], bits_to_dbl_simd(parsed.xorvals[i]));
    }

    decoded_count_ += total;
    return total;
}

} // namespace themis
