/**
 * @file lora_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/lora_api_handler.h"
#include <stdexcept>
#include "server/auth_middleware.h"
#include "auth/jwt_validator.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/lora_framework/lora_dataset_loader.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_config.h"
#include "llm/lora_framework/adapter_consistency_checker.h"
#include "llm/inference_engine_enhanced.h"
#include "utils/zstd_codec.h"
#include "utils/cursor.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <cstdint>
#include <regex>
#include <iostream>
#include "utils/tracing.h"

namespace themis::server {

LoRAApiHandler::LoRAApiHandler(
    std::shared_ptr<llm::lora::LoRAOrchestrator> orchestrator,
    std::optional<auth::JWTValidatorConfig> jwt_config)
    : orchestrator_(std::move(orchestrator)) {
    
    if (jwt_config) {
        jwt_validator_ = std::make_unique<auth::JWTValidator>(*jwt_config);
    }
}

/**
 * @brief Configure JWT.
 * @param[in] config Input parameter.
 * @details Implements configureJWT without additional internal calls.
 */
void LoRAApiHandler::configureJWT(const auth::JWTValidatorConfig& config) {
    jwt_validator_ = std::make_unique<auth::JWTValidator>(config);
}

/**
 * @brief Set Inference Engine.
 * @param[in] engine Input parameter.
 * @details Calls: std::move().
 */
void LoRAApiHandler::setInferenceEngine(
        std::shared_ptr<llm::InferenceEngineEnhanced> engine) {
    inference_engine_ = std::move(engine);
}

void LoRAApiHandler::setRequestAuthorizer(RequestAuthorizer authorizer) {
    authorizer_ = std::move(authorizer);
}

namespace {
constexpr std::string_view kAdaptersPrefix = "/api/v1/llm/lora/adapters/";
constexpr std::string_view kJobsPrefix = "/api/v1/llm/lora/jobs/";
constexpr std::string_view kPinsPrefix = "/api/v1/llm/lora/snapshot-pins/";

/// Admin-only LoRA endpoints (training on other base models, pin lifecycle).
bool isAdminEndpoint(std::string_view path, http::verb method) {
    return (method == http::verb::post && path.starts_with(kAdaptersPrefix) &&
            path.ends_with("/regenerate")) ||
           (method == http::verb::delete_ && path.starts_with(kPinsPrefix));
}

/// Path segment without query string; empty if it contains '/'.
std::string singleSegment(std::string_view path, std::string_view prefix, std::string_view suffix = {}) {
    if (!path.starts_with(prefix) || !path.ends_with(suffix) ||
        path.size() < prefix.size() + suffix.size()) {
        return {};
    }
    std::string_view seg = path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
    if (seg.empty() || seg.find('/') != std::string_view::npos) {
        return {};
    }
    return std::string(seg);
}

const char* jobStatusName(llm::lora::LoRAOrchestrator::JobStatus s) {
    using S = llm::lora::LoRAOrchestrator::JobStatus;
    switch (s) {
        case S::Pending: return "pending";
        case S::Running: return "running";
        case S::Completed: return "completed";
        case S::Failed: return "failed";
        case S::Cancelled: return "cancelled";
    }
    return "unknown";
}

/// Optional rank/alpha overrides from a request body.
std::optional<llm::lora::LoRAHyperparameters> hyperparamsFrom(const json& body) {
    if (!body.contains("rank") && !body.contains("alpha")) {
        return std::nullopt;
    }
    llm::lora::LoRAHyperparameters params;
    params.rank = body.value("rank", params.rank);
    params.alpha = body.value("alpha", params.alpha);
    return params;
}
}  // namespace

/**
 * @brief Handle Request.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), target(), method(), validateBearerToken(), THEMIS_INFO(), std::string(), method_string(), createErrorResponse().
 */
