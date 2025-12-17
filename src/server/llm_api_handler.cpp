#include "server/llm_api_handler.h"
#include "llm/llm_plugin_manager.h"
#include "llm/llm_plugin_interface.h"
#include "llm/async_inference_engine.h"
#include <boost/json/src.hpp>
#include <sstream>
#include <regex>

namespace themis::server {

namespace {
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
            prompt = json::value_to<std::string>(body->at("prompt"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'prompt' field");
        }
        
        if (body->contains("model")) {
            model_id = json::value_to<std::string>(body->at("model"));
        }
        
        if (body->contains("lora_adapter")) {
            lora_id = json::value_to<std::string>(body->at("lora_adapter"));
        }
        
        if (body->contains("max_tokens")) {
            max_tokens = json::value_to<int>(body->at("max_tokens"));
        }
        
        if (body->contains("temperature")) {
            temperature = json::value_to<double>(body->at("temperature"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid request parameters", e.what());
    }
    
    // TODO: Call LLMPluginManager for inference
    // For now, return stub response
    json::object response_data = {
        {"text", "TODO: Implement actual inference call to LLMPluginManager"},
        {"model", model_id.empty() ? "default" : model_id},
        {"tokens_generated", 42},
        {"inference_time_ms", 150},
        {"cache_hit", false}
    };
    
    return createJsonResponse(response_data);
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
            query = json::value_to<std::string>(body->at("query"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'query' field");
        }
        
        if (body->contains("collection")) {
            collection = json::value_to<std::string>(body->at("collection"));
        }
        
        if (body->contains("top_k")) {
            top_k = json::value_to<int>(body->at("top_k"));
        }
        
        if (body->contains("lora_adapter")) {
            lora_id = json::value_to<std::string>(body->at("lora_adapter"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid RAG parameters", e.what());
    }
    
    // TODO: Implement RAG workflow
    // 1. Vector search in collection
    // 2. Assemble RAG context
    // 3. Call LLMPluginManager.generateRAG()
    
    json::object response_data = {
        {"text", "TODO: Implement RAG workflow with vector search"},
        {"query", query},
        {"documents_retrieved", top_k},
        {"inference_time_ms", 180}
    };
    
    return createJsonResponse(response_data);
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
            text = json::value_to<std::string>(body->at("text"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'text' field");
        }
        
        if (body->contains("model")) {
            model_id = json::value_to<std::string>(body->at("model"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid embed parameters", e.what());
    }
    
    // TODO: Generate embeddings via LLMPluginManager
    json::array embedding_vector;
    for (int i = 0; i < 4096; ++i) {
        embedding_vector.push_back(0.0);  // Stub: zeros
    }
    
    json::object response_data = {
        {"embedding", embedding_vector},
        {"model", model_id.empty() ? "default" : model_id},
        {"dimensions", 4096}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleStreamInference(
    const http::request<http::string_body>& req) {
    
    // TODO: Implement Server-Sent Events (SSE) streaming
    // For now, return not implemented
    return createErrorResponse(
        http::status::not_implemented,
        "Streaming not yet implemented",
        "SSE streaming will be available in Phase 3.2"
    );
}

http::response<http::string_body> LLMApiHandler::handleListModels(
    const http::request<http::string_body>& req) {
    
    // TODO: Get model list from LLMPluginManager
    json::array models;
    json::object model1 = {
        {"model_id", "mistral-7b"},
        {"status", "loaded"},
        {"size_mb", 6400},
        {"loaded_at", "2025-12-17T09:00:00Z"}
    };
    models.push_back(model1);
    
    json::object response_data = {
        {"models", models},
        {"total", 1}
    };
    
    return createJsonResponse(response_data);
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
            model_id = json::value_to<std::string>(body->at("model_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
        }
        
        if (body->contains("path")) {
            path = json::value_to<std::string>(body->at("path"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid load model parameters", e.what());
    }
    
    // TODO: Call LazyModelLoader to load model
    json::object response_data = {
        {"model_id", model_id},
        {"status", "loading"},
        {"message", "Model load initiated"}
    };
    
    return createJsonResponse(response_data, http::status::accepted);
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
            model_id = json::value_to<std::string>(body->at("model_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'model_id' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid unload model parameters", e.what());
    }
    
    // TODO: Call LazyModelLoader to unload model
    json::object response_data = {
        {"model_id", model_id},
        {"status", "unloaded"},
        {"message", "Model unloaded successfully"}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleModelInfo(
    const http::request<http::string_body>& req) {
    
    // Extract model_id from path
    std::string_view target = req.target();
    std::string model_id = std::string(target.substr(target.find_last_of('/') + 1));
    
    // TODO: Get model info from LLMPluginManager
    json::object response_data = {
        {"model_id", model_id},
        {"status", "loaded"},
        {"size_mb", 6400},
        {"format", "GGUF"},
        {"quantization", "Q4_K_M"},
        {"context_length", 4096},
        {"loaded_at", "2025-12-17T09:00:00Z"}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleIngestModel(
    const http::request<http::string_body>& req) {
    
    // TODO: Implement model ingestion with chunked upload to RocksDB Blob Store
    // This requires multipart/form-data parsing and streaming upload
    return createErrorResponse(
        http::status::not_implemented,
        "Model ingestion not yet implemented",
        "Will be available in Phase 3.2"
    );
}

http::response<http::string_body> LLMApiHandler::handleListLoRAs(
    const http::request<http::string_body>& req) {
    
    // TODO: Get LoRA list from MultiLoRAManager
    json::array loras;
    json::object lora1 = {
        {"lora_id", "legal-qa"},
        {"status", "loaded"},
        {"base_model", "mistral-7b"},
        {"size_mb", 20}
    };
    loras.push_back(lora1);
    
    json::object response_data = {
        {"loras", loras},
        {"total", 1}
    };
    
    return createJsonResponse(response_data);
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
            lora_id = json::value_to<std::string>(body->at("lora_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'lora_id' field");
        }
        
        if (body->contains("path")) {
            path = json::value_to<std::string>(body->at("path"));
        }
        
        if (body->contains("base_model")) {
            base_model = json::value_to<std::string>(body->at("base_model"));
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid load LoRA parameters", e.what());
    }
    
    // TODO: Call MultiLoRAManager to load LoRA
    json::object response_data = {
        {"lora_id", lora_id},
        {"base_model", base_model},
        {"status", "loaded"},
        {"message", "LoRA loaded successfully"}
    };
    
    return createJsonResponse(response_data);
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
            lora_id = json::value_to<std::string>(body->at("lora_id"));
        } else {
            return createErrorResponse(http::status::bad_request, "Missing 'lora_id' field");
        }
    } catch (const std::exception& e) {
        return createErrorResponse(http::status::bad_request, "Invalid unload LoRA parameters", e.what());
    }
    
    // TODO: Call MultiLoRAManager to unload LoRA
    json::object response_data = {
        {"lora_id", lora_id},
        {"status", "unloaded"},
        {"message", "LoRA unloaded successfully"}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleStats(
    const http::request<http::string_body>& req) {
    
    // TODO: Get statistics from AsyncInferenceEngine and LLMPluginManager
    json::object response_data = {
        {"throughput_req_per_sec", 128.0},
        {"average_latency_ms", 28.0},
        {"cache_hit_rate", 0.85},
        {"total_requests", 10523},
        {"active_workers", 4},
        {"queue_depth", 12}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleCacheStats(
    const http::request<http::string_body>& req) {
    
    // TODO: Get cache statistics from LLMResponseCache and LLMPrefixCache
    json::object response_cache = {
        {"hits", 8945},
        {"misses", 1578},
        {"hit_rate", 0.85},
        {"total_entries", 3456}
    };
    
    json::object prefix_cache = {
        {"hits", 6823},
        {"misses", 3700},
        {"hit_rate", 0.65},
        {"total_prefixes", 1234}
    };
    
    json::object response_data = {
        {"response_cache", response_cache},
        {"prefix_cache", prefix_cache}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleClearCache(
    const http::request<http::string_body>& req) {
    
    // TODO: Clear LLMResponseCache and LLMPrefixCache
    json::object response_data = {
        {"status", "cleared"},
        {"message", "All LLM caches cleared successfully"}
    };
    
    return createJsonResponse(response_data);
}

http::response<http::string_body> LLMApiHandler::handleHealth(
    const http::request<http::string_body>& req) {
    
    // TODO: Check health of LLMPluginManager and AsyncInferenceEngine
    json::object response_data = {
        {"status", "healthy"},
        {"plugin_manager", "operational"},
        {"async_engine", "operational"},
        {"models_loaded", 1},
        {"loras_loaded", 1}
    };
    
    return createJsonResponse(response_data);
}

bool LLMApiHandler::validateBearerToken(const http::request<http::string_body>& req) {
    auto token = extractBearerToken(req);
    if (!token) {
        return false;
    }
    
    // TODO: Implement actual JWT validation
    // For Phase 3.1, we accept any non-empty token
    // Real validation should:
    // 1. Parse JWT
    // 2. Verify signature
    // 3. Check expiration
    // 4. Validate claims
    
    return !token->empty();
}

http::response<http::string_body> LLMApiHandler::createErrorResponse(
    http::status status,
    std::string_view error,
    std::string_view details) {
    
    json::object error_obj = {
        {"error", std::string(error)}
    };
    
    if (!details.empty()) {
        error_obj["details"] = std::string(details);
    }
    
    error_obj["status"] = static_cast<int>(status);
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB-LLM/1.3.0");
    res.body() = json::serialize(error_obj);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> LLMApiHandler::createJsonResponse(
    const json::object& data,
    http::status status) {
    
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB-LLM/1.3.0");
    res.body() = json::serialize(data);
    res.prepare_payload();
    return res;
}

std::optional<json::object> LLMApiHandler::parseRequestBody(
    const http::request<http::string_body>& req) {
    
    try {
        auto parsed = json::parse(req.body());
        if (parsed.is_object()) {
            return parsed.as_object();
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }
    
    return std::nullopt;
}

} // namespace themis::server
