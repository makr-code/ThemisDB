/**
 * @file query_compiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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


// ============================================================================
// QueryParams — bind-value container passed on each execution
// ============================================================================

using QueryParams = std::unordered_map<std::string, nlohmann::json>;

// ============================================================================
// QueryResult — result row set returned by execute()
// ============================================================================

struct QueryResult {
    std::vector<nlohmann::json> rows;

    size_t rows_examined = 0;

    bool used_compiled_path = false;

    uint64_t execution_time_us = 0;
};

// ============================================================================
// QueryCompiler
// ============================================================================

class QueryCompiler {
public:
    // =========================================================================
    // Types
    // =========================================================================

    enum class OptimizationLevel : int {
        O0 = 0,  ///< No optimisation (baseline; useful for testing)
        O1 = 1,  ///< Basic optimisation
        O2 = 2,  ///< Standard optimisation (default)
        O3 = 3   ///< Aggressive optimisation
    };

    using ExecuteFn = std::function<Result<QueryResult>(
        const std::string& query,
        const QueryParams& params)>;

    // =========================================================================
    // Configuration
    // =========================================================================

    struct Config {
        size_t hot_threshold = 100;

        bool enable_jit = true;

        OptimizationLevel opt_level = OptimizationLevel::O2;

        size_t max_cache_entries = 512;

        uint64_t compilation_timeout_ms = 100;

        Config() = default;
    };

    // =========================================================================
    // CompiledQuery — opaque handle returned by compile()
    // =========================================================================

    struct CompiledQuery {
        std::string key;

        std::string query_text;

        bool is_compiled = false;

        uint64_t compilation_time_us = 0;
    };

    // =========================================================================
    // Statistics
    // =========================================================================

    struct Stats {
        size_t total_calls = 0;

        size_t hot_hits = 0;

        size_t cold_hits = 0;

        size_t compilations = 0;

        size_t compilation_timeouts = 0;

        size_t compilation_failures = 0;

        size_t cache_size = 0;
    };

    // =========================================================================
    // Construction / destruction
    // =========================================================================

    QueryCompiler();
    /**
     * @brief Query Compiler.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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
     * @brief Compile.
     * @param[in] query_text Input parameter.
     * @param[in] params_meta Input parameter.
     * @param[in] executor Input parameter.
     * @return Return value.
     */
    CompiledQuery compile(
        const std::string&         query_text,
        const std::vector<std::string>& params_meta,
        ExecuteFn                  executor);

    /**
     * @brief Execute.
     * @param[in] compiled Input parameter.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    Result<QueryResult> execute(
        const CompiledQuery& compiled,
        const QueryParams&   params);

    // =========================================================================
    // Introspection
    // =========================================================================

    /**
     * @brief Is Compiled.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCompiled(const std::string& key) const;

    /**
     * @brief Call Count.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    size_t callCount(const std::string& key) const;

    /**
     * @brief Make Key.
     * @param[in] query_text Input parameter.
     * @return Return value.
     */
    static std::string makeKey(const std::string& query_text);

    // =========================================================================
    // Cache management
    // =========================================================================

    /**
     * @brief Invalidate.
     * @param[in] key Input parameter.
     */
    void invalidate(const std::string& key);

    /**
     * @brief Invalidate All.
     */
    void invalidateAll();

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Stats.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const Stats& stats() const noexcept;

    /**
     * @brief Reset Stats.
     * @note Exception safety: noexcept.
     */
    void resetStats() noexcept;

    // =========================================================================
    // Config accessor
    // =========================================================================

    /**
     * @brief Config.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const Config& config() const noexcept;

    // =========================================================================
    // JIT state health
    // =========================================================================

    /**
     * @brief Is Jit State Corrupted.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isJitStateCorrupted() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace query
}  // namespace themis
