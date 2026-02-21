/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_conversation_context.h                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 13:56:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     151                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • afade9ae9  2026-02-21  [aql] Interactive AQL query builder, validator, template ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "aql/llm_aql_handler.h"
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace aql {

/**
 * @brief Multi-turn conversation context for iterative AQL query refinement.
 *
 * Maintains a conversation history between the user and the LLM.  Each
 * `refine()` call appends the user's follow-up message to the history and
 * asks the LLM to produce an improved AQL query, taking all previous
 * exchanges into account.
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
     * @brief Construct a context that will use @p handler for LLM calls.
     * @param handler Reference to an LLMAQLHandler instance.
     *                The handler must outlive this context.
     */
    explicit AQLConversationContext(LLMAQLHandler& handler);
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
    const std::string& getSchemaContext() const;

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

    /** @brief Number of user turns so far (excludes system messages). */
    std::size_t turnCount() const;

    /** @brief Return the last AQL query generated, or empty string if none. */
    const std::string& lastQuery() const;

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
