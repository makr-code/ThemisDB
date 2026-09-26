// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief Persistent storage and comparison of evaluation results.
///
/// Stores benchmark results with versioning and enables trend analysis,
/// regression detection, and A/B comparison across scenarios. Supports
/// time-series queries and statistical significance testing.
///
/// @details
/// Result organization:
/// - Dataset: Dataset name (e.g., "trec_covid")
/// - Version: Timestamp (e.g., "2026-09-24T11:32Z")
/// - Scenario: Scenario name (e.g., "adaptive_rag")
/// - Query: Individual query result (e.g., "q123")
///
/// Storage:
/// - In-memory cache: Recent results for fast access
/// - RocksDB: Persistent storage (TODO)
/// - Time-series DB: Trend queries (TODO)
///
/// Comparison workflow:
/// 1. Load results for baseline scenario
/// 2. Load results for candidate scenario
/// 3. Compute statistical tests (paired t-test, bootstrap)
/// 4. Identify regressions and wins
/// 5. Report impact with confidence intervals
///
/// @code
/// auto store = std::make_unique<EvaluationResultStore>();
/// auto baseline_run = store->LoadResults("trec_covid", "baseline", "v1");
/// auto candidate_run = store->LoadResults("trec_covid", "adaptive_rag", "v2");
/// auto comparison = store->Compare(baseline_run, candidate_run);
/// auto report = comparison->GenerateReport("adaptive_rag");
/// @endcode
class EvaluationResultStore {
 public:
  /// @brief Query result.
  struct QueryResult {
    std::string query_id;
    std::string scenario;
    float ndcg_10;
    float ndcg_100;
    float mrr_10;
    float map_10;
    float precision_10;
    float recall_10;
    uint64_t query_latency_ms;
    float query_cost_usd;
    int64_t computed_at_us;
  };

  /// @brief Benchmark run (complete evaluation).
  struct BenchmarkRun {
    std::string run_id;
    std::string dataset_name;
    std::string scenario_name;
    std::string version;
    std::vector<QueryResult> query_results;
    int64_t created_at_us;
    std::map<std::string, std::string> metadata;

    // Aggregated metrics
    float mean_ndcg_10;
    float mean_ndcg_100;
    float mean_mrr_10;
    float mean_map_10;
    float mean_latency_ms;
    float mean_cost_usd;
  };

  /// @brief Comparison result.
  struct ComparisonResult {
    std::string baseline_run_id;
    std::string candidate_run_id;
    float mean_ndcg_10_delta;            ///< Candidate - Baseline
    float mean_ndcg_100_delta;
    float mean_mrr_10_delta;
    float mean_latency_delta_ms;
    float mean_cost_delta_usd;
    std::vector<float> query_level_deltas;  ///< Per-query NDCG@10 delta
    float p_value;                          ///< Statistical significance
    bool is_significant;                    ///< p-value < 0.05
    std::string verdict;                    ///< "win" | "neutral" | "regression"
    float confidence_interval_lower;    ///< 95% CI
    float confidence_interval_upper;
  };

  /// @brief Trend analysis.
  struct Trend {
    std::string metric_name;             ///< "ndcg_10", "latency_ms", etc
    std::vector<int64_t> timestamps_us;  ///< Chronological timestamps
    std::vector<float> values;           ///< Corresponding metric values
    float trend_coefficient;             ///< Slope (positive = improving)
    float r_squared;                     ///< R^2 (trend fit quality)
    std::string direction;               ///< "improving" | "degrading" | "stable"
  };

  /// @brief Constructor.
  EvaluationResultStore();

  /// @brief Store benchmark run.
  /// @param run Benchmark run with results.
  /// @return Run ID.
  std::string StoreRun(const BenchmarkRun& run);

  /// @brief Load results for scenario.
  /// @param dataset_name Dataset identifier.
  /// @param scenario_name Scenario name.
  /// @param version Version/timestamp.
  /// @return Benchmark run (empty if not found).
  std::optional<BenchmarkRun> LoadResults(
      const std::string& dataset_name,
      const std::string& scenario_name,
      const std::string& version);

  /// @brief Get all versions for scenario.
  /// @param dataset_name Dataset identifier.
  /// @param scenario_name Scenario name.
  /// @return List of versions (newest first).
  std::vector<std::string> GetVersions(
      const std::string& dataset_name,
      const std::string& scenario_name);

  /// @brief Compare two benchmark runs.
  /// @param baseline_run_id Baseline run ID.
  /// @param candidate_run_id Candidate run ID.
  /// @return Comparison result with significance testing.
  ComparisonResult Compare(
      const std::string& baseline_run_id,
      const std::string& candidate_run_id);

  /// @brief Get trend over time.
  /// @param dataset_name Dataset identifier.
  /// @param scenario_name Scenario name.
  /// @param metric_name Metric to track.
  /// @return Time-series trend.
  Trend GetTrend(
      const std::string& dataset_name,
      const std::string& scenario_name,
      const std::string& metric_name);

  /// @brief Generate comparison report (markdown/JSON).
  /// @param baseline_run_id Baseline run ID.
  /// @param candidate_run_id Candidate run ID.
  /// @param format "markdown" or "json".
  /// @return Report string.
  std::string GenerateReport(
      const std::string& baseline_run_id,
      const std::string& candidate_run_id,
      const std::string& format = "markdown");

  /// @brief Export run to file.
  /// @param run_id Run ID to export.
  /// @param output_path Output file path.
  /// @return true if exported successfully.
  bool ExportRun(const std::string& run_id, const std::string& output_path);

  /// @brief Delete old runs (retention policy).
  /// @param days_to_keep Keep runs from last N days.
  /// @return Number of runs deleted.
  uint32_t PruneOldRuns(uint32_t days_to_keep);

  /// @brief Get run statistics.
  /// @param run_id Run ID.
  /// @return Map of stat_name → value.
  std::map<std::string, float> GetRunStats(const std::string& run_id);

 private:
  std::map<std::string, BenchmarkRun> runs_;  // run_id -> BenchmarkRun
};

}  // namespace themis::rag
