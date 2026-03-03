/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            connection_compression.h                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:53:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     136                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3655c7e6  2026-02-25  feat(network): implement LZ4 and Zstd connection-level co... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Connection-level payload compression for the Wire Protocol V2.
//
// Wire format for all compressed payloads:
//   [original_size: uint32_t, little-endian] [compressed_data...]
//
// The 4-byte prefix allows decoders to pre-allocate the output buffer
// without relying on codec-internal metadata.

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include <lz4.h>
#include <zstd.h>

#include "themis/network/wire_protocol_v2.hpp"

namespace themis {
namespace network {

/// @brief Compress @p data with LZ4.
///
/// Returns the compressed payload (with 4-byte original-size prefix) when
/// compression reduces the size and the input meets the minimum threshold.
/// Returns an empty vector when compression is skipped or fails.
///
/// @param data     Input bytes to compress.
/// @param min_size Skip compression when input is smaller than this.
inline std::vector<uint8_t> compressLZ4(
    const std::vector<uint8_t>& data, uint32_t min_size = 256)
{
    if (data.size() < min_size) return {};
    const int src_size = static_cast<int>(data.size());
    const int bound    = LZ4_compressBound(src_size);
    if (bound <= 0) return {};

    std::vector<uint8_t> out(4 + static_cast<size_t>(bound));
    const uint32_t orig_le = static_cast<uint32_t>(src_size);
    std::memcpy(out.data(), &orig_le, 4);

    const int compressed = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(out.data() + 4),
        src_size, bound);
    if (compressed <= 0) return {};

    out.resize(4 + static_cast<size_t>(compressed));
    return out;
}

/// @brief Decompress an LZ4-compressed payload (with 4-byte original-size prefix).
///
/// Returns the decompressed bytes, or an empty vector on error.
inline std::vector<uint8_t> decompressLZ4(const std::vector<uint8_t>& data)
{
    if (data.size() < 4) return {};
    uint32_t orig_le = 0;
    std::memcpy(&orig_le, data.data(), 4);
    const int orig_size = static_cast<int>(orig_le);
    if (orig_size <= 0 ||
        static_cast<size_t>(orig_size) > wire::V2_MAX_PAYLOAD) return {};

    std::vector<uint8_t> out(static_cast<size_t>(orig_size));
    const int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data.data() + 4),
        reinterpret_cast<char*>(out.data()),
        static_cast<int>(data.size() - 4),
        orig_size);
    if (result != orig_size) return {};
    return out;
}

/// @brief Compress @p data with Zstd.
///
/// Returns the compressed payload (with 4-byte original-size prefix) when
/// compression reduces the size and the input meets the minimum threshold.
/// Returns an empty vector when compression is skipped or fails.
///
/// @param data      Input bytes to compress.
/// @param min_size  Skip compression when input is smaller than this.
/// @param level     Zstd compression level (1–22, default 3).
inline std::vector<uint8_t> compressZstd(
    const std::vector<uint8_t>& data,
    uint32_t min_size = 256,
    int      level    = ZSTD_CLEVEL_DEFAULT)
{
    if (data.size() < min_size) return {};
    const size_t bound = ZSTD_compressBound(data.size());
    if (ZSTD_isError(bound)) return {};

    std::vector<uint8_t> out(4 + bound);
    const uint32_t orig_le = static_cast<uint32_t>(data.size());
    std::memcpy(out.data(), &orig_le, 4);

    const size_t compressed = ZSTD_compress(
        out.data() + 4, bound,
        data.data(), data.size(),
        level);
    if (ZSTD_isError(compressed)) return {};

    out.resize(4 + compressed);
    return out;
}

/// @brief Decompress a Zstd-compressed payload (with 4-byte original-size prefix).
///
/// Returns the decompressed bytes, or an empty vector on error.
inline std::vector<uint8_t> decompressZstd(const std::vector<uint8_t>& data)
{
    if (data.size() < 4) return {};
    uint32_t orig_le = 0;
    std::memcpy(&orig_le, data.data(), 4);
    const size_t orig_size = static_cast<size_t>(orig_le);
    if (orig_size == 0 || orig_size > wire::V2_MAX_PAYLOAD) return {};

    std::vector<uint8_t> out(orig_size);
    const size_t result = ZSTD_decompress(
        out.data(), orig_size,
        data.data() + 4, data.size() - 4);
    if (ZSTD_isError(result) || result != orig_size) return {};
    return out;
}

} // namespace network
} // namespace themis
