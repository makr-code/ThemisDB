/**
 * @file llm_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=2, C=13, H=62, M=46, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/llm_api_handler.h"
#include <stdexcept>
#include "server/lora_api_handler.h"
#include "auth/jwt_validator.h"
#include "governance/policy_engine.h"
#include "llm/llm_plugin_manager.h"
#include "llm/llm_plugin_interface.h"
#include "llm/async_inference_engine.h"
#include "llm/embedded_llm.h"
#include "llm/docs_assistant.h"
#include "llm/feedback_store.h"
#include "llm/openai_compat_adapter.h"
#include "aql/llm_aql_handler.h"
#include "aql/llm_error_codes.h"
#include "llm/context_window_budget.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <iostream>
#include <chrono>
#include <exception>
#include <limits>
#include "utils/tracing.h"
#include "server/model_integrity_verifier.h"
#include <filesystem>
#include <cstdlib>

namespace themis::server {

namespace {
    template <typename T>
    T json_value_to(const json& value) {
        return value.get<T>();
    }

    // Helper to extract JWT token from Authorization header
    std::optional<std::string> extractBearerToken(const http::request<http::string_body>& req) {
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            return std::nullopt;
        }
        
        std::string auth_str{auth_header.data(),static_cast<int>(auth_header.size())};
        std::regex bearer_regex(R"(^Bearer\s+(.+)$)", std::regex::icase);
        std::smatch matches = {};
        
        if (std::regex_match(auth_str, matches, bearer_regex) && static_cast<int>(matches.size()) == 2) {
            return matches[1].str();
        }
        
        return std::nullopt;
    }

    void logCurrentException(const char* context) {
        try {
            auto ex = std::current_exception();
            if (ex) {
                std::rethrow_exception(ex);
            }
            THEMIS_ERROR("{}: unknown exception", context);
        } catch (const std::exception& e) {
            THEMIS_ERROR("{}: {}", context, e.what());
        }
    }

    std::optional<std::string> extractRagDocumentContent(const nlohmann::json& entity) {
        // Check "content" field first
        if (entity.contains("content") && entity["content"].is_string()) {
            const std::string& content = entity["content"].get_ref<const std::string&>();
            if (!content.empty()) {
                return content;
            }
        }
        // Check "text" field
        if (entity.contains("text") && entity["text"].is_string()) {
            const std::string& content = entity["text"].get_ref<const std::string&>();
            if (!content.empty()) {
                return content;
            }
        }
        // Check "body" field
        if (entity.contains("body") && entity["body"].is_string()) {
            const std::string& content = entity["body"].get_ref<const std::string&>();
            if (!content.empty()) {
                return content;
            }
        }
        return std::nullopt;
    }

    std::string extractRagDocumentSource(const std::string& primary_key, const nlohmann::json& entity) {
        if (!primary_key.empty()) {
            return primary_key;
        }
        if (entity.contains("source") && entity["source"].is_string()) {
            const std::string& source = entity["source"].get_ref<const std::string&>();
            if (!source.empty()) {
                return source;
            }
        }
        if (entity.contains("id") && entity["id"].is_string()) {
            const std::string& source = entity["id"].get_ref<const std::string&>();
            if (!source.empty()) {
                return source;
            }
        }
        return {};
    }
}

LLMApiHandler::LLMApiHandler(
    std::shared_ptr<llm::LLMPluginManager> plugin_manager,
    std::optional<auth::JWTValidatorConfig> jwt_config)
    : plugin_manager_(std::move(plugin_manager)) {
    
    if (jwt_config) {
        jwt_validator_ = std::make_unique<auth::JWTValidator>(*jwt_config);
    }
}

void LLMApiHandler::configureJWT([[maybe_unused]] const auth::JWTValidatorConfig& config) {
    jwt_validator_ = std::make_unique<auth::JWTValidator>(config);
}

void LLMApiHandler::setLoRAHandler([[maybe_unused]] std::shared_ptr<LoRAApiHandler> lora_handler) {
    lora_handler_ = std::move([[maybe_unused]] lora_handler);
}

void LLMApiHandler::setFeedbackStore([[maybe_unused]] std::shared_ptr<llm::FeedbackStore> feedback_store) {
    feedback_store_ = std::move(feedback_store);
}

void LLMApiHandler::setPolicyEngine([[maybe_unused]] governance::PolicyEngine* policy_engine) {
    policy_engine_ = policy_engine;
}

void LLMApiHandler::setQueryEngine([[maybe_unused]] std::shared_ptr<query::QueryEngine> query_engine) {
    query_engine_ = std::move(query_engine);
}

http::response<http::string_body> LLMApiHandler::handleRequest(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan([[maybe_unused]] "handleRequest");

    try {
        // HIGH-GAP FIX: hardcoded_path — use centralized path constants
        // API route constants for maintainability and runtime configurability
        static constexpr std::string_view kLoraPrefix = "/api/v1/llm/lora/";
        static constexpr std::string_view kOpenAIChatCompletions = "/v1/chat/completions";
        static constexpr std::string_view kOpenAIModels = "/v1/models";
        
        // Delegate to LoRAApiHandler for LoRA-specific paths
        std::string_view target = req.target();
        if ([[maybe_unused]] lora_handler_ && target.starts_with(kLoraPrefix)) {
            return lora_handler_->handleRequest([[maybe_unused]] req);
        }

        // OpenAI-compatible endpoints use API key auth via PolicyEngine, not JWT.
        // Route them BEFORE the JWT gate so that OpenAI SDK clients (which send a
        // plain API key, not a signed JWT) are not rejected by validateBearerToken().
        auto method = req.method();
        if (target == kOpenAIChatCompletions && method == http::verb::post) {
            return handleOpenAIChatCompletions(req);
        } else if (target == kOpenAIModels && method == http::verb::get) {
            return handleOpenAIListModels(req);
        }

        const bool known_llm_route =
            ((target == "/api/v1/llm/inference" && method == http::verb::post) ||
            (target == "/api/v1/llm/rag" && method == http::verb::post) ||
            (target == "/api/v1/llm/embed" && method == http::verb::post) ||
            (target == "/api/v1/llm/stream" && method == http::verb::get) ||
            (target == "/api/v1/llm/models" && method == http::verb::get) ||
            (target == "/api/v1/llm/models/load" && method == http::verb::post) ||
            (target == "/api/v1/llm/models/unload" && method == http::verb::post) ||
            (target.starts_with("/api/v1/llm/models/") && method == http::verb::get) ||
            (target == "/api/v1/llm/models/ingest" && method == http::verb::post) ||
            (target == "/api/v1/llm/loras" && method == http::verb::get) ||
            (target == "/api/v1/llm/loras/load" && method == http::verb::post) ||
            (target == "/api/v1/llm/loras/unload" && method == http::verb::post) ||
            (target == "/api/v1/llm/stats" && method == http::verb::get) ||
            (target == "/api/v1/llm/cache/stats" && method == http::verb::get) ||
            (target == "/api/v1/llm/cache" && method == http::verb::delete_) ||
            (target == "/api/v1/llm/health" && method == http::verb::get) ||
            (target == "/api/v1/llm/docs/query" && method == http::verb::post) ||
            (target == "/api/v1/llm/docs/config" && method == http::verb::post) ||
            (target == "/api/v1/llm/docs/troubleshoot" && method == http::verb::post) ||
            (target == "/api/v1/llm/feedback" && method == http::verb::post) ||
            (target == "/api/v1/llm/feedback" && method == http::verb::get) ||
            (target == "/api/v1/llm/feedback/stats" && method == http::verb::get) ||
            (target.starts_with("/api/v1/llm/feedback/") && method == http::verb::get) ||
            (target == "/api/v1/llm/aql/explain/stream" && method == http::verb::post));

        if (!known_llm_route) {
            return createErrorResponse(
                http::status::not_found,
                "Not Found",
                "LLM API endpoint not found"
            );
        }

        // Validate Bearer Token (JWT) authentication for all other LLM API endpoints
        if (!validateBearerToken(req)) {
            return createErrorResponse(
                http::status::unauthorized,
                "Unauthorized",
                "Valid Bearer Token required. Include 'Authorization: Bearer <token>' header."
            );
        }

        // Route to appropriate handler based on path and method
        if (target == "/api/v1/llm/inference" && method == http::verb::post) {
            return handleInference(req);
        } else if (target == "/api/v1/llm/rag" && method == http::verb::post) {
            return handleRAG([[maybe_unused]] req);
        } else if (target == "/api/v1/llm/embed" && method == http::verb::post) {
            return handleEmbed(req);
        } else if (target == "/api/v1/llm/stream" && method == http::verb::get) {
            return handleStreamInference(req);
        } else if (target == "/api/v1/llm/models" && method == http::verb::get) {
            return handleListModels(req);
        } else if (target == "/api/v1/llm/models/load" && method == http::verb::post) {
            return handleLoadModel(req);
        } else if (target == "/api/v1/llm/models/unload" && method == http::verb::post) {
            return handleUnloadModel(req);
        } else if (target.starts_with("/api/v1/llm/models/") && method == http::verb::get) {
            return handleModelInfo(req);
        } else if (target == "/api/v1/llm/models/ingest" && method == http::verb::post) {
            return handleIngestModel(req);
        } else if (target == "/api/v1/llm/loras" && method == http::verb::get) {
            return handleListLoRAs(req);
        } else if (target == "/api/v1/llm/loras/load" && method == http::verb::post) {
            return handleLoadLoRA(req);
        } else if (target == "/api/v1/llm/loras/unload" && method == http::verb::post) {
            return handleUnloadLoRA(req);
        } else if (target == "/api/v1/llm/stats" && method == http::verb::get) {
            return handleStats(req);
        } else if (target == "/api/v1/llm/cache/stats" && method == http::verb::get) {
            return handleCacheStats(req);
        } else if (target == "/api/v1/llm/cache" && method == http::verb::delete_) {
            return handleClearCache(req);
        } else if (target == "/api/v1/llm/health" && method == http::verb::get) {
            return handleHealth(req);
        } else if (target == "/api/v1/llm/docs/query" && method == http::verb::post) {
            return handleDocsQuery(req);
        } else if (target == "/api/v1/llm/docs/config" && method == http::verb::post) {
            return handleDocsConfig(req);
        } else if (target == "/api/v1/llm/docs/troubleshoot" && method == http::verb::post) {
            return handleDocsTroubleshoot(req);
        } else if (target == "/api/v1/llm/feedback" && method == http::verb::post) {
            return handleCreateFeedback(req);
        } else if (target == "/api/v1/llm/feedback" && method == http::verb::get) {
            return handleListFeedback(req);
        } else if (target == "/api/v1/llm/feedback/stats" && method == http::verb::get) {
            return handleFeedbackStats(req);
        } else if (target.starts_with("/api/v1/llm/feedback/") && method == http::verb::get) {
            return handleGetFeedback(req);
        } else if (target == "/api/v1/llm/aql/explain/stream" && method == http::verb::post) {
            return handleStreamExplainAql(req);
        }

        return createErrorResponse(
            http::status::not_found,
            "Not Found",
            "LLM API endpoint not found"
        );
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLMApiHandler::handleRequest: {}", e.what());
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleRequest failed");
        return createErrorResponse(
            http::status::internal_server_error,
            "Internal Server Error",
            "Failed to handle LLM API request"
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleInference(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleInference");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    // Extract request parameters
    std::string prompt = {};
    std::string model_id = {};
    std::string lora_id = {};
    int max_tokens = 512;
    double temperature = 0.7;
    
    try {
        if (body->contains("prompt")) {
            prompt = json_value_to<std::string>(body->at("prompt"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'prompt' field");
        }
        
        if (body->contains("model")) {
            model_id = json_value_to<std::string>(body->at("model"));
        }
        
        if (body->contains("lora_adapter")) {
            lora_id = json_value_to<std::string>(body->at("lora_adapter"));
        }
        
        if (body->contains("max_tokens")) {
            max_tokens = json_value_to<int>(body->at("max_tokens"));
        }
        
        if (body->contains("temperature")) {
            temperature = json_value_to<double>(body->at("temperature"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid request parameters", e.what());
    }
    
    // ── B2-INPUT-VALIDATION (2026-08-26 Wave-7 Security Hardening) ─────────
    // Validates all user-supplied fields before they reach the inference engine.
    {
        static constexpr std::size_t kMaxPromptBytes = 1 * 1024 * 1024; // 1 MB
        if (static_cast<int>(prompt.size()) > kMaxPromptBytes) {
            THEMIS_WARN("[SEC] Input validation failed: field=prompt reason=too_large size={}",static_cast<int>(prompt.size()));
            return createErrorResponse(http::status::bad_request,
                "prompt too large",
                "prompt must be <= 1 MB");
        }
        // lora_id: alphanumeric, hyphens, underscores only
        if (!lora_id.empty()) {
            static const std::regex kLoraIdRe{"^[a-zA-Z0-9_-]+$"};
            if (!std::regex_match(lora_id, kLoraIdRe)) {
                THEMIS_WARN("[SEC] Input validation failed: field=lora_id reason=invalid_chars value='{}'", lora_id);
                return createErrorResponse(http::status::bad_request,
                    "lora_id contains invalid characters",
                    "lora_id must match [a-zA-Z0-9_-]+");
            }
        }
        // max_tokens: 1–32768
        if (max_tokens < 1 || max_tokens > 32768) {
            THEMIS_WARN("[SEC] Input validation failed: field=max_tokens reason=out_of_range value={}", max_tokens);
            return createErrorResponse(http::status::bad_request,
                "max_tokens out of range",
                "max_tokens must be between 1 and 32768");
        }
        // temperature: 0.0–2.0
        if (temperature < 0.0 || temperature > 2.0) {
            THEMIS_WARN("[SEC] Input validation failed: field=temperature reason=out_of_range value={}", temperature);
            return createErrorResponse(http::status::bad_request,
                "temperature out of range",
                "temperature must be between 0.0 and 2.0");
        }
    }
    // ── end input validation ────────────────────────────────────────────────
    
    // Use the plugin manager path (same as RAG) for consistent runtime behavior.
    try {
        llm::InferenceRequest llm_request;
        llm_request.prompt = prompt;
        llm_request.model_id = model_id.empty() ? std::string("default") : model_id;
        llm_request.max_tokens = max_tokens;
        llm_request.temperature = static_cast<float>(temperature);
        if (!lora_id.empty()) {
            llm_request.lora_adapter_id = lora_id;
        }

        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto llm_response = plugin_mgr.generate(llm_request);

        // Create response with backward-compatible base fields plus quality-oriented KPIs.
        const std::string resolved_model =
            llm_response.model_id.empty() ? (model_id.empty() ? "default" : model_id) : llm_response.model_id;
        const auto prompt_length = static_cast<int>(prompt.length());
        const auto generated_length = static_cast<int>(llm_response.text.length());
        const auto tokens_generated = llm_response.tokens_generated;
        const auto inference_time_ms = llm_response.inference_time_ms;
        const double safe_inference_time_ms = inference_time_ms > 0.0 ? inference_time_ms : 1.0;
        const int safe_tokens_generated = tokens_generated > 0 ? tokens_generated : 1;
        const double tokens_per_second =
            static_cast<double>(tokens_generated) * 1000.0 / safe_inference_time_ms;
        const double ms_per_token = static_cast<double>(inference_time_ms) / static_cast<double>(safe_tokens_generated);
        const bool hit_max_tokens_limit = max_tokens > 0 && tokens_generated >= max_tokens;
        const bool non_empty_text = !llm_response.text.empty();

        json response_body = {
            {"text", llm_response.text},
            {"model", resolved_model},
            {"prompt_length", prompt_length},
            {"generated_length", generated_length},
            {"tokens_generated", tokens_generated},
            {"inference_time_ms", inference_time_ms},
            {"max_tokens_requested", max_tokens},
            {"hit_max_tokens_limit", hit_max_tokens_limit},
            {"non_empty_text", non_empty_text},
            {"tokens_per_second", tokens_per_second},
            {"ms_per_token", ms_per_token}
        };

        THEMIS_INFO(
            "LLMApiHandler::handleInference success: model='{}' prompt_len={} tokens_generated={} inference_time_ms={:.2f} lora='{}'",
            resolved_model,
            prompt_length,
            tokens_generated,
            inference_time_ms,
            lora_id.empty() ? "<none>" : lora_id
        );
        
        return createJsonResponse(http::status::ok, response_body);
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLMApiHandler::handleInference: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            "Inference failed",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleRAG(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan([[maybe_unused]] "handleRAG");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    // Extract RAG parameters
    std::string query;
    std::string collection;
    int top_k = 5;
    std::string rag_mode = "text";
    std::string lora_id = {};
    int max_context_tokens = 0;
    int response_budget_tokens = 512;
    int max_tokens = 512;
    double temperature = 0.7;
    std::string model_id = {};
    
    try {
        if (body->contains("query")) {
            query = json_value_to<std::string>(body->at("query"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'query' field");
        }
        
        if (body->contains("collection")) {
            collection = json_value_to<std::string>(body->at("collection"));
        }
        
        if (body->contains("top_k")) {
            top_k = json_value_to<int>(body->at("top_k"));
        }

        if (body->contains("max_context_tokens")) {
            max_context_tokens = json_value_to<int>(body->at("max_context_tokens"));
        }

        if (body->contains("response_budget_tokens")) {
            response_budget_tokens = json_value_to<int>(body->at("response_budget_tokens"));
        }

        if (body->contains("max_tokens")) {
            max_tokens = json_value_to<int>(body->at("max_tokens"));
        }

        if (body->contains("temperature")) {
            temperature = json_value_to<double>(body->at("temperature"));
        }

        if (body->contains("model")) {
            model_id = json_value_to<std::string>(body->at("model"));
        }

        if (body->contains("rag_mode")) {
            rag_mode = json_value_to<std::string>(body->at("rag_mode"));
        }
        
        if (body->contains("lora_adapter")) {
            lora_id = json_value_to<std::string>(body->at("lora_adapter"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid RAG parameters", e.what());
    }

    if (query.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing 'query' field");
    }

    if (top_k < aql::ValidationLimits::MIN_RAG_TOP_K ||
        top_k > aql::ValidationLimits::MAX_RAG_TOP_K) {
        return createErrorResponse(
            http::status::bad_request,
            "top_k out of range",
            "top_k must be between " +
                std::to_string(aql::ValidationLimits::MIN_RAG_TOP_K) +
                " and " +
                std::to_string(aql::ValidationLimits::MAX_RAG_TOP_K)
        );
    }

    if (max_context_tokens < 0) {
        return createErrorResponse(http::status::bad_request, "max_context_tokens must be >= 0");
    }

    if (response_budget_tokens <= 0) {
        return createErrorResponse(http::status::bad_request, "response_budget_tokens must be greater than 0");
    }

    if (max_tokens <= 0) {
        return createErrorResponse(http::status::bad_request, "max_tokens must be greater than 0");
    }

    // ── B2-INPUT-VALIDATION (2026-08-26 Wave-7 Security Hardening) ─────────
    {
        static constexpr std::size_t kMaxQueryBytes = 1 * 1024 * 1024; // 1 MB
        if (static_cast<int>(query.size()) > kMaxQueryBytes) {
            THEMIS_WARN("[SEC] Input validation failed: field=query reason=too_large size={}",static_cast<int>(query.size()));
            return createErrorResponse(http::status::bad_request,
                "prompt too large",
                "query must be <= 1 MB");
        }
        if (!lora_id.empty()) {
            static const std::regex kLoraIdRe{"^[a-zA-Z0-9_-]+$"};
            if (!std::regex_match(lora_id, kLoraIdRe)) {
                THEMIS_WARN("[SEC] Input validation failed: field=lora_id reason=invalid_chars value='{}'", lora_id);
                return createErrorResponse(http::status::bad_request,
                    "lora_id contains invalid characters",
                    "lora_id must match [a-zA-Z0-9_-]+");
            }
        }
        // Tighten max_tokens upper bound (existing check only enforces > 0)
        if (max_tokens > 32768) {
            THEMIS_WARN("[SEC] Input validation failed: field=max_tokens reason=out_of_range value={}", max_tokens);
            return createErrorResponse(http::status::bad_request,
                "max_tokens out of range",
                "max_tokens must be between 1 and 32768");
        }
        if (temperature < 0.0 || temperature > 2.0) {
            THEMIS_WARN("[SEC] Input validation failed: field=temperature reason=out_of_range value={}", temperature);
            return createErrorResponse(http::status::bad_request,
                "temperature out of range",
                "temperature must be between 0.0 and 2.0");
        }
    }
    // ── end input validation ────────────────────────────────────────────────
    
    // Implement RAG workflow
    try {
        // Prepare RAG context.
        llm::RAGContext rag_context;
        rag_context.query = query;
        rag_context.collection_name = collection;
        rag_context.top_k = top_k;
        const auto normalized_budget = llm::ContextWindowBudget::compute(
            static_cast<std::size_t>(max_context_tokens),
            std::string{},
            query,
            static_cast<std::size_t>(response_budget_tokens));
        const int effective_response_budget_tokens =
            static_cast<int>(std::min<std::size_t>(
                normalized_budget.reserved_response_tokens,
                static_cast<std::size_t>(std::numeric_limits<int>::max())));
        const int effective_generation_max_tokens =
            std::min(max_tokens, effective_response_budget_tokens);
        rag_context.max_context_tokens = static_cast<int>(normalized_budget.model_max_tokens);
        rag_context.response_budget_tokens = effective_response_budget_tokens;

        spdlog::info(
            "LLMApiHandler::handleRAG request: query_len={} collection='{}' top_k={} rag_mode='{}' model='{}' max_context_tokens={} response_budget_tokens={} request_max_tokens={}",
            query.size(),
            collection,
            top_k,
            rag_mode,
            model_id.empty() ? std::string{"default"} : model_id,
            rag_context.max_context_tokens,
            rag_context.response_budget_tokens,
            max_tokens);

        auto& plugin_mgr = llm::LLMPluginManager::instance();

        if (!query_engine_) {
            return createErrorResponse(
                http::status::service_unavailable,
                "RAG retrieval engine not configured",
                "Call setQueryEngine() before using /api/v1/llm/rag"
            );
        }


        std::size_t rejected_documents = 0;
        if (!collection.empty() && top_k > 0) {
            try {
                const std::vector<float> query_vec = plugin_mgr.embed(query);
                if (query_vec.empty()) {
                    return createErrorResponse(
                        http::status::service_unavailable,
                        "RAG query embedding unavailable",
                        "The default LLM plugin returned an empty embedding for the query"
                    );
                }

                query::VectorGeoQuery vector_query;
                vector_query.table = collection;
                vector_query.vector_field = body->value("vector_field", std::string{"embedding"});
                vector_query.query_vector = query_vec;
                vector_query.k = static_cast<size_t>(top_k);

                auto retrieval_result = query_engine_->executeVectorGeoQuery(vector_query);
                if (!retrieval_result) {
                    return createErrorResponse(
                        http::status::service_unavailable,
                        "RAG retrieval failed",
                        retrieval_result.error().context()
                    );
                }

                rag_context.documents.reserve(retrieval_result.value().size());
                for (const auto& result : retrieval_result.value()) {
                    const auto content_opt = extractRagDocumentContent(result.entity);
                    const auto source = extractRagDocumentSource(result.pk, result.entity);
                    if (!content_opt.has_value() || source.empty()) {
                        ++rejected_documents;
                        continue;
                    }

                    llm::RAGContext::Document document;
                    document.source = source;
                    document.relevance_score = 1.0f - result.vector_distance;
                    document.metadata = result.entity;
                    document.content = *content_opt;
                    rag_context.documents.push_back(std::move(document));
                }

                if (rag_context.documents.empty()) {
                    const std::string reason = retrieval_result.value().empty()
                        ? "No documents matched the retrieval query"
                        : "Retrieved documents are missing required source/content fields";
                    return createErrorResponse(
                        http::status::service_unavailable,
                        "RAG retrieval returned no usable documents",
                        reason + " (retrieved=" + std::to_string(retrieval_result.value().size()) +
                            ", rejected=" + std::to_string(rejected_documents) + ")"
                    );
                }
            } catch (const std::exception& ve) {
                THEMIS_ERROR("LLMApiHandler::handleRAG retrieval: {}", ve.what());
                return createErrorResponse(
                    http::status::service_unavailable,
                    "RAG retrieval failed",
                    ve.what()
                );
            }
        }

        spdlog::info(
            "LLMApiHandler::handleRAG retrieval prepared: retrieval_attempted={} docs={} rejected={} top_k_effective={}",
            !collection.empty(),
            rag_context.documents.size(),
            rejected_documents,
            rag_context.top_k);
        
        // Prepare inference request
        llm::InferenceRequest llm_request;
        llm_request.prompt = query;
        llm_request.model_id = model_id.empty() ? std::string("default") : model_id;
        llm_request.max_tokens = effective_generation_max_tokens;
        llm_request.temperature = static_cast<float>(temperature);
        llm_request.lora_adapter_id = lora_id;
        llm_request.metadata["rag_mode"] = rag_mode;
        if (body->contains("rag_tensor_slots")) {
            llm_request.metadata["rag_tensor_slots"] = body->at("rag_tensor_slots");
        }
        if (body->contains("rag_tensor_slot_chars")) {
            llm_request.metadata["rag_tensor_slot_chars"] = body->at("rag_tensor_slot_chars");
        }

        // Call LLMPluginManager for RAG inference
        auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);

        json response_data = {
            {"text", llm_response.text},
            {"model", llm_response.model_id.empty() ? llm_request.model_id : llm_response.model_id},
            {"query", query},
            {"collection_effective", collection},
            {"rag_mode_effective", rag_mode},
            {"retrieval_attempted", !collection.empty()},
            {"documents_retrieved", static_cast<int>(rag_context.documents.size())},
            {"documents_rejected", static_cast<int>(rejected_documents)},
            {"top_k_effective", rag_context.top_k},
            {"max_context_tokens_effective", rag_context.max_context_tokens},
            {"response_budget_tokens_effective", rag_context.response_budget_tokens},
            {"tokens_generated", llm_response.tokens_generated},
            {"inference_time_ms", llm_response.inference_time_ms},
            {"cache_hit", llm_response.cache_hit}
        };

        THEMIS_INFO(
            "LLMApiHandler::handleRAG success: query_len={} collection='{}' top_k={} docs_retrieved={} tokens_generated={} inference_time_ms={:.2f} cache_hit={} rag_mode='{}' lora='{}'",
            query.size(),
            collection,
            top_k,
            rag_context.documents.size(),
            llm_response.tokens_generated,
            llm_response.inference_time_ms,
            llm_response.cache_hit,
            rag_mode,
            lora_id.empty() ? "<none>" : lora_id
        );

        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLMApiHandler::handleRAG: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            "RAG inference failed",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleEmbed(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleEmbed");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string text = {};
    std::string model_id = {};
    
    try {
        if (body->contains("text")) {
            text = json_value_to<std::string>(body->at("text"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'text' field");
        }
        
        if (body->contains("model")) {
            model_id = json_value_to<std::string>(body->at("model"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid embed parameters", e.what());
    }
    
    // Generate embeddings using EmbeddedLLM
    try {
        auto embedding = THEMIS_LLM_EMBED(text);
        
        json embedding_vector = json::array();
        for (const auto& val : embedding) {
            embedding_vector.push_back(val);
        }
        
        json response_body = {
            {"embedding", embedding_vector},
            {"dimensions",static_cast<int>(embedding.size())},
            {"text_length", text.length()}
        };
        
        return createJsonResponse(http::status::ok, response_body);
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLMApiHandler::handleEmbed: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            "Embedding generation failed",
            e.what()
        );
    }
}

static constexpr int kMaxTokensLimit = 4096;

http::response<http::string_body> LLMApiHandler::handleStreamInference(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleStreamInference");

    // Parse query parameters from URL: prompt, request_id, max_tokens
    std::string prompt = {};
    std::string request_id = {};
    int max_tokens = 512;

    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        std::string qs = target.substr(qpos + 1);
        auto extract = [&]([[maybe_unused]] const std::string& key) -> std::string {
            std::string prefix = key + "=";
            auto pos = qs.find(prefix);
            if (pos == std::string::npos) return {};
            auto end = qs.find('&', pos);
            std::string raw = qs.substr(pos + static_cast<int>(prefix.size()) ,
                end == std::string::npos ? std::string::npos : end - pos - static_cast<int>(prefix.size()) );
            // Basic URL-decode
            std::string decoded = {};
            decoded.reserve(raw.size());
            for (size_t i = 0; i <static_cast<int>(raw.size()); ) {
                if (raw[i] == '+') { decoded += ' '; ++i; }
                else if (raw[i] == '%' && i + 2 <static_cast<int>(raw.size())) {
                    auto is_hex = [](char c) {
                        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
                    };
                    if (is_hex(raw[i+1]) && is_hex(raw[i+2])) {
                        char hex[3] = {raw[i+1], raw[i+2], '\0'};
                        decoded += static_cast<char>(std::strtol(hex, nullptr, 16));
                        i += 3;
                    } else {
                        decoded += raw[i++]; // keep invalid % sequence as-is
                    }
                } else {
                    decoded += raw[i++];
                }
            }
            return decoded;
        };
        prompt = extract("prompt");
        request_id = extract("request_id");
        std::string max_tokens_str = extract("max_tokens");
        if (!max_tokens_str.empty()) {
            try {
                int n = std::stoi(max_tokens_str);
                if (n > 0 && n <= kMaxTokensLimit) {
                  max_tokens = n;
                }
            } catch (const std::exception& e) {
                THEMIS_DEBUG("LLMApiHandler: ignoring invalid max_tokens query parameter: {}", e.what());
            }
        }
    }

    if (prompt.empty()) {
        return createErrorResponse(http::status::bad_request,
            "Missing 'prompt' query parameter");
    }

    spdlog::info(
        "LLMApiHandler::handleStreamInference start: request_id='{}' prompt_len={} max_tokens={}",
        request_id,
        prompt.size(),
        max_tokens);

    // PHASE2-OPTIMIZATION: missing_vector_reserve — pre-allocate for streaming response body
    // SSE responses can accumulate many events; reserve space to avoid repeated reallocations
    std::string sse_body = {};
    sse_body.reserve(64 * 1024);  // Reserve 64 KB for typical SSE response
    sse_body += "retry: 3000\n\n";

    try {
        auto& llm = llm::EmbeddedLLMManager::instance().get();
        llm.generateStreamingSSE(
            prompt,
            [&sse_body]([[maybe_unused]] const std::string& sse_event) {
                sse_body += sse_event;
            },
            request_id,
            max_tokens
        );
        // Emit terminal done event
        sse_body += "event: done\ndata: {\"done\":true}\n\n";
        spdlog::info(
            "LLMApiHandler::handleStreamInference complete: request_id='{}' sse_bytes={}",
            request_id,
            sse_body.size());
    } catch (const std::exception& e) {
        spdlog::warn(
            "LLMApiHandler::handleStreamInference failed: request_id='{}' error='{}'",
            request_id,
            e.what());
        json err_event = {{"error", true}, {"message", std::string(e.what())}};
        sse_body += "event: error\ndata: " + err_event.dump() + "\n\n";
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/event-stream");
    res.set(http::field::cache_control, "no-cache, no-transform");
    res.set(http::field::connection, "keep-alive");
    // GAP-012 fixed: no hardcoded CORS wildcard; CORS is applied by the central
    // HttpServer dispatch layer via THEMIS_CORS_* env vars.
    res.set(http::field::server, "ThemisDB-LLM/1.3.0");
    res.keep_alive(true);
    res.body() = std::move(sse_body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> LLMApiHandler::handleStreamExplainAql(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleStreamExplainAql");

    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }

    std::string aql_query = {};
    std::string schema_context = {};
    std::string request_id = {};

    try {
        if (body->contains("query")) {
            aql_query = body->at("query").get<std::string>();
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'query' field");
        }
        if (body->contains("schema_context")) {
            schema_context = body->at("schema_context").get<std::string>();
        }
        if (body->contains("request_id")) {
            request_id = body->at("request_id").get<std::string>();
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request,
            "Invalid request parameters", e.what());
    }

    // Collect SSE events from AQL explanation streaming
    std::string sse_body = {};
    sse_body += "retry: 3000\n\n";

    spdlog::info(
        "LLMApiHandler::handleStreamExplainAql start: request_id='{}' query_len={} schema_ctx_len={}",
        request_id,
        aql_query.size(),
        schema_context.size());

    try {
        aql::LLMAQLHandler aql_handler;
        aql_handler.streamExplainAQLAsSSE(
            aql_query,
            [&sse_body]([[maybe_unused]] const std::string& sse_event) {
                sse_body += sse_event;
            },
            request_id,
            schema_context
        );
        // Emit terminal done event
        sse_body += "event: done\ndata: {\"done\":true}\n\n";
        spdlog::info(
            "LLMApiHandler::handleStreamExplainAql complete: request_id='{}' sse_bytes={}",
            request_id,
            sse_body.size());
    } catch (const aql::LLMException& e) {
        spdlog::warn(
            "LLMApiHandler::handleStreamExplainAql LLMException: request_id='{}' code={} error='{}'",
            request_id,
            static_cast<int>(e.getErrorCode()),
            e.what());
        json err_event = {{"error", true}, {"message", std::string(e.what())},
                          {"code", static_cast<int>(e.getErrorCode())}};
        sse_body += "event: error\ndata: " + err_event.dump() + "\n\n";
    } catch (const std::exception& e) {
        spdlog::warn(
            "LLMApiHandler::handleStreamExplainAql failed: request_id='{}' error='{}'",
            request_id,
            e.what());
        json err_event = {{"error", true}, {"message", std::string(e.what())}};
        sse_body += "event: error\ndata: " + err_event.dump() + "\n\n";
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/event-stream");
    res.set(http::field::cache_control, "no-cache, no-transform");
    res.set(http::field::connection, "keep-alive");
    // GAP-012 fixed: no hardcoded CORS wildcard; CORS is applied by the central
    // HttpServer dispatch layer via THEMIS_CORS_* env vars.
    res.set(http::field::server, "ThemisDB-LLM/1.3.0");
    res.keep_alive(true);
    res.body() = std::move(sse_body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> LLMApiHandler::handleListModels(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleListModels");
    
    // Get model list from LLMPluginManager
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto model_list = plugin_mgr.listModels();
        
        json models = json::array();
        for (const auto& model_id : model_list) {
            models.push_back(json{{"model_id", model_id}});
        }
        
        json response_data = {
            {"models", models},
            {"total", static_cast<int>(models.size())}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLMApiHandler::handleListModels: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to list models",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleLoadModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleLoadModel");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string model_id = {};
    std::string path = {};
    
    try {
        if (body->contains("model_id")) {
            model_id = json_value_to<std::string>(body->at("model_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
        }
        
        if (body->contains("path")) {
            path = json_value_to<std::string>(body->at("path"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid load model parameters", e.what());
    }
    
    // --- S1: Empty-path guard — reject before attempting any fs/integrity work ---
    if (path.empty()) {
        return createErrorResponse(http::status::bad_request,
                                   "Missing 'path' field",
                                   "model path must be provided for load operation");
    }

    // --- S2: Path canonicalization and traversal guard ---
    try {
        auto canonical = std::filesystem::weakly_canonical(std::filesystem::path(path));
        const char* base_env = std::getenv("THEMIS_MODEL_BASE_DIR");
        if (base_env) {
            auto base = std::filesystem::weakly_canonical(std::filesystem::path(base_env));
            auto rel  = std::mismatch(base.begin(), base.end(), canonical.begin());
            if (rel.first != base.end()) {
                return createErrorResponse(http::status::bad_request,
                                           "Invalid model path",
                                           "path traversal detected");
            }
        }
        path = canonical.string();
    } catch (const std::filesystem::filesystem_error& e) {
        return createErrorResponse(http::status::bad_request, "Invalid model path", e.what());
    }

    // --- A1: Model Integrity Gate ---
    // If a manifest entry exists for this model_id, the SHA-256 of 'path' MUST
    // match before we allow the load.  Missing manifest → graceful pass-through
    // (operator has not yet deployed an integrity manifest).
    if (!path.empty()) {
        auto expected_hash = ModelIntegrityVerifier::getExpectedHash(model_id);
        if (expected_hash.has_value()) {
            const bool hash_ok = ModelIntegrityVerifier::verifyModel(path, *expected_hash);
            if (!hash_ok) {
                THEMIS_ERROR(
                    "handleLoadModel: integrity check FAILED for model_id='{}' path='{}'",
                    model_id, path);
                return createErrorResponse(http::status::forbidden,
                                           "Model integrity verification failed",
                                           "SHA-256 mismatch — model may be corrupt or tampered");
            }
            THEMIS_INFO(
                "handleLoadModel: integrity check passed for model_id='{}'", model_id);
        } else {
            THEMIS_INFO(
                "handleLoadModel: no manifest entry for model_id='{}', skipping integrity check",
                model_id);
        }
    }

    // Call LLMPluginManager to load model
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        bool loaded = plugin_mgr.loadModel(model_id, path);
        if (!loaded && plugin_mgr.getDefaultPlugin() == nullptr) {
            if (llm::createLlamaWrapper("llamacpp", "", json::object())) {
                loaded = plugin_mgr.loadModel(model_id, path);
            }
        }
        if (!loaded) {
            return createErrorResponse(
                http::status::internal_server_error,
                "Failed to load model",
                "Plugin returned false while loading model"
            );
        }
        
        json response_data = {
            {"model_id", model_id},
            {"status", "loaded"},
            {"message", "Model loaded successfully"}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLMApiHandler::handleLoadModel: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to load model",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleUnloadModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleUnloadModel");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string model_id = {};
    
    try {
        if (body->contains("model_id")) {
            model_id = json_value_to<std::string>(body->at("model_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid unload model parameters", e.what());
    }
    
    // Call LLMPluginManager to unload model
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.unloadModel(model_id);
        
        json response_data = {
            {"model_id", model_id},
            {"status", "unloaded"},
            {"message", "Model unloaded successfully"}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to unload model",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleUnloadModel");
        return createErrorResponse(http::status::internal_server_error, "Failed to unload model");
    }
}

http::response<http::string_body> LLMApiHandler::handleModelInfo(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleModelInfo");
    
    // Extract model_id from path
    std::string_view target = req.target();
    std::string model_id = std::string(target.substr(target.find_last_of('/') + 1));
    
    // Get model info from LLMPluginManager
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto model_info = plugin_mgr.getModelInfo(model_id);
        if (!model_info) {
            return createErrorResponse(
                http::status::not_found,
                "Model not found",
                "Unknown model_id"
            );
        }
        
        json response_data = {
            {"model_id", model_info->model_id.empty() ? model_id : model_info->model_id},
            {"status", model_info->is_loaded ? "loaded" : "available"},
            {"size_mb", model_info->size_bytes / (1024 * 1024)},
            {"format", model_info->format},
            {"quantization", model_info->quantization},
            {"context_length", model_info->context_length},
            {"loaded_at", model_info->loaded_at}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::not_found,
            "Model not found",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleModelInfo");
        return createErrorResponse(http::status::internal_server_error, "Model info retrieval failed");
    }
}

http::response<http::string_body> LLMApiHandler::handleIngestModel(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleIngestModel");
    
    // Implement model ingestion with chunked upload to RocksDB Blob Store
    // Parse multipart/form-data for model file and metadata
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid request body");
    }
    
    std::string model_id = {};
    std::string file_data = {};
    
    try {
        if (body->contains("model_id")) {
            model_id = json_value_to<std::string>(body->at("model_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
        }
        
        // In production, this would handle multipart/form-data with chunked streaming
        // For now, we accept JSON with base64-encoded data (simplified)
        if (body->contains("file_data")) {
            file_data = json_value_to<std::string>(body->at("file_data"));
        }
        
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.ingestModel(model_id, file_data);
        
        json response_data = {
            {"model_id", model_id},
            {"status", "ingested"},
            {"message", "Model successfully ingested and replicated"},
            {"urn", "urn:themis:model:" + model_id + ":v1"}
        };
        
        return createJsonResponse(response_data, http::status::created);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Model ingestion failed",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleIngestModel");
        return createErrorResponse(http::status::internal_server_error, "Model ingestion failed");
    }
}

http::response<http::string_body> LLMApiHandler::handleListLoRAs(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleListLoRAs");
    
    // Get LoRA list from LLMPluginManager
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto lora_list = plugin_mgr.listLoRAs();
        
        json loras = json::array();
        for (const auto& lora : lora_list) {
            json lora_obj = {
                {"lora_id", lora.lora_id.empty() ? lora.id : lora.lora_id},
                {"status", lora.is_loaded ? "loaded" : "available"},
                {"base_model", lora.base_model},
                {"size_mb", lora.size_bytes / (1024 * 1024)}
            };
            loras.push_back(lora_obj);
        }
        
        json response_data = {
            {"loras", loras},
            {"total", static_cast<int>(loras.size())}
        };

        THEMIS_INFO("LLMApiHandler::handleListLoRAs success: total={}",static_cast<int>(loras.size()));
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to list LoRAs",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleListLoRAs");
        return createErrorResponse(http::status::internal_server_error, "Failed to list LoRAs");
    }
}

http::response<http::string_body> LLMApiHandler::handleLoadLoRA(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleLoadLoRA");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string lora_id = {};
    std::string path = {};
    std::string base_model = {};
    
    try {
        if (body->contains("lora_id")) {
            lora_id = json_value_to<std::string>(body->at("lora_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'lora_id' field");
        }
        
        if (body->contains("path")) {
            path = json_value_to<std::string>(body->at("path"));
        }
        
        if (body->contains("base_model")) {
            base_model = json_value_to<std::string>(body->at("base_model"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid load LoRA parameters", e.what());
    }
    
    // Call LLMPluginManager to load LoRA
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.loadLoRA(lora_id, path, base_model);
        
        json response_data = {
            {"lora_id", lora_id},
            {"base_model", base_model},
            {"status", "loaded"},
            {"message", "LoRA loaded successfully"}
        };

        THEMIS_INFO(
            "LLMApiHandler::handleLoadLoRA success: lora_id='{}' base_model='{}' path='{}'",
            lora_id,
            base_model.empty() ? "<unknown>" : base_model,
            path.empty() ? "<unspecified>" : path
        );
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to load LoRA",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleLoadLoRA");
        return createErrorResponse(http::status::internal_server_error, "Failed to load LoRA");
    }
}

http::response<http::string_body> LLMApiHandler::handleUnloadLoRA(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleUnloadLoRA");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string lora_id = {};
    
    try {
        if (body->contains("lora_id")) {
            lora_id = json_value_to<std::string>(body->at("lora_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'lora_id' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid unload LoRA parameters", e.what());
    }
    
    // Call LLMPluginManager to unload LoRA
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.unloadLoRA(lora_id);
        
        json response_data = {
            {"lora_id", lora_id},
            {"status", "unloaded"},
            {"message", "LoRA unloaded successfully"}
        };

        THEMIS_INFO("LLMApiHandler::handleUnloadLoRA success: lora_id='{}'", lora_id);
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to unload LoRA",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleUnloadLoRA");
        return createErrorResponse(http::status::internal_server_error, "Failed to unload LoRA");
    }
}

http::response<http::string_body> LLMApiHandler::handleStats(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleStats");
    
    // Get statistics from AsyncInferenceEngine and LLMPluginManager
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto stats = plugin_mgr.getStatistics();
        
        json response_data = {
            {"throughput_req_per_sec", stats.throughput},
            {"average_latency_ms", stats.average_latency_ms},
            {"cache_hit_rate", stats.cache_hit_rate},
            {"total_requests", stats.total_requests},
            {"active_workers", stats.active_workers},
            {"queue_depth", stats.queue_depth}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get statistics",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleStats");
        return createErrorResponse(http::status::internal_server_error, "Failed to get statistics");
    }
}

http::response<http::string_body> LLMApiHandler::handleCacheStats(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleCacheStats");
    
    // Get cache statistics from LLMResponseCache and LLMPrefixCache
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto cache_stats = plugin_mgr.getCacheStatistics();
        
        json response_cache = {
            {"hits", cache_stats.response_cache_hits},
            {"misses", cache_stats.response_cache_misses},
            {"hit_rate", cache_stats.response_cache_hit_rate},
            {"total_entries", cache_stats.response_cache_entries}
        };
        
        json prefix_cache = {
            {"hits", cache_stats.prefix_cache_hits},
            {"misses", cache_stats.prefix_cache_misses},
            {"hit_rate", cache_stats.prefix_cache_hit_rate},
            {"total_prefixes", cache_stats.prefix_cache_entries}
        };
        
        json response_data = {
            {"response_cache", response_cache},
            {"prefix_cache", prefix_cache}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to get cache statistics",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleCacheStats");
        return createErrorResponse(http::status::internal_server_error, "Failed to get cache statistics");
    }
}

http::response<http::string_body> LLMApiHandler::handleClearCache(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleClearCache");
    
    // Clear LLMResponseCache and LLMPrefixCache
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.clearAllCaches();
        
        json response_data = {
            {"status", "cleared"},
            {"message", "All LLM caches cleared successfully"}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to clear caches",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleClearCache");
        return createErrorResponse(http::status::internal_server_error, "Failed to clear caches");
    }
}

http::response<http::string_body> LLMApiHandler::handleHealth(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleHealth");
    
    // Check health of LLMPluginManager and AsyncInferenceEngine
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto health = plugin_mgr.getHealthStatus();
        
        json response_data = {
            {"status", health.is_healthy ? "healthy" : "degraded"},
            {"plugin_manager", health.plugin_manager_status},
            {"async_engine", health.async_engine_status},
            {"models_loaded", health.models_loaded},
            {"loras_loaded", health.loras_loaded}
        };
        
        auto http_status = health.is_healthy ? http::status::ok : http::status::service_unavailable;
        return createJsonResponse(response_data, http_status);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Health check failed",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleHealth");
        return createErrorResponse(http::status::internal_server_error, "Health check failed");
    }
}

bool LLMApiHandler::validateBearerToken([[maybe_unused]] const http::request<http::string_body>& req) {
    auto token = extractBearerToken(req);
    if (!token) {
        return false;
    }
    
    if (!jwt_validator_) {
        static bool warning_logged = false;
        if (!warning_logged) {
            std::cerr << "WARNING: JWT validator not configured. Denying access." << std::endl;
            warning_logged = true;
        }
        return false;
    }
    auto& jwt_validator = *jwt_validator_;
    
    try {
        auto claims = jwt_validator.parseAndValidate(*token);
        // Token is valid
        return true;
    } catch (const std::exception& e) {
        // Token validation failed (expired, invalid signature, etc.).
        // Log at DEBUG level to avoid flooding logs with expected auth failures,
        // but provide a hook for diagnosing misconfigured tokens.
        THEMIS_DEBUG("LLMApiHandler: JWT validation exception — treating token as invalid: {}", e.what());
        return false;
    } catch (...) {
        // Preserve fail-closed auth semantics for non-std exceptions too.
        THEMIS_DEBUG([[maybe_unused]] "LLMApiHandler: JWT validation non-standard exception — treating token as invalid");
        return false;
    }
}

http::response<http::string_body> LLMApiHandler::createErrorResponse(
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
    res.set(http::field::server, "ThemisDB-LLM/1.3.0");
    res.body() = error_obj.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> LLMApiHandler::createJsonResponse(
    const json& data,
    http::status status) {
    auto span = Tracer::startSpan("createJsonResponse");
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB-LLM/1.3.0");
    res.body() = data.dump();
    res.prepare_payload();
    return res;
}

std::optional<json> LLMApiHandler::parseRequestBody(
    const http::request<http::string_body>& req) {
    
    try {
        auto parsed = json::parse(req.body());
        if (parsed.is_object()) {
            return parsed;
        }
    } catch (const json::exception& e) {
        THEMIS_DEBUG("LLMApiHandler: failed to parse JSON request body: {}", e.what());
        return std::nullopt;
    }
    
    return std::nullopt;
}

// Documentation Assistant Endpoints

http::response<http::string_body> LLMApiHandler::handleDocsQuery(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDocsQuery");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string query = {};
    
    try {
        if (body->contains("query")) {
            query = json_value_to<std::string>(body->at("query"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'query' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid query parameters", e.what());
    }
    
    try {
        // Create and configure documentation assistant
        static llm::DocsAssistant assistant;
        static bool initialized = false;
        
        if (!initialized) {
            if (!assistant.loadDatabase()) {
                return createErrorResponse(
                    http::status::service_unavailable,
                    "Documentation database not available",
                    "docs_database.json not found or failed to load"
                );
            }
            initialized = true;
        }
        
        // Query documentation
        auto result = assistant.query(query);
        
        // Build response
        json response_data = {
            {"query", query},
            {"answer", result.generated_answer},
            {"confidence_score", result.confidence_score},
            {"documents_searched", result.total_docs_searched},
            {"documents_used", result.docs_included_in_context},
            {"search_time_ms", result.search_time_ms.count()},
            {"generation_time_ms", result.generation_time_ms.count()}
        };
        
        // Include relevant documents metadata
        json docs_array = json::array();
        for (const auto& doc : result.relevant_docs) {
            docs_array.push_back({
                {"file_name", doc.file_name},
                {"relevance_score", doc.relevance_score},
                {"content_preview", doc.text_content.substr(0, 200) + "..."}
            });
        }
        response_data["relevant_documents"] = docs_array;
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Documentation query failed",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleDocsQuery");
        return createErrorResponse(http::status::internal_server_error, "Documentation query failed");
    }
}

http::response<http::string_body> LLMApiHandler::handleDocsConfig(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDocsConfig");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string topic = {};
    
    try {
        if (body->contains("topic")) {
            topic = json_value_to<std::string>(body->at("topic"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'topic' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid parameters", e.what());
    }
    
    try {
        // Create and configure documentation assistant
        static llm::DocsAssistant assistant;
        static bool initialized = false;
        
        if (!initialized) {
            if (!assistant.loadDatabase()) {
                return createErrorResponse(
                    http::status::service_unavailable,
                    "Documentation database not available",
                    "docs_database.json not found or failed to load"
                );
            }
            initialized = true;
        }
        
        // Get configuration help
        auto result = assistant.getConfigHelp(topic);
        
        // Build response
        json response_data = {
            {"topic", topic},
            {"configuration_help", result.generated_answer},
            {"confidence_score", result.confidence_score},
            {"documents_used", result.docs_included_in_context}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Configuration help failed",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleDocsConfig");
        return createErrorResponse(http::status::internal_server_error, "Configuration help failed");
    }
}

http::response<http::string_body> LLMApiHandler::handleDocsTroubleshoot(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDocsTroubleshoot");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string error_description = {};
    
    try {
        if (body->contains("error")) {
            error_description = json_value_to<std::string>(body->at("error"));
        } else if (body->contains("issue")) {
            error_description = json_value_to<std::string>(body->at("issue"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'error' or 'issue' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid parameters", e.what());
    }
    
    try {
        // Create and configure documentation assistant
        static llm::DocsAssistant assistant;
        static bool initialized = false;
        
        if (!initialized) {
            if (!assistant.loadDatabase()) {
                return createErrorResponse(
                    http::status::service_unavailable,
                    "Documentation database not available",
                    "docs_database.json not found or failed to load"
                );
            }
            initialized = true;
        }
        
        // Get troubleshooting help
        auto result = assistant.getTroubleshootingHelp(error_description);
        
        // Build response
        json response_data = {
            {"error", error_description},
            {"troubleshooting_help", result.generated_answer},
            {"confidence_score", result.confidence_score},
            {"documents_used", result.docs_included_in_context}
        };
        
        return createJsonResponse(response_data);
        
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Troubleshooting help failed",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleDocsTroubleshoot");
        return createErrorResponse(http::status::internal_server_error, "Troubleshooting help failed");
    }
}

// ===== Feedback Endpoints =====

http::response<http::string_body> LLMApiHandler::handleCreateFeedback(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleCreateFeedback");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    // Extract feedback parameters
    try {
        llm::FeedbackStore::FeedbackEntry feedback;
        
        // Required fields
        if (!body->contains("type")) {
            return createErrorResponse(http::status::bad_request, "Missing 'type' field (positive or negative)");
        }
        std::string type_str = json_value_to<std::string>(body->at("type"));
        if (type_str == "positive") {
            feedback.type = llm::FeedbackType::POSITIVE;
        } else if (type_str == "negative") {
            feedback.type = llm::FeedbackType::NEGATIVE;
        } else {
            return createErrorResponse(http::status::bad_request, "Invalid 'type' value (must be 'positive' or 'negative')");
        }
        
        if (!body->contains("question")) {
            return createErrorResponse(http::status::bad_request, "Missing 'question' field");
        }
        feedback.question = json_value_to<std::string>(body->at("question"));
        
        if (!body->contains("answer")) {
            return createErrorResponse(http::status::bad_request, "Missing 'answer' field");
        }
        feedback.answer = json_value_to<std::string>(body->at("answer"));
        
        // Optional fields
        if (body->contains("user_id")) {
            feedback.user_id = json_value_to<std::string>(body->at("user_id"));
        }
        
        if (body->contains("interaction_id")) {
            feedback.interaction_id = json_value_to<std::string>(body->at("interaction_id"));
        }
        
        if (body->contains("correction")) {
            feedback.correction = json_value_to<std::string>(body->at("correction"));
        }
        
        if (body->contains("comment")) {
            feedback.comment = json_value_to<std::string>(body->at("comment"));
        }
        
        if (body->contains("model_version")) {
            feedback.model_version = json_value_to<std::string>(body->at("model_version"));
        }
        
        if (body->contains("adapter_id")) {
            feedback.adapter_id = json_value_to<std::string>(body->at("adapter_id"));
        }
        
        if (body->contains("adapter_version")) {
            feedback.adapter_version = json_value_to<std::string>(body->at("adapter_version"));
        }
        
        // Integrate with FeedbackStore
        if (!feedback_store_) {
            return createErrorResponse(
                http::status::service_unavailable, 
                "FeedbackStore not available",
                "FeedbackStore has not been configured for this handler"
            );
        }
        auto& feedback_store = *feedback_store_;
        
        // Store feedback using FeedbackStore
        auto stored = feedback_store.createFeedback(feedback);
        
        // Convert to JSON response
        json response_data = stored.toJson();
        response_data["message"] = "Feedback recorded successfully";
        
        return createJsonResponse(response_data, http::status::created);
        
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid feedback parameters", e.what());
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleCreateFeedback");
        return createErrorResponse(http::status::internal_server_error, "Feedback creation failed");
    }
}

http::response<http::string_body> LLMApiHandler::handleGetFeedback(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleGetFeedback");
    
    // Extract feedback ID from path
    std::string_view target = req.target();
    std::string prefix = "/api/v1/llm/feedback/";
    
    if (!target.starts_with(prefix)) {
        return createErrorResponse(http::status::bad_request, "Invalid feedback endpoint");
    }
    
    // Extract ID, stopping at query parameters if present
    std::string_view id_part = target.substr(prefix.length());
    size_t query_pos = id_part.find('?');
    if (query_pos != std::string_view::npos) {
        id_part = id_part.substr(0, query_pos);
    }
    
    std::string feedback_id{id_part};
    
    if (feedback_id.empty()) {
        return createErrorResponse(http::status::bad_request, "Missing feedback ID");
    }
    
    // Check if FeedbackStore is available
    if (!feedback_store_) {
        return createErrorResponse(
            http::status::service_unavailable, 
            "FeedbackStore not available",
            "FeedbackStore has not been configured for this handler"
        );
    }
    auto& feedback_store = *feedback_store_;
    
    // Get feedback from store
    auto feedback = feedback_store.getFeedback(feedback_id);
    
    if (!feedback) {
        return createErrorResponse(
            http::status::not_found, 
            "Feedback not found",
            "No feedback with ID: " + feedback_id
        );
    }
    
    // Convert to JSON and return
    json response_data = feedback->toJson();
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleListFeedback(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleListFeedback");
    
    // Parse query parameters
    std::string_view target = req.target();
    size_t limit = 100;
    std::string type_filter = {};
    std::string status_filter = {};
    
    // Simple query parameter parsing
    size_t query_pos = target.find('?');
    if (query_pos != std::string_view::npos) {
        std::string_view query_string = target.substr(query_pos + 1);
        
        // Parse limit parameter
        size_t limit_pos = query_string.find("limit=");
        if (limit_pos != std::string_view::npos) {
            size_t limit_end = query_string.find('&', limit_pos);
            std::string limit_str{query_string.substr(limit_pos + 6, 
                limit_end == std::string_view::npos ? std::string_view::npos : limit_end - limit_pos - 6)};
            try {
                limit = std::stoul(limit_str);
                if (limit > 1000) limit = 1000; // Cap at 1000
            } catch (const std::exception& e) {
                THEMIS_DEBUG("LLMApiHandler: ignoring invalid feedback limit query parameter: {}", e.what());
            }
        }
        
        // Parse type filter
        size_t type_pos = query_string.find("type=");
        if (type_pos != std::string_view::npos) {
            size_t type_end = query_string.find('&', type_pos);
            type_filter = std::string{query_string.substr(type_pos + 5,
                type_end == std::string_view::npos ? std::string_view::npos : type_end - type_pos - 5)};
        }
        
        // Parse status filter
        size_t status_pos = query_string.find("status=");
        if (status_pos != std::string_view::npos) {
            size_t status_end = query_string.find('&', status_pos);
            status_filter = std::string{query_string.substr(status_pos + 7,
                status_end == std::string_view::npos ? std::string_view::npos : status_end - status_pos - 7)};
        }
    }
    
    // Check if FeedbackStore is available
    if (!feedback_store_) {
        return createErrorResponse(
            http::status::service_unavailable, 
            "FeedbackStore not available",
            "FeedbackStore has not been configured for this handler"
        );
    }
    auto& feedback_store = *feedback_store_;
    
    // Build list options
    llm::FeedbackStore::ListOptions options;
    options.limit = limit;
    
    // Apply type filter
    if (!type_filter.empty()) {
        if (type_filter == "positive") {
            options.filter_type = llm::FeedbackType::POSITIVE;
        } else if (type_filter == "negative") {
            options.filter_type = llm::FeedbackType::NEGATIVE;
        }
    }
    
    // Apply status filter
    if (!status_filter.empty()) {
        if (status_filter == "pending") {
            options.filter_status = llm::ValidationStatus::PENDING;
        } else if (status_filter == "approved") {
            options.filter_status = llm::ValidationStatus::APPROVED;
        } else if (status_filter == "rejected") {
            options.filter_status = llm::ValidationStatus::REJECTED;
        } else if (status_filter == "flagged") {
            options.filter_status = llm::ValidationStatus::FLAGGED;
        }
    }
    
    // Get feedback list from store
    try {
        auto feedback_list = feedback_store.listFeedback(options);
        
        // Convert to JSON array
        json feedback_array = json::array();
        for (const auto& feedback : feedback_list) {
            feedback_array.push_back(feedback.toJson());
        }
        
        json response_data = {
            {"feedback", feedback_array},
            {"count",static_cast<int>(feedback_array.size())},
            {"limit", limit}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to list feedback",
            e.what()
        );
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleListFeedback");
        return createErrorResponse(http::status::internal_server_error, "Failed to list feedback");
    }
}

http::response<http::string_body> LLMApiHandler::handleFeedbackStats(
    const http::request<http::string_body>& /*req*/) {
    auto span = Tracer::startSpan("handleFeedbackStats");
    
    // Check if FeedbackStore is available
    if (!feedback_store_) {
        return createErrorResponse(
            http::status::service_unavailable, 
            "FeedbackStore not available",
            "FeedbackStore has not been configured for this handler"
        );
    }
    auto& feedback_store = *feedback_store_;
    
    // Get stats from store
    auto stats = feedback_store.getStats();
    
    // Convert to JSON
    json response_data = {
        {"total_feedback", stats.total_feedback},
        {"positive_count", stats.positive_count},
        {"negative_count", stats.negative_count},
        {"pending_validation", stats.pending_validation},
        {"approved_count", stats.approved_count},
        {"rejected_count", stats.rejected_count},
        {"unused_for_training", stats.unused_for_training},
        {"used_for_training", stats.used_for_training},
        {"positive_ratio", stats.positive_ratio}
    };
    
    return createJsonResponse(response_data);
}

