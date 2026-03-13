/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cost_based_rate_limiter.cpp                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-13                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/cost_based_rate_limiter.h"
#include "utils/logger.h"

namespace themis {
namespace server {

// ============================================================================
// Construction
// ============================================================================

CostBasedRateLimiter::CostBasedRateLimiter(const Config& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{
}

// ============================================================================
// Public API
// ============================================================================

bool CostBasedRateLimiter::allowRequest(const std::string& client_id,
                                        OperationType op)
{
    return allowRequest(client_id, defaultCostFor(op));
}

bool CostBasedRateLimiter::allowRequest(const std::string& client_id,
                                        size_t cost)
{
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    // Periodic cleanup of expired windows (amortised per request).
    {
        auto now = std::chrono::steady_clock::now();
        if (now - last_cleanup_ >=
            std::chrono::seconds(config_.window_seconds)) {
            cleanupExpired();
            last_cleanup_ = now;
        }
    }

    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        if (clients_.size() >= config_.max_clients) {
            THEMIS_WARN("CostBasedRateLimiter: max_clients ({}) reached; "
                        "rejecting new client '{}'",
                        config_.max_clients, client_id);
            total_rejections_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        it = clients_.emplace(client_id,
                               ClientBudget{config_.budget_per_window}).first;
    }

    auto& budget = it->second;
    refreshWindow(budget);

    if (budget.remaining < cost) {
        THEMIS_DEBUG("CostBasedRateLimiter: client='{}' rejected "
                     "(cost={}, remaining={})",
                     client_id, cost, budget.remaining);
        total_rejections_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    budget.remaining -= cost;
    total_cost_consumed_.fetch_add(cost, std::memory_order_relaxed);
    return true;
}

size_t CostBasedRateLimiter::getRemainingBudget(
    const std::string& client_id) const
{
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        return config_.budget_per_window;
    }

    return computeEffectiveRemaining(it->second);
}

size_t CostBasedRateLimiter::getActiveClients() const
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

void CostBasedRateLimiter::cleanupExpired()
{
    std::lock_guard<std::mutex> lock(clients_mutex_);

    const auto now      = std::chrono::steady_clock::now();
    const auto lifetime = std::chrono::seconds(config_.window_seconds);

    for (auto it = clients_.begin(); it != clients_.end(); ) {
        if (now - it->second.window_start > lifetime) {
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

void CostBasedRateLimiter::reset()
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.clear();
    total_requests_.store(0, std::memory_order_relaxed);
    total_rejections_.store(0, std::memory_order_relaxed);
    total_cost_consumed_.store(0, std::memory_order_relaxed);
}

// ============================================================================
// Private helpers
// ============================================================================

void CostBasedRateLimiter::refreshWindow(ClientBudget& budget) const
{
    const auto now     = std::chrono::steady_clock::now();
    const auto elapsed = now - budget.window_start;

    if (elapsed >= std::chrono::seconds(config_.window_seconds)) {
        budget.remaining    = config_.budget_per_window;
        budget.window_start = now;
    }
}

size_t CostBasedRateLimiter::computeEffectiveRemaining(
    const ClientBudget& budget) const
{
    const auto now     = std::chrono::steady_clock::now();
    const auto elapsed = now - budget.window_start;

    if (elapsed >= std::chrono::seconds(config_.window_seconds)) {
        return config_.budget_per_window; // window expired → full budget
    }
    return budget.remaining;
}

} // namespace server
} // namespace themis
