/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            citation_highlighter.h                             ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:08:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     281                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file citation_highlighter.h
 * @brief Citation highlighting: map answer sentences to source chunks
 *
 * Maps each sentence of a generated answer back to the source document
 * chunk(s) that most strongly support it, enabling explainable RAG and
 * grounding of generated content.
 *
 * Algorithm (heuristic, no external model required):
 *  1. Split the answer into sentences using punctuation boundaries.
 *  2. For each sentence compute a term-overlap (Jaccard) similarity against
 *     every source chunk.
 *  3. Select the chunk with the highest similarity above the configured
 *     @c min_similarity_threshold as the primary citation.
 *  4. Optionally collect secondary citations above a lower secondary threshold.
 *
 * When an LLM inference engine is wired via @c LLMIntegration the
 * implementation can fall back to a semantic-similarity path.  Without an
 * engine the pure term-overlap path is used and produces deterministic,
 * testable results.
 *
 * Integration:
 * @code
 *   #include "rag/citation_highlighter.h"
 *   using namespace themis::rag;
 *
 *   CitationHighlighter highlighter;
 *
 *   std::vector<SourceChunk> chunks = {
 *       {"doc1", 0, "Paris is the capital of France."},
 *       {"doc2", 0, "The Eiffel Tower was built in 1889."},
 *   };
 *
 *   auto result = highlighter.highlight(
 *       "Paris is France's capital. The tower dates to 1889.",
 *       chunks);
 *
 *   for (const auto& mapping : result.mappings) {
 *       std::cout << "[" << mapping.answer_sentence << "]\n";
 *       std::cout << "  → " << mapping.primary_chunk_id
 *                 << " (score=" << mapping.similarity_score << ")\n";
 *   }
 * @endcode
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace themis::rag {

// ---------------------------------------------------------------------------
// Supporting types
// ---------------------------------------------------------------------------

/**
 * @brief A single chunk of source text to map answer sentences against
 */
struct SourceChunk {
    std::string doc_id;       ///< Identifier of the parent document
    size_t      chunk_index;  ///< Zero-based chunk index within the document
    std::string content;      ///< Full text of the chunk
    /// Optional free-form metadata (e.g. page number, section heading)
    std::string metadata;
};

/**
 * @brief Mapping of one answer sentence to its best-matching source chunk(s)
 */
struct SentenceCitationMapping {
    /// The answer sentence (leading/trailing whitespace stripped)
    std::string answer_sentence;
    /// Index of this sentence in the original answer (0-based)
    size_t      sentence_index = 0;

    /// doc_id of the primary (best-matching) source chunk.
    /// Empty string when no chunk exceeded the minimum threshold.
    std::string primary_chunk_id;
    /// chunk_index of the primary source chunk
    size_t      primary_chunk_index = 0;
    /// Similarity score for the primary chunk [0, 1]
    double      similarity_score    = 0.0;

    /// Additional chunks that also provide supporting evidence
    struct SecondarySource {
        std::string doc_id;
        size_t      chunk_index;
        double      similarity_score;
    };
    std::vector<SecondarySource> secondary_sources;

    /// True when at least one chunk exceeded the minimum similarity threshold
    bool has_citation() const { return !primary_chunk_id.empty(); }
};

/**
 * @brief Full citation-highlight result for one answer
 */
struct CitationHighlightResult {
    /// Per-sentence citation mappings (same order as sentences in the answer)
    std::vector<SentenceCitationMapping> mappings;

    /// Fraction of answer sentences that received at least one citation [0, 1]
    double citation_coverage = 0.0;

    /// Overall mean similarity across all cited sentences
    double mean_similarity = 0.0;

    /// Elapsed time for the highlight operation (milliseconds)
    double highlight_time_ms = 0.0;
};

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for CitationHighlighter
 */
struct CitationHighlighterConfig {
    /**
     * @brief Minimum term-overlap similarity [0, 1] for a primary citation.
     *
     * Sentences whose best-matching chunk scores below this threshold are
     * reported with an empty @c primary_chunk_id.
     */
    double min_similarity_threshold = 0.15;

    /**
     * @brief Minimum similarity for secondary (supporting) citations.
     *
     * Must be ≤ @c min_similarity_threshold.  Set to 0.0 to disable
     * secondary citations.
     */
    double secondary_similarity_threshold = 0.08;

    /**
     * @brief Maximum number of secondary citations to attach per sentence.
     *
     * 0 means no secondary citations are collected.
     */
    size_t max_secondary_citations = 3;

    /**
     * @brief Sentence split character set.
     *
     * Sentences are split on these characters followed by whitespace or
     * end-of-string.  Default: '.', '!', '?'
     */
    std::string sentence_delimiters = ".!?";

    /**
     * @brief Minimum sentence length (characters) to consider for citation.
     *
     * Very short sentences (e.g. "OK.") are skipped.
     */
    size_t min_sentence_length = 5;
};

// ---------------------------------------------------------------------------
// Main class
// ---------------------------------------------------------------------------

/**
 * @brief Citation highlighter: maps answer sentences to source chunks
 *
 * Thread-safe: const methods (@c highlight, @c splitSentences,
 * @c computeSimilarity) may be called from multiple threads concurrently.
 * Configuration updates via @c setConfig are guarded by a mutex.
 */
class CitationHighlighter {
public:
    explicit CitationHighlighter(
        CitationHighlighterConfig config = CitationHighlighterConfig{});
    ~CitationHighlighter();

    // Non-copyable, movable
    CitationHighlighter(const CitationHighlighter&)            = delete;
    CitationHighlighter& operator=(const CitationHighlighter&) = delete;
    CitationHighlighter(CitationHighlighter&&)                 = default;
    CitationHighlighter& operator=(CitationHighlighter&&)      = default;

    /**
     * @brief Map every sentence in @p answer to the best-matching source chunk
     *
     * @param answer   Generated answer text
     * @param chunks   Source chunks retrieved for this query
     * @return         Per-sentence citation mappings and aggregate statistics
     */
    CitationHighlightResult highlight(
        const std::string&              answer,
        const std::vector<SourceChunk>& chunks) const;

    /**
     * @brief Split text into sentences using the configured delimiters
     *
     * @param text Input text
     * @return     Trimmed, non-empty sentence strings
     */
    std::vector<std::string> splitSentences(const std::string& text) const;

    /**
     * @brief Compute Jaccard term-overlap similarity between two strings
     *
     * Tokenises both strings into lower-cased word tokens (≥ 2 chars),
     * then returns |intersection| / |union|.
     *
     * @param a First text
     * @param b Second text
     * @return  Similarity in [0, 1]
     */
    static double computeSimilarity(const std::string& a,
                                    const std::string& b);

    /**
     * @brief Return current configuration
     */
    CitationHighlighterConfig getConfig() const;

    /**
     * @brief Update configuration
     * @param config New configuration to apply
     */
    void setConfig(const CitationHighlighterConfig& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for common CitationHighlighter configurations
 */
class CitationHighlighterFactory {
public:
    /**
     * @brief Strict mode: only very strong matches are cited (threshold=0.30)
     */
    static std::unique_ptr<CitationHighlighter> createStrict();

    /**
     * @brief Balanced mode: default thresholds (threshold=0.15)
     */
    static std::unique_ptr<CitationHighlighter> createBalanced();

    /**
     * @brief Permissive mode: even weak overlaps are cited (threshold=0.05)
     */
    static std::unique_ptr<CitationHighlighter> createPermissive();
};

} // namespace themis::rag
