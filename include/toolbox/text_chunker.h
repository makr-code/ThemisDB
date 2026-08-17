/**
 * @file text_chunker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 97/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/document_splitter.h"
#include <string>
#include <vector>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// TextChunker
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Token-based text chunking façade over `rag::DocumentSplitter`.
 *
 * Provides a stable toolbox-namespace API for text chunking so that modules
 * outside `rag/` do not need to depend on `rag::DocumentSplitter` directly.
 * Delegates all splitting logic to the underlying `DocumentSplitter`.
 *
 * Thread-safety: instances are not thread-safe; use one instance per thread
 * or protect with an external lock.  The free function `chunkText()` creates
 * a temporary instance and is therefore safe to call from multiple threads.
 */
class TextChunker {
public:
    /// Construct with default `DocumentSplitterConfig`
    /// (chunk_size=512, overlap=64, strategy=Sentence).
    TextChunker();

    /// Construct with a custom configuration.
    explicit TextChunker(const rag::DocumentSplitterConfig& config);

    ~TextChunker();

    TextChunker(const TextChunker&)            = delete;
    TextChunker& operator=(const TextChunker&) = delete;
    TextChunker(TextChunker&&)                 noexcept = default;
    TextChunker& operator=(TextChunker&&)      noexcept = default;

    // ── Configuration ─────────────────────────────────────────────────────────

    /// Return the active splitting configuration.
    const rag::DocumentSplitterConfig& getConfig() const;

    /// Replace the active configuration.
    /// @throws std::invalid_argument on invalid parameters (overlap >= chunk_size
    ///         or chunk_size == 0).
    void setConfig(const rag::DocumentSplitterConfig& config);

    // ── Chunking ──────────────────────────────────────────────────────────────

    /**
     * @brief Split @p text into overlapping chunks.
     *
     * @param text        UTF-8 source text.
     * @param document_id Optional document identifier embedded in each chunk.
     * @return Ordered vector of `DocumentChunk` objects (empty when @p text
     *         is empty or only whitespace).
     */
    std::vector<rag::DocumentChunk> chunk(
        const std::string& text,
        const std::string& document_id = "") const;

    /**
     * @brief Split @p text and return only the raw text strings.
     *
     * Convenience variant for callers that only need the text and not the
     * full `DocumentChunk` metadata.
     *
     * @param text        UTF-8 source text.
     * @param document_id Optional document identifier.
     * @return Ordered vector of chunk text strings.
     */
    std::vector<std::string> chunkTexts(
        const std::string& text,
        const std::string& document_id = "") const;

    /**
     * @brief Estimate the token count for @p text using the current config.
     *
     * Uses the same `chars_per_token` factor as the underlying splitter.
     *
     * @param text Input text.
     * @return Estimated token count.
     */
    std::size_t estimateTokens(const std::string& text) const;

private:
    rag::DocumentSplitter splitter_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Free functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Split @p text into overlapping chunks using sentence-boundary strategy.
 *
 * Convenience free function suitable for one-off chunking without managing a
 * `TextChunker` instance.
 *
 * @param text       UTF-8 text to split.
 * @param chunk_size Target chunk size in estimated tokens (default: 512).
 * @param overlap    Token overlap between consecutive chunks (default: 64).
 * @return Ordered vector of chunk text strings.
 */

#pragma once
std::vector<std::string> chunkText(
    const std::string& text,
    std::size_t        chunk_size = 512,
    std::size_t        overlap    = 64);

} // namespace toolbox
} // namespace themis
