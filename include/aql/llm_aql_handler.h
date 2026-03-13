/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_aql_handler.h                                  ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:52:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     449                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a0dce8967  2026-02-26  feat(aql): API stability guaranteed - version constant, t... ║
    • 5fcab4ddf  2026-02-26  feat(aql): implement few-shot example library for improve... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 849800c79  2026-02-22  Add streaming natural language responses for long AQL exp... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "aql/aql_syntax_highlighter.h"
#include "aql/aql_confidence_scorer.h"
#include "aql/aql_fewshot_example_library.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llama_wrapper.h"
#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <functional>

namespace themis {
namespace aql {

/**
 * @brief Frozen API contract version for the LLM AQL handler.
 *
 * Increment this constant (and bump the major digit) only when a
 * **breaking** change is made to the public interface of LLMAQLHandler,
 * AQLConversationSession, or any of their nested types.  Additive changes
 * (new methods, new optional struct fields) do NOT require a bump.
 *
 * Current version: 1.0 → encoded as 100 (major * 100 + minor).
 */
inline constexpr uint32_t LLM_AQL_HANDLER_API_VERSION = 100; // v1.0

/**
 * @brief Controls how post-generation AQL validation errors are handled
 *        in translateNLToAQL(), translateNLToAQLStreaming(), and
 *        translateNLToAQLWithExamples().
 *
 * Default is WARN_ONLY for backward compatibility.
 */
enum class TranslationValidationMode {
    WARN_ONLY,       ///< Log validation errors as warnings; return the query as-is (default)
    REJECT_ON_ERROR, ///< Throw LLMException(INVALID_RESPONSE) when any ERROR-severity issue is found
    RETRY_ON_ERROR,  ///< Re-invoke the LLM with error feedback; throw after all retries exhausted
};

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

