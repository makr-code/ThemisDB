#pragma once

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <optional>

// Forward declarations for LLM components
namespace themis {
namespace llm {
class ILLMPlugin;
class LLMPluginManager;
class AsyncInferenceEngine;
}
}

namespace themis::server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

/**
 * @brief LLM API Handler for ThemisDB HTTP Server
 * 
 * Implements RESTful endpoints for LLM operations:
 * - POST /api/v1/llm/inference - Standard text generation
 * - POST /api/v1/llm/rag - Retrieval-Augmented Generation
 * - POST /api/v1/llm/embed - Generate embeddings
 * - GET  /api/v1/llm/stream - Server-Sent Events streaming
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
 * 
 * All endpoints require Bearer Token (JWT) authentication via Authorization header.
 */
class LLMApiHandler {
public:
    /**
     * @brief Construct LLM API handler
     * 
     * @param plugin_manager LLM plugin manager instance
     */
    explicit LLMApiHandler(std::shared_ptr<llm::LLMPluginManager> plugin_manager);
    
    /**
     * @brief Handle LLM API request
     * 
     * Routes request to appropriate handler based on path and method.
     * Validates JWT Bearer Token authentication.
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
    
    // Helper methods
    bool validateBearerToken(const http::request<http::string_body>& req);
    
    http::response<http::string_body> createErrorResponse(
        http::status status,
        std::string_view error,
        std::string_view details = "");
    
    http::response<http::string_body> createJsonResponse(
        const json::object& data,
        http::status status = http::status::ok);
    
    std::optional<json::object> parseRequestBody(
        const http::request<http::string_body>& req);
    
    std::shared_ptr<llm::LLMPluginManager> plugin_manager_;
};

} // namespace themis::server
