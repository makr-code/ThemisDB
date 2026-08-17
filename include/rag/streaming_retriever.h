/**
 * @file streaming_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <optional>

namespace themis::rag::streaming {

// ---------------------------------------------------------------------------
// Supporting types
// ---------------------------------------------------------------------------

/**
 * @brief A single document chunk that can be streamed to a caller.
 */
struct StreamedDocument {
    std::string id;              ///< Unique document identifier
    std::string content;         ///< Document text
    double      relevance_score; ///< Semantic similarity to the query (0–1)
    size_t      token_count;     ///< Estimated token count for this document
};

/**
 * @brief Snapshot of the context window at a point in time.
 */
struct ContextWindowState {
    std::vector<StreamedDocument> documents;  ///< Documents currently in context
    size_t total_tokens_used;                 ///< Tokens consumed so far
    size_t max_tokens;                        ///< Configured budget
    double fill_ratio;                        ///< total_tokens_used / max_tokens
    bool   is_full;                           ///< True when no more docs can fit
};

/**
 * @brief Result returned after streaming completes.
 */
struct StreamingResult {
    std::vector<StreamedDocument> selected_documents; ///< Documents that fit
    std::vector<StreamedDocument> skipped_documents;  ///< Docs that did not fit
    size_t total_tokens_used;
    size_t documents_considered;
    size_t documents_added;
    double elapsed_ms;           ///< Wall-clock time for the streaming pass
    bool   cancelled;            ///< True if streaming was cancelled early
};

// ---------------------------------------------------------------------------
// Callback types
// ---------------------------------------------------------------------------

/**
 * @brief Called each time a document is accepted into the context window.
 *
 * @param doc     The newly accepted document.
 * @param state   Current context window state after acceptance.
 */
using DocumentAcceptedCallback =
    std::function<void(const StreamedDocument&, const ContextWindowState&)>;

/**
 * @brief Called each time a document is skipped (token budget would exceed).
 */
using DocumentSkippedCallback =
    std::function<void(const StreamedDocument&, const ContextWindowState&)>;

/**
 * @brief Called when the context window is full.
 */
using WindowFullCallback = std::function<void(const ContextWindowState&)>;

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for StreamingRetriever.
 */
struct StreamingRetrieverConfig {
    /// Maximum number of tokens the context window may hold.
    size_t max_context_tokens = 4096;

    /// Average tokens-per-character used when a document has no pre-computed
    /// token count (approx. 0.25 for English prose).
    double chars_per_token = 4.0;

    /// Maximum number of documents to consider (0 = unlimited).
    size_t max_documents_to_consider = 0;

    /// Minimum relevance score for a document to be eligible (0–1).
    double min_relevance_score = 0.0;

    /// Enable Maximal Marginal Relevance deduplication.
    /// Documents whose content overlap with already-selected documents exceeds
    /// @ref mmr_similarity_threshold are skipped.
    bool enable_mmr_deduplication = false;

    /// Jaccard similarity threshold for MMR deduplication (0–1).
    double mmr_similarity_threshold = 0.85;

    /// If true, emit documents sorted by relevance (highest first).
    /// If false, emit in the order provided by the caller.
    bool sort_by_relevance = true;
};

// ---------------------------------------------------------------------------
// ContextWindowFiller
// ---------------------------------------------------------------------------

/**
 * @brief Manages the bounded token budget for an incremental context window.
 *
 * Tracks which documents have been added, enforces the token budget, and
 * provides a snapshot of the current window state at any time.
 */
class ContextWindowFiller {
public:
    /**
     * @brief Construct with a token budget.
     * @param max_tokens Maximum tokens the window may hold.
     * @param chars_per_token Conversion factor for estimating token counts.
     */
    explicit ContextWindowFiller(size_t max_tokens,
                                 double chars_per_token = 4.0);

    /**
     * @brief Attempt to add a document to the window.
     * @param doc The candidate document.
     * @return true if the document was accepted, false if it did not fit.
     */
    bool tryAdd(const StreamedDocument& doc);

    /**
     * @brief Check whether there is still room for at least @p min_tokens more.
     * @param min_tokens Minimum tokens needed (default 1).
     */
    bool hasCapacity(size_t min_tokens = 1) const;

    /**
     * @brief Return a snapshot of the current context window state.
     */
    ContextWindowState snapshot() const;

    /**
     * @brief Return only the documents currently in the window.
     */
    const std::vector<StreamedDocument>& documents() const;

    /**
     * @brief Reset the window to empty (keep the same token budget).
     */
    void reset();

    /**
     * @brief Estimate token count for a string.
     * @param text Input text.
     * @return Estimated number of tokens.
     */
    size_t estimateTokens(const std::string& text) const;

private:
    size_t max_tokens_;
    double chars_per_token_;
    size_t tokens_used_{0};
    std::vector<StreamedDocument> documents_;
};

// ---------------------------------------------------------------------------
// StreamingRetriever
// ---------------------------------------------------------------------------

/**
 * @brief Streams retrieved documents one at a time, filling the context window
 *        incrementally.
 *
 * Usage:
 * @code
 * StreamingRetrieverConfig cfg;
 * cfg.max_context_tokens = 8192;
 * cfg.sort_by_relevance  = true;
 *
 * StreamingRetriever retriever(cfg);
 *
 * retriever.setDocumentAcceptedCallback(
 *     [](const StreamedDocument& doc, const ContextWindowState& state) {
 *         // handle newly accepted document
 *     });
 *
 * auto result = retriever.stream(query, candidates);
 * @endcode
 */
class StreamingRetriever {
public:
    /**
     * @brief Construct with configuration.
     * @param config Streaming and window configuration.
     */
    explicit StreamingRetriever(const StreamingRetrieverConfig& config = {});
    ~StreamingRetriever();

    // Non-copyable, movable
    StreamingRetriever(const StreamingRetriever&)            = delete;
    StreamingRetriever& operator=(const StreamingRetriever&) = delete;
    StreamingRetriever(StreamingRetriever&&)                 noexcept = default;
    StreamingRetriever& operator=(StreamingRetriever&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /**
     * @brief Register callback invoked for each accepted document.
     */
    void setDocumentAcceptedCallback(DocumentAcceptedCallback cb);

    /**
     * @brief Register callback invoked for each skipped document.
     */
    void setDocumentSkippedCallback(DocumentSkippedCallback cb);

    /**
     * @brief Register callback invoked once the context window is full.
     */
    void setWindowFullCallback(WindowFullCallback cb);

    // -----------------------------------------------------------------------
    // Streaming
    // -----------------------------------------------------------------------

    /**
     * @brief Stream documents from @p candidates into the context window.
     *
     * Documents are processed in relevance order (highest first) when
     * StreamingRetrieverConfig::sort_by_relevance is true.  Each accepted
     * document triggers the DocumentAcceptedCallback.  Streaming stops when
     * the window is full or all candidates have been processed.
     *
     * @param query      Original query text (used for MMR if enabled).
     * @param candidates Candidate documents to stream.
     * @return Final streaming result with selection statistics.
     */
    StreamingResult stream(const std::string& query,
                           std::vector<StreamedDocument> candidates);

    /**
     * @brief Signal that the ongoing stream should stop after the current
     *        document.  Safe to call from another thread.
     */
    void cancel();

    /**
     * @brief Return true if a stream is currently in progress.
     */
    bool isStreaming() const;

    // -----------------------------------------------------------------------
    // Configuration access
    // -----------------------------------------------------------------------

    StreamingRetrieverConfig getConfig() const;
    void setConfig(const StreamingRetrieverConfig& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::streaming
