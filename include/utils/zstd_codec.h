/**
 * @file zstd_codec.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
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
    constexpr size_t MAX_COMPRESSION_RATIO = 1024;  // Phase 2.4a: Max expansion ratio (output <= input * ratio)
    /// Recommended chunk size for streaming APIs (256 KB).
    constexpr size_t STREAM_CHUNK_SIZE = 256 * 1024;
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

/**
 * @brief Safely compress data using ZSTD with parameter validation.
 *
 * @param data Input buffer pointer (must not be nullptr).
 * @param size Size in bytes (must not exceed compression::MAX_INPUT_SIZE).
 * @param level Compression level (1-22); values outside range are auto-clamped to [1-22].
 * @return Result containing compressed data or error description.
 *
 * @note Parameter Validation: Compression level is silently clamped to valid range [1-22].
 *       Invalid levels are logged as WARN but do not cause compression to fail.
 * @note Max Input: Enforces maximum input size to prevent DoS via oversized allocations.
 * @note Thread-Safe: Uses thread-local ZSTD_CCtx for safe concurrent use (Phase 2.4a).
 * @note Output Bounds: Enforces MAX_OUTPUT_SIZE and MAX_COMPRESSION_RATIO to prevent buffer overflow.
 *
 * @error COMPRESSION_FAILED ZSTD compression failed (insufficient memory or corrupt state).
 * @error COMPRESSION_INPUT_INVALID Input size exceeds limit or exceeds compression ratio bounds.
 * @error CODEC_NOT_SUPPORTED ZSTD library not available (THEMIS_HAS_ZSTD not defined).
 */
Result<std::vector<uint8_t>> zstd_compress_safe(const uint8_t* data, size_t size, int level = 3);

/**
 * @brief Safely decompress ZSTD-compressed data with validation.
 *
 * @param compressed ZSTD-compressed bytes (output of zstd_compress_safe).
 * @return Result containing decompressed data or error description.
 *
 * @note Size Validation: Decompressed size is checked against MAX_DECOMPRESSED_SIZE to prevent
 *       decompression bomb attacks (e.g., 1MB compressed -> 4GB decompressed).
 * @note Checksum Verification: ZSTD frame checksum is validated if present in the frame.
 * @note Corrupt Data: Malformed or truncated compressed data returns an error, not silent failure.
 * @note Output Bounds: Enforces MAX_COMPRESSION_RATIO for decompression expansion bounds.
 *
 * @error DECOMPRESSION_FAILED ZSTD decompression failed (invalid or corrupt input).
 * @error COMPRESSION_BOMB_DETECTED Output size exceeds MAX_DECOMPRESSED_SIZE (DoS detected).
 * @error COMPRESSION_RATIO_EXCEEDED Expansion ratio exceeds MAX_COMPRESSION_RATIO.
 * @error CODEC_NOT_SUPPORTED ZSTD library not available (THEMIS_HAS_ZSTD not defined).
 *
 * @note Phase 2.4a Hardening: Corrupt data detection and decompression bomb prevention.
 */
Result<std::vector<uint8_t>> zstd_decompress_safe(const std::vector<uint8_t>& compressed);

// ---------------------------------------------------------------------------
// Streaming compression API
// ---------------------------------------------------------------------------

/**
 * @brief Streaming ZSTD compressor.
 *
 * Compress an arbitrarily large data stream chunk by chunk without loading
 * the entire input into memory at once.
 *
 * Typical usage:
 * @code
 *   ZstdStreamCompressor enc(3);
 *   while (has_more_data()) {
 *       auto chunk = enc.compress_chunk(buf, len);
 *       if (chunk) write(*chunk);
 *   }
 *   auto tail = enc.flush();
 *   if (tail) write(*tail);
 * @endcode
 */
class ZstdStreamCompressor {
public:
    /**
     * @brief Construct a stream compressor.
     * @param level Zstd compression level (1–22; default 3).
     */
    explicit ZstdStreamCompressor(int level = 3);
    ~ZstdStreamCompressor();

    // Non-copyable; movable.
    ZstdStreamCompressor(const ZstdStreamCompressor&)            = delete;
    ZstdStreamCompressor& operator=(const ZstdStreamCompressor&) = delete;
    ZstdStreamCompressor(ZstdStreamCompressor&&)                 noexcept = default;
    ZstdStreamCompressor& operator=(ZstdStreamCompressor&&)      noexcept = default;

    /**
     * @brief Feed a chunk of uncompressed data into the stream.
     *
     * @param data  Pointer to input bytes.
     * @param size  Number of bytes to compress.
     * @return Ok(compressed_output) — may be empty if ZSTD buffered data
     *         internally; Err on failure.
     */
    Result<std::vector<uint8_t>> compress_chunk(const uint8_t* data, size_t size);

    /// Convenience overload for std::vector input.
    Result<std::vector<uint8_t>> compress_chunk(const std::vector<uint8_t>& data) {
        return compress_chunk(data.data(), data.size());
    }

    /**
     * @brief Flush all buffered output and finalise the ZSTD frame.
     *
     * Must be called after the last compress_chunk() call. The returned
     * bytes complete the ZSTD frame and must be appended to the output
     * stream before the decompressor can verify integrity.
     *
     * @return Ok(final_compressed_bytes); Err on failure.
     */
    Result<std::vector<uint8_t>> flush();

    /**
     * @brief Reset the compressor for reuse with a new compression level.
     * @param level New compression level (≥1; 0 keeps the current level).
     */
    void reset(int level = 0);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Streaming ZSTD decompressor.
 *
 * Decompress a ZSTD-framed stream chunk by chunk.
 *
 * Typical usage:
 * @code
 *   ZstdStreamDecompressor dec;
 *   while (read_chunk(buf, len)) {
 *       auto out = dec.decompress_chunk(buf, len);
 *       if (out) process(*out);
 *       if (dec.is_done()) break;
 *   }
 * @endcode
 */
class ZstdStreamDecompressor {
public:
    ZstdStreamDecompressor();
    ~ZstdStreamDecompressor();

    // Non-copyable; movable.
    ZstdStreamDecompressor(const ZstdStreamDecompressor&)            = delete;
    ZstdStreamDecompressor& operator=(const ZstdStreamDecompressor&) = delete;
    ZstdStreamDecompressor(ZstdStreamDecompressor&&)                 noexcept = default;
    ZstdStreamDecompressor& operator=(ZstdStreamDecompressor&&)      noexcept = default;

    /**
     * @brief Feed a chunk of compressed data into the stream.
     *
     * @param data  Pointer to compressed bytes.
     * @param size  Number of compressed bytes.
     * @return Ok(decompressed_output) — may be empty if more input needed;
     *         Err on decompression failure.
     */
    Result<std::vector<uint8_t>> decompress_chunk(const uint8_t* data, size_t size);

    /// Convenience overload for std::vector input.
    Result<std::vector<uint8_t>> decompress_chunk(const std::vector<uint8_t>& data) {
        return decompress_chunk(data.data(), data.size());
    }

    /**
     * @brief Returns true when the current ZSTD frame has been fully decoded.
     *
     * After is_done() returns true, reset() must be called before feeding
     * bytes from a new frame.
     */
    bool is_done() const;

    /**
     * @brief Reset the decompressor to accept a new ZSTD frame.
     */
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace utils
} // namespace themis