http::response<http::string_body> LoRAApiHandler::handleRequest(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleRequest");
    
    std::string_view target = req.target();
    auto method = req.method();
    const std::string_view path = target.substr(0, target.find('?'));
    const bool admin = isAdminEndpoint(path, method);

    if (authorizer_) {
        // Server-delegated auth (scopes/roles, e.g. HttpServer::requireAccess).
        if (auto denied = authorizer_(req, admin)) {
            THEMIS_INFO("[AUDIT] {} {} path='{}' admin={} result=DENY",
                        std::string(req.method_string()), "lora_api", std::string(target), admin);
            return std::move(*denied);
        }
    } else if (admin) {
        // The local JWT validator has no role check: never grant admin endpoints.
        return createErrorResponse(http::status::forbidden, "Forbidden",
                                   "Admin LoRA endpoints require the server authorizer");
    } else if (!validateBearerToken(req)) {
        // Validate Bearer Token (JWT) authentication for all endpoints.
        // Cross-shard calls to /api/v1/lora/receive are authenticated via
        // Authorization: Bearer <token> forwarded by SecureTransportClient.
        THEMIS_INFO("[AUDIT] {} {} path='{}' user='<unauthenticated>' result=DENY",
                    std::string(req.method_string()), "lora_api", std::string(target));
        return createErrorResponse(
            http::status::unauthorized,
            "Unauthorized",
            "Valid Bearer Token required. Include 'Authorization: Bearer <token>' header."
        );
    }
    THEMIS_INFO("[AUDIT] {} {} path='{}' user='authenticated' result=ALLOW",
                std::string(req.method_string()), "lora_api", std::string(target));
    
    // Route to appropriate handler based on path and method

    // Training jobs, regeneration and snapshot pins (WP D/H)
    if (path.starts_with(kJobsPrefix) && method == http::verb::get) {
        return handleGetJob(req);
    }
    if (admin && method == http::verb::post) {
        return handleRegenerateAdapter(req);
    }
    if (admin && method == http::verb::delete_) {
        return handleReleaseSnapshotPin(req);
    }
    
    // Model management endpoints
    if (target == "/api/v1/llm/models" && method == http::verb::post) {
        return handleRegisterModel(req);
    } else if (target == "/api/v1/llm/models" && method == http::verb::get) {
        return handleListModels(req);
    } else if (target.starts_with("/api/v1/llm/models/") && method == http::verb::get) {
        return handleGetModel(req);
    } else if (target.starts_with("/api/v1/llm/models/") && method == http::verb::delete_) {
        return handleDeleteModel(req);
    }
    
    // Adapter CRUD endpoints
    else if (target == "/api/v1/llm/lora/adapters" && method == http::verb::post) {
        return handleCreateAdapter(req);
    } else if (target == "/api/v1/llm/lora/adapters" && method == http::verb::get) {
        return handleListAdapters(req);
    } else if (target.starts_with("/api/v1/llm/lora/adapters/") && method == http::verb::get) {
        // Check if it's a status, provenance, audit, snapshots, or load-status request
        if (target.ends_with("/load-status")) {
            return handleHotLoadStatus(req);
        } else if (target.ends_with("/status")) {
            return handleAdapterStatus(req);
        } else if (target.ends_with("/provenance")) {
            return handleGetProvenance(req);
        } else if (target.ends_with("/audit")) {
            return handleGetAuditLog(req);
        } else if (target.ends_with("/snapshots")) {
            return handleListSnapshots(req);
        } else {
            return handleGetAdapter(req);
        }
    } else if (target.starts_with("/api/v1/llm/lora/adapters/") && method == http::verb::put) {
        return handleUpdateAdapter(req);
    } else if (target.starts_with("/api/v1/llm/lora/adapters/") && method == http::verb::delete_) {
        return handleDeleteAdapter(req);
    }
    
    // Adapter lifecycle endpoints
    else if (target.starts_with("/api/v1/llm/lora/adapters/") && target.ends_with("/load") && method == http::verb::post) {
        return handleLoadAdapter(req);
    } else if (target.starts_with("/api/v1/llm/lora/adapters/") && target.ends_with("/unload") && method == http::verb::post) {
        return handleUnloadAdapter(req);
    }

    // Provenance POST endpoints
    else if (target.starts_with("/api/v1/llm/lora/adapters/") && target.ends_with("/provenance") && method == http::verb::post) {
        return handleAttachProvenance(req);
    } else if (target.starts_with("/api/v1/llm/lora/adapters/") && target.ends_with("/verify") && method == http::verb::post) {
        return handleVerifyAuditChain(req);
    }
    
    // Inference endpoint
    else if (target == "/api/v1/llm/lora/query" && method == http::verb::post) {
        return handleLoRAQuery(req);
    }
    
    // Cross-shard sync endpoint
    else if (target == "/api/v1/lora/receive" && method == http::verb::post) {
        return handleReceiveAdapter(req);
    }
    
    // Health & monitoring endpoints
    else if (target == "/api/v1/llm/lora/stats" && method == http::verb::get) {
        return handleLoRAStats(req);
    } else if (target == "/api/v1/llm/lora/health" && method == http::verb::get) {
        return handleLoRAHealth(req);
    }
    
    return createErrorResponse(
        http::status::not_found,
        "Not Found",
        "LoRA API endpoint not found"
    );
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Model Management Endpoints ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), parseRequestBody(), createErrorResponse(), contains(), at(), value(), std::chrono::system_clock::now(), time_since_epoch().
 */

http::response<http::string_body> LoRAApiHandler::handleRegisterModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleRegisterModel");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    try {
        std::string model_id = {};
        if (body->contains("model_id")) {
            model_id = body->at("model_id").get<std::string>();
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
        }
        
        // Extract model parameters
        std::string architecture = body->value("architecture", "");
        std::string gguf_path = body->value("gguf_path", "");
        std::string description = body->value("description", "");
        
        // Store model metadata (in production, this would register with model registry)
        json response_data = {
            {"model_id", model_id},
            {"status", "registered"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"message", "Model registered successfully"}
        };
        
        if (!architecture.empty()) {
          response_data["architecture"] = architecture;
        }
        if (!description.empty()) {
          response_data["description"] = description;
        }
        
        return createJsonResponse(response_data, http::status::created);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to register model",
            e.what()
        );
    }
}

/**
 * @brief Handle Get Model.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), extractPathParameter(), target(), empty(), createErrorResponse(), std::chrono::system_clock::now(), time_since_epoch(), count().
 */
http::response<http::string_body> LoRAApiHandler::handleGetModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleGetModel");
    
    std::string model_id = extractPathParameter(req.target(), "/api/v1/llm/models/");
    
    if (model_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing model_id");
    }
    
    try {
        // In production, this would query model registry
        json response_data = {
            {"model_id", model_id},
            {"architecture", "llama"},
            {"parameter_count", 7000000000},
            {"created_at", std::chrono::system_clock::now().time_since_epoch().count()},
            {"metadata", json::object()}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::not_found,
            "Model not found",
            e.what()
        );
    }
}

/**
 * @brief Handle List Models.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), target(), find(), substr(), std::stoul(), json::array(), createJsonResponse(), createErrorResponse().
 */
http::response<http::string_body> LoRAApiHandler::handleListModels(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleListModels");
    
    try {
        // Parse query parameters
        std::string_view target = req.target();
        size_t limit = 10;
        size_t offset = 0;
        
        // Simple query parameter parsing
        size_t query_pos = target.find('?');
        if (query_pos != std::string_view::npos) {
            std::string_view query_string = target.substr(query_pos + 1);
            
            // Parse limit
            size_t limit_pos = query_string.find("limit=");
            if (limit_pos != std::string_view::npos) {
                size_t limit_end = query_string.find('&', limit_pos);
                std::string limit_str{query_string.substr(limit_pos + 6, 
                    limit_end == std::string_view::npos ? std::string_view::npos : limit_end - limit_pos - 6)};
                try {
                    limit = std::stoul(limit_str);
                } catch (...) {}
            }
            
            // Parse offset
            size_t offset_pos = query_string.find("offset=");
            if (offset_pos != std::string_view::npos) {
                size_t offset_end = query_string.find('&', offset_pos);
                std::string offset_str{query_string.substr(offset_pos + 7,
                    offset_end == std::string_view::npos ? std::string_view::npos : offset_end - offset_pos - 7)};
                try {
                    offset = std::stoul(offset_str);
                } catch (...) {}
            }
        }
        
        // In production, this would query model registry
        json models = json::array();
        
        json response_data = {
            {"models", models},
            {"total", 0},
            {"limit", limit},
            {"offset", offset}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to list models",
            e.what()
        );
    }
}

/**
 * @brief Handle Delete Model.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), extractPathParameter(), target(), empty(), createErrorResponse(), createJsonResponse(), json::object(), what().
 */
