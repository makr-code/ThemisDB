/**
 * @file vectorized_execution.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief A single comparison predicate for the FILTER stage.
 *
 * Applied over a named JSON field in every processed row.  Multiple
 * predicates in one addFilter() call are combined with AND.
 */
struct VectorizedPredicate {
    enum class Op { Eq, Ne, Lt, Le, Gt, Ge, IsNull, IsNotNull };

    std::string    field = {};
    Op             op    = Op::Eq;
    nlohmann::json value;  // unused for IsNull / IsNotNull

    // Convenience factories
    static VectorizedPredicate eq(std::string field, nlohmann::json value);
    static VectorizedPredicate ne(std::string field, nlohmann::json value);
    static VectorizedPredicate lt(std::string field, nlohmann::json value);
    static VectorizedPredicate le(std::string field, nlohmann::json value);
    static VectorizedPredicate gt(std::string field, nlohmann::json value);
    static VectorizedPredicate ge(std::string field, nlohmann::json value);
    static VectorizedPredicate isNull(std::string field);
    static VectorizedPredicate isNotNull(std::string field);
};

// ============================================================================
// VectorizedAggregation
// ============================================================================

/**
 * @brief Specification for one aggregation column.
 *
 * Mirrors analytics::AggregateSpec but expressed over JSON field names.
 * Supports COUNT(*), SUM, AVG, MIN, MAX, COUNT_DISTINCT with optional
 * GROUP BY.
 */
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

/**
 * @brief A single sort key for the SORT stage.
 */
struct VectorizedSortKey {
    std::string field;
    bool        ascending = true;
};

// ============================================================================
// VectorizedQueryPlan
// ============================================================================

/**
 * @brief A composable pipeline of vectorized operator stages.
 *
 * Stages are applied in the order they are added.  Example:
 * @code
 *   VectorizedQueryPlan plan;
 *   plan
 *       .addFilter({VectorizedPredicate::gt("amount", 100.0)})
 *       .addProject({"region", "amount"})
 *       .addAggregate({{
 *           .result_field = "total",
 *           .input_field  = "amount",
 *           .function     = VectorizedAggregation::Function::Sum,
 *           .group_by     = {"region"}
 *       }});
 * @endcode
 */
class VectorizedQueryPlan {
public:
    VectorizedQueryPlan() = default;

    VectorizedQueryPlan& addFilter(std::vector<VectorizedPredicate> predicates);
    VectorizedQueryPlan& addProject(std::vector<std::string> fields);
    VectorizedQueryPlan& addAggregate(std::vector<VectorizedAggregation> aggregations);
    VectorizedQueryPlan& addSort(std::vector<VectorizedSortKey> keys);

    /** Apply a row-count limit to the final result. */
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

/**
 * @brief Vectorized execution engine for the query module.
 *
 * Accepts a collection of JSON rows and a VectorizedQueryPlan, converts the
 * rows to columnar (ColumnBatch) format, delegates execution to the analytics
 * ColumnarExecutionEngine, and converts results back to JSON rows.
 *
 * Performance targets (vs row-wise iteration):
 *   - 5–10× faster aggregations over numeric fields
 *   - 3–5× faster filter evaluation
 *   - Batch size 1 024 tuples balances L1/L2 cache pressure and overhead
 *
 * Usage:
 * @code
 *   VectorizedExecutionEngine engine;
 *
 *   VectorizedQueryPlan plan;
 *   plan.addFilter({VectorizedPredicate::gt("price", 50.0)})
 *       .addAggregate({{
 *           .result_field = "total",
 *           .input_field  = "price",
 *           .function     = VectorizedAggregation::Function::Sum,
 *           .group_by     = {"category"}
 *       }});
 *
 *   auto result = engine.execute(rows, plan);
 *   if (result) {
 *       for (const auto& row : *result) { ... }
 *   }
 * @endcode
 */
class VectorizedExecutionEngine {
public:
    struct Config {
        size_t batch_size       = themisdb::analytics::ColumnBatch::kDefaultBatchSize;
        bool   enable_simd      = true;
        size_t max_memory_bytes = 512ULL * 1024 * 1024;  // 512 MB soft limit
    };

