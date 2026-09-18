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
  struct Config {
    float k1 = 1.5f;     ///< Term frequency saturation parameter (default: 1.5)
    float b = 0.75f;     ///< Length normalization parameter (default: 0.75)
  };
  
  BM25Scorer();
  /**
   * @brief BM25 Scorer.
   * @param[in] config Input parameter.
   * @return Return value.
   */
  explicit BM25Scorer(const Config& config);
  
  /**
   * @brief Compute.
   * @param[in] doc_id Identifier of the doc.
   * @param[in] query Input parameter.
   * @param[in] index_stats Input parameter.
   * @return Return value.
   */
  float compute(
      uint64_t doc_id,
      const SearchNode& query,
      const IndexStatistics& index_stats) const;
  
  /**
   * @brief Compute IDF.
   * @param[in] doc_freq Input parameter.
   * @param[in] total_docs Input parameter.
   * @return Return value.
   */
  static float computeIDF(
      uint32_t doc_freq,
      uint32_t total_docs);
  
  /**
   * @brief Compute TFComponent.
   * @param[in] term_freq Input parameter.
   * @param[in] doc_length Input parameter.
   * @param[in] avg_doc_length Input parameter.
   * @return Return value.
   */
  float computeTFComponent(
      uint32_t term_freq,
      uint32_t doc_length,
      float avg_doc_length) const;
  
 private:
  Config config_;
};

}  // namespace themis::query::fts
