/**
 * @file aql_conversation_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: aql_conversation_context.h | Version: 0.0.39 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 206
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4151 feat(aql): Bounded conversa... (2026-03-13)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "aql/llm_aql_handler.h"
#include "aql/llm_token_estimator.h"
#include "aql/i_history_compressor.h"
#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include <functional>

namespace themis {
namespace aql {

// Forward declarations
struct IHistoryCompressor;

/**
 * @brief Multi-turn conversation context for iterative AQL query refinement.
 *
 * Maintains a conversation history between the user and the LLM.  Each
 * `refine()` call appends the user's follow-up message to the history and
 * asks the LLM to produce an improved AQL query, taking all previous
 * exchanges into account.
 *
 * The context enforces a sliding-window budget: when the conversation exceeds
 * @c Config::max_turns rounds or @c Config::max_history_tokens estimated
 * tokens, the oldest user+assistant message pair is evicted (the system
 * message is always preserved).
 *
 * All reads and writes to the conversation history are protected by an
 * internal mutex, so `start()`, `refine()`, `reset()`, `turnCount()`,
 * `tokenCount()`, and `getHistory()` are safe to call concurrently on the
 * same instance.
 *
 * Usage example:
 * @code
 * LLMAQLHandler handler;
 * AQLConversationContext ctx(handler);
 * ctx.setSchemaContext("Collections:\n- users: {name, email, city}\n");
 *
 * // First attempt – natural-language intent
 * std::string q1 = ctx.start("Find all users in Berlin");
 * // q1: "FOR u IN users FILTER u.city == \"Berlin\" RETURN u"
 *
 * // Iterative refinement
 * std::string q2 = ctx.refine("Also filter by age > 18");
 * // q2: "FOR u IN users FILTER u.city == \"Berlin\" AND u.age > 18 RETURN u"
 *
 * std::string q3 = ctx.refine("Sort by name and limit to 20");
 * // q3: "FOR u IN users FILTER u.city == \"Berlin\" AND u.age > 18
 * //       SORT u.name ASC LIMIT 20 RETURN u"
 *
 * // Inspect the full conversation history
 * auto history = ctx.getHistory();
 * @endcode
 */
class AQLConversationContext {
public:
    /**
     * @brief Runtime configuration for the conversation context.
     *
     * Pass an instance to the constructor to override the defaults.
     */
    struct Config {
        /** Maximum number of user/assistant turn pairs to keep in history.
         *  When this limit is reached, the oldest pair is evicted before
         *  adding a new user message.  Set to 0 to disable turn-count
         *  eviction.  Defaults to 50. */
        std::size_t max_turns = 50;

        /** Maximum total estimated tokens in the conversation history
         *  (including the system message).  Oldest user+assistant pairs are
         *  evicted until the new message fits within this budget.
         *  Set to 0 to disable token-budget enforcement.  Defaults to 8192.
         *
         *  @note This is a best-effort limit.  If the system prompt alone
         *  exceeds the budget, it is preserved and no further eviction is
         *  possible; the configured limit will be exceeded in that case. */
        std::size_t max_history_tokens = 8192;

        /**
         * @brief Optional LLM executor override.
         *
         * If set, this function is called instead of `LLMAQLHandler::executeChat`
         * for every LLM invocation.  Primarily intended for unit testing so that
         * tests can drive the conversation without a real model loaded.
         *
         * The callback receives the current conversation history and must return
         * the assistant response string (or throw on error).
         *
         * Leave as `nullptr` (default) to use the real `LLMAQLHandler`.
         */
        std::function<std::string(const std::vector<std::pair<std::string, std::string>>&)>
            llm_executor = nullptr;

        /**
         * @brief Enable episodic memory compression (L2 rotation) for conversation history.
         *
         * When enabled and history exceeds episodic_compaction_trigger_tokens,
         * conversation is compressed using extractive summarization to preserve
         * semantic meaning while reducing token count.
         *
         * Defaults to false (disabled). Enable via P2-D03 Phase 2+ implementation.
         *
         * Related: P2-D03 L2 Episodic Memory Compression (SSM-hybrid Phase 2).
         */
        bool enable_episodic_compaction = false;

        /**
         * @brief Token count threshold to trigger episodic memory compression.
         *
         * When total conversation history exceeds this value and
         * enable_episodic_compaction is true, compression is triggered.
         *
         * Defaults to 0 (disabled). Set to value < max_history_tokens for
         * early compression before eviction.
         *
         * Typical value: 6144 (compress when approaching 8192 budget).
         */
        int32_t episodic_compaction_trigger_tokens = 0;

