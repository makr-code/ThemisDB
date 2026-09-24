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

/// @brief Multi-tenant cost tracking and attribution.
///
/// Tracks costs across multiple dimensions (tenant, operation, model, time)
/// with fine-grained attribution for billing and cost optimization. Supports
/// cost forecasting and alerts on budget overruns.
///
/// @details
/// Cost dimensions:
/// - Tenant: Multi-tenant isolation
/// - Operation: Retrieval, reranking, refresh, etc.
/// - Model: Embedding model, reranker model, LLM
/// - Time: Hourly, daily, monthly windows
///
/// Cost components:
/// - Retrieval: BM25 indexing, HNSW search, network
/// - Reranking: Cross-encoder forward pass, batching overhead
/// - Freshness: Index refresh, delta indexing, staleness monitoring
/// - Infrastructure: Compute, memory, storage, network
///
/// Attribution model:
/// - Per-query: retrieval_cost + rerank_cost * rerank_fraction
/// - Per-shard: refresh_cost / num_shards
/// - Infrastructure: amortized over query volume
///
/// Forecasting:
/// - Hourly: Project 24-hour spend based on current rate
/// - Daily: Project 30-day spend based on daily average
/// - Seasonal: Track weekly/monthly patterns
///
/// @code
/// auto tracker = std::make_unique<CostAttributionTracker>();
/// tracker->RecordCost("tenant_1", "retrieval", 0.002f);
/// tracker->RecordCost("tenant_1", "rerank", 0.005f);
/// auto daily_cost = tracker->GetTenantCost("tenant_1", "1d");
/// auto forecast = tracker->ForecastTenantCost("tenant_1", 30);  // 30-day
/// @endcode
class CostAttributionTracker {
 public:
  /// @brief Cost record.
  struct CostRecord {
    std::string tenant_id;
    std::string operation_type;      ///< "retrieval", "rerank", "refresh", etc.
    std::string model_id;            ///< Model identifier (optional)
    float cost_usd;
    uint64_t timestamp_us;
    std::map<std::string, std::string> attributes;  ///< Custom tags
  };

  /// @brief Cost metrics over time window.
  struct CostMetrics {
    uint64_t window_start_us;
    uint64_t window_end_us;
    float total_cost_usd;
    float average_cost_per_query_usd;
    std::map<std::string, float> cost_by_operation;  ///< operation_type → cost
    std::map<std::string, float> cost_by_model;      ///< model_id → cost
    uint64_t query_count;
  };

  /// @brief Cost forecast.
  struct CostForecast {
    uint64_t forecast_start_us;
    uint64_t forecast_end_us;
    float projected_cost_usd;
    float confidence_interval_lower;  ///< 95% CI lower bound
    float confidence_interval_upper;  ///< 95% CI upper bound
    std::string forecast_method;      ///< "linear", "exponential", "seasonal"
  };

  /// @brief Budget alert.
  struct BudgetAlert {
    enum class AlertType {
      Approaching,   ///< Cost approaching budget (>80%)
      Exceeded,      ///< Budget exceeded
      TrendWarning   ///< Current rate would exceed budget by window end
    };

    AlertType type;
    std::string tenant_id;
    float current_spend_usd;
    float budget_usd;
    float percentage_used;
    std::string recommendation;
    int64_t timestamp_us;
  };

  /// @brief Constructor.
  CostAttributionTracker();

  /// @brief Record cost event.
  /// @param tenant_id Tenant identifier.
  /// @param operation_type Operation type.
  /// @param cost_usd Cost in USD.
  /// @param model_id Model identifier (optional).
  /// @param attributes Custom tags (optional).
  void RecordCost(
      const std::string& tenant_id,
      const std::string& operation_type,
      float cost_usd,
      const std::string& model_id = "",
      const std::map<std::string, std::string>& attributes = {});

  /// @brief Get tenant cost for time window.
  /// @param tenant_id Tenant identifier.
  /// @param window_duration "1h", "1d", "30d", etc.
  /// @return Cost metrics.
  CostMetrics GetTenantCost(
      const std::string& tenant_id,
      const std::string& window_duration);

  /// @brief Get cost by operation type.
  /// @param tenant_id Tenant identifier.
  /// @param operation_type Operation to query.
  /// @param window_duration Time window.
  /// @return Cost for operation.
  float GetOperationCost(
      const std::string& tenant_id,
      const std::string& operation_type,
      const std::string& window_duration);

  /// @brief Forecast tenant cost.
  /// @param tenant_id Tenant identifier.
  /// @param days Days to forecast.
  /// @return Forecast with confidence intervals.
  CostForecast ForecastTenantCost(const std::string& tenant_id, uint32_t days);

  /// @brief Set tenant budget.
  /// @param tenant_id Tenant identifier.
  /// @param budget_usd Monthly budget.
  void SetTenantBudget(const std::string& tenant_id, float budget_usd);

  /// @brief Get active budget alerts.
  /// @return List of alerts.
  std::vector<BudgetAlert> GetBudgetAlerts();

  /// @brief Get recent cost records (for audit).
  /// @param tenant_id Tenant identifier.
  /// @param hours Hours to look back.
  /// @param limit Max records to return.
  /// @return Recent cost records.
  std::vector<CostRecord> GetRecentRecords(
      const std::string& tenant_id,
      uint32_t hours,
      uint32_t limit = 1000);

  /// @brief Get cost attribution report.
  /// @param window_duration Time window.
  /// @return Map of tenant_id → CostMetrics.
  std::map<std::string, CostMetrics> GetAttributionReport(
      const std::string& window_duration);

  /// @brief Compare cost with forecast.
  /// @param tenant_id Tenant identifier.
  /// @return Variance percentage (actual vs forecast).
  float GetForecastVariance(const std::string& tenant_id);

 private:
  std::vector<CostRecord> cost_records_;
  std::map<std::string, float> tenant_budgets_;
  std::map<std::string, std::vector<BudgetAlert>> budget_alerts_;
};

}  // namespace themis::rag
