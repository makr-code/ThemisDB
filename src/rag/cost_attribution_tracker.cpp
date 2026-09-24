// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/cost_attribution_tracker.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <set>

namespace themis::rag {

CostAttributionTracker::CostAttributionTracker() {}

void CostAttributionTracker::RecordCost(
    const std::string& tenant_id,
    const std::string& operation_type,
    float cost_usd,
    const std::string& model_id,
    const std::map<std::string, std::string>& attributes) {
  CostRecord record;
  record.tenant_id = tenant_id;
  record.operation_type = operation_type;
  record.model_id = model_id;
  record.cost_usd = cost_usd;
  record.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();
  record.attributes = attributes;

  cost_records_.push_back(record);
}

CostAttributionTracker::CostMetrics CostAttributionTracker::GetTenantCost(
    const std::string& tenant_id,
    const std::string& window_duration) {
  CostMetrics metrics;

  // Parse window duration (e.g., "1h", "1d")
  uint64_t window_us = 3600LL * 1000000LL;  // Default 1 hour
  if (window_duration == "1d") {
    window_us = 24LL * 3600LL * 1000000LL;
  } else if (window_duration == "30d") {
    window_us = 30LL * 24LL * 3600LL * 1000000LL;
  }

  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  metrics.window_start_us = now_us - window_us;
  metrics.window_end_us = now_us;
  metrics.total_cost_usd = 0.0f;
  metrics.query_count = 0;

  // Aggregate costs for tenant within window
  for (const auto& record : cost_records_) {
    if (record.tenant_id == tenant_id &&
        record.timestamp_us >= metrics.window_start_us &&
        record.timestamp_us <= metrics.window_end_us) {
      metrics.total_cost_usd += record.cost_usd;
      metrics.cost_by_operation[record.operation_type] += record.cost_usd;
      if (!record.model_id.empty()) {
        metrics.cost_by_model[record.model_id] += record.cost_usd;
      }
      metrics.query_count++;
    }
  }

  if (metrics.query_count > 0) {
    metrics.average_cost_per_query_usd = metrics.total_cost_usd / metrics.query_count;
  }

  return metrics;
}

float CostAttributionTracker::GetOperationCost(
    const std::string& tenant_id,
    const std::string& operation_type,
    const std::string& window_duration) {
  auto metrics = GetTenantCost(tenant_id, window_duration);
  auto it = metrics.cost_by_operation.find(operation_type);
  if (it != metrics.cost_by_operation.end()) {
    return it->second;
  }
  return 0.0f;
}

CostAttributionTracker::CostForecast CostAttributionTracker::ForecastTenantCost(
    const std::string& tenant_id,
    uint32_t days) {
  CostForecast forecast;

  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  forecast.forecast_start_us = now_us;
  forecast.forecast_end_us = now_us + static_cast<uint64_t>(days) * 24LL * 3600LL * 1000000LL;

  // Get current hourly rate
  auto hourly_metrics = GetTenantCost(tenant_id, "1h");
  float hourly_cost = hourly_metrics.total_cost_usd;

  // Project to N days
  forecast.projected_cost_usd = hourly_cost * 24.0f * days;

  // Confidence intervals (assume ±10% uncertainty)
  forecast.confidence_interval_lower = forecast.projected_cost_usd * 0.9f;
  forecast.confidence_interval_upper = forecast.projected_cost_usd * 1.1f;
  forecast.forecast_method = "linear";

  return forecast;
}

void CostAttributionTracker::SetTenantBudget(
    const std::string& tenant_id,
    float budget_usd) {
  tenant_budgets_[tenant_id] = budget_usd;
}

std::vector<CostAttributionTracker::BudgetAlert> CostAttributionTracker::GetBudgetAlerts() {
  std::vector<BudgetAlert> alerts;

  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  for (const auto& [tenant_id, budget] : tenant_budgets_) {
    auto monthly_metrics = GetTenantCost(tenant_id, "30d");

    float percentage_used = (monthly_metrics.total_cost_usd / budget) * 100.0f;

    if (percentage_used > 100.0f) {
      BudgetAlert alert;
      alert.type = BudgetAlert::AlertType::Exceeded;
      alert.tenant_id = tenant_id;
      alert.current_spend_usd = monthly_metrics.total_cost_usd;
      alert.budget_usd = budget;
      alert.percentage_used = percentage_used;
      alert.recommendation = "Reduce query volume or optimize settings";
      alert.timestamp_us = now_us;

      alerts.push_back(alert);
    } else if (percentage_used > 80.0f) {
      BudgetAlert alert;
      alert.type = BudgetAlert::AlertType::Approaching;
      alert.tenant_id = tenant_id;
      alert.current_spend_usd = monthly_metrics.total_cost_usd;
      alert.budget_usd = budget;
      alert.percentage_used = percentage_used;
      alert.recommendation = "Monitor spending; consider optimizations";
      alert.timestamp_us = now_us;

      alerts.push_back(alert);
    }
  }

  return alerts;
}

std::vector<CostAttributionTracker::CostRecord> CostAttributionTracker::GetRecentRecords(
    const std::string& tenant_id,
    uint32_t hours,
    uint32_t limit) {
  std::vector<CostRecord> recent;

  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();
  uint64_t cutoff_us = now_us - static_cast<uint64_t>(hours) * 3600LL * 1000000LL;

  for (const auto& record : cost_records_) {
    if (record.tenant_id == tenant_id && record.timestamp_us >= cutoff_us) {
      recent.push_back(record);
    }
    if (recent.size() >= limit) {
      break;
    }
  }

  return recent;
}

std::map<std::string, CostAttributionTracker::CostMetrics>
CostAttributionTracker::GetAttributionReport(const std::string& window_duration) {
  std::map<std::string, CostMetrics> report;

  // Get unique tenants
  std::set<std::string> tenants;
  for (const auto& record : cost_records_) {
    tenants.insert(record.tenant_id);
  }

  // Get metrics for each tenant
  for (const auto& tenant_id : tenants) {
    report[tenant_id] = GetTenantCost(tenant_id, window_duration);
  }

  return report;
}

float CostAttributionTracker::GetForecastVariance(const std::string& tenant_id) {
  // TODO: Compare actual cost vs forecast from cost model
  return 0.0f;  // Placeholder: 0% variance
}

}  // namespace themis::rag
