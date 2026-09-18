/**
 * @file runtime_reoptimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/adaptive_optimizer.h"
#include <chrono>
#include <memory>
#include <string>

namespace themis {

using AdaptiveQueryStats = ::themis::query::AdaptiveQueryStats;
using AdaptivePlanSelector = ::themis::query::AdaptivePlanSelector;

class RuntimeReoptimizer {
public:
    struct ExecutionContext {
        std::string query_hash;
        size_t estimated_rows = 0;
        std::chrono::steady_clock::time_point start_time;
    };

    class ExecutionGuard {
    public:
        ExecutionGuard(RuntimeReoptimizer& owner, ExecutionContext ctx);
        ExecutionGuard(const ExecutionGuard&) = delete;
        ExecutionGuard& operator=(const ExecutionGuard&) = delete;
        ExecutionGuard(ExecutionGuard&&) noexcept;
        ExecutionGuard& operator=(ExecutionGuard&&) noexcept;
        ~ExecutionGuard();

        /**
         * @brief Finish.
         * @param[in] actual_rows Input parameter.
         */
        void finish(size_t actual_rows);

        const ExecutionContext& context() const { return ctx_; }

    private:
        RuntimeReoptimizer* owner_ = nullptr;
        ExecutionContext ctx_;
        size_t actual_rows_ = 0;
        bool finished_ = false;
    };

    RuntimeReoptimizer();

    /**
     * @brief Compute Query Hash.
     * @param[in] aql_text Input parameter.
     * @return Return value.
     */
    static std::string computeQueryHash(const std::string& aql_text);

    /**
     * @brief Begin Execution.
     * @param[in] query_hash Input parameter.
     * @param[in] estimated_rows Input parameter.
     * @return Return value.
     */
    ExecutionContext beginExecution(const std::string& query_hash,
                                   size_t estimated_rows) const;

    /**
     * @brief Begin Execution Guard.
     * @param[in] query_hash Input parameter.
     * @param[in] estimated_rows Input parameter.
     * @return Return value.
     */
    ExecutionGuard beginExecutionGuard(const std::string& query_hash,
                                       size_t estimated_rows);

    bool shouldReoptimize(const std::string& query_hash,
                          size_t rows_so_far,
                          size_t estimated_total,
                          double progress,
                          double threshold = 5.0) const;

    /**
     * @brief Record Execution.
     * @param[in] query_hash Input parameter.
     * @param[in] estimated_rows Input parameter.
     * @param[in] actual_rows Input parameter.
     * @param[in] execution_time_ms Input parameter.
     */
    void recordExecution(const std::string& query_hash,
                         size_t estimated_rows,
                         size_t actual_rows,
                         double execution_time_ms);

    /**
     * @brief Get Adjustment Factor.
     * @param[in] query_hash Input parameter.
     * @return Return value.
     */
    double getAdjustmentFactor(const std::string& query_hash) const;

    bool hasMisestimation(const std::string& query_hash,
                          double threshold = 2.0) const;

    void pruneOldStats(std::chrono::hours retention = std::chrono::hours(24));

    void enable(bool enabled = true);
    /**
     * @brief Is Enabled.
     * @return True when the operation succeeds.
     */
    bool isEnabled() const;

    /**
     * @brief Total Executions.
     * @return Return value.
     */
    size_t totalExecutions() const;

    /**
     * @brief Stats.
     * @return Return value.
     */
    const AdaptiveQueryStats& stats() const;

private:
    std::shared_ptr<AdaptiveQueryStats> stats_;
    std::shared_ptr<AdaptivePlanSelector> selector_;
    bool enabled_ = true;
};

} // namespace themis
