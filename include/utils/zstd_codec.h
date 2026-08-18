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
 * @note Max Input: Enforces maximum input size (1GB) to prevent DoS via oversized allocations.
 *       Inputs larger than MAX_INPUT_SIZE will return error code ERR_UTIL_INVALID_ARGUMENT.
 * @note Max Output: Bounded output buffer allocation to prevent uncontrolled memory growth
 *       (max 2GB). Compression that would exceed this limit returns ERR_UTIL_INVALID_ARGUMENT.
 * @note Thread-Safe: Uses thread-local ZSTD_CCtx for safe concurrent use.
 *       Multiple threads can call compress_safe concurrently without synchronization.
 * @note Concurrent Encode/Decode: ZSTD contexts are NOT reused between threads.
 *       Each zstd_compress_safe() and zstd_decompress_safe() call creates its own context
 *       to guarantee thread-safety. Reusing a ZSTD_CCtx across threads is unsafe.
 *
 * @error E_INVALID_INPUT Input data is empty or exceeds size limits.
 * @error E_COMPRESSION_FAILED ZSTD compression failed (insufficient memory or corrupt state).
 * 
 * @error_contract
 * | Code | Condition | Recovery |
 * |------|-----------|----------|
 * | ERR_UTIL_INVALID_ARGUMENT | input size > 1GB | Return error; do not attempt compression |
 * | ERR_UTIL_ALLOCATION_FAILED | cannot allocate output buffer | Return error; check system memory |
 * | COMPRESSION_FAILED | ZSTD library error | Return error; check ZSTD_getErrorName() for details |
 */
Result<std::vector<uint8_t>> zstd_compress_safe(const uint8_t* data, size_t size, int level = 3);

/**
 * @brief Safely decompress ZSTD-compressed data with decompression bomb detection.
 *
 * @param compressed Compressed bytes produced by zstd_compress*.
 * @return Result containing decompressed data or error description.
 *
 * @note Decompression Bomb Protection: Validates decompressed size against MAX_DECOMPRESSED_SIZE (4GB).
 *       Blocks decompression of frames claiming to expand to >4GB.
 *       Extreme compression ratios (>100:1) are logged as warnings.
 * @note Corrupted Frame Detection: Validates frame header and checksum.
 *       Returns COMPRESSION_INPUT_INVALID if frame is truncated or corrupted.
 * @note Unknown Size Handling: When decompressed size is not stored in frame,
 *       estimates based on 4:1 ratio (capped at MAX_DECOMPRESSED_SIZE/2 to prevent bomb attacks).
 * @note Thread-Safe: Each call creates its own decompression context.
 *       Safe for concurrent calls from multiple threads without synchronization.
 * @note Buffer Growth: Output buffer is allocated once based on decompressed size hint,
 *       then trimmed to actual decompressed bytes. No repeated reallocation.
 *
 * @error E_COMPRESSION_FAILED Invalid compressed data or decompression error.
 * @error COMPRESSION_BOMB_DETECTED Decompressed size exceeds safety limit (potential attack).
 * @error COMPRESSION_INPUT_INVALID Frame header corrupted or truncated.
 * 
 * @error_contract
 * | Code | Condition | Recovery |
 * |------|-----------|----------|
 * | COMPRESSION_INPUT_INVALID | corrupted/truncated frame header | Return error; data unrecoverable |
 * | COMPRESSION_BOMB_DETECTED | decompressed size > 4GB | Return error; reject suspicious input |
 * | ERR_UTIL_ALLOCATION_FAILED | cannot allocate decompression buffer | Return error; check memory |
 * | DECOMPRESSION_FAILED | ZSTD library error during decompression | Return error; data may be corrupted |
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
