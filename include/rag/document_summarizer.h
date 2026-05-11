/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            document_summarizer.h                              ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:46:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     289                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file document_summarizer.h
 * @brief Multi-document summarization before context injection (RAG Phase 3)
 *
 * Provides DocumentSummarizer for condensing multiple retrieved documents
 * into a compact summary before they are injected into the LLM context
 * window.  This reduces token consumption and keeps the most relevant
 * information front and center.
 *
 * Two summarization strategies are supported:
 *  - **EXTRACTIVE**: selects and concatenates the most query-relevant
 *    sentences from each document.  No LLM required; <2 ms for 10 documents.
 *  - **ABSTRACTIVE**: calls the configured LLM via LLMIntegration to
 *    produce a fluent, compressed summary.  Requires a connected LLM engine.
 *  - **AUTO**: uses ABSTRACTIVE when a LLM engine is available, falls back
 *    to EXTRACTIVE automatically.
 *
 * Integration with existing RAG components:
 * @code
 *   // After reranking, before building the final prompt:
 *   DocumentSummarizer summarizer;
 *   auto summary = summarizer.summarizeMultiple(rerank_result.documents, query);
 *   // summary.combined_summary is ready for injection into the LLM prompt
 * @endcode
 *
 * Compatible document types:
 *  - themis::rag::judge::RetrievedDocument  (from rag_judge.h)
 *  - themis::rag::streaming::StreamedDocument (from streaming_retriever.h)
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

    /// Whether to prepend a "[Source: &lt;id&gt;]" tag to each per-document
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
    DocumentSummarizer(DocumentSummarizer&&)                 = default;
    DocumentSummarizer& operator=(DocumentSummarizer&&)      = default;

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
