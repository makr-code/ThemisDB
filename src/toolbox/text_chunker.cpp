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


#include "toolbox/text_chunker.h"
#include "rag/document_splitter.h"

#include <atomic>
#include <sstream>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Helper metrics tracking
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::atomic<uint64_t> g_text_chunker_errors_total(0);  ///< Helper error counter
}

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

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Metrics export for helper diagnostics
// ─────────────────────────────────────────────────────────────────────────────

std::string getTextChunkerMetrics() {
    const uint64_t errors = g_text_chunker_errors_total.load(std::memory_order_relaxed);
    if (errors == 0) {
      return "";
    }
    
    std::ostringstream out;
    out << "# HELP toolbox_text_chunker_errors_total Text chunker helper errors.\n";
    out << "# TYPE toolbox_text_chunker_errors_total counter\n";
    out << "toolbox_text_chunker_errors_total " << errors << "\n";
    return out.str();
}

} // namespace toolbox
} // namespace themis
