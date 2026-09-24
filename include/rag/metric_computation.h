// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace themis::rag {

/// @brief Information retrieval metric computation.
///
/// Computes standard IR metrics (NDCG, MRR, MAP, Precision, Recall) for
/// evaluating retrieval and reranking quality. Supports both graded and
/// binary relevance judgments.
///
/// @details
/// Metrics:
/// - NDCG@K: Normalized Discounted Cumulative Gain (standard in TREC)
/// - MRR@K: Mean Reciprocal Rank (for finding first relevant doc)
/// - MAP@K: Mean Average Precision (average of precisions at each relevant doc)
/// - Precision@K: % of top K results that are relevant
/// - Recall@K: % of all relevant docs found in top K
///
/// Relevance models:
/// - Binary: relevant/not-relevant (0/1)
/// - Graded: 0-3 scale (TREC standard)
///   - 0: Not relevant
///   - 1: Marginally relevant
///   - 2: Highly relevant
///   - 3: Perfectly relevant
/// - Custom: User-provided relevance function
///
/// Computation:
/// - DCG@K: Sum(rel_i / log2(i+1)) for i=1 to K
/// - IDCG@K: DCG@K of perfect ranking
/// - NDCG@K: DCG@K / IDCG@K
///
/// @code
/// auto computer = std::make_unique<MetricComputer>();
/// computer->SetRelevanceModel("graded", 0, 3);  // 0-3 scale
/// 
/// std::vector<std::string> ranked_docs = {"d1", "d2", "d3"};
/// std::map<std::string, uint32_t> relevance = {
///   {"d1", 3}, {"d2", 2}, {"d3", 0}
/// };
/// 
/// float ndcg = computer->ComputeNDCG(ranked_docs, relevance, 10);
/// float mrr = computer->ComputeMRR(ranked_docs, relevance, 10);
/// @endcode
class MetricComputer {
 public:
  /// @brief Metric result.
  struct MetricResult {
    float ndcg_10;
    float ndcg_100;
    float mrr_10;
    float mrr_100;
    float map_10;
    float map_100;
    float precision_10;
    float recall_10;
    float precision_100;
    float recall_100;
  };

  /// @brief Relevance model.
  enum class RelevanceModel {
    Binary,  ///< 0/1 judgments
    Graded,  ///< 0-N scale (user-specified)
    Custom   ///< User callback function
  };

  /// @brief Constructor.
  /// @param model Relevance model type.
  /// @param min_grade Minimum grade (for graded model).
  /// @param max_grade Maximum grade (for graded model).
  explicit MetricComputer(
      RelevanceModel model = RelevanceModel::Graded,
      uint32_t min_grade = 0,
      uint32_t max_grade = 3);

  /// @brief Compute NDCG@K.
  /// @param ranked_doc_ids Ranked document IDs (best to worst).
  /// @param relevance_judgments Map of doc_id → relevance grade.
  /// @param k Cutoff (typically 10 or 100).
  /// @return NDCG@K score [0, 1].
  float ComputeNDCG(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

  /// @brief Compute MRR@K.
  /// @param ranked_doc_ids Ranked document IDs.
  /// @param relevance_judgments Relevance map.
  /// @param k Cutoff.
  /// @return MRR@K score [0, 1].
  float ComputeMRR(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

  /// @brief Compute MAP@K.
  /// @param ranked_doc_ids Ranked document IDs.
  /// @param relevance_judgments Relevance map.
  /// @param k Cutoff.
  /// @return MAP@K score [0, 1].
  float ComputeMAP(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

  /// @brief Compute Precision@K.
  /// @param ranked_doc_ids Ranked document IDs.
  /// @param relevance_judgments Relevance map.
  /// @param k Cutoff.
  /// @return Precision@K score [0, 1].
  float ComputePrecision(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

  /// @brief Compute Recall@K.
  /// @param ranked_doc_ids Ranked document IDs.
  /// @param relevance_judgments Relevance map.
  /// @param k Cutoff.
  /// @return Recall@K score [0, 1].
  float ComputeRecall(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

  /// @brief Compute all metrics.
  /// @param ranked_doc_ids Ranked document IDs.
  /// @param relevance_judgments Relevance map.
  /// @return Struct with all metric values.
  MetricResult ComputeAll(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments);

  /// @brief Set custom relevance function.
  /// @param fn Function: doc_id, doc_content -> relevance grade.
  void SetCustomRelevanceFunction(
      std::function<uint32_t(const std::string&, const std::string&)> fn);

  /// @brief Compute idear DCG (for NDCG normalization).
  /// @param relevance_judgments All relevant documents with grades.
  /// @param k Cutoff.
  /// @return Ideal DCG@K.
  float ComputeIdealDCG(
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

 private:
  float ComputeDCG(
      const std::vector<std::string>& ranked_doc_ids,
      const std::map<std::string, uint32_t>& relevance_judgments,
      uint32_t k);

  RelevanceModel model_;
  uint32_t min_grade_;
  uint32_t max_grade_;
  std::function<uint32_t(const std::string&, const std::string&)> custom_fn_;
};

}  // namespace themis::rag
