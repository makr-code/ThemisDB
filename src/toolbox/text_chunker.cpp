/*
 * ThemisDB | File: text_chunker.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 79
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=10 | delta=7 | status=divergent
 * External Severity (v3): C=0, H=8, M=2
 * PR: none
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
