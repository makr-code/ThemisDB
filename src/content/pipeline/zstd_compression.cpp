// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include "content/pipeline/zstd_compression.h"

namespace themis::content::pipeline {

std::vector<uint8_t> ZstdCompression::compress(const std::vector<uint8_t>& data) {
    // Placeholder implementation - returns data as-is
    // TODO: Implement actual ZSTD compression using libzstd
    // Future: Add compression level support, streaming, dictionary-based compression
    return data;
}

std::vector<uint8_t> ZstdCompression::decompress(const std::vector<uint8_t>& compressed_data) {
    // Placeholder implementation - returns data as-is
    // TODO: Implement actual ZSTD decompression using libzstd
    // Future: Add error handling, validation, streaming decompression
    return compressed_data;
}

void ZstdCompression::set_compression_level(int level) {
    // Placeholder implementation - stores level but doesn't use it yet
    // TODO: Validate level range (1-22 for ZSTD)
    compression_level_ = level;
}

int ZstdCompression::get_compression_level() const {
    return compression_level_;
}

}  // namespace themis::content::pipeline
