/**
 * @file columnar_execution.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Columnar Execution Engine with Vectorized Operator Pipeline
 *
 * Provides high-performance columnar query execution for analytical workloads,
 * inspired by DuckDB and ClickHouse.  Data is processed in fixed-size batches
 * (default: 1024 tuples) with late materialization via SelectionVector to
 * avoid unnecessary data movement.
 *
 * Architecture overview:
 *   ColumnBatch  – a horizontal slice of data stored column-by-column
 *   SelectionVector – indices of selected rows (lazy filter result)
 *   Column       – a typed columnar buffer with optional null bitmap
 *   FilterOperator   – evaluates predicates; produces SelectionVector
 *   ProjectOperator  – column subset (zero-copy column sharing)
 *   AggregateOperator – vectorized COUNT/SUM/AVG/MIN/MAX/COUNT_DISTINCT
 *   SortOperator     – in-batch sort by one or more columns
 *   VectorizedPipeline – composes operators; materializes lazily
 *   ColumnarExecutionEngine – entry-point with stats tracking
 *
 * Thread safety:
 *   ColumnarExecutionEngine instances are NOT thread-safe.
 *   Use one instance per thread or protect with an external mutex.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// Arena allocator used by AggregateOperator for GROUP BY scratch memory.
#include "detail/memory_pool.h"

namespace themisdb {
namespace analytics {

// ============================================================================
// Column value types
// ============================================================================

using ColumnValue = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

// ============================================================================
// SelectionVector
// ============================================================================

class SelectionVector {
public:
    SelectionVector() = default;
    /**
     * @brief Selection Vector.
     * @param[in] capacity Input parameter.
     * @return Return value.
     */
    explicit SelectionVector(size_t capacity);

    /**
     * @brief Reset the modification detection flag.
     * @param[in] total_rows Input parameter.
     */
    void reset(size_t total_rows);

    /**
     * @brief Push back.
     * @param[in] idx Input parameter.
     */
    void    push_back(uint32_t idx);
    /**
     * @brief Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t  size() const noexcept;
    /**
     * @brief Empty.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool    empty() const noexcept;
    uint32_t operator[](size_t pos) const;
    /**
     * @brief Indices.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const std::vector<uint32_t>& indices() const noexcept;

    /**
     * @brief All.
     * @param[in] n Input parameter.
     * @return Return value.
     */
    static SelectionVector all(size_t n);

private:
    std::vector<uint32_t> indices_;
};

// ============================================================================
// ColumnType
// ============================================================================

enum class ColumnType { Int64, Double, String, Bool, Null };

// ============================================================================
// Column
// ============================================================================

class Column {
public:
    Column() = default;
    Column(std::string name, ColumnType type);

    const std::string& name() const noexcept { return name_; }
    ColumnType         type() const noexcept { return type_; }
    size_t             size() const noexcept { return row_count_; }

    /**
     * @brief Is Null.
     * @param[in] row Input parameter.
     * @return True when the operation succeeds.
     */
    bool isNull(size_t row) const;

    // Typed data access (unchecked – caller must verify type())
    const std::vector<int64_t>&     int64Data()  const noexcept { return int64_data_;  }
    const std::vector<double>&      doubleData() const noexcept { return double_data_; }
    const std::vector<std::string>& stringData() const noexcept { return string_data_; }
    const std::vector<bool>&        boolData()   const noexcept { return bool_data_;   }

    bool                            hasNulls()   const noexcept { return has_nulls_; }

    const std::vector<bool>&        nullBitmap() const noexcept { return null_bitmap_; }

    // Typed append
    void appendInt64(int64_t     value, bool is_null = false);
    void appendDouble(double     value, bool is_null = false);
    void appendString(std::string value, bool is_null = false);
    void appendBool(bool         value, bool is_null = false);
    /**
     * @brief Append Null.
     */
    void appendNull();

    /**
     * @brief Get.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    ColumnValue get(size_t row) const;

    /**
     * @brief Reserve.
     * @param[in] n Input parameter.
     */
    void reserve(size_t n);
    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Filter.
     * @param[in] sel Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Column> filter(const SelectionVector& sel) const;

    /**
     * @brief Slice.
     * @param[in] offset Input parameter.
     * @param[in] length Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Column> slice(size_t offset, size_t length) const;

private:
    std::string             name_;
    ColumnType              type_        = ColumnType::Null;
    std::vector<int64_t>    int64_data_;
    std::vector<double>     double_data_;
    std::vector<std::string> string_data_;
    std::vector<bool>       bool_data_;
    std::vector<bool>       null_bitmap_;   // true == null
    bool                    has_nulls_ = false;  // true iff at least one null was ever appended
    size_t                  row_count_ = 0;
};

// ============================================================================
// ColumnBatch
// ============================================================================

class ColumnBatch {
public:
    static constexpr size_t kDefaultBatchSize = 1024;

    ColumnBatch() = default;
    /**
     * @brief Column Batch.
     * @param[in] row_count Input parameter.
     * @return Return value.
     */
    explicit ColumnBatch(size_t row_count);

