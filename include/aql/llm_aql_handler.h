/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_aql_handler.h                                  ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     298                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 9f4b4c45b  2026-02-21  [aql] AQL syntax highlighting, error annotation, and prom... ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "aql/aql_syntax_highlighter.h"
#include "aql/aql_confidence_scorer.h"
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
     *
     * Uses the configured LLM to convert @p nl_query to an executable AQL
     * statement.  Both @p nl_query and @p schema_context are sanitized before
     * being embedded in the LLM prompt: prompt injection attempts (instruction
     * override phrases, persona hijacking, system-block markers) are rejected
     * with @c LLMException(PROMPT_INJECTION).  After the LLM response is cleaned
     * up (markdown fences stripped, whitespace trimmed), the generated AQL is
     * validated with @c AQLSyntaxHighlighter::annotateErrors().  Structural issues
     * are logged as warnings but do not prevent the result from being returned.
     *
     * @param nl_query        Natural language query (e.g., "Find all users in Seattle").
     *                        Maximum length: @c ValidationLimits::MAX_NL_QUERY_LENGTH.
     * @param schema_context  Optional database schema context for better translation.
     *                        Maximum length: @c ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH.
     * @return Generated AQL query as string
     * @throws LLMException(PROMPT_INJECTION) if either input contains injection patterns
     * @throws LLMException(PROMPT_TOO_LONG)  if either input exceeds its size limit
     * @throws std::runtime_error if translation fails
     */
    std::string translateNLToAQL(
        const std::string& nl_query,
        const std::string& schema_context = ""
    );

    /**
     * @brief Result of a natural-language-to-AQL translation with confidence scoring
     */
    struct AQLTranslationResult {
        std::string aql_query;           ///< Generated AQL query
        AQLConfidenceScore confidence;   ///< Confidence score for the generated query
    };

    /**
     * @brief Translate natural language query to AQL and attach a confidence score
     * @param nl_query        Natural language query
     * @param schema_context  Optional database schema context
     * @return AQLTranslationResult containing the query and its confidence score
     * @throws std::runtime_error if translation fails
     */
    AQLTranslationResult translateNLToAQLWithConfidence(
        const std::string& nl_query,
        const std::string& schema_context = ""
    // Batch NL-to-AQL Translation for offline workloads
    /**
     * @brief Single request for batch NL-to-AQL translation
     */
    struct BatchNLToAQLRequest {
        std::string nl_query;       ///< Natural language query
        std::string schema_context; ///< Optional database schema context
    };

    /**
     * @brief Result of a single NL-to-AQL translation within a batch
     */
    struct BatchNLToAQLResult {
        std::string aql_query; ///< Translated AQL query; empty when translation failed
        std::string error;     ///< Error message if translation failed; empty on success
        bool success;          ///< true if translation succeeded
    };

    /**
     * @brief Translate a batch of natural language queries to AQL for offline workloads.
     *
     * Processes every request in order and returns one result per request.
     * Individual translation failures are captured in the result's @c error field
     * so that a single failure does not abort the entire batch.
     *
     * @param requests Vector of NL queries with optional schema contexts
     * @return Vector of results in the same order as the input requests
     */
    std::vector<BatchNLToAQLResult> translateBatchNLToAQL(
        const std::vector<BatchNLToAQLRequest>& requests
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

    // AQL Syntax Highlighting
    /**
     * @brief Highlight AQL code blocks inside an LLM response and annotate errors.
     *
     * Scans @p llm_response for every @c ```aql … ``` block, applies ANSI
     * colour highlighting (if @p use_ansi is @c true) to the AQL inside, and
     * collects structural error annotations (unbalanced brackets, missing IN,
     * unterminated strings).
     *
     * @param llm_response  Raw text returned by an LLM.
     * @param use_ansi      When @c true (default) ANSI escape sequences are
     *                      embedded.  Pass @c false for plain-text output.
     * @return              Struct containing the highlighted text and any
     *                      AQL syntax errors found.
     */
    HighlightedResponse formatLLMResponse(
        const std::string& llm_response,
        bool use_ansi = true
    ) const;
    // =========================================================================
    // Confidence scoring
    // =========================================================================

    /**
     * @brief Score the quality and correctness of a generated AQL query.
     *
     * Asks the LLM to rate the query on a scale from 0.0 to 1.0 and to
     * provide a brief explanation and improvement suggestions.
     *
     * When no LLM model is loaded the method returns a default result with
     * score = -1.0 indicating that scoring is unavailable.
     */
    struct QueryConfidenceScore {
        float                    score;       ///< 0.0 (worst) to 1.0 (best); -1.0 = unavailable
        std::string              explanation; ///< Why this score was assigned
        std::vector<std::string> suggestions; ///< Concrete improvement suggestions
    };

    /**
     * @brief Score a generated AQL query.
     * @param aql_query      The AQL query to evaluate
     * @param original_intent Natural-language intent used to generate the query
     *                        (empty string if unknown)
     * @param schema_context  Optional schema description used during generation
     * @return QueryConfidenceScore (score = -1.0 when LLM unavailable)
     */
    QueryConfidenceScore scoreQueryConfidence(
        const std::string& aql_query,
        const std::string& original_intent  = "",
        const std::string& schema_context   = ""
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
