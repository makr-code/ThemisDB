/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_query_rewriter.h                               ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-04-15 18:05:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Result of an LLM-based query rewrite operation.
 */
struct RewrittenQuery {
    std::string original;                  ///< Original input query
    std::vector<std::string> rewrites;     ///< LLM-generated alternative queries
    bool llm_used = false;                 ///< Whether the LLM backend was actually invoked
};

/**
 * @brief LLM-based query rewriting for improved search recall.
 *
 * LlmQueryRewriter uses an LLM to generate semantically equivalent
 * reformulations of a user query.  These alternative phrasings broaden
 * recall by covering vocabulary and phrasing variations that synonym
 * dictionaries cannot anticipate.
 *
 * The LLM backend is injected as a `std::function<std::string(const
 * std::string&)>` (a *prompt → text* callable), making the class
 * independent of any particular LLM library and trivially testable with
 * a mock function.
 *
 * ### Usage
 * ```cpp
 * // Wire up a real LLM backend:
 * LlmQueryRewriter::LlmBackend backend = [&](const std::string& prompt) {
 *     return my_llm.generate(prompt, 256);
 * };
 *
 * LlmQueryRewriter::Config cfg;
 * cfg.num_rewrites = 3;
 * LlmQueryRewriter rewriter(cfg, backend);
 *
 * auto result = rewriter.rewrite("fast db insert");
 * // result.rewrites might contain:
 * //   "high-throughput database insertion"
 * //   "quick record insertion in a database"
 * //   "rapid data write performance"
 * ```
 *
 * ### Fallback behaviour
 * When no backend is set, or when the backend throws or returns empty
 * text, `rewrite()` returns the original query as the sole entry in
 * `rewrites` (when `Config::fallback_to_original == true`) and sets
 * `llm_used = false`.
 *
 * @note Thread Safety: A single instance is NOT thread-safe.  Create
 *   one instance per thread or protect shared instances with a mutex.
 *
 * @note Exception Safety: `rewrite()` never propagates exceptions from
 *   the backend.  All errors are caught internally and trigger the
 *   fallback path.
 */
class LlmQueryRewriter {
public:
    /**
     * @brief Callable type for the LLM backend.
     *
     * The function receives a fully-formed prompt string and returns the
     * LLM's raw text response.  It may throw; `rewrite()` catches all
     * exceptions from it.
     */
    using LlmBackend = std::function<std::string(const std::string& prompt)>;

    struct Config {
        /// Number of alternative rewrites to request from the LLM.
        size_t num_rewrites = 3;
        /// Recommended maximum tokens for the LLM backend to generate per call.
        /// This value is embedded in the prompt and passed to the backend via the
        /// prompt text; the actual enforcement is the backend's responsibility.
        int max_tokens = 256;
        /// Recommended sampling temperature for the LLM backend.
        /// Passed as a hint in the prompt; actual enforcement is backend-specific.
        float temperature = 0.7f;
        /// When true, the original query is appended to rewrites if the LLM
        /// produces no usable output.
        bool fallback_to_original = true;
        /// Character budget per individual rewrite; longer strings are dropped.
        size_t max_rewrite_length = 256;
        static Config defaults() { return {}; }
    };

    /**
     * @brief Construct a rewriter with optional config and LLM backend.
     *
     * @param config   Rewriting parameters.
     * @param backend  LLM backend callable; may be nullptr / empty.
     * @throws std::invalid_argument if num_rewrites == 0.
     */
    explicit LlmQueryRewriter(const Config& config = Config::defaults(),
                               LlmBackend backend = nullptr);

    LlmQueryRewriter(const LlmQueryRewriter&) = delete;
    LlmQueryRewriter& operator=(const LlmQueryRewriter&) = delete;
    LlmQueryRewriter(LlmQueryRewriter&&) = default;
    LlmQueryRewriter& operator=(LlmQueryRewriter&&) = default;

    // -----------------------------------------------------------------------
    // Backend management
    // -----------------------------------------------------------------------

    /**
     * @brief Replace the LLM backend at runtime (e.g. after model load).
     * @param backend  New backend; pass nullptr / empty to disable LLM.
     */
    void setBackend(LlmBackend backend);

    // -----------------------------------------------------------------------
    // Core operation
    // -----------------------------------------------------------------------

    /**
     * @brief Rewrite a query using the LLM backend.
     *
     * Builds a structured prompt, calls the LLM, parses numbered lines
     * from the response, deduplicates, and limits to `Config::num_rewrites`
     * entries.
     *
     * Never throws; backend exceptions trigger the fallback path.
     *
     * @param query  Raw user query.
     * @return Populated RewrittenQuery.
     */
    RewrittenQuery rewrite(const std::string& query) const;

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    LlmBackend backend_;

    /// Build the prompt that instructs the LLM to produce numbered rewrites.
    std::string buildPrompt(const std::string& query) const;

    /// Parse numbered lines (e.g. "1. some rewrite") from the LLM response.
    std::vector<std::string> parseRewrites(const std::string& llm_output,
                                           const std::string& original) const;
};

}  // namespace themis