    // Column management
    /**
     * @brief Add Column.
     * @param[in] col Input parameter.
     */
    void addColumn(std::shared_ptr<Column> col);
    /**
     * @brief Has Column.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasColumn(const std::string& name) const;
    /**
     * @brief Get Column.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Column> getColumn(const std::string& name) const;
    /**
     * @brief Get Column At.
     * @param[in] idx Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Column> getColumnAt(size_t idx) const;
    /**
     * @brief Column Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t columnCount() const noexcept;
    /**
     * @brief Columns.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const std::vector<std::shared_ptr<Column>>& columns() const noexcept;

    // Row count
    size_t rowCount() const noexcept { return row_count_; }

    /**
     * @brief Set Selection.
     * @param[in] sel Input parameter.
     */
    void setSelection(const SelectionVector& sel);
    const SelectionVector& selection() const noexcept { return selection_; }
    bool   hasSelection() const noexcept { return has_selection_; }
    /**
     * @brief Selected Row Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t selectedRowCount() const noexcept;

    /**
     * @brief Materialize.
     * @return Return value.
     */
    ColumnBatch materialize() const;

    /**
     * @brief Split.
     * @param[in] max_rows_per_batch Input parameter.
     * @return Return value.
     */
    std::vector<ColumnBatch> split(size_t max_rows_per_batch) const;

    /**
     * @brief Clear.
     */
    void clear();

private:
    std::vector<std::shared_ptr<Column>>        columns_;
    std::unordered_map<std::string, size_t>     column_index_;
    size_t                                      row_count_     = 0;
    SelectionVector                             selection_;
    bool                                        has_selection_ = false;
};

// ============================================================================
// Predicate
// ============================================================================

struct Predicate {
    enum class Op { Eq, Ne, Lt, Le, Gt, Ge, IsNull, IsNotNull };

    std::string  column;
    Op           op    = Op::Eq;
    ColumnValue  value;   // unused for IsNull / IsNotNull

    // Convenience factories
    /**
     * @brief Eq.
     * @param[in] col Input parameter.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static Predicate eq(std::string col, ColumnValue val);
    /**
     * @brief Ne.
     * @param[in] col Input parameter.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static Predicate ne(std::string col, ColumnValue val);
    /**
     * @brief Lt.
     * @param[in] col Input parameter.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static Predicate lt(std::string col, ColumnValue val);
    /**
     * @brief Le.
     * @param[in] col Input parameter.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static Predicate le(std::string col, ColumnValue val);
    /**
     * @brief Gt.
     * @param[in] col Input parameter.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static Predicate gt(std::string col, ColumnValue val);
    /**
     * @brief Ge.
     * @param[in] col Input parameter.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static Predicate ge(std::string col, ColumnValue val);
    /**
     * @brief Is Null.
     * @param[in] col Input parameter.
     * @return Return value.
     */
    static Predicate isNull(std::string col);
    /**
     * @brief Is Not Null.
     * @param[in] col Input parameter.
     * @return Return value.
     */
    static Predicate isNotNull(std::string col);
};

// ============================================================================
// AggregateSpec
// ============================================================================

struct AggregateSpec {
    enum class Function { Count, Sum, Avg, Min, Max, CountDistinct };

    std::string              result_name;
    std::string              input_column;    // empty for Count(*)
    Function                 function   = Function::Count;
    std::vector<std::string> group_by;        // shared across specs in one call
};

// ============================================================================
// FilterOperator
// ============================================================================

class FilterOperator {
public:
    /**
     * @brief Filter Operator.
     * @param[in] predicates Input parameter.
     * @return Return value.
     */
    explicit FilterOperator(std::vector<Predicate> predicates);

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    ColumnBatch execute(const ColumnBatch& input) const;

    size_t predicateCount() const noexcept { return predicates_.size(); }

private:
    /**
     * @brief Eval Predicate.
     * @param[in] batch Input parameter.
     * @param[in] pred Input parameter.
     * @return Return value.
     */
    SelectionVector evalPredicate(const ColumnBatch& batch,
                                  const Predicate&   pred) const;

    std::vector<Predicate> predicates_;
};

// ============================================================================
// ProjectOperator
// ============================================================================

class ProjectOperator {
public:
    /**
     * @brief Project Operator.
     * @param[in] column_names Input parameter.
     * @return Return value.
     */
    explicit ProjectOperator(std::vector<std::string> column_names);

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    ColumnBatch execute(const ColumnBatch& input) const;

private:
    std::vector<std::string> column_names_;
};

// ============================================================================
// AggregateOperator
// ============================================================================

class AggregateOperator {
public:
    /**
     * @brief Aggregate Operator.
     * @param[in] specs Input parameter.
     * @return Return value.
     */
    explicit AggregateOperator(std::vector<AggregateSpec> specs);

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    ColumnBatch execute(const ColumnBatch& input) const;

