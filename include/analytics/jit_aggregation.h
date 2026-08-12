/**
 * @file jit_aggregation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB JIT Aggregation Compiler for Hot Aggregation Paths
 *
 * Implements hot-path detection and specialised aggregation dispatch for
 * columnar analytics workloads.  The design follows the same warm-up /
 * specialise pattern used by JVM JIT compilers:
 *
 *   Cold path  (calls < hot_threshold):
 *     Aggregation is executed via the generic AggregateOperator dispatch
 *     table (switch-on-function-enum per row).
 *
 *   Compilation (calls == hot_threshold):
 *     A specialised std::function<ColumnBatch(const ColumnBatch&)> is
 *     generated for the exact combination of AggregateSpec::Functions
 *     present in this call site, avoiding per-row virtual dispatch.
 *     When THEMIS_HAS_LLVM_JIT is defined the compiler may instead emit
 *     native machine code via LLVM MCJIT (future extension point).
 *
 *   Hot path   (calls > hot_threshold):
 *     The cached specialised function is invoked directly (jit_hits).
 *
 * Thread safety:
 *   JITAggregationCompiler is NOT thread-safe.  Use one instance per
 *   thread, or protect concurrent access with an external mutex.
 *
 * Compilation guard:
 *   THEMIS_HAS_LLVM_JIT – if defined, enables LLVM MCJIT code-generation
 *   in addition to the always-available template-specialisation path.
 *
 * Copyright (c) 2026 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "analytics/columnar_execution.h"

namespace themisdb {
namespace analytics {

// ============================================================================
// JITAggregationCompiler
// ============================================================================

/**
 * @brief Hot-path JIT compiler for columnar aggregation operators.
 *
 * Wraps AggregateOperator with a call-count based specialisation layer.
 * Repeated calls with the same aggregation specification are detected and
 * replaced with a cached specialised implementation after
 * Config::hot_threshold invocations.
 *
 * Example:
 * @code
 *   JITAggregationCompiler jit;
 *
 *   std::vector<AggregateSpec> specs = {{
 *       .result_name  = "total",
 *       .input_column = "price",
 *       .function     = AggregateSpec::Function::Sum,
 *       .group_by     = {"category"}
 *   }};
 *
 *   // First few calls: cold path (generic dispatch)
 *   for (int i = 0; i < 5; ++i)
 *       jit.aggregate(batch, specs);
 *
 *   // Calls >= hot_threshold: specialised path (jit_hits++)
 *   ColumnBatch result = jit.aggregate(batch, specs);
 *
 *   auto s = jit.stats();
 *   std::cout << "JIT hits: " << s.jit_hits << "\n";
 * @endcode
 */
class JITAggregationCompiler {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    struct Config {
        /// Number of invocations of the same spec-set before specialisation.
        size_t hot_threshold = 10;

        /// Enable the specialisation layer (disable for benchmarking baseline).
        bool enable_jit = true;

        /// Optimisation level hint (0 = none, 1 = basic, 2 = standard, 3 = aggressive).
        /// Currently affects only the LLVM backend when THEMIS_HAS_LLVM_JIT is defined.
        int optimization_level = 2;

        /// Maximum number of distinct spec-sets to keep in the compilation cache.
        size_t max_cache_entries = 256;
    };

    // -------------------------------------------------------------------------
    // Construction / destruction
    // -------------------------------------------------------------------------

    JITAggregationCompiler();
    explicit JITAggregationCompiler(const Config& config);
    ~JITAggregationCompiler();

    JITAggregationCompiler(const JITAggregationCompiler&)            = delete;
    JITAggregationCompiler& operator=(const JITAggregationCompiler&) = delete;
    JITAggregationCompiler(JITAggregationCompiler&&)                 = default;
    JITAggregationCompiler& operator=(JITAggregationCompiler&&)      = default;

    // -------------------------------------------------------------------------
    // Core API
    // -------------------------------------------------------------------------

    /**
     * @brief Execute an aggregation, using the specialised path when hot.
     *
     * @param input  Input ColumnBatch (may carry a pending SelectionVector;
     *               it is materialised internally before aggregation).
     * @param specs  Aggregation specifications (function + group-by).
     *               All specs must share the same group_by list (first wins).
     * @return       Aggregated ColumnBatch (one row per group, or one row
     *               when group_by is empty).
     */
    ColumnBatch aggregate(const ColumnBatch& input,
                          const std::vector<AggregateSpec>& specs);

    // -------------------------------------------------------------------------
    // Introspection
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true when the given spec-set has a compiled specialisation.
     * @param spec_key  Cache key as returned by makeSpecKey() or equivalent.
     */
    bool isCompiled(const std::string& spec_key) const;

    /**
     * @brief Returns the current call count for a spec-set key.
     * @param spec_key  Cache key for the spec-set.
     * @return Call count (0 when key is unknown).
     */
    size_t callCount(const std::string& spec_key) const;

    /**
     * @brief Compute the canonical cache key for a spec-set (for testing).
     */
    static std::string makeSpecKey(const std::vector<AggregateSpec>& specs);

    // -------------------------------------------------------------------------
    // Cache management
    // -------------------------------------------------------------------------

    /**
     * @brief Evict the compiled specialisation for one spec-set.
     * @param spec_key  Cache key to invalidate.
     */
    void invalidate(const std::string& spec_key);

    /** @brief Evict all compiled specialisations. */
    void invalidateAll();

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    struct Stats {
        /// Total aggregate() invocations since construction (or last reset).
        size_t total_calls = 0;

        /// Invocations served by a compiled specialisation.
        size_t jit_hits = 0;

        /// Number of spec-sets that have been compiled.
        size_t jit_compilations = 0;

        /// Current number of entries in the compilation cache.
        size_t cache_size = 0;
    };

    /** @brief Return a snapshot of the current statistics. */
    Stats stats() const noexcept;

    /** @brief Reset statistics counters (does not invalidate compiled code). */
    void resetStats() noexcept;

    // -------------------------------------------------------------------------
    // Config accessor
    // -------------------------------------------------------------------------

    const Config& config() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace analytics
}  // namespace themisdb
