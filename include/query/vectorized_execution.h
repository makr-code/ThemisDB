/**
 * @file vectorized_execution.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Vectorized Execution Engine – Query Module Facade
 *
 * Provides column-store style batch processing for the AQL query pipeline.
 * Rows stored as nlohmann::json objects are converted to a columnar layout
 * (ColumnBatch), processed by the vectorized operator pipeline in
 * analytics/columnar_execution.h, and materialized back to JSON results.
 *
 * Architecture:
 *   VectorizedExecutionEngine          – entry-point; drives batched execution
 *   VectorizedQueryPlan                – composable operator stages
 *   VectorizedPredicate                – filter predicate for FILTER stage
 *   VectorizedAggregation              – aggregation spec for AGGREGATE stage
 *   VectorizedSortKey                  – sort key spec for SORT stage
 *
 * Integration:
 *   The engine accepts any std::vector<nlohmann::json> row set (e.g. the
 *   result of a full-collection scan) and a VectorizedQueryPlan describing
 *   what to compute.  Internally it delegates to
 *   themisdb::analytics::ColumnarExecutionEngine, which uses late-materialization
 *   via SelectionVector for filter operators, keeping CPU cache hot.
 *
 * Thread safety:
 *   VectorizedExecutionEngine instances are NOT thread-safe.
 *   Use one instance per thread or protect with an external mutex.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "analytics/columnar_execution.h"
#include "utils/expected.h"

namespace themis {
namespace query {

// ============================================================================
// VectorizedPredicate
// ============================================================================

struct VectorizedPredicate {
    enum class Op { Eq, Ne, Lt, Le, Gt, Ge, IsNull, IsNotNull };

    std::string    field = {};
    Op             op    = Op::Eq;
    nlohmann::json value;  // unused for IsNull / IsNotNull

    // Convenience factories
    /**
     * @brief Eq.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate eq(std::string field, nlohmann::json value);
    /**
     * @brief Ne.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate ne(std::string field, nlohmann::json value);
    /**
     * @brief Lt.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate lt(std::string field, nlohmann::json value);
    /**
     * @brief Le.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate le(std::string field, nlohmann::json value);
    /**
     * @brief Gt.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate gt(std::string field, nlohmann::json value);
    /**
     * @brief Ge.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate ge(std::string field, nlohmann::json value);
    /**
     * @brief Is Null.
     * @param[in] field Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate isNull(std::string field);
    /**
     * @brief Is Not Null.
     * @param[in] field Input parameter.
     * @return Return value.
     */
    static VectorizedPredicate isNotNull(std::string field);
};

// ============================================================================
// VectorizedAggregation
// ============================================================================

struct VectorizedAggregation {
    enum class Function { Count, Sum, Avg, Min, Max, CountDistinct };

    std::string              result_field;
    std::string              input_field;    // empty for Count(*)
    Function                 function   = Function::Count;
    std::vector<std::string> group_by;       // shared across specs in one call
};

// ============================================================================
// VectorizedSortKey
// ============================================================================

struct VectorizedSortKey {
    std::string field;
    bool        ascending = true;
};

// ============================================================================
// VectorizedQueryPlan
// ============================================================================

class VectorizedQueryPlan {
public:
    VectorizedQueryPlan() = default;

    /**
     * @brief Add Filter.
     * @param[in] predicates Input parameter.
     * @return Return value.
     */
    VectorizedQueryPlan& addFilter(std::vector<VectorizedPredicate> predicates);
    /**
     * @brief Add Project.
     * @param[in] fields Input parameter.
     * @return Return value.
     */
    VectorizedQueryPlan& addProject(std::vector<std::string> fields);
    /**
     * @brief Add Aggregate.
     * @param[in] aggregations Input parameter.
     * @return Return value.
     */
    VectorizedQueryPlan& addAggregate(std::vector<VectorizedAggregation> aggregations);
    /**
     * @brief Add Sort.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    VectorizedQueryPlan& addSort(std::vector<VectorizedSortKey> keys);

    /**
     * @brief Set Limit.
     * @param[in] n Input parameter.
     * @return Return value.
     */
    VectorizedQueryPlan& setLimit(size_t n);

    size_t                stageCount() const noexcept { return stages_.size(); }
    std::optional<size_t> limit()      const noexcept { return limit_; }

