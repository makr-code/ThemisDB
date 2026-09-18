/**
 * @file jit_aggregation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class JITAggregationCompiler {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    struct Config {
        size_t hot_threshold = 10;

        bool enable_jit = true;

        int optimization_level = 2;

        size_t max_cache_entries = 256;
    };

    // -------------------------------------------------------------------------
    // Construction / destruction
    // -------------------------------------------------------------------------

    JITAggregationCompiler();
    /**
     * @brief JITAggregation Compiler.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit JITAggregationCompiler(const Config& config);
    ~JITAggregationCompiler();

    JITAggregationCompiler(const JITAggregationCompiler&)            = delete;
    JITAggregationCompiler& operator=(const JITAggregationCompiler&) = delete;
    JITAggregationCompiler(JITAggregationCompiler&&)                 noexcept = default;
    JITAggregationCompiler& operator=(JITAggregationCompiler&&)      noexcept = default;

    // -------------------------------------------------------------------------
    // Core API
    // -------------------------------------------------------------------------

    /**
     * @brief Aggregate.
     * @param[in] input Input parameter.
     * @param[in] specs Input parameter.
     * @return Return value.
     */
    ColumnBatch aggregate(const ColumnBatch& input,
                          const std::vector<AggregateSpec>& specs);

    // -------------------------------------------------------------------------
    // Introspection
    // -------------------------------------------------------------------------

    /**
     * @brief Is Compiled.
     * @param[in] spec_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCompiled(const std::string& spec_key) const;

    /**
     * @brief Call Count.
     * @param[in] spec_key Input parameter.
     * @return Return value.
     */
    size_t callCount(const std::string& spec_key) const;

    /**
     * @brief Make Spec Key.
     * @param[in] specs Input parameter.
     * @return Return value.
     */
    static std::string makeSpecKey(const std::vector<AggregateSpec>& specs);

    // -------------------------------------------------------------------------
    // Cache management
    // -------------------------------------------------------------------------

    /**
     * @brief Invalidate.
     * @param[in] spec_key Input parameter.
     */
    void invalidate(const std::string& spec_key);

    /**
     * @brief Invalidate All.
     */
    void invalidateAll();

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    struct Stats {
        size_t total_calls = 0;

        size_t jit_hits = 0;

        size_t jit_compilations = 0;

        size_t cache_size = 0;
    };

    /**
     * @brief Stats.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    Stats stats() const noexcept;

    /**
     * @brief Reset Stats.
     * @note Exception safety: noexcept.
     */
    void resetStats() noexcept;

    // -------------------------------------------------------------------------
    // Config accessor
    // -------------------------------------------------------------------------

    /**
     * @brief Config.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const Config& config() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace analytics
}  // namespace themisdb
