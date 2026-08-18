/**
 * @file query_compiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "utils/expected.h"

namespace themis {
namespace query {

/**
 * @brief Query JIT Compiler — v1.8.0
 *
 * Hot-path detection and specialised query execution for frequently
 * executed AQL queries.  Follows the same warm-up / specialise pattern
 * used by JVM JIT compilers (and by analytics::JITAggregationCompiler):
 *
 *   Cold path  (call_count < hot_threshold):
 *     Each execution is delegated to the generic interpreted path via the
 *     ExecuteFn supplied at compile() time.  The call-site key (query
 *     fingerprint) is tracked in an internal counter map.
 *
 *   Compilation (call_count == hot_threshold):
 *     specialise() builds a std::function<Result<nlohmann::json>(
 *     const nlohmann::json&)> that hard-codes the query's execution
 *     strategy (filter predicates, projection list, sort, limit) for
 *     this specific query shape, eliminating per-execution AST
 *     traversal overhead.  When THEMIS_HAS_LLVM_JIT is defined, an
 *     LLVM MCJIT backend may be used instead (future extension point).
 *
 *   Hot path (call_count > hot_threshold):
 *     The cached specialised function is invoked directly.
 *
 * Thread safety:
 *   QueryCompiler is NOT thread-safe.  Use one instance per thread, or
 *   protect concurrent access with an external mutex.
 *
 * Compilation guard:
 *   THEMIS_HAS_LLVM_JIT – if defined at compile time, enables an LLVM
 *   MCJIT backend for native code generation.
 *
 * Copyright (c) 2026 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

// ============================================================================
// QueryParams — bind-value container passed on each execution
// ============================================================================

/**
 * @brief Named bind-parameter values for a compiled query.
 *
 * Keys correspond to @param-style placeholders in the AQL query text.
 * Values are JSON scalars or arrays.
 */
using QueryParams = std::unordered_map<std::string, nlohmann::json>;

// ============================================================================
// QueryResult — result row set returned by execute()
// ============================================================================

/**
 * @brief Result set returned by a compiled query execution.
 */
struct QueryResult {
    /// Result rows (each row is a JSON object or scalar).
    std::vector<nlohmann::json> rows;

    /// Number of rows examined before limit/filter (for diagnostics).
    size_t rows_examined = 0;

    /// True if the hot (compiled/specialised) path was used.
    bool used_compiled_path = false;

    /// Execution time in microseconds.
    uint64_t execution_time_us = 0;
};

// ============================================================================
// QueryCompiler
// ============================================================================

/**
 * @brief Hot-path JIT compiler for frequently executed AQL queries.
 *
 * The caller provides the query text and a generic interpreted executor
 * (ExecuteFn) at compile() time.  QueryCompiler wraps the executor with
 * call-count tracking and hot-path specialisation.
 *
 * Example:
 * @code
 *   // Provide the interpreted execution back-end
 *   QueryCompiler::ExecuteFn interp = [&](const std::string& q,
 *                                         const QueryParams& p) {
 *       return engine.executeInterpreted(q, p);
 *   };
 *
 *   QueryCompiler compiler;
 *   auto compiled = compiler.compile("FOR u IN users RETURN u", {}, interp);
 *
 *   // Cold path for first hot_threshold calls
 *   for (int i = 0; i < compiler.config().hot_threshold; ++i)
 *       compiler.execute(compiled, {});
 *
 *   // Hot path from here on
 *   auto result = compiler.execute(compiled, {});
 *   assert(result->used_compiled_path);
 * @endcode
 */
class QueryCompiler {
public:
    // =========================================================================
    // Types
    // =========================================================================

    /**
     * @brief Optimisation levels (mirrors standard compiler -O flags).
     */
    enum class OptimizationLevel : int {
        O0 = 0,  ///< No optimisation (baseline; useful for testing)
        O1 = 1,  ///< Basic optimisation
        O2 = 2,  ///< Standard optimisation (default)
        O3 = 3   ///< Aggressive optimisation
    };

    /**
     * @brief Signature of the generic interpreted executor.
     *
     * The compiler delegates to this function on cold paths and uses it
     * as the basis for generating specialised hot-path functions.
     *
     * @param query  Raw AQL query string.
     * @param params Bind-parameter values for this execution.
     * @return       Result rows on success; Error on failure.
     */
    using ExecuteFn = std::function<Result<QueryResult>(
        const std::string& query,
        const QueryParams& params)>;

    // =========================================================================
    // Configuration
    // =========================================================================

    struct Config {
        /// Number of cold executions before the query is specialised.
        size_t hot_threshold = 100;

        /// Enable the specialisation layer (disable for benchmarking).
        bool enable_jit = true;

        /// Optimisation level hint for the specialised implementation.
        OptimizationLevel opt_level = OptimizationLevel::O2;

        /// Maximum number of distinct queries to keep in the compiled cache.
        size_t max_cache_entries = 512;

        /**
         * @brief Maximum compile time in milliseconds before falling back.
         *
         * **Wave A Timeout Safety (§12, ROADMAP.md):**
         * Compilation is strictly bounded by this deadline. If specialisation
         * would exceed this timeout, the compiler aborts the compilation,
         * marks the entry as failed, and falls back to the interpreted path
         * indefinitely.
         *
         * **SLA Reasoning:**
         * - Default: 100 ms (generous for template specialisation on modern CPUs)
         * - Rationale: Ensures compilation overhead never dominates query latency
         * - Failure mode: Silent fallback to cold path with no loss of correctness
         * - Logging: All timeout events logged with query key for observability
         * - Future: When THEMIS_HAS_LLVM_JIT is enabled, LLVM compilation
         *   will also respect this deadline with early abort.
         */
        uint64_t compilation_timeout_ms = 100;

