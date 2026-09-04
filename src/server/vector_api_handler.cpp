/**
 * @file vector_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/vector_api_handler.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/vector_index.h"
#include "server/auth_middleware.h"
#include "security/encryption.h"
#include "security/key_provider.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/hkdf_helper.h"
#include "utils/input_validator.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

constexpr size_t MAX_VECTOR_SEARCH_BODY_SIZE = 2'000'000;
constexpr size_t MAX_VECTOR_BATCH_BODY_SIZE = 120'000'000;
constexpr size_t MAX_VECTOR_FILTER_BODY_SIZE = 4'000'000;
constexpr size_t MAX_VECTOR_FIELD_NAME_LENGTH = 128;
constexpr size_t MAX_VECTOR_PK_LENGTH = 1024;

bool isBodyWithinLimit(std::string_view body, size_t max_len) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(body), max_len);
}

bool isValidVectorFieldName(std::string_view field_name) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(field_name), MAX_VECTOR_FIELD_NAME_LENGTH) &&
           validator.validatePathSegment(std::string(field_name));
}

bool isValidVectorPk(std::string_view pk) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(pk), MAX_VECTOR_PK_LENGTH) &&
           validator.validatePathSegment(std::string(pk));
}

} // namespace

VectorApiHandler::VectorApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<::themis::AuthMiddleware> auth,
    std::shared_ptr<FieldEncryption> field_encryption,
    std::shared_ptr<KeyProvider> key_provider
)
    : storage_(std::move(storage))
    , vector_index_(std::move(vector_index))
    , auth_(std::move(auth))
    , field_encryption_(std::move(field_encryption))
    , key_provider_(std::move(key_provider))
{
}

http::response<http::string_body> VectorApiHandler::handleSearch(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "data:read", "vector.search", path_only)) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("handleVectorSearch");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/vector/search");
    
    try {
        if (!isBodyWithinLimit(req.body(), MAX_VECTOR_SEARCH_BODY_SIZE)) {
            span.setStatus(false, "Request body exceeds maximum allowed size");
            return makeErrorResponse(http::status::bad_request,
                "Request body exceeds maximum allowed size", req);
        }

        // Governance enforcement: block ANN for certain classifications in enforce mode
        auto to_lower = [](std::string s){ for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c))); return s; };
        std::string classification = {};
        std::string mode = "observe";
        for (const auto& h : req) {
            auto name = h.name_string();
            if (beast::iequals(name, "X-Classification")) {
              classification = to_lower(std::string(h.value()));
            }
            else if (beast::iequals(name, "X-Governance-Mode")) mode = to_lower(std::string(h.value()));
        }
        if (mode == "enforce") {
            if (classification == "geheim" || classification == "streng-geheim") {
                nlohmann::json j = {{"error","policy_denied"},{"message","ANN blocked by classification"}};
                auto res = makeResponse(http::status::forbidden, j.dump(), req);
                return res;
            }
        }

        auto body_json = json::parse(req.body());
        
        // Validate required fields
        if (!body_json.contains("vector")) {
            span.setAttribute("error", "missing_vector_field");
            span.setStatus(false, "Missing vector field");
            return makeErrorResponse(http::status::bad_request,
                "Missing required field: vector", req);
        }
        
        // Parse query vector
        std::vector<float> queryVector = {};

        if (body_json["vector"].is_array()) {
            for (const auto& val : body_json["vector"]) {
                if (val.is_number()) {
                    queryVector.push_back(val.get<float>());
                } else {
                    span.setStatus(false, "Invalid vector element");
                    return makeErrorResponse(http::status::bad_request,
                        "Vector elements must be numbers", req);
                }
            }
        } else {
            span.setStatus(false, "Vector must be array");
            return makeErrorResponse(http::status::bad_request,
                "Field 'vector' must be an array", req);
        }
        
        // Parse k (default: 10)
        size_t k = body_json.value("k", 10);
            span.setAttribute("vector.k", static_cast<int64_t>(k));
            span.setAttribute("vector.dimension", static_cast<int64_t>(queryVector.size()));
        
    if (k == 0) {
        span.setAttribute("error", "invalid_k_value");
        span.setStatus(false, "K must be greater than 0");
            return makeErrorResponse(http::status::bad_request,
                "Field 'k' must be greater than 0", req);
        }
        
        // Validate dimension
        int expectedDim = vector_index_->getDimension();
        if (expectedDim > 0  && static_cast<size_t>(static_cast) < int>(queryVector.size()) != expectedDim) {
            span.setStatus(false, "Dimension mismatch");
            return makeErrorResponse(http::status::bad_request,
                "Vector dimension mismatch: expected " + std::to_string(expectedDim) +
                ", got " + std::to_string(queryVector.size()), req);
        }
        
        // Optional cursor-based pagination for vector search
        bool use_cursor = body_json.value("use_cursor", false);
        size_t offset = 0;
        if (use_cursor && body_json.contains("cursor")) {
            try {
                // Einfaches Cursor-Format: numerischer Offset als String
                std::string cur = body_json["cursor"].get<std::string>();
                if (static_cast<int>(cur.size()) > 64) {
                    span.setStatus(false, "Cursor too long");
                    return makeErrorResponse(http::status::bad_request,
                        "Field 'cursor' exceeds maximum allowed length", req);
                }
                offset = static_cast<size_t>(std::stoull(cur));
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "vector_api_handler: unhandled exception caught");
                offset = 0;
            }
        }

        size_t want_k = use_cursor ? (k + offset + 1) : k;

        // Perform k-NN search (ggf. mit erweitertem k für Pagination)
        auto [status, results] = vector_index_->searchKnn(queryVector, want_k);
        
        if (!status.ok) {
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Vector search failed: " + status.message, req);
        }
        
        if (use_cursor) {
            // Slice [offset, offset+k) und Cursor-Felder setzen
            json items = json::array();
            size_t start = std::min(offset,static_cast<int>(results.size()));
            size_t end = std::min(results.size(), start + k);
            for (size_t i = start; i < end; ++i) {
                items.push_back({{"pk", results[i].pk}, {"distance", results[i].distance}});
            }
            bool has_more = results.size() > end;
            json response = {
                {"items", items},
                {"batch_size", end - start},
                {"has_more", has_more}
            };
            if (has_more) {
              response["next_cursor"] = std::to_string(end);
            }
            span.setAttribute("vector.results_count", static_cast<int64_t>(end - start));
            span.setStatus(true);
            return makeResponse(http::status::ok, response.dump(), req);
        } else {
            // Legacy Format
            json resultJson = json::array();
            for (const auto& result : results) {
                resultJson.push_back({{"pk", result.pk}, {"distance", result.distance}});
            }
            json response = {{"results", resultJson}, {"k", k}, {"count",static_cast<int>(results.size())}};
            span.setAttribute("vector.results_count", static_cast<int64_t>(results.size()));
            span.setStatus(true);
            return makeResponse(http::status::ok, response.dump(), req);
        }

    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        THEMIS_ERROR("Vector search error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleBatchInsert(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "data:write", "vector.write", path_only)) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("handleVectorBatchInsert");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/vector/batch_insert");

    try {
        if (!isBodyWithinLimit(req.body(), MAX_VECTOR_BATCH_BODY_SIZE)) {
            span.setStatus(false, "Request body exceeds maximum allowed size");
            return makeErrorResponse(http::status::bad_request,
                "Request body exceeds maximum allowed size", req);
        }

        auto body = json::parse(req.body());

        if (!body.contains("items") || !body["items"].is_array()) {
            span.setStatus(false, "missing_items");
            return makeErrorResponse(http::status::bad_request, "Missing required field: items (array)", req);
        }

        std::string vector_field = body.value("vector_field", std::string("embedding"));
        if (!isValidVectorFieldName(vector_field)) {
            span.setStatus(false, "invalid_vector_field");
            return makeErrorResponse(http::status::bad_request,
                "Invalid field: vector_field", req);
        }

        std::string object_name = vector_index_->getObjectName();
        int configured_dim = vector_index_->getDimension();
        size_t inserted = 0;
        size_t errors = 0;

        // Optionale Auto-Init falls noch nicht konfiguriert
        if (configured_dim <= 0) {
            // Dimension aus dem ersten Element ableiten
            for (const auto& it : body["items"]) {
                if (it.contains("vector") && it["vector"].is_array()) {
                    int dim = static_cast<int>(it["vector"].size());
                    if (dim > 0) {
                        auto st = vector_index_->init("vectors", dim, themis::VectorIndexManager::Metric::COSINE);
                        if (!st.ok) {
                            span.setStatus(false, st.message);
                            return makeErrorResponse(http::status::internal_server_error, std::string("Failed to init vector index: ") + st.message, req);
                        }
                        configured_dim = dim;
                        object_name = vector_index_->getObjectName();
                    }
                    break;
                }
            }
            if (configured_dim <= 0) {
                span.setStatus(false, "cannot_infer_dim");
                return makeErrorResponse(http::status::bad_request, "Cannot infer dimension from items", req);
            }
        }

        // Use a single WriteBatch for higher throughput
        // Load optional encryption schema once (for vector metadata encryption)
        json vector_enc_cfg;
        bool vector_enc_enabled = false;
        std::vector<std::string> vector_enc_fields;
        try {
            if (auto schema_bytes = storage_->get("config:encryption_schema")) {
                std::string s(schema_bytes->begin(), schema_bytes->end());
                auto schema_json = json::parse(s);
                if (schema_json.contains("collections") && schema_json["collections"].is_object()) {
                    // Collection name: use object_name resolved after possible auto-init (e.g. "vectors" or custom)
                    if (schema_json["collections"].contains(object_name)) {
                        auto coll = schema_json["collections"][object_name];
                        if (coll.contains("encryption") && coll["encryption"].is_object()) {
                            auto ecfg = coll["encryption"];
                            vector_enc_enabled = ecfg.value("enabled", false);
                            if (ecfg.contains("fields") && ecfg["fields"].is_array()) {
                                for (const auto& f : ecfg["fields"]) {
                                  if (f.is_string()) vector_enc_fields.push_back(f.get<std::string>());
                                }
                            }
                        }
                        // Backward-compatible schema: { collections: { name: { fields: { fld: { encrypt: true } } } } }
                        if (!vector_enc_enabled && coll.contains("fields") && coll["fields"].is_object()) {
                            for (auto itf = coll["fields"].begin(); itf != coll["fields"].end(); ++itf) {
                                try {
                                    if (itf.value().is_object() && itf.value().value("encrypt", false)) {
                                        vector_enc_fields.push_back(itf.key());
                                    }
                                } catch (...) { /* ignore */ }
                            }
                            vector_enc_enabled = !vector_enc_fields.empty();
                        }
                    }
                }
            }
        } catch (...) {
            THEMIS_WARN([[maybe_unused]] "vector_api_handler: unhandled exception caught");
            vector_enc_enabled = false; // fail-safe
        }

        // Extract auth context for user-based HKDF (salt = user_id) if encryption active
        std::string enc_user_ctx = {};
        if (vector_enc_enabled) {
            auto auth_ctx = extractAuthContext(req);
            enc_user_ctx = auth_ctx.user_id.empty() ? "anonymous" : auth_ctx.user_id;
        }

        auto batch = storage_->createWriteBatch();
        for (const auto& it : body["items"]) {
            try {
                if (!it.contains("pk") || !it["pk"].is_string()) { ++errors; continue; }
                if (!it.contains("vector") || !it["vector"].is_array()) { ++errors; continue; }

                std::string pk = it["pk"].get<std::string>();
                if (!isValidVectorPk(pk)) { ++errors; continue; }

                std::vector<float> vec = {};

                vec.reserve(it["vector"].size());
                for (const auto& v : it["vector"]) {
                    if (!v.is_number()) { vec.clear(); break; }
                    vec.push_back(v.get<float>());
                }
                if (vec.empty()  || static_cast<size_t>(static_cast) < int>(vec.size()) != configured_dim) { ++errors; continue; }

                // Build entity
                BaseEntity e(pk);
                e.setField(vector_field, vec);
                if (it.contains("fields") && it["fields"].is_object()) {
                    for (auto fit = it["fields"].begin(); fit != it["fields"].end(); ++fit) {
                        const auto& val = fit.value();
                        const std::string key = fit.key();
                        if (val.is_string()) {
                          e.setField(key, val.get<std::string>());
                        }
                        else if (val.is_number_integer()) e.setField(key, static_cast<int64_t>(val.get<int64_t>()));
                        else if (val.is_number_float()) e.setField(key, val.get<double>());
                        else if (val.is_boolean()) e.setField(key, val.get<bool>());
                    }
                }

                // Vector metadata encryption (schema-driven) - do NOT encrypt actual embedding
                if (vector_enc_enabled && !vector_enc_fields.empty() && field_encryption_ && key_provider_) {
                    for (const auto& mf : vector_enc_fields) {
                        if (mf == vector_field) continue; // never encrypt embedding itself
                        if (!e.hasField(mf)) {
                          continue;
                        }
                        auto valOpt = e.getField(mf);
                        if (!valOpt.has_value()) {
                          continue;
                        }
                        // Serialize value to string (reuse logic similar to handlePutEntity for primitives)
                        std::string plain_str = {};
                        const auto& v = *valOpt;
                        if (std::holds_alternative<std::string>(v)) {
                          plain_str = std::get<std::string>(v);
                        }
                        else if (std::holds_alternative<int64_t>(v)) plain_str = std::to_string(std::get<int64_t>(v));
                        else if (std::holds_alternative<double>(v)) plain_str = std::to_string(std::get<double>(v));
                        else if (std::holds_alternative<bool>(v)) plain_str = std::get<bool>(v) ? "true" : "false";
                        else {
                            // skip unsupported complex types in metadata for now
                            continue;
                        }
                        try {
                            // Derive field key: HKDF(DEK, user_id, "field:"+mf)
                            auto dek = key_provider_->getKey("dek");
                            std::vector<uint8_t> salt = {};

                            if (!enc_user_ctx.empty()) {
                              salt.assign(enc_user_ctx.begin(), enc_user_ctx.end());
                            }
                            std::string info = std::string("field:") + mf;
                            auto raw_key = utils::HKDFHelper::derive(dek, salt, info, 32);
                            auto blob = field_encryption_->encryptWithKey(plain_str, "vector_meta:" + mf, 1, raw_key);
                            auto j = blob.toJson();
                            e.setField(mf + "_encrypted", j.dump());
                            e.setField(mf + "_enc", true);
                            // remove plaintext
                            e.setField(mf, std::monostate{});
                        } catch (const std::exception& ex) {
                            THEMIS_WARN("Vector metadata encryption failed for {}: {}", mf, ex.what());
                        }
                    }
                }

                auto st = vector_index_->addEntity(e, *batch, vector_field);
                if (st.ok) ++inserted; else { ++errors; }
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "vector_api_handler: unhandled exception caught");
                ++errors;
            }
        }
        // Commit once for the whole batch
        if (!batch->commit()) {
            span.setStatus(false, "batch_commit_failed");
            return makeErrorResponse(http::status::internal_server_error, "Vector batch commit failed", req);
        }

        json response = {
            {"inserted", inserted},
            {"errors", errors},
            {"objectName", vector_index_->getObjectName()},
            {"dimension", vector_index_->getDimension()}
        };
        span.setAttribute("batch.inserted", static_cast<int64_t>(inserted));
        span.setAttribute("batch.errors", static_cast<int64_t>(errors));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleDeleteByFilter(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "data:write", "vector.write", path_only)) {
          return *resp;
        }
    }
    auto span = Tracer::startSpan("handleVectorDeleteByFilter");
    span.setAttribute("http.method", "DELETE");
    span.setAttribute("http.path", "/vector/by-filter");

    try {
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request, "Empty body; expected { pks: [...]} or { prefix: '...' }", req);
        }
        if (!isBodyWithinLimit(req.body(), MAX_VECTOR_FILTER_BODY_SIZE)) {
            return makeErrorResponse(http::status::bad_request,
                "Request body exceeds maximum allowed size", req);
        }
        auto body = json::parse(req.body());

        size_t deleted = 0;
        if (body.contains("pks") && body["pks"].is_array()) {
            for (const auto& v : body["pks"]) {
                if (!v.is_string()) {
                  continue;
                }
                const std::string pk = v.get<std::string>();
                if (!isValidVectorPk(pk)) {
                  continue;
                }
                auto st = vector_index_->removeByPk(pk);
                if (st.ok) {
                  ++deleted;
                }
            }
            json resp = {{"deleted", deleted}, {"method", "pks"}};
            span.setAttribute("deleted", static_cast<int64_t>(deleted));
            span.setStatus(true);
            return makeResponse(http::status::ok, resp.dump(), req);
        }

        if (body.contains("prefix") && body["prefix"].is_string()) {
            std::string prefix = body["prefix"].get<std::string>();
            if (!isValidVectorPk(prefix)) {
                return makeErrorResponse(http::status::bad_request,
                    "Invalid field: prefix", req);
            }
            // Scan RocksDB for keys starting with objectName:prefix
            std::string fullPrefix = vector_index_->getObjectName() + ":" + prefix;
            storage_->scanPrefix(fullPrefix, [&](std::string_view key, std::string_view /*value*/){
                try {
                    std::string pk = KeySchema::extractPrimaryKey(key);
                    auto st = vector_index_->removeByPk(pk);
                    if (st.ok) {
                      ++deleted;
                    }
                } catch (...) {}
                return true; // continue
            });
            json resp = {{"deleted", deleted}, {"method", "prefix"}, {"prefix", prefix}};
            span.setAttribute("deleted", static_cast<int64_t>(deleted));
            span.setStatus(true);
            return makeResponse(http::status::ok, resp.dump(), req);
        }

        return makeErrorResponse(http::status::bad_request, "Provide either 'pks' array or 'prefix' string", req);
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request, std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleIndexSave(
    const http::request<http::string_body>& req
) {
    try {
        auto body_json = json::parse(req.body());
        
        // Optional: directory parameter, default to "./data/vector_index"
        std::string directory = body_json.value("directory", "./data/vector_index");
        
        auto status = vector_index_->saveIndex(directory);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to save index: " + status.message, req);
        }
        
        json response = {
            {"message", "Vector index saved successfully"},
            {"directory", directory}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Vector index save error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleIndexLoad(
    const http::request<http::string_body>& req
) {
    try {
        auto body_json = json::parse(req.body());
        
        // Required: directory parameter
        if (!body_json.contains("directory")) {
            return makeErrorResponse(http::status::bad_request,
                "Missing required field: directory", req);
        }
        
        std::string directory = body_json["directory"];
        
        auto status = vector_index_->loadIndex(directory);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to load index: " + status.message, req);
        }
        
        json response = {
            {"message", "Vector index loaded successfully"},
            {"directory", directory}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Vector index load error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleIndexConfigGet(
    const http::request<http::string_body>& req
) {
    try {
        std::string metricStr = {};
        if (vector_index_->getMetric() == themis::VectorIndexManager::Metric::L2) {
            metricStr = "L2";
        } else if (vector_index_->getMetric() == themis::VectorIndexManager::Metric::DOT) {
            metricStr = "DOT";
        } else {
            metricStr = "COSINE";
        }
        
        json response = {
            {"objectName", vector_index_->getObjectName()},
            {"dimension", vector_index_->getDimension()},
            {"metric", metricStr},
            {"efSearch", vector_index_->getEfSearch()},
            {"M", vector_index_->getM()},
            {"efConstruction", vector_index_->getEfConstruction()},
            {"hnswEnabled", vector_index_->isHnswEnabled()}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const std::exception& e) {
        THEMIS_ERROR("Vector config get error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleIndexConfigPut(
    const http::request<http::string_body>& req
) {
    try {
        auto body_json = json::parse(req.body());
        
        // Hot-update efSearch
        if (body_json.contains("efSearch")) {
            int efSearch = body_json["efSearch"];
            if (efSearch < 1 || efSearch > 10000) {
                return makeErrorResponse(http::status::bad_request,
                    "efSearch must be between 1 and 10000", req);
            }
            
            auto status = vector_index_->setEfSearch(efSearch);
            if (!status.ok) {
                return makeErrorResponse(http::status::internal_server_error,
                    "Failed to set efSearch: " + status.message, req);
            }
        }
        
        json response = {
            {"message", "Vector index configuration updated"},
            {"updated_fields", body_json}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Vector config update error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleIndexStats(
    const http::request<http::string_body>& req
) {
    try {
        std::string metricStr = {};
        if (vector_index_->getMetric() == themis::VectorIndexManager::Metric::L2) {
            metricStr = "L2";
        } else if (vector_index_->getMetric() == themis::VectorIndexManager::Metric::DOT) {
            metricStr = "DOT";
        } else {
            metricStr = "COSINE";
        }
        
        json response = {
            {"objectName", vector_index_->getObjectName()},
            {"dimension", vector_index_->getDimension()},
            {"metric", metricStr},
            {"vectorCount", vector_index_->getVectorCount()},
            {"efSearch", vector_index_->getEfSearch()},
            {"M", vector_index_->getM()},
            {"efConstruction", vector_index_->getEfConstruction()},
            {"hnswEnabled", vector_index_->isHnswEnabled()}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const std::exception& e) {
        THEMIS_ERROR("Vector stats error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::handleIncrementalReindex(
    const http::request<http::string_body>& req
) {
    if (auth_) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) {
          path_only = path_only.substr(0, qpos);
        }
        if (auto resp = requireAccess(req, "index:write", "vector.incremental-reindex", path_only))
            return *resp;
    }
    auto span = Tracer::startSpan("handleVectorIncrementalReindex");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/vector/index/incremental-reindex");

    try {
        float rebuild_threshold = 0.20f;
        std::string vector_field = "embedding";

        if (!req.body().empty()) {
            auto body = json::parse(req.body());
            if (body.contains("rebuild_threshold"))
                rebuild_threshold = body["rebuild_threshold"].get<float>();
            if (body.contains("vector_field"))
                vector_field = body["vector_field"].get<std::string>();
        }

        auto [status, stats] = vector_index_->incrementalReindex(rebuild_threshold, vector_field);

        if (!status.ok) {
            THEMIS_ERROR("Incremental reindex failed: {}", status.message);
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error, status.message, req);
        }

        json response = {
            {"success",                  true},
            {"added",                    stats.added},
            {"removed",                  stats.removed},
            {"updated",                  stats.updated},
            {"unchanged",                stats.unchanged},
            {"total_scanned",            stats.total_scanned},
            {"full_rebuild_triggered",   stats.full_rebuild_triggered},
            {"vector_count",             vector_index_->getVectorCount()}
        };

        THEMIS_INFO("Incremental reindex: added={} removed={} updated={} unchanged={} scanned={}",
                    stats.added, stats.removed, stats.updated, stats.unchanged, stats.total_scanned);
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Incremental reindex exception: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> VectorApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> VectorApiHandler::makeResponse(
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

std::optional<http::response<http::string_body>> VectorApiHandler::requireAccess(
    [[maybe_unused]] const http::request<http::string_body>& req,
    const std::string& permission,
    const std::string& /*resource*/,
    const std::string& /*path*/)
{
    if (!auth_ || !auth_->isEnabled()) {
        return std::nullopt;
    }

    // GAP-001: Enforce scope-based authorization (CWE-862).
    // Extract Bearer token and use auth_->authorize() to check the required
    // permission scope, replacing the previous stub that granted access to any
    // authenticated user without checking their role.
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return makeErrorResponse(http::status::unauthorized, "Authentication required", req);
    }

    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(),static_cast<int>(auth_header.size()))
    );
    if (!token) {
        return makeErrorResponse(http::status::unauthorized, "Invalid authorization header", req);
    }

    auto ar = auth_->authorize(*token, permission);
    if (!ar.authorized) {
        return makeErrorResponse(http::status::forbidden,
                                 "Insufficient permissions for scope: " + permission, req);
    }

    return std::nullopt;
}

AuthContext VectorApiHandler::extractAuthContext([[maybe_unused]] const http::request<http::string_body>& req) const {
    AuthContext ctx;
    
    // Extract from Authorization header
    const auto auth_header = req[http::field::authorization];
    if (!auth_header.empty()) {
        std::string auth_value(auth_header.data(),static_cast<int>(auth_header.size()));
        // Simple extraction - in real impl, parse JWT or other tokens
        // For now, just extract basic info from headers
    }
    
    // Extract from custom headers
    auto user_id_header = req.find("X-User-ID");
    if (user_id_header != req.end()) {
        ctx.user_id = std::string(user_id_header->value());
    }
    
    auto tenant_id_header = req.find("X-Tenant-ID");
    if (tenant_id_header != req.end()) {
        ctx.tenant_id = std::string(tenant_id_header->value());
    }
    
    return ctx;
}

} // namespace server
} // namespace themis


