/**
 * @file multimodal_chunker.h
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

#include "content/pipeline/content_chunker.h"
#include <string>
#include <vector>
#include <cstdint>

namespace themis::content::pipeline {

enum class ContentType {
    BINARY,    // Generic binary data
    TEXT,      // Text content (UTF-8)
    IMAGE,     // Image data
    AUDIO,     // Audio data
    VIDEO      // Video data
};

class MultiModalChunker {
public:
    struct MultiModalConfig {
        ContentType content_type = ContentType::BINARY;
        size_t chunk_size = 1024 * 1024;  // Default 1MB
        size_t overlap = 0;
        
        // Text-specific options
        bool respect_sentences = true;      // For TEXT: respect sentence boundaries
        bool respect_paragraphs = true;     // For TEXT: respect paragraph boundaries
        
        // Image-specific options
        bool tile_based = false;             // For IMAGE: tile-based chunking
        size_t tile_width = 256;             // Tile width in pixels
        size_t tile_height = 256;            // Tile height in pixels
        
        // Audio/Video-specific options
        bool time_based = false;             // For AUDIO/VIDEO: time-based chunking
        double chunk_duration_seconds = 10.0; // Duration per chunk in seconds
    };

    MultiModalChunker();
    /**
     * @brief Multi Modal Chunker.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MultiModalChunker(const MultiModalConfig& config);
    ~MultiModalChunker() = default;

    /**
     * @brief Chunk.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::vector<ContentChunker::Chunk> chunk(const std::vector<uint8_t>& data);

    /**
     * @brief Chunk text.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<ContentChunker::Chunk> chunk_text(const std::string& text);

    /**
     * @brief Chunk image.
     * @param[in] data Input parameter.
     * @param[in] width Input parameter.
     * @param[in] height Input parameter.
     * @param[in] bytes_per_pixel Input parameter.
     * @return Return value.
     */
    std::vector<ContentChunker::Chunk> chunk_image(
        const std::vector<uint8_t>& data,
        size_t width,
        size_t height,
        size_t bytes_per_pixel
    );

    /**
     * @brief Get config.
     * @return Return value.
     */
    const MultiModalConfig& get_config() const;

    /**
     * @brief Set config.
     * @param[in] config Input parameter.
     */
    void set_config(const MultiModalConfig& config);

private:
    MultiModalConfig config_;
    ContentChunker generic_chunker_;  // Fallback to generic chunking
    
    // Helper methods
    /**
     * @brief Find sentence boundaries.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<size_t> find_sentence_boundaries(const std::string& text);
    /**
     * @brief Find paragraph boundaries.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<size_t> find_paragraph_boundaries(const std::string& text);
};

}  // namespace themis::content::pipeline
