/**
 * @file lz4_codec.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "utils/expected.h"

// Thin wrapper around the LZ4 compression library.
// Available only when compiled with THEMIS_HAS_LZ4. Without that flag all
// functions return empty vectors / error Results so callers can degrade
// gracefully.

namespace themis {
namespace utils {

namespace lz4_compression {
    /// Maximum input size accepted by lz4_compress_safe (prevents DoS).
    constexpr size_t MAX_INPUT_SIZE       = 512ULL * 1024 * 1024;   // 512 MB
    /// Maximum decompressed size accepted by lz4_decompress_safe.
    constexpr size_t MAX_DECOMPRESSED_SIZE = 2ULL * 1024 * 1024 * 1024; // 2 GB
    /// Default LZ4 acceleration factor (1 = balanced; higher = faster + lower ratio).
    constexpr int    DEFAULT_ACCELERATION = 1;
} // namespace lz4_compression

// ---------------------------------------------------------------------------
// Result<T>-based safe API (recommended for new code)
// ---------------------------------------------------------------------------

/**
 * @brief Compress @p size bytes at @p data with LZ4.
 *
 * @param data         Pointer to input bytes (must not be nullptr unless size==0).
 * @param size         Number of bytes to compress.
 * @param acceleration LZ4 acceleration factor (≥1; default 1).
 * @return Ok(compressed_bytes) on success; Err on failure or unsupported.
 */
Result<std::vector<uint8_t>> lz4_compress_safe(const uint8_t* data, size_t size,
                                                int acceleration = lz4_compression::DEFAULT_ACCELERATION);

/**
 * @brief Decompress an LZ4-compressed block.
 *
 * @param compressed      Compressed bytes produced by lz4_compress*.
 * @param original_size   Exact size of the original uncompressed data (required
 *                        by the LZ4 block format; must be stored alongside the
 *                        compressed payload by the caller).
 * @return Ok(decompressed_bytes) on success; Err on failure or unsupported.
 */
Result<std::vector<uint8_t>> lz4_decompress_safe(const std::vector<uint8_t>& compressed,
                                                  size_t original_size);

// ---------------------------------------------------------------------------
// Legacy API — backward compatible, returns empty vector on error
// ---------------------------------------------------------------------------

/// Compress @p size bytes. Returns empty vector on failure / unsupported.
std::vector<uint8_t> lz4_compress(const uint8_t* data, size_t size,
                                   int acceleration = lz4_compression::DEFAULT_ACCELERATION);

/// Convenience overload for std::string input.
inline std::vector<uint8_t> lz4_compress(const std::string& s,
                                          int acceleration = lz4_compression::DEFAULT_ACCELERATION) {
    return lz4_compress(reinterpret_cast<const uint8_t*>(s.data()), s.size(), acceleration);
}

/// Convenience overload for std::vector<uint8_t> input.
inline std::vector<uint8_t> lz4_compress(const std::vector<uint8_t>& input,
                                          int acceleration = lz4_compression::DEFAULT_ACCELERATION) {
    return lz4_compress(input.data(), input.size(), acceleration);
}

/// Decompress an LZ4-compressed block. Returns empty vector on failure / unsupported.
std::vector<uint8_t> lz4_decompress(const std::vector<uint8_t>& compressed, size_t original_size);

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

/**
 * @brief Return an upper bound on the compressed size for @p input_size bytes.
 *
 * Mirrors LZ4_compressBound(). Returns 0 when LZ4 is not available or when
 * @p input_size exceeds MAX_INPUT_SIZE.
 */
size_t lz4_compress_bound(size_t input_size);

} // namespace utils
} // namespace themis
