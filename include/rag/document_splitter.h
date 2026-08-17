/**
 * @file document_splitter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag {

// ---------------------------------------------------------------------------
// SplitStrategy
// ---------------------------------------------------------------------------

/**
 * @brief Document splitting strategy.
 */
enum class SplitStrategy {
    /// Divide text into non-overlapping blocks of @c chunk_size tokens, then
    /// prepend @c overlap tokens from the previous block to each block.
    Fixed,

    /// Advance a window of size @c chunk_size by (@c chunk_size – @c overlap)
    /// tokens per step (classic sliding-window chunking).
    Sliding,

    /// Break at sentence boundaries, accumulating sentences until the chunk
    /// would exceed @c chunk_size; carry over @c overlap tokens into the next
    /// chunk.
    Sentence,
};

// ---------------------------------------------------------------------------
// DocumentSplitterConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for DocumentSplitter.
 */
struct DocumentSplitterConfig {
    /// Target chunk size measured in estimated tokens.
    /// Must be > 0.  Defaults to 512.
    size_t chunk_size = 512;

    /// Number of tokens of overlap between consecutive chunks.
    /// Must be < @c chunk_size.  Defaults to 64.
    size_t overlap = 64;

    /// Splitting strategy to apply.
    SplitStrategy strategy = SplitStrategy::Sentence;

    /// Characters per token used when estimating token counts.
    /// Typical English prose ≈ 4 chars/token.
    double chars_per_token = 4.0;

    /// Minimum chunk size in estimated tokens.  Chunks shorter than this
    /// (e.g. a trailing fragment) are still emitted unless they are completely
    /// empty.  Set to 0 to disable filtering.
    size_t min_chunk_size = 0;
};

// ---------------------------------------------------------------------------
// DocumentChunk
// ---------------------------------------------------------------------------

/**
 * @brief A single chunk produced by DocumentSplitter.
 */
struct DocumentChunk {
    /// Zero-based sequential index within the original document.
    size_t index = 0;

    /// Text content of this chunk.
    std::string text;

    /// Source document identifier (passed in by the caller).
    std::string document_id;

    /// Estimated token count for this chunk.
    size_t token_count = 0;

    /// Byte offset of the first character of this chunk within the original
    /// document text (before any overlap is prepended).
    size_t start_offset = 0;

    /// Byte offset one past the last character of this chunk's core content
    /// (excluding overlap carried over from the preceding chunk).
    size_t end_offset = 0;
};

// ---------------------------------------------------------------------------
// DocumentSplitter
// ---------------------------------------------------------------------------

/**
 * @brief Splits a document into overlapping text chunks.
 *
 * Performance targets (1 MB / ~250 k-token document, Intel Core i7):
 *  - Fixed strategy:    < 5 ms
 *  - Sliding strategy:  < 5 ms
 *  - Sentence strategy: < 20 ms  (sentence detection adds minor overhead)
 */
class DocumentSplitter {
public:
    /**
     * @brief Construct with default configuration.
     */
    DocumentSplitter();

    /**
     * @brief Construct with custom configuration.
     * @param config Splitting configuration.
     * @throws std::invalid_argument if @c config.overlap >= @c config.chunk_size
     *         or @c config.chunk_size == 0 or @c config.chars_per_token <= 0.
     */
    explicit DocumentSplitter(const DocumentSplitterConfig& config);

    ~DocumentSplitter();

    // Non-copyable, movable
    DocumentSplitter(const DocumentSplitter&)            = delete;
    DocumentSplitter& operator=(const DocumentSplitter&) = delete;
    DocumentSplitter(DocumentSplitter&&)                 noexcept = default;
    DocumentSplitter& operator=(DocumentSplitter&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Splitting
    // -----------------------------------------------------------------------

    /**
     * @brief Split @p text into chunks according to the current configuration.
     *
     * @param text        Raw document text to split.
     * @param document_id Identifier embedded in every produced chunk.
     * @return Ordered vector of chunks (empty when @p text is empty).
     */
    std::vector<DocumentChunk> split(const std::string& text,
                                     const std::string& document_id = "") const;

    // -----------------------------------------------------------------------
    // Token estimation
    // -----------------------------------------------------------------------

    /**
     * @brief Estimate the token count for an arbitrary string.
     * @param text Input text.
     * @return Estimated number of tokens.
     */
    size_t estimateTokens(const std::string& text) const;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Return the active configuration.
     */
    const DocumentSplitterConfig& getConfig() const;

    /**
     * @brief Replace the active configuration.
     * @throws std::invalid_argument on invalid parameters (same as constructor).
     */
    void setConfig(const DocumentSplitterConfig& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// DocumentSplitterFactory
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for common splitting configurations.
 */
class DocumentSplitterFactory {
public:
    /**
     * @brief Sentence-boundary splitter with typical RAG defaults.
     *
     * chunk_size=512, overlap=64, strategy=Sentence.
     */
    static std::unique_ptr<DocumentSplitter> createDefault();

    /**
     * @brief Fixed-size splitter optimised for embedding models with 256-token
     *        input windows.
     *
     * chunk_size=256, overlap=32, strategy=Fixed.
     */
    static std::unique_ptr<DocumentSplitter> createSmall();

    /**
     * @brief Sliding-window splitter suitable for long-context models.
     *
     * chunk_size=1024, overlap=128, strategy=Sliding.
     */
    static std::unique_ptr<DocumentSplitter> createLarge();

    /**
     * @brief Create a splitter with fully custom parameters.
     *
     * @param chunk_size    Target chunk size in tokens.
     * @param overlap       Overlap tokens between consecutive chunks.
     * @param strategy      Splitting strategy.
     * @param chars_per_token Characters per token conversion factor.
     */
    static std::unique_ptr<DocumentSplitter> create(
        size_t        chunk_size,
        size_t        overlap,
        SplitStrategy strategy      = SplitStrategy::Sentence,
        double        chars_per_token = 4.0);
};

} // namespace themis::rag
