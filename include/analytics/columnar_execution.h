/**
 * @file columnar_execution.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/** @brief Variant covering all column value types. */
using ColumnValue = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

// ============================================================================
// SelectionVector
// ============================================================================

/**
 * @brief A vector of selected row indices for late materialization.
 *
 * Operators work with a SelectionVector to avoid physically copying rows
 * until necessary.  This technique is adopted from vectorized databases
 * (MonetDB, DuckDB) and amortises predicate evaluation cost.
 */
class SelectionVector {
public:
    SelectionVector() = default;
    explicit SelectionVector(size_t capacity);

    /** Reset to a dense "select all" over @p total_rows rows. */
    void reset(size_t total_rows);

    void    push_back(uint32_t idx);
    size_t  size() const noexcept;
    bool    empty() const noexcept;
    uint32_t operator[](size_t pos) const;
    const std::vector<uint32_t>& indices() const noexcept;

    /** Construct a dense "select all" selection for @p n rows. */
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

/**
 * @brief A single typed column in columnar layout.
 *
 * Numeric data (Int64, Double) is stored in a contiguous std::vector that
 * allows compiler auto-vectorization.  A null bitmap tracks missing values.
 */
class Column {
public:
    Column() = default;
    Column(std::string name, ColumnType type);

    const std::string& name() const noexcept { return name_; }
    ColumnType         type() const noexcept { return type_; }
    size_t             size() const noexcept { return row_count_; }

    bool isNull(size_t row) const;

    // Typed data access (unchecked – caller must verify type())
    const std::vector<int64_t>&     int64Data()  const noexcept { return int64_data_;  }
    const std::vector<double>&      doubleData() const noexcept { return double_data_; }
    const std::vector<std::string>& stringData() const noexcept { return string_data_; }
    const std::vector<bool>&        boolData()   const noexcept { return bool_data_;   }

    /** Returns true when at least one null has been appended.
     *  Use this in SIMD fast-path guards instead of nullBitmap().empty(),
     *  since null_bitmap_ is always populated regardless of whether any
     *  row is actually null. */
    bool                            hasNulls()   const noexcept { return has_nulls_; }

    /** Null bitmap — one entry per row; true == null.
     *  Always populated (never empty for a non-empty column).
     *  Call hasNulls() first to check whether any null is present. */
    const std::vector<bool>&        nullBitmap() const noexcept { return null_bitmap_; }

    // Typed append
    void appendInt64(int64_t     value, bool is_null = false);
    void appendDouble(double     value, bool is_null = false);
    void appendString(std::string value, bool is_null = false);
    void appendBool(bool         value, bool is_null = false);
    void appendNull();

    /** Generic value access – slower; use typed accessors in hot paths. */
    ColumnValue get(size_t row) const;

    void reserve(size_t n);
    void clear();

    /** Return a new Column containing only the rows in @p sel. */
    std::shared_ptr<Column> filter(const SelectionVector& sel) const;

    /** Return a shallow copy of rows [offset, offset+length). */
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

/**
 * @brief A horizontal batch of columns (similar to Arrow RecordBatch).
 *
 * The default batch size is 1 024 tuples – a size that fits comfortably in
 * L1/L2 cache on modern CPUs.  Each operator in the pipeline receives and
 * emits a ColumnBatch.  Lazy evaluation is achieved via SelectionVector:
 * FilterOperator attaches a selection rather than copying rows.
 */
class ColumnBatch {
public:
    /** Default batch size (tuples). Balances cache and overhead. */
    static constexpr size_t kDefaultBatchSize = 1024;

    ColumnBatch() = default;
    explicit ColumnBatch(size_t row_count);

    // Column management
    void addColumn(std::shared_ptr<Column> col);
    bool hasColumn(const std::string& name) const;
    std::shared_ptr<Column> getColumn(const std::string& name) const;
    std::shared_ptr<Column> getColumnAt(size_t idx) const;
    size_t columnCount() const noexcept;
    const std::vector<std::shared_ptr<Column>>& columns() const noexcept;