    /**
     * @brief Streaming version of executeInfer — invokes @p token_callback for each
     *        generated token as it is produced, enabling real-time output for long
     *        LLM responses such as AQL explanations.
     *
     * The method validates and sanitizes the prompt the same way as executeInfer(),
     * then delegates to the underlying LLM's streaming interface.  The full
     * concatenated response is also returned as the function's return value so
     * callers can use it in either style.
     *
     * @param prompt         Input prompt text.
     * @param token_callback Callable invoked once per generated token with the token
     *                       string.  Must be thread-safe; it is called from the
     *                       inference thread.
     * @param model_id       Optional model identifier (empty = default model).
     * @param lora_id        Optional LoRA adapter identifier.
     * @param options        Additional generation options (max_tokens, temperature, …).
     * @return Full generated text (concatenation of all streamed tokens).
     * @throws LLMException on validation or inference error.
     */
    std::string executeInferStreaming(
        const std::string& prompt,
        std::function<void(const std::string& token)> token_callback,
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
     * @throws LLMException(INVALID_RESPONSE) if validation mode is REJECT_ON_ERROR or
     *         RETRY_ON_ERROR and the generated query has ERROR-severity structural issues
     * @throws std::runtime_error if translation fails
     */
    std::string translateNLToAQL(
        const std::string& nl_query,
        const std::string& schema_context = ""
    );

    /**
     * @brief Streaming variant of translateNLToAQL — streams the LLM explanation
     *        token by token via @p token_callback as the response is generated.
     *
     * This is designed for long AQL explanations where users benefit from seeing
     * progressive output rather than waiting for the full response.  Inputs are
     * sanitized identically to translateNLToAQL() and the final AQL query is
     * extracted and returned once generation is complete.
     *
     * @param nl_query        Natural language query.
     * @param token_callback  Called for each token as it is generated.
     * @param schema_context  Optional database schema context.
     * @return Extracted AQL query (same as translateNLToAQL would return).
     * @throws LLMException(PROMPT_INJECTION) if inputs contain injection patterns.
     * @throws LLMException(PROMPT_TOO_LONG)  if inputs exceed size limits.
     * @throws std::runtime_error if translation fails.
     */
    std::string translateNLToAQLStreaming(
        const std::string& nl_query,
        std::function<void(const std::string& token)> token_callback,
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
    );

    /**
     * @brief Translate natural language query to AQL using a few-shot example library.
     *
     * Selects up to @p max_examples from @p library that are most relevant to
     * @p nl_query and injects them into the LLM prompt as demonstration pairs.
     * This improves translation accuracy, especially for uncommon query patterns.
     *
     * Inputs are sanitized identically to translateNLToAQL().
     *
     * @param nl_query        Natural-language query.
     * @param library         AQL few-shot example library to draw examples from.
     * @param schema_context  Optional database schema context.
     * @param max_examples    Maximum number of examples to inject (default: 3).
     * @return Generated AQL query as string.
     * @throws LLMException(PROMPT_INJECTION) if either input contains injection patterns.
     * @throws LLMException(PROMPT_TOO_LONG)  if either input exceeds its size limit.
     * @throws std::runtime_error if translation fails.
     */
    std::string translateNLToAQLWithExamples(
        const std::string& nl_query,
        const AQLFewShotExampleLibrary& library,
        const std::string& schema_context = "",
        std::size_t max_examples = 3
    );

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
    // Streaming natural language explanations
    // =========================================================================

    /**
     * @brief Stream a natural language explanation of an AQL query token by token.
     *
     * Builds an explanation prompt for @p aql_query and feeds it to the configured
     * LLM with token-level streaming.  Each generated token is delivered to
     * @p stream_callback as it becomes available, enabling real-time display of
     * long explanations without waiting for the full response.
     *
     * @p aql_query is sanitized before being embedded in the prompt: prompt
     * injection attempts are rejected with @c LLMException(PROMPT_INJECTION).
     * @p schema_context (if non-empty) is sanitized with the same check.
     *
     * @param aql_query       The AQL query to explain.
     *                        Maximum length: @c ValidationLimits::MAX_NL_QUERY_LENGTH.
     * @param stream_callback Called once per token with the token text.
     * @param schema_context  Optional database schema context for richer explanations.
     *                        Maximum length: @c ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH.
     * @return Full accumulated explanation text (concatenation of all streamed tokens).
     * @throws LLMException(PROMPT_INJECTION) if either input contains injection patterns
     * @throws LLMException(PROMPT_TOO_LONG)  if either input exceeds its size limit
     * @throws std::runtime_error if the LLM backend reports an error
     */
    std::string streamExplainAQL(
        const std::string& aql_query,
        std::function<void(const std::string& token)> stream_callback,
        const std::string& schema_context = ""
    );

    /**
     * @brief Stream a natural language explanation of an AQL query as SSE events.
     *
     * Like @c streamExplainAQL() but formats every token as a Server-Sent Events
     * (SSE) data frame before passing it to @p stream_callback, making the output
     * ready to send over an HTTP/SSE connection.
     *
     * @param aql_query       The AQL query to explain.
     * @param stream_callback Called once per token with the SSE-formatted event string.
     * @param request_id      Optional request identifier embedded in each SSE event.
     * @param schema_context  Optional database schema context.
     * @return Full accumulated explanation text.
     * @throws LLMException(PROMPT_INJECTION) if either input contains injection patterns
     * @throws LLMException(PROMPT_TOO_LONG)  if either input exceeds its size limit
     * @throws std::runtime_error if the LLM backend reports an error
     */
    std::string streamExplainAQLAsSSE(
        const std::string& aql_query,
        std::function<void(const std::string& sse_event)> stream_callback,
        const std::string& request_id = "",
        const std::string& schema_context = ""
    );

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

    // =========================================================================
    // Post-generation validation mode
    // =========================================================================

    /**
     * @brief Set the enforcement level for post-generation AQL structural validation.
     *
     * Controls how translateNLToAQL(), translateNLToAQLStreaming(), and
     * translateNLToAQLWithExamples() react when AQLQueryValidator finds
     * ERROR-severity issues in the generated query:
     *  - WARN_ONLY        (default) — log a warning and return the query as-is
     *  - REJECT_ON_ERROR  — throw LLMException(INVALID_RESPONSE) immediately
     *  - RETRY_ON_ERROR   — re-invoke the LLM with error feedback; throw after
     *                       all retries are exhausted
     *
     * @param mode  The desired validation enforcement level.
     */
    void setValidationMode(TranslationValidationMode mode);

    /**
     * @brief Return the current post-generation AQL validation enforcement level.
     */
    TranslationValidationMode getValidationMode() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
