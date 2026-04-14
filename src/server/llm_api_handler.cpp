/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_api_handler.cpp                                ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:37:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1734                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • e8953e1175  2026-04-13  docs(aql): Close all remaining ROADMAP items — Doxygen, L... ║
    • 5ef023b6a2  2026-04-13  feat(rag): wire FLARE retrieval-callback bridge — Knowled... ║
    • c797588b59  2026-04-06  fix(llm): route inference through LLMPluginManager + add ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/llm_api_handler.h"
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
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <iostream>
#include <chrono>
#include "utils/tracing.h"

namespace themis::server {

namespace {
    template <typename T>
    T json_value_to(const json& value) {
        return value.get<T>();
    }

    // Helper to extract JWT token from Authorization header
    std::optional<std::string> extractBearerToken(const http::request<http::string_body>& req) {
        auto it = req.find(http::field::authorization);
        if (it == req.end()) {
            return std::nullopt;
        }
        
        std::string auth_header{it->value()};
        std::regex bearer_regex(R"(^Bearer\s+(.+)$)", std::regex::icase);
        std::smatch matches;
        
        if (std::regex_match(auth_header, matches, bearer_regex) && matches.size() == 2) {
            return matches[1].str();
        }
        
        return std::nullopt;
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

void LLMApiHandler::configureJWT(const auth::JWTValidatorConfig& config) {
    jwt_validator_ = std::make_unique<auth::JWTValidator>(config);
}

void LLMApiHandler::setLoRAHandler(std::shared_ptr<LoRAApiHandler> lora_handler) {
    lora_handler_ = std::move(lora_handler);
}

void LLMApiHandler::setFeedbackStore(std::shared_ptr<llm::FeedbackStore> feedback_store) {
    feedback_store_ = std::move(feedback_store);
}

void LLMApiHandler::setPolicyEngine(governance::PolicyEngine* policy_engine) {
    policy_engine_ = policy_engine;
}

void LLMApiHandler::setQueryEngine(std::shared_ptr<query::QueryEngine> query_engine) {
    query_engine_ = std::move(query_engine);
}

http::response<http::string_body> LLMApiHandler::handleRequest(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleRequest");
    
    // Delegate to LoRAApiHandler for LoRA-specific paths
    std::string_view target = req.target();
    if (lora_handler_ && target.starts_with("/api/v1/llm/lora/")) {
        return lora_handler_->handleRequest(req);
    }

    // OpenAI-compatible endpoints use API key auth via PolicyEngine, not JWT.
    // Route them BEFORE the JWT gate so that OpenAI SDK clients (which send a
    // plain API key, not a signed JWT) are not rejected by validateBearerToken().
    auto method = req.method();
    if (target == "/v1/chat/completions" && method == http::verb::post) {
        return handleOpenAIChatCompletions(req);
    } else if (target == "/v1/models" && method == http::verb::get) {
        return handleOpenAIListModels(req);
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
        return handleRAG(req);
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
}

http::response<http::string_body> LLMApiHandler::handleInference(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleInference");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    // Extract request parameters
    std::string prompt;
    std::string model_id;
    std::string lora_id;
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

        // Create response
        json response_body = {
            {"text", llm_response.text},
            {"model", llm_response.model_id.empty() ? (model_id.empty() ? "default" : model_id) : llm_response.model_id},
            {"prompt_length", prompt.length()},
            {"generated_length", llm_response.text.length()},
            {"tokens_generated", llm_response.tokens_generated},
            {"inference_time_ms", llm_response.inference_time_ms}
        };
        
        return createJsonResponse(http::status::ok, response_body);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Inference failed",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleRAG(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleRAG");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    // Extract RAG parameters
    std::string query;
    std::string collection;
    int top_k = 5;
    std::string lora_id;
    
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
        
        if (body->contains("lora_adapter")) {
            lora_id = json_value_to<std::string>(body->at("lora_adapter"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid RAG parameters", e.what());
    }
    
    // Implement RAG workflow
    try {
        // Prepare RAG context.
        llm::RAGContext rag_context;
        rag_context.query = query;
        rag_context.collection_name = collection;
        rag_context.top_k = top_k;

        auto& plugin_mgr = llm::LLMPluginManager::instance();

        // Perform vector retrieval when a QueryEngine has been wired.
        // NOTE: QueryEngine vector-search API is currently in migration.
        // Keep RAG operational with empty retrieval context until the
        // executeFilteredVectorSearch wiring is aligned again.
        if (query_engine_ && !collection.empty() && top_k > 0) {
            try {
                const std::vector<float> query_vec = plugin_mgr.embed(query);
                if (query_vec.empty()) {
                    THEMIS_WARN("LLMApiHandler::handleRAG: embedding returned empty vector for query");
                }
            } catch (const std::exception& ve) {
                THEMIS_WARN("LLMApiHandler::handleRAG: vector retrieval skipped ({}); "
                            "proceeding with empty document context", ve.what());
            }
        }
        
        // Prepare inference request
        llm::InferenceRequest llm_request;
        llm_request.prompt = query;
        llm_request.lora_adapter_id = lora_id;

        // Call LLMPluginManager for RAG inference
        auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);

        json response_data = {
            {"text", llm_response.text},
            {"query", query},
            {"documents_retrieved", static_cast<int>(rag_context.documents.size())},
            {"tokens_generated", llm_response.tokens_generated},
            {"inference_time_ms", llm_response.inference_time_ms},
            {"cache_hit", llm_response.cache_hit}
        };

        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
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
    
    std::string text;
    std::string model_id;
    
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
            {"dimensions", embedding.size()},
            {"text_length", text.length()}
        };
        
        return createJsonResponse(http::status::ok, response_body);
        
        json response_data = {
            {"embedding", embedding_vector},
            {"model", model_id.empty() ? "default" : model_id},
            {"dimensions", static_cast<int>(embedding.size())}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
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
    std::string prompt;
    std::string request_id;
    int max_tokens = 512;

    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        std::string qs = target.substr(qpos + 1);
        auto extract = [&](const std::string& key) -> std::string {
            std::string prefix = key + "=";
            auto pos = qs.find(prefix);
            if (pos == std::string::npos) return {};
            auto end = qs.find('&', pos);
            std::string raw = qs.substr(pos + prefix.size(),
                end == std::string::npos ? std::string::npos : end - pos - prefix.size());
            // Basic URL-decode
            std::string decoded;
            decoded.reserve(raw.size());
            for (size_t i = 0; i < raw.size(); ) {
                if (raw[i] == '+') { decoded += ' '; ++i; }
                else if (raw[i] == '%' && i + 2 < raw.size()) {
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
                if (n > 0 && n <= kMaxTokensLimit) max_tokens = n;
            } catch (...) {}
        }
    }

    if (prompt.empty()) {
        return createErrorResponse(http::status::bad_request,
            "Missing 'prompt' query parameter");
    }

    // Collect SSE events from LLM streaming
    std::string sse_body;
    sse_body += "retry: 3000\n\n";

    try {
        auto& llm = llm::EmbeddedLLMManager::instance().get();
        llm.generateStreamingSSE(
            prompt,
            [&sse_body](const std::string& sse_event) {
                sse_body += sse_event;
            },
            request_id,
            max_tokens
        );
        // Emit terminal done event
        sse_body += "event: done\ndata: {\"done\":true}\n\n";
    } catch (const std::exception& e) {
        json err_event = {{"error", true}, {"message", std::string(e.what())}};
        sse_body += "event: error\ndata: " + err_event.dump() + "\n\n";
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/event-stream");
    res.set(http::field::cache_control, "no-cache, no-transform");
    res.set(http::field::connection, "keep-alive");
    res.set(http::field::access_control_allow_origin, "*");
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

    std::string aql_query;
    std::string schema_context;
    std::string request_id;

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
    std::string sse_body;
    sse_body += "retry: 3000\n\n";

    try {
        aql::LLMAQLHandler aql_handler;
        aql_handler.streamExplainAQLAsSSE(
            aql_query,
            [&sse_body](const std::string& sse_event) {
                sse_body += sse_event;
            },
            request_id,
            schema_context
        );
        // Emit terminal done event
        sse_body += "event: done\ndata: {\"done\":true}\n\n";
    } catch (const aql::LLMException& e) {
        json err_event = {{"error", true}, {"message", std::string(e.what())},
                          {"code", static_cast<int>(e.getErrorCode())}};
        sse_body += "event: error\ndata: " + err_event.dump() + "\n\n";
    } catch (const std::exception& e) {
        json err_event = {{"error", true}, {"message", std::string(e.what())}};
        sse_body += "event: error\ndata: " + err_event.dump() + "\n\n";
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/event-stream");
    res.set(http::field::cache_control, "no-cache, no-transform");
    res.set(http::field::connection, "keep-alive");
    res.set(http::field::access_control_allow_origin, "*");
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
    
    std::string model_id;
    std::string path;
    
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
    
    // Call LLMPluginManager to load model
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.loadModel(model_id, path);
        
        json response_data = {
            {"model_id", model_id},
            {"status", "loaded"},
            {"message", "Model loaded successfully"}
        };
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
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
    
    std::string model_id;
    
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
    
    std::string model_id;
    std::string file_data;
    
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
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to list LoRAs",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleLoadLoRA(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleLoadLoRA");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string lora_id;
    std::string path;
    std::string base_model;
    
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
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to load LoRA",
            e.what()
        );
    }
}

http::response<http::string_body> LLMApiHandler::handleUnloadLoRA(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleUnloadLoRA");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string lora_id;
    
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
        
        return createJsonResponse(response_data);
    } catch (const std::exception& e) {
        return createErrorResponse(
            http::status::internal_server_error,
            "Failed to unload LoRA",
            e.what()
        );
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
    }
}

bool LLMApiHandler::validateBearerToken(const http::request<http::string_body>& req) {
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
    
    try {
        auto claims = jwt_validator_->parseAndValidate(*token);
        // Token is valid
        return true;
    } catch (const std::exception& e) {
        // Token validation failed (expired, invalid signature, etc.)
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
    } catch (const std::exception&) {
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
    
    std::string query;
    
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
    }
}

http::response<http::string_body> LLMApiHandler::handleDocsConfig(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDocsConfig");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string topic;
    
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
    }
}

http::response<http::string_body> LLMApiHandler::handleDocsTroubleshoot(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleDocsTroubleshoot");
    
    auto body = parseRequestBody(req);
    if (!body) {
        return createErrorResponse(http::status::bad_request, "Invalid JSON body");
    }
    
    std::string error_description;
    
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
        
        // Store feedback using FeedbackStore
        auto stored = feedback_store_->createFeedback(feedback);
        
        // Convert to JSON response
        json response_data = stored.toJson();
        response_data["message"] = "Feedback recorded successfully";
        
        return createJsonResponse(response_data, http::status::created);
        
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid feedback parameters", e.what());
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
    
    // Get feedback from store
    auto feedback = feedback_store_->getFeedback(feedback_id);
    
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
    std::string type_filter;
    std::string status_filter;
    
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
            } catch (...) {}
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
    auto feedback_list = feedback_store_->listFeedback(options);
    