http::response<http::string_body> LoRAApiHandler::handleDeleteModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDeleteModel");
    
    std::string model_id = extractPathParameter(req.target(), "/api/v1/llm/models/");
    
    if (model_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing model_id");
    }
    
    try {
        // In production, this would delete from model registry
        
        return createJsonResponse(json::object(), http::status::no_content);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to delete model",
            e.what()
        );
    }
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Adapter CRUD Endpoints ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), parseRequestBody(), createErrorResponse(), contains(), at(), value(), empty(), createAdapter().
 */

http::response<http::string_body> LoRAApiHandler::handleCreateAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleCreateAdapter");

    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }

    try {
        const std::string adapter_id = body->value("adapter_id", std::string{});
        if (adapter_id.empty()) {
            return createErrorResponse(http::status::bad_request, "Missing 'adapter_id' field");
        }
        // The base model of a training job is fixed by server configuration
        // (llm.lora.training.*); training on another base is /regenerate.
        const auto& defaults = orchestrator_->getConfig().training;
        const std::string base_model = body->value("base_model", std::string{});
        if (!base_model.empty() && base_model != defaults.base_model) {
            return createErrorResponse(
                http::status::bad_request, "base_model mismatch",
                "Training uses the configured base model '" + defaults.base_model +
                    "'; use POST /api/v1/llm/lora/adapters/{id}/regenerate for another base model");
        }

        const json empty_obj = json::object();
        const json& td = (body->contains("training_data") && (*body)["training_data"].is_object())
                             ? (*body)["training_data"]
                             : empty_obj;
        std::string dataset_id = body->value("dataset_id", td.value("dataset_id", std::string{}));
        const json* samples = nullptr;
        if (body->contains("samples")) {
            samples = &(*body)["samples"];
        } else if (td.contains("samples")) {
            samples = &td["samples"];
        }
        if (!dataset_id.empty() && samples != nullptr) {
            return createErrorResponse(http::status::bad_request,
                                       "Specify either 'dataset_id' or 'samples', not both");
        }
        const auto hyperparams = hyperparamsFrom(*body);

        std::string job_id;
        if (!dataset_id.empty()) {
            // Snapshot-provenanced path: MVCC snapshot of lora_ds:<dataset_id>:*,
            // pinned and Merkle-fingerprinted (see training_snapshot.h).
            if (!llm::lora::LoRADatasetLoader::isValidCollectionName(dataset_id)) {
                return createErrorResponse(http::status::bad_request, "Invalid 'dataset_id'",
                                           "dataset_id must match ^[A-Za-z_][A-Za-z0-9_]{0,127}$");
            }
            json limit_j = std::uint64_t{10000};
            if (body->contains("limit")) {
                limit_j = (*body)["limit"];
            } else if (td.contains("limit")) {
                limit_j = td["limit"];
            }
            if (!limit_j.is_number_unsigned() || limit_j.get<std::uint64_t>() < 1 ||
                limit_j.get<std::uint64_t>() > llm::lora::LoRADatasetLoader::kMaxSamples) {
                return createErrorResponse(
                    http::status::bad_request, "Invalid 'limit'",
                    "limit must be an integer in [1, " +
                        std::to_string(llm::lora::LoRADatasetLoader::kMaxSamples) + "]");
            }
            job_id = orchestrator_->createAdapterFromSnapshot(
                adapter_id, dataset_id, static_cast<std::size_t>(limit_j.get<std::uint64_t>()),
                hyperparams, /*async=*/true);
        } else if (samples != nullptr) {
            // Inline samples: trained and signed, but carry no snapshot reference.
            if (!samples->is_array()) {
                return createErrorResponse(http::status::bad_request, "'samples' must be an array");
            }
            auto loaded = llm::lora::LoRADatasetLoader::fromRows(
                *samples, "inline", llm::lora::LoRADatasetLoader::kMaxSamples);
            if (!loaded.success) {
                return createErrorResponse(http::status::bad_request, "Invalid training samples",
                                           loaded.error);
            }
            job_id = orchestrator_->createAdapter(adapter_id, loaded.data, hyperparams, /*async=*/true);
        } else {
            return createErrorResponse(http::status::bad_request, "Missing training data",
                                       "Provide 'dataset_id' (MVCC snapshot) or inline 'samples'");
        }

        THEMIS_INFO("LoRAApiHandler::handleCreateAdapter submitted: adapter_id='{}' dataset='{}' job_id='{}'",
                    adapter_id, dataset_id.empty() ? std::string("inline") : dataset_id, job_id);
        return submittedJobResponse(adapter_id, job_id);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to create adapter",
            e.what()
        );
    }
}

http::response<http::string_body> LoRAApiHandler::submittedJobResponse(const std::string& adapter_id,
                                                                       const std::string& job_id) {
    const auto job = orchestrator_->getJob(job_id);
    if (!job) {
        return createErrorResponse(http::status::internal_server_error, "Job not found after submission");
    }
    json data = job->toJSON();
    data["status"] = jobStatusName(job->status);
    data["adapter_id"] = adapter_id;
    data["status_url"] = std::string(kJobsPrefix) + job_id;
    if (job->status == llm::lora::LoRAOrchestrator::JobStatus::Failed) {
        // Failed on submission: missing server components (training backend,
        // signer, snapshot reader, converter) → 503; otherwise the request or
        // the referenced data was unusable → 422.
        const bool unavailable = job->error_message.find("configured") != std::string::npos;
        data["error"] = job->error_message;
        return createJsonResponse(data, unavailable ? http::status::service_unavailable
                                                    : http::status::unprocessable_entity);
    }
    return createJsonResponse(data, http::status::accepted);
}

http::response<http::string_body> LoRAApiHandler::handleGetJob(
    const http::request<http::string_body>& req) {
    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    const std::string_view target = req.target();
    const std::string job_id = singleSegment(target.substr(0, target.find('?')), kJobsPrefix);
    if (job_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing job_id");
    }
    const auto job = orchestrator_->getJob(job_id);
    if (!job) {
        return createErrorResponse(http::status::not_found, "Job not found");
    }
    json data = job->toJSON();
    data["status"] = jobStatusName(job->status);
    return createJsonResponse(data);
}

