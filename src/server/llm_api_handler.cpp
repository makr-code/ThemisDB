#include "server/llm_api_handler.h"
#include "llm/llm_plugin_manager.h"
#include "llm/llm_plugin_interface.h"
#include "llm/async_inference_engine.h"
#include "llm/embedded_llm.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <iostream>

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

LLMApiHandler::LLMApiHandler(std::shared_ptr<llm::LLMPluginManager> plugin_manager)
    : plugin_manager_(std::move(plugin_manager)) {
}

http::response<http::string_body> LLMApiHandler::handleRequest(
    const http::request<http::string_body>& req) {
    
    // Validate Bearer Token (JWT) authentication
    if (!validateBearerToken(req)) {
        return createErrorResponse(
            http::status::unauthorized,
            "Unauthorized",
            "Valid Bearer Token required. Include 'Authorization: Bearer <token>' header."
        );
    }
    
    std::string_view target = req.target();
    auto method = req.method();
    
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
    }
    
    return createErrorResponse(
        http::status::not_found,
        "Not Found",
        "LLM API endpoint not found"
    );
}

http::response<http::string_body> LLMApiHandler::handleInference(
    const http::request<http::string_body>& req) {
    
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
    
    // Call EmbeddedLLM for inference
    try {
        // Use simplified EmbeddedLLM API
        std::string result = THEMIS_LLM_GENERATE(prompt);
        
        // Create response
        json response_body = {
            {"text", result},
            {"model", model_id.empty() ? "default" : model_id},
            {"prompt_length", prompt.length()},
            {"generated_length", result.length()}
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
        // Prepare RAG context (vector search would happen here in production)
        llm::RAGContext rag_context;
        rag_context.query = query;
        rag_context.collection_name = collection;
        rag_context.top_k = top_k;
        // TODO: Add actual vector search results to rag_context.documents
        
        // Prepare inference request
        llm::InferenceRequest llm_request;
        llm_request.prompt = query;
        llm_request.lora_adapter_id = lora_id;
        
        // Call LLMPluginManager for RAG inference
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);
        
        json response_data = {
            {"text", llm_response.text},
            {"query", query},
            {"documents_retrieved", top_k},
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

http::response<http::string_body> LLMApiHandler::handleStreamInference(
    const http::request<http::string_body>& req) {
    (void)req;
    return createErrorResponse(http::status::not_implemented, "Streaming not supported in this build");
}

http::response<http::string_body> LLMApiHandler::handleListModels(
    const http::request<http::string_body>& req) {
    
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
    const http::request<http::string_body>& req) {
    
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
    const http::request<http::string_body>& req) {
    
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
    const http::request<http::string_body>& req) {
    
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
    const http::request<http::string_body>& req) {
    
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
    const http::request<http::string_body>& req) {
    
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
    
    // TODO: Implement actual JWT validation
    // SECURITY WARNING: This is a placeholder implementation for development only.
    // DO NOT USE IN PRODUCTION without implementing proper JWT validation:
    // 1. Parse JWT structure (header.payload.signature)
    // 2. Verify signature using public key/secret
    // 3. Check expiration (exp claim)
    // 4. Validate issuer (iss claim)
    // 5. Validate audience (aud claim)
    // 6. Check not-before time (nbf claim)
    // Consider using a JWT library like jwt-cpp or libjwt for production.
    
    // For now, reject empty tokens as a minimal safety check
    if (token->empty()) {
        return false;
    }
    
    // Development-only: log warning about placeholder validation
    static bool warning_logged = false;
    if (!warning_logged) {
        std::cerr << "WARNING: Using placeholder JWT validation. "
                  << "Implement proper JWT verification before production use." << std::endl;
        warning_logged = true;
    }
    
    return true;
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

} // namespace themis::server
