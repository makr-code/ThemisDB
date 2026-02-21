/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zstd_codec.h                                       ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

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

} // namespace utils
} // namespace themis
