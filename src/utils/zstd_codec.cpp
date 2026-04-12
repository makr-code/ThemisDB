/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zstd_codec.cpp                                     ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:22:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     251                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include <fmt/format.h>

#include <algorithm>
#include <stdexcept>

#ifdef THEMIS_HAS_ZSTD
#include <zstd.h>
#endif

namespace themis {
namespace utils {

// Internal implementation with full Result<T> error handling
Result<std::vector<uint8_t>> zstd_compress_safe(const uint8_t* data, size_t size, int level) {
#ifdef THEMIS_HAS_ZSTD
    // Step 1: Handle empty input
    if (!data || size == 0) {
        return Ok(std::vector<uint8_t>());
    }
    
    // Step 2: Validate input size to prevent DoS
    if (size > compression::MAX_INPUT_SIZE) {
        THEMIS_ERROR("Input too large for compression: {} bytes (max: {})",
                    size, compression::MAX_INPUT_SIZE);
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("Input size {} exceeds maximum {}", size, compression::MAX_INPUT_SIZE)
        );
    }
    
    // Step 3: Calculate max output size with validation
    size_t max_size = ZSTD_compressBound(size);
    
    // Double-check: ZSTD_compressBound should never exceed reasonable limit
    if (max_size > compression::MAX_OUTPUT_SIZE) {
        THEMIS_ERROR("Compression bound too large: {} bytes (max: {})",
                    max_size, compression::MAX_OUTPUT_SIZE);
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("Compression would require {} bytes, max {}",
                       max_size, compression::MAX_OUTPUT_SIZE)
        );
    }
    
    // Step 4: Try to allocate with exception safety
    std::vector<uint8_t> output;
    try {
        output.reserve(max_size);
        output.resize(max_size);
    } catch (const std::bad_alloc& e) {
        THEMIS_ERROR("Failed to allocate memory for compression: {}", e.what());
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            fmt::format("Cannot allocate {} bytes for compression", max_size)
        );
    } catch (const std::exception& e) {
        THEMIS_ERROR("Unexpected error during memory allocation: {}", e.what());
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            std::string(e.what())
        );
    }
    
    // Step 5: Perform compression
    size_t compressed_size = ZSTD_compress(
        output.data(),
        output.size(),
        data,
        size,
        level
    );
    
    if (ZSTD_isError(compressed_size)) {
        THEMIS_ERROR("Compression failed: {}", ZSTD_getErrorName(compressed_size));
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            ZSTD_getErrorName(compressed_size)
        );
    }
    
    // Step 6: Trim to actual compressed size
    output.resize(compressed_size);
    
    THEMIS_DEBUG("Compressed {} bytes to {} bytes (ratio: {:.2f}%)",
                size, compressed_size,
                (100.0 * compressed_size) / size);
    
    return Ok(std::move(output));
#else
    (void)data; (void)size; (void)level;
    return Err<std::vector<uint8_t>>(
        errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
        "ZSTD support not compiled in (THEMIS_HAS_ZSTD not defined)"
    );
#endif
}

