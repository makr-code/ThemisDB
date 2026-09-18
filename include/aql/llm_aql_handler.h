/**
 * @file llm_aql_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

inline constexpr uint32_t LLM_AQL_HANDLER_API_VERSION = 100; // v1.0

enum class TranslationValidationMode {
    WARN_ONLY,       ///< Log validation errors as warnings; return the query as-is (default)
    REJECT_ON_ERROR, ///< Throw LLMException(INVALID_RESPONSE) when any ERROR-severity issue is found
    RETRY_ON_ERROR,  ///< Re-invoke the LLM with error feedback; throw after all retries exhausted
};

struct ConversationTurn {
    std::string nl_query;   ///< Natural language query from the user
    std::string aql_result; ///< AQL query generated for this turn
};

class AQLConversationSession {
public:
    /**
     * @brief Add Turn.
     * @param[in] nl_query Input parameter.
     * @param[in] aql_result Input parameter.
     */
    void addTurn(const std::string& nl_query, const std::string& aql_result);

    /**
     * @brief Return a bounded slice of the recent retention action history.
     * @return Most recent actions, or the full history when the limit is zero or oversized.
     */
    const std::vector<ConversationTurn>& getHistory() const;

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Empty.
     * @return True when the operation succeeds.
     */
    bool empty() const;

    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

