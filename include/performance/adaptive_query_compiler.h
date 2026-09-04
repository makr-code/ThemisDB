/**
 * @file adaptive_query_compiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace themis {
namespace performance {

// ============================================================================
// Lightweight query types for the performance module
// ============================================================================

/**
 * @brief Value type for query parameters and results.
 *
 * Supports the common scalar types used in ThemisDB predicates.
 */
using QueryValue = std::variant<std::monostate,  // NULL
                                int64_t,
                                double,
                                bool,
                                std::string>;

/**
 * @brief Bind parameters supplied at query execution time.
 *
 * Named parameters (e.g. "@age", "@name") are resolved by name.
 */
struct QueryParams {
    std::unordered_map<std::string, QueryValue> bindings;

    QueryParams() = default;

    QueryParams& set(std::string name, QueryValue value) {
        bindings[std::move(name)] = std::move(value);
        return *this;
    }

    const QueryValue* get(const std::string& name) const {
        auto it = bindings.find(name);
        return it == bindings.end() ? nullptr : &it->second;
    }
};

/**
 * @brief A single result row returned by a compiled query.
 */
struct QueryRow {
    std::vector<std::string>  column_names;
    std::vector<QueryValue>   values;

    size_t size() const noexcept { return values.size(); }

    const QueryValue* get(const std::string& col) const {
        for (size_t i = 0; i < column_names.size(); ++i) {
            if (column_names[i] == col) {
              return &values[i];
            }
        }
        return nullptr;
    }
};

/**
 * @brief Result set produced by a query execution.
 */
struct QueryResult {
    std::vector<QueryRow> rows;
    std::string           error;   ///< Non-empty on failure
    bool                  ok = true;

    size_t size() const noexcept { return rows.size(); }
    bool   empty() const noexcept { return rows.empty(); }
};

// ─── Schema types ────────────────────────────────────────────────────────────

/** @brief Value type descriptor for a schema column. */
enum class ColumnType { Int64, Double, Bool, String, Unknown };

/** @brief Schema descriptor for one table column. */
struct ColumnSchema {
    std::string name;
    ColumnType  type        = ColumnType::Unknown;
    bool        nullable    = true;
    bool        has_index   = false;
};

/** @brief Lightweight schema descriptor for a single table. */
struct TableSchema {
    std::string              table_name;
    std::vector<ColumnSchema> columns;

    ColumnType columnType(const std::string& col_name) const {
        for (const auto& c : columns) {
            if (c.name == col_name) {
              return c.type;
            }
        }
        return ColumnType::Unknown;
    }
};

/** @brief Multi-table schema snapshot passed to the compiler. */
struct Schema {
    std::unordered_map<std::string, TableSchema> tables;

    const TableSchema* getTable(const std::string& name) const {
        auto it = tables.find(name);
        return it == tables.end() ? nullptr : &it->second;
    }
};

// ─── ParsedQuery representation ──────────────────────────────────────────────

/** @brief Enumeration of supported query operations. */
enum class QueryOpType {
    Filter,       ///< SELECT … WHERE predicate
    Aggregate,    ///< GROUP BY + aggregate functions (COUNT/SUM/AVG/MIN/MAX)
    Join,         ///< Two-table equi-join
    Projection,   ///< SELECT subset of columns
    Sort,         ///< ORDER BY
    Limit,        ///< LIMIT / OFFSET
    Unknown
};

/** @brief A single predicate clause (e.g. column op value). */
struct Predicate {
    enum class Op { EQ, NEQ, LT, LE, GT, GE, LIKE, IN };

    std::string  column;
    Op           op    = Op::EQ;
    QueryValue   value;            ///< Constant value (monostate = bind param)
    std::string  param_name;       ///< Bind-parameter name when value is monostate
};

/** @brief Lightweight representation of a parsed query. */
struct ParsedQuery {
    std::string   query_text;          ///< Original query string
    std::string   fingerprint;         ///< Structural fingerprint (no literals)
    std::string   table;               ///< Primary source table
    QueryOpType   op_type = QueryOpType::Unknown;

    // Filter
    std::vector<Predicate> predicates; ///< WHERE clause predicates (AND-ed)

    // Projection
    std::vector<std::string> select_columns; ///< Empty = SELECT *

    // Aggregation
    std::string              group_by_column;
    std::string              agg_function;    ///< "COUNT","SUM","AVG","MIN","MAX"
    std::string              agg_column;

    // Join
    std::string              join_table;
    std::string              join_key_left;
    std::string              join_key_right;

    // Sort
    std::string              order_by_column;
    bool                     order_asc = true;

    // Limit
    size_t                   limit  = 0;   ///< 0 = no limit
    size_t                   offset = 0;
};

// ============================================================================
// AdaptiveQueryCompiler
// ============================================================================