http::response<http::string_body> LoRAApiHandler::handleRegenerateAdapter(
    const http::request<http::string_body>& req) {
    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    const std::string_view target = req.target();
    const std::string source_id =
        singleSegment(target.substr(0, target.find('?')), kAdaptersPrefix, "/regenerate");
    if (source_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing source adapter id");
    }
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    try {
        llm::lora::LoRAOrchestrator::RegenerationTarget t;
        t.adapter_id = body->value("target_adapter_id", std::string{});
        t.base_model = body->value("base_model", std::string{});
        t.base_model_gguf = body->value("base_model_gguf", std::string{});
        t.base_model_gguf_sha256 = body->value("base_model_gguf_sha256", std::string{});
        if (t.adapter_id.empty() || t.base_model.empty() || t.base_model_gguf.empty()) {
            return createErrorResponse(http::status::bad_request, "Missing fields",
                                       "target_adapter_id, base_model and base_model_gguf are required");
        }
        const std::string job_id =
            orchestrator_->regenerateAdapter(source_id, t, hyperparamsFrom(*body), /*async=*/true);
        THEMIS_INFO("LoRAApiHandler: regenerate '{}' -> '{}' on base '{}' job_id='{}'", source_id,
                    t.adapter_id, t.base_model, job_id);
        return submittedJobResponse(t.adapter_id, job_id);
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::internal_server_error, "Failed to regenerate adapter",
                                   e.what());
    }
}

http::response<http::string_body> LoRAApiHandler::handleReleaseSnapshotPin(
    const http::request<http::string_body>& req) {
    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    const std::string_view target = req.target();
    const std::string lease_id = singleSegment(target.substr(0, target.find('?')), kPinsPrefix);
    if (lease_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing lease_id");
    }
    if (!orchestrator_->releaseSnapshotPin(lease_id)) {
        return createErrorResponse(http::status::not_found, "Snapshot pin not found");
    }
    THEMIS_INFO("[AUDIT] LoRA training snapshot pin released: lease_id='{}'", lease_id);
    return createJsonResponse(json{{"lease_id", lease_id}, {"released", true}});
}

/**
 * @brief Handle Get Adapter.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), extractPathParameter(), target(), empty(), createErrorResponse(), getAdapter(), toJSON(), std::chrono::system_clock::to_time_t().
 */
http::response<http::string_body> LoRAApiHandler::handleGetAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleGetAdapter");
    
    std::string adapter_id = extractPathParameter(req.target(), "/api/v1/llm/lora/adapters/");
    
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        auto adapter_info = orchestrator.getAdapter(adapter_id);
        
        if (!adapter_info) {
            return createErrorResponse(
                http::status::not_found,
                "Adapter not found",
                "Unknown adapter_id: " + adapter_id
            );
        }
        
        json response_data = adapter_info->toJSON();
        response_data["status"] = adapter_info->is_loaded ? "ready" : "stored";
        response_data["created_at"] = std::chrono::system_clock::to_time_t(
            adapter_info->metadata.created_at
        );
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get adapter",
            e.what()
        );
    }
}

/**
 * @brief Handle Update Adapter.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), extractPathParameter(), target(), empty(), createErrorResponse(), parseRequestBody(), contains(), at().
 */
http::response<http::string_body> LoRAApiHandler::handleUpdateAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleUpdateAdapter");
    
    std::string adapter_id = extractPathParameter(req.target(), "/api/v1/llm/lora/adapters/");
    
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        // Extract additional training data
        llm::lora::TrainingData training_data;
        if (body->contains("additional_training_data")) {
            auto& td = body->at("additional_training_data");
            if (td.contains("dataset_id")) {
                training_data.dataset_name = td["dataset_id"];
            }
        }
        
        // Update adapter through orchestrator (async)
        std::string job_id = orchestrator.updateAdapter(
            adapter_id,
            training_data,
            true,  // incremental
            true   // async
        );
        
        // Get current version
        std::string current_version = orchestrator.getCurrentVersion(adapter_id);
        
        // Parse version number and increment
        std::string new_version = "v1.1";  // Simplified versioning
        
        json response_data = {
            {"adapter_id", adapter_id},
            {"version", new_version},
            {"status", "training"},
            {"job_id", job_id}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to update adapter",
            e.what()
        );
    }
}

/**
 * @brief Handle Delete Adapter.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), extractPathParameter(), target(), empty(), createErrorResponse(), find(), substr(), deleteAdapter().
 */
http::response<http::string_body> LoRAApiHandler::handleDeleteAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDeleteAdapter");
    
    std::string adapter_id = extractPathParameter(req.target(), "/api/v1/llm/lora/adapters/");
    
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        // Parse query parameters for version
        std::string_view target = req.target();
        bool delete_all = true;
        
        size_t query_pos = target.find('?');
        if (query_pos != std::string_view::npos) {
            std::string_view query_string = target.substr(query_pos + 1);
            if (query_string.find("version=") != std::string_view::npos) {
                delete_all = false;
            }
        }
        
        bool success = orchestrator.deleteAdapter(adapter_id, delete_all);
        
        if (!success) {
            return createErrorResponse(
                http::status::not_found,
                "Adapter not found",
                "Failed to delete adapter: " + adapter_id
            );
        }
        
        return createJsonResponse(json::object(), http::status::no_content);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to delete adapter",
            e.what()
        );
    }
}

