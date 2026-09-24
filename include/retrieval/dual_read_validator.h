/**
 * @file dual_read_validator.h
 * @brief Query-time validation and metric collection for embedding version canary
 *
 * Executes queries against both current and previous index versions and compares
 * retrieval quality metrics to detect regressions before canary rollout.
 *
 * **Metrics Tracked:**
 * - Recall@10: Fraction of relevant docs in top-10 results
 * - nDCG@10: Normalized Discounted Cumulative Gain@10
 * - MRR@10: Mean Reciprocal Rank@10
 *
 * **Tolerance:** 2pp (percentage point) delta allowed
 * - ✅ Accept: (prev_metric - current_metric) <= 2pp
 * - ❌ Reject: (prev_metric - current_metric) > 2pp (regression detected)
 *
 * @date 2026-09-24
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace themis::retrieval {

/**
 * @struct RetrievalResult
 * @brief Single retrieval result from index query
 */
struct RetrievalResult {
  std::string document_id;  ///< Document ID returned by index
  double relevance_score;   ///< Relevance score (0.0-1.0)
  int rank;                 ///< Position in result set (1-based)

  bool operator==(const RetrievalResult& other) const {
    return document_id == other.document_id;
  }
};

/**
 * @struct RetrievalMetrics
 * @brief Quality metrics for a query result set
 */
struct RetrievalMetrics {
  double recall_at_10{0.0};   ///< Recall@10 (0.0-1.0)
  double ndcg_at_10{0.0};     ///< nDCG@10 (0.0-1.0, normalized)
  double mrr_at_10{0.0};      ///< MRR@10 (Mean Reciprocal Rank)
  size_t relevant_count{0};   ///< Number of relevant docs retrieved
  size_t total_relevant{0};   ///< Total relevant docs in corpus (for recall)

  /**
   * @brief Check if any metric regressed by more than tolerance
   * @param other Baseline metrics to compare against
   * @param tolerance_pp Tolerance in percentage points (e.g., 2.0 for 2pp)
   * @return true if regression detected, false otherwise
   */
  bool HasRegression(const RetrievalMetrics& other, double tolerance_pp = 2.0) const;

  /**
   * @brief Compute absolute delta for each metric
   * @param other Baseline metrics
   * @return Vector of deltas: [delta_recall, delta_ndcg, delta_mrr]
   */
  std::vector<double> ComputeDeltas(const RetrievalMetrics& other) const;
};

/**
 * @class DualReadValidator
 * @brief Compares query results between current and previous index versions
 *
 * Usage:
 * ```cpp
 * DualReadValidator validator(current_index, previous_index);
 * auto query = "search term";
 * auto [current_results, previous_results, metrics] = validator.ValidateQuery(query);
 *
 * if (metrics.HasRegression(baseline, 2.0)) {
 *   // Regression detected, abort canary upgrade
 * }
 * ```
 */
class DualReadValidator {
 public:
  /**
   * @brief Create a dual-read validator
   * @param current_index_path Path to current index
   * @param previous_index_path Path to previous (baseline) index
   */
  DualReadValidator(const std::string& current_index_path,
                    const std::string& previous_index_path);

  ~DualReadValidator();

  // Delete copy operations
  DualReadValidator(const DualReadValidator&) = delete;
  DualReadValidator& operator=(const DualReadValidator&) = delete;

  /**
   * @brief Validate a single query against both index versions
   *
   * Executes the same query on both indices and computes quality metrics.
   *
   * @param query Search query
   * @param top_k Number of results to retrieve (typically 10)
   * @return Tuple of (current_results, previous_results, metrics)
   * @throws std::runtime_error if query execution fails
   */
  struct ValidationResult {
    std::vector<RetrievalResult> current_results;
    std::vector<RetrievalResult> previous_results;
    RetrievalMetrics metrics;
  };

  ValidationResult ValidateQuery(const std::string& query, size_t top_k = 10);

  /**
   * @brief Validate a batch of queries and aggregate metrics
   *
   * Executes multiple queries and computes aggregate metrics.
   *
   * @param queries Vector of search queries
   * @param top_k Number of results per query
   * @return Aggregated metrics across all queries
   * @throws std::runtime_error if any query fails
   */
  struct BatchValidationResult {
    std::vector<ValidationResult> query_results;
    RetrievalMetrics aggregate_metrics;
    std::chrono::milliseconds total_time{0};
  };

  BatchValidationResult ValidateQueryBatch(const std::vector<std::string>& queries,
                                            size_t top_k = 10);

  /**
   * @brief Get ground truth relevance judgments for a query
   *
   * Required for computing recall and nDCG metrics.
   *
   * @param query Search query
   * @return Set of document IDs considered relevant for this query
   */
  std::vector<std::string> GetGroundTruth(const std::string& query) const;

  /**
   * @brief Set ground truth judgments (for testing)
   * @param query_judgments Map of query → relevant doc IDs
   */
  void SetGroundTruth(
      const std::map<std::string, std::vector<std::string>>& query_judgments);

 private:
  std::string current_index_path_;
  std::string previous_index_path_;
  std::map<std::string, std::vector<std::string>> ground_truth_cache_;

  /**
   * @brief Execute a query on a specific index
   * @param index_path Path to index
   * @param query Search query
   * @param top_k Number of results
   * @return Retrieval results sorted by score
   */
  std::vector<RetrievalResult> ExecuteQuery(const std::string& index_path,
                                             const std::string& query, size_t top_k);

  /**
   * @brief Compute metrics for a result set against ground truth
   * @param results Query results
   * @param ground_truth Relevant document IDs
   * @return RetrievalMetrics for this result set
   */
  RetrievalMetrics ComputeMetrics(const std::vector<RetrievalResult>& results,
                                  const std::vector<std::string>& ground_truth);

  /**
   * @brief Compute recall@k
   * @param results Results (must be sorted by rank)
   * @param ground_truth Relevant docs
   * @param k Cutoff rank (typically 10)
   * @return Recall@k (fraction of relevant docs retrieved)
   */
  static double ComputeRecallAtK(const std::vector<RetrievalResult>& results,
                                 const std::vector<std::string>& ground_truth, int k);

  /**
   * @brief Compute nDCG@k (normalized DCG)
   * @param results Results (must be sorted by rank)
   * @param ground_truth Relevant docs
   * @param k Cutoff rank (typically 10)
   * @return Normalized DCG@k (0.0-1.0)
   */
  static double ComputeNdcgAtK(const std::vector<RetrievalResult>& results,
                               const std::vector<std::string>& ground_truth, int k);

  /**
   * @brief Compute MRR@k (mean reciprocal rank)
   * @param results Results (must be sorted by rank)
   * @param ground_truth Relevant docs
   * @param k Cutoff rank (typically 10)
   * @return MRR@k (reciprocal of rank of first relevant doc)
   */
  static double ComputeMrrAtK(const std::vector<RetrievalResult>& results,
                              const std::vector<std::string>& ground_truth, int k);
};

}  // namespace themis::retrieval
