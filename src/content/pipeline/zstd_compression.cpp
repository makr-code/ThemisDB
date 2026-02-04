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
