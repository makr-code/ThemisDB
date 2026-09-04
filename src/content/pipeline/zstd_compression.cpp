/**
 * @file zstd_compression.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include "content/pipeline/zstd_compression.h"
#include "utils/zstd_codec.h"

namespace themis::content::pipeline {

std::vector<uint8_t> ZstdCompression::compress(const std::vector<uint8_t>& data) {
    // Delegate to existing ThemisDB ZSTD implementation
    // utils::zstd_compress handles:
    // - THEMIS_HAS_ZSTD conditional compilation
    // - Security validation (max input size: 1GB)
    // - Error handling (returns empty on failure)
    return themis::utils::zstd_compress(data, compression_level_);
}

std::vector<uint8_t> ZstdCompression::decompress(const std::vector<uint8_t>& compressed_data) {
    // Delegate to existing ThemisDB ZSTD implementation
    // utils::zstd_decompress handles:
    // - THEMIS_HAS_ZSTD conditional compilation
    // - Security validation (max decompressed size: 4GB)
    // - Error handling (returns empty on failure)
    return themis::utils::zstd_decompress(compressed_data);
}

std::vector<uint8_t> ZstdCompression::compress_streaming(
    const std::vector<uint8_t>& data,
    size_t chunk_size,
    StreamCallback callback
) {
    // Simple streaming compression implementation
    // Note: For very large files, this still requires full input in memory.
    // For true streaming (file-to-file without full memory load), use
    // ZSTD's streaming API (ZSTD_createCStream, etc.) directly.
    //
    // This implementation provides progress tracking for large in-memory data.
    
    if (data.empty()) {
        return {};
    }
    
    // For simplicity, compress the entire data at once but provide progress callbacks
    // This maintains compatibility with standard decompress()
    auto result = themis::utils::zstd_compress(data, compression_level_);
    
    if (result.empty() && !data.empty()) {
        return {};  // Compression failed
    }
    
    // Simulate progress for large data
    if (callback) {
        size_t total = data.size();
        size_t processed = 0;
        
        // Report progress in chunks
        while (processed < total) {
            processed = std::min(processed + chunk_size, total);
            callback(processed, total);
        }
    }
    
    return result;
}

std::vector<uint8_t> ZstdCompression::decompress_streaming(
    const std::vector<uint8_t>& compressed_data,
    StreamCallback callback
) {
    // Streaming decompression implementation
    // Decompresses with progress callback
    
    if (compressed_data.empty()) {
        return {};
    }
    
    // For ZSTD, we decompress the entire frame
    // (ZSTD frames are self-contained)
    auto result = themis::utils::zstd_decompress(compressed_data);
    
    // Call progress callback when done
    if (callback && !result.empty()) {
        callback(result.size(),static_cast<int>(result.size()));
    }
    
    return result;
}

void ZstdCompression::set_compression_level(int level) {
    // Validate level range for ZSTD (1-22)
    // Level 3 is default, 19 is high compression used in ContentManager
    if (level < 1) {
        compression_level_ = 1;
    } else if (level > 22) {
        compression_level_ = 22;
    } else {
        compression_level_ = level;
    }
}

int ZstdCompression::get_compression_level() const {
    return compression_level_;
}

}  // namespace themis::content::pipeline
