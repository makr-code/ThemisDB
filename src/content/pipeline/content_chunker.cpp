/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            content_chunker.cpp                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include "content/pipeline/content_chunker.h"
#include <algorithm>

namespace themis::content::pipeline {

ContentChunker::ContentChunker()
    : config_() {
}

ContentChunker::ContentChunker(const ChunkConfig& config)
    : config_(config) {
}

std::vector<ContentChunker::Chunk> ContentChunker::chunk(const std::vector<uint8_t>& data) {
    // Generic byte-based chunking implementation
    // 
    // This provides simple fixed-size chunking for pipeline operations.
    // For content-aware chunking that respects semantic boundaries:
    // - Text: Use TextProcessor::chunk() (sentence-based with overlap)
    // - Images: Use ImageProcessor::chunk() (tile or region-based)
    // - Audio: Use AudioProcessor chunking (time-based segments)
    // - Video: Use VideoProcessor chunking (frame-based)
    //
    // Future: Add content-aware boundary detection, adaptive sizing,
    // and integration with IContentProcessor strategies.
    
    std::vector<Chunk> chunks;
    
    if (data.empty()) {
        return chunks;
    }
    
    const size_t chunk_size = config_.chunk_size;
    const size_t total_chunks = (data.size() + chunk_size - 1) / chunk_size;
    
    for (size_t i = 0; i < data.size(); i += chunk_size) {
        Chunk chunk;
        size_t remaining = data.size() - i;
        size_t current_chunk_size = std::min(chunk_size, remaining);
        
        chunk.data = std::vector<uint8_t>(data.begin() + i, 
                                          data.begin() + i + current_chunk_size);
        chunk.index = chunks.size();
        chunk.total_chunks = total_chunks;
        chunk.original_offset = i;
        
        chunks.push_back(std::move(chunk));
    }
    
    return chunks;
}

std::vector<uint8_t> ContentChunker::reassemble(const std::vector<Chunk>& chunks) {
    // Simple concatenation for generic chunks
    // For content-type-specific reassembly, use the respective processor's
    // reconstruction logic which may handle metadata, overlaps, and
    // content-specific validation.
    
    std::vector<uint8_t> result;
    
    for (const auto& chunk : chunks) {
        result.insert(result.end(), chunk.data.begin(), chunk.data.end());
    }
    
    return result;
}

const ContentChunker::ChunkConfig& ContentChunker::get_config() const {
    return config_;
}

void ContentChunker::set_config(const ChunkConfig& config) {
    config_ = config;
}

}  // namespace themis::content::pipeline
