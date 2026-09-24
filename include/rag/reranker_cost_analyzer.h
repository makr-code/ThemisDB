// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief Tracks re-ranking costs and quality outcomes for billing & ROI analysis.
///
/// Records per-query re-ranking costs (latency + tokens) and quality improvements
/// to support cost accounting, tenant billing, and ROI-based feedback loop.
///
/// @details
/// Cost model:
///   cost_unit = base_cost * tokens_used + latency_penalty * latency_ms
///   Example: 100ms inference, 150 tokens = 0.001 * 150 + 0.01 * 100 = 1.15 units
///
/// Beneficial rate tracking:
/// - Beneficial = quality_improvement > 2% nDCG@10
/// - Target: ≥80% beneficial rate (cost-effective reranking)
/// - Alerts if drops below 75%
///
/// Tenant budget forecasting:
///   daily_consumption = current_hour * (24 / hour_of_day)
///   at_risk = daily_consumption > daily_budget * 0.8
class RerankerCostAnalyzer {
 public:
  /// @brief Single cost record for a reranking operation.
  struct CostRecord {
    std::string query_id;
    std::string tenant_id;
    uint64_t request_time_ms;          ///< Model inference latency
    uint32_t tokens_used;              ///< Token count for billing
    float quality_improvement;         ///< nDCG@10 delta vs no-rerank
    bool was_beneficial;               ///< Improvement > 2%?
    std::string cost_breakdown;        ///< JSON with detailed breakdown
    int64_t timestamp_us;              ///< When operation occurred
  };

  /// @brief Aggregated cost metrics (hourly/daily).
  struct CostMetrics {
    float p50_cost_ms;
    float p95_cost_ms;
    float p99_cost_ms;
    float mean_quality_improvement;    ///< Avg nDCG@10 delta
    float beneficial_rate;             ///< % with improvement > 2%
    uint64_t sample_count;             ///< Operations in window
    int64_t window_start_us;
    int64_t window_end_us;
  };

  /// @brief Tenant cost allocation and forecasting.
  struct TenantCostAllocation {
    std::string tenant_id;
    float daily_budget;                ///< Daily limit in credits
    float consumed_today;              ///< Used so far today
    float forecasted_total;            ///< Projected total if trend continues
    bool is_at_risk;                   ///< Will exceed budget?
    float hours_until_budget_exceeded; ///< Estimated time to cap
  };

  /// @brief Constructor.
  explicit RerankerCostAnalyzer();

  /// @brief Destructor.
  ~RerankerCostAnalyzer() = default;

  /// @brief Record cost and quality outcome for a reranking operation.
  ///
  /// @param record Cost record with all metrics.
  ///
  /// @details
  /// - Stores record with timestamp
  /// - Aggregates into hourly buckets
  /// - Updates beneficial rate tracking
  /// - Checks if tenant budget forecast indicates risk
  void RecordCostAndQuality(const CostRecord& record);

  /// @brief Get aggregated cost metrics for time window.
  ///
  /// @param tenant_id Tenant identifier.
  /// @param since Time window start (if nullopt, last 1 hour).
  /// @return CostMetrics with p50/p95/p99 and quality stats.
  ///
  /// @details
  /// - Aggregates all records in window
  /// - Computes percentile metrics (p50/p95/p99)
  /// - Calculates beneficial rate
  /// - Returns std::nullopt if no records in window
  std::optional<CostMetrics> GetCostMetrics(
      const std::string& tenant_id,
      const std::optional<std::chrono::system_clock::time_point>& since = 
          std::nullopt);

  /// @brief Get tenant cost allocation and forecast.
  ///
  /// @return Vector of TenantCostAllocation for all active tenants.
  ///
  /// @details
  /// - Queries current consumption for today
  /// - Computes trend line (linear regression on hourly data)
  /// - Forecasts total consumption by midnight
  /// - Flags as at_risk if forecast > 80% of budget
  std::vector<TenantCostAllocation> GetTenantAllocations();

  /// @brief Get cost trends over multiple periods.
  ///
  /// @param tenant_id Tenant identifier.
  /// @param num_hours Number of hourly buckets to return (default 24 for daily view).
  /// @return Vector of CostMetrics sorted by time (oldest first).
  ///
  /// @details
  /// - Returns hourly-aggregated metrics
  /// - Useful for dashboard trend visualization
  /// - Shows cost improving/degrading over time
  std::vector<CostMetrics> GetCostTrends(
      const std::string& tenant_id,
      uint32_t num_hours = 24);

  /// @brief Beneficial rate alert: check if below threshold.
  ///
  /// @param tenant_id Tenant identifier.
  /// @param threshold Beneficial rate threshold (default 0.75 = 75%).
  /// @return true if beneficial rate is below threshold, indicating cost waste.
  bool IsBeneficialRateAboveThreshold(
      const std::string& tenant_id,
      float threshold = 0.75f);

  /// @brief Get hourly cost breakdown by component.
  ///
  /// @param tenant_id Tenant identifier.
  /// @return Map: component_name → mean cost (tokens + latency).
  ///
  /// @details
  /// - Tokens: token_count * $0.001 per 1000 tokens
  /// - Latency: inference_time * $0.01 per ms
  /// - Helpful for identifying cost drivers
  std::map<std::string, float> GetCostComponentBreakdown(
      const std::string& tenant_id);

  /// @brief Export all cost records for audit trail.
  ///
  /// @param tenant_id Tenant identifier.
  /// @return JSON string with all records.
  std::string ExportCostRecordsAsJSON(const std::string& tenant_id);

 private:
  // Cost record storage (in-memory with optional persistence to RocksDB)
  std::vector<CostRecord> records_;
  
  // Hourly aggregation cache (timestamp_hour → CostMetrics)
  std::map<int64_t, CostMetrics> hourly_aggregates_;
  
  // Tenant daily consumption tracking
  std::map<std::string, float> tenant_daily_consumption_;
  std::map<std::string, float> tenant_daily_budgets_;

  // Aggregation helpers
  void AggregateHourly();
  void UpdateTenantForecast(const std::string& tenant_id);
};

}  // namespace themis::rag
