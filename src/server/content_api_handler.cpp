/**
 * @file content_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/content_api_handler.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "content/content_manager.h"
#include "content/content_processor.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/input_validator.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace themis {
namespace server {

// Bring frequently used types from other namespaces into local scope
using themis::Tracer;
using themis::AuthMiddleware;
using themis::content::ContentManager;
using themis::content::ContentProcessor;

ContentApiHandler::ContentApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<ContentManager> content_manager,
    std::shared_ptr<ContentProcessor> content_processor,
    std::shared_ptr<AuthMiddleware> auth,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<VectorIndexManager> vector_index
)
    : storage_(std::move(storage))
    , content_manager_(std::move(content_manager))
    , content_processor_(std::move(content_processor))
    , auth_(std::move(auth))
    , secondary_index_(std::move(secondary_index))
    , vector_index_(std::move(vector_index))
{
}

// Helper method to extract path parameter from URL
static std::string extractPathParam(const std::string& path, const std::string& prefix) {
    if (!(path.rfind(prefix, 0) == 0)) {
        return "";
    }
    auto param = path.substr(prefix.length());
    // Remove query string if present
    auto query_pos = param.find('?');
    if (query_pos != std::string::npos) {
        param = param.substr(0, query_pos);
    }
    return param;
}

// Helper method to extract auth context from request
static std::string extractUserId(const http::request<http::string_body>& req, std::shared_ptr<AuthMiddleware> auth) {
    if (!auth || !auth->isEnabled()) {
        return "";
    }
    
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return "";
    }
    
    auto token = AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(), auth_header.size())
    );
    if (!token) {
        return "";
    }
    
    auto result = auth->validateToken(*token);
    if (!result.authorized) {
        return "";
    }

    return result.user_id;
}

http::response<http::string_body> ContentApiHandler::handleImport(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto body = nlohmann::json::parse(req.body());
        
        // Extract optional blob (can be base64 or raw string)
        std::optional<std::string> blob;
        if (body.contains("blob")) {
            blob = body["blob"].get<std::string>();
        } else if (body.contains("blob_base64")) {
            // Decode base64 to binary, then treat as UTF-8 string
            const std::string& encoded = body["blob_base64"].get<std::string>();
            static const int kBase64DecodeTable[256] = {
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
                52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
                -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
                15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
                -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
                41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            };
            std::string decoded;
            decoded.reserve((encoded.size() * 3) / 4);
            int val = 0, valb = -8;
            for (unsigned char c : encoded) {
                if (c == '=') {
                  break;
                }
                int d = kBase64DecodeTable[c];
                if (d == -1) {
                  continue;
                }
                val = (val << 6) + d;
                valb += 6;
                if (valb >= 0) {
                    decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
                    valb -= 8;
                }
            }
            blob = std::move(decoded);
        }
        
        // Call ContentManager::importContent with structured JSON spec
        std::string user_ctx = extractUserId(req, auth_);
        auto status = content_manager.importContent(body, blob, user_ctx);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error, status.message, req);
        }
        
        // Return success response with content_id from the spec
        nlohmann::json response_json = {{"status", "success"}};
        if (body.contains("content") && body["content"].contains("id")) {
            response_json["content_id"] = body["content"]["id"].get<std::string>();
        }
        
        return makeResponse(http::status::ok, response_json.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleGet(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto id = extractPathParam(std::string(req.target()), "/content/");
        if (id.empty()) {
          return makeErrorResponse(http::status::bad_request, "Missing content id", req);
        }
        auto meta = content_manager.getContentMeta(id);
        if (!meta) {
          return makeErrorResponse(http::status::not_found, "Content not found", req);
        }
        return makeResponse(http::status::ok, meta->toJson().dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleGetBlob(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto path = std::string(req.target());
        // path format: /content/{id}/blob
        auto prefix = std::string("/content/");
        auto pos = path.find("/blob");
        if (pos == std::string::npos) {
          return makeErrorResponse(http::status::bad_request, "Invalid path", req);
        }
        auto id = path.substr(prefix.size(), pos - prefix.size());
        std::string user_ctx = extractUserId(req, auth_);
        auto blob = content_manager.getContentBlob(id, user_ctx);
        if (!blob) {
          return makeErrorResponse(http::status::not_found, "Blob not found", req);
        }
        auto meta = content_manager.getContentMeta(id);
        std::string mime = (meta ? meta->mime_type : std::string("application/octet-stream"));

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, mime);
        res.keep_alive(req.keep_alive());
        res.body() = *blob; // may contain binary data
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleGetChunks(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto path = std::string(req.target());
        // path format: /content/{id}/chunks
        auto prefix = std::string("/content/");
        auto pos = path.find("/chunks");
        if (pos == std::string::npos) {
          return makeErrorResponse(http::status::bad_request, "Invalid path", req);
        }
        auto id = path.substr(prefix.size(), pos - prefix.size());
        auto chunks = content_manager.getContentChunks(id);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& c : chunks) {
            nlohmann::json j = c.toJson();
            // For response size, omit full embedding by default
            if (j.contains("embedding")) {
              j["embedding"] = nlohmann::json::array();
            }
            arr.push_back(std::move(j));
        }
        nlohmann::json resp = { {"count", chunks.size()}, {"chunks", std::move(arr)} };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleHybridSearch(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
          return makeErrorResponse(http::status::service_unavailable, "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        nlohmann::json body = nlohmann::json::parse(req.body());
        std::string query = body.value("query", "");
        int k = body.value("k", 10);
        int hops = 1;
        if (body.contains("expand") && body["expand"].is_object()) {
            hops = body["expand"].value("hops", 1);
        }
        nlohmann::json filters = nlohmann::json::object();
        if (body.contains("filters")) {
          filters = body["filters"];
        }
        if (body.contains("scoring")) {
          filters["scoring"] = body["scoring"];
        }

        auto results = content_manager.searchWithExpansion(query, k, hops, filters);
        nlohmann::json resp = nlohmann::json::array();
        for (const auto& result : results) {
            resp.push_back({{"pk", result.first}, {"score", result.second}});
        }
        nlohmann::json out = {
            {"count", resp.size()},
            {"results", resp}
        };
        return makeResponse(http::status::ok, out.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("Hybrid search error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN("handleHybridSearch: unknown exception");
        return makeErrorResponse(http::status::bad_request, "Hybrid search error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleFusionSearch(
    const http::request<http::string_body>& req
) {
    try {
        if (!secondary_index_) {
          return makeErrorResponse(http::status::service_unavailable, "SecondaryIndexManager not initialized", req);
        }
        if (!vector_index_) {
          return makeErrorResponse(http::status::service_unavailable, "VectorIndexManager not initialized", req);
        }
        auto& secondary_index = *secondary_index_;
        auto& vector_index = *vector_index_;
        
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("table") || !body["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'table' field", req);
        }
        
        std::string table = body["table"];
        
        // QW-46 Guard: Fail-closed collection name validation
        {
            utils::InputValidator validator;
            if (!validator.validateStringLength(table, 256) || !validator.validatePathSegment(table)) {
                THEMIS_ERROR("QW-46 Guard: Invalid table name in handleFusionSearch");
                return makeErrorResponse(http::status::bad_request,
                    "Invalid table name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
            }
        }
        
        int k = body.value("k", 10);
        std::string fusionMode = body.value("fusion_mode", "rrf"); // "rrf" or "weighted"
        
        // Text search parameters (optional)
        std::vector<SecondaryIndexManager::FulltextResult> textResults;
        bool hasTextQuery = body.contains("text_query") && body.contains("text_column");
        
        if (hasTextQuery) {
            std::string textColumn = body["text_column"];
            std::string textQuery = body["text_query"];
            int textLimit = body.value("text_limit", 1000);
            
            if (!secondary_index.hasFulltextIndex(table, textColumn)) {
                return makeErrorResponse(http::status::bad_request, 
                    "No fulltext index on " + table + "." + textColumn, req);
            }
            
            auto [textStatus, textRes] = secondary_index.scanFulltextWithScores(table, textColumn, textQuery, textLimit);
            if (!textStatus.ok) {
                return makeErrorResponse(http::status::internal_server_error, "Text search failed: " + textStatus.message, req);
            }
            textResults = std::move(textRes);
        }
        
        // Vector search parameters (optional)
        std::vector<VectorIndexManager::Result> vectorResults;
        bool hasVectorQuery = body.contains("vector_query");
        
        if (hasVectorQuery) {
            if (!body["vector_query"].is_array()) {
                return makeErrorResponse(http::status::bad_request, "vector_query must be array of floats", req);
            }
            
            std::vector<float> vectorQuery;
            for (const auto& val : body["vector_query"]) {
                if (val.is_number()) {
                    vectorQuery.push_back(val.get<float>());
                }
            }
            
            if (vectorQuery.empty()) {
                return makeErrorResponse(http::status::bad_request, "vector_query array is empty", req);
            }
            
            int vectorLimit = body.value("vector_limit", 1000);
            auto [vecStatus, vecRes] = vector_index.searchKnn(vectorQuery, vectorLimit);
            if (!vecStatus.ok) {
                return makeErrorResponse(http::status::internal_server_error, "Vector search failed: " + vecStatus.message, req);
            }
            vectorResults = std::move(vecRes);
        }
        
        // Require at least one query type
        if (!hasTextQuery && !hasVectorQuery) {
            return makeErrorResponse(http::status::bad_request, "At least one of text_query or vector_query required", req);
        }
        
        // Fusion logic
        std::vector<std::pair<std::string, double>> fusedResults;
        
        if (fusionMode == "rrf") {
            // Reciprocal Rank Fusion: score = sum(1 / (k + rank))
            int kRrf = body.value("k_rrf", 60);
            std::unordered_map<std::string, double> scores;
            
            // Text contributions
            for (size_t i = 0; i < textResults.size(); ++i) {
                scores[textResults[i].pk] += 1.0 / (kRrf + i + 1);
            }
            
            // Vector contributions
            for (size_t i = 0; i < vectorResults.size(); ++i) {
                scores[vectorResults[i].pk] += 1.0 / (kRrf + i + 1);
            }
            
            // Convert to vector and sort
            fusedResults.reserve(scores.size());
            for (const auto& [pk, score] : scores) {
                fusedResults.emplace_back(pk, score);
            }
            std::sort(fusedResults.begin(), fusedResults.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
        } else if (fusionMode == "weighted") {
            // Weighted fusion: alpha * normalize(text_score) + (1 - alpha) * normalize(vector_sim)
            double alpha = body.value("weight_text", 0.5);
            alpha = std::clamp(alpha, 0.0, 1.0);
            
            // Normalize text scores (min-max)
            double textMin = textResults.empty() ? 0.0 : textResults.back().score;
            double textMax = textResults.empty() ? 1.0 : textResults.front().score;
            double textRange = (textMax - textMin) > 1e-9 ? (textMax - textMin) : 1.0;
            
            // Normalize vector distances (convert to similarity: 1 - normalized_dist)
            // Assuming L2 or COSINE metric; smaller distance = better
            double vecMin = vectorResults.empty() ? 0.0 : vectorResults.front().distance;
            double vecMax = vectorResults.empty() ? 1.0 : vectorResults.back().distance;
            double vecRange = (vecMax - vecMin) > 1e-9 ? (vecMax - vecMin) : 1.0;
            
            std::unordered_map<std::string, double> scores;
            
            // Text contributions
            for (const auto& res : textResults) {
                double normScore = (res.score - textMin) / textRange;
                scores[res.pk] += alpha * normScore;
            }
            
            // Vector contributions (convert distance to similarity)
            for (const auto& res : vectorResults) {
                double normDist = (res.distance - vecMin) / vecRange;
                double similarity = 1.0 - normDist;
                scores[res.pk] += (1.0 - alpha) * similarity;
            }
            
            // Convert to vector and sort
            fusedResults.reserve(scores.size());
            for (const auto& [pk, score] : scores) {
                fusedResults.emplace_back(pk, score);
            }
            std::sort(fusedResults.begin(), fusedResults.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
        } else {
            return makeErrorResponse(http::status::bad_request, 
                "Invalid fusion_mode: " + fusionMode + " (must be 'rrf' or 'weighted')", req);
        }
        
        // Limit to top-k
        if (fusedResults.size() > static_cast<size_t>(k)) {
            fusedResults.resize(k);
        }
        
        // Build response
        nlohmann::json resp = nlohmann::json::array();
        for (const auto& [pk, score] : fusedResults) {
            resp.push_back({
                {"pk", pk},
                {"score", score}
            });
        }
        
        nlohmann::json out = {
            {"count", resp.size()},
            {"fusion_mode", fusionMode},
            {"table", table},
            {"results", resp}
        };
        
        if (hasTextQuery) {
            out["text_count"] = textResults.size();
        }
        if (hasVectorQuery) {
            out["vector_count"] = vectorResults.size();
        }
        
        return makeResponse(http::status::ok, out.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("Fusion search error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN("handleFusionSearch: unknown exception");
        return makeErrorResponse(http::status::internal_server_error, "Unknown fusion search error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleFulltextSearch(
    const http::request<http::string_body>& req
) {
    try {
        if (!secondary_index_) {
          return makeErrorResponse(http::status::service_unavailable, "IndexManager not initialized", req);
        }
        auto& secondary_index = *secondary_index_;
        
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Required fields
        if (!body.contains("table") || !body["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'table' field", req);
        }
        if (!body.contains("column") || !body["column"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'column' field", req);
        }
        if (!body.contains("query") || !body["query"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'query' field", req);
        }
        
        std::string table = body["table"];
        
        // QW-46 Guard: Fail-closed collection name validation
        {
            utils::InputValidator validator;
            if (!validator.validateStringLength(table, 256) || !validator.validatePathSegment(table)) {
                THEMIS_ERROR("QW-46 Guard: Invalid table name in handleFulltext");
                return makeErrorResponse(http::status::bad_request,
                    "Invalid table name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
            }
        }
        
        std::string column = body["column"];
        std::string query = body["query"];
        size_t limit = body.value("limit", 1000);
        
        // Check if fulltext index exists
        if (!secondary_index.hasFulltextIndex(table, column)) {
            return makeErrorResponse(http::status::bad_request, 
                "No fulltext index on " + table + "." + column, req);
        }
        
        // Perform BM25-scored fulltext search
        auto [status, results] = secondary_index.scanFulltextWithScores(table, column, query, limit);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error, status.message, req);
        }
        
        // Build response with scores
        nlohmann::json resp = nlohmann::json::array();
        for (const auto& result : results) {
            resp.push_back({
                {"pk", result.pk},
                {"score", result.score}
            });
        }
        
        nlohmann::json out = {
            {"count", resp.size()},
            {"results", resp},
            {"table", table},
            {"column", column},
            {"query", query}
        };
        
        return makeResponse(http::status::ok, out.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("Fulltext search error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN("handleFulltextSearch: unknown exception");
        return makeErrorResponse(http::status::internal_server_error, "Unknown fulltext search error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleConfigGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleContentConfigGet");
    
    try {
        auto v = storage_->get("config:content");
        nlohmann::json resp;
        if (v) {
            std::string s(v->begin(), v->end());
            resp = nlohmann::json::parse(s);
        } else {
            // Return defaults
            resp = {
                {"compress_blobs", false},
                {"compression_level", 19},
                {"skip_compressed_mimes", nlohmann::json::array({"image/", "video/", "application/zip", "application/gzip"})}
            };
        }
        
        span.setStatus(true);
        return makeResponse(http::status::ok, resp.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, 
            std::string("config read error: ") + e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleConfigPut(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleContentConfigPut");
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Get current config or defaults
        nlohmann::json config;
        auto v = storage_->get("config:content");
        if (v) {
            std::string s(v->begin(), v->end());
            config = nlohmann::json::parse(s);
        } else {
            config = {
                {"compress_blobs", false},
                {"compression_level", 19},
                {"skip_compressed_mimes", nlohmann::json::array({"image/", "video/", "application/zip", "application/gzip"})}
            };
        }
        
        // Update with provided values
        if (body.contains("compress_blobs")) {
            if (!body["compress_blobs"].is_boolean()) {
                span.setStatus(false, "invalid_compress_blobs");
                return makeErrorResponse(http::status::bad_request, 
                    "compress_blobs must be boolean", req);
            }
            config["compress_blobs"] = body["compress_blobs"];
        }
        
        if (body.contains("compression_level")) {
            if (!body["compression_level"].is_number_integer()) {
                span.setStatus(false, "invalid_compression_level");
                return makeErrorResponse(http::status::bad_request, 
                    "compression_level must be an integer", req);
            }
            int level = body["compression_level"];
            if (level < 1 || level > 22) {
                span.setStatus(false, "compression_level_out_of_range");
                return makeErrorResponse(http::status::bad_request, 
                    "compression_level must be between 1 and 22", req);
            }
            config["compression_level"] = level;
        }
        
        if (body.contains("skip_compressed_mimes")) {
            if (!body["skip_compressed_mimes"].is_array()) {
                span.setStatus(false, "invalid_skip_mimes");
                return makeErrorResponse(http::status::bad_request, 
                    "skip_compressed_mimes must be an array of strings", req);
            }
            // Validate all elements are strings
            for (const auto& item : body["skip_compressed_mimes"]) {
                if (!item.is_string()) {
                    span.setStatus(false, "invalid_skip_mimes_element");
                    return makeErrorResponse(http::status::bad_request, 
                        "All elements in skip_compressed_mimes must be strings", req);
                }
            }
            config["skip_compressed_mimes"] = body["skip_compressed_mimes"];
        }
        
        // Store updated config
        std::string config_str = config.dump();
        std::vector<uint8_t> bytes(config_str.begin(), config_str.end());
        bool ok = storage_->put("config:content", bytes);
        
        if (!ok) {
            span.setStatus(false, "storage_error");
            return makeErrorResponse(http::status::internal_server_error, 
                "Failed to store content config", req);
        }
        
        nlohmann::json response = config;
        response["status"] = "ok";
        response["note"] = "Configuration updated. Changes apply to new content imports only.";
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, 
            std::string("config write error: ") + e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleContentFilterSchemaGet(
    const http::request<http::string_body>& req
) {
    try {
        auto v = storage_->get("config:content_filter_schema");
        nlohmann::json resp;
        if (v) {
            std::string s(v->begin(), v->end());
            resp = nlohmann::json::parse(s);
        } else {
            resp = nlohmann::json{{"field_map", nlohmann::json::object()}};
        }
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("config read error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "content_api_handler: unhandled exception caught");
        return makeErrorResponse(http::status::internal_server_error, "config read error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleContentFilterSchemaPut(
    const http::request<http::string_body>& req
) {
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.is_object() || !body.contains("field_map") || !body["field_map"].is_object()) {
            return makeErrorResponse(http::status::bad_request, "Body must be { field_map: { key: path } }", req);
        }
        std::string s = body.dump();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        bool ok = storage_->put("config:content_filter_schema", bytes);
        if (!ok) {
          return makeErrorResponse(http::status::internal_server_error, "Failed to store filter schema", req);
        }
        return makeResponse(http::status::ok, nlohmann::json{{"status","ok"}}.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("config write error: ") + e.what(), req);
    } catch (...) {
        THEMIS_DEBUG([[maybe_unused]] "content_api_handler: unhandled exception caught");
        return makeErrorResponse(http::status::bad_request, "config write error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleEdgeWeightConfigGet(
    const http::request<http::string_body>& req
) {
    try {
        auto v = storage_->get("config:edge_weights");
        nlohmann::json resp;
        if (v) {
            std::string s(v->begin(), v->end());
            resp = nlohmann::json::parse(s);
        } else {
            resp = nlohmann::json{{"weights", nlohmann::json{{"parent", 1.0}, {"next", 1.0}, {"prev", 1.0}}}};
        }
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("config read error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "content_api_handler: unhandled exception caught");
        return makeErrorResponse(http::status::internal_server_error, "config read error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleEdgeWeightConfigPut(
    const http::request<http::string_body>& req
) {
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.is_object() || !body.contains("weights") || !body["weights"].is_object()) {
            return makeErrorResponse(http::status::bad_request, "Body must be { weights: { parent: number, next: number, prev: number } }", req);
        }
        // Validate all values numeric
        for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
            if (!it.value().is_number()) {
                return makeErrorResponse(http::status::bad_request, "All weights must be numeric", req);
            }
        }
        std::string s = body.dump();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        bool ok = storage_->put("config:edge_weights", bytes);
        if (!ok) {
          return makeErrorResponse(http::status::internal_server_error, "Failed to store edge weights", req);
        }
        return makeResponse(http::status::ok, nlohmann::json{{"status","ok"}}.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("config write error: ") + e.what(), req);
    } catch (...) {
        THEMIS_DEBUG([[maybe_unused]] "content_api_handler: unhandled exception caught");
        return makeErrorResponse(http::status::bad_request, "config write error", req);
    }
}

http::response<http::string_body> ContentApiHandler::handleEncryptionSchemaGet(
    const http::request<http::string_body>& req
) {
    // Note: Access control should be handled by HttpServer's requireAccess before calling this handler
    try {
        auto schema_bytes = storage_->get("config:encryption_schema");
        if (!schema_bytes) {
            // Return empty schema if not configured
            nlohmann::json empty_schema = {
                {"collections", nlohmann::json::object()}
            };
            return makeResponse(http::status::ok, empty_schema.dump(2), req);
        }
        
        std::string schema_json(schema_bytes->begin(), schema_bytes->end());
        // Validate JSON before returning
        try {
            auto parsed = nlohmann::json::parse(schema_json);
            return makeResponse(http::status::ok, parsed.dump(2), req);
        } catch (const nlohmann::json::exception& e) {
            return makeErrorResponse(http::status::internal_server_error, 
                std::string("Stored schema is invalid JSON: ") + e.what(), req);
        }
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::handleEncryptionSchemaPut(
    const http::request<http::string_body>& req
) {
    // Note: Access control should be handled by HttpServer's requireAccess before calling this handler
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate schema structure
        if (!body.contains("collections") || !body["collections"].is_object()) {
            return makeErrorResponse(http::status::bad_request, 
                "Schema must contain 'collections' object", req);
        }
        
        // Validate each collection
        for (auto& [collection_name, collection_config] : body["collections"].items()) {
            // QW-46 Guard: Fail-closed collection name validation for keys
            {
                utils::InputValidator validator;
                if (!validator.validateStringLength(collection_name, 256) || !validator.validatePathSegment(collection_name)) {
                    THEMIS_ERROR("QW-46 Guard: Invalid collection name in handleBatchConfig");
                    return makeErrorResponse(http::status::bad_request,
                        "Invalid collection name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
                }
            }
            
            if (!collection_config.is_object()) {
                return makeErrorResponse(http::status::bad_request, 
                    "Collection config for '" + collection_name + "' must be an object", req);
            }
            
            if (!collection_config.contains("encryption")) {
              continue;
            }
            
            auto& enc = collection_config["encryption"];
            if (!enc.is_object()) {
                return makeErrorResponse(http::status::bad_request, 
                    "Encryption config for '" + collection_name + "' must be an object", req);
            }
            
            // Validate required fields
            if (!enc.contains("enabled") || !enc["enabled"].is_boolean()) {
                return makeErrorResponse(http::status::bad_request, 
                    "Encryption 'enabled' must be boolean for collection '" + collection_name + "'", req);
            }
            
            if (enc["enabled"].get<bool>()) {
                // If enabled, require fields array
                if (!enc.contains("fields") || !enc["fields"].is_array()) {
                    return makeErrorResponse(http::status::bad_request, 
                        "Encryption 'fields' must be array for collection '" + collection_name + "'", req);
                }
                
                // Validate fields are strings
                for (auto& field : enc["fields"]) {
                    if (!field.is_string()) {
                        return makeErrorResponse(http::status::bad_request, 
                            "All fields must be strings for collection '" + collection_name + "'", req);
                    }
                }
                
                // Validate context_type if present
                if (enc.contains("context_type")) {
                    std::string ctx = enc["context_type"].get<std::string>();
                    if (ctx != "user" && ctx != "group") {
                        return makeErrorResponse(http::status::bad_request, 
                            "context_type must be 'user' or 'group' for collection '" + collection_name + "'", req);
                    }
                    
                    // If group context, allowed_groups is optional but should be array if present
                    if (ctx == "group" && enc.contains("allowed_groups")) {
                        if (!enc["allowed_groups"].is_array()) {
                            return makeErrorResponse(http::status::bad_request, 
                                "allowed_groups must be array for collection '" + collection_name + "'", req);
                        }
                    }
                }
            }
        }
        
        // Store validated schema
        std::string schema_str = body.dump();
        std::vector<uint8_t> bytes(schema_str.begin(), schema_str.end());
        bool ok = storage_->put("config:encryption_schema", bytes);
        
        if (!ok) {
            return makeErrorResponse(http::status::internal_server_error, 
                "Failed to store encryption schema", req);
        }
        
        THEMIS_INFO("Encryption schema updated: {} collections configured", 
            body["collections"].size());
        
        nlohmann::json response = {
            {"status", "ok"},
            {"collections_configured", body["collections"].size()}
        };
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        return makeErrorResponse(http::status::bad_request, 
            std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ContentApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> ContentApiHandler::makeResponse(
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

} // namespace server
} // namespace themis

