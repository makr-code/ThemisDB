// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace themis::content::pipeline {

/**
 * @brief ZSTD compression interface for content pipeline
 * 
 * This is a placeholder class for GAP-005 implementation.
 * Future enhancements:
 * - Support for compression levels
 * - Streaming compression for large files
 * - Dictionary-based compression for similar content
 * - Compression statistics and metrics
 */
class ZstdCompression {
public:
    ZstdCompression() = default;
    ~ZstdCompression() = default;

    /**
     * @brief Compress data using ZSTD algorithm (placeholder)
     * @param data Input data to compress
     * @return Compressed data (currently returns input as-is)
     */
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);

    /**
     * @brief Decompress ZSTD compressed data (placeholder)
     * @param compressed_data Compressed input data
     * @return Decompressed data (currently returns input as-is)
     */
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data);

    /**
     * @brief Set compression level (placeholder)
     * @param level Compression level (1-22, higher = better compression)
     */
    void set_compression_level(int level);

    /**
     * @brief Get current compression level
     * @return Current compression level
     */
    int get_compression_level() const;

private:
    int compression_level_ = 3;  // Default ZSTD compression level
};

}  // namespace themis::content::pipeline
