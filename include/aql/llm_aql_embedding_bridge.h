/**
 * @file llm_aql_embedding_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
