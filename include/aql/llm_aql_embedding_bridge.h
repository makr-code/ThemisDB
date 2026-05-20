/*
 * ThemisDB | File: llm_aql_embedding_bridge.h | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 91
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #4789 RAG: make LLMJudge mock-state reflect active inference path (2026-04-22T06:57:56Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "aql/aql_fewshot_example_library.h"
#include <vector>
#include <string>

namespace themis {
namespace aql {

// Forward declaration to avoid circular include with llm_aql_handler.h.
class LLMAQLHandler;

/**
 * @brief Adapter: bridges `LLMAQLHandler::executeEmbed()` to `IEmbeddingProvider`.
 *
 * This class lets callers wire the AQL handler's embedding circuit (which
 * includes circuit-breaker, sharding locality, and metric instrumentation)
 * directly into an `AQLFewShotExampleLibrary` for semantic few-shot ranking.
 *
 * ## Usage
 * @code
 * LLMAQLHandler handler;
 * AQLFewShotExampleLibrary library;
 * library.addBuiltinSamples();
 *
 * // Create and inject the bridge
 * auto bridge = handler.makeEmbeddingBridge();
 * library.setEmbeddingProvider(bridge.get());
 * library.rebuildEmbeddingIndex();
 *
 * // Translate – semantic few-shot selection now active
 * std::string aql = handler.translateNLToAQLWithExamples(
 *     "find all users with role admin",
 *     library,
 *     schema_context,
 *     5);
 * @endcode
 *
 * Lifetime: the `LLMAQLHandler` reference passed to the constructor
 * must outlive this bridge object.
 *
 * Thread-safety: `embed()` is thread-safe to the extent that
 * `LLMAQLHandler::executeEmbed()` is thread-safe (each call acquires the
 * handler's circuit-breaker atomically; no shared mutable state here).
 *
 * Source: src/aql/FUTURE_ENHANCEMENTS.md §8 – "Use the LLMAQLHandler's
 * existing executeEmbed() as the default embedding provider bridge"
 */
class LLMAQLEmbeddingBridge final : public IEmbeddingProvider {
public:
    /**
     * @param handler  Live handler. Must outlive this bridge.
     */
    explicit LLMAQLEmbeddingBridge(LLMAQLHandler& handler) noexcept
        : handler_(handler) {}

    ~LLMAQLEmbeddingBridge() override = default;

    // Non-copyable (holds a reference to the handler)
    LLMAQLEmbeddingBridge(const LLMAQLEmbeddingBridge&)            = delete;
    LLMAQLEmbeddingBridge& operator=(const LLMAQLEmbeddingBridge&) = delete;
    LLMAQLEmbeddingBridge(LLMAQLEmbeddingBridge&&)                 = delete;
    LLMAQLEmbeddingBridge& operator=(LLMAQLEmbeddingBridge&&)      = delete;

    /**
     * @brief Compute an embedding via the handler's executeEmbed() circuit.
     *
     * Delegates to `LLMAQLHandler::executeEmbed(text)`.  Any exception thrown
     * by the handler (e.g. circuit-breaker open, LLM unavailable) is caught
     * and an empty vector is returned, causing `AQLFewShotExampleLibrary` to
     * fall back to Jaccard word-overlap ranking for that query.
     *
     * @param text  Input text (may be empty; returns empty vector).
     * @return Dense float embedding, or an empty vector on failure.
     */
    std::vector<float> embed(const std::string& text) override;

private:
    LLMAQLHandler& handler_;  ///< non-owning reference to the wired handler
};

} // namespace aql
} // namespace themis
