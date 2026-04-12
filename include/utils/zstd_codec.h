/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zstd_codec.h                                       ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:13:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     68                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <cstdint>
#include <string>
#include "utils/expected.h"

// Thin wrapper around Zstandard (ZSTD) compression library
// Functions are available only if compiled with THEMIS_HAS_ZSTD. If not,
// the functions will return empty vectors to signal unsupported operation.

namespace themis {
namespace utils {

// Maximum sizes to prevent denial of service attacks
namespace compression {
    constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;           // 1GB max input
    constexpr size_t MAX_OUTPUT_SIZE = 1024ULL * 1024 * 1024 * 2;      // 2GB max compressed output
    constexpr size_t MAX_DECOMPRESSED_SIZE = 1024ULL * 1024 * 1024 * 4; // 4GB max decompressed output
}

// Compress a buffer with ZSTD. Returns compressed bytes on success; empty on failure/unsupported.
// Now includes size validation to prevent buffer overflow and denial of service.
std::vector<uint8_t> zstd_compress(const uint8_t* data, size_t size, int level = 3);

// Compress a string with ZSTD.
inline std::vector<uint8_t> zstd_compress(const std::string& s, int level = 3) {
    return zstd_compress(reinterpret_cast<const uint8_t*>(s.data()), s.size(), level);
}

// Compress a vector with ZSTD.
inline std::vector<uint8_t> zstd_compress(const std::vector<uint8_t>& input, int level = 3) {
    return zstd_compress(input.data(), input.size(), level);
}

// Decompress a buffer that contains ZSTD frame. Empty on failure/unsupported.
// Now includes size validation to prevent buffer overflow and denial of service.
std::vector<uint8_t> zstd_decompress(const std::vector<uint8_t>& compressed);

// New Result<T>-based API for better error handling and security validation
// These provide detailed error information and are recommended for new code
Result<std::vector<uint8_t>> zstd_compress_safe(const uint8_t* data, size_t size, int level = 3);
Result<std::vector<uint8_t>> zstd_decompress_safe(const std::vector<uint8_t>& compressed);

// ---------------------------------------------------------------------------
// Streaming API (ZSTD_CStream / ZSTD_DStream)
// ---------------------------------------------------------------------------
//
// Both functions accept a `source` callback that returns successive input
// chunks as (pointer, size) pairs and a `sink` callback that receives the
// processed output chunks.  The source signals end-of-input by returning a
// pair with a null pointer or zero size.  The sink returns `false` to abort
// processing (the function will then return ERR_UTIL_COMPRESSION_FAILED).
//
// The streaming API never buffers the entire input or output in memory,
// making it suitable for large files or network streams.
//
// Example – compress a file in 64 KiB chunks:
//   std::ifstream in("input.bin", std::ios::binary);
//   std::ofstream out("output.zst", std::ios::binary);
//   zstd_compress_stream(
//       [&] {
//           static std::vector<uint8_t> buf(65536);
//           in.read(reinterpret_cast<char*>(buf.data()), buf.size());
//           return std::make_pair(buf.data(), static_cast<size_t>(in.gcount()));
//       },
//       [&](const uint8_t* p, size_t n) {
//           out.write(reinterpret_cast<const char*>(p), n);
//           return true;
//       });

/**
 * @brief Stream-compress arbitrary data using ZSTD_CStream.
 *
 * @param source   Callback that supplies the next input chunk; return
 *                 `{nullptr, 0}` or `{ptr, 0}` to signal end of input.
 * @param sink     Callback that receives each compressed output chunk.
 *                 Return `false` to abort.
 * @param level    ZSTD compression level (default 3).
 * @return Result<void> — ok on success, error otherwise.
 */
Result<void> zstd_compress_stream(
    std::function<std::pair<const uint8_t*, size_t>()> source,
    std::function<bool(const uint8_t*, size_t)>        sink,
    int                                                level = 3);

/**
 * @brief Stream-decompress ZSTD-compressed data using ZSTD_DStream.
 *
 * @param source          Callback that supplies the next compressed chunk.
 * @param sink            Callback that receives each decompressed chunk.
 * @param max_output_bytes Hard cap on total decompressed bytes (DoS guard).
 *                        Pass 0 to use `compression::MAX_DECOMPRESSED_SIZE`.
 * @return Result<void> — ok on success, error otherwise.
 */
Result<void> zstd_decompress_stream(
    std::function<std::pair<const uint8_t*, size_t>()> source,
    std::function<bool(const uint8_t*, size_t)>        sink,
    size_t                                             max_output_bytes = 0);

} // namespace utils
} // namespace themis
