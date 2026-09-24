// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/evaluation_result_store.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

namespace themis::rag {

EvaluationResultStore::EvaluationResultStore() {}

std::string EvaluationResultStore::StoreRun(const BenchmarkRun& run) {
  std::string run_id = run.run_id;
  if (run_id.empty()) {
    run_id = "run_" + std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  }

  runs_[run_id] = run;
  return run_id;
}

std::optional<EvaluationResultStore::BenchmarkRun> EvaluationResultStore::LoadResults(
    const std::string& dataset_name,
    const std::string& scenario_name,
    const std::string& version) {
  for (const auto& [run_id, run] : runs_) {
    if (run.dataset_name == dataset_name && run.scenario_name == scenario_name &&
        run.version == version) {
      return run;
    }
  }
  return std::nullopt;
}

std::vector<std::string> EvaluationResultStore::GetVersions(
    const std::string& dataset_name,
    const std::string& scenario_name) {
  std::vector<std::string> versions;

  for (const auto& [run_id, run] : runs_) {
    if (run.dataset_name == dataset_name && run.scenario_name == scenario_name) {
      versions.push_back(run.version);
    }
  }

  std::sort(versions.begin(), versions.end(), std::greater<std::string>());
  return versions;
}

EvaluationResultStore::ComparisonResult EvaluationResultStore::Compare(
    const std::string& baseline_run_id,
    const std::string& candidate_run_id) {
  ComparisonResult result;
  result.baseline_run_id = baseline_run_id;
  result.candidate_run_id = candidate_run_id;

  auto baseline_it = runs_.find(baseline_run_id);
  auto candidate_it = runs_.find(candidate_run_id);

  if (baseline_it == runs_.end() || candidate_it == runs_.end()) {
    result.verdict = "error";
    return result;
  }

  const auto& baseline_run = baseline_it->second;
  const auto& candidate_run = candidate_it->second;

  result.mean_ndcg_10_delta = candidate_run.mean_ndcg_10 - baseline_run.mean_ndcg_10;
  result.mean_ndcg_100_delta = candidate_run.mean_ndcg_100 - baseline_run.mean_ndcg_100;
  result.mean_mrr_10_delta = candidate_run.mean_mrr_10 - baseline_run.mean_mrr_10;
  result.mean_latency_delta_ms = candidate_run.mean_latency_ms - baseline_run.mean_latency_ms;
  result.mean_cost_delta_usd = candidate_run.mean_cost_usd - baseline_run.mean_cost_usd;

  // Compute per-query deltas
  for (size_t i = 0; i < baseline_run.query_results.size() && i < candidate_run.query_results.size();
       i++) {
    float delta = candidate_run.query_results[i].ndcg_10 - baseline_run.query_results[i].ndcg_10;
    result.query_level_deltas.push_back(delta);
  }

  // Statistical significance (simplified: p-value based on mean delta and variance)
  if (result.query_level_deltas.empty()) {
    result.p_value = 1.0f;
    result.is_significant = false;
  } else {
    float mean_delta =
        std::accumulate(result.query_level_deltas.begin(), result.query_level_deltas.end(), 0.0f) /
        result.query_level_deltas.size();
    float variance = 0.0f;
    for (float delta : result.query_level_deltas) {
      variance += (delta - mean_delta) * (delta - mean_delta);
    }
    variance /= result.query_level_deltas.size();
    float std_error = std::sqrt(variance / result.query_level_deltas.size());

    // Simplified t-test
    float t_statistic = std::abs(mean_delta) / std::max(std_error, 0.001f);
    result.p_value = 2.0f / (1.0f + std::exp(t_statistic));  // Approximate p-value

    result.is_significant = (result.p_value < 0.05f);
  }

  // Confidence intervals
  result.confidence_interval_lower = result.mean_ndcg_10_delta - 0.05f;
  result.confidence_interval_upper = result.mean_ndcg_10_delta + 0.05f;

  // Verdict
  if (result.mean_ndcg_10_delta > 0.02f && result.is_significant) {
    result.verdict = "win";
  } else if (result.mean_ndcg_10_delta < -0.02f && result.is_significant) {
    result.verdict = "regression";
  } else {
    result.verdict = "neutral";
  }

  return result;
}

EvaluationResultStore::Trend EvaluationResultStore::GetTrend(
    const std::string& dataset_name,
    const std::string& scenario_name,
    const std::string& metric_name) {
  Trend trend;
  trend.metric_name = metric_name;

  // Collect metric values over time
  std::vector<std::pair<int64_t, float>> time_value_pairs;

  for (const auto& [run_id, run] : runs_) {
    if (run.dataset_name == dataset_name && run.scenario_name == scenario_name) {
      float value = 0.0f;

      if (metric_name == "ndcg_10") {
        value = run.mean_ndcg_10;
      } else if (metric_name == "latency_ms") {
        value = run.mean_latency_ms;
      } else if (metric_name == "cost_usd") {
        value = run.mean_cost_usd;
      }

      time_value_pairs.push_back({run.created_at_us, value});
    }
  }

  // Sort by timestamp
  std::sort(time_value_pairs.begin(), time_value_pairs.end());

  for (const auto& [timestamp, value] : time_value_pairs) {
    trend.timestamps_us.push_back(timestamp);
    trend.values.push_back(value);
  }

  // Compute trend (slope)
  if (trend.values.size() >= 2) {
    float mean_x = 0.0f;
    float mean_y = 0.0f;
    for (size_t i = 0; i < trend.timestamps_us.size(); i++) {
      mean_x += i;
      mean_y += trend.values[i];
    }
    mean_x /= trend.timestamps_us.size();
    mean_y /= trend.values.size();

    float numerator = 0.0f;
    float denominator = 0.0f;
    for (size_t i = 0; i < trend.timestamps_us.size(); i++) {
      numerator += (i - mean_x) * (trend.values[i] - mean_y);
      denominator += (i - mean_x) * (i - mean_x);
    }

    trend.trend_coefficient = (denominator > 0) ? numerator / denominator : 0.0f;
    trend.direction = (trend.trend_coefficient > 0.01f)
                          ? "improving"
                          : (trend.trend_coefficient < -0.01f) ? "degrading" : "stable";
  }

  return trend;
}

std::string EvaluationResultStore::GenerateReport(
    const std::string& baseline_run_id,
    const std::string& candidate_run_id,
    const std::string& format) {
  auto comparison = Compare(baseline_run_id, candidate_run_id);

  if (format == "json") {
    return "{}";  // TODO: Serialize to JSON
  }

  // Markdown format
  std::string report = "# Evaluation Report\n\n";
  report += "## Comparison: " + baseline_run_id + " vs " + candidate_run_id + "\n\n";
  report += "| Metric | Baseline | Candidate | Delta | Verdict |\n";
  report += "|--------|----------|-----------|-------|----------|\n";
  report += "| NDCG@10 | " + std::to_string(runs_[baseline_run_id].mean_ndcg_10) + " | " +
            std::to_string(runs_[candidate_run_id].mean_ndcg_10) + " | " +
            std::to_string(comparison.mean_ndcg_10_delta) + " | " + comparison.verdict + " |\n";

  return report;
}

bool EvaluationResultStore::ExportRun(
    const std::string& run_id,
    const std::string& output_path) {
  // TODO: Export run to file
  return true;
}

uint32_t EvaluationResultStore::PruneOldRuns(uint32_t days_to_keep) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();
  uint64_t cutoff_us = now_us - static_cast<uint64_t>(days_to_keep) * 24LL * 3600LL * 1000000LL;

  uint32_t deleted_count = 0;

  for (auto it = runs_.begin(); it != runs_.end();) {
    if (it->second.created_at_us < cutoff_us) {
      it = runs_.erase(it);
      deleted_count++;
    } else {
      ++it;
    }
  }

  return deleted_count;
}

std::map<std::string, float> EvaluationResultStore::GetRunStats(const std::string& run_id) {
  std::map<std::string, float> stats;

  auto it = runs_.find(run_id);
  if (it != runs_.end()) {
    const auto& run = it->second;
    stats["mean_ndcg_10"] = run.mean_ndcg_10;
    stats["mean_ndcg_100"] = run.mean_ndcg_100;
    stats["mean_mrr_10"] = run.mean_mrr_10;
    stats["mean_latency_ms"] = run.mean_latency_ms;
    stats["mean_cost_usd"] = run.mean_cost_usd;
    stats["num_queries"] = static_cast<float>(run.query_results.size());
  }

  return stats;
}

}  // namespace themis::rag
