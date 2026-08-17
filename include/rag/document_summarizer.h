/**
 * @file document_summarizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/streaming_retriever.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for DocumentSummarizer
 */
struct DocumentSummarizerConfig {
    /**
     * @brief Summarization strategy
     */
    enum class Strategy {
        EXTRACTIVE,  ///< Sentence-extraction (no LLM, fast)
        ABSTRACTIVE, ///< LLM-generated summary (requires engine)
        AUTO         ///< ABSTRACTIVE if LLM available, else EXTRACTIVE
    };

    /// Summarization strategy to use.
    Strategy strategy = Strategy::AUTO;

    /// Maximum character length of the combined summary produced by
    /// summarizeMultiple().  0 means no limit (use with care).
    size_t max_summary_chars = 2000;

    /// Maximum number of sentences to extract per document in EXTRACTIVE mode.
    size_t max_sentences_per_doc = 5;

    /// Minimum sentence length (characters) to be eligible for extraction.
    size_t min_sentence_chars = 20;

    /// Whether to prepend a "[Source: <id>]" tag to each per-document
    /// summary in the combined output.
    bool include_source_attribution = true;

    /// LLM sampling temperature for ABSTRACTIVE summarization (lower → more
    /// focused / deterministic output).
    double temperature = 0.3;

    /// Maximum tokens the LLM may generate for the summary.
    size_t max_output_tokens = 512;
};

// ---------------------------------------------------------------------------
// Result types
// ---------------------------------------------------------------------------

/**
 * @brief Summary produced for a single document
 */
struct DocumentSummary {
    std::string document_id;    ///< Identifier of the source document
    std::string summary;        ///< Condensed text for this document
    double      coverage_score; ///< Fraction of distinct sentences included [0,1]
    bool        used_llm;       ///< True when abstractive (LLM) path was taken
};

/**
 * @brief Aggregated result of summarizing multiple documents
 */
struct MultiDocumentSummary {
    /// Single combined summary ready for context injection.
    std::string combined_summary;

    /// Per-document breakdowns (in the same order as the input documents).
    std::vector<DocumentSummary> per_document_summaries;

    /// Total character count of all input documents.
    size_t total_input_chars = 0;

    /// Character count of combined_summary.
    size_t summary_chars = 0;

    /// Compression ratio: summary_chars / total_input_chars.
    double compression_ratio = 0.0;

    /// True when at least one document was summarized via the LLM.
    bool used_llm = false;

    /// Wall-clock time for the entire summarization pass (milliseconds).
    double elapsed_ms = 0.0;
};

// ---------------------------------------------------------------------------
// DocumentSummarizer
// ---------------------------------------------------------------------------

/**
 * @brief Summarizes multiple retrieved documents before context injection
 *
 * Usage (default AUTO strategy, no explicit LLM needed for tests):
 * @code
 *   DocumentSummarizer summarizer;
 *   auto result = summarizer.summarizeMultiple(docs, query);
 *   // inject result.combined_summary into the LLM prompt
 * @endcode
 *
 * Usage with explicit config:
 * @code
 *   DocumentSummarizerConfig cfg;
 *   cfg.strategy          = DocumentSummarizerConfig::Strategy::EXTRACTIVE;
 *   cfg.max_summary_chars = 1000;
 *   cfg.max_sentences_per_doc = 3;
 *   DocumentSummarizer summarizer(cfg);
 * @endcode
 *
 * Performance targets:
 *  - EXTRACTIVE: <5 ms for 20 documents of 500 chars each
 *  - ABSTRACTIVE: latency depends on the connected LLM engine
 */
class DocumentSummarizer {
public:
    /**
     * @brief Construct with default (AUTO) configuration
     */
    DocumentSummarizer();

    /**
     * @brief Construct with a custom configuration
     * @param config Summarization configuration
     */
    explicit DocumentSummarizer(const DocumentSummarizerConfig& config);

    ~DocumentSummarizer();

    // Non-copyable, movable
    DocumentSummarizer(const DocumentSummarizer&)            = delete;
    DocumentSummarizer& operator=(const DocumentSummarizer&) = delete;
    DocumentSummarizer(DocumentSummarizer&&)                 noexcept = default;
    DocumentSummarizer& operator=(DocumentSummarizer&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Single-document summarization
    // -----------------------------------------------------------------------

    /**
     * @brief Summarize a single document
     *
     * @param document_id  Identifier for attribution / tracking
     * @param content      Raw document text
     * @param query        Optional query; improves sentence selection in
     *                     EXTRACTIVE mode and prompt focus in ABSTRACTIVE mode
     * @return Per-document summary
     */
    DocumentSummary summarize(const std::string& document_id,
                              const std::string& content,
                              const std::string& query = "") const;

    // -----------------------------------------------------------------------
    // Multi-document summarization
    // -----------------------------------------------------------------------

    /**
     * @brief Summarize multiple retrieved documents into a single context
     *
     * Processes each document individually, then concatenates the results
     * into a combined summary that respects @c max_summary_chars.  In
     * ABSTRACTIVE mode a single batched LLM call is used to produce a
     * coherent cross-document summary.
     *
     * @param documents  Documents from the re-ranking / retrieval stage
     * @param query      Original query (improves relevance scoring)
     * @return Combined summary with per-document breakdown and statistics
     */
    MultiDocumentSummary summarizeMultiple(
        const std::vector<judge::RetrievedDocument>& documents,
        const std::string& query = "") const;

    /**
     * @brief Summarize multiple streamed documents into a single context
     *
     * Overload compatible with StreamingRetriever output.
     *
     * @param documents  Documents from StreamingRetriever::stream()
     * @param query      Original query
     * @return Combined summary
     */
    MultiDocumentSummary summarizeMultiple(
        const std::vector<streaming::StreamedDocument>& documents,
        const std::string& query = "") const;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /** @brief Return the active configuration */
    const DocumentSummarizerConfig& getConfig() const;

    /** @brief Replace the active configuration */
    void setConfig(const DocumentSummarizerConfig& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// Factory helpers
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for common DocumentSummarizer configurations
 */
class DocumentSummarizerFactory {
public:
    /**
     * @brief Fast extractive summarizer (no LLM required)
     * @param max_sentences Maximum sentences extracted per document
     */
    static std::unique_ptr<DocumentSummarizer> createExtractive(
        size_t max_sentences = 5);

    /**
     * @brief Abstractive summarizer backed by the configured LLM engine
     * @param max_summary_chars Maximum characters in the combined output
     */
    static std::unique_ptr<DocumentSummarizer> createAbstractive(
        size_t max_summary_chars = 2000);

    /**
     * @brief Auto-selecting summarizer (abstractive when LLM is available)
     */
    static std::unique_ptr<DocumentSummarizer> createAuto();
};

} // namespace themis::rag

