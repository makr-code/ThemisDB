/**
 * @file cost_based_rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace themis {
namespace server {

/**
 * @brief Operation types with pre-defined cost weights.
 *
 * Costs represent the relative resource consumption of each operation,
 * enabling fairer sharing of available budget across heterogeneous
 * workloads and better alignment with usage-based pricing models.
 *
 * | OperationType   | Default cost |
 * |-----------------|-------------|
 * | SIMPLE_GET      | 1           |
 * | COMPLEX_QUERY   | 10          |
 * | VECTOR_SEARCH   | 20          |
 * | LLM_COMPLETION  | 100         |
 * | CUSTOM          | caller-defined |
 */
enum class OperationType {
    SIMPLE_GET,      ///< Simple key/value or point lookup — cost 1.
    COMPLEX_QUERY,   ///< Multi-predicate or join query — cost 10.
    VECTOR_SEARCH,   ///< ANN / embedding similarity search — cost 20.
    LLM_COMPLETION,  ///< LLM inference / completion call — cost 100.
    CUSTOM           ///< Caller supplies an explicit cost via allowRequest().
};

/**
 * @brief Return the default cost weight for an @c OperationType.
 *
 * For OperationType::CUSTOM the function returns 1; the caller must supply
 * the real cost directly to CostBasedRateLimiter::allowRequest().
 */
inline size_t defaultCostFor(OperationType op) {
    switch (op) {
        case OperationType::SIMPLE_GET:     return 1;
        case OperationType::COMPLEX_QUERY:  return 10;
        case OperationType::VECTOR_SEARCH:  return 20;
        case OperationType::LLM_COMPLETION: return 100;
        case OperationType::CUSTOM:         return 1;
    }
    return 1; // unreachable, but satisfies -Wreturn-type
}

/**
 * @brief Cost-Based Rate Limiter.
 *
 * Instead of counting raw requests, this limiter tracks a **cost budget**
 * per client per time window.  Expensive operations consume more budget,
 * preventing them from monopolising shared resources.
 *
 * ### Default operation costs
 * - SIMPLE_GET      =   1 unit
 * - COMPLEX_QUERY   =  10 units
 * - VECTOR_SEARCH   =  20 units
 * - LLM_COMPLETION  = 100 units
 *
 * ### Algorithm
 * Each client (identified by @c client_id) has a fixed-window (tumbling-window)
 * budget.  The budget is fully replenished at the start of each new window.
 * A request is rejected when the remaining budget is insufficient to cover the
 * operation cost.
 *
 * ### Thread-safety
 * All public methods are thread-safe.
 *
 * ### Example
 * @code
 *   CostBasedRateLimiter::Config cfg;
 *   cfg.budget_per_window  = 1000;   // 1 000 cost-units per minute
 *   cfg.window_seconds     = 60;
 *   CostBasedRateLimiter limiter(cfg);
 *
 *   // Simple GET costs 1 unit:
 *   if (!limiter.allowRequest("tenant_a", OperationType::SIMPLE_GET)) {
 *       return HTTP_429;
 *   }
 *
 *   // LLM call costs 100 units:
 *   if (!limiter.allowRequest("tenant_a", OperationType::LLM_COMPLETION)) {
 *       return HTTP_429;
 *   }
 * @endcode
 */
class CostBasedRateLimiter {
public:
    struct Config {
        /// Total cost budget available per client per window.
        size_t budget_per_window = 1000;

        /// Duration of one rate-limit window in seconds.
        uint32_t window_seconds = 60;

        /// Maximum number of distinct clients to track simultaneously.
        /// When exceeded, new clients are rejected (returns false).
        size_t max_clients = 10000;
    };

    explicit CostBasedRateLimiter(const Config& config);

    /**
     * @brief Attempt to consume the cost of @c op from @c client_id's budget.
     *
     * @param client_id  Client identifier (API key, tenant, IP, …).
     * @param op         Operation type; determines cost via defaultCostFor().
     * @return true if budget is sufficient and has been deducted; false if
     *         the client's budget is exhausted for the current window.
     */
    bool allowRequest(const std::string& client_id, OperationType op);

    /**
     * @brief Attempt to consume an explicit @c cost from @c client_id's budget.
     *
     * Use this overload with OperationType::CUSTOM or when a caller-defined
     * cost is required.
     *
     * @param client_id  Client identifier.
     * @param cost       Cost units to consume.
     * @return true if allowed, false if budget insufficient.
     */
    bool allowRequest(const std::string& client_id, size_t cost);

    /**
     * @brief Return the remaining budget for @c client_id in the current window.
     *
     * Returns @c budget_per_window for unknown clients (full budget).
     */
    size_t getRemainingBudget(const std::string& client_id) const;

    /**
     * @brief Return the total number of actively tracked clients.
     */
    size_t getActiveClients() const;

    /**
     * @brief Total cost units consumed (allowed requests only).
     */
    uint64_t getTotalCostConsumed() const {
        return total_cost_consumed_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Total requests seen (allowed + rejected).
     */
    uint64_t getTotalRequests() const {
        return total_requests_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Total requests rejected due to insufficient budget.
     */
    uint64_t getTotalRejections() const {
        return total_rejections_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Evict idle client state that is older than one window.
     *
     * Called automatically on each allowRequest(); exposed for testing.
     */
    void cleanupExpired();

    /**
     * @brief Reset all client budgets and metrics (for testing).
     */
    void reset();

private:
    struct ClientBudget {
        size_t remaining = 0;
        std::chrono::steady_clock::time_point window_start;

        explicit ClientBudget(size_t budget)
            : remaining(budget)
            , window_start(std::chrono::steady_clock::now())
        {}
    };

    /// Ensure @p budget is current for the active window; reset if expired.
    /// Note: modifies budget in-place; callers in const context must use
    /// computeEffectiveRemaining() instead.
    void refreshWindow(ClientBudget& budget) const;

    /// Return the effective remaining budget without modifying @p budget state.
    size_t computeEffectiveRemaining(const ClientBudget& budget) const;

    /// Internal cleanup helper — caller must hold clients_mutex_.
    void cleanupExpiredUnlocked();

    Config config_;

    mutable std::mutex clients_mutex_;
    std::unordered_map<std::string, ClientBudget> clients_;
    std::chrono::steady_clock::time_point last_cleanup_;

    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_rejections_{0};
    std::atomic<uint64_t> total_cost_consumed_{0};
};

} // namespace server
} // namespace themis
