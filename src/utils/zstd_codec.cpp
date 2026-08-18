/**
 * @file zstd_codec.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include "utils/error_contracts.h"
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
    
    // Step 1.5: Phase A.3 Hardening - Validate compression level
    // ZSTD valid levels: 1-22 (default 3). Clamp out-of-range values.
    if (level < 1 || level > 22) {
        THEMIS_WARN("Compression level {} is out of valid range [1, 22]; clamping to default 3", level);
        level = 3;  // Default ZSTD compression level
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
        const auto err_msg = fmt::format("ZSTD compress failed: {}", ZSTD_getErrorName(compressed_size));
        THEMIS_ERROR("{}", err_msg);
        logErrorWithContext(makeErrorContext(
            ErrorCode::COMPRESSION_FAILED, err_msg,
            "zstd_compress_safe", ErrorSeverity::Error, /*is_recoverable=*/true));
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
    (void)level;
    if (!data || size == 0) {
        return Ok(std::vector<uint8_t>());
    }
    if (size > compression::MAX_INPUT_SIZE) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            fmt::format("Input size {} exceeds maximum {}", size, compression::MAX_INPUT_SIZE)
        );
    }

    // Lightweight fallback when ZSTD is unavailable:
    // byte-oriented RLE framing: ['T','R','L','E',mode,payload...]
    // mode=1: RLE pairs [count,value], mode=0: raw payload passthrough.
    std::vector<uint8_t> rle;
    rle.reserve(size + 5);
    rle.push_back(static_cast<uint8_t>('T'));
    rle.push_back(static_cast<uint8_t>('R'));
    rle.push_back(static_cast<uint8_t>('L'));
    rle.push_back(static_cast<uint8_t>('E'));
    rle.push_back(1u);

    for (size_t i = 0; i < size;) {
        const uint8_t value = data[i];
        size_t run = 1;
        while (i + run < size && data[i + run] == value && run < 255) {
            ++run;
        }
        rle.push_back(static_cast<uint8_t>(run));
        rle.push_back(value);
        i += run;
    }

    if (rle.size() >= size + 5) {
        std::vector<uint8_t> raw;
        raw.reserve(size + 5);
        raw.push_back(static_cast<uint8_t>('T'));
        raw.push_back(static_cast<uint8_t>('R'));
        raw.push_back(static_cast<uint8_t>('L'));
        raw.push_back(static_cast<uint8_t>('E'));
        raw.push_back(0u);
        raw.insert(raw.end(), data, data + size);
        return Ok(std::move(raw));
    }

    return Ok(std::move(rle));
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
        const auto err_msg = std::string("Not a valid ZSTD compressed frame");
        THEMIS_ERROR("Failed to get decompressed size: {}", err_msg);
        logErrorWithContext(makeErrorContext(
            ErrorCode::COMPRESSION_INPUT_INVALID, err_msg,
            "zstd_decompress_safe", ErrorSeverity::Error, /*is_recoverable=*/false));
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
    
    // Phase 2.4a Hardening: Check compression ratio to detect decompression bombs
    if (compressed.size() > 0) {
        size_t ratio = decompressed_size / compressed.size();
        if (ratio > compression::MAX_COMPRESSION_RATIO) {
            const auto err_msg = fmt::format(
                "Decompression bomb detected: ratio {}x exceeds max {}x",
                ratio, compression::MAX_COMPRESSION_RATIO);
            THEMIS_ERROR("{}", err_msg);
            logErrorWithContext(makeErrorContext(
                ErrorCode::COMPRESSION_BOMB_DETECTED, err_msg,
                "zstd_decompress_safe", ErrorSeverity::Error, /*is_recoverable=*/false));
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                err_msg
            );
        }
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
        const auto err_msg = fmt::format("ZSTD decompress failed: {}", ZSTD_getErrorName(result));
        THEMIS_ERROR("{}", err_msg);
        logErrorWithContext(makeErrorContext(
            ErrorCode::DECOMPRESSION_FAILED, err_msg,
            "zstd_decompress_safe", ErrorSeverity::Error, /*is_recoverable=*/true));
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
    if (compressed.empty()) {
        return Ok(std::vector<uint8_t>());
    }
    if (compressed.size() < 5
        || compressed[0] != static_cast<uint8_t>('T')
        || compressed[1] != static_cast<uint8_t>('R')
        || compressed[2] != static_cast<uint8_t>('L')
        || compressed[3] != static_cast<uint8_t>('E')) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            "Fallback RLE frame header missing"
        );
    }

    const uint8_t mode = compressed[4];
    if (mode == 0u) {
        return Ok(std::vector<uint8_t>(compressed.begin() + 5, compressed.end()));
    }
    if (mode != 1u) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            "Fallback RLE frame mode invalid"
        );
    }
    if (((compressed.size() - 5) % 2) != 0) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
            "Fallback RLE payload malformed"
        );
    }

    std::vector<uint8_t> output;
    for (size_t i = 5; i + 1 < compressed.size(); i += 2) {
        const uint8_t count = compressed[i];
        const uint8_t value = compressed[i + 1];
        if (count == 0) {
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                "Fallback RLE count must be > 0"
            );
        }
        output.insert(output.end(), static_cast<size_t>(count), value);
        if (output.size() > compression::MAX_DECOMPRESSED_SIZE) {
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                fmt::format("Decompressed size {} exceeds maximum {}",
                            output.size(), compression::MAX_DECOMPRESSED_SIZE)
            );
        }
    }

    return Ok(std::move(output));
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
// Streaming compressor
// ---------------------------------------------------------------------------

