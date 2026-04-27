/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            codec_tags.h                                       ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-27                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file codec_tags.h
 * @brief Canonical wire-format tag bytes for ThemisDB compression codecs.
 *
 * Two independent compression subsystems previously defined the same tag bytes
 * locally, risking byte-value drift if one side changed:
 *
 *   - `storage/compression_strategy.cpp` (full pipeline: DataType, level,
 *     adaptive selection, GPU variants)
 *   - `performance/advanced_cache_manager.cpp` (LRU cache compression; uses a
 *     single leading tag byte to identify the algorithm at decompress time)
 *
 * This header is the **single source of truth** for the leading tag byte in
 * any ThemisDB framed payload.  Both subsystems must include this header
 * instead of defining their own magic constants.
 *
 * ### Wire format
 *
 * Every compressed payload starts with exactly **one tag byte** followed by
 * algorithm-specific framing:
 *
 * ```
 * +--------+---------------------------+---------------------+
 * | Tag    | Extra header (optional)   | Compressed payload  |
 * +--------+---------------------------+---------------------+
 * | 1 byte | Algorithm-specific        | Variable length     |
 * +--------+---------------------------+---------------------+
 * ```
 *
 * | Tag constant        | Value  | Algorithm     | Extra header        |
 * |---------------------|--------|---------------|---------------------|
 * | `kTagPassthrough`   | `0x00` | No compression | None               |
 * | `kTagLZ4`           | `0x01` | LZ4 HC        | 4-byte LE orig size |
 * | `kTagSnappy`        | `0x02` | Snappy        | None                |
 * | `kTagZstd`          | `0x03` | Zstd (lvl 3)  | 4-byte LE orig size |
 *
 * ### Allocation of future tag values
 *
 * Values `0x04`–`0x7F` are reserved for future standard codecs.
 * Values `0x80`–`0xFF` are reserved for experimental / vendor-specific use.
 * New tags MUST be registered here before use; do not add magic bytes
 * in individual compilation units.
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
