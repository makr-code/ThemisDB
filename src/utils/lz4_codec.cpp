/**
 * @file lz4_codec.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/lz4_codec.h"
#include "utils/logger.h"
#include "utils/error_contracts.h"
#include <fmt/format.h>

#ifdef THEMIS_HAS_LZ4
#include <lz4.h>
#endif

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Result<T>-based safe API
// ---------------------------------------------------------------------------

Result<std::vector<uint8_t>> lz4_compress_safe(const uint8_t* data, size_t size, int acceleration) {
#ifdef THEMIS_HAS_LZ4
    if (!data || size == 0) {
        return Ok(std::vector<uint8_t>());
    }
    
    // Phase A.3 Hardening - Validate and clamp acceleration parameter
    // LZ4_compress_fast accepts acceleration >= 1. Typical range is 1-1000.
    // Values outside this range are clamped to DEFAULT_ACCELERATION
    if (acceleration < 1 || acceleration > 1000) {
        THEMIS_WARN("LZ4 acceleration {} is out of valid range [1, 1000]; using default {}", 
                    acceleration, lz4_compression::DEFAULT_ACCELERATION);
        acceleration = lz4_compression::DEFAULT_ACCELERATION;
    }

    if (size > lz4_compression::MAX_INPUT_SIZE) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("LZ4 input size {} exceeds maximum {}", size, lz4_compression::MAX_INPUT_SIZE));
    }

    if (size > static_cast<size_t>(LZ4_MAX_INPUT_SIZE)) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("LZ4 input size {} exceeds LZ4_MAX_INPUT_SIZE", size));
    }

    const int src_size = static_cast<int>(size);
    const int bound    = LZ4_compressBound(src_size);
    if (bound <= 0) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            "LZ4_compressBound returned 0 — input too large");
    }

    std::vector<uint8_t> output;
    try {
        output.resize(static_cast<size_t>(bound));
    } catch (const std::bad_alloc&) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            fmt::format("Cannot allocate {} bytes for LZ4 output", bound));
    }

    const int compressed_size = LZ4_compress_fast(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(output.data()),
        src_size,
        bound,
        acceleration);

    if (compressed_size <= 0) {
        const auto err_msg = std::string("LZ4_compress_fast failed");
        logErrorWithContext(makeErrorContext(
            ErrorCode::COMPRESSION_FAILED, err_msg,
            "lz4_compress_safe", ErrorSeverity::Error, /*is_recoverable=*/true));
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            "LZ4_compress_fast failed");
    }

    output.resize(static_cast<size_t>(compressed_size));
    THEMIS_DEBUG("LZ4 compressed {} → {} bytes", size, compressed_size);
    return Ok(std::move(output));
#else
    (void)data;
    (void)size;
    (void)acceleration;
    return Err<std::vector<uint8_t>>(
        errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
        "LZ4 support not compiled in (THEMIS_HAS_LZ4 not defined)");
#endif
}

Result<std::vector<uint8_t>> lz4_decompress_safe(const std::vector<uint8_t>& compressed,
                                                  size_t original_size) {
#ifdef THEMIS_HAS_LZ4
    if (compressed.empty() || original_size == 0) {
        return Ok(std::vector<uint8_t>());
    }

    if (original_size > lz4_compression::MAX_DECOMPRESSED_SIZE) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("LZ4 decompressed size {} exceeds maximum {}",
                        original_size, lz4_compression::MAX_DECOMPRESSED_SIZE));
    }
    
    // Phase 2.4b Hardening: Check compression ratio to detect decompression bombs
    if (static_cast<int>(compressed.size()) > 0) {
        size_t ratio = original_size / compressed.size();
        if (ratio > lz4_compression::MAX_COMPRESSION_RATIO) {
            const auto err_msg = fmt::format(
                "LZ4 decompression bomb detected: ratio {}x exceeds max {}x",
                ratio, lz4_compression::MAX_COMPRESSION_RATIO);
            THEMIS_ERROR("{}", err_msg);
            logErrorWithContext(makeErrorContext(
                ErrorCode::COMPRESSION_BOMB_DETECTED, err_msg,
                "lz4_decompress_safe", ErrorSeverity::Error, /*is_recoverable=*/false));
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                err_msg);
        }
    }

    std::vector<uint8_t> output;
    try {
        output.resize(original_size);
    } catch (const std::bad_alloc&) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
            fmt::format("Cannot allocate {} bytes for LZ4 decompressed output", original_size));
    }

    const int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed.data()),
        reinterpret_cast<char*>(output.data()),
        static_cast<int>(compressed.size()),
        static_cast<int>(original_size));

    if (result < 0) {
        const auto err_msg = fmt::format("LZ4_decompress_safe failed with code {}", result);
        logErrorWithContext(makeErrorContext(
            ErrorCode::DECOMPRESSION_FAILED, err_msg,
            "lz4_decompress_safe", ErrorSeverity::Error, /*is_recoverable=*/true));
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            err_msg);
    }

    output.resize(static_cast<size_t>(result));
    THEMIS_DEBUG("LZ4 decompressed {} → {} bytes",static_cast<int>(compressed.size()), result);
    return Ok(std::move(output));
#else
    (void)compressed;
    (void)original_size;
    return Err<std::vector<uint8_t>>(
        errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
        "LZ4 support not compiled in (THEMIS_HAS_LZ4 not defined)");
#endif
}

// ---------------------------------------------------------------------------
// Legacy API
// ---------------------------------------------------------------------------

std::vector<uint8_t> lz4_compress(const uint8_t* data, size_t size, int acceleration) {
    auto result = lz4_compress_safe(data, size, acceleration);
    return result ? std::move(*result) : std::vector<uint8_t>{};
}

std::vector<uint8_t> lz4_decompress(const std::vector<uint8_t>& compressed, size_t original_size) {
    auto result = lz4_decompress_safe(compressed, original_size);
    return result ? std::move(*result) : std::vector<uint8_t>{};
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

size_t lz4_compress_bound([[maybe_unused]] size_t input_size) {
#ifdef THEMIS_HAS_LZ4
    if (input_size == 0 || input_size > lz4_compression::MAX_INPUT_SIZE) {
      return 0;
    }
    if (input_size > static_cast<size_t>(LZ4_MAX_INPUT_SIZE)) {
      return 0;
    }
    const int bound = LZ4_compressBound(static_cast<int>(input_size));
    return bound > 0 ? static_cast<size_t>(bound) : 0;
#else
    (void)input_size;
    return 0;
#endif
}

} // namespace utils
} // namespace themis

