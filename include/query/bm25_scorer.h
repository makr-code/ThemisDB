// ============================================================================
// include/query/bm25_scorer.h
// ============================================================================
// BM25 Scoring Algorithm Implementation
// Okapi BM25 ranking function for relevance scoring
//
// Formula:
//   score(D, Q) = Σ(i=1 to n) IDF(qi) * (tf(qi, D) * (k1 + 1)) / 
//                                      (tf(qi, D) + k1 * (1 - b + b * |D| / avgDL))
//
// Parameters:
//   k1 = 1.5  (controls term frequency saturation)
//   b  = 0.75 (controls length normalization)
//
// Thread-Safety: NOT thread-safe (immutable after construction)
//   - All members const after construction
//   - Safe to share const pointer across threads
//
// ============================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "query/fts_parser.h"  // SearchNode AST
#include "query/fts_index.h"   // Index statistics

namespace themis::query::fts {

class BM25Scorer {
 public:
  /// Configuration for BM25 algorithm
  struct Config {
    float k1 = 1.5f;     ///< Term frequency saturation parameter (default: 1.5)
    float b = 0.75f;     ///< Length normalization parameter (default: 0.75)
  };
  
  /// @brief Construct a BM25 scorer with configurable tuning parameters.
  /// @param config: BM25 algorithm parameters (k1, b)
  BM25Scorer();
  /**
   * @brief TBD: Describe BM25Scorer.
   * @param[in] config Input parameter.
   * @return Return value.
   */
  explicit BM25Scorer(const Config& config);
  
  /**
   * @brief @brief Compute the BM25 score for one document-query match.
   * @param[in] doc_id Input parameter.
   * @param[in] query Input parameter.
   * @param[in] index_stats Input parameter.
   * @return Return value.
   * @details @param doc_id: document identifier @param query: SearchNode AST (already parsed by FtsParser) @param index_stats: index statistics (document count, term frequencies) @return BM25 score ≥ 0, or error @note Thread safety: yes (const method, no state mutation). @note: score = 0 if term not in index or document not matched
   */
  float compute(
      uint64_t doc_id,
      const SearchNode& query,
      const IndexStatistics& index_stats) const;
  
  /**
   * @brief @brief Compute inverse document frequency (IDF) for BM25.
   * @param[in] doc_freq Input parameter.
   * @param[in] total_docs Input parameter.
   * @return Return value.
   * @details @param doc_freq: number of documents containing this term @param total_docs: total number of documents in index @return IDF score ≥ 0 @note Formula: log((N - df + 0.5) / (df + 0.5))
   */
  static float computeIDF(
      uint32_t doc_freq,
      uint32_t total_docs);
  
  /**
   * @brief @brief Compute the TF contribution used by BM25 scoring.
   * @param[in] term_freq Input parameter.
   * @param[in] doc_length Input parameter.
   * @param[in] avg_doc_length Input parameter.
   * @return Return value.
   * @details @param term_freq: raw term frequency in document @param doc_length: document length in tokens @param avg_doc_length: average document length in corpus @return TF component [0, ∞)
   */
  float computeTFComponent(
      uint32_t term_freq,
      uint32_t doc_length,
      float avg_doc_length) const;
  
 private:
  Config config_;
};

}  // namespace themis::query::fts