struct ZstdStreamCompressor::Impl {
#ifdef THEMIS_HAS_ZSTD
    ZSTD_CStream* cstream = nullptr;
    int           level   = 3;

    explicit Impl(int lvl) : level(lvl) {
        cstream = ZSTD_createCStream();
        if (cstream) ZSTD_initCStream(cstream, level);
    }
    ~Impl() { if (cstream) ZSTD_freeCStream(cstream); }

    void reinit(int new_level) {
        level = (new_level > 0) ? new_level : level;
        if (cstream) ZSTD_initCStream(cstream, level);
    }
#else
    explicit Impl(int) {}
#endif
};

ZstdStreamCompressor::ZstdStreamCompressor(int level)
    : impl_(std::make_unique<Impl>(level)) {}

ZstdStreamCompressor::~ZstdStreamCompressor() = default;

Result<std::vector<uint8_t>> ZstdStreamCompressor::compress_chunk(const uint8_t* data, size_t size) {
#ifdef THEMIS_HAS_ZSTD
    if (!impl_->cstream) {
        return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                          "ZSTD stream not initialised");
    }
    if (!data || size == 0) return Ok(std::vector<uint8_t>());

    const size_t out_buf_size = ZSTD_CStreamOutSize();
    std::vector<uint8_t> output;
    output.reserve(out_buf_size);

    ZSTD_inBuffer  in  = { data, size, 0 };
    while (in.pos < in.size) {
        std::vector<uint8_t> chunk(out_buf_size);
        ZSTD_outBuffer out = { chunk.data(), chunk.size(), 0 };
        const size_t rc = ZSTD_compressStream(impl_->cstream, &out, &in);
        if (ZSTD_isError(rc)) {
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                              ZSTD_getErrorName(rc));
        }
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<ptrdiff_t>(out.pos));
    }
    return Ok(std::move(output));
#else
    (void)data;
    (void)size;
    return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                      "ZSTD support not compiled in");
#endif
}

Result<std::vector<uint8_t>> ZstdStreamCompressor::flush() {
#ifdef THEMIS_HAS_ZSTD
    if (!impl_->cstream) {
        return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                          "ZSTD stream not initialised");
    }
    const size_t out_buf_size = ZSTD_CStreamOutSize();
    std::vector<uint8_t> output;

    // Flush then end-frame loop.
    for (bool done = false; !done; ) {
        std::vector<uint8_t> chunk(out_buf_size);
        ZSTD_outBuffer out = { chunk.data(), chunk.size(), 0 };
        const size_t remaining = ZSTD_endStream(impl_->cstream, &out);
        if (ZSTD_isError(remaining)) {
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                              ZSTD_getErrorName(remaining));
        }
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<ptrdiff_t>(out.pos));
        done = (remaining == 0);
    }
    // Reset so the compressor can be reused.
    ZSTD_initCStream(impl_->cstream, impl_->level);
    return Ok(std::move(output));
#else
    return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                      "ZSTD support not compiled in");
#endif
}

void ZstdStreamCompressor::reset(int level) {
#ifdef THEMIS_HAS_ZSTD
    if (impl_->cstream) impl_->reinit(level);
#else
    (void)level;
#endif
}

// ---------------------------------------------------------------------------
// Streaming decompressor
// ---------------------------------------------------------------------------

struct ZstdStreamDecompressor::Impl {
#ifdef THEMIS_HAS_ZSTD
    ZSTD_DStream* dstream = nullptr;
    bool          done    = false;

    Impl() {
        dstream = ZSTD_createDStream();
        if (dstream) ZSTD_initDStream(dstream);
    }
    ~Impl() { if (dstream) ZSTD_freeDStream(dstream); }

    void reinit() {
        done = false;
        if (dstream) ZSTD_initDStream(dstream);
    }
#else
    bool done = false;
    Impl() {}
#endif
};

ZstdStreamDecompressor::ZstdStreamDecompressor()
    : impl_(std::make_unique<Impl>()) {}

ZstdStreamDecompressor::~ZstdStreamDecompressor() = default;

Result<std::vector<uint8_t>> ZstdStreamDecompressor::decompress_chunk(const uint8_t* data, size_t size) {
#ifdef THEMIS_HAS_ZSTD
    if (!impl_->dstream) {
        return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                          "ZSTD decompression stream not initialised");
    }
    if (!data || size == 0) return Ok(std::vector<uint8_t>());

    const size_t out_buf_size = ZSTD_DStreamOutSize();
    std::vector<uint8_t> output;
    output.reserve(out_buf_size);

    ZSTD_inBuffer in = { data, size, 0 };
    while (in.pos < in.size) {
        std::vector<uint8_t> chunk(out_buf_size);
        ZSTD_outBuffer out = { chunk.data(), chunk.size(), 0 };
        const size_t rc = ZSTD_decompressStream(impl_->dstream, &out, &in);
        if (ZSTD_isError(rc)) {
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                              ZSTD_getErrorName(rc));
        }
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<ptrdiff_t>(out.pos));
        if (rc == 0) {
            impl_->done = true;
            break; // Frame complete.
        }
    }
    return Ok(std::move(output));
#else
    (void)data;
    (void)size;
    return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
                                      "ZSTD support not compiled in");
#endif
}

bool ZstdStreamDecompressor::is_done() const {
    return impl_->done;
}

void ZstdStreamDecompressor::reset() {
#ifdef THEMIS_HAS_ZSTD
    if (impl_->dstream) impl_->reinit();
#endif
}

} // namespace utils
} // namespace themis
