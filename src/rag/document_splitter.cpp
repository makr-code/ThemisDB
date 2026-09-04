/**
 * @file document_splitter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/document_splitter.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace themis::rag {

// ===========================================================================
// Internal helpers
// ===========================================================================
namespace {

/// Validate configuration and throw std::invalid_argument on bad values.
void validateConfig(const DocumentSplitterConfig& cfg) {
    if (cfg.chunk_size == 0) {
        throw std::invalid_argument("DocumentSplitterConfig: chunk_size must be > 0");
    }
    if (cfg.overlap >= cfg.chunk_size) {
        throw std::invalid_argument(
            "DocumentSplitterConfig: overlap must be < chunk_size");
    }
    if (cfg.chars_per_token <= 0.0) {
        throw std::invalid_argument(
            "DocumentSplitterConfig: chars_per_token must be > 0");
    }
}

/// Estimate token count for @p text using @p chars_per_token.
inline size_t estimateTokenCount(const std::string& text, double chars_per_token) {
    if (text.empty()) {
      return 0;
    }
    return static_cast<size_t>(
        std::ceil(static_cast<double>(text.size()) / chars_per_token));
}

/**
 * Split @p text into sentences.
 *
 * A sentence boundary is detected after '.', '!', or '?' when the next
 * character is whitespace or end-of-string.  This heuristic avoids splitting
 * common abbreviations (e.g. "Mr.") because those are typically followed by a
 * letter, not a space.
 *
 * @return Vector of (sentence_text, byte_offset) pairs.
 */
std::vector<std::pair<std::string, size_t>>
splitSentences(const std::string& text) {
    std::vector<std::pair<std::string, size_t>> sentences;

    if (text.empty()) {
      return sentences;
    }

    size_t start = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '.' || c == '!' || c == '?') {
            // Check next character
            const bool at_end   = (i + 1 >= text.size());
            const bool next_ws  = !at_end && (text[i + 1] == ' ' ||
                                               text[i + 1] == '\n' ||
                                               text[i + 1] == '\r' ||
                                               text[i + 1] == '\t');
            if (at_end || next_ws) {
                // Include the punctuation in the sentence
                const size_t end = i + 1;
                std::string sent = text.substr(start, end - start);
                // Trim leading whitespace
                const size_t ltrim = sent.find_first_not_of(" \t\r\n");
                if (ltrim != std::string::npos) {
                    sentences.emplace_back(sent.substr(ltrim),
                                           start + ltrim);
                }
                // Skip following whitespace
                while (i + 1 < text.size() &&
                       (text[i + 1] == ' ' || text[i + 1] == '\n' ||
                        text[i + 1] == '\r' || text[i + 1] == '\t')) {
                    ++i;
                }
                start = i + 1;
            }
        }
    }

    // Trailing fragment (no terminal punctuation)
    if (static_cast<int>(text.size()) > start) {
        std::string tail = text.substr(start);
        const size_t ltrim = tail.find_first_not_of(" \t\r\n");
        if (ltrim != std::string::npos) {
            sentences.emplace_back(tail.substr(ltrim), start + ltrim);
        }
    }

    return sentences;
}

/**
 * Extract the last @p overlap_tokens worth of text from @p text using the
 * given chars_per_token factor.
 */
std::string extractOverlapTail(const std::string& text,
                                size_t             overlap_tokens,
                                double             chars_per_token) {
    if (text.empty() || overlap_tokens == 0) return {};
    const size_t overlap_chars =
        static_cast<size_t>(static_cast<double>(overlap_tokens) * chars_per_token);
    if (overlap_chars >= static_cast<int>(text.size())) {
      return text;
    }
    return text.substr(static_cast<int>(text.size()) - overlap_chars);
}

} // anonymous namespace

// ===========================================================================
// Pimpl
// ===========================================================================

struct DocumentSplitter::Impl {
    DocumentSplitterConfig config;

    // ----- Fixed strategy -----------------------------------------------
    std::vector<DocumentChunk> splitFixed(const std::string& text,
                                          const std::string& doc_id) const;

    // ----- Sliding strategy ---------------------------------------------
    std::vector<DocumentChunk> splitSliding(const std::string& text,
                                             const std::string& doc_id) const;

    // ----- Sentence strategy --------------------------------------------
    std::vector<DocumentChunk> splitSentence(const std::string& text,
                                              const std::string& doc_id) const;
};