Result<std::vector<uint8_t>> zstd_decompress_safe(const std::vector<uint8_t>& compressed) {
#ifdef THEMIS_HAS_ZSTD
    // Step 1: Handle empty input
    if (compressed.empty()) {
        return Ok(std::vector<uint8_t>());
    }
    
    // Step 2: Validate compressed input size
    if (compressed.size() > compression::MAX_DECOMPRESSED_SIZE) {
        THEMIS_ERROR("Compressed data too large: {} bytes", compressed.size());
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Compressed data too large"
        );
    }
    
    // Step 3: Get decompressed size hint
    unsigned long long decompressed_size = ZSTD_getFrameContentSize(
        compressed.data(), 
        compressed.size()
    );
    
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        THEMIS_ERROR("Failed to get decompressed size: not valid ZSTD frame");
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            "Not a valid ZSTD compressed frame"
        );
    }
    
    if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        // Decompressed size unknown - use reasonable default based on compression ratio
        decompressed_size = compressed.size() * 4;  // Assume 4:1 ratio
        THEMIS_DEBUG("Decompressed size unknown, estimating {} bytes", decompressed_size);
    }
    
    // Step 4: Validate decompressed size
    if (decompressed_size > compression::MAX_DECOMPRESSED_SIZE) {
        THEMIS_ERROR("Decompressed size too large: {} bytes (max: {})", 
                    decompressed_size, compression::MAX_DECOMPRESSED_SIZE);
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("Decompressed size {} exceeds maximum {}",
                       decompressed_size, compression::MAX_DECOMPRESSED_SIZE)
        );
    }
    
    // Step 5: Allocate output buffer with exception safety
    std::vector<uint8_t> output;
    try {
        output.resize(static_cast<size_t>(decompressed_size));
    } catch (const std::bad_alloc& e) {
        THEMIS_ERROR("Failed to allocate memory for decompression: {}", e.what());
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            fmt::format("Cannot allocate {} bytes for decompression", decompressed_size)
        );
    } catch (const std::exception& e) {
        THEMIS_ERROR("Unexpected error during memory allocation: {}", e.what());
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            std::string(e.what())
        );
    }
    
    // Step 6: Perform decompression
    // Use decompression context for better error handling
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    if (!dctx) {
        THEMIS_ERROR("Failed to create ZSTD decompression context");
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            "Failed to create ZSTD decompression context"
        );
    }
    
    size_t result = ZSTD_decompressDCtx(
        dctx,
        output.data(),
        output.size(),
        compressed.data(),
        compressed.size()
    );
    
    ZSTD_freeDCtx(dctx);
    
    if (ZSTD_isError(result)) {
        THEMIS_ERROR("Decompression failed: {}", ZSTD_getErrorName(result));
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            ZSTD_getErrorName(result)
        );
    }
    
    // Step 7: Trim to actual decompressed size
    output.resize(result);
    
    THEMIS_DEBUG("Decompressed {} bytes to {} bytes", compressed.size(), result);
    
    return Ok(std::move(output));
#else
    (void)compressed;
    return Err<std::vector<uint8_t>>(
        errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
        "ZSTD support not compiled in (THEMIS_HAS_ZSTD not defined)"
    );
#endif
}

// Public API - backward compatible, but now with security validation
std::vector<uint8_t> zstd_compress(const uint8_t* data, size_t size, int level) {
    auto result = zstd_compress_safe(data, size, level);
    if (result) {
        return *result;
    }
    // On error, return empty vector (backward compatible behavior)
    return {};
}

std::vector<uint8_t> zstd_decompress(const std::vector<uint8_t>& compressed) {
    auto result = zstd_decompress_safe(compressed);
    if (result) {
        return *result;
    }
    // On error, return empty vector (backward compatible behavior)
    return {};
}

// ---------------------------------------------------------------------------
// Streaming compress / decompress (ZSTD_CStream / ZSTD_DStream)
// ---------------------------------------------------------------------------

