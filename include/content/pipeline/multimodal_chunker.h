/**
 * @file multimodal_chunker.h
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

#include "content/pipeline/content_chunker.h"
#include <string>
#include <vector>
#include <cstdint>

namespace themis::content::pipeline {

/**
 * @brief Content type for multi-modal chunking
 */
enum class ContentType {
    BINARY,    // Generic binary data
    TEXT,      // Text content (UTF-8)
    IMAGE,     // Image data
    AUDIO,     // Audio data
    VIDEO      // Video data
};

/**
 * @brief Multi-modal chunking strategies
 * 
 * Provides content-aware chunking for different content types,
 * integrating with IContentProcessor strategies where appropriate.
 * 
 * This complements the existing IContentProcessor::chunk() by providing
 * a unified pipeline interface for common chunking patterns.
 */
class MultiModalChunker {
public:
    /**
     * @brief Configuration for multi-modal chunking
     */
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
    explicit MultiModalChunker(const MultiModalConfig& config);
    ~MultiModalChunker() = default;

    /**
     * @brief Chunk content based on content type
     * 
     * Uses content-aware strategies for different types.
     * For production use with actual content analysis, integrate
     * with IContentProcessor implementations.
     * 
     * @param data Input data to chunk
     * @return Vector of chunks
     */
    std::vector<ContentChunker::Chunk> chunk(const std::vector<uint8_t>& data);

    /**
     * @brief Chunk text content with sentence/paragraph awareness
     * 
     * For production, consider using TextProcessor::chunk() which provides
     * token-based chunking with semantic embeddings.
     * 
     * @param text Input text (UTF-8)
     * @return Vector of text chunks
     */
    std::vector<ContentChunker::Chunk> chunk_text(const std::string& text);

    /**
     * @brief Chunk image data with tile-based strategy
     * 
     * For production, consider using ImageProcessor::chunk() which provides
     * EXIF metadata extraction and CLIP embeddings.
     * 
     * @param data Image data
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param bytes_per_pixel Bytes per pixel (e.g., 3 for RGB, 4 for RGBA)
     * @return Vector of image tile chunks
     */
    std::vector<ContentChunker::Chunk> chunk_image(
        const std::vector<uint8_t>& data,
        size_t width,
        size_t height,
        size_t bytes_per_pixel
    );

    /**
     * @brief Get current configuration
     */
    const MultiModalConfig& get_config() const;

    /**
     * @brief Set new configuration
     */
    void set_config(const MultiModalConfig& config);

private:
    MultiModalConfig config_;
    ContentChunker generic_chunker_;  // Fallback to generic chunking
    
    // Helper methods
    std::vector<size_t> find_sentence_boundaries(const std::string& text);
    std::vector<size_t> find_paragraph_boundaries(const std::string& text);
};

}  // namespace themis::content::pipeline
