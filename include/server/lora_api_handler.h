/**
 * @file lora_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class LoRAApiHandler {
public:
    explicit LoRAApiHandler(
        std::shared_ptr<llm::lora::LoRAOrchestrator> orchestrator,
        std::optional<auth::JWTValidatorConfig> jwt_config = std::nullopt);
    
    /**
     * @brief Configure JWT.
     * @param[in] config Input parameter.
     */
    void configureJWT(const auth::JWTValidatorConfig& config);

    /**
     * @brief Set Inference Engine.
     * @param[in] engine Input parameter.
     */
    void setInferenceEngine(std::shared_ptr<llm::InferenceEngineEnhanced> engine);
    
    /**
     * @brief Handle Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

private:
    // Model management endpoints
    /**
     * @brief Handle Register Model.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRegisterModel(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Get Model.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetModel(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle List Models.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListModels(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Delete Model.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteModel(
        const http::request<http::string_body>& req);
    
    // Adapter CRUD endpoints
    /**
     * @brief Handle Create Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateAdapter(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Get Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetAdapter(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Update Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateAdapter(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Delete Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteAdapter(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle List Adapters.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListAdapters(
        const http::request<http::string_body>& req);
    
    // Adapter lifecycle endpoints
    /**
     * @brief Handle Load Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLoadAdapter(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Unload Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUnloadAdapter(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Adapter Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAdapterStatus(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Hot Load Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHotLoadStatus(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Receive Adapter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReceiveAdapter(
        const http::request<http::string_body>& req);
    
    // Inference endpoint
    /**
     * @brief Handle Lo RAQuery.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLoRAQuery(
        const http::request<http::string_body>& req);
    
    // Health & monitoring endpoints
    /**
     * @brief Handle Lo RAStats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLoRAStats(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Lo RAHealth.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLoRAHealth(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Provenance.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetProvenance(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Attach Provenance.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAttachProvenance(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Audit Log.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetAuditLog(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle List Snapshots.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListSnapshots(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Verify Audit Chain.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleVerifyAuditChain(
        const http::request<http::string_body>& req);
    
    // Helper methods
    /**
     * @brief Validate Bearer Token.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateBearerToken(const http::request<http::string_body>& req);
    
    http::response<http::string_body> createErrorResponse(
        http::status status,
        std::string_view error,
        std::string_view details = "");
    
    http::response<http::string_body> createJsonResponse(
        const json& data,
        http::status status = http::status::ok);
    
    /**
     * @brief Parse Request Body.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    std::optional<json> parseRequestBody(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Extract Path Parameter.
     * @param[in] target Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractPathParameter(
        std::string_view target,
        std::string_view prefix);
    
    std::shared_ptr<llm::lora::LoRAOrchestrator> orchestrator_;
    std::unique_ptr<auth::JWTValidator> jwt_validator_;
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine_;
};

} // namespace themis::server
