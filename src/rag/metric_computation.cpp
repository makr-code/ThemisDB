// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/metric_computation.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis::rag {

MetricComputer::MetricComputer(
    RelevanceModel model,
    uint32_t min_grade,
    uint32_t max_grade)
    : model_(model),
      min_grade_(min_grade),
      max_grade_(max_grade) {}

float MetricComputer::ComputeDCG(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  float dcg = 0.0f;

  for (uint32_t i = 0; i < std::min(static_cast<uint32_t>(ranked_doc_ids.size()), k); i++) {
    const auto& doc_id = ranked_doc_ids[i];
    auto it = relevance_judgments.find(doc_id);

    uint32_t relevance = 0;
    if (it != relevance_judgments.end()) {
      relevance = it->second;
    }

    if (relevance > 0) {
      dcg += relevance / std::log2(i + 2.0f);  // log2(position) where position starts at 1
    }
  }

  return dcg;
}

float MetricComputer::ComputeNDCG(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  float dcg = ComputeDCG(ranked_doc_ids, relevance_judgments, k);
  float idcg = ComputeIdealDCG(relevance_judgments, k);

  if (idcg == 0.0f) {
    return 0.0f;
  }

  return dcg / idcg;
}

float MetricComputer::ComputeMRR(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  for (uint32_t i = 0; i < std::min(static_cast<uint32_t>(ranked_doc_ids.size()), k); i++) {
    auto it = relevance_judgments.find(ranked_doc_ids[i]);
    if (it != relevance_judgments.end() && it->second > 0) {
      return 1.0f / (i + 1.0f);
    }
  }

  return 0.0f;
}

float MetricComputer::ComputeMAP(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  uint32_t num_relevant = 0;
  float sum_precisions = 0.0f;

  for (uint32_t i = 0; i < std::min(static_cast<uint32_t>(ranked_doc_ids.size()), k); i++) {
    auto it = relevance_judgments.find(ranked_doc_ids[i]);
    if (it != relevance_judgments.end() && it->second > 0) {
      num_relevant++;
      sum_precisions += static_cast<float>(num_relevant) / (i + 1.0f);
    }
  }

  if (num_relevant == 0) {
    return 0.0f;
  }

  return sum_precisions / num_relevant;
}

float MetricComputer::ComputePrecision(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  uint32_t num_relevant = 0;
  uint32_t num_retrieved = std::min(static_cast<uint32_t>(ranked_doc_ids.size()), k);

  for (uint32_t i = 0; i < num_retrieved; i++) {
    auto it = relevance_judgments.find(ranked_doc_ids[i]);
    if (it != relevance_judgments.end() && it->second > 0) {
      num_relevant++;
    }
  }

  return static_cast<float>(num_relevant) / num_retrieved;
}

float MetricComputer::ComputeRecall(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  uint32_t num_relevant_total = 0;
  uint32_t num_relevant_retrieved = 0;

  // Count total relevant docs
  for (const auto& [doc_id, relevance] : relevance_judgments) {
    if (relevance > 0) {
      num_relevant_total++;
    }
  }

  // Count relevant docs in top K
  for (uint32_t i = 0; i < std::min(static_cast<uint32_t>(ranked_doc_ids.size()), k); i++) {
    auto it = relevance_judgments.find(ranked_doc_ids[i]);
    if (it != relevance_judgments.end() && it->second > 0) {
      num_relevant_retrieved++;
    }
  }

  if (num_relevant_total == 0) {
    return 0.0f;
  }

  return static_cast<float>(num_relevant_retrieved) / num_relevant_total;
}

MetricComputer::MetricResult MetricComputer::ComputeAll(
    const std::vector<std::string>& ranked_doc_ids,
    const std::map<std::string, uint32_t>& relevance_judgments) {
  MetricResult result;
  result.ndcg_10 = ComputeNDCG(ranked_doc_ids, relevance_judgments, 10);
  result.ndcg_100 = ComputeNDCG(ranked_doc_ids, relevance_judgments, 100);
  result.mrr_10 = ComputeMRR(ranked_doc_ids, relevance_judgments, 10);
  result.mrr_100 = ComputeMRR(ranked_doc_ids, relevance_judgments, 100);
  result.map_10 = ComputeMAP(ranked_doc_ids, relevance_judgments, 10);
  result.map_100 = ComputeMAP(ranked_doc_ids, relevance_judgments, 100);
  result.precision_10 = ComputePrecision(ranked_doc_ids, relevance_judgments, 10);
  result.recall_10 = ComputeRecall(ranked_doc_ids, relevance_judgments, 10);
  result.precision_100 = ComputePrecision(ranked_doc_ids, relevance_judgments, 100);
  result.recall_100 = ComputeRecall(ranked_doc_ids, relevance_judgments, 100);
  return result;
}

void MetricComputer::SetCustomRelevanceFunction(
    std::function<uint32_t(const std::string&, const std::string&)> fn) {
  custom_fn_ = fn;
  model_ = RelevanceModel::Custom;
}

float MetricComputer::ComputeIdealDCG(
    const std::map<std::string, uint32_t>& relevance_judgments,
    uint32_t k) {
  // Sort judgments by relevance (descending)
  std::vector<uint32_t> sorted_relevances;
  for (const auto& [doc_id, relevance] : relevance_judgments) {
    sorted_relevances.push_back(relevance);
  }

  std::sort(sorted_relevances.begin(), sorted_relevances.end(),
            [](uint32_t a, uint32_t b) { return a > b; });

  float idcg = 0.0f;
  for (uint32_t i = 0; i < std::min(static_cast<uint32_t>(sorted_relevances.size()), k); i++) {
    if (sorted_relevances[i] > 0) {
      idcg += sorted_relevances[i] / std::log2(i + 2.0f);
    }
  }

  return idcg;
}

}  // namespace themis::rag
