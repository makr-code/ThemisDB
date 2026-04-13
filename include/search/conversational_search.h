/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            conversational_search.h                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-13 04:19:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     190                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9a7d418c01  2026-04-11  feat(search): Phase 5 interfaces — ConversationalSearch, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "search/hybrid_search.h"
#include <deque>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Multi-turn conversational search with context-aware query rewriting.
 *
 * ConversationalSearch maintains a per-session conversation history and
 * reformulates each new user query in the context of previous turns.  Each
 * turn comprises a user query and the retrieved result list.
 *
 * ### Context rewriting
 *
 * When `reformulate()` is called with a raw follow-up query, it appends the
 * most recent `Config::context_window` turns of query text to form a
 * contextualised query string.  For example, given the history
 * `["machine learning", "neural networks"]` and new query `"gradient descent"`,
 * the context-enriched query becomes
 * `"machine learning neural networks gradient descent"`.
 *
 * The enriched query is then passed to the underlying `HybridSearch` engine.
 * Callers that prefer to supply their own contextual query can call
 * `search()` directly.
 *
 * ### Session management
 *
 * A session is identified by a `session_id` string.  Each
 * `ConversationalSearch` instance maintains one implicit session.  Use
 * separate instances (or call `clearHistory()`) for independent sessions.
 *
 * ### Typical usage
 * ```cpp
 * ConversationalSearch::Config cfg;
 * cfg.context_window = 3;
 * ConversationalSearch cs(&hybrid_search, cfg);
 *
 * // Turn 1
 * auto results1 = cs.search("machine learning");
 *
 * // Turn 2 — query is expanded with prior context
 * auto results2 = cs.search("what about overfitting?");
 *
 * // Inspect history
 * for (const auto& turn : cs.getHistory()) {
 *     std::cout << turn.query << " -> " << turn.results.size() << " results\n";
 * }
 * ```
 *
 * @note Thread Safety: A single ConversationalSearch instance is NOT
 *   thread-safe.  Create separate instances per session/thread or provide
 *   external synchronisation.
 * @note Exception Safety: `search()` and `reformulate()` never throw.
 *   The constructor throws `std::invalid_argument` on invalid config.
 *
 * @since v2.4.0 (Phase 5 — Conversational Search)
 */
class ConversationalSearch {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Engine configuration.
     */
    struct Config {
        /// Number of past query turns to include in context rewriting.
        /// 0 disables context rewriting (pure stateless search per turn).
        size_t context_window = 3;

        /// Maximum turns retained in history; oldest turns are evicted
        /// when this limit is reached.
        size_t max_history = 50;

        /// Separator inserted between historical query terms in the
        /// reformulated query string.
        std::string context_separator = " ";
    };

    /**
     * @brief A single conversation turn (query + retrieved results).
     */
    struct Turn {
        std::string query;                         ///< Original user query
        std::string reformulated_query;            ///< Context-enriched query sent to index
        std::vector<HybridSearch::Result> results; ///< Results retrieved for this turn
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a ConversationalSearch engine.
     *
     * @param hybrid_search  Non-owning pointer to an underlying HybridSearch
     *                       instance.  May be null; all searches return empty.
     * @param config         Engine configuration.
     * @throws std::invalid_argument on invalid config values.
     */
    explicit ConversationalSearch(HybridSearch* hybrid_search);
    ConversationalSearch(HybridSearch* hybrid_search, const Config& config);

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a context-aware search turn.
     *
     * The user @p query is reformulated using the most recent
     * `Config::context_window` turns from the conversation history
     * (via `reformulate()`), the enriched query is dispatched to the
     * underlying `HybridSearch`, and the turn is appended to the history.
     *
     * Never throws; all index exceptions are caught internally.
     *
     * @param query  Raw user query for this turn.
     * @return Retrieved results, sorted by hybrid score descending.
     */
    std::vector<HybridSearch::Result> search(const std::string& query);

    // -----------------------------------------------------------------------
    // Context reformulation (public for unit testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Build a context-enriched query string.
     *
     * Concatenates the most recent `Config::context_window` queries from the
     * history with @p query using `Config::context_separator`.  When
     * `context_window == 0` or the history is empty, @p query is returned
     * unchanged.
     *
     * @param query  New raw user query.
     * @return Context-enriched query string.
     */
    std::string reformulate(const std::string& query) const;

    // -----------------------------------------------------------------------
    // History management
    // -----------------------------------------------------------------------

    /** @brief Return the full conversation history (oldest first). */
    const std::deque<Turn>& getHistory() const { return history_; }

    /** @brief Number of turns in the history. */
    size_t historySize() const { return history_.size(); }

    /** @brief Remove all history entries. */
    void clearHistory();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config);

private:
    HybridSearch* hybrid_search_;
    Config config_;
    std::deque<Turn> history_;
};

} // namespace themis
