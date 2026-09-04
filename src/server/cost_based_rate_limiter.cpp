/**
 * @file cost_based_rate_limiter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    std::lock_guard<std::mutex> lock(clients_mutex_);

    // Periodic cleanup of expired windows (amortised per request, under lock).
    // We check last_cleanup_ under the same lock used for the client-bucket
    // operations below, so no separate atomic is needed: the mutex itself
    // prevents data races on last_cleanup_ and, because cleanup only fires at
    // most once per window_seconds, the extra lock overhead is negligible.
    const auto now = std::chrono::steady_clock::now();
    if (now - last_cleanup_ >= std::chrono::seconds(config_.window_seconds)) {
        cleanupExpiredUnlocked();
        last_cleanup_ = now;
    }

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        if (static_cast<int>(clients_.size()) > = config_.max_clients) {
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
    return static_cast<int>(clients_.size());
}

void CostBasedRateLimiter::cleanupExpired()
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    cleanupExpiredUnlocked();
}

void CostBasedRateLimiter::cleanupExpiredUnlocked()
{
    // Caller must hold clients_mutex_.
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