/**
 * @brief Handle List Adapters.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), target(), find(), substr(), std::stoul(), listAdapters(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleListAdapters(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleListAdapters");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        // Parse query parameters
        std::string_view target = req.target();
        size_t limit = 10;
        size_t offset = 0;
        std::string base_model_filter = {};
        std::string status_filter = {};
        
        // Simple query parameter parsing
        size_t query_pos = target.find('?');
        if (query_pos != std::string_view::npos) {
            std::string_view query_string = target.substr(query_pos + 1);
            
            // Parse limit
            size_t limit_pos = query_string.find("limit=");
            if (limit_pos != std::string_view::npos) {
                size_t limit_end = query_string.find('&', limit_pos);
                std::string limit_str{query_string.substr(limit_pos + 6, 
                    limit_end == std::string_view::npos ? std::string_view::npos : limit_end - limit_pos - 6)};
                try {
                    limit = std::stoul(limit_str);
                } catch (...) {}
            }
            
            // Parse offset
            size_t offset_pos = query_string.find("offset=");
            if (offset_pos != std::string_view::npos) {
                size_t offset_end = query_string.find('&', offset_pos);
                std::string offset_str{query_string.substr(offset_pos + 7,
                    offset_end == std::string_view::npos ? std::string_view::npos : offset_end - offset_pos - 7)};
                try {
                    offset = std::stoul(offset_str);
                } catch (...) {}
            }
            
            // Parse base_model filter
            size_t base_model_pos = query_string.find("base_model=");
            if (base_model_pos != std::string_view::npos) {
                size_t base_model_end = query_string.find('&', base_model_pos);
                base_model_filter = std::string{query_string.substr(base_model_pos + 11,
                    base_model_end == std::string_view::npos ? std::string_view::npos : base_model_end - base_model_pos - 11)};
            }
            
            // Parse status filter
            size_t status_pos = query_string.find("status=");
            if (status_pos != std::string_view::npos) {
                size_t status_end = query_string.find('&', status_pos);
                status_filter = std::string{query_string.substr(status_pos + 7,
                    status_end == std::string_view::npos ? std::string_view::npos : status_end - status_pos - 7)};
            }
        }
        
        // Get adapters from orchestrator
        auto all_adapters = orchestrator.listAdapters(
            base_model_filter.empty() ? std::nullopt : std::optional<std::string>(base_model_filter)
        );
        
        // Apply status filter if specified
        std::vector<llm::lora::AdapterInfo> filtered_adapters = {};

        for (const auto& adapter : all_adapters) {
            if (status_filter.empty() ||
                (status_filter == "ready" && adapter.is_loaded) ||
                (status_filter == "stored" && !adapter.is_loaded)) {
                filtered_adapters.push_back(adapter);
            }
        }
        
        // Apply pagination
        size_t start = std::min(offset, filtered_adapters.size());
        size_t end = std::min(offset + limit, filtered_adapters.size());
        
        json adapters = json::array();
        for (size_t i = start; i < end; i++) {
            adapters.push_back(filtered_adapters[i].toJSON());
        }
        
        json response_data = {
            {"adapters", adapters},
            {"total",filtered_adapters.size()},
            {"limit", limit},
            {"offset", offset}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to list adapters",
            e.what()
        );
    }
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Adapter Lifecycle Endpoints ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), target(), starts_with(), ends_with(), createErrorResponse(), substr(), length(), empty().
 */

http::response<http::string_body> LoRAApiHandler::handleLoadAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleLoadAdapter");
    
    std::string_view target = req.target();
    std::string prefix = "/api/v1/llm/lora/adapters/";
    std::string suffix = "/load";
    
    if (!target.starts_with(prefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    
    std::string adapter_id{target.substr(prefix.length(), target.length() - prefix.length() - suffix.length())};
    
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        // Trigger hot-load asynchronously; returns a job_id immediately.
        std::string job_id = orchestrator.loadAdapter(adapter_id, /*async=*/true);
        
        json response_data = {
            {"adapter_id", adapter_id},
            {"job_id",     job_id},
            {"status",     "loading"}
        };

        THEMIS_INFO(
            "LoRAApiHandler::handleLoadAdapter accepted: adapter_id='{}' job_id='{}'",
            adapter_id,
            job_id
        );
        
        // 202 Accepted: the load is in progress; poll /load-status for completion.
        return createJsonResponse(response_data, http::status::accepted);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to initiate adapter hot-load",
            e.what()
        );
    }
}

/**
 * @brief Handle Unload Adapter.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), target(), starts_with(), ends_with(), createErrorResponse(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleUnloadAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleUnloadAdapter");
    
    std::string_view target = req.target();
    std::string prefix = "/api/v1/llm/lora/adapters/";
    std::string suffix = "/unload";
    
    if (!target.starts_with(prefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    
    std::string adapter_id{target.substr(prefix.length(), target.length() - prefix.length() - suffix.length())};
    
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        bool success = orchestrator.unloadAdapter(adapter_id, false);
        
        if (!success) {
            return createErrorResponse(
                http::status::not_found,
                "Adapter not found or not loaded",
                "Failed to unload adapter: " + adapter_id
            );
        }
        
        json response_data = {
            {"adapter_id", adapter_id},
            {"status", "unloaded"}
        };

        THEMIS_INFO("LoRAApiHandler::handleUnloadAdapter success: adapter_id='{}'", adapter_id);
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to unload adapter",
            e.what()
        );
    }
}

/**
 * @brief Handle Adapter Status.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), target(), starts_with(), ends_with(), createErrorResponse(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleAdapterStatus(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleAdapterStatus");
    
    std::string_view target = req.target();
    std::string prefix = "/api/v1/llm/lora/adapters/";
    std::string suffix = "/status";
    
    if (!target.starts_with(prefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    
    std::string adapter_id{target.substr(prefix.length(), target.length() - prefix.length() - suffix.length())};
    
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        bool is_loaded = orchestrator.isLoaded(adapter_id);

        // Calculate per-adapter memory from orchestrator.
        double memory_mb = 0.0;
        auto adapter_opt = orchestrator.getAdapter(adapter_id);
        if (adapter_opt.has_value()) {
            memory_mb = static_cast<double>(adapter_opt->memory_bytes) / (1024.0 * 1024.0);
        }

        json response_data = {
            {"adapter_id", adapter_id},
            {"is_loaded", is_loaded},
            {"memory_usage_mb", memory_mb},
            {"last_used", std::chrono::system_clock::now().time_since_epoch().count()}
        };

        // Most recent training job for this adapter (real orchestrator state).
        std::optional<llm::lora::LoRAOrchestrator::JobInfo> latest;
        for (const auto& job : orchestrator.listJobs()) {
            if (job.adapter_id == adapter_id && (!latest || job.started_at > latest->started_at)) {
                latest = job;
            }
        }
        if (latest) {
            json job_json = latest->toJSON();
            job_json["status"] = jobStatusName(latest->status);
            response_data["latest_job"] = std::move(job_json);
        }
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get adapter status",
            e.what()
        );
    }
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Hot-Load Status Endpoint ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), target(), starts_with(), ends_with(), createErrorResponse(), substr(), length(), empty().
 */

