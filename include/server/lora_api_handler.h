/**
 * @file lora_api_handler.h
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

// Forward declarations for LoRA components
namespace themis {
namespace llm {
namespace lora {
class LoRAOrchestrator;
class LoRAStorageService;
class LoRATrainingService;
}
class InferenceEngineEnhanced;
}
namespace auth {
class JWTValidator;
}
}

namespace themis::server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

/**
 * @brief LoRA API Handler for ThemisDB HTTP Server
 * 
 * Implements RESTful endpoints for LoRA framework operations:
 * 
 * LLM Model Management:
 * - POST   /api/v1/llm/models - Register a new model
 * - GET    /api/v1/llm/models/{model_id} - Get model details
 * - GET    /api/v1/llm/models - List models with filters
 * - DELETE /api/v1/llm/models/{model_id} - Delete model
 * 
 * LoRA Adapter Management:
 * - POST   /api/v1/llm/lora/adapters - Create new adapter
 * - GET    /api/v1/llm/lora/adapters/{adapter_id} - Get adapter details
 * - PUT    /api/v1/llm/lora/adapters/{adapter_id} - Update adapter
 * - DELETE /api/v1/llm/lora/adapters/{adapter_id} - Delete adapter
 * - GET    /api/v1/llm/lora/adapters - List adapters with filters
 * 
 * Adapter Lifecycle:
 * - POST   /api/v1/llm/lora/adapters/{adapter_id}/load - Hot-load adapter (returns 202 Accepted + job_id)
 * - POST   /api/v1/llm/lora/adapters/{adapter_id}/unload - Unload adapter
 * - GET    /api/v1/llm/lora/adapters/{adapter_id}/status - Get adapter status
 * - GET    /api/v1/llm/lora/adapters/{adapter_id}/load-status - Get hot-load job status
 * 
 * Inference:
 * - POST   /api/v1/llm/lora/query - Query with LoRA adapter
 * 
 * Health & Monitoring:
 * - GET    /api/v1/llm/lora/stats - Get framework statistics
 * - GET    /api/v1/llm/lora/health - Health check
 *
 * Provenance, Snapshots, and Audit Log:
 * - GET    /api/v1/llm/lora/adapters/{adapter_id}/provenance - Get cryptographic provenance record
 * - POST   /api/v1/llm/lora/adapters/{adapter_id}/provenance - Attach provenance record
 * - GET    /api/v1/llm/lora/adapters/{adapter_id}/audit      - Get Merkle-chained audit log
 * - GET    /api/v1/llm/lora/adapters/{adapter_id}/snapshots  - List MVCC snapshots
 * - POST   /api/v1/llm/lora/adapters/{adapter_id}/verify     - Verify Merkle audit chain integrity
 *
 * All endpoints require Bearer Token (JWT) authentication via Authorization header.
 */
class LoRAApiHandler {
public:
    /**
     * @brief Construct LoRA API handler
     * 
     * @param orchestrator LoRA orchestrator instance
     * @param jwt_config Optional JWT validator configuration
     */
    explicit LoRAApiHandler(
        std::shared_ptr<llm::lora::LoRAOrchestrator> orchestrator,
        std::optional<auth::JWTValidatorConfig> jwt_config = std::nullopt);
    
    /**
     * @brief Configure JWT validation after construction
     * 
     * @param config JWT validator configuration
     */
    void configureJWT(const auth::JWTValidatorConfig& config);

    /**
     * @brief Attach an inference engine for LoRA query execution.
     *
     * When set, POST /api/v1/llm/lora/query routes inference requests
     * through this engine.  Without an engine the endpoint returns 501.
     *
     * @param engine InferenceEngineEnhanced instance (may be null to detach).
     */
    void setInferenceEngine(std::shared_ptr<llm::InferenceEngineEnhanced> engine);
    
    /**
     * @brief Handle LoRA API request
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
    // Model management endpoints
    http::response<http::string_body> handleRegisterModel(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleGetModel(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleListModels(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleDeleteModel(
        const http::request<http::string_body>& req);
    
    // Adapter CRUD endpoints
    http::response<http::string_body> handleCreateAdapter(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleGetAdapter(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleUpdateAdapter(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleDeleteAdapter(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleListAdapters(
        const http::request<http::string_body>& req);
    
    // Adapter lifecycle endpoints
    http::response<http::string_body> handleLoadAdapter(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleUnloadAdapter(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleAdapterStatus(
        const http::request<http::string_body>& req);

    /// GET /api/v1/llm/lora/adapters/{id}/load-status
    /// Returns the status of the latest hot-load job for the adapter.
    http::response<http::string_body> handleHotLoadStatus(
        const http::request<http::string_body>& req);
    
    // Cross-shard sync endpoint
    http::response<http::string_body> handleReceiveAdapter(
        const http::request<http::string_body>& req);
    
    // Inference endpoint
    http::response<http::string_body> handleLoRAQuery(
        const http::request<http::string_body>& req);
    
    // Health & monitoring endpoints
    http::response<http::string_body> handleLoRAStats(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleLoRAHealth(
        const http::request<http::string_body>& req);

    // Provenance, Snapshots, and Audit Log endpoints
    http::response<http::string_body> handleGetProvenance(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleAttachProvenance(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleGetAuditLog(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleListSnapshots(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleVerifyAuditChain(
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
    
    std::string extractPathParameter(
        std::string_view target,
        std::string_view prefix);
    
    std::shared_ptr<llm::lora::LoRAOrchestrator> orchestrator_;
    std::unique_ptr<auth::JWTValidator> jwt_validator_;
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine_;
};

} // namespace themis::server