// ─────────────────────────────────────────────────────────────────────────────
// OpenAI-compatible endpoints
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> LLMApiHandler::handleOpenAIChatCompletions(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleOpenAIChatCompletions");
    try {
        auto body = parseRequestBody(req);
        if (!body) {
            auto err = llm::OpenAICompatAdapter::buildError(
                "Invalid JSON body", "invalid_request_error");
            return createJsonResponse(err, http::status::bad_request);
        }

        // ── API key / governance check ──────────────────────────────────────
        // When a PolicyEngine is configured, validate the caller's identity and
        // data-classification policy before any inference work is started.
        // Returns HTTP 401 for missing/malformed tokens, HTTP 403 for denied policy.
        if (policy_engine_) {
            // Collect headers from the Boost.Beast request into the flat map
            // expected by PolicyEngine::checkInferencePermission().
            std::unordered_map<std::string, std::string> header_map = {};

            for (const auto& field : req) {
                header_map[std::string(field.name_string())] =
                    std::string(field.value());
            }
            auto perm = policy_engine_->checkInferencePermission(header_map);
            if (!perm.allowed) {
                auto err = llm::OpenAICompatAdapter::buildError(
                    perm.denial_reason, "invalid_request_error",
                    perm.http_status == 401 ? "invalid_api_key" : "policy_denied");
                return createJsonResponse(
                    err,
                    perm.http_status == 401 ? http::status::unauthorized
                                            : http::status::forbidden);
            }
        }

        // Determine whether the client wants streaming output
        bool streaming = false;
        if (body->contains("stream") && (*body)["stream"].is_boolean()) {
            streaming = (*body)["stream"].get<bool>();
        }

        // Parse the OpenAI request into an InferenceRequest
        auto parse_result = llm::OpenAICompatAdapter::parseRequest(*body);
        if (std::holds_alternative<std::string>(parse_result)) {
            const std::string& msg = std::get<std::string>(parse_result);
            auto err = llm::OpenAICompatAdapter::buildError(msg, "invalid_request_error");
            return createJsonResponse(err, http::status::bad_request);
        }
        auto& llm_request = std::get<llm::InferenceRequest>(parse_result);

        // Capture the model name for the response (may be empty → engine picks default)
        const std::string model_id = llm_request.model_id;

        // Pre-generate the completion ID and created timestamp so they are
        // consistent across all chunks / the single response object
        const std::string completion_id =
            llm::OpenAICompatAdapter::generateCompletionId();

        if (streaming) {
            // ── Streaming path ────────────────────────────────────────────
            // Collect SSE chunks into a single response body.
            // In a production server with a real async HTTP layer this would
            // write chunks incrementally; here we buffer them for compatibility
            // with the synchronous response model used by LLMApiHandler.
            std::string sse_body;

            // Capture created timestamp once so all chunks share it
            int64_t created = static_cast<int64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());

            spdlog::info(
                "LLMApiHandler::handleOpenAIChatCompletions stream start: model='{}' prompt_len={} request_max_tokens={}",
                model_id.empty() ? std::string{"default"} : model_id,
                llm_request.prompt.size(),
                llm_request.max_tokens);

            llm_request.stream_callback = [&]([[maybe_unused]] const std::string& token) {
                sse_body += llm::OpenAICompatAdapter::buildStreamChunk(
                    token, completion_id, model_id, created);
            };

            try {
                auto& plugin_mgr = llm::LLMPluginManager::instance();
                plugin_mgr.generate(llm_request);
            } catch (const std::exception& e) {
                spdlog::warn(
                    "LLMApiHandler::handleOpenAIChatCompletions stream failed: model='{}' error='{}'",
                    model_id.empty() ? std::string{"default"} : model_id,
                    e.what());
                auto err = llm::OpenAICompatAdapter::buildError(
                    std::string{"Inference failed: "} + e.what(),
                    "server_error");
                return createJsonResponse(err, http::status::internal_server_error);
            } catch (...) {
                spdlog::warn(
                    "LLMApiHandler::handleOpenAIChatCompletions stream failed with unknown error: model='{}'",
                    model_id.empty() ? std::string{"default"} : model_id);
                logCurrentException([[maybe_unused]] "LLMApiHandler::handleOpenAIChatCompletions streaming");
                auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
                return createJsonResponse(err, http::status::internal_server_error);
            }

            sse_body += llm::OpenAICompatAdapter::buildStreamFinalChunk(
                completion_id, model_id, created);
            sse_body += llm::OpenAICompatAdapter::buildStreamDone();

            spdlog::info(
                "LLMApiHandler::handleOpenAIChatCompletions stream complete: model='{}' sse_bytes={}",
                model_id.empty() ? std::string{"default"} : model_id,
                sse_body.size());

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "text/event-stream");
            res.set(http::field::cache_control, "no-cache");
            res.set(http::field::connection, "keep-alive");
            res.set(http::field::server, "ThemisDB-LLM/1.7.0");
            res.body() = std::move(sse_body);
            res.prepare_payload();
            return res;

        } else {
            // ── Non-streaming path ────────────────────────────────────────
            spdlog::info(
                "LLMApiHandler::handleOpenAIChatCompletions non-stream start: model='{}' prompt_len={} request_max_tokens={}",
                model_id.empty() ? std::string{"default"} : model_id,
                llm_request.prompt.size(),
                llm_request.max_tokens);

            llm::InferenceResponse llm_response;
            try {
                auto& plugin_mgr = llm::LLMPluginManager::instance();
                llm_response = plugin_mgr.generate(llm_request);
            } catch (const std::exception& e) {
                spdlog::warn(
                    "LLMApiHandler::handleOpenAIChatCompletions non-stream failed: model='{}' error='{}'",
                    model_id.empty() ? std::string{"default"} : model_id,
                    e.what());
                auto err = llm::OpenAICompatAdapter::buildError(
                    std::string{"Inference failed: "} + e.what(),
                    "server_error");
                return createJsonResponse(err, http::status::internal_server_error);
            } catch (...) {
                spdlog::warn(
                    "LLMApiHandler::handleOpenAIChatCompletions non-stream failed with unknown error: model='{}'",
                    model_id.empty() ? std::string{"default"} : model_id);
                logCurrentException([[maybe_unused]] "LLMApiHandler::handleOpenAIChatCompletions non-streaming");
                auto err = llm::OpenAICompatAdapter::buildError("Inference failed", "server_error");
                return createJsonResponse(err, http::status::internal_server_error);
            }

            json response_json = llm::OpenAICompatAdapter::buildResponse(
                llm_response, model_id, completion_id);

            spdlog::info(
                "LLMApiHandler::handleOpenAIChatCompletions non-stream complete: model='{}' tokens_generated={} inference_time_ms={:.2f}",
                model_id.empty() ? std::string{"default"} : model_id,
                llm_response.tokens_generated,
                llm_response.inference_time_ms);

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::server, "ThemisDB-LLM/1.7.0");
            res.body() = response_json.dump();
            res.prepare_payload();
            return res;
        }
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "llm_api_handler: unhandled exception caught");
        logCurrentException([[maybe_unused]] "LLMApiHandler::handleOpenAIChatCompletions failed");
        auto err = llm::OpenAICompatAdapter::buildError(
            "Failed to handle chat completions request", "server_error");
        return createJsonResponse(err, http::status::internal_server_error);
    }
}

http::response<http::string_body> LLMApiHandler::handleOpenAIListModels(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleOpenAIListModels");

    // Return a basic OpenAI-compatible model list using registered plugins
    json models_arr = json::array();

    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto model_ids = plugin_mgr.listModels();

        for (const auto& mid : model_ids) {
            models_arr.push_back(json{
                {"id",       mid},
                {"object",   "model"},
                {"created",  0},
                {"owned_by", "themisdb"}
            });
        }
    } catch (const std::exception& e) {
        // If listing fails, return an empty list rather than an error.
        // Log at WARN level so model enumeration failures can be diagnosed.
        THEMIS_WARN("LLMApiHandler: handleOpenAIListModels: exception while listing models — returning empty list: {}", e.what());
    } catch (...) {
        // Keep empty-list fallback behavior even for non-standard exceptions.
        THEMIS_WARN([[maybe_unused]] "LLMApiHandler: handleOpenAIListModels: non-standard exception while listing models — returning empty list");
    }

    json response_json{
        {"object", "list"},
        {"data",   std::move(models_arr)}
    };

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB-LLM/1.7.0");
    res.body() = response_json.dump();
    res.prepare_payload();
    return res;
}

} // namespace themis::server

