// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "reranker_cost_analyzer.h"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <numeric>

namespace themis::rag {

using json = nlohmann::json;

RerankerCostAnalyzer::RerankerCostAnalyzer() {}

void RerankerCostAnalyzer::RecordCostAndQuality(const CostRecord& record) {
  // Store record
  records_.push_back(record);

  // Update tenant daily consumption
  if (tenant_daily_consumption_.find(record.tenant_id) ==
      tenant_daily_consumption_.end()) {
    tenant_daily_consumption_[record.tenant_id] = 0.0f;
  }

  // Cost unit: base_cost * tokens + latency_penalty * latency_ms
  float cost_unit = 0.001f * record.tokens_used + 0.01f * record.request_time_ms;
  tenant_daily_consumption_[record.tenant_id] += cost_unit;

  // Aggregate hourly metrics
  AggregateHourly();

  // Update forecast
  UpdateTenantForecast(record.tenant_id);
}

std::optional<RerankerCostAnalyzer::CostMetrics>
RerankerCostAnalyzer::GetCostMetrics(
    const std::string& tenant_id,
    const std::optional<std::chrono::system_clock::time_point>& since) {
  // Filter records by tenant and time window
  std::vector<CostRecord> tenant_records;
  for (const auto& record : records_) {
    if (record.tenant_id != tenant_id) {
      continue;
    }

    if (since) {
      int64_t since_us =
          std::chrono::duration_cast<std::chrono::microseconds>(
              since->time_since_epoch()).count();
      if (record.timestamp_us < since_us) {
        continue;
      }
    }

    tenant_records.push_back(record);
  }

  if (tenant_records.empty()) {
    return std::nullopt;
  }

  // Compute metrics
  CostMetrics metrics;
  metrics.sample_count = tenant_records.size();

  // Compute cost distribution
  std::vector<uint64_t> costs;
  for (const auto& record : tenant_records) {
    costs.push_back(record.request_time_ms);
  }
  std::sort(costs.begin(), costs.end());

  size_t p50_idx = costs.size() / 2;
  size_t p95_idx = (costs.size() * 95) / 100;
  size_t p99_idx = (costs.size() * 99) / 100;

  metrics.p50_cost_ms = costs[p50_idx];
  metrics.p95_cost_ms = costs[std::min(p95_idx, costs.size() - 1)];
  metrics.p99_cost_ms = costs[std::min(p99_idx, costs.size() - 1)];

  // Compute quality improvement
  float sum_quality_improvement = 0.0f;
  uint64_t beneficial_count = 0;
  for (const auto& record : tenant_records) {
    sum_quality_improvement += record.quality_improvement;
    if (record.was_beneficial) {
      beneficial_count++;
    }
  }

  metrics.mean_quality_improvement =
      sum_quality_improvement / static_cast<float>(tenant_records.size());
  metrics.beneficial_rate =
      static_cast<float>(beneficial_count) /
      static_cast<float>(tenant_records.size());

  return metrics;
}

std::vector<RerankerCostAnalyzer::TenantCostAllocation>
RerankerCostAnalyzer::GetTenantAllocations() {
  std::vector<TenantCostAllocation> allocations;

  for (const auto& [tenant_id, consumption] : tenant_daily_consumption_) {
    TenantCostAllocation allocation;
    allocation.tenant_id = tenant_id;
    allocation.daily_budget =
        tenant_daily_budgets_[tenant_id];
    allocation.consumed_today = consumption;
    allocation.forecasted_total = consumption;  // Simplified forecast
    allocation.is_at_risk = consumption > allocation.daily_budget * 0.8f;
    allocation.hours_until_budget_exceeded =
        allocation.is_at_risk ?
        (allocation.daily_budget - consumption) /
            (std::max(consumption, 0.1f) / 24.0f) :
        24.0f;

    allocations.push_back(allocation);
  }

  return allocations;
}

std::vector<RerankerCostAnalyzer::CostMetrics>
RerankerCostAnalyzer::GetCostTrends(const std::string& tenant_id,
                                     uint32_t num_hours) {
  std::vector<CostMetrics> trends;

  // Group records by hour
  std::map<int64_t, std::vector<CostRecord>> hourly_records;
  for (const auto& record : records_) {
    if (record.tenant_id != tenant_id) {
      continue;
    }

    int64_t hour_bucket = record.timestamp_us / (3600 * 1000000);
    hourly_records[hour_bucket].push_back(record);
  }

  // Sort by time and take last num_hours
  for (auto it = hourly_records.rbegin();
       it != hourly_records.rend() && trends.size() < num_hours;
       ++it) {
    const auto& hour_records = it->second;
    if (hour_records.empty()) {
      continue;
    }

    CostMetrics metrics;
    metrics.sample_count = hour_records.size();

    // Compute metrics for this hour
    std::vector<uint64_t> costs;
    for (const auto& record : hour_records) {
      costs.push_back(record.request_time_ms);
    }
    std::sort(costs.begin(), costs.end());

    if (!costs.empty()) {
      metrics.p50_cost_ms = costs[costs.size() / 2];
      metrics.p95_cost_ms = costs[(costs.size() * 95) / 100];
      metrics.p99_cost_ms = costs[(costs.size() * 99) / 100];
    }

    // Quality stats
    float sum_quality = 0.0f;
    uint64_t beneficial_count = 0;
    for (const auto& record : hour_records) {
      sum_quality += record.quality_improvement;
      if (record.was_beneficial) {
        beneficial_count++;
      }
    }
    metrics.mean_quality_improvement =
        sum_quality / static_cast<float>(hour_records.size());
    metrics.beneficial_rate =
        static_cast<float>(beneficial_count) /
        static_cast<float>(hour_records.size());

    trends.push_back(metrics);
  }

  return trends;
}

bool RerankerCostAnalyzer::IsBeneficialRateAboveThreshold(
    const std::string& tenant_id,
    float threshold) {
  auto metrics = GetCostMetrics(tenant_id);
  if (!metrics) {
    return true;  // Default to safe if no data
  }
  return metrics->beneficial_rate >= threshold;
}

std::map<std::string, float>
RerankerCostAnalyzer::GetCostComponentBreakdown(
    const std::string& tenant_id) {
  std::map<std::string, float> breakdown;

  // Filter tenant records
  std::vector<CostRecord> tenant_records;
  for (const auto& record : records_) {
    if (record.tenant_id == tenant_id) {
      tenant_records.push_back(record);
    }
  }

  if (tenant_records.empty()) {
    breakdown["tokens"] = 0.0f;
    breakdown["latency"] = 0.0f;
    return breakdown;
  }

  // Sum costs
  float token_cost = 0.0f;
  float latency_cost = 0.0f;

  for (const auto& record : tenant_records) {
    token_cost += 0.001f * record.tokens_used;  // $0.001 per 1000 tokens
    latency_cost += 0.01f * record.request_time_ms;  // $0.01 per ms
  }

  breakdown["tokens"] = token_cost / static_cast<float>(tenant_records.size());
  breakdown["latency"] = latency_cost / static_cast<float>(tenant_records.size());

  return breakdown;
}

std::string RerankerCostAnalyzer::ExportCostRecordsAsJSON(
    const std::string& tenant_id) {
  json records_json = json::array();

  for (const auto& record : records_) {
    if (record.tenant_id != tenant_id) {
      continue;
    }

    json j;
    j["query_id"] = record.query_id;
    j["tenant_id"] = record.tenant_id;
    j["request_time_ms"] = record.request_time_ms;
    j["tokens_used"] = record.tokens_used;
    j["quality_improvement"] = record.quality_improvement;
    j["was_beneficial"] = record.was_beneficial;
    j["timestamp_us"] = record.timestamp_us;
    j["cost_breakdown"] = record.cost_breakdown;

    records_json.push_back(j);
  }

  return records_json.dump(2);
}

void RerankerCostAnalyzer::AggregateHourly() {
  // Group records by hour and compute metrics
  std::map<int64_t, std::vector<CostRecord>> hourly_groups;

  for (const auto& record : records_) {
    int64_t hour_bucket = record.timestamp_us / (3600 * 1000000);
    hourly_groups[hour_bucket].push_back(record);
  }

  for (const auto& [hour, hour_records] : hourly_groups) {
    if (hour_records.empty()) {
      continue;
    }

    CostMetrics metrics;
    metrics.sample_count = hour_records.size();

    // Compute stats
    std::vector<uint64_t> costs;
    for (const auto& record : hour_records) {
      costs.push_back(record.request_time_ms);
    }
    std::sort(costs.begin(), costs.end());

    metrics.p50_cost_ms = costs[costs.size() / 2];
    metrics.p95_cost_ms = costs[(costs.size() * 95) / 100];
    metrics.p99_cost_ms = costs[(costs.size() * 99) / 100];

    hourly_aggregates_[hour] = metrics;
  }
}

void RerankerCostAnalyzer::UpdateTenantForecast(const std::string& tenant_id) {
  // Simple linear forecast: current consumption * (24 / hour_of_day)
  auto now = std::chrono::system_clock::now();
  auto now_time = std::chrono::system_clock::to_time_t(now);
  struct tm* tm_info = std::localtime(&now_time);
  int hour_of_day = tm_info->tm_hour + 1;

  float current = tenant_daily_consumption_[tenant_id];
  float forecast = current * (24.0f / std::max(1, hour_of_day));

  // TODO: Store forecast for later use
}

}  // namespace themis::rag
