/**
 * @file text_chunker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: text_chunker.cpp | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 80
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/text_chunker.h"
#include "rag/document_splitter.h"

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// TextChunker
// ─────────────────────────────────────────────────────────────────────────────

TextChunker::TextChunker()
    : splitter_()
{}

TextChunker::TextChunker(const rag::DocumentSplitterConfig& config)
    : splitter_(config)
{}

TextChunker::~TextChunker() = default;

const rag::DocumentSplitterConfig& TextChunker::getConfig() const {
    return splitter_.getConfig();
}

void TextChunker::setConfig(const rag::DocumentSplitterConfig& config) {
    splitter_.setConfig(config);
}

std::vector<rag::DocumentChunk> TextChunker::chunk(
    const std::string& text,
    const std::string& document_id) const
{
    return splitter_.split(text, document_id);
}

std::vector<std::string> TextChunker::chunkTexts(
    const std::string& text,
    const std::string& document_id) const
{
    auto chunks = splitter_.split(text, document_id);
    std::vector<std::string> result;
    result.reserve(chunks.size());
    for (auto& c : chunks) {
        result.push_back(std::move(c.text));
    }
    return result;
}

std::size_t TextChunker::estimateTokens(const std::string& text) const {
    return splitter_.estimateTokens(text);
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> chunkText(
    const std::string& text,
    std::size_t        chunk_size,
    std::size_t        overlap)
{
    rag::DocumentSplitterConfig cfg;
    cfg.chunk_size = chunk_size;
    cfg.overlap    = overlap;
    cfg.strategy   = rag::SplitStrategy::Sentence;
    TextChunker chunker(cfg);
    return chunker.chunkTexts(text);
}

} // namespace toolbox
} // namespace themis