        Config() = default;
    };

    // =========================================================================
    // CompiledQuery — opaque handle returned by compile()
    // =========================================================================

    /**
     * @brief Opaque handle to a compiled (or pending) query.
     *
     * Callers treat this as an opaque token.  The compiler stores all
     * internal state; the handle merely carries the query key.
     */
    struct CompiledQuery {
        /// Canonical cache key (fingerprint of query text).
        std::string key;

        /// Raw query text retained for fallback/logging.
        std::string query_text;

        /// True once a specialised function has been generated.
        bool is_compiled = false;

        /// Compilation time (microseconds); set after specialisation.
        uint64_t compilation_time_us = 0;
    };

    // =========================================================================
    // Statistics
    // =========================================================================

    struct Stats {
        /// Total execute() calls since construction (or last reset).
        size_t total_calls = 0;

        /// Calls served by a compiled specialisation.
        size_t hot_hits = 0;

        /// Calls served by the interpreted cold path.
        size_t cold_hits = 0;

        /// Number of queries that have been specialised.
        size_t compilations = 0;

        /// Number of compilations that exceeded compilation_timeout_ms.
        size_t compilation_timeouts = 0;

        /// Number of compilations that fell back due to errors.
        size_t compilation_failures = 0;

        /// Current number of entries in the compilation cache.
        size_t cache_size = 0;
    };

    // =========================================================================
    // Construction / destruction
    // =========================================================================

    QueryCompiler();
    explicit QueryCompiler(const Config& config);
    ~QueryCompiler();

    QueryCompiler(const QueryCompiler&)            = delete;
    QueryCompiler& operator=(const QueryCompiler&) = delete;
    QueryCompiler(QueryCompiler&&)                 noexcept = default;
    QueryCompiler& operator=(QueryCompiler&&)      noexcept = default;

    // =========================================================================
    // Core API
    // =========================================================================

    /**
     * @brief Register a query for hot-path tracking and specialisation.
     *
     * Subsequent calls to execute() on the returned handle are counted.
     * Once Config::hot_threshold is reached the interpreter function is
     * used to build a specialised executor for this query shape.
     *
     * @param query_text  Raw AQL query string.
     * @param params_meta Bind-parameter names (used to build specialisations).
     * @param executor    Interpreted execution back-end for cold path and
     *                    as the basis for specialisation.
     * @return            CompiledQuery handle for subsequent execute() calls.
     */
    CompiledQuery compile(
        const std::string&         query_text,
        const std::vector<std::string>& params_meta,
        ExecuteFn                  executor);

    /**
     * @brief Execute a compiled query (cold or hot path).
     *
     * Implements a two-tier execution strategy:
     *
     * **Hot Path (JIT Compiled):**
     *   If the query has been specialised via JIT compilation and call count
     *   exceeds the configured hot_threshold, the compiled specialised function
     *   is invoked. This path captures the executor and query text by value,
     *   eliminating per-call map lookups and improving latency for hot queries.
     *   Exceptions from the compiled function are caught, logged, and transformed
     *   to Result<QueryResult> (strong exception safety guarantee).
     *
     * **Cold Path (Interpreted):**
     *   For queries below the hot threshold or when specialisation has failed,
     *   the interpreter is invoked directly. This path is always available as
     *   a fallback and ensures correctness over performance.
     *
     * **Exception Safety (Wave A §12-13):**
     *   - Strong exception safety: execute() never propagates exceptions.
     *   - All exceptions are caught, logged with full context (query key, error type),
     *     and transformed to Result<QueryResult>::Err() with appropriate error codes.
     *   - Both hot and cold paths apply uniform error handling.
     *   - Unknown exceptions (catch-all) are logged with type information for debugging.
     *
     * @param compiled  Handle produced by compile().
     * @param params    Bind-parameter values for this execution.
     * @return          Result<QueryResult>: Ok with result on success;
     *                  Err with ErrorCode::ERR_QUERY_EXECUTION_FAILED and detailed
     *                  message on any exception (std::exception or unknown).
     *
     * @throws          Never. All exceptions are caught and transformed to Result.
     *
     * @post            - If compilation failed or hot threshold not exceeded,
     *                    interpreter is used (no silent fallback; caller can verify
     *                    via isCompiled() or CompiledQuery::used_compiled_path flag).
     *                  - All errors are observable via the Result<> return type.
     */
    Result<QueryResult> execute(
        const CompiledQuery& compiled,
        const QueryParams&   params);

    // =========================================================================
    // Introspection
    // =========================================================================

    /**
     * @brief Returns true when the query identified by @p key has a
     *        compiled specialisation ready.
     */
    bool isCompiled(const std::string& key) const;

    /**
     * @brief Returns the total call count for the given @p key.
     * @return 0 when the key is unknown.
     */
    size_t callCount(const std::string& key) const;

    /**
     * @brief Compute the canonical cache key for @p query_text.
     *
     * The key is a 16-character FNV-1a hex digest, deterministic
     * across process restarts.
     */
    static std::string makeKey(const std::string& query_text);

    // =========================================================================
    // Cache management
    // =========================================================================

    /**
     * @brief Evict the compiled specialisation for @p key.
     * @param key  Cache key to invalidate.
     */
    void invalidate(const std::string& key);

    /** @brief Evict all compiled specialisations (resets call counts). */
    void invalidateAll();

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Return a snapshot of the current statistics. */
    const Stats& stats() const noexcept;

    /** @brief Reset statistics counters (does not invalidate compiled code). */
    void resetStats() noexcept;

    // =========================================================================
    // Config accessor
    // =========================================================================

    const Config& config() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace query
}  // namespace themis