http::response<http::string_body> LoRAApiHandler::handleHotLoadStatus(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleHotLoadStatus");

    std::string_view target = req.target();
    std::string prefix = "/api/v1/llm/lora/adapters/";
    std::string suffix = "/load-status";

    if (!target.starts_with(prefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }

    std::string adapter_id{target.substr(prefix.length(),
                                          target.length() - prefix.length() - suffix.length())};

    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;

    try {
        // Find the most recent loading job for this adapter.
        auto jobs = orchestrator.listJobs();
        std::optional<llm::lora::LoRAOrchestrator::JobInfo> latest = {};

        for (const auto& job : jobs) {
            if (job.adapter_id == adapter_id &&
                job.type == llm::lora::LoRAOrchestrator::JobType::Loading) {
                if (!latest || job.started_at > latest->started_at) {
                    latest = job;
                }
            }
        }

        if (!latest) {
            return createErrorResponse(
                http::status::not_found,
                "No load job found for adapter",
                "No hot-load job has been submitted for adapter: " + adapter_id
            );
        }

        std::string status_str = {};
        switch (latest->status) {
            case llm::lora::LoRAOrchestrator::JobStatus::Pending:   status_str = "pending";   break;
            case llm::lora::LoRAOrchestrator::JobStatus::Running:   status_str = "loading";   break;
            case llm::lora::LoRAOrchestrator::JobStatus::Completed: status_str = "loaded";    break;
            case llm::lora::LoRAOrchestrator::JobStatus::Failed:    status_str = "failed";    break;
            case llm::lora::LoRAOrchestrator::JobStatus::Cancelled: status_str = "cancelled"; break;
        }

        json response_data = {
            {"adapter_id", adapter_id},
            {"job_id",     latest->job_id},
            {"status",     status_str},
            {"progress",   latest->progress}
        };
        if (!latest->error_message.empty()) {
            response_data["error"] = latest->error_message;
        }

        return createJsonResponse(response_data);

    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get hot-load status",
            e.what()
        );
    }
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Inference Endpoint ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), parseRequestBody(), createErrorResponse(), value(), empty(), std::chrono::steady_clock::now(), std::chrono::milliseconds(), submit().
 */

http::response<http::string_body> LoRAApiHandler::handleLoRAQuery(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleLoRAQuery");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    try {
        std::string model_id = body->value("model_id", "");
        std::string adapter_id = body->value("adapter_id", "");
        std::string prompt = body->value("prompt", "");
        int max_tokens = body->value("max_tokens", 500);
        double temperature = body->value("temperature", 0.7);
        std::string user_id = body->value("user_id", "");
        
        if (prompt.empty()) {
            return createErrorResponse(http::status::bad_request, "Missing 'prompt' field");
        }
        
        if (adapter_id.empty()) {
            return createErrorResponse(http::status::bad_request, "Missing 'adapter_id' field");
        }
        
        // Perform inference using InferenceEngineEnhanced when available.
        auto start_time = std::chrono::steady_clock::now();

        std::string response_text = {};
        int tokens_used = 0;

        if (inference_engine_) {
            // Build an EnhancedInferenceRequest from the LoRA query parameters.
            llm::InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;
            eng_req.base_request.prompt     = prompt;
            eng_req.base_request.model_id   = model_id.empty() ? "default" : model_id;
            eng_req.base_request.max_tokens = max_tokens;
            eng_req.base_request.temperature = static_cast<float>(temperature);
            if (!adapter_id.empty()) {
                eng_req.base_request.lora_adapter_id = adapter_id;
            }
            eng_req.priority             = 0;
            eng_req.timeout              = std::chrono::milliseconds(30000);
            eng_req.preferred_model_id   = model_id;

            try {
                auto handle   = inference_engine_->submit(eng_req);
                auto response = handle.get();  // blocking wait
                response_text = response.text;
                tokens_used   = response.tokens_generated;
            } catch (const std::exception& ex) {
                return createErrorResponse(
                    http::status::internal_server_error,
                    "Inference failed",
                    ex.what()
                );
            }
        } else {
            // Inference engine not configured — return a clear 501.
            return createErrorResponse(
                http::status::not_implemented,
                "Inference engine not configured",
                "Set up an InferenceEngineEnhanced via LoRAApiHandler::setInferenceEngine() "
                "to enable actual LLM inference."
            );
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        json response_data = {
            {"response", response_text},
            {"model_id", model_id.empty() ? "default" : model_id},
            {"adapter_id", adapter_id},
            {"tokens_used", tokens_used},
            {"inference_time_ms", duration.count()},
            {"audit_id", "audit_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}
        };

        THEMIS_INFO(
            "LoRAApiHandler::handleLoRAQuery success: model_id='{}' adapter_id='{}' prompt_len={} tokens_used={} inference_time_ms={}",
            model_id.empty() ? "default" : model_id,
            adapter_id,
            prompt.size(),
            tokens_used,
            duration.count()
        );
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Query failed",
            e.what()
        );
    }
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Health & Monitoring Endpoints ═══════════════════════════════════════════════════════════
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), getStats(), value(), createJsonResponse(), what().
 */

http::response<http::string_body> LoRAApiHandler::handleLoRAStats(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleLoRAStats");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        auto stats = orchestrator.getStats();
        
        json response_data = {
            {"total_adapters", stats.value("total_adapters", 0)},
            {"loaded_adapters", stats.value("loaded_adapters", 0)},
            {"cache_hit_rate", stats.value("cache_hit_rate", 0.0)},
            {"total_inferences", stats.value("total_inferences", 0)},
            {"avg_load_time_ms", stats.value("avg_load_time_ms", 0)},
            {"uptime_seconds", stats.value("uptime_seconds", 0)}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get statistics",
            e.what()
        );
    }
}

/**
 * @brief Handle Lo RAHealth.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), healthCheck(), createJsonResponse(), what().
 */
http::response<http::string_body> LoRAApiHandler::handleLoRAHealth(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleLoRAHealth");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        bool healthy = orchestrator.healthCheck();
        
        json response_data = {
            {"status", healthy ? "healthy" : "unhealthy"},
            {"storage", "ok"},
            {"manager", "ok"},
            {"training", "ok"},
            {"checks_passed", 3},
            {"checks_failed", 0}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::service_unavailable,
            "Health check failed",
            e.what()
        );
    }
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Helper Methods ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), AuthMiddleware::extractBearerToken(), std::string_view(), data(), size(), parseAndValidate(), THEMIS_DEBUG().
 */

bool LoRAApiHandler::validateBearerToken(const http::request<http::string_body>& req) {
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return false;
    }

    auto token = AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(),auth_header.size()));
    if (!token) {
        return false;
    }
    
    if (!jwt_validator_) {
        // JWT validator not configured - deny access
        // Note: This is intentional. In production, JWT validation should always be configured.
        return false;
    }
    auto& jwt_validator = *jwt_validator_;
    
    try {
        auto claims = jwt_validator.parseAndValidate(*token);
        // Token is valid
        return true;
    } catch (...) {
        THEMIS_DEBUG("lora_api_handler: unhandled exception caught");
        // Token validation failed (expired, invalid signature, etc.)
        return false;
    }
}

/**
 * @brief Create Error Response.
 * @param[in] status Input parameter.
 * @param[in] error Input parameter.
 * @param[in] details Input parameter.
 * @return Return value.
 * @details Calls: json::object(), std::string(), empty(), set(), body(), dump(), prepare_payload().
 */