    // Row count
    size_t rowCount() const noexcept { return row_count_; }

    // Lazy selection (produced by FilterOperator)
    void setSelection(const SelectionVector& sel);
    const SelectionVector& selection() const noexcept { return selection_; }
    bool   hasSelection() const noexcept { return has_selection_; }
    size_t selectedRowCount() const noexcept;

    /**
     * @brief Materialize: apply the selection vector and return a new,
     * dense batch without a selection vector.
     */
    ColumnBatch materialize() const;

    /** Split into sub-batches of at most @p max_rows_per_batch rows. */
    std::vector<ColumnBatch> split(size_t max_rows_per_batch) const;

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

/**
 * @brief A single comparison predicate for FilterOperator.
 *
 * Comparisons are evaluated in tight inner loops that the compiler can
 * auto-vectorize for numeric types.
 */
struct Predicate {
    enum class Op { Eq, Ne, Lt, Le, Gt, Ge, IsNull, IsNotNull };

    std::string  column;
    Op           op    = Op::Eq;
    ColumnValue  value;   // unused for IsNull / IsNotNull

    // Convenience factories
    static Predicate eq(std::string col, ColumnValue val);
    static Predicate ne(std::string col, ColumnValue val);
    static Predicate lt(std::string col, ColumnValue val);
    static Predicate le(std::string col, ColumnValue val);
    static Predicate gt(std::string col, ColumnValue val);
    static Predicate ge(std::string col, ColumnValue val);
    static Predicate isNull(std::string col);
    static Predicate isNotNull(std::string col);
};

// ============================================================================
// AggregateSpec
// ============================================================================

/**
 * @brief Specification for one aggregation column produced by AggregateOperator.
 */
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

/**
 * @brief Vectorized filter operator with late materialization.
 *
 * Returns a ColumnBatch that shares the input columns and carries a
 * SelectionVector.  No row data is copied until ColumnBatch::materialize()
 * is called.  Multiple predicates are combined with AND.
 */
class FilterOperator {
public:
    explicit FilterOperator(std::vector<Predicate> predicates);

    /** Apply predicates; returns batch with a SelectionVector attached. */
    ColumnBatch execute(const ColumnBatch& input) const;

    size_t predicateCount() const noexcept { return predicates_.size(); }

private:
    SelectionVector evalPredicate(const ColumnBatch& batch,
                                  const Predicate&   pred) const;

    std::vector<Predicate> predicates_;
};

// ============================================================================
// ProjectOperator
// ============================================================================

/**
 * @brief Vectorized projection operator (zero-copy column sharing).
 *
 * Returns a new ColumnBatch containing only the requested columns.
 * Column objects are shared (not copied) so this is O(k) where k is the
 * number of projected columns.
 */
class ProjectOperator {
public:
    explicit ProjectOperator(std::vector<std::string> column_names);

    ColumnBatch execute(const ColumnBatch& input) const;

private:
    std::vector<std::string> column_names_;
};

// ============================================================================
// AggregateOperator
// ============================================================================

/**
 * @brief Vectorized aggregation operator.
 *
 * Supports COUNT(*), SUM, AVG, MIN, MAX, COUNT_DISTINCT with optional
 * GROUP BY.  Without GROUP BY the whole batch collapses to a single row.
 * With GROUP BY a hash map is used for grouping; the result is one row
 * per distinct key combination.
 */
class AggregateOperator {
public:
    explicit AggregateOperator(std::vector<AggregateSpec> specs);

    ColumnBatch execute(const ColumnBatch& input) const;

    size_t specCount() const noexcept { return specs_.size(); }

private:
    ColumnBatch aggregateAll(const ColumnBatch& input) const;
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

/**
 * @brief Materializing sort operator.
 *
 * Sorts the batch (after materializing any pending selection vector) by the
 * specified keys.  Produces a new dense ColumnBatch in sorted order.
 */
class SortOperator {
public:
    struct SortKey {
        std::string column;
        bool        ascending = true;
    };

    explicit SortOperator(std::vector<SortKey> keys);

