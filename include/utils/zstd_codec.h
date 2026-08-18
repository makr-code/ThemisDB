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
 * @param size Size in bytes (must not exceed MAX_ZSTD_INPUT_SIZE).
 * @param level Compression level (1-22); values outside range are auto-clamped to [1-22].
 * @return Result containing compressed data or error description.
 *
 * @note Parameter Validation: Compression level is silently clamped to valid range [1-22].
 *       Invalid levels are logged as WARN but do not cause compression to fail.
 * @note Max Input: Enforces maximum input size to prevent DoS via oversized allocations.
 * @note Thread-Safe: Uses thread-local ZSTD_CCtx for safe concurrent use.
 *
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Input nullptr with size > 0 | ERR_UTIL_INVALID_ARGUMENT (COMPRESSION_INPUT_INVALID 9063) | Error | zstd_compress_safe | Return Err |
 * | Input size > MAX_INPUT_SIZE (1 GB) | ERR_UTIL_INVALID_ARGUMENT (COMPRESSION_INPUT_INVALID 9063) | Error | size, limit | Return Err |
 * | Output bound > MAX_OUTPUT_SIZE (2 GB) | ERR_UTIL_INVALID_ARGUMENT (COMPRESSION_INPUT_INVALID 9063) | Error | bound, limit | Return Err |
 * | Memory allocation failure | ERR_UTIL_ALLOCATION_FAILED | Error | requested bytes | Return Err |
 * | ZSTD compress error | ERR_UTIL_COMPRESSION_FAILED (COMPRESSION_FAILED 9060) | Error | ZSTD error name | Return Err; logErrorWithContext |
 * | ZSTD unavailable (no THEMIS_HAS_ZSTD) | ERR_UTIL_COMPRESSION_FAILED (CODEC_NOT_SUPPORTED 9067) | Error | – | Return Err |
 *
 * @degradation Never silently returns uncompressed data; any failure path returns Err.
 * @bounded_resources
 * - Input capped at compression::MAX_INPUT_SIZE (1 GB)
 * - Output buffer capped at compression::MAX_OUTPUT_SIZE (2 GB)
 * - Compression level auto-clamped to [1, 22]; invalid values logged as WARN
 */
Result<std::vector<uint8_t>> zstd_compress_safe(const uint8_t* data, size_t size, int level = 3);

/**
 * @brief Safely decompress a ZSTD-framed buffer with bomb protection.
 *
 * @param compressed ZSTD-framed bytes to decompress.
 * @return Ok(decompressed_bytes) on success; Err on any failure.
 *
 * @note Decompression bomb protection: Rejects any frame whose declared
 *       content size exceeds MAX_DECOMPRESSED_SIZE (4 GB).
 * @note Thread-Safe: Uses a per-call ZSTD_DCtx; safe for concurrent use.
 *
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Compressed > MAX_DECOMPRESSED_SIZE (4 GB) | ERR_UTIL_INVALID_ARGUMENT (COMPRESSION_BOMB_DETECTED 9064) | Error | declared size, limit | Return Err |
 * | Memory allocation failure | ERR_UTIL_ALLOCATION_FAILED | Error | requested bytes | Return Err |
 * | ZSTD_DCtx creation failure | ERR_UTIL_COMPRESSION_FAILED (CODEC_INITIALIZATION_FAILED 9066) | Error | – | Return Err |
 * | ZSTD decompress error | ERR_UTIL_COMPRESSION_FAILED (DECOMPRESSION_FAILED 9061) | Error | ZSTD error name | Return Err; logErrorWithContext |
 * | ZSTD unavailable (no THEMIS_HAS_ZSTD) | ERR_UTIL_COMPRESSION_FAILED (CODEC_NOT_SUPPORTED 9067) | Error | – | Return Err |
 *
 * @degradation Never returns partial data; any decompression failure returns Err.
 * @bounded_resources
 * - Declared decompressed size capped at compression::MAX_DECOMPRESSED_SIZE (4 GB)
 * - Uses ZSTD_DCtx per call; context freed on any exit path (RAII)
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
