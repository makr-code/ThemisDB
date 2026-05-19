/*
 * ThemisDB | File: rope_api_handler.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 913
 * Open Issues: TODOs=1, Stubs=5, Gaps=9, Unimpl=0, Mock=1, Sim=2, Debt=0
 * Gap Correlation: internal=9 | external_v3=221 | delta=212 | status=divergent
 * External Severity (v3): C=3, H=132, M=86
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/rope_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/rotary_embeddings.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <chrono>

namespace themis {
namespace server {

using json = nlohmann::json;

RopeApiHandler::RopeApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<::themis::AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , vector_index_(std::move(vector_index))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> RopeApiHandler::handleConfigPost(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "vector:write", "rope.config", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeConfigPost");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/config", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        auto body_json = json::parse(req.body());
        
        // Validate required fields
        if (!body_json.contains("hidden_dim") || !body_json["hidden_dim"].is_number_integer()) {
            span.setStatus(false, "Missing or invalid hidden_dim");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: hidden_dim (must be integer)", req);
        }
        
        if (!body_json.contains("num_rotation_pairs") || !body_json["num_rotation_pairs"].is_number_integer()) {
            span.setStatus(false, "Missing or invalid num_rotation_pairs");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: num_rotation_pairs (must be integer)", req);
        }
        
        // Parse configuration
        RotationConfig config;
        config.hidden_dim = body_json["hidden_dim"].get<size_t>();
        config.num_rotation_pairs = body_json["num_rotation_pairs"].get<size_t>();
        config.base_theta = body_json.value("base_theta", 10000.0);
        config.normalize_after = body_json.value("normalize_after", false);
        
        span.setAttribute("rope.hidden_dim", static_cast<int64_t>(config.hidden_dim));
        span.setAttribute("rope.num_rotation_pairs", static_cast<int64_t>(config.num_rotation_pairs));
        
        // Validate configuration
        if (!config.isValid()) {
            span.setStatus(false, "Invalid configuration");
            std::string error_msg = "Invalid RoPE configuration: ";
            if (config.hidden_dim == 0 || config.hidden_dim % 2 != 0) {
                error_msg += "hidden_dim must be a positive even number";
            } else if (config.num_rotation_pairs == 0) {
                error_msg += "num_rotation_pairs must be greater than 0";
            } else if (config.num_rotation_pairs > config.hidden_dim / 2) {
                error_msg += "num_rotation_pairs must be <= hidden_dim/2";
            }
            return makeErrorResponse(http::status::bad_request, error_msg, req);
        }
        
        // Compute theta cache
        config.computeThetaCache();
        
        // Apply configuration to vector index
        auto status = vector_index_->setRotaryEmbeddingConfig(config);
        
        if (!status.ok) {
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to apply RoPE configuration: " + status.message, req);
        }
        
        // Build response
        json response = {
            {"status", "success"},
            {"message", "RoPE configuration enabled for index '" + index_name + "'"},
            {"config", {
                {"hidden_dim", config.hidden_dim},
                {"num_rotation_pairs", config.num_rotation_pairs},
                {"base_theta", config.base_theta},
                {"normalize_after", config.normalize_after},
                {"theta_cache_size", config.theta_cache.size()}
            }}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE config error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleConfigGet(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "vector:read", "rope.config", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeConfigGet");
    span.setAttribute("http.method", "GET");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/config", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        // Check if RoPE is enabled
        if (!vector_index_->isRotaryEmbeddingEnabled()) {
            span.setStatus(false, "RoPE not enabled");
            return makeErrorResponse(http::status::not_found,
                "RoPE is not enabled for index '" + index_name + "'", req);
        }
        
        // Get configuration
        auto config_opt = vector_index_->getRotaryEmbeddingConfig();
        if (!config_opt) {
            span.setStatus(false, "Failed to get config");
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to retrieve RoPE configuration", req);
        }
        
        const auto& config = *config_opt;
        
        // Build response
        json response = {
            {"enabled", true},
            {"config", {
                {"hidden_dim", config.hidden_dim},
                {"num_rotation_pairs", config.num_rotation_pairs},
                {"base_theta", config.base_theta},
                {"normalize_after", config.normalize_after},
                {"theta_cache_size", config.theta_cache.size()}
            }}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE config get error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleConfigDelete(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "vector:write", "rope.config", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeConfigDelete");
    span.setAttribute("http.method", "DELETE");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/config", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        // Check if RoPE is currently enabled
        if (!vector_index_->isRotaryEmbeddingEnabled()) {
            span.setStatus(false, "RoPE not enabled");
            return makeErrorResponse(http::status::not_found,
                "RoPE is not enabled for index '" + index_name + "'", req);
        }
        
        // Note: VectorIndexManager does not currently provide a disable method.
        // This endpoint returns success to indicate intent, but RoPE remains configured.
        // Future enhancement: Add disableRotaryEmbedding() method to VectorIndexManager
        // to fully support disabling RoPE at runtime.
        
        json response = {
            {"status", "success"},
            {"message", "RoPE disable requested for index '" + index_name + "'"},
            {"note", "RoPE configuration persists until server restart"}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE config delete error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleAddPost(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "data:write", "rope.add", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeAddPost");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/add", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        // Check if RoPE is enabled
        if (!vector_index_->isRotaryEmbeddingEnabled()) {
            span.setStatus(false, "RoPE not enabled");
            return makeErrorResponse(http::status::bad_request,
                "RoPE is not enabled. Please configure RoPE first.", req);
        }
        
        auto body_json = json::parse(req.body());
        
        // Validate required fields
        if (!body_json.contains("entity") || !body_json["entity"].is_object()) {
            span.setStatus(false, "Missing or invalid entity");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: entity (must be object)", req);
        }
        
        if (!body_json.contains("position") || !body_json["position"].is_number_integer()) {
            span.setStatus(false, "Missing or invalid position");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: position (must be integer)", req);
        }
        
        // Parse entity
        const auto& entity_json = body_json["entity"];
        if (!entity_json.contains("id") || !entity_json["id"].is_string()) {
            span.setStatus(false, "Missing entity id");
            return makeErrorResponse(http::status::bad_request,
                "Entity must have an 'id' field (string)", req);
        }
        
        std::string entity_id = entity_json["id"];
        size_t position = body_json["position"].get<size_t>();
        std::string vector_field = body_json.value("vector_field", "embedding");
        
        span.setAttribute("rope.entity_id", entity_id);
        span.setAttribute("rope.position", static_cast<int64_t>(position));
        
        // Build entity from JSON
        BaseEntity entity(entity_id);
        
        // Add all fields from entity JSON
        for (auto it = entity_json.begin(); it != entity_json.end(); ++it) {
            const std::string& key = it.key();
            const auto& val = it.value();
            
            if (key == "id") continue; // Already set as PK
            
            if (val.is_string()) {
                entity.setField(key, val.get<std::string>());
            } else if (val.is_number_integer()) {
                entity.setField(key, static_cast<int64_t>(val.get<int64_t>()));
            } else if (val.is_number_float()) {
                entity.setField(key, val.get<double>());
            } else if (val.is_boolean()) {
                entity.setField(key, val.get<bool>());
            } else if (val.is_array() && !val.empty() && val[0].is_number()) {
                // Assume numeric array is the embedding vector
                std::vector<float> vec;
                for (const auto& v : val) {
                    if (!v.is_number()) {
                        span.setStatus(false, "Invalid vector element");
                        return makeErrorResponse(http::status::bad_request,
                            "Vector elements must be numbers", req);
                    }
                    vec.push_back(v.get<float>());
                }
                entity.setField(key, vec);
            }
        }
        
        // Add entity with rotation
        auto status = vector_index_->addEntityWithRotation(entity, vector_field, position);
        
        if (!status.ok) {
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to add entity with rotation: " + status.message, req);
        }
        
        json response = {
            {"status", "success"},
            {"message", "Entity added with rotation"},
            {"entity_id", entity_id},
            {"position", position}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE add error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleAddRelationalPost(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "data:write", "rope.add_relational", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeAddRelationalPost");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/add-relational", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        // Check if RoPE is enabled
        if (!vector_index_->isRotaryEmbeddingEnabled()) {
            span.setStatus(false, "RoPE not enabled");
            return makeErrorResponse(http::status::bad_request,
                "RoPE is not enabled. Please configure RoPE first.", req);
        }
        
        auto body_json = json::parse(req.body());
        
        // Validate required fields
        if (!body_json.contains("entity") || !body_json["entity"].is_object()) {
            span.setStatus(false, "Missing or invalid entity");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: entity (must be object)", req);
        }
        
        if (!body_json.contains("relation_type") || !body_json["relation_type"].is_string()) {
            span.setStatus(false, "Missing or invalid relation_type");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: relation_type (must be string)", req);
        }
        
        // Parse entity
        const auto& entity_json = body_json["entity"];
        if (!entity_json.contains("id") || !entity_json["id"].is_string()) {
            span.setStatus(false, "Missing entity id");
            return makeErrorResponse(http::status::bad_request,
                "Entity must have an 'id' field (string)", req);
        }
        
        std::string entity_id = entity_json["id"];
        std::string relation_type = body_json["relation_type"];
        std::string vector_field = body_json.value("vector_field", "embedding");
        
        span.setAttribute("rope.entity_id", entity_id);
        span.setAttribute("rope.relation_type", relation_type);
        
        // Build entity from JSON
        BaseEntity entity(entity_id);
        
        // Add all fields from entity JSON
        for (auto it = entity_json.begin(); it != entity_json.end(); ++it) {
            const std::string& key = it.key();
            const auto& val = it.value();
            
            if (key == "id") continue; // Already set as PK
            
            if (val.is_string()) {
                entity.setField(key, val.get<std::string>());
            } else if (val.is_number_integer()) {
                entity.setField(key, static_cast<int64_t>(val.get<int64_t>()));
            } else if (val.is_number_float()) {
                entity.setField(key, val.get<double>());
            } else if (val.is_boolean()) {
                entity.setField(key, val.get<bool>());
            } else if (val.is_array() && !val.empty() && val[0].is_number()) {
                std::vector<float> vec;
                for (const auto& v : val) {
                    if (!v.is_number()) {
                        span.setStatus(false, "Invalid vector element");
                        return makeErrorResponse(http::status::bad_request,
                            "Vector elements must be numbers", req);
                    }
                    vec.push_back(v.get<float>());
                }
                entity.setField(key, vec);
            }
        }
        
        // Add entity with relational rotation
        auto status = vector_index_->addEntityWithRelationalRotation(entity, vector_field, relation_type);
        
        if (!status.ok) {
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to add entity with relational rotation: " + status.message, req);
        }
        
        json response = {
            {"status", "success"},
            {"message", "Entity added with relational rotation"},
            {"entity_id", entity_id},
            {"relation_type", relation_type}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE add relational error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleSearchPost(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "data:read", "rope.search", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeSearchPost");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/search", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        // Check if RoPE is enabled
        if (!vector_index_->isRotaryEmbeddingEnabled()) {
            span.setStatus(false, "RoPE not enabled");
            return makeErrorResponse(http::status::bad_request,
                "RoPE is not enabled. Please configure RoPE first.", req);
        }
        
        auto body_json = json::parse(req.body());
        
        // Validate required fields
        if (!body_json.contains("query") || !body_json["query"].is_array()) {
            span.setStatus(false, "Missing or invalid query");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: query (must be array)", req);
        }
        
        if (!body_json.contains("position") || !body_json["position"].is_number_integer()) {
            span.setStatus(false, "Missing or invalid position");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: position (must be integer)", req);
        }
        
        // Parse query vector
        std::vector<float> query_vector;
        for (const auto& val : body_json["query"]) {
            if (!val.is_number()) {
                span.setStatus(false, "Invalid query element");
                return makeErrorResponse(http::status::bad_request,
                    "Query vector elements must be numbers", req);
            }
            query_vector.push_back(val.get<float>());
        }
        
        size_t position = body_json["position"].get<size_t>();
        int k = body_json.value("k", 10);
        
        span.setAttribute("rope.position", static_cast<int64_t>(position));
        span.setAttribute("rope.k", static_cast<int64_t>(k));
        span.setAttribute("rope.query_dim", static_cast<int64_t>(query_vector.size()));
        
        // Measure search time
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Perform search with rotation
        auto [status, results] = vector_index_->searchWithRotation(query_vector, k, position);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        if (!status.ok) {
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Search with rotation failed: " + status.message, req);
        }
        
        // Build results array
        json results_array = json::array();
        for (const auto& result : results) {
            results_array.push_back({
                {"id", result.pk},
                {"distance", result.distance},
                {"position", position}  // Note: In real impl, might want to store actual position
            });
        }
        
        json response = {
            {"status", "success"},
            {"results", results_array},
            {"query_time_ms", duration_ms},
            {"rotation_enabled", true},
            {"k", k},
            {"count", results.size()}
        };
        
        span.setAttribute("rope.results_count", static_cast<int64_t>(results.size()));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE search error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleBatchAddPost(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "data:write", "rope.batch_add", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeBatchAddPost");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/batch-add", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        // Check if RoPE is enabled
        if (!vector_index_->isRotaryEmbeddingEnabled()) {
            span.setStatus(false, "RoPE not enabled");
            return makeErrorResponse(http::status::bad_request,
                "RoPE is not enabled. Please configure RoPE first.", req);
        }
        
        auto body_json = json::parse(req.body());
        
        // Validate required fields
        if (!body_json.contains("entities") || !body_json["entities"].is_array()) {
            span.setStatus(false, "Missing or invalid entities");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid required field: entities (must be array)", req);
        }
        
        std::string vector_field = body_json.value("vector_field", "embedding");
        size_t inserted = 0;
        size_t errors = 0;
        
        // Process each entity
        for (const auto& item : body_json["entities"]) {
            try {
                if (!item.contains("entity") || !item["entity"].is_object()) {
                    ++errors;
                    continue;
                }
                
                if (!item.contains("position") || !item["position"].is_number_integer()) {
                    ++errors;
                    continue;
                }
                
                const auto& entity_json = item["entity"];
                if (!entity_json.contains("id") || !entity_json["id"].is_string()) {
                    ++errors;
                    continue;
                }
                
                std::string entity_id = entity_json["id"];
                size_t position = item["position"].get<size_t>();
                
                // Build entity
                BaseEntity entity(entity_id);
                
                // Add all fields
                for (auto it = entity_json.begin(); it != entity_json.end(); ++it) {
                    const std::string& key = it.key();
                    const auto& val = it.value();
                    
                    if (key == "id") continue;
                    
                    if (val.is_string()) {
                        entity.setField(key, val.get<std::string>());
                    } else if (val.is_number_integer()) {
                        entity.setField(key, static_cast<int64_t>(val.get<int64_t>()));
                    } else if (val.is_number_float()) {
                        entity.setField(key, val.get<double>());
                    } else if (val.is_boolean()) {
                        entity.setField(key, val.get<bool>());
                    } else if (val.is_array() && !val.empty() && val[0].is_number()) {
                        std::vector<float> vec;
                        for (const auto& v : val) {
                            if (!v.is_number()) {
                                vec.clear();
                                break;
                            }
                            vec.push_back(v.get<float>());
                        }
                        if (!vec.empty()) {
                            entity.setField(key, vec);
                        }
                    }
                }
                
                // Add entity with rotation
                auto status = vector_index_->addEntityWithRotation(entity, vector_field, position);
                
                if (status.ok) {
                    ++inserted;
                } else {
                    ++errors;
                }
                
            } catch (const std::exception&) {
                ++errors;
            }
        }
        
        json response = {
            {"status", "success"},
            {"message", "Batch add completed"},
            {"inserted", inserted},
            {"errors", errors},
            {"total", body_json["entities"].size()}
        };
        
        span.setAttribute("rope.inserted", static_cast<int64_t>(inserted));
        span.setAttribute("rope.errors", static_cast<int64_t>(errors));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE batch add error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> RopeApiHandler::handleStatsGet(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "vector:read", "rope.stats", path_only)) return *resp;
    }
    
    auto span = Tracer::startSpan("handleRopeStatsGet");
    span.setAttribute("http.method", "GET");
    span.setAttribute("http.path", std::string(req.target()));
    
    try {
        // Extract index_name from path
        auto index_name_opt = extractIndexName(std::string(req.target()));
        if (!index_name_opt) {
            span.setStatus(false, "Invalid path format");
            return makeErrorResponse(http::status::bad_request,
                "Invalid path format. Expected: /api/v1/vector-index/{index_name}/rope/stats", req);
        }
        std::string index_name = *index_name_opt;
        span.setAttribute("rope.index_name", index_name);
        
        bool enabled = vector_index_->isRotaryEmbeddingEnabled();
        
        json response = {
            {"enabled", enabled}
        };
        
        if (enabled) {
            auto config_opt = vector_index_->getRotaryEmbeddingConfig();
            if (config_opt) {
                const auto& config = *config_opt;
                response["config"] = {
                    {"hidden_dim", config.hidden_dim},
                    {"num_rotation_pairs", config.num_rotation_pairs},
                    {"base_theta", config.base_theta},
                    {"normalize_after", config.normalize_after}
                };
            }
            
            const auto stats = vector_index_->getRotaryEmbeddingStats();
            response["statistics"] = {
                {"total_rotated_entities", stats.total_rotations},
                {"avg_rotation_time_us", stats.avg_rotation_time_us},
                {"positional_rotations", stats.positional_rotations},
                {"relational_rotations", stats.relational_rotations},
                {"query_rotations", stats.query_rotations}
            };
        }
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("RoPE stats error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// Helper methods

http::response<http::string_body> RopeApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> RopeApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

std::optional<http::response<http::string_body>> RopeApiHandler::requireAccess(
    [[maybe_unused]] const http::request<http::string_body>& req,
    const std::string& permission,
    [[maybe_unused]] const std::string& resource,
    [[maybe_unused]] const std::string& path)
{
    if (!auth_ || !auth_->isEnabled()) {
        return std::nullopt; // Open mode — allow all
    }

    auto auth_hdr = req.find(http::field::authorization);
    if (auth_hdr == req.end()) {
        return makeErrorResponse(http::status::unauthorized,
                                 "Authentication required", req);
    }

    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_hdr->value().data(), auth_hdr->value().size()));
    if (!token) {
        return makeErrorResponse(http::status::unauthorized,
                                 "Invalid authorization header", req);
    }

    auto ar = auth_->authorize(*token, permission);
    if (!ar.authorized) {
        return makeErrorResponse(http::status::forbidden,
                                 "Insufficient permissions for scope: " + permission,
                                 req);
    }

    return std::nullopt; // Access granted
}

std::optional<std::string> RopeApiHandler::extractIndexName(const std::string& path) {
    // Expected format: /api/v1/vector-index/{index_name}/rope/...
    const std::string prefix = "/api/v1/vector-index/";
    const std::string suffix = "/rope/";
    
    if (path.find(prefix) != 0) {
        return std::nullopt;
    }
    
    size_t start = prefix.length();
    size_t suffix_pos = path.find(suffix, start);
    
    if (suffix_pos == std::string::npos) {
        return std::nullopt;
    }
    
    std::string index_name = path.substr(start, suffix_pos - start);
    if (index_name.empty()) {
        return std::nullopt;
    }
    
    return index_name;
}

} // namespace server
} // namespace themis
