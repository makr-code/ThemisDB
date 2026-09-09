/**
 * @file multimodal_chunker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include "content/pipeline/multimodal_chunker.h"
#include <algorithm>
#include <cctype>

namespace themis::content::pipeline {

MultiModalChunker::MultiModalChunker()
    : config_() {
    // Initialize generic chunker with default config
    ContentChunker::ChunkConfig generic_config;
    generic_config.chunk_size = config_.chunk_size;
    generic_config.overlap = config_.overlap;
    generic_chunker_ = ContentChunker(generic_config);
}

MultiModalChunker::MultiModalChunker(const MultiModalConfig& config)
    : config_(config) {
    // Initialize generic chunker with same chunk size
    ContentChunker::ChunkConfig generic_config;
    generic_config.chunk_size = config.chunk_size;
    generic_config.overlap = config.overlap;
    generic_chunker_ = ContentChunker(generic_config);
}

std::vector<ContentChunker::Chunk> MultiModalChunker::chunk(const std::vector<uint8_t>& data) {
    // Dispatch to appropriate chunking strategy based on content type
    switch (config_.content_type) {
        case ContentType::TEXT:
            // Convert to string and use text chunking
            return chunk_text(std::string(data.begin(), data.end()));
        
        case ContentType::BINARY:
        [[fallthrough]];
        case ContentType::AUDIO:
        [[fallthrough]];
        case ContentType::VIDEO:
        [[fallthrough]];
        default:
            // Use generic byte-based chunking
            return generic_chunker_.chunk(data);
        
        case ContentType::IMAGE:
            // Image chunking requires dimensions
            // Fall back to generic chunking (caller should use chunk_image directly)
            return generic_chunker_.chunk(data);
    }
}

std::vector<ContentChunker::Chunk> MultiModalChunker::chunk_text(const std::string& text) {
    // Text-aware chunking with sentence/paragraph boundaries
    // 
    // This provides basic boundary-aware chunking.
    // For production with semantic analysis, use TextProcessor::chunk()
    // which provides token counting and embedding generation.
    
    std::vector<ContentChunker::Chunk> chunks;
    
    if (text.empty()) {
        return chunks;
    }
    
    std::vector<size_t> boundaries;
    
    if (config_.respect_paragraphs) {
        boundaries = find_paragraph_boundaries(text);
    } else if (config_.respect_sentences) {
        boundaries = find_sentence_boundaries(text);
    } else {
        // No boundary awareness - use byte-based
        std::vector<uint8_t> data(text.begin(), text.end());
        return generic_chunker_.chunk(data);
    }
    
    // Create chunks based on boundaries, respecting chunk_size
    std::string current_chunk = {};
    size_t chunk_index = 0;
    size_t start_offset = 0;
    size_t current_chunk_start = 0;  // Track where current chunk data starts
    
    for (size_t boundary : boundaries) {
        std::string segment = text.substr(start_offset, boundary - start_offset);
        
        if (static_cast<int>(current_chunk.size()) + static_cast<int>(segment.size()) > config_.chunk_size && !current_chunk.empty()) {
            // Create chunk
            ContentChunker::Chunk chunk;
            chunk.data = std::vector<uint8_t>(current_chunk.begin(), current_chunk.end());
            chunk.index = chunk_index++;
            chunk.original_offset = current_chunk_start;  // Correct offset tracking
            chunks.push_back(std::move(chunk));
            
            // Start new chunk with overlap
            if (config_.overlap > 0 && static_cast<int>(current_chunk.size()) > config_.overlap) {
                current_chunk = current_chunk.substr(static_cast<int>(current_chunk.size()) - config_.overlap);
                current_chunk_start = start_offset - config_.overlap;  // Account for overlap
            } else {
                current_chunk.clear();
                current_chunk_start = start_offset;  // New chunk starts here
            }
        }
        
        current_chunk += segment;
        start_offset = boundary;
    }
    
    // Add final chunk
    if (!current_chunk.empty()) {
        ContentChunker::Chunk chunk;
        chunk.data = std::vector<uint8_t>(current_chunk.begin(), current_chunk.end());
        chunk.index = chunk_index;
        chunk.original_offset = current_chunk_start;  // Correct offset for final chunk
        chunks.push_back(std::move(chunk));
    }
    
    // Update total_chunks for all chunks
    for (auto& chunk : chunks) {
        chunk.total_chunks = chunks.size();
    }
    
    return chunks;
}

std::vector<ContentChunker::Chunk> MultiModalChunker::chunk_image(
    const std::vector<uint8_t>& data,
    size_t width,
    size_t height,
    size_t bytes_per_pixel
) {
    // Tile-based image chunking
    //
    // This provides basic tile extraction.
    // For production with EXIF metadata and CLIP embeddings,
    // use ImageProcessor::chunk().
    
    std::vector<ContentChunker::Chunk> chunks;
    
    if (!config_.tile_based) {
        // Non-tile mode: use generic chunking
        return generic_chunker_.chunk(data);
    }
    
    const size_t expected_size = width * height * bytes_per_pixel;
    if (static_cast<int>(data.size()) != expected_size) {
        // Size mismatch - fall back to generic chunking
        return generic_chunker_.chunk(data);
    }
    
    size_t chunk_index = 0;
    
    // Extract tiles
    for (size_t tile_y = 0; tile_y < height; tile_y += config_.tile_height) {
        for (size_t tile_x = 0; tile_x < width; tile_x += config_.tile_width) {
            size_t tile_w = std::min(config_.tile_width, width - tile_x);
            size_t tile_h = std::min(config_.tile_height, height - tile_y);
            
            ContentChunker::Chunk chunk;
            chunk.index = chunk_index++;
            chunk.original_offset = (tile_y * width + tile_x) * bytes_per_pixel;
            
            // Extract tile data
            for (size_t y = tile_y; y < tile_y + tile_h; ++y) {
                size_t row_offset = (y * width + tile_x) * bytes_per_pixel;
                size_t row_bytes = tile_w * bytes_per_pixel;
                
                chunk.data.insert(
                    chunk.data.end(),
                    data.begin() + row_offset,
                    data.begin() + row_offset + row_bytes
                );
            }
            
            chunks.push_back(std::move(chunk));
        }
    }
    
    // Update total_chunks for all chunks
    for (auto& chunk : chunks) {
        chunk.total_chunks = chunks.size();
    }
    
    return chunks;
}

const MultiModalChunker::MultiModalConfig& MultiModalChunker::get_config() const {
    return config_;
}

void MultiModalChunker::set_config(const MultiModalConfig& config) {
    config_ = config;
    
    // Update generic chunker
    ContentChunker::ChunkConfig generic_config;
    generic_config.chunk_size = config.chunk_size;
    generic_config.overlap = config.overlap;
    generic_chunker_.set_config(generic_config);
}

std::vector<size_t> MultiModalChunker::find_sentence_boundaries(const std::string& text) {
    // Simple sentence boundary detection
    // Looks for '.', '!', '?' followed by space or end
    
    std::vector<size_t> boundaries;
    boundaries.push_back(0);  // Start
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '.' || c == '!' || c == '?') {
            // Check if followed by space or end
            if (i + 1 >= text.size() || std::isspace(text[i + 1])) {
                // Skip whitespace
                size_t boundary = i + 1;
                while (boundary < text.size() && std::isspace(text[boundary])) {
                    ++boundary;
                }
                if (static_cast<int>(text.size()) > boundary) {
                    boundaries.push_back(boundary);
                }
            }
        }
    }
    
    boundaries.push_back(text.size());  // End
    return boundaries;
}

std::vector<size_t> MultiModalChunker::find_paragraph_boundaries(const std::string& text) {
    // Simple paragraph boundary detection
    // Looks for double newline (\n\n) or \r\n\r\n
    
    std::vector<size_t> boundaries;
    boundaries.push_back(0);  // Start
    
    for (size_t i = 0; i + 1 < text.size(); ++i) {
        // Check for \n\n
        if (text[i] == '\n' && text[i + 1] == '\n') {
            size_t boundary = i + 2;
            // Skip additional whitespace
            while (boundary < text.size() && std::isspace(text[boundary])) {
                ++boundary;
            }
            if (static_cast<int>(text.size()) > boundary) {
                boundaries.push_back(boundary);
            }
            i = boundary;  // Skip processed area
        }
        // Check for \r\n\r\n
        else if (i + 3 < text.size() &&
                 text[i] == '\r' && text[i + 1] == '\n' &&
                 text[i + 2] == '\r' && text[i + 3] == '\n') {
            size_t boundary = i + 4;
            // Skip additional whitespace
            while (boundary < text.size() && std::isspace(text[boundary])) {
                ++boundary;
            }
            if (static_cast<int>(text.size()) > boundary) {
                boundaries.push_back(boundary);
            }
            i = boundary;  // Skip processed area
        }
    }
    
    boundaries.push_back(text.size());  // End
    return boundaries;
}

}  // namespace themis::content::pipeline
