/**
 * @file parallel_executor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "storage/base_entity.h"
#include "utils/expected.h"

namespace themis {

// ============================================================================
// ParallelExecutor
// ============================================================================

class ParallelExecutor {
public:
    // ========================================================================
    // Configuration
    // ========================================================================

    struct ParallelConfig {
        size_t max_threads = std::thread::hardware_concurrency();

        size_t morsel_size = 1'024;

        bool enable_parallel_scan = true;

        bool enable_parallel_join = true;

        bool enable_parallel_aggregate = true;
    };

    // ========================================================================
    // Data types
    // ========================================================================

    using Table = std::vector<BaseEntity>;

    using FilterFn = std::function<bool(const BaseEntity&)>;

    struct JoinTuple {
        BaseEntity left;   ///< Row from the left (probe) side.
        BaseEntity right;  ///< Row from the right (build) side.
    };

    struct JoinSpec {
        std::string left_key;   ///< Field name on the left (probe) side.
        std::string right_key;  ///< Field name on the right (build) side.
    };

    enum class AggregateFunction {
        Count,          ///< COUNT(*) – counts all input rows (field is ignored).
        Sum,            ///< SUM(field)
        Avg,            ///< AVG(field)
        Min,            ///< MIN(field)
        Max,            ///< MAX(field)
    };

    struct AggregateSpec {
        std::string field;               ///< Field to aggregate (ignored for Count).
        AggregateFunction function;      ///< Aggregation function.
        std::vector<std::string> group_by; ///< Optional GROUP BY fields.
    };

    using AggregateResult = std::unordered_map<std::string, double>;

    // ========================================================================
    // Construction
    // ========================================================================

    ParallelExecutor();
    /**
     * @brief Parallel Executor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ParallelExecutor(ParallelConfig config);

    const ParallelConfig& getConfig() const noexcept { return config_; }
    /**
     * @brief Set Config.
     * @param[in] cfg Input parameter.
     * @details Calls: validateConfig().
     */
    void setConfig(const ParallelConfig& cfg) {
        config_ = cfg;
        validateConfig(config_);
    }

    // ========================================================================
    // Public API
    // ========================================================================

    Result<Table> parallelScan(
        const Table&    input,
        const FilterFn& filter,
        size_t          num_threads = 0) const;

    Result<std::vector<JoinTuple>> parallelHashJoin(
        const Table&    left,
        const Table&    right,
        const JoinSpec& spec,
        size_t          num_threads = 0) const;

    Result<AggregateResult> parallelAggregate(
        const Table&         input,
        const AggregateSpec& spec,
        size_t               num_threads = 0) const;

private:
    ParallelConfig config_;

    /**
     * @brief Resolve Threads.
     * @param[in] requested Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t resolveThreads(size_t requested) const noexcept;

    /**
     * @brief Validate Config.
     * @param[in,out] cfg Input/output parameter.
     * @note Exception safety: noexcept.
     */
    static void validateConfig(ParallelConfig& cfg) noexcept;

    /**
     * @brief ── Internal sequential helpers ─────────────────────────────────────────
     * @param[in] input Input parameter.
     * @param[in] filter Input parameter.
     * @return Return value.
     */

    static Table sequentialScan(
        const Table& input, const FilterFn& filter);

    /**
     * @brief Sequential Hash Join.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    static std::vector<JoinTuple> sequentialHashJoin(
        const Table& left, const Table& right, const JoinSpec& spec);

    /**
     * @brief Sequential Aggregate.
     * @param[in] input Input parameter.
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    static AggregateResult sequentialAggregate(
        const Table& input, const AggregateSpec& spec);

    // ── Partial aggregate bookkeeping ───────────────────────────────────────

    struct PartialAgg {
        double sum   = 0.0;
        double count = 0.0;
        double min   = std::numeric_limits<double>::max();
        double max   = std::numeric_limits<double>::lowest();
    };

    using PartialMap = std::unordered_map<std::string, PartialAgg>;

    /**
     * @brief Merge Partial.
     * @param[in,out] dst Input/output parameter.
     * @param[in] src Input parameter.
     */
    static void mergePartial(PartialMap& dst, const PartialMap& src);
    /**
     * @brief Finalise.
     * @param[in] p Input parameter.
     * @param[in] fn Input parameter.
     * @return Return value.
     */
    static double finalise(const PartialAgg& p, AggregateFunction fn);

    /**
     * @brief Group Key.
     * @param[in] e Input parameter.
     * @param[in] group_by Input parameter.
     * @return Return value.
     */
    static std::string groupKey(
        const BaseEntity& e, const std::vector<std::string>& group_by);
};

} // namespace themis
