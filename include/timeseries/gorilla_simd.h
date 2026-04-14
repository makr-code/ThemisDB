/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gorilla_simd.h                                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:27:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f151b2a1bc  2026-03-13  feat(timeseries): vectorised Gorilla chunk decoder with S... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_GORILLA_SIMD_H
#define THEMIS_GORILLA_SIMD_H

#include "timeseries/gorilla.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace themis {

/// Returns true if AVX2 is available on this CPU at runtime (x86-64 only).
/// On all other platforms this always returns false.
bool gorilla_simd_has_avx2() noexcept;

/// Returns true if NEON is available at runtime.
/// On ARM64 targets this always returns true; on all other platforms false.
bool gorilla_simd_has_neon() noexcept;

/**
 * GorillaSIMDDecoder — Vectorised batch Gorilla chunk decoder.
 *
 * Decodes an entire Gorilla-compressed chunk in one call via a two-phase
 * approach that uses SIMD intrinsics for the reconstruction phase:
 *
 *   Phase 1 (scalar):  Parse the bit-stream into flat intermediate arrays
 *                      of delta-of-deltas (timestamps) and XOR values
 *                      (double bit-patterns).
 *
 *   Phase 2 (SIMD):    Reconstruct timestamps with two SIMD prefix-sum
 *                      passes (dod → Δt → t) and reconstruct double
 *                      values with a SIMD prefix-XOR pass.
 *
 * Platform selection is performed at runtime:
 *   • AVX2   on x86-64 (checked via CPUID leaf 7, EBX bit 5)
 *   • NEON   on AArch64 (always present on ARMv8-A and later)
 *   • Scalar fallback via GorillaDecoder on all other platforms
 *
 * The output of decodeAll() is byte-for-byte identical to the output of
 * GorillaDecoder::next() called in a loop.
 *
 * Input validation: decodeAll() sets hasError() to true when a truncated or
 * structurally corrupt chunk is encountered; partial results decoded before
 * the error are still appended to `out`.
 */
class GorillaSIMDDecoder {
public:
    explicit GorillaSIMDDecoder(std::vector<uint8_t> data);

    /**
     * Decode all points from the chunk and append them to `out`.
     * @return  Number of newly decoded points appended.
     */
    size_t decodeAll(std::vector<std::pair<int64_t, double>>& out);

    /// True if a decode error (truncated / corrupt data) was encountered.
    bool hasError() const { return error_; }

    /// Total number of successfully decoded points so far.
    size_t decodedCount() const { return decoded_count_; }

private:
    std::vector<uint8_t> data_;
    bool error_{false};
    size_t decoded_count_{0};
};

} // namespace themis

#endif // THEMIS_GORILLA_SIMD_H