    VectorizedExecutionEngine();
    explicit VectorizedExecutionEngine(const Config& config);
    ~VectorizedExecutionEngine() = default;

    /**
     * @brief Execute a plan over a JSON row collection.
     *
     * Rows are split into batches of config.batch_size, each batch processed
     * through the operator pipeline, and all results concatenated.
     *
     * @param rows   Input JSON row collection.
     * @param plan   Operator stages to apply.
     * @return       Resulting JSON rows, or an error.
     */
    Result<std::vector<nlohmann::json>> execute(
        const std::vector<nlohmann::json>& rows,
        const VectorizedQueryPlan&         plan);

    /** Convenience: apply one or more filter predicates (AND-combined). */
    Result<std::vector<nlohmann::json>> filter(
        const std::vector<nlohmann::json>& rows,
        std::vector<VectorizedPredicate>   predicates);

    /** Convenience: apply aggregation(s) with optional GROUP BY. */
    Result<std::vector<nlohmann::json>> aggregate(
        const std::vector<nlohmann::json>&  rows,
        std::vector<VectorizedAggregation>  aggregations);

    /** Convenience: retain only named fields in each row. */
    Result<std::vector<nlohmann::json>> project(
        const std::vector<nlohmann::json>& rows,
        std::vector<std::string>           fields);

    /** Convenience: sort rows by one or more fields. */
    Result<std::vector<nlohmann::json>> sort(
        const std::vector<nlohmann::json>& rows,
        std::vector<VectorizedSortKey>     keys);

    /** Statistics gathered since construction or the last resetStats() call. */
    struct ExecStats {
        size_t batches_processed = 0;
        size_t rows_in           = 0;
        size_t rows_out          = 0;
        double elapsed_ms        = 0.0;
    };

    const ExecStats& lastStats() const noexcept { return stats_; }
    void             resetStats() noexcept;

    const Config& config() const noexcept { return config_; }

private:
    Config    config_;
    ExecStats stats_;

    // ── JSON ↔ ColumnBatch conversion ──────────────────────────────────────

    /**
     * @brief Convert a contiguous slice of JSON rows to a ColumnBatch.
     *
     * Column types are inferred from the first non-null value per field.
     * Rows that are missing a field contribute a null entry.
     *
     * @param rows    Full row collection.
     * @param offset  First row index to include.
     * @param count   Number of rows to include.
     * @return        A populated ColumnBatch ready for the operator pipeline.
     */
    static themisdb::analytics::ColumnBatch jsonToColumnBatch(
        const std::vector<nlohmann::json>& rows,
        size_t offset,
        size_t count);

    /**
     * @brief Materialize a ColumnBatch back to a vector of JSON objects.
     *
     * Any pending SelectionVector is applied first (materialization).
     */
    static std::vector<nlohmann::json> columnBatchToJson(
        const themisdb::analytics::ColumnBatch& batch);

    // ── Plan translation ───────────────────────────────────────────────────

    /**
     * @brief Translate a VectorizedQueryPlan into an analytics VectorizedPipeline.
     *
     * Predicate Op, AggregateSpec::Function, and SortKey enumerators are
     * mapped 1-to-1 to their analytics counterparts.
     */
    static themisdb::analytics::VectorizedPipeline buildPipeline(
        const VectorizedQueryPlan& plan);

    /** Map a VectorizedPredicate to an analytics::Predicate. */
    static themisdb::analytics::Predicate translatePredicate(
        const VectorizedPredicate& pred);

    /** Map a nlohmann::json value to a ColumnValue variant. */
    static themisdb::analytics::ColumnValue jsonToColumnValue(
        const nlohmann::json& val);
};

}  // namespace query
}  // namespace themis