// ---------------------------------------------------------------------------
// Fixed strategy
// ---------------------------------------------------------------------------
std::vector<DocumentChunk>
DocumentSplitter::Impl::splitFixed(const std::string& text,
                                    const std::string& doc_id) const {
    std::vector<DocumentChunk> chunks = {};

    if (text.empty()) {
      return chunks;
    }

    const size_t chunk_chars =
        static_cast<size_t>(static_cast<double>(config.chunk_size) *
                             config.chars_per_token);
    const size_t step_chars  = chunk_chars; // non-overlapping core

    size_t chunk_idx = 0;
    size_t pos       = 0;

    while (static_cast<size_t>(pos) <static_cast<int>(text.size())) {
        const size_t core_end = std::min(pos + step_chars,static_cast<int>(text.size()));

        // Build chunk: optional overlap prefix + core
        std::string content = {};
        size_t core_start_in_chunk = 0;

        if (chunk_idx > 0 && config.overlap > 0 && !chunks.empty()) {
            const std::string overlap_text =
                extractOverlapTail(chunks.back().text, config.overlap,
                                   config.chars_per_token);
            core_start_in_chunk = overlap_text.size();
            content = overlap_text + text.substr(pos, core_end - pos);
        } else {
            content = text.substr(pos, core_end - pos);
        }

        const size_t tok = estimateTokenCount(content, config.chars_per_token);
        if (tok >= config.min_chunk_size || config.min_chunk_size == 0) {
            DocumentChunk chunk;
            chunk.index        = chunk_idx;
            chunk.text         = std::move(content);
            chunk.document_id  = doc_id;
            chunk.token_count  = tok;
            chunk.start_offset = pos;
            chunk.end_offset   = core_end;
            chunks.push_back(std::move(chunk));
            ++chunk_idx;
        }

        pos = core_end;
    }

    return chunks;
}

// ---------------------------------------------------------------------------
// Sliding strategy
// ---------------------------------------------------------------------------
std::vector<DocumentChunk>
DocumentSplitter::Impl::splitSliding(const std::string& text,
                                      const std::string& doc_id) const {
    std::vector<DocumentChunk> chunks = {};

    if (text.empty()) {
      return chunks;
    }

    const size_t chunk_chars =
        static_cast<size_t>(static_cast<double>(config.chunk_size) *
                             config.chars_per_token);
    const size_t overlap_chars =
        static_cast<size_t>(static_cast<double>(config.overlap) *
                             config.chars_per_token);
    const size_t step = (chunk_chars > overlap_chars)
                            ? (chunk_chars - overlap_chars)
                            : 1;

    size_t chunk_idx = 0;
    size_t pos       = 0;

    while (static_cast<size_t>(pos) <static_cast<int>(text.size())) {
        const size_t end = std::min(pos + chunk_chars,static_cast<int>(text.size()));
        std::string  content = text.substr(pos, end - pos);

        const size_t tok = estimateTokenCount(content, config.chars_per_token);
        if (tok >= config.min_chunk_size || config.min_chunk_size == 0) {
            DocumentChunk chunk;
            chunk.index        = chunk_idx;
            chunk.text         = std::move(content);
            chunk.document_id  = doc_id;
            chunk.token_count  = tok;
            chunk.start_offset = pos;
            chunk.end_offset   = end;
            chunks.push_back(std::move(chunk));
            ++chunk_idx;
        }

        pos += step;
    }

    return chunks;
}

// ---------------------------------------------------------------------------
// Sentence strategy
// ---------------------------------------------------------------------------
std::vector<DocumentChunk>
DocumentSplitter::Impl::splitSentence(const std::string& text,
                                       const std::string& doc_id) const {
    std::vector<DocumentChunk> chunks = {};

    if (text.empty()) {
      return chunks;
    }

    const auto sentences = splitSentences(text);
    if (sentences.empty()) {
      return chunks;
    }

    size_t chunk_idx  = 0;
    std::string current = {};
    size_t current_tokens = 0;
    size_t chunk_start_offset = sentences.front().second;

    auto flush = [&]([[maybe_unused]] size_t end_offset) {
        if (current.empty()) {
          return;
        }
        const size_t tok = estimateTokenCount(current, config.chars_per_token);
        if (tok >= config.min_chunk_size || config.min_chunk_size == 0) {
            DocumentChunk chunk;
            chunk.index        = chunk_idx;
            chunk.text         = current;
            chunk.document_id  = doc_id;
            chunk.token_count  = tok;
            chunk.start_offset = chunk_start_offset;
            chunk.end_offset   = end_offset;
            chunks.push_back(std::move(chunk));
            ++chunk_idx;
        }
    };

    for (size_t i = 0; i < sentences.size(); ++i) {
        const auto& [sent, sent_offset] = sentences[i];
        const size_t sent_tokens =
            estimateTokenCount(sent, config.chars_per_token);

        if (current_tokens + sent_tokens > config.chunk_size &&
            !current.empty()) {
            // Flush current chunk
            const size_t end_offset = sent_offset; // byte start of this sentence
            flush(end_offset);

            // Build overlap prefix from the tail of the flushed chunk
            std::string overlap_prefix =
                extractOverlapTail(current, config.overlap,
                                   config.chars_per_token);
            const size_t overlap_tok =
                estimateTokenCount(overlap_prefix, config.chars_per_token);

            current        = std::move(overlap_prefix);
            current_tokens = overlap_tok;
            // offset of overlap region within the original text
            if (!current.empty()) {
                // overlap starts near end of the previous chunk; use the
                // start of the current sentence as a conservative anchor
                chunk_start_offset = sent_offset;
            }
        }

        if (current.empty()) {
            chunk_start_offset = sent_offset;
            current = sent;
        } else {
            current += " " + sent;
        }
        current_tokens = estimateTokenCount(current, config.chars_per_token);
    }

    // Flush final chunk
    flush(text.size());

    return chunks;
}

