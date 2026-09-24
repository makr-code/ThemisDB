/**
 * @file dual_read_validator.cpp
 * @brief Implementation of DualReadValidator for embedding version canary
 *
 * Compares retrieval quality metrics between index versions using precision,
 * recall, and ranking-based metrics (nDCG, MRR).
 *
 * @date 2026-09-24
 */

#include "retrieval/dual_read_validator.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace themis::retrieval {

// ============================================================================
// RetrievalMetrics Implementation
// ============================================================================

bool RetrievalMetrics::HasRegression(const RetrievalMetrics& other,
                                     double tolerance_pp) const {
  // Convert tolerance from percentage points to fraction (e.g., 2pp → 0.02)
  double tolerance = tolerance_pp / 100.0;

  // Check each metric: if previous is better than current by more than tolerance,
  // it's a regression
  double recall_delta = other.recall_at_10 - recall_at_10;
  double ndcg_delta = other.ndcg_at_10 - ndcg_at_10;
  double mrr_delta = other.mrr_at_10 - mrr_at_10;

  return recall_delta > tolerance || ndcg_delta > tolerance || mrr_delta > tolerance;
}

std::vector<double> RetrievalMetrics::ComputeDeltas(const RetrievalMetrics& other) const {
  return {other.recall_at_10 - recall_at_10, other.ndcg_at_10 - ndcg_at_10,
          other.mrr_at_10 - mrr_at_10};
}

// ============================================================================
// DualReadValidator Implementation
// ============================================================================

DualReadValidator::DualReadValidator(const std::string& current_index_path,
                                     const std::string& previous_index_path)
    : current_index_path_(current_index_path),
      previous_index_path_(previous_index_path) {
  spdlog::info(
      "[DualReadValidator] Initialized with current={}, previous={}", current_index_path,
      previous_index_path);
}

DualReadValidator::~DualReadValidator() = default;

double DualReadValidator::ComputeRecallAtK(const std::vector<RetrievalResult>& results,
                                            const std::vector<std::string>& ground_truth,
                                            int k) {
  if (ground_truth.empty()) {
    return 0.0;
  }

  int found = 0;
  for (int i = 0; i < std::min(k, static_cast<int>(results.size())); ++i) {
    const auto& result = results[i];
    if (std::find(ground_truth.begin(), ground_truth.end(), result.document_id) !=
        ground_truth.end()) {
      found++;
    }
  }

  return static_cast<double>(found) / static_cast<double>(ground_truth.size());
}

double DualReadValidator::ComputeNdcgAtK(const std::vector<RetrievalResult>& results,
                                         const std::vector<std::string>& ground_truth,
                                         int k) {
  if (ground_truth.empty()) {
    return 0.0;
  }

  // Compute DCG@k
  double dcg = 0.0;
  for (int i = 0; i < std::min(k, static_cast<int>(results.size())); ++i) {
    const auto& result = results[i];
    bool is_relevant = std::find(ground_truth.begin(), ground_truth.end(),
                                 result.document_id) != ground_truth.end();

    if (is_relevant) {
      // DCG formula: relevance / log2(rank + 1)
      double discount = std::log2(i + 2);  // rank is 1-based, so i+2 for log
      dcg += 1.0 / discount;
    }
  }

  // Compute IDCG@k (ideal DCG with all relevant docs at top)
  double idcg = 0.0;
  int ideal_count = std::min(k, static_cast<int>(ground_truth.size()));
  for (int i = 0; i < ideal_count; ++i) {
    double discount = std::log2(i + 2);
    idcg += 1.0 / discount;
  }

  if (idcg == 0.0) {
    return 0.0;
  }

  return dcg / idcg;
}

double DualReadValidator::ComputeMrrAtK(const std::vector<RetrievalResult>& results,
                                        const std::vector<std::string>& ground_truth,
                                        int k) {
  for (int i = 0; i < std::min(k, static_cast<int>(results.size())); ++i) {
    const auto& result = results[i];
    if (std::find(ground_truth.begin(), ground_truth.end(), result.document_id) !=
        ground_truth.end()) {
      // Found first relevant result at rank i+1
      return 1.0 / static_cast<double>(i + 1);
    }
  }

  // No relevant results found
  return 0.0;
}