    // Stage variant types (public for VectorizedExecutionEngine access)
    struct FilterStage  { std::vector<VectorizedPredicate>    predicates; };
    struct ProjectStage { std::vector<std::string>            fields; };
    struct AggStage     { std::vector<VectorizedAggregation>  aggregations; };
    struct SortStage    { std::vector<VectorizedSortKey>       keys; };

    enum class StageType { Filter, Project, Aggregate, Sort };

    struct Stage {
        StageType    type;
        FilterStage  filter;
        ProjectStage project;
        AggStage     aggregate;
        SortStage    sort;
    };

    const std::vector<Stage>& stages() const noexcept { return stages_; }

private:
    std::vector<Stage>    stages_;
    std::optional<size_t> limit_;
};

// ============================================================================
// VectorizedExecutionEngine
// ============================================================================

class VectorizedExecutionEngine {
public:
    struct Config {
        size_t batch_size       = themisdb::analytics::ColumnBatch::kDefaultBatchSize;
        bool   enable_simd      = true;
        size_t max_memory_bytes = 512ULL * 1024 * 1024;  // 512 MB soft limit
    };

    VectorizedExecutionEngine();
    /**
     * @brief Vectorized Execution Engine.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit VectorizedExecutionEngine(const Config& config);
    ~VectorizedExecutionEngine() = default;

    /**
     * @brief Execute.
     * @param[in] rows Input parameter.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    Result<std::vector<nlohmann::json>> execute(
        const std::vector<nlohmann::json>& rows,
        const VectorizedQueryPlan&         plan);

    /**
     * @brief Filter.
     * @param[in] rows Input parameter.
     * @param[in] predicates Input parameter.
     * @return Return value.
     */
    Result<std::vector<nlohmann::json>> filter(
        const std::vector<nlohmann::json>& rows,
        std::vector<VectorizedPredicate>   predicates);

    /**
     * @brief Aggregate.
     * @param[in] rows Input parameter.
     * @param[in] aggregations Input parameter.
     * @return Return value.
     */
    Result<std::vector<nlohmann::json>> aggregate(
        const std::vector<nlohmann::json>&  rows,
        std::vector<VectorizedAggregation>  aggregations);

    /**
     * @brief Project.
     * @param[in] rows Input parameter.
     * @param[in] fields Input parameter.
     * @return Return value.
     */
    Result<std::vector<nlohmann::json>> project(
        const std::vector<nlohmann::json>& rows,
        std::vector<std::string>           fields);

    /**
     * @brief Sort.
     * @param[in] rows Input parameter.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    Result<std::vector<nlohmann::json>> sort(
        const std::vector<nlohmann::json>& rows,
        std::vector<VectorizedSortKey>     keys);

    struct ExecStats {
        size_t batches_processed = 0;
        size_t rows_in           = 0;
        size_t rows_out          = 0;
        double elapsed_ms        = 0.0;
    };

    const ExecStats& lastStats() const noexcept { return stats_; }
    /**
     * @brief Reset Stats.
     * @note Exception safety: noexcept.
     */
    void             resetStats() noexcept;

    const Config& config() const noexcept { return config_; }

private:
    Config    config_;
    ExecStats stats_;

    /**
     * @brief ── JSON ↔ ColumnBatch conversion ──────────────────────────────────────
     * @param[in] rows Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] count Input parameter.
     * @return Return value.
     */

    static themisdb::analytics::ColumnBatch jsonToColumnBatch(
        const std::vector<nlohmann::json>& rows,
        size_t offset,
        size_t count);

    /**
     * @brief Column Batch To Json.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    static std::vector<nlohmann::json> columnBatchToJson(
        const themisdb::analytics::ColumnBatch& batch);

    /**
     * @brief ── Plan translation ───────────────────────────────────────────────────
     * @param[in] plan Input parameter.
     * @return Return value.
     */

    static themisdb::analytics::VectorizedPipeline buildPipeline(
        const VectorizedQueryPlan& plan);

    /**
     * @brief Translate Predicate.
     * @param[in] pred Input parameter.
     * @return Return value.
     */
    static themisdb::analytics::Predicate translatePredicate(
        const VectorizedPredicate& pred);

    /**
     * @brief Json To Column Value.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static themisdb::analytics::ColumnValue jsonToColumnValue(
        const nlohmann::json& val);
};

}  // namespace query
}  // namespace themis
