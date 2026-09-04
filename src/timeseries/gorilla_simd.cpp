/**
 * @file gorilla_simd.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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

static inline double bits_to_dbl_simd([[maybe_unused]] uint64_t b) {
    double v = 0;
    std::memcpy(&v, &b, sizeof(v));
    return v;
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
// GorillaSIMDDecoder implementation
//
// Decodes in batches of 4 points to leverage the SIMD prefix-sum/XOR helpers
// while keeping intermediate buffers tiny (4×8 bytes each → register-sized).
// This avoids the large heap allocations and extra memory passes that a
// full-chunk intermediate-array approach would require.
// ──────────────────────────────────────────────────────────────────────────

GorillaSIMDDecoder::GorillaSIMDDecoder(std::vector<uint8_t> data)
    : data_(std::move(data)) {}

size_t GorillaSIMDDecoder::decodeAll(std::vector<std::pair<int64_t, double>>& out) {
    if (data_.empty()) {
      return 0;
    }

#if defined(__AVX2__)
    // Runtime guard for binaries compiled with AVX2 but executed on non-AVX2 CPUs.
    // In that case we must not execute AVX2 instructions.
    // Cache the CPUID result as a static constant so the CPUID instruction is
    // executed only once per process lifetime (not on every decode call).
    static const bool kHasAVX2 = gorilla_simd_has_avx2();
    if (!kHasAVX2) {
        GorillaDecoder fallback(data_);
        const size_t out_begin = out.size();
        // Same conservative estimate used by the SIMD path below:
        // Gorilla points are at least ~1 byte encoded, so payload_size/2 + 1
        // avoids repeated reallocations without changing decode semantics.
        out.reserve(out.size() + data_.size() / 2 + 1);
        while (auto p = fallback.next()) {
            out.push_back(*p);
        }
        error_ = fallback.hasError();
        const size_t appended = out.size() - out_begin;
        decoded_count_ += appended;
        return appended;
    }
#endif

    // Detect and validate the Gorilla chunk header (3 bytes: magic0, magic1, version).
    // Legacy chunks (encoded before v1) have no header; fall through to decode as-is.
    const uint8_t* payload_ptr  = data_.data();
    size_t         payload_size = data_.size();

    if (data_.size() >= 3 &&
            data_[0] == kGorillaMagic0 &&
            data_[1] == kGorillaMagic1) {
        if (data_[2] != kGorillaCurrentVersion) {
            error_ = true;
            return 0;  // Unsupported chunk format version
        }
        payload_ptr  += 3;
        payload_size -= 3;
    }

    if (payload_size == 0) {
      return 0;
    }

    // Pass raw pointer+size directly — avoids an unnecessary heap allocation
    // that would otherwise be needed just to strip the 3-byte header.
    BitReader br(payload_ptr, payload_size);

    // ── First point ───────────────────────────────────────────────────────
    br.alignToByte();
    if (br.eof()) {
      return 0;
    }

    int64_t  first_ts    = br.readZigZag64();
    if (br.eof()) { error_ = true; return 0; }
    uint64_t first_vbits = br.readBits(64);

    // Reserve output with a conservative estimate (each compressed point is at
    // minimum ~1 byte, so payload_size / 2 + 1 is a safe upper bound).
    out.reserve(out.size() + payload_size / 2 + 1);
    out.emplace_back(first_ts, bits_to_dbl_simd(first_vbits));

    // Carry state: dt and ts of the last emitted point.
    int64_t  carry_dt  = 0;           // dt for the first point is 0
    int64_t  carry_ts  = first_ts;
    uint64_t carry_xor = first_vbits;

    // Per-batch buffers: sized to process exactly one AVX2 register worth of
    // int64_t elements (4 × 64-bit = 256 bits = one __m256i) per SIMD call.
    // These are stack-allocated and stay in registers/L1 cache throughout.
    static constexpr int kBatchSize = 4;
    alignas(32) int64_t  dods_buf[kBatchSize] = {};
    alignas(32) uint64_t xors_buf[kBatchSize] = {};

    size_t total = 1;

    while (true) {
        // ── Parse up to kBatchSize subsequent points ──────────────────────
        int  batch_size  = 0;
        bool parse_error = false;

        for (int b = 0; b < kBatchSize && !parse_error; ++b) {
            br.alignToByte();
            if (br.eof()) {
              break;
            }

            int64_t dod = br.readZigZag64();

            // EOF immediately after the ZigZag means the value field was
            // never written — truncated/corrupt chunk.
            if (br.eof()) { parse_error = true; break; }
            bool different = br.readBit();

            uint64_t xorv = 0;
            if (different) {
                if (br.eof()) { parse_error = true; break; }
                int leading    = static_cast<int>(br.readBits(6));
                if (br.eof()) { parse_error = true; break; }
                int significant = static_cast<int>(br.readBits(6));
                if (significant == 0) {
                  significant = 64;
                }
                if (leading + significant > 64) { parse_error = true; break; }
                if (br.eof() && significant > 0) { parse_error = true; break; }
                uint64_t payload_bits = br.readBits(significant);
                int trailing = 64 - leading - significant;
                xorv = payload_bits << trailing;
            }

            dods_buf[b] = dod;
            xors_buf[b] = xorv;
            ++batch_size;
        }

        if (batch_size == 0 && !parse_error) break;  // clean end-of-stream
        if (parse_error) {
          error_ = true;
        }
        if (batch_size == 0) {
          break;
        }

        // ── Phase 2a: reconstruct dt[] and ts[] via two prefix-sum passes ──
        //
        //   Pass 1 (seed = carry_dt):  dods_buf[b] → dt values
        //   Pass 2 (seed = carry_ts):  dt values   → ts values
        //
        // carry_dt must be saved BEFORE the second pass overwrites dods_buf.
        prefix_sum_i64(dods_buf, batch_size, carry_dt);
        int64_t new_carry_dt = dods_buf[static_cast<int>(batch_size - 1)];   // dt of last point

        prefix_sum_i64(dods_buf, batch_size, carry_ts);
        int64_t new_carry_ts = dods_buf[static_cast<int>(batch_size - 1)];   // ts of last point

        // ── Phase 2b: reconstruct vbits[] via prefix-XOR ─────────────────
        prefix_xor_u64(xors_buf, batch_size, carry_xor);
        uint64_t new_carry_xor = xors_buf[static_cast<int>(batch_size - 1)]; // vbits of last point

        // ── Emit this batch ───────────────────────────────────────────────
        for (int b = 0; b < batch_size; ++b) {
            out.emplace_back(dods_buf[b], bits_to_dbl_simd(xors_buf[b]));
        }

        total      += static_cast<size_t>(batch_size);
        carry_dt    = new_carry_dt;
        carry_ts    = new_carry_ts;
        carry_xor   = new_carry_xor;

        if (parse_error || batch_size < kBatchSize) {
          break;
        }
    }

    decoded_count_ += total;
    return total;
}

} // namespace themis

