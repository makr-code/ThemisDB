/**
 * @file budget_allocator.h
 * @brief Per-tenant budget management and fair scheduling for RAG Phase 12
 *
 * Enforces hard SLO limits and soft thresholds with fair queuing under overload.
 *
 * @version 0.1.0
 * @note Phase: 12 (Advanced Cost Optimization)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace themis::rag::optimization {

/**
 * @brief Tenant budget configuration
 */
struct TenantBudget {
  std::string tenant_id;
  
  // Hard limits (queries rejected if exceeded)
  double max_daily_cost = 100.0;        ///< USD per day
  double max_hourly_cost = 10.0;        ///< USD per hour
  double max_query_latency_ms = 5000.0; ///< Max latency per query
  
  // Soft thresholds (warn but allow, may be throttled)
  double warn_daily_cost = 80.0;        ///< Warn at 80% of daily limit
  double warn_hourly_cost = 8.0;        ///< Warn at 80% of hourly limit
  double warn_query_latency_ms = 3000.0; ///< Warn if approaching limit
  
  // Priority for fair queuing (0=low, 10=high)
  uint32_t priority_level = 5;
  
  // SLO target: p95 latency should be <= this
  double target_p95_latency_ms = 500.0;
};

/**
 * @brief Current budget utilization status
 */
struct BudgetStatus {
  double daily_cost_used = 0.0;
  double hourly_cost_used = 0.0;
  double daily_cost_remaining = 100.0;
  double hourly_cost_remaining = 10.0;
  
  bool is_daily_limit_exceeded = false;
  bool is_hourly_limit_exceeded = false;
  bool is_warn_threshold_reached = false;
  
  double recent_p95_latency_ms = 0.0;
  bool is_slo_violated = false;
};

/**
 * @brief Budget Allocator — Tenant resource management
 *
 * Enforces per-tenant budgets (cost and latency) with fair queue scheduling
 * when resources are constrained. Integrates with Phase 10 cost model for
 * accurate cost prediction.
 *
 * Thread-safe for concurrent budget checks and updates.
 */
class BudgetAllocator {
 public:
  /**
   * @brief Constructor
   *
   * @param enable_fair_queuing true to enable queue-based fairness under overload
   * @param max_queue_depth Maximum pending queries per tenant (default 100)
   */
  BudgetAllocator(bool enable_fair_queuing = true, uint32_t max_queue_depth = 100);
  ~BudgetAllocator();

  /**
   * @brief Register a tenant with budget configuration
   *
   * @param budget Tenant budget settings
   * @return true if registered, false if tenant already exists
   */
  bool RegisterTenant(const TenantBudget& budget);

  /**
   * @brief Check if query is allowed under budget constraints
   *
   * Returns decision based on:
   * 1. Hard limits (daily/hourly cost, max latency)
   * 2. Available queue depth
   * 3. Current load and fair share calculation
   *
   * @param tenant_id Tenant making the query
   * @param estimated_cost Estimated cost from Phase 10 cost model
   * @param estimated_latency_ms Estimated latency from Phase 12 QueryPlanner
   * @return true if query can proceed, false if rejected
   */
  bool CanExecuteQuery(const std::string& tenant_id, double estimated_cost,
                      double estimated_latency_ms);

  /**
   * @brief Reserve budget for query execution
   *
   * Called before query starts to lock in budget allocation.
   * Returns reservation ID for later confirmation.
   *
   * @param tenant_id Tenant making the query
   * @param estimated_cost Estimated cost
   * @param estimated_latency_ms Estimated latency
   * @return Reservation ID (>0) if reserved, 0 if failed
   */
  uint64_t ReserveBudget(const std::string& tenant_id, double estimated_cost,
                        double estimated_latency_ms);

  /**
   * @brief Confirm query execution and charge budget
   *
   * Called after query completes to deduct actual cost from budget.
   *
   * @param reservation_id ID from ReserveBudget()
   * @param actual_cost Actual cost incurred
   * @param actual_latency_ms Actual latency observed
   */
  void ConfirmBudget(uint64_t reservation_id, double actual_cost, double actual_latency_ms);

  /**
   * @brief Release reservation without charging
   *
   * Called if query fails or is cancelled before completion.
   *
   * @param reservation_id ID from ReserveBudget()
   */
  void ReleaseBudget(uint64_t reservation_id);

  /**
   * @brief Get current budget status
   *
   * @param tenant_id Tenant to check
   * @return Current utilization and remaining budget
   */
  BudgetStatus GetBudgetStatus(const std::string& tenant_id) const;

  /**
   * @brief Update tenant budget configuration
   *
   * Can be called at runtime to adjust budgets based on usage patterns.
   *
   * @param budget Updated budget settings
   * @return true if updated, false if tenant not found
   */
  bool UpdateTenantBudget(const TenantBudget& budget);

  /**
   * @brief Get queue depth for tenant
   *
   * Returns number of pending queries in fair queue.
   *
   * @param tenant_id Tenant to check
   * @return Queue depth
   */
  uint32_t GetQueueDepth(const std::string& tenant_id) const;

  /**
   * @brief Reset hourly budget counters
   *
   * Called hourly to reset counters for new budget window.
   * Automatically called internally on hour boundaries.
   */
  void ResetHourlyBudgets();

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::optimization