    ColumnBatch execute(const ColumnBatch& input) const;

private:
    std::vector<SortKey> keys_;
};

// ============================================================================
// VectorizedPipeline
// ============================================================================

/**
 * @brief A composable pipeline of vectorized operators.
 *
 * Stages are applied left-to-right.  FilterOperator produces lazy
 * SelectionVectors; they are materialized automatically when a non-lazy
 * stage (Aggregate, Sort) is reached.
 *
 * Example:
 * @code
 *   VectorizedPipeline pipeline;
 *   pipeline
 *       .addFilter({Predicate::gt("amount", 100.0)})
 *       .addProject({"region", "amount"})
 *       .addAggregate({{
 *           .result_name  = "total",
 *           .input_column = "amount",
 *           .function     = AggregateSpec::Function::Sum,
 *           .group_by     = {"region"}
 *       }});
 *
 *   ColumnBatch result = pipeline.execute(input_batch);
 * @endcode
 */
class VectorizedPipeline {
public:
    VectorizedPipeline() = default;

    VectorizedPipeline& addFilter(std::vector<Predicate> predicates);
    VectorizedPipeline& addProject(std::vector<std::string> column_names);
    VectorizedPipeline& addAggregate(std::vector<AggregateSpec> specs);
    VectorizedPipeline& addSort(std::vector<SortOperator::SortKey> keys);

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

/**
 * @brief Entry-point for the columnar execution engine.
 *
 * Manages configuration and per-query execution statistics.  Delegates
 * actual work to the operator classes above.
 *
 * Performance targets (vs row-wise execution):
 *   - 5–10× faster aggregations over numeric columns
 *   - 3–5× faster filtering
 *   - Batch size 1 024 tuples balances cache reuse and call overhead
 *
 * Usage:
 * @code
 *   ColumnarExecutionEngine engine;
 *
 *   ColumnBatch batch(1000);
 *   // ... populate columns ...
 *
 *   auto result = engine.execute(
 *       batch,
 *       VectorizedPipeline{}
 *           .addFilter({Predicate::gt("price", 50.0)})
 *           .addAggregate({{
 *               .result_name  = "total",
 *               .input_column = "price",
 *               .function     = AggregateSpec::Function::Sum,
 *               .group_by     = {"category"}
 *           }}));
 * @endcode
 */
class ColumnarExecutionEngine {
public:
    struct Config {
        size_t batch_size       = ColumnBatch::kDefaultBatchSize;
        bool   enable_simd      = true;
        size_t max_memory_bytes = 512ULL * 1024 * 1024;  // 512 MB soft limit
    };

    ColumnarExecutionEngine();
    explicit ColumnarExecutionEngine(const Config& config);
    ~ColumnarExecutionEngine() = default;

    /** Execute a pipeline over a single ColumnBatch. */
    ColumnBatch execute(const ColumnBatch& input, const VectorizedPipeline& pipeline);

    /** Execute a pipeline over multiple batches and return all results. */
    std::vector<ColumnBatch> executeBatched(const std::vector<ColumnBatch>& batches,
                                             const VectorizedPipeline& pipeline);

    // Convenience single-operator shortcuts
    ColumnBatch filter(const ColumnBatch& input, std::vector<Predicate> predicates);
    ColumnBatch aggregate(const ColumnBatch& input, std::vector<AggregateSpec> specs);
    ColumnBatch project(const ColumnBatch& input, std::vector<std::string> columns);
    ColumnBatch sort(const ColumnBatch& input, std::vector<SortOperator::SortKey> keys);

    /** Statistics gathered over the lifetime of this engine (or since resetStats()). */
    struct ExecutionStats {
        size_t batches_processed = 0;
        size_t rows_in           = 0;
        size_t rows_out          = 0;
        double elapsed_ms        = 0.0;
    };

    const ExecutionStats& lastStats() const noexcept { return stats_; }
    void resetStats() noexcept;

    const Config& config() const noexcept { return config_; }

private:
    Config         config_;
    ExecutionStats stats_;
};

}  // namespace analytics
}  // namespace themisdb
