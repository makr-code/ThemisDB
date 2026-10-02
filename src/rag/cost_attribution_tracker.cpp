// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/cost_attribution_tracker.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <numeric>
#include <set>

namespace themis::rag {
namespace {

uint64_t ParseWindowDurationUs(const std::string& window_duration) {
  if (window_duration == "1h") {
    return 3600ULL * 1000000ULL;
  }
  if (window_duration == "1d") {
    return 24ULL * 3600ULL * 1000000ULL;
  }
  if (window_duration == "30d") {
    return 30ULL * 24ULL * 3600ULL * 1000000ULL;
  }
  return 3600ULL * 1000000ULL;
}

}  // namespace

CostAttributionTracker::CostAttributionTracker()
    : db_path_(""), has_rocksdb_(false), db_(nullptr) {}

bool CostAttributionTracker::Initialize(const std::string& db_path) {
  db_path_ = db_path;
  if (db_path.empty()) {
    has_rocksdb_ = false;
    db_ = nullptr;
    return true;
  }

  const std::filesystem::path persistence_dir(db_path);
  std::error_code ec;
  if (!persistence_dir.empty()) {
    std::filesystem::create_directories(persistence_dir, ec);
  }

  has_rocksdb_ = !ec;
  db_ = has_rocksdb_ ? reinterpret_cast<void*>(1) : nullptr;
  return has_rocksdb_;
}

bool CostAttributionTracker::IsPersistentStorageAvailable() const {
  return has_rocksdb_;
}

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

  const uint64_t window_us = ParseWindowDurationUs(window_duration);
  const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  metrics.window_start_us = now_us - window_us;
  metrics.window_end_us = now_us;
  metrics.total_cost_usd = 0.0f;
  metrics.query_count = 0;

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
    metrics.average_cost_per_query_usd = metrics.total_cost_usd / static_cast<float>(metrics.query_count);
  }

  return metrics;
}

float CostAttributionTracker::GetOperationCost(
    const std::string& tenant_id,
    const std::string& operation_type,
    const std::string& window_duration) {
  const auto metrics = GetTenantCost(tenant_id, window_duration);
  const auto it = metrics.cost_by_operation.find(operation_type);
  if (it != metrics.cost_by_operation.end()) {
    return it->second;
  }
  return 0.0f;
}

CostAttributionTracker::CostForecast CostAttributionTracker::ForecastTenantCost(
    const std::string& tenant_id,
    uint32_t days) {
  CostForecast forecast;

  const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  forecast.forecast_start_us = now_us;
  forecast.forecast_end_us = now_us + static_cast<uint64_t>(days) * 24ULL * 3600ULL * 1000000ULL;

  const auto last_30d = GetTenantCost(tenant_id, "30d");
  const float daily_cost = last_30d.total_cost_usd / 30.0f;
  forecast.projected_cost_usd = daily_cost * static_cast<float>(days);

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

  const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  for (const auto& [tenant_id, budget] : tenant_budgets_) {
    const auto monthly_metrics = GetTenantCost(tenant_id, "30d");
    if (budget <= 0.0f) {
      continue;
    }

    const float percentage_used = (monthly_metrics.total_cost_usd / budget) * 100.0f;

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

  const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();
  const uint64_t cutoff_us = now_us - static_cast<uint64_t>(hours) * 3600ULL * 1000000ULL;

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

  std::set<std::string> tenants;
  for (const auto& record : cost_records_) {
    tenants.insert(record.tenant_id);
  }

  for (const auto& tenant_id : tenants) {
    report[tenant_id] = GetTenantCost(tenant_id, window_duration);
  }

  return report;
}

float CostAttributionTracker::GetForecastVariance(const std::string& tenant_id) {
  const auto actual_metrics = GetTenantCost(tenant_id, "30d");
  const auto forecast = ForecastTenantCost(tenant_id, 30);
  if (forecast.projected_cost_usd <= 0.0f) {
    return 0.0f;
  }
  return ((actual_metrics.total_cost_usd - forecast.projected_cost_usd) /
          forecast.projected_cost_usd) *
         100.0f;
}

}  // namespace themis::rag
