// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "utils/zstd_codec.h"

namespace themis::content::pipeline {

/**
 * @brief ZSTD compression interface for content pipeline
 * 
 * This class provides a pipeline-specific wrapper around ThemisDB's
 * existing ZSTD compression utilities (utils::zstd_codec).
 * 
 * It integrates with the existing, fully-functional ZSTD implementation
 * while providing a consistent API for pipeline operations.
 * 
 * Future enhancements:
 * - Streaming compression for large files
 * - Dictionary-based compression for similar content
 * - Compression statistics and metrics
 * - Batch compression optimization
 */
class ZstdCompression {
public:
    ZstdCompression() = default;
    ~ZstdCompression() = default;

    /**
     * @brief Compress data using ZSTD algorithm
     * 
     * Uses ThemisDB's existing utils::zstd_compress implementation.
     * Returns empty vector on failure or if ZSTD is not available.
     * 
     * @param data Input data to compress
     * @return Compressed data (empty on failure)
     */
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);

    /**
     * @brief Decompress ZSTD compressed data
     * 
     * Uses ThemisDB's existing utils::zstd_decompress implementation.
     * Returns empty vector on failure or if ZSTD is not available.
     * 
     * @param compressed_data Compressed input data
     * @return Decompressed data (empty on failure)
     */
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data);

    /**
     * @brief Set compression level
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