private:
    std::vector<ConversationTurn> history_;
};

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

    struct Config {
        sharding::CircuitBreaker::Config infer_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        sharding::CircuitBreaker::Config rag_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        sharding::CircuitBreaker::Config embed_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        sharding::CircuitBreaker::Config finetune_circuit_breaker{
            .failure_threshold = 5,
            .timeout           = std::chrono::seconds(60),
            .success_threshold = 2,
            .failure_window    = std::chrono::seconds(120)
        };

        bool enable_c1_cai_safety_gate = false;
        double c1_min_safety_score = 0.80;
        CAISafetyEvalFn c1_cai_eval_fn;

        bool enable_c2_federated_telemetry = false;
        FederatedTelemetryFn c2_federated_telemetry_fn;

        std::shared_ptr<query::AQLParserService> parser_service = nullptr;

        LLMValidationPipelineConfig validation_config{
            .max_retries = 1,
            .timeout_ms = 5000,
            .reject_on_error = false,
            .log_level = "warn"
        };

        std::shared_ptr<llm::LLMClient> llm_client = nullptr;
    };

    struct CircuitBreakerStates {
        std::string infer;    ///< "CLOSED", "OPEN", or "HALF_OPEN"
        std::string rag;      ///< "CLOSED", "OPEN", or "HALF_OPEN"
        std::string embed;    ///< "CLOSED", "OPEN", or "HALF_OPEN"
        std::string finetune; ///< "CLOSED", "OPEN", or "HALF_OPEN"
    };

    LLMAQLHandler();
    /**
     * @brief LLMAQLHandler.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LLMAQLHandler(const Config& config);
    ~LLMAQLHandler();

    // Inference commands
    std::string executeInfer(
        const std::string& prompt,
        const std::string& model_id = "",
        const std::string& lora_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

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
    /**
     * @brief Execute Model Load.
     * @param[in] model_id Identifier of the model.
     * @param[in] path Input parameter.
     */
    void executeModelLoad(const std::string& model_id, const std::string& path);
    /**
     * @brief Execute Model Unload.
     * @param[in] model_id Identifier of the model.
     */
    void executeModelUnload(const std::string& model_id);
    /**
     * @brief Execute Model List.
     * @return Return value.
     */
    std::vector<std::string> executeModelList();
    /**
     * @brief Execute Model Ingest.
     * @param[in] model_id Identifier of the model.
     * @param[in] blob_urn Input parameter.
     */
    void executeModelIngest(const std::string& model_id, const std::string& blob_urn);

    // LoRA management commands
    /**
     * @brief Execute Lo RALoad.
     * @param[in] lora_id Identifier of the lora.
     * @param[in] path Input parameter.
     */
    void executeLoRALoad(const std::string& lora_id, const std::string& path);
    /**
     * @brief Execute Lo RAUnload.
     * @param[in] lora_id Identifier of the lora.
     */
    void executeLoRAUnload(const std::string& lora_id);
    /**
     * @brief Execute Lo RAList.
     * @return Return value.
     */
    std::vector<std::string> executeLoRAList();

    // Statistics commands
    /**
     * @brief Execute Stats.
     * @return Return value.
     */
    std::string executeStats();
    /**
     * @brief Execute Cache Stats.
     * @return Return value.
     */
    std::string executeCacheStats();
    /**
     * @brief Execute Cache Clear.
     */
    void executeCacheClear();

    /**
     * @brief Get Circuit Breaker States.
     * @return Return value.
     */
    CircuitBreakerStates getCircuitBreakerStates() const;

    // Batch optimization
    struct BatchInferRequest {
        std::string prompt;
        std::string model_id;
        std::string lora_id;
        std::unordered_map<std::string, std::string> options;
    };

    /**
     * @brief Execute Batch Infer.
     * @param[in] requests Input parameter.
     * @return Return value.
     */
    std::vector<std::string> executeBatchInfer(
        const std::vector<BatchInferRequest>& requests
    );

    // Natural Language to AQL Translation
    std::string translateNLToAQL(
        const std::string& nl_query,
        const std::string& schema_context = ""
    );

    std::string translateNLToAQLStreaming(
        const std::string& nl_query,
        std::function<void(const std::string& token)> token_callback,
        const std::string& schema_context = ""
    );

    struct AQLTranslationResult {
        std::string aql_query;           ///< Generated AQL query
        AQLConfidenceScore confidence;   ///< Confidence score for the generated query
    };

    AQLTranslationResult translateNLToAQLWithConfidence(
        const std::string& nl_query,
        const std::string& schema_context = ""
    );

    std::string translateNLToAQLWithExamples(
        const std::string& nl_query,
        const AQLFewShotExampleLibrary& library,
        const std::string& schema_context = "",
        std::size_t max_examples = 3
    );

    // Batch NL-to-AQL Translation for offline workloads
    struct BatchNLToAQLRequest {
        std::string nl_query;       ///< Natural language query
        std::string schema_context; ///< Optional database schema context
    };

    struct BatchNLToAQLResult {
        std::string aql_query; ///< Translated AQL query; empty when translation failed
        std::string error;     ///< Error message if translation failed; empty on success
        bool success;          ///< true if translation succeeded
    };

    std::vector<BatchNLToAQLResult> translateBatchNLToAQL(
        const std::vector<BatchNLToAQLRequest>& requests,
        std::size_t max_concurrent_requests = 0
    );

    std::future<std::vector<BatchNLToAQLResult>> translateBatchNLToAQLAsync(
        std::vector<BatchNLToAQLRequest> requests,
        std::size_t max_concurrent_requests = 0
    );

    // Conversation/Chat Support
    std::string executeChat(
        const std::vector<llm::ChatMessage>& messages,
        const std::string& model_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

    // AQL Syntax Highlighting
    HighlightedResponse formatLLMResponse(
        const std::string& llm_response,
        bool use_ansi = true
    ) const;
    // =========================================================================
    // Streaming natural language explanations
    // =========================================================================

    std::string streamExplainAQL(
        const std::string& aql_query,
        std::function<void(const std::string& token)> stream_callback,
        const std::string& schema_context = ""
    );

    std::string streamExplainAQLAsSSE(
        const std::string& aql_query,
        std::function<void(const std::string& sse_event)> stream_callback,
        const std::string& request_id = "",
        const std::string& schema_context = ""
    );

    // =========================================================================
    // Confidence scoring
    // =========================================================================

    struct QueryConfidenceScore {
        float                    score = 0;       ///< 0.0 (worst) to 1.0 (best); -1.0 = unavailable
        std::string              explanation; ///< Why this score was assigned
        std::vector<std::string> suggestions; ///< Concrete improvement suggestions
    };

    QueryConfidenceScore scoreQueryConfidence(
        const std::string& aql_query,
        const std::string& original_intent  = "",
        const std::string& schema_context   = ""
    );

    // =========================================================================
    // Post-generation validation mode
    // =========================================================================

    /**
     * @brief Set Validation Mode.
     * @param[in] mode Input parameter.
     */
    void setValidationMode(TranslationValidationMode mode);

    /**
     * @brief Get Validation Mode.
     * @return Return value.
     */
    TranslationValidationMode getValidationMode() const;

    /**
     * @brief ========================================================================= Phase 0.
     * @param[in] parser_service Input parameter.
     * @details 3: Parser service configuration for AST-based validation =========================================================================
     */

    void setParserService(std::shared_ptr<query::AQLParserService> parser_service);

    /**
     * @brief Get Parser Service.
     * @return Return value.
     */
    std::shared_ptr<query::AQLParserService> getParserService() const;

    /**
     * @brief Set Validation Pipeline Config.
     * @param[in] config Input parameter.
     */
    void setValidationPipelineConfig(const LLMValidationPipelineConfig& config);

    /**
     * @brief Get Validation Pipeline Config.
     * @return Return value.
     */
    LLMValidationPipelineConfig getValidationPipelineConfig() const;

    /**
     * @brief ========================================================================= Phase 0.
     * @param[in] llm_client Input parameter.
     * @details 4: LLM Client configuration for full validation pipeline =========================================================================
     */

    void setLLMClient(std::shared_ptr<llm::LLMClient> llm_client);

    /**
     * @brief Get LLMClient.
     * @return Return value.
     */
    std::shared_ptr<llm::LLMClient> getLLMClient() const;

    /**
     * @brief Get Validation Pipeline.
     * @return Return value.
     */
    std::shared_ptr<LLMValidationPipeline> getValidationPipeline() const;

    // =========================================================================
    // Collection-level access control for generated AQL (LLM-2 fix)
    // =========================================================================


    void setCollectionAccessChecker(
        std::function<bool(const std::string& collection_name)> checker
    );

    // =========================================================================
    // Runtime-overridable validation limits
    // =========================================================================

    /**
     * @brief Set Validation Limits.
     * @param[in] config Input parameter.
     */
    void setValidationLimits(const ValidationLimitsConfig& config);

    /**
     * @brief Get Validation Limits.
     * @return Return value.
     */
    ValidationLimitsConfig getValidationLimits() const;

    /**
     * @brief Set Timeout Config.
     * @param[in] config Input parameter.
     */
    void setTimeoutConfig(const LLMTimeoutManager::TimeoutConfig& config);
    /**
     * @brief Set Domain Route Resolver.
     * @param[in] resolver Input parameter.
     */
    void setDomainRouteResolver(DomainRouteResolver resolver);
    /**
     * @brief Set Adaptive Shard Router.
     * @param[in] router Input parameter.
     */
    void setAdaptiveShardRouter(std::shared_ptr<sharding::AdaptiveShardRouter> router);
    /**
     * @brief Set Sharding Manager.
     * @param[in,out] sharding_manager Input/output parameter.
     */
    void setShardingManager(sharding::ShardingManager* sharding_manager);

    /**
     * @brief Set Batch Scheduler.
     * @param[in,out] sched Input/output parameter.
     * @param[in] local_shard_id Identifier of the local shard.
     */
    void setBatchScheduler(llm::ContinuousBatchScheduler* sched,
                           std::string local_shard_id);

    /**
     * @brief Set KVPrefix Transfer Manager.
     * @param[in] mgr Input parameter.
     */
    void setKVPrefixTransferManager(std::unique_ptr<llm::KVPrefixTransferManager> mgr);

    // =========================================================================
    // Test / dependency injection
    // =========================================================================

    void setChatExecutor(
        std::function<std::string(const std::vector<llm::ChatMessage>&)> executor
    );

    /**
     * @brief Set Token Estimator.
     * @param[in] estimator Input parameter.
     */
    void setTokenEstimator(std::unique_ptr<TokenEstimator> estimator);

    // =========================================================================
    // Ingestion bridge (optional enrichment)
    // =========================================================================

    /**
     * @brief Set Ingestion Bridge.
     * @param[in] bridge Input parameter.
     */
    void setIngestionBridge(std::shared_ptr<AQLIngestionBridge> bridge);

    /**
     * @brief Ingestion Bridge.
     * @return Return value.
     */
    std::shared_ptr<AQLIngestionBridge> ingestionBridge() const;

    /**
     * @brief Set Storage.
     * @param[in] storage Input parameter.
     */
    void setStorage(std::shared_ptr<RocksDBWrapper> storage);

    /**
     * @brief Make Embedding Bridge.
     * @return Return value.
     */
    std::unique_ptr<IEmbeddingProvider> makeEmbeddingBridge();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // =========================================================================
    // Private prompt-building and post-processing helpers
    // =========================================================================

    std::string buildNLToAQLSystemPrompt(
        const std::string& schema_context,
        const std::vector<AQLFewShotExample>& examples = {},
        const std::string& validation_feedback = ""
    ) const;

    /**
     * @brief Strip Markdown Fences.
     * @param[in] raw Input parameter.
     * @return Return value.
     */
    static std::string stripMarkdownFences(std::string raw);

    /**
     * @brief Log Annotations.
     * @param[in] annotations Input parameter.
     * @param[in] query_preview Input parameter.
     * @param[in] function_name Name of the function.
     */
    static void logAnnotations(
        const std::vector<AQLAnnotation>& annotations,
        const std::string& query_preview,
        const std::string& function_name
    );
};

} // namespace aql
} // namespace themis
