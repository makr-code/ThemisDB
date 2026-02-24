/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            citation_highlighter.h                             ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     237                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file citation_highlighter.h
 * @brief Citation highlighting: map answer sentences to source chunks
 *
 * Given a generated answer and a set of retrieved source document chunks,
 * CitationHighlighter splits the answer into sentences and maps each
 * sentence to the source chunk(s) that best support it using a
 * calibrated term-overlap heuristic.
 *
 * Architecture:
 *   Generated Answer
 *     ↓
 *   Sentence Splitter (punctuation-aware)
 *     ↓
 *   Term-Overlap Scorer (per sentence × per chunk)
 *     ↓
 *   SentenceChunkMapping list
 *     ↓
 *   CitationHighlightResult (mappings + coverage metrics)
 *
 * The heuristic scorer is intentionally dependency-free and runs in
 * O(S × C × T) where S = sentence count, C = chunk count, T = average
 * token count.  No external model files are required.
 *
 * Usage example:
 * @code
 *   using namespace themis::rag;
 *
 *   CitationHighlighter highlighter;
 *
 *   std::vector<SourceChunk> chunks = {
 *       {"doc1", "Paris is the capital of France."},
 *       {"doc2", "The Eiffel Tower was built in 1889."}
 *   };
 *
 *   auto result = highlighter.highlight(
 *       "Paris is the capital of France. The tower was constructed in 1889.",
 *       chunks
 *   );
 *
 *   for (const auto& m : result.mappings) {
 *       std::cout << "[" << m.sentence_index << "] \""
 *                 << m.sentence_text << "\"\n"
 *                 << "  -> " << m.chunk_id
 *                 << " (score=" << m.support_score << ")\n";
 *   }
 * @endcode
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace themis::rag {

/**
 * @brief A retrieved source chunk provided to the highlighter
 */
struct SourceChunk {
    std::string id;       ///< Unique chunk identifier (e.g., "doc1", "chunk_42")
    std::string content;  ///< Text content of the chunk
};

/**
 * @brief Mapping from one answer sentence to one supporting source chunk
 */
struct SentenceChunkMapping {
    size_t      sentence_index; ///< 0-based index of the sentence in the answer
    std::string sentence_text;  ///< Trimmed sentence text
    std::string chunk_id;       ///< ID of the matched source chunk
    std::string chunk_text;     ///< Content of the matched source chunk
    double      support_score;  ///< Term-overlap similarity in [0, 1]
};

/**
 * @brief Result of a citation highlighting pass
 */
struct CitationHighlightResult {
    /// All sentence-to-chunk mappings, ordered by sentence_index then
    /// descending support_score.
    std::vector<SentenceChunkMapping> mappings;

    /// All sentences extracted from the answer (including unmapped ones).
    std::vector<std::string> sentences;

    /// Fraction of non-empty sentences that have at least one mapping.
    /// 0.0 when there are no non-empty sentences.
    double coverage;

    /// Wall-clock time for the full highlighting pass.
    std::chrono::milliseconds elapsed_ms{0};
};

/**
 * @brief Configuration for CitationHighlighter
 */
struct CitationHighlighterConfig {
    /// Minimum term-overlap score [0, 1] required to form a mapping.
    /// Sentences below this threshold are left unmapped.
    double min_support_score = 0.1;

    /// Maximum number of source chunks to map to a single sentence.
    /// Set to 1 for a one-to-one mapping; 0 means no limit.
    size_t max_chunks_per_sentence = 3;

    /// Minimum sentence length in characters.  Very short fragments
    /// (e.g., isolated words) are skipped.
    size_t min_sentence_length = 5;
};

/**
 * @brief Maps answer sentences to their supporting source chunks
 *
 * CitationHighlighter is the entry point for Phase 3 citation highlighting.
 * It is intentionally lightweight: no LLM, no external model, no network I/O.
 *
 * Thread-safety: highlight() and the const accessors are safe to call from
 * multiple threads; the configuration is set at construction time and is
 * immutable thereafter.
 */
class CitationHighlighter {
public:
    /**
     * @brief Construct with default configuration
     */
    CitationHighlighter();

    /**
     * @brief Construct with custom configuration
     * @param config Highlighter configuration
     * @throws std::invalid_argument if config values are out of range
     */
    explicit CitationHighlighter(const CitationHighlighterConfig& config);

    /**
     * @brief Destructor
     */
    ~CitationHighlighter();

    /**
     * @brief Map answer sentences to source chunks
     *
     * For each sentence in @p answer the method finds the source chunk(s)
     * in @p chunks whose content best overlaps with that sentence and whose
     * support score meets the configured threshold.
     *
     * @param answer  Generated answer text (may contain multiple sentences)
     * @param chunks  Ordered list of retrieved source chunks
     * @return        Citation highlight result with mappings and coverage
     */
    CitationHighlightResult highlight(
        const std::string& answer,
        const std::vector<SourceChunk>& chunks
    ) const;

    /**
     * @brief Split text into sentences
     *
     * Splits on `.`, `!`, and `?` followed by whitespace or end-of-string,
     * trims leading/trailing whitespace, and drops empty fragments.
     *
     * @param text  Input text
     * @return      Vector of trimmed sentence strings
     */
    static std::vector<std::string> splitSentences(const std::string& text);

    /**
     * @brief Compute term-overlap similarity between a sentence and a chunk
     *
     * Uses a TF-IDF-inspired unigram + bigram weighted overlap fraction
     * consistent with the heuristic in CrossEncoderReranker.  The result
     * is in [0, 1]; higher means more overlap.
     *
     * @param sentence  Answer sentence
     * @param chunk     Source chunk text
     * @return          Overlap similarity score in [0, 1]
     */
    static double scoreSentenceChunk(
        const std::string& sentence,
        const std::string& chunk
    );

    /**
     * @brief Return current configuration
     */
    const CitationHighlighterConfig& getConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Factory helpers for common citation highlighter configurations
 */
class CitationHighlighterFactory {
public:
    /**
     * @brief Strict highlighter: only high-confidence mappings (score >= 0.3)
     */
    static std::unique_ptr<CitationHighlighter> createStrict();

    /**
     * @brief Balanced highlighter: default thresholds (score >= 0.1)
     */
    static std::unique_ptr<CitationHighlighter> createBalanced();

    /**
     * @brief Permissive highlighter: all non-zero overlaps are mapped
     *        (score > 0.0); useful for debugging attribution.
     */
    static std::unique_ptr<CitationHighlighter> createPermissive();
};

} // namespace themis::rag
