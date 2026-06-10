/**
 * @file codec_tags.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstdint>

namespace themis {
namespace compression {

// ============================================================================
// Canonical tag byte constants
// ============================================================================

/// No compression applied; payload is stored verbatim.
/// This is the safe fallback when no compression library is compiled in.
constexpr uint8_t kTagPassthrough = 0x00;

/// LZ4 (High Compression variant — LZ4HC).
/// Requires `THEMIS_ENABLE_LZ4`; falls back to passthrough otherwise.
/// Framing: 1 tag byte + 4-byte little-endian original size + LZ4 data.
constexpr uint8_t kTagLZ4         = 0x01;

/// Google Snappy compression.
/// Requires `THEMIS_ENABLE_SNAPPY`; falls back to passthrough otherwise.
/// Framing: 1 tag byte + Snappy data (Snappy encodes its own length).
constexpr uint8_t kTagSnappy      = 0x02;

/// Facebook Zstandard (Zstd) compression at level 3.
/// Requires `THEMIS_ENABLE_ZSTD`; falls back to passthrough otherwise.
/// Framing: 1 tag byte + 4-byte little-endian original size + Zstd frame.
constexpr uint8_t kTagZstd        = 0x03;

// ============================================================================
// Helper: check whether a tag byte is known to this version
// ============================================================================

/// Returns true if @p tag is a recognised codec tag in this version of
/// ThemisDB.  Unknown tags should be treated as a framing error.
[[nodiscard]] constexpr bool is_known_tag(uint8_t tag) noexcept {
    return tag <= kTagZstd;
}

} // namespace compression
} // namespace themis