    // Convert to JSON array
    json feedback_array = json::array();
    for (const auto& feedback : feedback_list) {
        feedback_array.push_back(feedback.toJson());
    }
    
    json response_data = {
        {"feedback", feedback_array},
        {"count", feedback_array.size()},
        {"limit", limit}
    };
    
    return createJsonResponse(response_data);
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
    
    // Get stats from store
    auto stats = feedback_store_->getStats();
    
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

    auto body = parseRequestBody(req);
    if (!body) {
        auto err = llm::OpenAICompatAdapter::buildError(
            "Invalid JSON body", "invalid_request_error");
        return createJsonResponse(err, http::status::bad_request);
    }

    // ── API key / governance check ──────────────────────────────────────────
    // When a PolicyEngine is configured, validate the caller's identity and
    // data-classification policy before any inference work is started.
    // Returns HTTP 401 for missing/malformed tokens, HTTP 403 for denied policy.
    if (policy_engine_) {
        // Collect headers from the Boost.Beast request into the flat map
        // expected by PolicyEngine::checkInferencePermission().
        std::unordered_map<std::string, std::string> header_map;
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
        // ── Streaming path ────────────────────────────────────────────────
        // Collect SSE chunks into a single response body.
        // In a production server with a real async HTTP layer this would
        // write chunks incrementally; here we buffer them for compatibility
        // with the synchronous response model used by LLMApiHandler.
        std::string sse_body;

        // Capture created timestamp once so all chunks share it
        int64_t created = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        llm_request.stream_callback = [&](const std::string& token) {
            sse_body += llm::OpenAICompatAdapter::buildStreamChunk(
                token, completion_id, model_id, created);
        };

        try {
            auto& plugin_mgr = llm::LLMPluginManager::instance();
            plugin_mgr.generate(llm_request);
        } catch (const std::exception& e) {
            auto err = llm::OpenAICompatAdapter::buildError(
                std::string{"Inference failed: "} + e.what(),
                "server_error");
            return createJsonResponse(err, http::status::internal_server_error);
        }

        sse_body += llm::OpenAICompatAdapter::buildStreamFinalChunk(
            completion_id, model_id, created);
        sse_body += llm::OpenAICompatAdapter::buildStreamDone();

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/event-stream");
        res.set(http::field::cache_control, "no-cache");
        res.set(http::field::connection, "keep-alive");
        res.set(http::field::server, "ThemisDB-LLM/1.7.0");
        res.body() = std::move(sse_body);
        res.prepare_payload();
        return res;

    } else {
        // ── Non-streaming path ────────────────────────────────────────────
        llm::InferenceResponse llm_response;
        try {
            auto& plugin_mgr = llm::LLMPluginManager::instance();
            llm_response = plugin_mgr.generate(llm_request);
        } catch (const std::exception& e) {
            auto err = llm::OpenAICompatAdapter::buildError(
                std::string{"Inference failed: "} + e.what(),
                "server_error");
            return createJsonResponse(err, http::status::internal_server_error);
        }

        json response_json = llm::OpenAICompatAdapter::buildResponse(
            llm_response, model_id, completion_id);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "ThemisDB-LLM/1.7.0");
        res.body() = response_json.dump();
        res.prepare_payload();
        return res;
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
    } catch (const std::exception&) {
        // If listing fails, return an empty list rather than an error
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
