/**
 * @file citation_highlighter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    CitationHighlighter(CitationHighlighter&&)                 noexcept = default;
    CitationHighlighter& operator=(CitationHighlighter&&)      noexcept = default;

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
