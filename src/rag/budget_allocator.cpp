/**
 * @file budget_allocator.cpp
 * @brief Implementation of BudgetAllocator for Phase 12
 */

#include "rag/budget_allocator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <queue>

namespace themis::rag::optimization {

struct TenantBudgetState {
  TenantBudget config;
  double daily_cost_used = 0.0;
  double hourly_cost_used = 0.0;
  std::chrono::system_clock::time_point hour_start = std::chrono::system_clock::now();
  
  uint32_t queue_depth = 0;
  double total_latency_sum = 0.0;
  uint32_t latency_samples = 0;
};

struct BudgetAllocator::Impl {
  std::mutex mu;
  std::unordered_map<std::string, TenantBudgetState> tenants;
  bool enable_fair_queuing = true;
  uint32_t max_queue_depth = 100;
  uint64_t reservation_counter = 0;
  
  // Track reservations for cleanup
  struct Reservation {
    std::string tenant_id;
    double reserved_cost = 0.0;
    uint64_t reservation_id = 0;
  };
  std::unordered_map<uint64_t, Reservation> reservations;
};

BudgetAllocator::BudgetAllocator(bool enable_fair_queuing, uint32_t max_queue_depth)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->enable_fair_queuing = enable_fair_queuing;
  pimpl_->max_queue_depth = max_queue_depth;
}

BudgetAllocator::~BudgetAllocator() = default;

bool BudgetAllocator::RegisterTenant(const TenantBudget& budget) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  if (pimpl_->tenants.count(budget.tenant_id)) {
    return false;  // Already registered
  }
  
  TenantBudgetState state;
  state.config = budget;
  pimpl_->tenants[budget.tenant_id] = state;
  return true;
}

bool BudgetAllocator::CanExecuteQuery(const std::string& tenant_id, double estimated_cost,
                                     double estimated_latency_ms) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto it = pimpl_->tenants.find(tenant_id);
  if (it == pimpl_->tenants.end()) {
    return false;  // Tenant not registered
  }
  
  TenantBudgetState& state = it->second;
  
  // Reset hourly counters if hour boundary crossed
  auto now = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - state.hour_start);
  if (elapsed.count() > 0) {
    state.hourly_cost_used = 0.0;
    state.hour_start = now;
  }
  
  // Check hard limits
  if (state.daily_cost_used + estimated_cost > state.config.max_daily_cost) {
    return false;  // Daily limit exceeded
  }
  if (state.hourly_cost_used + estimated_cost > state.config.max_hourly_cost) {
    return false;  // Hourly limit exceeded
  }
  if (estimated_latency_ms > state.config.max_query_latency_ms) {
    return false;  // Query would exceed max latency
  }
  
  // Check queue depth
  if (pimpl_->enable_fair_queuing && state.queue_depth >= pimpl_->max_queue_depth) {
    return false;  // Queue full
  }
  
  return true;
}

uint64_t BudgetAllocator::ReserveBudget(const std::string& tenant_id, double estimated_cost,
                                       double estimated_latency_ms) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  if (!CanExecuteQuery(tenant_id, estimated_cost, estimated_latency_ms)) {
    return 0;  // Can't reserve
  }
  
  auto it = pimpl_->tenants.find(tenant_id);
  if (it == pimpl_->tenants.end()) {
    return 0;
  }
  
  // Generate reservation ID
  uint64_t res_id = ++pimpl_->reservation_counter;
  
  // Record reservation
  Impl::Reservation res;
  res.tenant_id = tenant_id;
  res.reserved_cost = estimated_cost;
  res.reservation_id = res_id;
  pimpl_->reservations[res_id] = res;
  
  // Increment queue depth
  if (pimpl_->enable_fair_queuing) {
    it->second.queue_depth++;
  }
  
  return res_id;
}

void BudgetAllocator::ConfirmBudget(uint64_t reservation_id, double actual_cost,
                                   double actual_latency_ms) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto res_it = pimpl_->reservations.find(reservation_id);
  if (res_it == pimpl_->reservations.end()) {
    return;  // Reservation not found
  }
  
  const auto& res = res_it->second;
  auto tenant_it = pimpl_->tenants.find(res.tenant_id);
  if (tenant_it == pimpl_->tenants.end()) {
    return;
  }
  
  TenantBudgetState& state = tenant_it->second;
  
  // Charge actual cost (if less than reserved, it's a credit)
  state.daily_cost_used += actual_cost;
  state.hourly_cost_used += actual_cost;
  
  // Track latency for SLO
  state.total_latency_sum += actual_latency_ms;
  state.latency_samples++;
  
  // Decrement queue depth
  if (pimpl_->enable_fair_queuing && state.queue_depth > 0) {
    state.queue_depth--;
  }
  
  // Clean up reservation
  pimpl_->reservations.erase(res_it);
}

void BudgetAllocator::ReleaseBudget(uint64_t reservation_id) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto res_it = pimpl_->reservations.find(reservation_id);
  if (res_it == pimpl_->reservations.end()) {
    return;
  }
  
  const auto& res = res_it->second;
  auto tenant_it = pimpl_->tenants.find(res.tenant_id);
  if (tenant_it != pimpl_->tenants.end()) {
    TenantBudgetState& state = tenant_it->second;
    if (pimpl_->enable_fair_queuing && state.queue_depth > 0) {
      state.queue_depth--;
    }
  }
  
  pimpl_->reservations.erase(res_it);
}

BudgetStatus BudgetAllocator::GetBudgetStatus(const std::string& tenant_id) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  BudgetStatus status;
  
  auto it = pimpl_->tenants.find(tenant_id);
  if (it == pimpl_->tenants.end()) {
    return status;  // Empty status
  }
  
  const TenantBudgetState& state = it->second;
  
  status.daily_cost_used = state.daily_cost_used;
  status.hourly_cost_used = state.hourly_cost_used;
  status.daily_cost_remaining = state.config.max_daily_cost - state.daily_cost_used;
  status.hourly_cost_remaining = state.config.max_hourly_cost - state.hourly_cost_used;
  
  status.is_daily_limit_exceeded = status.daily_cost_remaining < 0;
  status.is_hourly_limit_exceeded = status.hourly_cost_remaining < 0;
  
  // Check warn thresholds
  if (state.daily_cost_used >= state.config.warn_daily_cost ||
      state.hourly_cost_used >= state.config.warn_hourly_cost) {
    status.is_warn_threshold_reached = true;
  }
  
  // Compute P95 latency
  if (state.latency_samples > 0) {
    status.recent_p95_latency_ms = state.total_latency_sum / state.latency_samples;
    // This is a simplification; real P95 would require storing percentile data
  }
  
  if (status.recent_p95_latency_ms > state.config.target_p95_latency_ms) {
    status.is_slo_violated = true;
  }
  
  return status;
}

bool BudgetAllocator::UpdateTenantBudget(const TenantBudget& budget) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto it = pimpl_->tenants.find(budget.tenant_id);
  if (it == pimpl_->tenants.end()) {
    return false;
  }
  
  it->second.config = budget;
  return true;
}

uint32_t BudgetAllocator::GetQueueDepth(const std::string& tenant_id) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto it = pimpl_->tenants.find(tenant_id);
  if (it == pimpl_->tenants.end()) {
    return 0;
  }
  
  return it->second.queue_depth;
}

void BudgetAllocator::ResetHourlyBudgets() {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto now = std::chrono::system_clock::now();
  for (auto& [tenant_id, state] : pimpl_->tenants) {
    state.hourly_cost_used = 0.0;
    state.hour_start = now;
  }
}

}  // namespace themis::rag::optimization
