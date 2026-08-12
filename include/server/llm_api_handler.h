/**
 * @file llm_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <nlohmann/json.hpp>
#include <auth/jwt_validator.h>

// Forward declarations for LLM components
namespace themis {
namespace llm {
class ILLMPlugin;
class LLMPluginManager;
class AsyncInferenceEngine;
class FeedbackStore;
}
namespace aql {
class LLMAQLHandler;
}
namespace auth {
class JWTValidator;
}
namespace governance {
class PolicyEngine;
}
namespace query {
class QueryEngine;
}
namespace server {
class LoRAApiHandler;
}
class VectorIndexManager;
class RocksDBWrapper;
}

namespace themis::server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

/**
 * @brief LLM API Handler for ThemisDB HTTP Server
 * 
 * Implements RESTful endpoints for LLM operations:
 * - POST /api/v1/llm/inference - Standard text generation
 * - POST /api/v1/llm/rag - Retrieval-Augmented Generation
 * - POST /api/v1/llm/embed - Generate embeddings
 * - GET  /api/v1/llm/stream - Server-Sent Events streaming (general LLM inference)
 * - POST /api/v1/llm/aql/explain/stream - Stream AQL natural language explanation as SSE
 * - GET  /api/v1/llm/models - List available models
 * - POST /api/v1/llm/models/load - Load a model
 * - POST /api/v1/llm/models/unload - Unload a model
 * - POST /api/v1/llm/models/ingest - Ingest new model (streaming upload)
 * - GET  /api/v1/llm/loras - List LoRA adapters
 * - POST /api/v1/llm/loras/load - Load LoRA adapter
 * - POST /api/v1/llm/loras/unload - Unload LoRA adapter
 * - GET  /api/v1/llm/stats - Get performance statistics
 * - GET  /api/v1/llm/cache/stats - Get cache statistics
 * - DELETE /api/v1/llm/cache - Clear caches
 * - GET  /api/v1/llm/health - Health check
 * - POST /api/v1/llm/docs/query - Query documentation assistant
 * - POST /api/v1/llm/docs/config - Get configuration help
 * - POST /api/v1/llm/docs/troubleshoot - Get troubleshooting help
 * - POST /api/v1/llm/feedback - Submit user feedback
 * - GET  /api/v1/llm/feedback/{id} - Retrieve specific feedback
 * - GET  /api/v1/llm/feedback - List feedback with filters
 * - GET  /api/v1/llm/feedback/stats - Get feedback statistics
 * - POST /v1/chat/completions - OpenAI-compatible chat completions passthrough
 * - GET  /v1/models - OpenAI-compatible model list
 * 
 * All endpoints require Bearer Token (JWT) authentication via Authorization header.
 * Exception: /v1/chat/completions and /v1/models accept the API key in the
 * Authorization: Bearer header for drop-in OpenAI SDK compatibility.
 */
class LLMApiHandler {
public:
    /**
     * @brief Construct LLM API handler
     * 
     * @param plugin_manager LLM plugin manager instance
     * @param jwt_config Optional JWT validator configuration
     */
    explicit LLMApiHandler(
        std::shared_ptr<llm::LLMPluginManager> plugin_manager,
        std::optional<auth::JWTValidatorConfig> jwt_config = std::nullopt);
    
    /**
     * @brief Configure JWT validation after construction
     * 
     * @param config JWT validator configuration
     */
    void configureJWT(const auth::JWTValidatorConfig& config);
    
    /**
     * @brief Set LoRA API handler for delegating LoRA-specific requests
     * 
     * @param lora_handler LoRA API handler instance
     */
    void setLoRAHandler(std::shared_ptr<LoRAApiHandler> lora_handler);
    
    /**
     * @brief Set FeedbackStore for persisting user feedback
     * 
     * @param feedback_store FeedbackStore instance
     */
    void setFeedbackStore(std::shared_ptr<llm::FeedbackStore> feedback_store);

    /**
     * @brief Attach a PolicyEngine for inference permission checks on the
     *        OpenAI-compatible @c /v1/chat/completions endpoint.
     *
     * When set, @c handleOpenAIChatCompletions() calls
     * @c PolicyEngine::checkInferencePermission() before processing the request
     * and returns HTTP 401/403 on denial.  Pass @c nullptr to detach (disables
     * the policy gate; JWT validation still applies).
     *
     * @param policy_engine Pointer to a PolicyEngine instance.  The caller is
     *                      responsible for the lifetime of the engine.
     */
    void setPolicyEngine(governance::PolicyEngine* policy_engine);

    /**
     * @brief Wire a QueryEngine for RAG vector retrieval.
     *
     * When set, handleRAG() embeds the user query via the LLM plugin and
     * executes a filtered vector search on the named collection before
     * forwarding the retrieved documents to LLMPluginManager::generateRAG().
     */
    void setQueryEngine(std::shared_ptr<query::QueryEngine> query_engine);
    
    /**
     * @brief Handle LLM API request
     * 
     * Routes request to appropriate handler based on path and method.
     * Validates JWT Bearer Token authentication.
     * Delegates LoRA-specific requests to LoRAApiHandler if configured.
     * 
     * @param req HTTP request
     * @return HTTP response (JSON)
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

private:
    // Core inference endpoints
    http::response<http::string_body> handleInference(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleRAG(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleEmbed(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleStreamInference(
        const http::request<http::string_body>& req);
    
    // AQL streaming explanation endpoint
    http::response<http::string_body> handleStreamExplainAql(
        const http::request<http::string_body>& req);
    
    // Model management endpoints
    http::response<http::string_body> handleListModels(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleLoadModel(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleUnloadModel(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleModelInfo(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleIngestModel(
        const http::request<http::string_body>& req);
    
    // LoRA management endpoints
    http::response<http::string_body> handleListLoRAs(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleLoadLoRA(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleUnloadLoRA(
        const http::request<http::string_body>& req);
    
    // Statistics and health
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleCacheStats(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleClearCache(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleHealth(
        const http::request<http::string_body>& req);
    
    // Documentation assistant endpoints
    http::response<http::string_body> handleDocsQuery(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleDocsConfig(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleDocsTroubleshoot(
        const http::request<http::string_body>& req);
    
    // Feedback endpoints
    http::response<http::string_body> handleCreateFeedback(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleGetFeedback(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleListFeedback(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleFeedbackStats(
        const http::request<http::string_body>& req);

    // OpenAI-compatible endpoints
    http::response<http::string_body> handleOpenAIChatCompletions(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleOpenAIListModels(
        const http::request<http::string_body>& req);
    
    // Helper methods
    bool validateBearerToken(const http::request<http::string_body>& req);
    
    http::response<http::string_body> createErrorResponse(
        http::status status,
        std::string_view error,
        std::string_view details = "");
    
    http::response<http::string_body> createJsonResponse(
        const json& data,
        http::status status = http::status::ok);
    
    std::optional<json> parseRequestBody(
        const http::request<http::string_body>& req);
    
    std::shared_ptr<llm::LLMPluginManager> plugin_manager_;
    std::unique_ptr<auth::JWTValidator> jwt_validator_;
    std::shared_ptr<LoRAApiHandler> lora_handler_;
    std::shared_ptr<llm::FeedbackStore> feedback_store_;
    /// Optional governance policy engine for /v1/chat/completions permission checks.
    /// Raw non-owning pointer; nullptr when not configured.
    governance::PolicyEngine* policy_engine_ = nullptr;
    /// Optional query engine for RAG vector retrieval.
    std::shared_ptr<query::QueryEngine> query_engine_;
};

} // namespace themis::server
