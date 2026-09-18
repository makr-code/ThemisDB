/**
 * @file content_chunker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace themis::content::pipeline {

class ContentChunker {
public:
    struct ChunkConfig {
        size_t chunk_size = 1024 * 1024;  // Default 1MB chunks
        size_t overlap = 0;                // No overlap by default
        bool content_aware = false;        // Simple byte-based chunking
    };

    struct Chunk {
        std::vector<uint8_t> data;
        size_t index;           // Chunk index in sequence
        size_t total_chunks;    // Total number of chunks
        size_t original_offset; // Offset in original content
    };

    ContentChunker();
    /**
     * @brief Content Chunker.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ContentChunker(const ChunkConfig& config);
    ~ContentChunker() = default;

    /**
     * @brief Chunk.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::vector<Chunk> chunk(const std::vector<uint8_t>& data);

    /**
     * @brief Reassemble.
     * @param[in] chunks Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> reassemble(const std::vector<Chunk>& chunks);

    /**
     * @brief Get config.
     * @return Return value.
     */
    const ChunkConfig& get_config() const;

    /**
     * @brief Set config.
     * @param[in] config Input parameter.
     */
    void set_config(const ChunkConfig& config);

private:
    ChunkConfig config_;
};

}  // namespace themis::content::pipeline