http::response<http::string_body> LoRAApiHandler::createErrorResponse(
    http::status status,
    std::string_view error,
    std::string_view details) {
    
    json error_obj = json::object({{"error", std::string(error)}});
    
    if (!details.empty()) {
        error_obj["details"] = std::string(details);
    }
    
    error_obj["status"] = static_cast<int>(status);
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB-LoRA/1.3.0");
    res.body() = error_obj.dump();
    res.prepare_payload();
    return res;
}

/**
 * @brief Create Json Response.
 * @param[in] data Input parameter.
 * @param[in] status Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), set(), body(), dump(), prepare_payload().
 */
http::response<http::string_body> LoRAApiHandler::createJsonResponse(
    const json& data,
    http::status status) {
    auto span = Tracer::startSpan("createJsonResponse");
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB-LoRA/1.3.0");
    res.body() = data.dump();
    res.prepare_payload();
    return res;
}

/**
 * @brief Parse Request Body.
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: json::parse(), body(), is_object(), THEMIS_DEBUG().
 */
std::optional<json> LoRAApiHandler::parseRequestBody(
    const http::request<http::string_body>& req) {
    
    try {
        auto parsed = json::parse(req.body());
        if (parsed.is_object()) {
            return parsed;
        }
    } catch (...) {
        THEMIS_DEBUG("lora_api_handler: unhandled exception caught");
        return std::nullopt;
    }
    
    return std::nullopt;
}

/**
 * @brief Extract Path Parameter.
 * @param[in] target Input parameter.
 * @param[in] prefix Input parameter.
 * @return Return value.
 * @details Calls: starts_with(), substr(), length(), find().
 */
std::string LoRAApiHandler::extractPathParameter(
    std::string_view target,
    std::string_view prefix) {
    
    if (!target.starts_with(prefix)) {
        return "";
    }
    
    std::string param{target.substr(prefix.length())};
    
    // Remove query string if present
    size_t query_pos = param.find('?');
    if (query_pos != std::string::npos) {
        param = param.substr(0, query_pos);
    }
    
    return param;
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Cross-Shard Sync Endpoint ═══════════════════════════════════════════════════════════
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), parseRequestBody(), createErrorResponse(), contains(), at(), is_string(), utils::Cursor::base64Decode(), has_value().
 */

http::response<http::string_body> LoRAApiHandler::handleReceiveAdapter(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleReceiveAdapter");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;
    
    try {
        // Extract metadata
        if (!body->contains("metadata")) {
            return createErrorResponse(http::status::bad_request, "Missing 'metadata' field");
        }
        
        if (!body->contains("data")) {
            return createErrorResponse(http::status::bad_request, "Missing 'data' field");
        }
        
        auto& metadata_json = body->at("metadata");
        
        // Extract adapter ID
        std::string adapter_id = metadata_json.at("adapter_id").get<std::string>();
        std::string version = metadata_json.at("version").get<std::string>();
        std::string base_model = metadata_json.at("base_model").get<std::string>();
        
        // Extract and decode base64-encoded data using Cursor::base64Decode.
        std::string data_str = {};
        if (body->at("data").is_string()) {
            std::string data_base64 = body->at("data").get<std::string>();
            auto decoded = utils::Cursor::base64Decode(data_base64);
            if (!decoded.has_value()) {
                return createErrorResponse(http::status::bad_request,
                                           "Invalid base64-encoded data");
            }
            data_str = std::move(*decoded);
        } else {
            return createErrorResponse(http::status::bad_request, "Data must be base64-encoded string");
        }
        
        // Check if data is compressed
        bool compressed = false;
        std::string compression = body->value("compression", "none");
        
        std::vector<uint8_t> weights_data = {};

        if (compression == "zstd") {
            // Decompress data
            auto decompressed = utils::zstd_decompress(
                std::vector<uint8_t>(data_str.begin(), data_str.end())
            );
            if (decompressed.empty()) {
                return createErrorResponse(
                    http::status::bad_request,
                    "Failed to decompress data"
                );
            }
            weights_data = decompressed;
            compressed = true;
        } else {
            weights_data = std::vector<uint8_t>(data_str.begin(), data_str.end());
        }
        
        // Get consistency checker once for all validations
        auto consistency_checker = orchestrator.getConsistencyChecker();
        
        // Verify integrity checks if present
        if (body->contains("checksum")) {
            std::string expected_checksum = body->at("checksum").get<std::string>();
            
            // Calculate checksum and verify
            if (consistency_checker) {
                std::string actual_checksum = consistency_checker->calculateChecksum(weights_data);
                if (actual_checksum != expected_checksum) {
                    return createErrorResponse(
                        http::status::bad_request,
                        "Checksum verification failed",
                        "Data integrity check failed"
                    );
                }
            }
        }
        
        if (body->contains("signature")) {
            std::string signature = body->at("signature").get<std::string>();
            
            // Verify signature
            if (consistency_checker) {
                if (!consistency_checker->verifySignature(weights_data, signature)) {
                    return createErrorResponse(
                        http::status::bad_request,
                        "Signature verification failed",
                        "Data authenticity check failed"
                    );
                }
            }
        }
        
        // Build AdapterWeights structure
        llm::lora::AdapterWeights weights;
        weights.data = weights_data;
        weights.size_bytes = weights_data.size();
        weights.format = metadata_json.value("format", "safetensors");
        
        if (metadata_json.contains("hyperparameters")) {
            weights.hyperparameters = llm::lora::LoRAHyperparameters::fromJSON(
                metadata_json["hyperparameters"]
            );
        }
        
        // Build AdapterMetadata structure
        llm::lora::AdapterMetadata metadata;
        metadata.adapter_id = adapter_id;
        metadata.version = version;
        metadata.base_model = base_model;
        metadata.description = metadata_json.value("description", "");
        metadata.training_samples = metadata_json.value("training_samples", 0);
        metadata.validation_accuracy = metadata_json.value("validation_accuracy", 0.0f);
        
        if (metadata_json.contains("checksum")) {
            metadata.checksum = metadata_json["checksum"].get<std::string>();
        }
        if (metadata_json.contains("signature")) {
            metadata.signature = metadata_json["signature"].get<std::string>();
        }
        
        // Parse timestamps
        if (metadata_json.contains("created_at")) {
            auto created_ns = metadata_json["created_at"].get<uint64_t>();
            metadata.created_at = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                    std::chrono::nanoseconds(created_ns)
                )
            );
        }
        if (metadata_json.contains("updated_at")) {
            auto updated_ns = metadata_json["updated_at"].get<uint64_t>();
            metadata.updated_at = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                    std::chrono::nanoseconds(updated_ns)
                )
            );
        }
        
        if (metadata_json.contains("custom_metadata")) {
            metadata.custom_metadata = metadata_json["custom_metadata"];
        }
        
        // Store the adapter via storage service
        auto storage_service = orchestrator.getStorageService();
        if (!storage_service) {
            return createErrorResponse(
                http::status::internal_server_error,
                "Storage service not available"
            );
        }
        
        bool stored = storage_service->saveAdapter(adapter_id, weights, metadata);
        
        if (stored) {
            json response_data = {
                {"adapter_id", adapter_id},
                {"version", version},
                {"status", "received"},
                {"bytes_received",weights_data.size()},
                {"compressed", compressed},
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
            };
            
            return createJsonResponse(response_data, http::status::created);
        } else {
            return createErrorResponse(
                http::status::internal_server_error,
                "Failed to store adapter"
            );
        }
        
    } catch (const json::exception& e) {
        return createErrorResponse(
            http::status::bad_request,
            "JSON parsing error",
            e.what()
        );
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            e.what()
        );
    }
}