/**
 * @brief Adaptive JIT-style compiler for hot ThemisDB queries (v1.8.0).
 *
 * Implements the "Adaptive Query Compilation" roadmap item
 * (performance domain, milestone v1.8.0, roadmap row #86).
 *
 * ## Design
 *
 * The compiler follows the warm-up/specialise pattern used in production
 * JIT compilers:
 *
 *  **Cold path** (execution_count < hot_threshold):
 *    Queries are executed through a generic interpreted dispatch path.
 *    Each call increments the per-fingerprint execution counter.
 *
 *  **Compilation** (execution_count == hot_threshold):
 *    When a query crosses the hot-threshold, `compileSpecialisation()`
 *    analyses the query structure and schema types to generate a
 *    type-specialised `std::function<QueryResult(const QueryParams&)>`.
 *    The specialised closure:
 *      - Hard-codes the query operation type (no per-row virtual dispatch).
 *      - Hard-codes predicate column types (type specialisation).
 *      - Applies constant propagation / expression folding for literal
 *        predicates discovered at compile time.
 *      - Emits simulated LLVM IR and assembly strings used for debugging
 *        and differential testing.
 *
 *  **Hot path** (execution_count > hot_threshold):
 *    The cached specialised function is invoked directly, bypassing the
 *    generic dispatch layer.
 *
 * ## Adaptive Recompilation
 *
 * A background check after every `recompile_check_interval` executions
 * compares the current runtime statistics (result-set cardinality,
 * predicate selectivity) against the snapshot captured at compile time.
 * If the cardinality has drifted by more than `recompile_drift_factor`
 * the cached specialisation is evicted and the query is recompiled on
 * the next hot-path invocation.
 *
 * ## LLVM Backend Guard
 *
 * When the compile-time symbol `THEMIS_HAS_LLVM_JIT` is defined the
 * compilation step may additionally emit real LLVM IR and invoke MCJIT to
 * produce a function pointer to native machine code.  The dispatch logic
 * and all statistics remain identical in both modes.
 *
 * ## Thread Safety
 *
 * All public methods are thread-safe.  Internal state is guarded by a
 * single `std::mutex`.  Each call to `execute()` acquires the lock only
 * for the counter increment and optional compilation step; the hot-path
 * function itself is invoked without holding the lock.
 *
 * ## Usage
 *
 * @code
 *   AdaptiveQueryCompiler compiler;
 *
 *   ParsedQuery q;
 *   q.fingerprint  = "filter:users:age:GE";
 *   q.table        = "users";
 *   q.op_type      = QueryOpType::Filter;
 *   q.predicates   = {{ "age", Predicate::Op::GE, int64_t{18} }};
 *
 *   Schema schema;
 *   schema.tables["users"] = { "users", {{ "age", ColumnType::Int64 }} };
 *
 *   // First 100 calls: interpreted path
 *   for (int i = 0; i < 100; ++i) {
 *       auto result = compiler.execute(q, schema, {});
 *   }
 *
 *   // Call 101+: compiled specialisation
 *   auto result = compiler.execute(q, schema, {});
 *   assert(result.ok);
 *
 *   auto s = compiler.getStats();
 *   assert(s.queries_compiled >= 1);
 * @endcode
 */
class AdaptiveQueryCompiler {
public:
    // =========================================================================
    // Configuration
    // =========================================================================

    /** @brief LLVM-style optimisation level hint. */
    enum class OptLevel { O0 = 0, O1 = 1, O2 = 2, O3 = 3 };

    /**
     * @brief Compiler configuration.
     *
     * All fields have production-ready defaults matching the roadmap spec.
     */
    struct CompilationConfig {
        /** Executions before JIT compilation is triggered. */
        size_t hot_threshold = 100;

        /** LLVM optimisation level applied during native code generation. */
        OptLevel optimization = OptLevel::O3;

        /** Emit SIMD-vectorised loops for batch predicate evaluation. */
        bool enable_vectorization = true;

        /** Emit software prefetch instructions ahead of sequential scans. */
        bool enable_prefetch = true;

        /** Inline helper functions into the compiled query body. */
        bool enable_inlining = true;

        /** Maximum wall-clock time (ms) the compilation step may consume. */
        size_t compilation_timeout_ms = 100;

        /**
         * After how many hot executions to re-check cardinality statistics
         * and decide whether recompilation is necessary.
         */
        size_t recompile_check_interval = 500;

        /**
         * Ratio of (current_rows / baseline_rows) that triggers recompilation.
         * A value of 10 means a 10× cardinality change will force a recompile.
         */
        double recompile_drift_factor = 10.0;
    };

    // =========================================================================
    // CompiledQuery
    // =========================================================================

    /**
     * @brief A compiled, ready-to-execute query specialisation.
     *
     * Produced by `compile()`.  May also be obtained implicitly by calling
     * `execute()` after the query has crossed the hot threshold.
     */
    struct CompiledQuery {
        /** Signature of the compiled execution function. */
        using ExecuteFn = std::function<QueryResult(const QueryParams&)>;

        /** Execution function; null when compilation failed. */
        ExecuteFn execute;

        /** Structural fingerprint of the source query (for cache lookup). */
        std::string fingerprint;

        /** Wall-clock time consumed by compilation, in microseconds. */
        uint64_t compilation_time_us = 0;

        /** Approximate size of the generated code object in bytes. */
        uint64_t code_size_bytes = 0;

        /** Simulated / actual LLVM IR for the compiled query (debug). */
        std::string llvm_ir;

