/**
 * @file zstd_compression.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
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
 * Features:
 * - Single-shot compression/decompression
 * - Streaming compression for large files
 * - Configurable compression levels
 * - Progress callbacks for streaming operations
 */
class ZstdCompression {
public:
    /**
     * @brief Stream processing callback
     * Called during streaming operations with processed bytes
     */
    using StreamCallback = std::function<void(size_t processed, size_t total)>;

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
     * @brief Streaming compression for large files
     * 
     * Compresses data in chunks with optional progress callback.
     * Useful for files larger than available memory.
     * 
     * @param data Input data to compress
     * @param chunk_size Size of chunks to process (default 1MB)
     * @param callback Optional progress callback
     * @return Compressed data (empty on failure)
     */
    std::vector<uint8_t> compress_streaming(
        const std::vector<uint8_t>& data,
        size_t chunk_size = 1024 * 1024,
        StreamCallback callback = nullptr
    );

    /**
     * @brief Streaming decompression for large files
     * 
     * Decompresses data with optional progress callback.
     * 
     * @param compressed_data Compressed input data
     * @param callback Optional progress callback
     * @return Decompressed data (empty on failure)
     */
    std::vector<uint8_t> decompress_streaming(
        const std::vector<uint8_t>& compressed_data,
        StreamCallback callback = nullptr
    );

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
