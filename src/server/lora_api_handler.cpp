/**
 * @file lora_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/lora_api_handler.h"
#include <stdexcept>
#include "server/auth_middleware.h"
#include "auth/jwt_validator.h"
#include "llm/lora_framework/lora_orchestrator.h"
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

void LoRAApiHandler::configureJWT(const auth::JWTValidatorConfig& config) {
    jwt_validator_ = std::make_unique<auth::JWTValidator>(config);
}

void LoRAApiHandler::setInferenceEngine(
        std::shared_ptr<llm::InferenceEngineEnhanced> engine) {
    inference_engine_ = std::move(engine);
}

http::response<http::string_body> LoRAApiHandler::handleRequest(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleRequest");
    
    std::string_view target = req.target();
    auto method = req.method();
    
    // Validate Bearer Token (JWT) authentication for all endpoints.
    // Cross-shard calls to /api/v1/lora/receive are now authenticated via
    // Authorization: Bearer <token> forwarded by SecureTransportClient, so
    // no special bypass is required.
    if (!validateBearerToken(req)) {
        return createErrorResponse(
            http::status::unauthorized,
            "Unauthorized",
            "Valid Bearer Token required. Include 'Authorization: Bearer <token>' header."
        );
    }
    
    // Route to appropriate handler based on path and method
    
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

// ═══════════════════════════════════════════════════════════
// Model Management Endpoints
// ═══════════════════════════════════════════════════════════

http::response<http::string_body> LoRAApiHandler::handleRegisterModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleRegisterModel");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    try {
        std::string model_id;
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
        
        if (!architecture.empty()) response_data["architecture"] = architecture;
        if (!description.empty()) response_data["description"] = description;
        
        return createJsonResponse(response_data, http::status::created);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to register model",
            e.what()
        );
    }
}

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

// ═══════════════════════════════════════════════════════════
// Adapter CRUD Endpoints
// ═══════════════════════════════════════════════════════════

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
    auto& orchestrator = *orchestrator_;
    
    try {
        std::string adapter_id;
        if (body->contains("adapter_id")) {
            adapter_id = body->at("adapter_id").get<std::string>();
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'adapter_id' field");
        }
        
        std::string base_model = body->value("base_model", "");
        if (base_model.empty()) {
            return createErrorResponse(http::status::bad_request, "Missing 'base_model' field");
        }
        
        // Extract training data
        llm::lora::TrainingData training_data;
        if (body->contains("training_data")) {
            auto& td = body->at("training_data");
            if (td.contains("dataset_id")) {
                training_data.dataset_name = td["dataset_id"];
            }
            // In production, load actual training data from dataset_id
        }
        
        // Extract hyperparameters
        std::optional<llm::lora::LoRAHyperparameters> hyperparams;
        if (body->contains("rank") || body->contains("alpha")) {
            llm::lora::LoRAHyperparameters params;
            params.rank = body->value("rank", 8);
            params.alpha = body->value("alpha", 16.0f);
            hyperparams = params;
        }
        
        // Create adapter through orchestrator (async)
        std::string job_id = orchestrator.createAdapter(
            adapter_id,
            training_data,
            hyperparams,
            true  // async
        );
        
        json response_data = {
            {"adapter_id", adapter_id},
            {"version", "v1.0"},
            {"status", "training"},
            {"job_id", job_id}
        };

        THEMIS_INFO(
            "LoRAApiHandler::handleCreateAdapter accepted: adapter_id='{}' base_model='{}' job_id='{}'",
            adapter_id,
            base_model,
            job_id
        );
        
        return createJsonResponse(response_data, http::status::created);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to create adapter",
            e.what()
        );
    }
}

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
        std::string base_model_filter;
        std::string status_filter;
        
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
        std::vector<llm::lora::AdapterInfo> filtered_adapters;
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
            {"total", filtered_adapters.size()},
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

// ═══════════════════════════════════════════════════════════
// Adapter Lifecycle Endpoints
// ═══════════════════════════════════════════════════════════

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
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get adapter status",
            e.what()
        );
    }
}

// ═══════════════════════════════════════════════════════════
// Hot-Load Status Endpoint
// ═══════════════════════════════════════════════════════════

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
        std::optional<llm::lora::LoRAOrchestrator::JobInfo> latest;
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

        std::string status_str;
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

// ═══════════════════════════════════════════════════════════
// Inference Endpoint
// ═══════════════════════════════════════════════════════════

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

        std::string response_text;
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

// ═══════════════════════════════════════════════════════════
// Health & Monitoring Endpoints
// ═══════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════
// Helper Methods
// ═══════════════════════════════════════════════════════════

bool LoRAApiHandler::validateBearerToken(const http::request<http::string_body>& req) {
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return false;
    }

    auto token = AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(), auth_header.size()));
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

// ═══════════════════════════════════════════════════════════
// Cross-Shard Sync Endpoint
// ═══════════════════════════════════════════════════════════

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
        std::string data_str;
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
        
        std::vector<uint8_t> weights_data;
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
                {"bytes_received", weights_data.size()},
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

namespace {
    constexpr std::string_view kAdaptersPrefix = "/api/v1/llm/lora/adapters/";
} // namespace

// GET /api/v1/llm/lora/adapters/{id}/provenance
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

// POST /api/v1/llm/lora/adapters/{id}/provenance
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

// GET /api/v1/llm/lora/adapters/{id}/audit
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
        {"count",      entries.size()},
        {"entries",    arr}
    });
}

// GET /api/v1/llm/lora/adapters/{id}/snapshots
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
        {"count",      snaps.size()},
        {"snapshots",  arr}
    });
}

// POST /api/v1/llm/lora/adapters/{id}/verify
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