        /**
         * @brief Minimum semantic similarity required for compressed episodes.
         *
         * Acceptance gate P2-GATE-03: compressed episode must preserve
         * >= this similarity score to the original conversation.
         *
         * Defaults to 0.85 (per P2-GATE-03 specification).
         */
        float episodic_compression_gate_similarity = 0.85f;
    };

    /**
     * @brief Construct a context with a handler (default configuration)
     * @param handler   Reference to an LLMAQLHandler instance.
     *                  The handler must outlive this context.
     */
    explicit AQLConversationContext(LLMAQLHandler& handler);
    
    /**
     * @brief Construct a context with handler, configuration, and custom estimator
     * @param handler   Reference to an LLMAQLHandler instance.
     *                  The handler must outlive this context.
     * @param config    Runtime configuration (max turns, token budget).
     * @param estimator Optional token estimator; defaults to
     *                  CharDivisionEstimator (4 chars per token).
     */
    explicit AQLConversationContext(
      LLMAQLHandler& handler,
      Config config,
      std::unique_ptr<TokenEstimator> estimator = nullptr
    );

    /**
     * @brief Construct a context with handler, configuration, estimator, and compressor
     * @param handler     Reference to an LLMAQLHandler instance.
     *                    The handler must outlive this context.
     * @param config      Runtime configuration (max turns, token budget).
     * @param estimator   Optional token estimator; defaults to CharDivisionEstimator.
     * @param compressor  Optional IHistoryCompressor for episodic compression (L2 rotation).
     *                    If nullptr and enable_episodic_compaction is true, compression
     *                    will be silently skipped. The compressor must outlive this context.
     */
    explicit AQLConversationContext(
      LLMAQLHandler& handler,
      Config config,
      std::unique_ptr<TokenEstimator> estimator,
      IHistoryCompressor* compressor
    );
    ~AQLConversationContext();

    // Non-copyable, movable
    AQLConversationContext(const AQLConversationContext&) = delete;
    AQLConversationContext& operator=(const AQLConversationContext&) = delete;
    AQLConversationContext(AQLConversationContext&&) noexcept;
    AQLConversationContext& operator=(AQLConversationContext&&) noexcept;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the database schema context injected into every LLM request.
     *
     * Should describe available collections and their fields so the LLM can
     * produce more accurate AQL.  Can be updated between calls.
     */
    void setSchemaContext(const std::string& schema);

    /** @brief Return the current schema context string. */
    std::string getSchemaContext() const;

    /**
     * @brief Set the history compressor for episodic memory compression (L2 rotation).
     *
     * The compressor is invoked automatically during `refine()` when conversation
     * history exceeds episodic_compaction_trigger_tokens and enable_episodic_compaction
     * is true.
     *
     * @param compressor Pointer to IHistoryCompressor instance. The compressor must
     *                   outlive this context. Can be nullptr to disable compression.
     * @note Thread-safe; can be called concurrently with conversation operations.
     */
    void setCompressor(IHistoryCompressor* compressor);

    /**
     * @brief Get the currently configured history compressor.
     *
     * @return Pointer to the IHistoryCompressor, or nullptr if not configured.
     * @note Thread-safe; returns a snapshot of the current compressor pointer.
     */
    IHistoryCompressor* getCompressor() const;

    // =========================================================================
    // Conversation
    // =========================================================================

    /**
     * @brief Start a new conversation with an initial natural-language intent.
     *
     * Clears any existing history before sending the first message.
     *
     * @param intent Natural-language description of what the user wants
     * @return Generated AQL query string; empty string if the LLM is unavailable
     */
    std::string start(const std::string& intent);

    /**
     * @brief Refine the last generated query with an additional instruction.
     *
     * Appends the instruction to the running conversation and asks the LLM to
     * revise the query accordingly.  Requires a prior call to `start()`.
     *
     * @param instruction Follow-up instruction (e.g., "Add a LIMIT 10 clause")
     * @return Refined AQL query string; empty string if the LLM is unavailable
     * @throws std::logic_error if called before `start()`
     */
    std::string refine(const std::string& instruction);

    /**
     * @brief Reset the conversation history.
     *
     * After this call `refine()` must not be called before `start()`.
     */
    void reset();

    // =========================================================================
    // Inspection
    // =========================================================================

    /** @brief Number of user turns currently retained in history (≤ Config::max_turns). */
    std::size_t turnCount() const;

    /** @brief Estimated total token count of the current conversation history. */
    std::size_t tokenCount() const;

    /** @brief Return the last AQL query generated, or empty string if none. */
    std::string lastQuery() const;

    /**
     * @brief Full conversation history as role/content message pairs.
     *
     * Each element is a pair: {role, content} where role is "system",
     * "user", or "assistant".
     */
    std::vector<std::pair<std::string, std::string>> getHistory() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
