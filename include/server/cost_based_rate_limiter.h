/**
 * @file cost_based_rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class OperationType {
    SIMPLE_GET,      ///< Simple key/value or point lookup — cost 1.
    COMPLEX_QUERY,   ///< Multi-predicate or join query — cost 10.
    VECTOR_SEARCH,   ///< ANN / embedding similarity search — cost 20.
    LLM_COMPLETION,  ///< LLM inference / completion call — cost 100.
    CUSTOM           ///< Caller supplies an explicit cost via allowRequest().
};

/**
 * @brief Default Cost For.
 * @param[in] op Input parameter.
 * @return Return value.
 * @details Implements defaultCostFor without additional internal calls.
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

class CostBasedRateLimiter {
public:
    struct Config {
        size_t budget_per_window = 1000;

        uint32_t window_seconds = 60;

        size_t max_clients = 10000;
    };

    /**
     * @brief Cost Based Rate Limiter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CostBasedRateLimiter(const Config& config);

    /**
     * @brief Allow Request.
     * @param[in] client_id Identifier of the client.
     * @param[in] op Input parameter.
     * @return True when the operation succeeds.
     */
    bool allowRequest(const std::string& client_id, OperationType op);

    /**
     * @brief Allow Request.
     * @param[in] client_id Identifier of the client.
     * @param[in] cost Input parameter.
     * @return True when the operation succeeds.
     */
    bool allowRequest(const std::string& client_id, size_t cost);

    /**
     * @brief Get Remaining Budget.
     * @param[in] client_id Identifier of the client.
     * @return Return value.
     */
    size_t getRemainingBudget(const std::string& client_id) const;

    /**
     * @brief Get Active Clients.
     * @return Return value.
     */
    size_t getActiveClients() const;

    uint64_t getTotalCostConsumed() const {
        return total_cost_consumed_.load(std::memory_order_relaxed);
    }

    uint64_t getTotalRequests() const {
        return total_requests_.load(std::memory_order_relaxed);
    }

    uint64_t getTotalRejections() const {
        return total_rejections_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Cleanup Expired.
     */
    void cleanupExpired();

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:
    struct ClientBudget {
        size_t remaining = 0;
        std::chrono::steady_clock::time_point window_start;

        /**
         * @brief Client Budget.
         * @param[in] budget Input parameter.
         * @return Return value.
         */
        explicit ClientBudget(size_t budget)
            : remaining(budget)
            , window_start(std::chrono::steady_clock::now())
        {}
    };

    /**
     * @brief Refresh Window.
     * @param[in,out] budget Input/output parameter.
     */
    void refreshWindow(ClientBudget& budget) const;

    /**
     * @brief Compute Effective Remaining.
     * @param[in] budget Input parameter.
     * @return Return value.
     */
    size_t computeEffectiveRemaining(const ClientBudget& budget) const;

    /**
     * @brief Cleanup Expired Unlocked.
     */
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