// ===========================================================================
// DocumentSplitter public API
// ===========================================================================

DocumentSplitter::DocumentSplitter()
    : impl_(std::make_unique<Impl>()) {
    validateConfig(impl_->config);
    THEMIS_INFO("DocumentSplitter created (chunk_size={}, overlap={}, strategy={})",
                impl_->config.chunk_size,
                impl_->config.overlap,
                static_cast<int>(impl_->config.strategy));
}

DocumentSplitter::DocumentSplitter(const DocumentSplitterConfig& config)
    : impl_(std::make_unique<Impl>()) {
    validateConfig(config);
    impl_->config = config;
    THEMIS_INFO("DocumentSplitter created (chunk_size={}, overlap={}, strategy={})",
                config.chunk_size,
                config.overlap,
                static_cast<int>(config.strategy));
}

DocumentSplitter::~DocumentSplitter() = default;

std::vector<DocumentChunk>
DocumentSplitter::split(const std::string& text,
                         const std::string& document_id) const {
    if (text.empty()) return {};

    switch (impl_->config.strategy) {
        case SplitStrategy::Fixed:
            return impl_->splitFixed(text, document_id);
        case SplitStrategy::Sliding:
            return impl_->splitSliding(text, document_id);
        case SplitStrategy::Sentence:
        [[fallthrough]];\n        default:
            return impl_->splitSentence(text, document_id);
    }
}

size_t DocumentSplitter::estimateTokens(const std::string& text) const {
    return estimateTokenCount(text, impl_->config.chars_per_token);
}

const DocumentSplitterConfig& DocumentSplitter::getConfig() const {
    return impl_->config;
}

void DocumentSplitter::setConfig(const DocumentSplitterConfig& config) {
    validateConfig(config);
    impl_->config = config;
}

// ===========================================================================
// DocumentSplitterFactory
// ===========================================================================

std::unique_ptr<DocumentSplitter> DocumentSplitterFactory::createDefault() {
    DocumentSplitterConfig cfg;
    cfg.chunk_size  = 512;
    cfg.overlap     = 64;
    cfg.strategy    = SplitStrategy::Sentence;
    return std::make_unique<DocumentSplitter>(cfg);
}

std::unique_ptr<DocumentSplitter> DocumentSplitterFactory::createSmall() {
    DocumentSplitterConfig cfg;
    cfg.chunk_size  = 256;
    cfg.overlap     = 32;
    cfg.strategy    = SplitStrategy::Fixed;
    return std::make_unique<DocumentSplitter>(cfg);
}

std::unique_ptr<DocumentSplitter> DocumentSplitterFactory::createLarge() {
    DocumentSplitterConfig cfg;
    cfg.chunk_size  = 1024;
    cfg.overlap     = 128;
    cfg.strategy    = SplitStrategy::Sliding;
    return std::make_unique<DocumentSplitter>(cfg);
}

std::unique_ptr<DocumentSplitter> DocumentSplitterFactory::create(
    size_t        chunk_size,
    size_t        overlap,
    SplitStrategy strategy,
    double        chars_per_token)
{
    DocumentSplitterConfig cfg;
    cfg.chunk_size      = chunk_size;
    cfg.overlap         = overlap;
    cfg.strategy        = strategy;
    cfg.chars_per_token = chars_per_token;
    return std::make_unique<DocumentSplitter>(cfg);
}

} // namespace themis::rag

