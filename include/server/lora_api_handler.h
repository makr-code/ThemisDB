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
#include <functional>
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
     * @brief Authorizes one request.
     * @param req   Incoming request.
     * @param admin True for admin-only endpoints (adapter regeneration,
     *              snapshot-pin release); the server binds these to its
     *              `admin` scope, all other endpoints to the `llm` scope.
     * @return std::nullopt if access is granted, otherwise the error response
     *         (401/403) to return unchanged.
     */
    using RequestAuthorizer = std::function<std::optional<http::response<http::string_body>>(
        const http::request<http::string_body>& req, bool admin)>;

    /**
     * @brief Delegate authentication/authorization to the hosting server
     *        (e.g. HttpServer::requireAccess).
     *
     * When set, it replaces the handler-local JWT check.  Without an
     * authorizer, admin-only endpoints are always rejected (403) because the
     * local JWT validator performs no role/scope check (fail closed).
     */
    void setRequestAuthorizer(RequestAuthorizer authorizer);
    
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
     * @brief POST /api/v1/llm/lora/adapters: submit a real training job.
     *
     * Body: `adapter_id` plus either `dataset_id` (+ optional `limit`, default
     * 10000) — trained via LoRAOrchestrator::createAdapterFromSnapshot() on a
     * pinned MVCC snapshot of `lora_ds:<dataset_id>:*` — or inline `samples`
     * (`[{input, output, context?}]`, ≥ 2; signed but without snapshot
     * reference).  Optional `rank`/`alpha`; `base_model`, if given, must equal
     * the configured training base model.
     * @return 202 with job id and `status_url`; 400 on invalid input; 503 if
     *         a required server component (backend, signer, snapshot reader)
     *         is not configured; 422 if the job failed on submission otherwise.
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

    /**
     * @brief GET /api/v1/llm/lora/jobs/{job_id}: real job state (status,
     *        progress, error, metrics, snapshot lease) from the orchestrator.
     */
    http::response<http::string_body> handleGetJob(
        const http::request<http::string_body>& req);

    /**
     * @brief POST /api/v1/llm/lora/adapters/{source_id}/regenerate (admin):
     *        retrain the source adapter's pinned training snapshot on another
     *        base model via LoRAOrchestrator::regenerateAdapter().
     *
     * Body: `{"target_adapter_id", "base_model", "base_model_gguf",
     * "base_model_gguf_sha256"?, "rank"?, "alpha"?}`.  Returns 202 with the
     * job id; 422/503 if the job failed before training (e.g. snapshot pruned
     * or fingerprint mismatch, missing backend/signer).
     */
    http::response<http::string_body> handleRegenerateAdapter(
        const http::request<http::string_body>& req);

    /**
     * @brief DELETE /api/v1/llm/lora/snapshot-pins/{lease_id} (admin):
     *        release a training-snapshot retention pin.  404 if unknown.
     */
    http::response<http::string_body> handleReleaseSnapshotPin(
        const http::request<http::string_body>& req);

    /// @brief 202 for a queued/running job, 503/422 for a job that failed on submission.
    http::response<http::string_body> submittedJobResponse(const std::string& adapter_id,
                                                           const std::string& job_id);
    
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
    RequestAuthorizer authorizer_;
};

} // namespace themis::server