Result<void> zstd_compress_stream(
        std::function<std::pair<const uint8_t*, size_t>()> source,
        std::function<bool(const uint8_t*, size_t)>        sink,
        int                                                 level) {
#ifdef THEMIS_HAS_ZSTD
    ZSTD_CStream* cstream = ZSTD_createCStream();
    if (!cstream) {
        return Err<void>(errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
                         "ZSTD_createCStream failed");
    }
    struct CStreamGuard {
        ZSTD_CStream* p;
        ~CStreamGuard() { ZSTD_freeCStream(p); }
    } guard{cstream};

    size_t init_result = ZSTD_initCStream(cstream, level);
    if (ZSTD_isError(init_result)) {
        return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                         fmt::format("ZSTD_initCStream: {}", ZSTD_getErrorName(init_result)));
    }

    const size_t in_buf_size  = ZSTD_CStreamInSize();
    const size_t out_buf_size = ZSTD_CStreamOutSize();
    std::vector<uint8_t> out_buf(out_buf_size);

    for (;;) {
        auto [chunk_data, chunk_size] = source();
        const bool is_last = (chunk_data == nullptr || chunk_size == 0);

        if (!is_last) {
            if (chunk_size > in_buf_size) {
                return Err<void>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                 fmt::format("Chunk size {} exceeds ZSTD input buffer {}", chunk_size, in_buf_size));
            }
            ZSTD_inBuffer in{chunk_data, chunk_size, 0};
            while (in.pos < in.size) {
                ZSTD_outBuffer out{out_buf.data(), out_buf_size, 0};
                size_t ret = ZSTD_compressStream(cstream, &out, &in);
                if (ZSTD_isError(ret)) {
                    return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                     fmt::format("ZSTD_compressStream: {}", ZSTD_getErrorName(ret)));
                }
                if (out.pos > 0 && !sink(out_buf.data(), out.pos)) {
                    return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                     "Sink callback rejected compressed data");
                }
            }
        } else {
            // Flush and end frame
            ZSTD_outBuffer out{out_buf.data(), out_buf_size, 0};
            size_t ret = ZSTD_endStream(cstream, &out);
            if (ZSTD_isError(ret)) {
                return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                 fmt::format("ZSTD_endStream: {}", ZSTD_getErrorName(ret)));
            }
            if (out.pos > 0 && !sink(out_buf.data(), out.pos)) {
                return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                 "Sink callback rejected final compressed frame");
            }
            break;
        }
    }
    return Ok();
#else
    (void)source; (void)sink; (void)level;
    return Err<void>(errors::ErrorCode::ERR_UTIL_NOT_IMPLEMENTED,
                     "ZSTD streaming not available (built without THEMIS_HAS_ZSTD)");
#endif
}

Result<void> zstd_decompress_stream(
        std::function<std::pair<const uint8_t*, size_t>()> source,
        std::function<bool(const uint8_t*, size_t)>        sink,
        size_t                                             max_output_bytes) {
#ifdef THEMIS_HAS_ZSTD
    if (max_output_bytes == 0) {
        max_output_bytes = compression::MAX_DECOMPRESSED_SIZE;
    }

    ZSTD_DStream* dstream = ZSTD_createDStream();
    if (!dstream) {
        return Err<void>(errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
                         "ZSTD_createDStream failed");
    }
    struct DStreamGuard {
        ZSTD_DStream* p;
        ~DStreamGuard() { ZSTD_freeDStream(p); }
    } guard{dstream};

    size_t init_result = ZSTD_initDStream(dstream);
    if (ZSTD_isError(init_result)) {
        return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                         fmt::format("ZSTD_initDStream: {}", ZSTD_getErrorName(init_result)));
    }

    const size_t out_buf_size = ZSTD_DStreamOutSize();
    std::vector<uint8_t> out_buf(out_buf_size);
    size_t total_output = 0;

    for (;;) {
        auto [chunk_data, chunk_size] = source();
        if (chunk_data == nullptr || chunk_size == 0) {
            break;  // No more input
        }

        ZSTD_inBuffer in{chunk_data, chunk_size, 0};
        while (in.pos < in.size) {
            ZSTD_outBuffer out{out_buf.data(), out_buf_size, 0};
            size_t ret = ZSTD_decompressStream(dstream, &out, &in);
            if (ZSTD_isError(ret)) {
                return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                 fmt::format("ZSTD_decompressStream: {}", ZSTD_getErrorName(ret)));
            }
            if (out.pos > 0) {
                total_output += out.pos;
                if (total_output > max_output_bytes) {
                    THEMIS_ERROR("Streaming decompression exceeded max output size {} bytes", max_output_bytes);
                    return Err<void>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                     fmt::format("Decompressed output exceeded limit of {} bytes", max_output_bytes));
                }
                if (!sink(out_buf.data(), out.pos)) {
                    return Err<void>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                     "Sink callback rejected decompressed data");
                }
            }
            if (ret == 0) {
                break;  // Frame complete
            }
        }
    }
    return Ok();
#else
    (void)source; (void)sink; (void)max_output_bytes;
    return Err<void>(errors::ErrorCode::ERR_UTIL_NOT_IMPLEMENTED,
                     "ZSTD streaming not available (built without THEMIS_HAS_ZSTD)");
#endif
}

} // namespace utils
} // namespace themis
