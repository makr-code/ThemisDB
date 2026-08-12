/**
 * @file content_chunker.h
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
#include <vector>
#include <string>

namespace themis::content::pipeline {

/**
 * @brief Content chunking mechanism for processing large files
 * 
 * This class provides generic byte-based chunking for content pipeline
 * operations. It complements ThemisDB's existing content processor system
 * (IContentProcessor::chunk()) which provides content-type-specific
 * chunking strategies.
 * 
 * Relationship to existing infrastructure:
 * - IContentProcessor::chunk() - Content-aware chunking (text sentences,
 *   image regions, audio segments) with type-specific metadata
 * - ContentChunker - Generic binary chunking for pipeline operations,
 *   useful for raw data processing, pre-processing, and streaming
 * 
 * Use cases:
 * - Pre-chunking before content type detection
 * - Generic binary data streaming
 * - Pipeline operations requiring fixed-size chunks
 * - Testing and development scenarios
 * 
 * For content-aware chunking with semantic boundaries:
 * - Text: Use TextProcessor::chunk() (sentence-based with overlap)
 * - Images: Use ImageProcessor::chunk() (tile-based or region-based)
 * - Audio/Video: Use respective processor chunking strategies
 * 
 * Future enhancements:
 * - Content-aware boundary detection
 * - Adaptive chunk size based on content type
 * - Overlapping chunks for context preservation
 * - Integration adapter for IContentProcessor strategies
 */
class ContentChunker {
public:
    /**
     * @brief Configuration for chunking strategy
     */
    struct ChunkConfig {
        size_t chunk_size = 1024 * 1024;  // Default 1MB chunks
        size_t overlap = 0;                // No overlap by default
        bool content_aware = false;        // Simple byte-based chunking
    };

    /**
     * @brief Represents a single chunk of content
     */
    struct Chunk {
        std::vector<uint8_t> data;
        size_t index;           // Chunk index in sequence
        size_t total_chunks;    // Total number of chunks
        size_t original_offset; // Offset in original content
    };

    ContentChunker();
    explicit ContentChunker(const ChunkConfig& config);
    ~ContentChunker() = default;

    /**
     * @brief Split content into chunks
     * 
     * Generic byte-based chunking. For content-aware chunking
     * that respects semantic boundaries, use IContentProcessor::chunk().
     * 
     * @param data Input data to chunk
     * @return Vector of chunks
     */
    std::vector<Chunk> chunk(const std::vector<uint8_t>& data);

    /**
     * @brief Reconstruct original content from chunks
     * @param chunks Vector of chunks to reassemble
     * @return Reconstructed data
     */
    std::vector<uint8_t> reassemble(const std::vector<Chunk>& chunks);

    /**
     * @brief Get chunk configuration
     * @return Current chunk configuration
     */
    const ChunkConfig& get_config() const;

    /**
     * @brief Update chunk configuration
     * @param config New configuration
     */
    void set_config(const ChunkConfig& config);

private:
    ChunkConfig config_;
};

}  // namespace themis::content::pipeline