std::vector<RetrievalResult> DualReadValidator::ExecuteQuery(const std::string& index_path,
                                                              const std::string& query,
                                                              size_t top_k) {
  // Placeholder implementation for testing
  // In production, this would interface with the actual index retrieval system
  spdlog::debug("[DualReadValidator] Executing query on index: {} (top_k={})", index_path,
                top_k);

  // For now, return empty results (to be implemented with actual index interface)
  return std::vector<RetrievalResult>();
}

RetrievalMetrics DualReadValidator::ComputeMetrics(const std::vector<RetrievalResult>& results,
                                                   const std::vector<std::string>& ground_truth) {
  RetrievalMetrics metrics;
  metrics.total_relevant = ground_truth.size();

  // Count relevant docs in results
  for (const auto& result : results) {
    if (std::find(ground_truth.begin(), ground_truth.end(), result.document_id) !=
        ground_truth.end()) {
      metrics.relevant_count++;
    }
  }

  // Compute quality metrics
  metrics.recall_at_10 = ComputeRecallAtK(results, ground_truth, 10);
  metrics.ndcg_at_10 = ComputeNdcgAtK(results, ground_truth, 10);
  metrics.mrr_at_10 = ComputeMrrAtK(results, ground_truth, 10);

  return metrics;
}

DualReadValidator::ValidationResult DualReadValidator::ValidateQuery(const std::string& query,
                                                                      size_t top_k) {
  ValidationResult result;

  // Get ground truth for this query
  auto ground_truth = GetGroundTruth(query);

  // Execute on both indices
  result.current_results = ExecuteQuery(current_index_path_, query, top_k);
  result.previous_results = ExecuteQuery(previous_index_path_, query, top_k);

  // Compute metrics (use previous results as baseline)
  result.metrics = ComputeMetrics(result.current_results, ground_truth);

  spdlog::debug(
      "[DualReadValidator] Query '{}': current_recall@10={:.3f}, "
      "previous_recall@10={:.3f}",
      query, result.metrics.recall_at_10,
      ComputeMetrics(result.previous_results, ground_truth).recall_at_10);

  return result;
}

DualReadValidator::BatchValidationResult DualReadValidator::ValidateQueryBatch(
    const std::vector<std::string>& queries, size_t top_k) {
  auto start = std::chrono::high_resolution_clock::now();
  BatchValidationResult result;

  for (const auto& query : queries) {
    try {
      result.query_results.push_back(ValidateQuery(query, top_k));
    } catch (const std::exception& e) {
      spdlog::error("[DualReadValidator] Failed to validate query '{}': {}", query, e.what());
      throw;
    }
  }

  // Aggregate metrics
  if (!result.query_results.empty()) {
    double total_recall = 0.0;
    double total_ndcg = 0.0;
    double total_mrr = 0.0;
    int count = result.query_results.size();

    for (const auto& vr : result.query_results) {
      total_recall += vr.metrics.recall_at_10;
      total_ndcg += vr.metrics.ndcg_at_10;
      total_mrr += vr.metrics.mrr_at_10;
    }

    result.aggregate_metrics.recall_at_10 = total_recall / count;
    result.aggregate_metrics.ndcg_at_10 = total_ndcg / count;
    result.aggregate_metrics.mrr_at_10 = total_mrr / count;
  }

  auto end = std::chrono::high_resolution_clock::now();
  result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  spdlog::info("[DualReadValidator] Validated {} queries in {}ms, recall@10={:.3f}",
               queries.size(), result.total_time.count(),
               result.aggregate_metrics.recall_at_10);

  return result;
}

std::vector<std::string> DualReadValidator::GetGroundTruth(const std::string& query) const {
  auto it = ground_truth_cache_.find(query);
  if (it != ground_truth_cache_.end()) {
    return it->second;
  }

  // If not in cache, return empty (to be loaded from external source in production)
  spdlog::warn("[DualReadValidator] No ground truth cached for query '{}'", query);
  return std::vector<std::string>();
}

void DualReadValidator::SetGroundTruth(
    const std::map<std::string, std::vector<std::string>>& query_judgments) {
  ground_truth_cache_ = query_judgments;
  spdlog::info("[DualReadValidator] Loaded {} ground truth judgments", query_judgments.size());
}

}  // namespace themis::retrieval
