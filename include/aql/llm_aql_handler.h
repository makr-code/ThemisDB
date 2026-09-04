/**
 * @file llm_aql_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Forward-declare RocksDBWrapper to allow setStorage() without pulling in its header.
namespace themis { class RocksDBWrapper; }

#include "aql/aql_syntax_highlighter.h"
#include "aql/aql_confidence_scorer.h"
#include "aql/aql_fewshot_example_library.h"
#include "aql/llm_token_estimator.h"
#include "aql/llm_error_codes.h"
#include "aql/llm_timeout_manager.h"
#include "aql/llm_validation_pipeline.h"
#include "llm/llm_client.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llama_wrapper.h"
#include "query/aql_parser_service.h"
#include "sharding/circuit_breaker.h"
#include "utils/expected.h"
#include <nlohmann/json.hpp>

// Forward-declare to avoid pulling in toolbox/ingestion headers transitively.
// Consumers that use setIngestionBridge() must include aql_ingestion_bridge.h.
namespace themis { namespace aql { class AQLIngestionBridge; } }
// Forward-declare embedding bridge; consumers must include llm_aql_embedding_bridge.h.
namespace themis { namespace aql { class LLMAQLEmbeddingBridge; } }
namespace themis { namespace sharding {
class ShardingManager;
class AdaptiveShardRouter;
} }
namespace themis { namespace llm {
class KVPrefixTransferManager;
class ContinuousBatchScheduler;
} }
#include <string>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

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
    using json = nlohmann::json;
    using DomainRouteResolver = std::function<
        std::optional<std::pair<std::string, double>>(const std::string& domain_hint)
    >;
    using CAISafetyEvalFn = std::function<Result<double>(
        const std::string& generated_response,
        const std::string& original_query)>;
    using FederatedTelemetryFn = std::function<Result<void>(const json& local_metrics)>;

    /**
     * @brief Per-operation-type circuit breaker configuration.
     *
     * Each field configures the circuit breaker for a specific operation type.
     * Default values match the original single-breaker configuration for
     * backward compatibility.
     */
    struct Config {
        /// Circuit breaker config for LLM INFER / INFER STREAMING operations.
        sharding::CircuitBreaker::Config infer_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        /// Circuit breaker config for LLM RAG operations.
        sharding::CircuitBreaker::Config rag_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        /// Circuit breaker config for LLM EMBED operations.
        sharding::CircuitBreaker::Config embed_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        /// Circuit breaker config for LLM FINETUNE operations.
        sharding::CircuitBreaker::Config finetune_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        /// Optional Wave C C1 safety gate applied to INFER/INFER STREAMING/RAG outputs.
        bool enable_c1_cai_safety_gate = false;
        double c1_min_safety_score = 0.80;
        CAISafetyEvalFn c1_cai_eval_fn;

        /// Optional Wave C C2 telemetry forwarding for runtime output metrics.
        bool enable_c2_federated_telemetry = false;
        FederatedTelemetryFn c2_federated_telemetry_fn;

        /// @brief Parser service for AQL validation (Phase 0.3 integration)
        /// When provided, enables parser-based validation with retry logic.
        /// If null, falls back to string-level AQLQueryValidator (backward compat).
        std::shared_ptr<query::AQLParserService> parser_service = nullptr;

        /// @brief Validation pipeline configuration (Phase 0.3 integration)
        /// Controls retry behavior, timeout, and feedback strategies.
        LLMValidationPipelineConfig validation_config{
            .max_retries = 1,
            .timeout_ms = 5000,
            .reject_on_error = false,
            .log_level = "warn"
        };

        /// @brief LLM client for text/AQL generation (Phase 0.4 integration)
        /// When provided, enables full validation pipeline with retry feedback.
        /// If null, creates a default mock client automatically.
        std::shared_ptr<llm::LLMClient> llm_client = nullptr;
    };

    /**
     * @brief Snapshot of circuit breaker states for all operation types.
     *
     * Returned by @c getCircuitBreakerStates() for observability.
     * Each string is one of "CLOSED", "OPEN", or "HALF_OPEN" as produced
     * by @c sharding::CircuitBreaker::stateToString().
     */
    struct CircuitBreakerStates {
        std::string infer;    ///< "CLOSED", "OPEN", or "HALF_OPEN"
        std::string rag;      ///< "CLOSED", "OPEN", or "HALF_OPEN"
        std::string embed;    ///< "CLOSED", "OPEN", or "HALF_OPEN"
        std::string finetune; ///< "CLOSED", "OPEN", or "HALF_OPEN"
    };

    LLMAQLHandler();
    explicit LLMAQLHandler(const Config& config);
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

    /**
     * @brief Return the current state of all per-operation circuit breakers.
     *
     * Each state string is one of "CLOSED", "OPEN", or "HALF_OPEN" as produced
     * by @c sharding::CircuitBreaker::stateToString().
     * Intended for observability dashboards and the @c LLM STATS output.
     */
    CircuitBreakerStates getCircuitBreakerStates() const;

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
     * @brief Translate a batch of natural language queries to AQL in parallel.
     *
     * Dispatches up to @p max_concurrent_requests translations concurrently
     * using @c std::async(std::launch::async, ...) and a
     * @c std::counting_semaphore to bound the number of simultaneous LLM
     * inferences.  Results are collected in the original request order.
     *
     * Individual translation failures are captured in the result's @c error
     * field so that a single failure does not abort the rest of the batch.
     *
     * @param requests               Vector of NL queries with optional schema
     *                               contexts.
     * @param max_concurrent_requests Maximum number of translations executed
     *                               simultaneously.  Pass @c 0 (the default)
     *                               to use @c std::thread::hardware_concurrency().
     * @return Vector of results in the same order as the input requests.
     */
    std::vector<BatchNLToAQLResult> translateBatchNLToAQL(
        const std::vector<BatchNLToAQLRequest>& requests,
        std::size_t max_concurrent_requests = 0
    );

    /**
     * @brief Asynchronous overload of translateBatchNLToAQL().
     *
     * Launches the entire parallel batch on a background thread and returns
     * immediately with a future that resolves to the completed result vector.
     *
     * @param requests               Vector of NL queries with optional schema
     *                               contexts (moved into the background task).
     * @param max_concurrent_requests Maximum number of translations executed
     *                               simultaneously.  Pass @c 0 (the default)
     *                               to use @c std::thread::hardware_concurrency().
     * @return Future that resolves to a vector of results in the same order
     *         as the input requests.
     *
     * @warning The caller **must** ensure that this @c LLMAQLHandler instance
     *          outlives the returned future (i.e. call @c future.get() before
     *          destroying the handler).  The background task holds a raw
     *          @c this pointer; destroying the handler while the future is
     *          still pending produces undefined behaviour.
     */
    std::future<std::vector<BatchNLToAQLResult>> translateBatchNLToAQLAsync(
        std::vector<BatchNLToAQLRequest> requests,
        std::size_t max_concurrent_requests = 0
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
        float                    score = 0;       ///< 0.0 (worst) to 1.0 (best); -1.0 = unavailable
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

    // =========================================================================
    // Phase 0.3: Parser service configuration for AST-based validation
    // =========================================================================

    /**
     * @brief Inject a custom AQL parser service for AST-based validation.
     *
     * When provided, all translateNLToAQL*() calls will use AST parsing via this
     * service instead of the fallback string-level AQLQueryValidator. This enables:
     *  - Detailed parser diagnostics (line/column, error category, suggestions)
     *  - Retry loop with meaningful LLM feedback
     *  - Guaranteed valid AQL before returning to user
     *
     * If not set, creates a default parser service automatically.
     *
     * @param parser_service  Shared pointer to AQLParserService. Pass nullptr to disable.
     */
    void setParserService(std::shared_ptr<query::AQLParserService> parser_service);

    /**
     * @brief Retrieve the current AQL parser service.
     * @return Shared pointer to the parser service (may be nullptr if not set).
     */
    std::shared_ptr<query::AQLParserService> getParserService() const;

    /**
     * @brief Configure validation pipeline behavior (retry count, timeout, etc).
     *
     * Phase 0.3 integration: Allows fine-grained control over retry logic and timing
     * for generated AQL validation.
     *
     * @param config  New validation pipeline configuration.
     */
    void setValidationPipelineConfig(const LLMValidationPipelineConfig& config);

    /**
     * @brief Retrieve the current validation pipeline configuration.
     * @return Current validation pipeline config.
     */
    LLMValidationPipelineConfig getValidationPipelineConfig() const;

    // =========================================================================
    // Phase 0.4: LLM Client configuration for full validation pipeline
    // =========================================================================

    /**
     * @brief Inject a custom LLM client for AQL generation.
     *
     * When provided, enables the full validation pipeline with retry feedback.
     * The LLM client is used by LLMValidationPipeline::execute() to generate
     * AQL from natural language queries.
     *
     * @param llm_client  Shared pointer to LLMClient implementation.
     *                    Pass nullptr to use default mock client.
     */
    void setLLMClient(std::shared_ptr<llm::LLMClient> llm_client);

    /**
     * @brief Retrieve the current LLM client.
     * @return Shared pointer to the LLM client (may be nullptr).
     */
    std::shared_ptr<llm::LLMClient> getLLMClient() const;

    /**
     * @brief Get the validation pipeline (read-only access).
     * @return Shared pointer to LLMValidationPipeline if wired, nullptr otherwise.
     */
    std::shared_ptr<LLMValidationPipeline> getValidationPipeline() const;

    // =========================================================================
    // Collection-level access control for generated AQL (LLM-2 fix)
    // =========================================================================


    /**
     * @brief Register a per-collection access checker for NL→AQL translation.
     *
     * When set, every `translateNLToAQL*()` call passes the generated AQL
     * through the AQL parser, extracts all referenced collection names (from
     * FOR…IN, REMOVE … IN, UPSERT INTO, UPDATE … IN, REPLACE … IN, INSERT
     * INTO clauses), and invokes @p checker for each one.  If the checker
     * returns `false` for any collection, the call throws
     * `LLMException(ACCESS_DENIED)` — the generated query is not returned.
     *
     * Set to an empty `std::function` (the default) to disable the check.
     *
     * Typical integration:
     * @code
     * handler.setCollectionAccessChecker([&acl, user_id](const std::string& col) {
     *     return acl.canRead(user_id, col);
     * });
     * @endcode
     *
     * @param checker  Callable that receives a collection name and returns
     *                 `true` when the caller is authorised to access it,
     *                 `false` to deny access and abort translation.
     */
    void setCollectionAccessChecker(
        std::function<bool(const std::string& collection_name)> checker
    );

    // =========================================================================
    // Runtime-overridable validation limits
    // =========================================================================

    /**
     * @brief Override all input-length and query-count validation limits at runtime.
     *
     * The supplied @p config replaces the handler's internal copy of the
     * limits. All subsequent calls to translateNLToAQL(), translateNLToAQLStreaming(),
     * translateNLToAQLWithExamples(), and the sanitizePromptInput() helper will
     * use the new values. Passing a default-constructed @c ValidationLimitsConfig
     * restores the original @c ValidationLimits constexpr values.
     *
     * Typical use (embedded deployment with tight memory):
     * @code
     * ValidationLimitsConfig cfg;
     * cfg.max_nl_query_length = 512;
     * cfg.max_schema_context_length = 4096;
     * handler.setValidationLimits(cfg);
     * @endcode
     *
     * @param config  New validation limits. All fields carry defaults identical
     *                to the original compile-time constants for backward compatibility.
     */
    void setValidationLimits(const ValidationLimitsConfig& config);

    /**
     * @brief Return the currently active validation limits.
     */
    ValidationLimitsConfig getValidationLimits() const;

    /**
     * @brief Override LLM timeout settings at runtime.
     *
     * Replaces the handler's internal @c LLMTimeoutManager configuration.
     * Individual timeout values (infer, rag, embed, model-load) are all
     * adjustable without recompilation.
     *
     * @param config  New timeout configuration.
     */
    void setTimeoutConfig(const LLMTimeoutManager::TimeoutConfig& config);
    void setDomainRouteResolver(DomainRouteResolver resolver);
    void setAdaptiveShardRouter(std::shared_ptr<sharding::AdaptiveShardRouter> router);
    void setShardingManager(sharding::ShardingManager* sharding_manager);

    /**
     * @brief Inject a ContinuousBatchScheduler for live LLM-queue telemetry.
     *
     * When both a scheduler and an AdaptiveShardRouter are set, the scheduler's
     * ShardLoadCallback is wired to call
     * @c AdaptiveShardRouter::updateShardLLMLoad() on every queue-depth change
     * so that LEAST_LOADED routing decisions reflect actual LLM queue pressure.
     *
     * @param sched           Pointer to the scheduler.  Pass @c nullptr to detach.
     *                        Ownership is NOT transferred.
     * @param local_shard_id  Shard identifier reported to the router for this node.
     */
    void setBatchScheduler(llm::ContinuousBatchScheduler* sched,
                           std::string local_shard_id);

    /**
     * @brief Inject a KVPrefixTransferManager for Phase 5 cross-shard KV
     *        prefix transfer.
     *
     * When set and a domain-routing decision selects a remote shard for an
     * @c executeInfer() call, the handler will ask the manager to transfer the
     * computed KV prefix (system-prompt) to that shard before returning the
     * inference result.  The transfer is best-effort; a failure never fails
     * the inference request.
     *
     * @param mgr  Owning pointer to the manager.  Pass @c nullptr to disable.
     */
    void setKVPrefixTransferManager(std::unique_ptr<llm::KVPrefixTransferManager> mgr);

    // =========================================================================
    // Test / dependency injection
    // =========================================================================

    /**
     * @brief Override the LLM chat backend used by translateNLToAQL() and
     *        related methods.
     *
     * When set, @p executor is called instead of
     * @c EmbeddedLLMManager::instance().get().chat() every time the handler
     * needs a chat completion.  Pass @c nullptr to restore the default
     * live-LLM path.
     *
     * **Intended for unit tests only.**  Production code must not inject a
     * custom executor.
     *
     * @param executor  Callable with signature
     *                  @c std::string(const std::vector<llm::ChatMessage>&).
     */
    void setChatExecutor(
        std::function<std::string(const std::vector<llm::ChatMessage>&)> executor
    );

    /**
     * @brief Inject a custom token estimator for all token-budget checks.
     *
     * Replaces the default `CharDivisionEstimator` (ratio = 4) with the
     * provided implementation.  The handler takes ownership of the estimator.
     * Pass @c nullptr to restore the default `CharDivisionEstimator`.
     *
     * Typical production use: supply a `TiktokenEstimator` wrapping the
     * llama.cpp tokenizer for accurate BPE token counts.
     *
     * @param estimator  Polymorphic token estimator; if null the default
     *                   `CharDivisionEstimator{4}` is reinstated.
     */
    void setTokenEstimator(std::unique_ptr<TokenEstimator> estimator);

    // =========================================================================
    // Ingestion bridge (optional enrichment)
    // =========================================================================

    /**
     * @brief Attach an `AQLIngestionBridge` to enable entity-context enrichment
     *        of `translateNLToAQL()` calls.
     *
     * When set, `translateNLToAQL()` passes @p nl_query through the bridge's
     * `extractEntitiesForContext()` and appends the resulting entity context
     * string to the @p schema_context before constructing the LLM prompt.
     * This improves NL→AQL translation accuracy for queries that reference
     * domain entities (legal provisions, organisations, etc.).
     *
     * All other translation methods (streaming, batch, with-examples) also
     * benefit from the injected entity context when a bridge is set.
     *
     * Pass @c nullptr to detach any previously set bridge (no-op).
     *
     * @param bridge  Shared `AQLIngestionBridge` instance, or nullptr.
     */
    void setIngestionBridge(std::shared_ptr<AQLIngestionBridge> bridge);

    /**
     * @brief Return the currently attached `AQLIngestionBridge`, or nullptr.
     */
    std::shared_ptr<AQLIngestionBridge> ingestionBridge() const;

    /**
     * @brief Inject a storage layer so that RAG result documents can be
     *        hydrated with their actual content instead of just the primary key.
     *
     * Call this before invoking executeRAG() / executeCommand(…, "RAG", …).
     * When not set, doc.content carries the primary key (backward-compatible).
     */
    void setStorage(std::shared_ptr<RocksDBWrapper> storage);

    /**
     * @brief Factory: create an `IEmbeddingProvider` backed by this handler's
     *        `executeEmbed()` circuit.
     *
     * The returned bridge can be injected into an `AQLFewShotExampleLibrary`
     * to enable semantic (cosine-similarity) few-shot example ranking instead
     * of the default Jaccard word-overlap metric:
     *
     * @code
     * auto bridge = handler.makeEmbeddingBridge();
     * library.setEmbeddingProvider(bridge.get());
     * library.rebuildEmbeddingIndex();
     * @endcode
     *
     * The bridge holds a non-owning reference to @c *this — the returned
     * pointer must not outlive this handler.
     *
     * Consumers must `#include "aql/llm_aql_embedding_bridge.h"` to access
     * the concrete type; this header only sees the forward declaration.
     *
     * @return `unique_ptr` owning the bridge (concrete type: `LLMAQLEmbeddingBridge`).
     */
    std::unique_ptr<IEmbeddingProvider> makeEmbeddingBridge();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // =========================================================================
    // Private prompt-building and post-processing helpers
    // =========================================================================

    /**
     * @brief Build the NL-to-AQL system prompt shared by all three translation methods.
     *
     * Centralises prompt construction to eliminate copy-paste duplication across
     * translateNLToAQL(), translateNLToAQLStreaming(), and
     * translateNLToAQLWithExamples().  The returned string is ready to be used as
     * the "system" chat message.
     *
     * @param schema_context  Optional database schema description (may be empty).
     * @param examples        Optional few-shot examples to inject (may be empty).
     * @param validation_feedback  When non-empty, appended as error feedback for
     *        retry attempts.
     * @return Fully assembled system prompt string.
     */
    std::string buildNLToAQLSystemPrompt(
        const std::string& schema_context,
        const std::vector<AQLFewShotExample>& examples = {},
        const std::string& validation_feedback = ""
    ) const;

    /**
     * @brief Strip a surrounding markdown code fence from @p raw.
     *
     * Handles both plain @c ``` and language-tagged @c ```aql fences.
     * Returns the untouched input when no fence is detected.
     *
     * @param raw  Raw LLM response, potentially wrapped in a markdown fence.
     *             Passed by value to enable the caller to move-in the source
     *             string (e.g. `stripMarkdownFences(std::move(response))`),
     *             avoiding a copy when the fence is absent.
     * @return Query string with fences stripped and leading/trailing whitespace removed.
     */
    static std::string stripMarkdownFences(std::string raw);

    /**
     * @brief Log syntax-highlighter annotations as a single @c spdlog::warn call.
     *
     * @param annotations   Annotations from AQLSyntaxHighlighter::annotateErrors().
     * @param query_preview Short description or prefix of the query (for log context).
     * @param function_name Calling function name (for log context).
     */
    static void logAnnotations(
        const std::vector<AQLAnnotation>& annotations,
        const std::string& query_preview,
        const std::string& function_name
    );
};

} // namespace aql
} // namespace themis
