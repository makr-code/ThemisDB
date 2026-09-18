/**
 * @file llm_api_handler.h
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

class LLMApiHandler {
public:
    explicit LLMApiHandler(
        std::shared_ptr<llm::LLMPluginManager> plugin_manager,
        std::optional<auth::JWTValidatorConfig> jwt_config = std::nullopt);
    
    /**
     * @brief Configure JWT.
     * @param[in] config Input parameter.
     */
    void configureJWT(const auth::JWTValidatorConfig& config);
    
    /**
     * @brief Set Lo RAHandler.
     * @param[in] lora_handler Callback that loras the entity.
     */
    void setLoRAHandler(std::shared_ptr<LoRAApiHandler> lora_handler);
    
    /**
     * @brief Set Feedback Store.
     * @param[in] feedback_store Input parameter.
     */
    void setFeedbackStore(std::shared_ptr<llm::FeedbackStore> feedback_store);

    /**
     * @brief Set Policy Engine.
     * @param[in,out] policy_engine Input/output parameter.
     */
    void setPolicyEngine(governance::PolicyEngine* policy_engine);

    /**
     * @brief Set Query Engine.
     * @param[in] query_engine Input parameter.
     */
    void setQueryEngine(std::shared_ptr<query::QueryEngine> query_engine);
    
    /**
     * @brief Handle Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

private:
    // Core inference endpoints
    /**
     * @brief Handle Inference.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleInference(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle RAG.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRAG(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Embed.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEmbed(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Stream Inference.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStreamInference(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Stream Explain Aql.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStreamExplainAql(
        const http::request<http::string_body>& req);
    
    // Model management endpoints
    /**
     * @brief Handle List Models.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListModels(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Load Model.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLoadModel(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Unload Model.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUnloadModel(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Model Info.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleModelInfo(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Ingest Model.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIngestModel(
        const http::request<http::string_body>& req);
    
    // LoRA management endpoints
    /**
     * @brief Handle List Lo RAs.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListLoRAs(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Load Lo RA.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLoadLoRA(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Unload Lo RA.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUnloadLoRA(
        const http::request<http::string_body>& req);
    
    // Statistics and health
    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Cache Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCacheStats(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Clear Cache.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleClearCache(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Health.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealth(
        const http::request<http::string_body>& req);
    
    // Documentation assistant endpoints
    /**
     * @brief Handle Docs Query.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDocsQuery(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Docs Config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDocsConfig(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Docs Troubleshoot.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDocsTroubleshoot(
        const http::request<http::string_body>& req);
    
    // Feedback endpoints
    /**
     * @brief Handle Create Feedback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateFeedback(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Get Feedback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetFeedback(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle List Feedback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListFeedback(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Feedback Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFeedbackStats(
        const http::request<http::string_body>& req);

    // OpenAI-compatible endpoints
    /**
     * @brief Handle Open AIChat Completions.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOpenAIChatCompletions(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Open AIList Models.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOpenAIListModels(
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
    
    std::shared_ptr<llm::LLMPluginManager> plugin_manager_;
    std::unique_ptr<auth::JWTValidator> jwt_validator_;
    std::shared_ptr<LoRAApiHandler> lora_handler_;
    std::shared_ptr<llm::FeedbackStore> feedback_store_;
    governance::PolicyEngine* policy_engine_ = nullptr;
    std::shared_ptr<query::QueryEngine> query_engine_;
};

} // namespace themis::server