    size_t specCount() const noexcept { return specs_.size(); }

private:
    /**
     * @brief Aggregate All.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    ColumnBatch aggregateAll(const ColumnBatch& input) const;
    /**
     * @brief Aggregate Group By.
     * @param[in] input Input parameter.
     * @param[in] group_cols Input parameter.
     * @return Return value.
     */
    ColumnBatch aggregateGroupBy(const ColumnBatch& input,
                                  const std::vector<std::string>& group_cols) const;

    std::vector<AggregateSpec> specs_;

    // Per-operator arena allocator.  Mutable so const execute() / aggregateGroupBy()
    // can call pool_.reset() at the start of each GROUP BY pass — no allocations
    // escape this class, so the logical const-ness of the operator is preserved.
    mutable ::themisdb::analytics::detail::AnalyticsMemoryPool pool_{
        4ULL * 1024 * 1024};  // 4 MiB initial (GROUP BY scratch, much smaller than OLAP)
};

// ============================================================================
// SortOperator
// ============================================================================

class SortOperator {
public:
    struct SortKey {
        std::string column;
        bool        ascending = true;
    };

    /**
     * @brief Sort Operator.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    explicit SortOperator(std::vector<SortKey> keys);

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    ColumnBatch execute(const ColumnBatch& input) const;

private:
    std::vector<SortKey> keys_;
};

// ============================================================================
// VectorizedPipeline
// ============================================================================

class VectorizedPipeline {
public:
    VectorizedPipeline() = default;

    /**
     * @brief Add Filter.
     * @param[in] predicates Input parameter.
     * @return Return value.
     */
    VectorizedPipeline& addFilter(std::vector<Predicate> predicates);
    /**
     * @brief Add Project.
     * @param[in] column_names Input parameter.
     * @return Return value.
     */
    VectorizedPipeline& addProject(std::vector<std::string> column_names);
    /**
     * @brief Add Aggregate.
     * @param[in] specs Input parameter.
     * @return Return value.
     */
    VectorizedPipeline& addAggregate(std::vector<AggregateSpec> specs);
    /**
     * @brief Add Sort.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    VectorizedPipeline& addSort(std::vector<SortOperator::SortKey> keys);

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    ColumnBatch execute(const ColumnBatch& input) const;

    size_t stageCount() const noexcept { return stages_.size(); }

private:
    enum class StageType { Filter, Project, Aggregate, Sort };

    struct Stage {
        StageType type;
        std::shared_ptr<FilterOperator>    filter;
        std::shared_ptr<ProjectOperator>   project;
        std::shared_ptr<AggregateOperator> aggregate;
        std::shared_ptr<SortOperator>      sort;
    };

    std::vector<Stage> stages_;
};

// ============================================================================
// ColumnarExecutionEngine
// ============================================================================

class ColumnarExecutionEngine {
public:
    struct Config {
        size_t batch_size       = ColumnBatch::kDefaultBatchSize;
        bool   enable_simd      = true;
        size_t max_memory_bytes = 512ULL * 1024 * 1024;  // 512 MB soft limit
    };

    ColumnarExecutionEngine();
    /**
     * @brief Columnar Execution Engine.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ColumnarExecutionEngine(const Config& config);
    ~ColumnarExecutionEngine() = default;

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @param[in] pipeline Input parameter.
     * @return Return value.
     */
    ColumnBatch execute(const ColumnBatch& input, const VectorizedPipeline& pipeline);

    /**
     * @brief Execute Batched.
     * @param[in] batches Input parameter.
     * @param[in] pipeline Input parameter.
     * @return Return value.
     */
    std::vector<ColumnBatch> executeBatched(const std::vector<ColumnBatch>& batches,
                                             const VectorizedPipeline& pipeline);

    /**
     * @brief Filter.
     * @param[in] input Input parameter.
     * @param[in] predicates Input parameter.
     * @return Return value.
     */
    ColumnBatch filter(const ColumnBatch& input, std::vector<Predicate> predicates);
    /**
     * @brief Aggregate.
     * @param[in] input Input parameter.
     * @param[in] specs Input parameter.
     * @return Return value.
     */
    ColumnBatch aggregate(const ColumnBatch& input, std::vector<AggregateSpec> specs);
    /**
     * @brief Project.
     * @param[in] input Input parameter.
     * @param[in] columns Input parameter.
     * @return Return value.
     */
    ColumnBatch project(const ColumnBatch& input, std::vector<std::string> columns);
    /**
     * @brief Sort.
     * @param[in] input Input parameter.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    ColumnBatch sort(const ColumnBatch& input, std::vector<SortOperator::SortKey> keys);

    struct ExecutionStats {
        size_t batches_processed = 0;
        size_t rows_in           = 0;
        size_t rows_out          = 0;
        double elapsed_ms        = 0.0;
    };

    const ExecutionStats& lastStats() const noexcept { return stats_; }
    /**
     * @brief Reset Stats.
     * @note Exception safety: noexcept.
     */
    void resetStats() noexcept;

    const Config& config() const noexcept { return config_; }

private:
    Config         config_;
    ExecutionStats stats_;
};

}  // namespace analytics
}  // namespace themisdb
