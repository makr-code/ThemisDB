/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_aql_handler.h                                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     137                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/llama_wrapper.h"
#include <string>
#include <memory>
#include <vector>

namespace themis {
namespace aql {

/**
 * @brief Represents a single turn in a multi-turn AQL conversation
 *
 * Stores the natural language query and the resulting AQL from one turn,
 * providing context for subsequent iterative refinements.
 */
struct ConversationTurn {
    std::string nl_query;   ///< Natural language query from the user
    std::string aql_result; ///< AQL query generated for this turn
};

/**
 * @brief Manages conversation history for iterative AQL query refinement
 *
 * Maintains an ordered list of turns so that follow-up questions can
 * reference previous queries and their results, enabling the LLM to
 * understand the user's intent across multiple refinement steps.
 */
class AQLConversationSession {
public:
    /**
     * @brief Record a completed turn in the session
     * @param nl_query The natural language query that was issued
     * @param aql_result The AQL query that was generated
     */
    void addTurn(const std::string& nl_query, const std::string& aql_result);

    /// Return the full ordered history of turns
    const std::vector<ConversationTurn>& getHistory() const;

    /// Reset the session, discarding all history
    void clear();

    /// True when the session has no turns
    bool empty() const;

    /// Number of completed turns in the session
    std::size_t size() const;

private:
    std::vector<ConversationTurn> history_;
};

/**
 * @brief Handler for LLM-specific AQL commands
 * 
 * Provides execution logic for all LLM commands in AQL:
 * - LLM INFER: Standard text generation
 * - LLM RAG: Retrieval-augmented generation with vector search
 * - LLM EMBED: Generate embeddings
 * - LLM MODEL: Model management (load, unload, list, ingest)
 * - LLM LORA: LoRA management (load, unload, list)
 * - LLM STATS: Performance statistics
 * - LLM CACHE: Cache management (stats, clear)
 */
class LLMAQLHandler {
public:
    LLMAQLHandler();
    ~LLMAQLHandler();

    // Inference commands
    std::string executeInfer(
        const std::string& prompt,
        const std::string& model_id = "",
        const std::string& lora_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

    std::string executeRAG(
        const std::string& query,
        const std::string& collection,
        int top_k = 5,
        const std::string& lora_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

    std::vector<float> executeEmbed(
        const std::string& text,
        const std::string& model_id = ""
    );

    // Model management commands
    void executeModelLoad(const std::string& model_id, const std::string& path);
    void executeModelUnload(const std::string& model_id);
    std::vector<std::string> executeModelList();
    void executeModelIngest(const std::string& model_id, const std::string& blob_urn);

    // LoRA management commands
    void executeLoRALoad(const std::string& lora_id, const std::string& path);
    void executeLoRAUnload(const std::string& lora_id);
    std::vector<std::string> executeLoRAList();

    // Statistics commands
    std::string executeStats();
    std::string executeCacheStats();
    void executeCacheClear();

    // Batch optimization
    struct BatchInferRequest {
        std::string prompt;
        std::string model_id;
        std::string lora_id;
        std::unordered_map<std::string, std::string> options;
    };

    std::vector<std::string> executeBatchInfer(
        const std::vector<BatchInferRequest>& requests
    );

    // Natural Language to AQL Translation
    /**
     * @brief Translate natural language query to AQL
     * @param nl_query Natural language query (e.g., "Find all users in Seattle")
     * @param schema_context Optional database schema context for better translation
     * @return Generated AQL query as string
     * @throws std::runtime_error if translation fails
     */
    std::string translateNLToAQL(
        const std::string& nl_query,
        const std::string& schema_context = ""
    );

    /**
     * @brief Translate a natural language query to AQL using multi-turn context
     *
     * Sends the full conversation history together with the new query so the
     * LLM can refine previously generated AQL.  The resulting turn is
     * automatically appended to @p session.
     *
     * @param nl_query  Natural language query for this turn
     * @param session   Session that holds (and will be updated with) the turn history
     * @param schema_context Optional database schema for better translation
     * @return AQL query generated for this turn
     * @throws std::runtime_error if translation fails
     */
    std::string translateNLToAQLIterative(
        const std::string& nl_query,
        AQLConversationSession& session,
        const std::string& schema_context = ""
    );

    // Conversation/Chat Support
    /**
     * @brief Execute chat interaction with message history
     * @param messages Conversation history
     * @param model_id Optional model identifier
     * @param options Generation options
     * @return Assistant response
     */
    std::string executeChat(
        const std::vector<llm::ChatMessage>& messages,
        const std::string& model_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