        /** Simulated / actual assembly output (debug). */
        std::string assembly;

        /** Row-count estimate captured at compile time (for drift detection). */
        size_t baseline_row_count = 0;

        /** True when the specialisation was compiled with vectorisation on. */
        bool vectorized = false;

        explicit operator bool() const noexcept { return execute != nullptr; }
    };

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Aggregate compiler statistics. */
    struct CompilationStats {
        /** Number of distinct query fingerprints that have been compiled. */
        size_t queries_compiled = 0;

        /** Number of compilation attempts that failed (timeout or error). */
        size_t compilation_failures = 0;

        /** Total wall-clock time spent in compilation steps (µs). */
        uint64_t total_compilation_time_us = 0;

        /** Estimated average execution speedup over the interpreted path (%). */
        uint64_t average_speedup_percent = 0;

        /** Current number of live entries in the compilation cache. */
        size_t cache_size = 0;

        /** Total recompilations triggered by cardinality drift. */
        size_t recompilations = 0;

        /** Total calls routed through the compiled (hot) path. */
        size_t hot_path_invocations = 0;

        /** Total calls routed through the interpreted (cold) path. */
        size_t cold_path_invocations = 0;
    };

    // =========================================================================
    // Construction
    // =========================================================================

    AdaptiveQueryCompiler();
    explicit AdaptiveQueryCompiler(CompilationConfig config);
    ~AdaptiveQueryCompiler();

    AdaptiveQueryCompiler(const AdaptiveQueryCompiler&)            = delete;
    AdaptiveQueryCompiler& operator=(const AdaptiveQueryCompiler&) = delete;
    AdaptiveQueryCompiler(AdaptiveQueryCompiler&&)                 noexcept = default;
    AdaptiveQueryCompiler& operator=(AdaptiveQueryCompiler&&)      noexcept = default;

    // =========================================================================
    // Core API
    // =========================================================================

    /**
     * @brief Execute a query, automatically switching to the compiled path
     *        once the hot-threshold has been crossed.
     *
     * On the Nth call where N == hot_threshold, the query is compiled and
     * the result of that first compiled execution is returned.  All
     * subsequent calls go through the specialised path.
     *
     * @param query   Parsed query descriptor.
     * @param schema  Current schema snapshot.
     * @param params  Bind parameter values.
     * @return        Query result (always populated; check result.ok and
     *                result.error for failure details).
     */
    QueryResult execute(const ParsedQuery& query,
                        const Schema& schema,
                        const QueryParams& params);

    /**
     * @brief Execute a pre-compiled query specialisation.
     *
     * This overload allows callers that obtained a `CompiledQuery` via
     * `compile()` to invoke it directly, bypassing the hot-path detection
     * layer entirely.
     *
     * @param compiled  A valid CompiledQuery (operator bool() must be true).
     * @param params    Bind parameter values.
     * @return          Query result; result.ok is false when compiled is
     *                  invalid (null execute function).
     */
    QueryResult execute(const CompiledQuery& compiled,
                        const QueryParams& params);

    /**
     * @brief Explicitly compile a query to a specialised execution function.
     *
     * May be called proactively (before the hot threshold is reached) to
     * pre-warm the compilation cache.
     *
     * @param query   Parsed query descriptor.
     * @param schema  Current schema snapshot.
     * @param config  Per-compile overrides (defaults to the instance config).
     * @return        CompiledQuery on success; operator bool() returns false
     *                and compilation_failures is incremented on failure.
     */
    CompiledQuery compile(const ParsedQuery& query,
                          const Schema& schema,
                          std::optional<CompilationConfig> config = std::nullopt);

    /**
     * @brief Return true when the query structure is eligible for compilation.
     *
     * Queries with op_type == Unknown are not compilable.  All other
     * recognised op types are supported by the specialisation layer.
     */
    bool is_compilable(const ParsedQuery& query) const noexcept;

    // =========================================================================
    // Cache management
    // =========================================================================

    /**
     * @brief Evict the compiled specialisation for one fingerprint.
     *
     * The query reverts to the cold path and will be recompiled after
     * `hot_threshold` more executions.
     */
    void invalidate(const std::string& fingerprint);

    /** @brief Evict all compiled specialisations. */
    void invalidateAll();

    /**
     * @brief Return the current execution count for a query fingerprint.
     *
     * Returns 0 when the fingerprint is unknown.
     */
    size_t executionCount(const std::string& fingerprint) const;

    /**
     * @brief Return true when a compiled specialisation exists for the
     *        given fingerprint.
     */
    bool isCompiled(const std::string& fingerprint) const;

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Return a snapshot of current compiler statistics. */
    CompilationStats getStats() const;

    /**
     * @brief Return a snapshot of current compiler statistics.
     *
     * Alias for `getStats()` matching the snake_case naming used in the
     * roadmap architecture specification.
     */
    CompilationStats get_stats() const { return getStats(); }

    /** @brief Reset all statistics counters without evicting compiled code. */
    void resetStats();

    // =========================================================================
    // Config accessor
    // =========================================================================

    const CompilationConfig& config() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace performance
}  // namespace themis
