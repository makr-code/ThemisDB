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
    // Streaming compression implementation
    // Processes data in chunks with progress callbacks
    // Useful for large files that don't fit in memory
    
    if (data.empty()) {
        return {};
    }
    
    std::vector<uint8_t> result;
    size_t processed = 0;
    const size_t total = data.size();
    
    // Process data in chunks
    for (size_t offset = 0; offset < total; offset += chunk_size) {
        size_t current_chunk_size = std::min(chunk_size, total - offset);
        
        // Extract chunk
        std::vector<uint8_t> chunk(
            data.begin() + offset,
            data.begin() + offset + current_chunk_size
        );
        
        // Compress chunk using existing utils::zstd_compress
        auto compressed_chunk = themis::utils::zstd_compress(chunk, compression_level_);
        
        if (compressed_chunk.empty() && !chunk.empty()) {
            // Compression failed
            return {};
        }
        
        // Append compressed chunk
        result.insert(result.end(), compressed_chunk.begin(), compressed_chunk.end());
        
        processed += current_chunk_size;
        
        // Call progress callback
        if (callback) {
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
        callback(result.size(), result.size());
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