// ============================================================================
// Provenance, Snapshots, and Audit Log endpoints
// ============================================================================

/**
 * @brief GET /api/v1/llm/lora/adapters/{id}/provenance
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), target(), starts_with(), ends_with(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleGetProvenance(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleGetProvenance");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;

    std::string_view target = req.target();
    constexpr std::string_view suffix = "/provenance";
    if (!target.starts_with(kAdaptersPrefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    const std::string adapter_id{target.substr(kAdaptersPrefix.length(),
        target.length() - (kAdaptersPrefix.length() + suffix.length()))};
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    auto prov_opt = orchestrator.getProvenanceRecord(adapter_id);
    if (!prov_opt) {
        return createErrorResponse(http::status::not_found,
                                   "Provenance record not found for adapter: " + adapter_id);
    }

    return createJsonResponse(prov_opt->toJSON());
}

/**
 * @brief POST /api/v1/llm/lora/adapters/{id}/provenance
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), target(), starts_with(), ends_with(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleAttachProvenance(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleAttachProvenance");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;

    std::string_view target = req.target();
    constexpr std::string_view suffix = "/provenance";
    if (!target.starts_with(kAdaptersPrefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    const std::string adapter_id{target.substr(kAdaptersPrefix.length(),
        target.length() - (kAdaptersPrefix.length() + suffix.length()))};
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    auto body_opt = parseRequestBody(req);
    if (!body_opt) {
        return createErrorResponse(http::status::bad_request, "Invalid or missing JSON body");
    }

    try {
        const auto record = llm::lora::LoRAProvenanceRecord::fromJSON(*body_opt);
        const bool ok = orchestrator.attachProvenance(adapter_id, record);
        if (!ok) {
            return createErrorResponse(http::status::not_found,
                                       "Adapter not found: " + adapter_id);
        }
        return createJsonResponse(json{{"adapter_id", adapter_id}, {"status", "provenance_attached"}},
                                  http::status::created);
    } catch (const json::exception& e) {
        return createErrorResponse(http::status::bad_request, "JSON parsing error", e.what());
    }
}

/**
 * @brief GET /api/v1/llm/lora/adapters/{id}/audit
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), target(), starts_with(), ends_with(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleGetAuditLog(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleGetAuditLog");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;

    std::string_view target = req.target();
    constexpr std::string_view suffix = "/audit";
    if (!target.starts_with(kAdaptersPrefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    const std::string adapter_id{target.substr(kAdaptersPrefix.length(),
        target.length() - (kAdaptersPrefix.length() + suffix.length()))};
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    const auto entries = orchestrator.getInferenceAuditLog(adapter_id);
    json arr = json::array();
    for (const auto& e : entries) {
        arr.push_back(e.toJSON());
    }
    return createJsonResponse(json{
        {"adapter_id", adapter_id},
        {"count",entries.size()},
        {"entries",    arr}
    });
}

/**
 * @brief GET /api/v1/llm/lora/adapters/{id}/snapshots
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), target(), starts_with(), ends_with(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleListSnapshots(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleListSnapshots");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;

    std::string_view target = req.target();
    constexpr std::string_view suffix = "/snapshots";
    if (!target.starts_with(kAdaptersPrefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    const std::string adapter_id{target.substr(kAdaptersPrefix.length(),
        target.length() - (kAdaptersPrefix.length() + suffix.length()))};
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    const auto snaps = orchestrator.listAdapterSnapshots(adapter_id);
    json arr = json::array();
    for (const auto& s : snaps) {
        arr.push_back(s.toJSON());
    }
    return createJsonResponse(json{
        {"adapter_id", adapter_id},
        {"count",snaps.size()},
        {"snapshots",  arr}
    });
}

/**
 * @brief POST /api/v1/llm/lora/adapters/{id}/verify
 * @param[in] req Input parameter.
 * @return Return value.
 * @details Calls: Tracer::startSpan(), createErrorResponse(), target(), starts_with(), ends_with(), substr(), length(), empty().
 */
http::response<http::string_body> LoRAApiHandler::handleVerifyAuditChain(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleVerifyAuditChain");

    if (!orchestrator_) {
        return createErrorResponse(http::status::service_unavailable, "Orchestrator not available");
    }
    auto& orchestrator = *orchestrator_;

    std::string_view target = req.target();
    constexpr std::string_view suffix = "/verify";
    if (!target.starts_with(kAdaptersPrefix) || !target.ends_with(suffix)) {
        return createErrorResponse(http::status::bad_request, "Invalid endpoint");
    }
    const std::string adapter_id{target.substr(kAdaptersPrefix.length(),
        target.length() - (kAdaptersPrefix.length() + suffix.length()))};
    if (adapter_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing adapter_id");
    }

    const bool intact = orchestrator.verifyAuditChain(adapter_id);
    const http::status status = intact ? http::status::ok : http::status::conflict;

    return createJsonResponse(json{
        {"adapter_id",  adapter_id},
        {"chain_valid", intact},
        {"message",     intact ? "Merkle audit chain is intact"
                                : "Merkle audit chain verification FAILED — possible tampering"}
    }, status);
}

} // namespace themis::server


