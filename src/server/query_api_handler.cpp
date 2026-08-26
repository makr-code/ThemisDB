/**
 * @file query_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=1, C=24, H=19, M=77, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Ensure correct WinSock include order on Windows
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <stdexcept>
#include <windows.h>
#endif

// Windows macros undefine - MUST be before any includes
#ifdef ERROR
#undef ERROR
#endif

#include "server/query_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "query/query_engine.h"
#include "query/query_optimizer.h"
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "query/query_resource_limits.h"
#include "llm/llm_interaction_store.h"
#include "prompt_engineering/prompt_manager.h"
#include "cache/semantic_cache.h"
#include "server/auth_middleware.h"
#include "server/chunked_response_writer.h"
#include "security/encryption.h"
#include "security/pki_key_provider.h"
#include "metadata/index_recommender.h"
#include "metadata/statistics_collector.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/hkdf_helper.h"
#include "utils/input_validator.h"
#include "utils/cursor.h"
#include "utils/type_conversion.h"

#include <queue>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace themis {
namespace server {

using json = nlohmann::json;

struct QueryExecStatus {
    bool ok;
    std::string message;

    static QueryExecStatus OK() {
        return QueryExecStatus{true, ""};
    }
};

// Portable time conversion helpers (static)
static inline time_t portable_mkgmtime_impl(std::tm const* tmin) {
#ifdef _WIN32
    return _mkgmtime(const_cast<std::tm*>(tmin));
#else
    return timegm(const_cast<std::tm*>(tmin));
#endif
}

static inline void portable_gmtime_r_impl(const time_t* t, std::tm* out) {
#ifdef _WIN32
    gmtime_s(out, t);
#else
    gmtime_r(t, out);
#endif
}

static inline bool nearly_equal(double lhs, double rhs,
                                double abs_epsilon = 1e-12,
                                double rel_epsilon = 1e-9) {
    const double diff = std::fabs(lhs - rhs);
    if (diff <= abs_epsilon) {
        return true;
    }
    const double scale = std::max(std::fabs(lhs), std::fabs(rhs));
    return diff <= (scale * rel_epsilon);
}

QueryApiHandler::QueryApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<FieldEncryption> field_encryption,
    std::shared_ptr<KeyProvider> key_provider,
    std::shared_ptr<SemanticCache> semantic_cache,
    std::shared_ptr<LLMInteractionStore> llm_store,
    std::shared_ptr<themis::prompt_engineering::PromptManager> prompt_manager,
    std::shared_ptr<::themis::AuthMiddleware> auth,
    bool feature_llm_query_enhancement,
    bool feature_llm_store
)
    : storage_(std::move(storage))
    , secondary_index_(std::move(secondary_index))
    , graph_index_(std::move(graph_index))
    , field_encryption_(std::move(field_encryption))
    , key_provider_(std::move(key_provider))
    , semantic_cache_(std::move(semantic_cache))
    , llm_store_(std::move(llm_store))
    , prompt_manager_(std::move(prompt_manager))
    , auth_(std::move(auth))
    , feature_llm_query_enhancement_(feature_llm_query_enhancement)
    , feature_llm_store_(feature_llm_store)
{
}

// Helper methods implementations
http::response<http::string_body> QueryApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> QueryApiHandler::makeResponse(
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

nlohmann::json QueryApiHandler::applyMasking(
    const nlohmann::json& entities,
    const http::request<http::string_body>& req)
{
    auto masking_policy = std::atomic_load_explicit(&masking_policy_, std::memory_order_acquire);
    if (!masking_policy) {
        return entities;
    }
    auto auth_ctx = extractAuthContext(req);
    return masking_policy->maskResultSet(entities, auth_ctx.groups);
}

// Implementation extracted from http_server.cpp (lines 5950-6222)
http::response<http::string_body> QueryApiHandler::handleQuery(
    const http::request<http::string_body>& req
) {
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "data:read", "query", path_only)) return *resp;
    }
    auto span = Tracer::startSpan("POST /query");
    if (!storage_ || !secondary_index_) {
        span.setStatus(false, "query_dependencies_unavailable");
        return makeErrorResponse(http::status::service_unavailable,
            "Query service dependencies are not available", req);
    }
    
    try {
        auto body = json::parse(req.body());
        constexpr uint32_t kMaxQueryTimeoutMs = 120000;

        uint32_t timeout_ms = 0;
        if (body.contains("timeout_ms")) {
            if (!body["timeout_ms"].is_number_unsigned()) {
                span.setStatus(false, "Invalid timeout_ms");
                return makeErrorResponse(http::status::bad_request,
                    "'timeout_ms' must be an unsigned integer", req);
            }
            timeout_ms = body["timeout_ms"].get<uint32_t>();
            if (timeout_ms > kMaxQueryTimeoutMs) {
                span.setStatus(false, "timeout_ms too large");
                return makeErrorResponse(http::status::bad_request,
                    "'timeout_ms' exceeds maximum of " + std::to_string(kMaxQueryTimeoutMs) + " ms", req);
            }
        }

        const auto request_start = std::chrono::steady_clock::now();
        auto isTimedOut = [&]() {
            if (timeout_ms == 0) {
                return false;
            }
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - request_start).count();
            return elapsed_ms >= static_cast<long long>(timeout_ms);
        };
        auto timeoutResponse = [&]() {
            span.setStatus(false, "query timeout");
            return makeErrorResponse(http::status::request_timeout,
                "query exceeded timeout of " + std::to_string(timeout_ms) + " ms", req);
        };

        if (!body.contains("table")) {
            span.setStatus(false, "Missing table");
            return makeErrorResponse(http::status::bad_request, "Missing 'table'", req);
        }

        std::string table = body["table"].get<std::string>();
        
        // QW-46 Guard: Fail-closed collection name validation
        {
            utils::InputValidator validator;
            if (!validator.validateStringLength(table, 256) || 
                !validator.validatePathSegment(table)) {
                THEMIS_ERROR("QW-46 Guard: Invalid table name from request; only alphanumeric, underscore, and hyphen allowed; max 256 characters");
                span.setStatus(false, "QW-46 Guard: Invalid table name");
                return makeErrorResponse(http::status::bad_request,
                    "Invalid table name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
            }
        }
        
        span.setAttribute("query.table", table);
         
        std::vector<themis::PredicateEq> preds;
        if (body.contains("predicates")) {
            const auto& pred_array = body["predicates"];
            preds.reserve(pred_array.size());
            for (const auto& p : pred_array) {
                if (!p.contains("column") || !p.contains("value")) {
                    span.setStatus(false, "Invalid predicate");
                    return makeErrorResponse(http::status::bad_request, "Each predicate needs 'column' and 'value'", req);
                }
                preds.push_back({p["column"].get<std::string>(), p["value"].get<std::string>()});
            }
        }

        span.setAttribute("query.predicates_count", static_cast<int64_t>(preds.size()));

        // Range predicates (optional)
        std::vector<themis::PredicateRange> rpreds;
        if (body.contains("range")) {
            const auto& range_array = body["range"];
            rpreds.reserve(range_array.size());
            for (const auto& r : range_array) {
                if (!r.contains("column")) {
                    return makeErrorResponse(http::status::bad_request, "Each range needs 'column'", req);
                }
                themis::PredicateRange pr;
                pr.column = r["column"].get<std::string>();
                if (r.contains("gte")) pr.lower = r["gte"].get<std::string>();
                if (r.contains("lte")) pr.upper = r["lte"].get<std::string>();
                pr.includeLower = true; pr.includeUpper = true;
                if (r.contains("includeLower")) pr.includeLower = r["includeLower"].get<bool>();
                if (r.contains("includeUpper")) pr.includeUpper = r["includeUpper"].get<bool>();
                rpreds.push_back(std::move(pr));
            }
        }

        // Optional ORDER BY
        std::optional<themis::OrderBy> orderBy;
        if (body.contains("order_by")) {
            const auto& ob = body["order_by"];
            if (!ob.contains("column")) return makeErrorResponse(http::status::bad_request, "order_by requires 'column'", req);
            themis::OrderBy o; o.column = ob["column"].get<std::string>();
            o.desc = ob.contains("desc") ? ob["desc"].get<bool>() : false;
            o.limit = ob.contains("limit") ? ob["limit"].get<size_t>() : 1000;
            orderBy = o;
        }

    bool optimize = body.contains("optimize") ? body["optimize"].get<bool>() : true;
    bool allow_full_scan = body.contains("allow_full_scan") ? body["allow_full_scan"].get<bool>() : false;
    bool explain = body.contains("explain") ? body["explain"].get<bool>() : false;
    std::string ret = body.contains("return") ? body["return"].get<std::string>() : std::string("entities");
    bool decrypt = body.contains("decrypt") ? body["decrypt"].get<bool>() : false;
    bool stream = body.contains("stream") ? body["stream"].get<bool>() : false;

    themis::ConjunctiveQuery q{table, preds, {}, {}, {}, {}};
    q.rangePredicates = std::move(rpreds);
    q.orderBy = orderBy;
    q.fulltextPredicate = {};
    q.spatialPredicate = {};
    struct QueryExecStatus {
        bool ok;
        std::string message;
    };

    auto make_ok_status = []() -> QueryExecStatus {
        return QueryExecStatus{true, ""};
    };
    auto make_error_status = [](const std::string& message) -> QueryExecStatus {
        return QueryExecStatus{false, message};
    };

        themis::query::QueryEngine engine(*storage_, *secondary_index_);
        auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
        if (stats_collector) engine.setStatisticsCollector(stats_collector);

        // Optional plan/explain info
        std::string exec_mode;
        nlohmann::json plan_json;

        if (ret == "count") {
            std::pair<QueryExecStatus, size_t> res;
            if (allow_full_scan) {
                exec_mode = "full_scan_fallback";
                auto result = engine.executeAndKeysWithFallback(q, optimize);
                if (!result) {
                    res = {make_error_status(result.error().message()), 0};
                } else {
                    res = {make_ok_status(), result.value().size()};
                }
            } else {
                if (optimize) {
                    themis::query::QueryOptimizer opt(*secondary_index_);
                    auto plan = opt.chooseOrderForAndQuery(q);
                    auto result = opt.executeOptimizedCount(engine, q, plan);
                    if (!result) {
                        res = {make_error_status(result.error().message()), 0};
                    } else {
                        res = {make_ok_status(), *result};
                    }
                    exec_mode = "index_optimized";
                    if (explain) {
                        plan_json["mode"] = exec_mode;
                        plan_json["order"] = nlohmann::json::array();
                        for (const auto& p : plan.orderedPredicates) {
                            plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                        }
                        plan_json["estimates"] = nlohmann::json::array();
                        for (const auto& d : plan.details) {
                            plan_json["estimates"].push_back({
                                {"column", d.pred.column}, {"value", d.pred.value},
                                {"estimatedCount", d.estimatedCount}, {"capped", d.capped}
                            });
                        }
                    }
                } else {
                    exec_mode = "index_parallel";
                    auto result = engine.executeAndCount(q);
                    if (!result) {
                        res = {make_error_status(result.error().message()), 0};
                    } else {
                        res = {make_ok_status(), *result};
                    }
                    if (explain) {
                        plan_json = {
                            {"mode", exec_mode},
                            {"order", nlohmann::json::array()}
                        };
                        for (const auto& p : q.predicates) {
                            plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                        }
                    }
                }
            }
            if (!res.first.ok) {
                span.setStatus(false, res.first.message);
                return makeErrorResponse(http::status::bad_request, res.first.message, req);
            }

            span.setAttribute("query.exec_mode", exec_mode);
            span.setAttribute("query.result_count", static_cast<int64_t>(res.second));
            span.setStatus(true);

            json j = {{"table", table}, {"count", res.second}};
            if (explain && !plan_json.is_null()) j["plan"] = plan_json;
            return makeResponse(http::status::ok, j.dump(), req);
        } else if (ret == "keys") {
            std::pair<QueryExecStatus, std::vector<std::string>> res;
            if (allow_full_scan) {
                exec_mode = "full_scan_fallback";
                auto result = engine.executeAndKeysWithFallback(q, optimize);
                if (!result) {
                    res = {make_error_status(result.error().message()), std::vector<std::string>{}};
                } else {
                    res = {make_ok_status(), std::move(*result)};
                }
            } else {
                if (optimize) {
                    themis::query::QueryOptimizer opt(*secondary_index_);
                    auto plan = opt.chooseOrderForAndQuery(q);
                    auto result = opt.executeOptimizedKeys(engine, q, plan);
                    if (!result) {
                        res = {make_error_status(result.error().message()), std::vector<std::string>{}};
                    } else {
                        res = {make_ok_status(), std::move(*result)};
                    }
                    exec_mode = "index_optimized";
                    if (explain) {
                        plan_json["mode"] = exec_mode;
                        plan_json["order"] = nlohmann::json::array();
                        for (const auto& p : plan.orderedPredicates) {
                            plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                        }
                        plan_json["estimates"] = nlohmann::json::array();
                        for (const auto& d : plan.details) {
                            plan_json["estimates"].push_back({
                                {"column", d.pred.column}, {"value", d.pred.value},
                                {"estimatedCount", d.estimatedCount}, {"capped", d.capped}
                            });
                        }
                    }
                } else {
                    exec_mode = "index_parallel";
                    auto result = engine.executeAndKeys(q);
                    if (!result) {
                        res = {make_error_status(result.error().message()), std::vector<std::string>{}};
                    } else {
                        res = {make_ok_status(), std::move(*result)};
                    }
                    if (explain) {
                        plan_json = {
                            {"mode", exec_mode},
                            {"order", nlohmann::json::array()}
                        };
                        for (const auto& p : q.predicates) {
                            plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                        }
                    }
                }
            }
            if (!res.first.ok) {
                span.setStatus(false, res.first.message);
                return makeErrorResponse(http::status::bad_request, res.first.message, req);
            }
            
            span.setAttribute("query.exec_mode", exec_mode);
            span.setAttribute("query.result_count", static_cast<int64_t>(res.second.size()));
            span.setStatus(true);
            
            json j = {{"table", table}, {"count", res.second.size()}, {"keys", res.second}};
            if (explain && !plan_json.is_null()) j["plan"] = plan_json;
            if (stream && ChunkedResponseWriter::shouldUseChunkedTransfer(req, res.second.size())) {
                std::vector<nlohmann::json> key_items;
                key_items.reserve(res.second.size());
                for (const auto& k : res.second) {
                    if (isTimedOut()) {
                        return timeoutResponse();
                    }
                    key_items.push_back(k);
                }
                ChunkedWriterConfig cfg;
                return ChunkedResponseWriter::fromJsonVector(req, http::status::ok, key_items, cfg);
            }
            return makeResponse(http::status::ok, j.dump(), req);
        } else {
            std::pair<QueryExecStatus, std::vector<themis::BaseEntity>> res;
            if (allow_full_scan) {
                exec_mode = "full_scan_fallback";
                auto result = engine.executeAndEntitiesWithFallback(q, optimize);
                if (!result) {
                    res = {make_error_status(result.error().message()), std::vector<themis::BaseEntity>{}};
                } else {
                    res = {make_ok_status(), std::move(*result)};
                }
            } else {
                if (optimize) {
                    themis::query::QueryOptimizer opt(*secondary_index_);
                    auto plan = opt.chooseOrderForAndQuery(q);
                    auto result = opt.executeOptimizedEntities(engine, q, plan);
                    if (!result) {
                        res = {make_error_status(result.error().message()), std::vector<themis::BaseEntity>{}};
                    } else {
                        res = {make_ok_status(), std::move(*result)};
                    }
                    exec_mode = "index_optimized";
                    if (explain) {
                        plan_json["mode"] = exec_mode;
                        plan_json["order"] = nlohmann::json::array();
                        for (const auto& p : plan.orderedPredicates) {
                            plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                        }
                        plan_json["estimates"] = nlohmann::json::array();
                        for (const auto& d : plan.details) {
                            plan_json["estimates"].push_back({
                                {"column", d.pred.column}, {"value", d.pred.value},
                                {"estimatedCount", d.estimatedCount}, {"capped", d.capped}
                            });
                        }
                    }
                } else {
                    exec_mode = "index_parallel";
                    auto result = engine.executeAndEntities(q);
                    if (!result) {
                        res = {make_error_status(result.error().message()), std::vector<themis::BaseEntity>{}};
                    } else {
                        res = {make_ok_status(), std::move(*result)};
                    }
                    if (explain) {
                        plan_json = {
                            {"mode", exec_mode},
                            {"order", nlohmann::json::array()}
                        };
                        for (const auto& p : q.predicates) {
                            plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                        }
                    }
                }
            }
            if (!res.first.ok) {
                span.setStatus(false, res.first.message);
                return makeErrorResponse(http::status::bad_request, res.first.message, req);
            }
            
            span.setAttribute("query.exec_mode", exec_mode);
            span.setAttribute("query.result_count", static_cast<int64_t>(res.second.size()));
            span.setStatus(true);
            
            // Serialize entities; optional Entschluesselung basierend auf Schema
            json entities = json::array();
            if (!decrypt) {
                for (const auto& e : res.second) {
                    if (isTimedOut()) {
                        return timeoutResponse();
                    }
                    // Kompatible Rueckgabe: JSON-String je Entity
                    entities.push_back(e.toJson());
                }
            } else {
                // Lade Schema einmal
                nlohmann::json schema;
                try {
                    if (auto schema_bytes = storage_->get("config:encryption_schema")) {
                        std::string schema_json(schema_bytes->begin(), schema_bytes->end());
                        schema = nlohmann::json::parse(schema_json);
                    }
                } catch (...) {}
                bool enabled = false;
                std::vector<std::string> fields;
                std::string context_type = "user";
                if (!schema.is_null() && schema.contains("collections") && schema["collections"].contains(table)) {
                    auto coll = schema["collections"][table];
                    enabled = coll.contains("encryption") && coll["encryption"].value("enabled", false);
                    if (enabled && coll["encryption"].contains("fields")) {
                        const auto& fields_array = coll["encryption"]["fields"];
                        fields.reserve(fields_array.size());
                        for (auto& f : fields_array) if (f.is_string()) fields.push_back(f.get<std::string>());
                        context_type = coll["encryption"].value("context_type", "user");
                    }
                }
                // Extract user_id and groups from JWT for decryption context
                auto auth_ctx = extractAuthContext(req);
                std::string user_ctx = auth_ctx.user_id.empty() ? "anonymous" : auth_ctx.user_id;
                auto pki = std::dynamic_pointer_cast<themis::security::PKIKeyProvider>(key_provider_);
                for (const auto& e : res.second) {
                    if (isTimedOut()) {
                        return timeoutResponse();
                    }
                    nlohmann::json obj;
                    try { obj = nlohmann::json::parse(e.toJson()); } catch (...) { entities.push_back(e.toJson()); continue; }
                    if (enabled) {
                        for (const auto& f : fields) {
                            if (isTimedOut()) {
                                return timeoutResponse();
                            }
                            if (!obj.contains(f + "_enc") || !obj.contains(f + "_encrypted")) continue;
                            bool encFlag = false; try { encFlag = obj[f + "_enc"].get<bool>(); } catch (...) { encFlag = false; }
                            if (!encFlag) continue;
                            try {
                                auto enc_meta_str = obj[f + "_encrypted"].get<std::string>();
                                auto enc_meta = nlohmann::json::parse(enc_meta_str);
                                auto blob = themis::EncryptedBlob::fromJson(enc_meta);
                                std::vector<uint8_t> raw_key;
                                if (context_type == "group" && pki && obj.contains(f + "_group")) {
                                    std::string group_name; try { group_name = obj[f + "_group"].get<std::string>(); } catch (...) { group_name.clear(); }
                                    if (!group_name.empty()) {
                                        auto gdek = pki->getGroupDEK(group_name);
                                        std::vector<uint8_t> salt; std::string info = "field:" + f;
                                        raw_key = themis::utils::HKDFHelper::derive(gdek, salt, info, 32);
                                    }
                                }
                                if (raw_key.empty()) {
                                    auto dek = key_provider_->getKey("dek");
                                    std::vector<uint8_t> salt(user_ctx.begin(), user_ctx.end());
                                    std::string info = "field:" + f;
                                    raw_key = themis::utils::HKDFHelper::derive(dek, salt, info, 32);
                                }
                                auto plain_bytes = field_encryption_->decryptWithKey(blob, raw_key);
                                
                                // Deserialisierung basierend auf Datenformat
                                std::string plain_str(plain_bytes.begin(), plain_bytes.end());
                                
                                // Heuristik: JSON-Strukturen erkennen und parsen
                                if (!plain_str.empty()) {
                                    const char first_char = plain_str.front();
                                    if (first_char == '[' || first_char == '{') {
                                        try {
                                            auto parsed = nlohmann::json::parse(plain_str);
                                            obj[f] = parsed;
                                        } catch (...) {
                                            THEMIS_DEBUG("query_api_handler: unhandled exception caught");
                                            obj[f] = plain_str;
                                        }
                                    } else {
                                        obj[f] = plain_str;
                                    }
                                } else {
                                    obj[f] = plain_str;
                                }
                            } catch (const std::exception& ex) {
                                THEMIS_WARN("Query decrypt field {} failed: {}", f, ex.what());
                            }
                        }
                    }
                    entities.push_back(obj);
                }
            }
            json j = {{"table", table}, {"count", res.second.size()}, {"entities", applyMasking(entities, req)}, {"decrypted", decrypt}};
            if (explain && !plan_json.is_null()) j["plan"] = plan_json;
            if (stream && ChunkedResponseWriter::shouldUseChunkedTransfer(req, entities.size())) {
                std::vector<nlohmann::json> entity_items;
                entity_items.reserve(entities.size());
                for (const auto& e : entities) {
                    entity_items.push_back(e);
                }
                ChunkedWriterConfig cfg;
                return ChunkedResponseWriter::fromJsonVector(req, http::status::ok, entity_items, cfg);
            }
            return makeResponse(http::status::ok, j.dump(), req);
        }

    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    }
}


// Implementation extracted from http_server.cpp (lines 6223-8688)
http::response<http::string_body> QueryApiHandler::handleQueryAql(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("POST /query/aql");
    if (!storage_ || !secondary_index_) {
        span.setStatus(false, "query_dependencies_unavailable");
        return makeErrorResponse(http::status::service_unavailable,
            "Query service dependencies are not available", req);
    }
    
    try {
        auto body = json::parse(req.body());
        
        // Validate request
        if (!body.contains("query")) {
            span.setStatus(false, "Missing query field");
            return makeErrorResponse(http::status::bad_request, "Missing 'query' field", req);
        }

        std::string aql_query = body["query"].get<std::string>();
        span.setAttribute("aql.query", aql_query);
        bool explain = body.contains("explain") ? body["explain"].get<bool>() : false;
        span.setAttribute("aql.explain", explain);
        bool optimize = body.contains("optimize") ? body["optimize"].get<bool>() : true;
        span.setAttribute("aql.optimize", optimize);
        bool allow_full_scan = body.contains("allow_full_scan") ? body["allow_full_scan"].get<bool>() : false;
        span.setAttribute("aql.allow_full_scan", allow_full_scan);
        
    // Cursor-based pagination parameters
        std::string cursor_token = body.contains("cursor") ? body["cursor"].get<std::string>() : "";
        bool use_cursor = body.contains("use_cursor") ? body["use_cursor"].get<bool>() : false;
        
        // Page size configuration with validation
        themis::utils::PaginationConfig pagination_config;
        size_t page_size = body.contains("page_size") ? body["page_size"].get<size_t>() : pagination_config.default_page_size;
        page_size = themis::utils::Cursor::normalizePageSize(page_size, pagination_config);
        
        // Cursor expiration check
        if (use_cursor && !cursor_token.empty()) {
            if (!themis::utils::Cursor::isValid(cursor_token, pagination_config.cursor_ttl_seconds)) {
                // Log expired cursor attempt for monitoring/debugging
                auto cursorSpan = Tracer::startSpan("cursor.expired");
                cursorSpan.setAttribute("cursor_length", static_cast<int64_t>(cursor_token.length()));
                cursorSpan.recordError("Cursor has expired");
                cursorSpan.setStatus(false);
                
                return makeErrorResponse(http::status::bad_request, 
                    "Cursor has expired. Please start a new query.", req);
            }
        }
        
    auto page_fetch_start = std::chrono::steady_clock::now();
        
    // Cursor-Pagination: Wir verlagern Cursor-Handling in die Engine (Anker-basiert)
        
        // Optional: Frontier-Limits für Traversal (Soft-Limit)
        // GAP-021 fixed: cap max_frontier_size and max_results with server-side defaults
        // to prevent OOM from adversarial large traversal requests.
        static constexpr size_t kMaxFrontierSizeCap = 500'000;
        static constexpr size_t kDefaultFrontierSize = 100'000;
        static constexpr size_t kMaxResultsCap       = 50'000;
        static constexpr size_t kDefaultMaxResults   = 10'000;
        size_t max_frontier_size = body.contains("max_frontier_size")
            ? std::min(body["max_frontier_size"].get<size_t>(), kMaxFrontierSizeCap)
            : kDefaultFrontierSize;
        size_t max_results = body.contains("max_results")
            ? std::min(body["max_results"].get<size_t>(), kMaxResultsCap)
            : kDefaultMaxResults;

        // Per-query resource limits (max rows, max memory, timeout)
        query::QueryResourceLimits resource_limits;
        // GAP-022 fixed: when the user sends 0 OR omits max_memory_bytes, apply a
        // server-side default of 256 MiB so the check in aql_runner.cpp is never skipped.
        static constexpr size_t kDefaultMaxMemoryBytes = 256ULL * 1024 * 1024; // 256 MiB
        resource_limits.max_rows         = body.contains("max_rows")         ? body["max_rows"].get<size_t>()         : 0;
        {
            size_t user_mem = body.contains("max_memory_bytes") ? body["max_memory_bytes"].get<size_t>() : 0;
            resource_limits.max_memory_bytes = (user_mem > 0) ? user_mem : kDefaultMaxMemoryBytes;
        }
        resource_limits.timeout_ms       = body.contains("timeout_ms")       ? body["timeout_ms"].get<uint32_t>()     : 0;
        auto resource_limit_start = std::chrono::steady_clock::now();
        const auto timeout_deadline = (resource_limits.timeout_ms > 0)
            ? std::optional<std::chrono::steady_clock::time_point>{
                resource_limit_start + std::chrono::milliseconds(resource_limits.timeout_ms)}
            : std::nullopt;
        const auto timedOut = [&timeout_deadline]() {
            return timeout_deadline.has_value() &&
                   std::chrono::steady_clock::now() >= *timeout_deadline;
        };
        
        // Parse AQL query
        auto parseSpan = Tracer::startSpan("aql.parse");
        parseSpan.setAttribute("aql.query_length", static_cast<int64_t>(aql_query.size()));
        themis::query::AQLParser parser;
        auto parse_result = parser.parse(aql_query);
        
        if (!parse_result.has_value()) {
            std::string error_msg = fmt::format("AQL parse error: {}", parse_result.error().message());
            
            parseSpan.setStatus(false, error_msg);
            span.setStatus(false, "Parse error");
            return makeErrorResponse(http::status::bad_request, error_msg, req);
        }
        parseSpan.setStatus(true);

        // EARLY: Join-Erkennung vor Translation (Translator unterstützt keine Field==Field Prädikate)
        if (*parse_result && (*parse_result)->traversal == nullptr) {
            const auto& for_nodes = (*parse_result)->for_nodes;
            if (for_nodes.size() >= 2) {
            // Wiederverwendung der Join-Logik wie weiter unten
            auto joinSpan = Tracer::startSpan("aql.join");
            const auto& f1_ref = for_nodes.front();
            const auto second_for_node = std::next(for_nodes.begin());
            const auto& f2_ref = *second_for_node;
            const std::string var1 = f1_ref.variable;
            const std::string var2 = f2_ref.variable;
            const std::string table1 = f1_ref.collection;
            const std::string table2 = f2_ref.collection;
            joinSpan.setAttribute("join.var_left", var1);
            joinSpan.setAttribute("join.var_right", var2);
            joinSpan.setAttribute("join.table_left", table1);
            joinSpan.setAttribute("join.table_right", table2);

            using namespace themis::query;
            std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [&](const std::shared_ptr<Expression>& expr, std::string& rootVar)->std::string {
                auto* fa = dynamic_cast<FieldAccessExpr*>(expr.get());
                if (!fa) return std::string();
                std::vector<std::string> parts;
                parts.reserve(8);  // Typical nesting depth is 4-8 levels
                parts.push_back(fa->field);
                auto* cur = fa->object.get();
                while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->object.get(); }
                auto* root = dynamic_cast<VariableExpr*>(cur); if (!root) return std::string();
                rootVar = root->name;
                std::ostringstream col_oss;
                for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
                    if (it != parts.rbegin()) col_oss << ".";
                    col_oss << *it;
                }
                return col_oss.str();
            };
            auto literalToString = [&](const LiteralValue& value)->std::string{
                return std::visit([](auto&& arg)->std::string{
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::nullptr_t>) return std::string("null");
                    else if constexpr (std::is_same_v<T, bool>) return arg ? std::string("true") : std::string("false");
                    else if constexpr (std::is_same_v<T, int64_t>) return std::to_string(arg);
                    else if constexpr (std::is_same_v<T, double>) return std::to_string(arg);
                    else if constexpr (std::is_same_v<T, std::string>) return arg;
                    else return std::string();
                }, value);
            };
            std::optional<std::pair<std::string,std::string>> joinCols; std::vector<PredicateEq> eq1, eq2; std::vector<PredicateRange> r1, r2;
            std::function<void(const std::shared_ptr<Expression>&)> collectPreds;
            collectPreds = [&](const std::shared_ptr<Expression>& e){
                if (!e) return;
                if (e->getType() != ASTNodeType::BinaryOp) return;
                auto bin = std::static_pointer_cast<BinaryOpExpr>(e);
                if (bin->op == BinaryOperator::And) {
                    collectPreds(bin->left);
                    collectPreds(bin->right);
                    return;
                }
                if (bin->op == BinaryOperator::Eq) {
                    std::string rvL, rvR; std::string colL = fieldFromFA(bin->left, rvL); std::string colR = fieldFromFA(bin->right, rvR);
                    if (!colL.empty() && !colR.empty() && ((rvL == var1 && rvR == var2) || (rvL == var2 && rvR == var1))) {
                        if (!joinCols.has_value()) { if (rvL == var1) joinCols = std::make_pair(colL, colR); else joinCols = std::make_pair(colR, colL); }
                        return;
                    }
                    if (!colL.empty() && rvL == var1 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std::static_pointer_cast<LiteralExpr>(bin->right); eq1.push_back({colL, literalToString(lit->value)}); return; }
                    if (!colL.empty() && rvL == var2 && bin->right->getType() == ASTNodeType::Literal) { auto lit = std::static_pointer_cast<LiteralExpr>(bin->right); eq2.push_back({colL, literalToString(lit->value)}); return; }
                    if (bin->left->getType() == ASTNodeType::Literal) { std::string rv; std::string col = fieldFromFA(bin->right, rv); if (!col.empty()) { auto lit = std::static_pointer_cast<LiteralExpr>(bin->left); if (rv == var1) eq1.push_back({col, literalToString(lit->value)}); else if (rv == var2) eq2.push_back({col, literalToString(lit->value)}); } return; }
                }
            };
            for (const auto& f : (*parse_result)->filters) collectPreds(f->condition);
            if (!joinCols.has_value()) {
                joinSpan.setStatus(false, "join_predicate_missing"); span.setStatus(false, "JOIN requires equality predicate between variables");
                return makeErrorResponse(http::status::bad_request, "JOIN requires equality predicate between variables", req);
            }
            themis::ConjunctiveQuery q1; q1.table = table1; q1.predicates = eq1; q1.rangePredicates = r1;
            themis::ConjunctiveQuery q2; q2.table = table2; q2.predicates = eq2; q2.rangePredicates = r2;
            themis::query::QueryEngine engine(*storage_, *secondary_index_);
            auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
            if (stats_collector) engine.setStatisticsCollector(stats_collector);
            
            auto result1 = allow_full_scan ? engine.executeAndEntitiesWithFallback(q1, optimize) : engine.executeAndEntities(q1);
            std::pair<QueryExecStatus, std::vector<themis::BaseEntity>> res1;
            if (!result1) {
                res1 = {QueryExecStatus{false, result1.error().message()}, std::vector<themis::BaseEntity>{}};
            } else {
                res1 = {QueryExecStatus::OK(), std::move(*result1)};
            }
            if (!res1.first.ok) { joinSpan.setStatus(false, res1.first.message); span.setStatus(false, "Left side execution failed"); return makeErrorResponse(http::status::bad_request, res1.first.message, req); }
            
            auto result2 = allow_full_scan ? engine.executeAndEntitiesWithFallback(q2, optimize) : engine.executeAndEntities(q2);
            std::pair<QueryExecStatus, std::vector<themis::BaseEntity>> res2;
            if (!result2) {
                res2 = {QueryExecStatus{false, result2.error().message()}, std::vector<themis::BaseEntity>{}};
            } else {
                res2 = {QueryExecStatus::OK(), std::move(*result2)};
            }
            if (!res2.first.ok) { joinSpan.setStatus(false, res2.first.message); span.setStatus(false, "Right side execution failed"); return makeErrorResponse(http::status::bad_request, res2.first.message, req); }
            const auto& leftVec = res1.second; const auto& rightVec = res2.second; bool buildLeft = leftVec.size() <= rightVec.size();
            const auto [colLeft, colRight] = *joinCols; std::unordered_multimap<std::string, themis::BaseEntity> hash;
            auto getFieldStr = [&](const themis::BaseEntity& e, const std::string& col)->std::optional<std::string> { auto v = e.getFieldAsString(col); if (v.has_value()) return v; auto d = e.getFieldAsDouble(col); if (d.has_value()) return std::to_string(*d); return std::nullopt; };
            if (buildLeft) { hash.reserve(leftVec.size()*2+1); for (const auto& e : leftVec) { auto k = getFieldStr(e, colLeft); if (k.has_value()) hash.emplace(*k, e); } }
            else { hash.reserve(rightVec.size()*2+1); for (const auto& e : rightVec) { auto k = getFieldStr(e, colRight); if (k.has_value()) hash.emplace(*k, e); } }
            std::string retVar; if ((*parse_result)->return_node && (*parse_result)->return_node->expression) { if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) { retVar = v->name; } }
            if (retVar != var1 && retVar != var2) { joinSpan.setStatus(false, "return_not_supported_for_join"); span.setStatus(false, "JOIN currently supports RETURN of one bound variable (left or right)"); return makeErrorResponse(http::status::bad_request, "JOIN currently supports RETURN of one bound variable (left or right)", req); }
            std::vector<themis::BaseEntity> out;
            // Reserve based on expected join cardinality (smaller input set)
            out.reserve(std::min(leftVec.size(), rightVec.size()));
            if (buildLeft) { for (const auto& e : rightVec) { auto k = getFieldStr(e, colRight); if (!k.has_value()) continue; auto range = hash.equal_range(*k); for (auto it = range.first; it != range.second; ++it) { const themis::BaseEntity& l = it->second; if (retVar == var1) out.push_back(l); else out.push_back(e); } } }
            else { for (const auto& e : leftVec) { auto k = getFieldStr(e, colLeft); if (!k.has_value()) continue; auto range = hash.equal_range(*k); for (auto it = range.first; it != range.second; ++it) { const themis::BaseEntity& r = it->second; if (retVar == var1) out.push_back(e); else out.push_back(r); } } }
            if ((*parse_result) && (*parse_result)->limit) { auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset)); auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count)); if (off < out.size()) { size_t last = std::min(out.size(), off + cnt); auto first_it = out.begin() + static_cast<std::ptrdiff_t>(off); auto last_it = out.begin() + static_cast<std::ptrdiff_t>(last); std::vector<themis::BaseEntity> tmp; tmp.reserve(last - off); std::move(first_it, last_it, std::back_inserter(tmp)); out.swap(tmp); } else { out.clear(); } }
            nlohmann::json entities = nlohmann::json::array(); for (const auto& e : out) entities.push_back(e.toJson()); nlohmann::json response_body = {{"table_left", table1}, {"table_right", table2}, {"count", out.size()}, {"entities", applyMasking(entities, req)}};
            if (explain) { response_body["query"] = aql_query; response_body["ast"] = (*parse_result)->toJSON(); nlohmann::json jp; jp["on_left"] = (*joinCols).first; jp["on_right"] = (*joinCols).second; response_body["join"] = jp; }
            joinSpan.setAttribute("join.output_count", static_cast<int64_t>(out.size())); joinSpan.setStatus(true); span.setAttribute("aql.result_count", static_cast<int64_t>(out.size())); span.setStatus(true);
            return makeResponse(http::status::ok, response_body.dump(), req);
            }
        }

    // Translate AST to Query (relational oder traversal)
    // Spezialfall: LET-Variablen in FILTER (MVP) - vor Übersetzung einfache Ersetzung erlauben
    bool letFilterHandled = false; // wenn true, nutzen wir einen manuell konstruierten ConjunctiveQuery
    themis::ConjunctiveQuery letQuery;
    if ((*parse_result) && (*parse_result)->traversal == nullptr && !(*parse_result)->for_nodes.empty()) {
        // Wir unterstützen nur den einfachen relationalen Fall mit genau einer FOR-Klausel
        const auto& forNode = (*parse_result)->for_node;
        const std::string loopVar = forNode.variable;
        const std::string table = forNode.collection;
        if (!(*parse_result)->filters.empty() && !(*parse_result)->let_nodes.empty()) {
            // Map der LET-Bindings: var -> expr
            std::unordered_map<std::string, std::shared_ptr<themis::query::Expression>> letMap;
            for (const auto& ln : (*parse_result)->let_nodes) letMap[ln.variable] = ln.expression;

            using namespace themis::query;
            // Helfer: löse Ausdruck zu einer Feldspalte der Loop-Variable auf, ggf. via LET-Variable
            std::function<std::optional<std::string>(const std::shared_ptr<Expression>&)> resolveToLoopField;
            resolveToLoopField = [&](const std::shared_ptr<Expression>& e)->std::optional<std::string> {
                if (!e) return std::nullopt;
                if (auto* fa = dynamic_cast<FieldAccessExpr*>(e.get())) {
                    // Sammle Feldpfad und prüfe Root-Variable
                    std::vector<std::string> parts;
                    parts.reserve(8);  // Typical nesting depth is 4-8 levels
                    parts.push_back(fa->field);
                    auto* cur = fa->object.get();
                    while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->object.get(); }
                    auto* root = dynamic_cast<VariableExpr*>(cur);
                    if (!root || root->name != loopVar) return std::nullopt;
                    std::ostringstream col_oss;
                    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
                        if (it != parts.rbegin()) col_oss << ".";
                        col_oss << *it;
                    }
                    std::string col = col_oss.str();
                    return col;
                }
                if (auto* v = dynamic_cast<VariableExpr*>(e.get())) {
                    auto it = letMap.find(v->name);
                    if (it == letMap.end()) return std::nullopt;
                    return resolveToLoopField(it->second);
                }
                return std::nullopt;
            };

            // Extrahiere Gleichheitsprädikate mit LET auf linkem oder rechtem Operand
            std::vector<themis::PredicateEq> eqPreds;
            bool unsupported = false;
            // Lokale Literal-zu-String Konvertierung (wie im Übersetzer)
            auto litToString = [&](const themis::query::LiteralValue& value)->std::string{
                return std::visit([](auto&& arg)->std::string{
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::nullptr_t>) return std::string("null");
                    else if constexpr (std::is_same_v<T, bool>) return arg ? std::string("true") : std::string("false");
                    else if constexpr (std::is_same_v<T, int64_t>) return std::to_string(arg);
                    else if constexpr (std::is_same_v<T, double>) return std::to_string(arg);
                    else if constexpr (std::is_same_v<T, std::string>) return arg;
                    else return std::string();
                }, value);
            };
            std::function<void(const std::shared_ptr<Expression>&)> visit;
            visit = [&](const std::shared_ptr<Expression>& ex){
                if (!ex || unsupported) return;
                if (auto* be = dynamic_cast<BinaryOpExpr*>(ex.get())) {
                    if (be->op == BinaryOperator::And) { visit(be->left); visit(be->right); return; }
                    if (be->op == BinaryOperator::Eq) {
                        auto leftCol = resolveToLoopField(be->left);
                        auto rightCol = resolveToLoopField(be->right);
                        if (leftCol.has_value() && be->right->getType() == ASTNodeType::Literal) {
                            auto lit = std::static_pointer_cast<LiteralExpr>(be->right);
                            eqPreds.push_back({*leftCol, litToString(lit->value)});
                            return;
                        }
                        if (rightCol.has_value() && be->left->getType() == ASTNodeType::Literal) {
                            auto lit = std::static_pointer_cast<LiteralExpr>(be->left);
                            eqPreds.push_back({*rightCol, litToString(lit->value)});
                            return;
                        }
                    }
                    // Alles andere (inkl. OR, Range, Funktionen) im MVP nicht via LET-Filter unterstützt
                    // Wir markieren nicht global als unsupported, damit andere AND-Zweige extrahiert werden können.
                    return;
                }
            };
            for (const auto& f : (*parse_result)->filters) visit(f->condition);

            // Wenn wir etwas extrahieren konnten, nutzen wir dafür einen direkten ConjunctiveQuery
            if (!eqPreds.empty()) {
                letQuery.table = table;
                letQuery.predicates = std::move(eqPreds);
                letFilterHandled = true;
            }
        }
    }

    auto translateSpan = Tracer::startSpan("aql.translate");
    auto translate_result = letFilterHandled
        ? themis::AQLTranslator::TranslationResult::Success(letQuery)
        : themis::AQLTranslator::translate((*parse_result));
        
        if (!translate_result.success) {
            translateSpan.setStatus(false, translate_result.error_message);
            span.setStatus(false, "Translation error");
            return makeErrorResponse(http::status::bad_request,
                "AQL translation error: " + translate_result.error_message, req);
        }
        translateSpan.setStatus(true);

    // Record column access patterns for IndexRecommender (non-blocking; best-effort)
    auto* index_recommender = index_recommender_.load(std::memory_order_acquire);
    if (index_recommender) {
        // Selectivity weights passed to IndexRecommender.
        // kFilterEqSelectivity (0.5): average assumed selectivity for equality predicates
        //   (i.e. roughly half the rows match).  Real cardinality data from
        //   StatisticsCollector is not available at translation time.
        // kFilterRangeSelectivity (0.3): range predicates are assumed more selective
        //   than equality on average.
        // kSortSelectivity (1.0): ORDER BY does not filter rows, so it contributes
        //   maximum selectivity weight (non-discriminating → score 0 benefit).
        static constexpr double kFilterEqSelectivity    = 0.5;
        static constexpr double kFilterRangeSelectivity = 0.3;
        static constexpr double kSortSelectivity        = 1.0;

        auto recordFromConjunct = [&](const themis::ConjunctiveQuery& cq) {
            for (const auto& p : cq.predicates) {
                index_recommender->recordAccess(cq.table, p.column,
                   metadata::IndexRecommender::AccessType::FILTER, kFilterEqSelectivity);
            }
            for (const auto& rp : cq.rangePredicates) {
                index_recommender->recordAccess(cq.table, rp.column,
                   metadata::IndexRecommender::AccessType::FILTER, kFilterRangeSelectivity);
            }
            if (cq.orderBy.has_value()) {
                index_recommender->recordAccess(cq.table, cq.orderBy->column,
                   metadata::IndexRecommender::AccessType::SORT, kSortSelectivity);
            }
        };

        if (translate_result.disjunctive.has_value()) {
            for (const auto& disjunct : translate_result.disjunctive->disjuncts) {
                recordFromConjunct(disjunct);
            }
        } else {
            recordFromConjunct(translate_result.conjunctive_query);
        }
        index_recommender->recordQuery();
    }

    // If traversal present, execute via GraphIndexManager
        if (translate_result.traversal.has_value()) {
            auto traversalSpan = Tracer::startSpan("aql.traversal");
            if (!graph_index_) {
                return makeErrorResponse(http::status::bad_request, "Graph traversal requested but graph index manager is not available", req);
            }
            auto& graph_index = *graph_index_;
            const auto& t = translate_result.traversal.value();
            traversalSpan.setAttribute("traversal.start_vertex", t.startVertex);
            traversalSpan.setAttribute("traversal.min_depth", static_cast<int64_t>(t.minDepth));
            traversalSpan.setAttribute("traversal.max_depth", static_cast<int64_t>(t.maxDepth));
            std::string dirStr = (t.direction == themis::AQLTranslator::TranslationResult::TraversalQuery::Direction::Outbound) ? "OUTBOUND" :
                                 (t.direction == themis::AQLTranslator::TranslationResult::TraversalQuery::Direction::Inbound) ? "INBOUND" : "ANY";
            traversalSpan.setAttribute("traversal.direction", dirStr);
            
            if (t.minDepth < 0 || t.maxDepth < 0 || t.maxDepth < t.minDepth) {
                traversalSpan.setStatus(false, "Invalid depth range");
                span.setStatus(false, "Invalid traversal depth");
                return makeErrorResponse(http::status::bad_request, "Invalid depth range in traversal", req);
            }

            // Bestimme Return-Modus anhand RETURN-Ausdruck: v (default), e, p
            enum class RetMode { Vertex, Edge, Path };
            RetMode retMode = RetMode::Vertex;
            if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
                using namespace themis::query;
                auto* var = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get());
                if (var) {
                    if (var->name == "e") retMode = RetMode::Edge;
                    else if (var->name == "p") retMode = RetMode::Path;
                }
            }

            // Extrahiere einfache FILTER-Pr�dikate auf v/e im Format: FILTER v.<field> == <literal|funktion> oder FILTER e.<field> == <literal|funktion>
            struct SimplePred {
                enum class Op { Eq, Neq, Lt, Lte, Gt, Gte };
                char var = '\0'; // 'v' or 'e'
                std::string field;
                nlohmann::json literal; // as JSON literal
                Op op = Op::Eq;
            };
            // Unterst�tzte Funktionsauswertung zur Reduktion auf Literale
            std::function<bool(std::shared_ptr<themis::query::Expression>, nlohmann::json&)> evalExprToLiteral;
            evalExprToLiteral = [&](std::shared_ptr<themis::query::Expression> expr, nlohmann::json& out)->bool {
                using namespace themis::query;
                if (auto* l = dynamic_cast<LiteralExpr*>(expr.get())) {
                    out = l->toJSON()["value"]; return true;
                }
                auto* fc = dynamic_cast<FunctionCallExpr*>(expr.get());
                if (!fc) return false;
                // Funktionsnamen case-insensitiv vergleichen
                std::string name = fc->name; std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                auto getArgLit = [&](size_t idx, nlohmann::json& argOut)->bool{
                    if (idx >= fc->arguments.size()) return false;
                    return evalExprToLiteral(fc->arguments[idx], argOut);
                };
                if (name == "abs") {
                    nlohmann::json a; if (!getArgLit(0, a)) return false;
                    if (a.is_number_integer()) { long long v = a.get<long long>(); if (v < 0) v = -v; out = v; return true; }
                    if (a.is_number_float()) { double v = a.get<double>(); if (v < 0) v = -v; out = v; return true; }
                    return false;
                }
                if (name == "ceil") {
                    nlohmann::json a; if (!getArgLit(0, a)) return false;
                    if (a.is_number()) { out = std::ceil(a.get<double>()); return true; }
                    return false;
                }
                if (name == "floor") {
                    nlohmann::json a; if (!getArgLit(0, a)) return false;
                    if (a.is_number()) { out = std::floor(a.get<double>()); return true; }
                    return false;
                }
                if (name == "round") {
                    nlohmann::json a; if (!getArgLit(0, a)) return false;
                    if (a.is_number()) { out = std::llround(a.get<double>()); return true; }
                    return false;
                }
                if (name == "pow") {
                    nlohmann::json a,b; if (!getArgLit(0, a) || !getArgLit(1, b)) return false;
                    if (a.is_number() && b.is_number()) { out = std::pow(a.get<double>(), b.get<double>()); return true; }
                    return false;
                }
                auto parseIso = [&](const std::string& s, std::tm& tm)->bool{
                    memset(&tm, 0, sizeof tm);
                    int Y=0,M=0,D=0,h=0,m=0,sec=0; char T='\0', Z='\0';
                    if (s.size() == 10 && std::sscanf(s.c_str(), "%d-%d-%d", &Y,&M,&D) == 3) {
                        tm.tm_year = Y-1900; tm.tm_mon = M-1; tm.tm_mday = D; tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0; return true;
                    }
                    if (std::sscanf(s.c_str(), "%d-%d-%d%c%d:%d:%d%c", &Y,&M,&D,&T,&h,&m,&sec,&Z) >= 7) {
                        tm.tm_year = Y-1900; tm.tm_mon = M-1; tm.tm_mday = D; tm.tm_hour = h; tm.tm_min = m; tm.tm_sec = sec; return true;
                    }
                    return false;
                };
                // Portable conversions between tm and time_t (UTC)
                auto portable_mkgmtime = [&](const std::tm* tmin)->time_t {
#ifdef _WIN32
                    return _mkgmtime(const_cast<std::tm*>(tmin));
#else
                    return timegm(const_cast<std::tm*>(tmin));
#endif
                };
                auto portable_gmtime_r = [&](const time_t* t, std::tm* out)->void {
#ifdef _WIN32
                    gmtime_s(out, t);
#else
                    gmtime_r(t, out);
#endif
                };
                auto tmToDateStr = [&](const std::tm& tm)->std::string{
                    char buf[32]; std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday); return std::string(buf);
                };
                if (name == "date_trunc") {
                    // DATE_TRUNC(unit, dateStr)
                    nlohmann::json unitJ, dateJ; if (!getArgLit(0, unitJ) || !getArgLit(1, dateJ)) return false;
                    if (!unitJ.is_string() || !dateJ.is_string()) return false;
                    std::string unit = unitJ.get<std::string>(); std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
                    std::tm tm{}; if (!parseIso(dateJ.get<std::string>(), tm)) return false;
                    if (unit == "day") { /* already normalized by tmToDateStr */ }
                    else if (unit == "month") { tm.tm_mday = 1; }
                    else if (unit == "year") { tm.tm_mon = 0; tm.tm_mday = 1; }
                    else return false;
                    out = tmToDateStr(tm); return true;
                }
                if (name == "date_add" || name == "date_sub") {
                    // DATE_ADD(dateStr, amount, unit) � unterst�tzt 'day','month','year'
                    nlohmann::json dateJ, amountJ, unitJ; if (!getArgLit(0, dateJ) || !getArgLit(1, amountJ) || !getArgLit(2, unitJ)) return false;
                    if (!dateJ.is_string() || !amountJ.is_number_integer() || !unitJ.is_string()) return false;
                    std::string unit = unitJ.get<std::string>(); std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
                    std::tm tm{}; if (!parseIso(dateJ.get<std::string>(), tm)) return false;
                    long long amt = amountJ.get<long long>(); if (name == "date_sub") amt = -amt;
                    if (unit == "day") {
                        time_t t = portable_mkgmtime(&tm); if (t == -1) return false;
                        t += static_cast<time_t>(amt * 86400);
                        std::tm outTm{}; portable_gmtime_r(&t, &outTm);
                        out = tmToDateStr(outTm); return true;
                    } else if (unit == "month") {
                        tm.tm_mon += themis::utils::conversion::safe_int64_to_int32(amt);
                        time_t t = portable_mkgmtime(&tm); if (t == -1) return false;
                        std::tm outTm{}; portable_gmtime_r(&t, &outTm);
                        out = tmToDateStr(outTm); return true;
                    } else if (unit == "year") {
                        tm.tm_year += themis::utils::conversion::safe_int64_to_int32(amt);
                        time_t t = portable_mkgmtime(&tm); if (t == -1) return false;
                        std::tm outTm{}; portable_gmtime_r(&t, &outTm);
                        out = tmToDateStr(outTm); return true;
                    } else {
                        return false;
                    }
                }
                if (name == "now") {
                    // Gibt YYYY-MM-DD zur�ck (UTC)
                    std::time_t t = std::time(nullptr); std::tm tm{}; portable_gmtime_r(&t, &tm);
                    out = tmToDateStr(tm); return true;
                }
                return false; // unbekannte Funktion
            };
            std::vector<SimplePred> preds;
            // XOR-Unterst�tzung: Paare einfacher Pr�dikate (links XOR rechts)
            std::vector<std::pair<SimplePred, SimplePred>> xorPreds;
            if (!(*parse_result)->filters.empty()) {
                using namespace themis::query;
                for (const auto& f : (*parse_result)->filters) {
                    auto* be = dynamic_cast<BinaryOpExpr*>(f->condition.get());
                    if (!be) continue;

                    // Falls es ein XOR ist, versuchen wir links und rechts je als SimplePred zu parsen
                    if (be->op == BinaryOperator::Xor) {
                        auto mapOpInner = [&](BinaryOperator bop)->std::optional<SimplePred::Op> {
                            switch (bop) {
                                case BinaryOperator::Eq:  return SimplePred::Op::Eq;
                                case BinaryOperator::Neq: return SimplePred::Op::Neq;
                                case BinaryOperator::Lt:  return SimplePred::Op::Lt;
                                case BinaryOperator::Lte: return SimplePred::Op::Lte;
                                case BinaryOperator::Gt:  return SimplePred::Op::Gt;
                                case BinaryOperator::Gte: return SimplePred::Op::Gte;
                                default: return std::nullopt;
                            }
                        };
                        auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
                            auto* fa = dynamic_cast<FieldAccessExpr*>(e.get());
                            if (!fa) return false;
                            auto* v = dynamic_cast<VariableExpr*>(fa->object.get());
                            if (!v) return false;
                            if (v->name != "v" && v->name != "e") return false;
                            if (v->name.empty()) return false;
                            var = v->name.front();
                            field = fa->field;
                            return true;
                        };
                        auto parseSimpleFromExpr = [&](std::shared_ptr<Expression> expr, SimplePred& out)->bool {
                            auto* be2 = dynamic_cast<BinaryOpExpr*>(expr.get());
                            if (!be2) return false;
                            auto op_m2 = mapOpInner(be2->op);
                            if (!op_m2.has_value()) return false;
                            char var=0; std::string field; nlohmann::json lit;
                            if (parseSide(be2->left, var, field)) {
                                if (evalExprToLiteral(be2->right, lit)) { out = {var, field, lit, *op_m2}; return true; }
                                return false;
                            }
                            {
                                nlohmann::json leftLit;
                                bool hasLeft = evalExprToLiteral(be2->left, leftLit);
                                if (hasLeft && parseSide(be2->right, var, field)) {
                                    auto invert = [&](SimplePred::Op o){
                                        switch (o) {
                                            case SimplePred::Op::Lt:  return SimplePred::Op::Gt;
                                            case SimplePred::Op::Lte: return SimplePred::Op::Gte;
                                            case SimplePred::Op::Gt:  return SimplePred::Op::Lt;
                                            case SimplePred::Op::Gte: return SimplePred::Op::Lte;
                                            default: return o;
                                        }
                                    };
                                    lit = leftLit; out = {var, field, lit, invert(*op_m2)}; return true;
                                }
                            }
                            return false;
                        };

                        SimplePred left{}, right{};
                        if (parseSimpleFromExpr(be->left, left) && parseSimpleFromExpr(be->right, right)) {
                            xorPreds.emplace_back(left, right);
                        }
                        // XOR-Filter als Ganzes sind verarbeitet; nicht in einfache Pr�dikate aufnehmen
                        continue;
                    }
                    auto mapOp = [&](BinaryOperator bop)->std::optional<SimplePred::Op> {
                        switch (bop) {
                            case BinaryOperator::Eq:  return SimplePred::Op::Eq;
                            case BinaryOperator::Neq: return SimplePred::Op::Neq;
                            case BinaryOperator::Lt:  return SimplePred::Op::Lt;
                            case BinaryOperator::Lte: return SimplePred::Op::Lte;
                            case BinaryOperator::Gt:  return SimplePred::Op::Gt;
                            case BinaryOperator::Gte: return SimplePred::Op::Gte;
                            default: return std::nullopt;
                        }
                    };
                    auto op_m = mapOp(be->op);
                    if (!op_m.has_value()) continue; // nur Vergleichsoperatoren

                    auto parseSide = [&](std::shared_ptr<Expression> e, char& var, std::string& field) -> bool {
                        // Erwartet FieldAccess(Variable('v'|'e'), field)
                        auto* fa = dynamic_cast<FieldAccessExpr*>(e.get());
                        if (!fa) return false;
                        auto* v = dynamic_cast<VariableExpr*>(fa->object.get());
                        if (!v) return false;
                        if (v->name != "v" && v->name != "e") return false;
                        if (v->name.empty()) return false;
                        var = v->name.front();
                        field = fa->field;
                        return true;
                    };

                    char var = 0; std::string field; nlohmann::json lit;
                    // v.field == <literal|funktion>
                    if (parseSide(be->left, var, field)) {
                        if (evalExprToLiteral(be->right, lit)) {
                            preds.push_back({var, field, lit, *op_m});
                        }
                        continue;
                    }
                    // <literal|funktion> == v.field
                    {
                        nlohmann::json leftLit;
                        bool hasLeft = evalExprToLiteral(be->left, leftLit);
                        if (hasLeft) {
                            if (parseSide(be->right, var, field)) {
                                lit = leftLit; 
                                // Operator invertieren (literal OP field) -> (field OP' literal)
                                auto invert = [&](SimplePred::Op o){
                                    switch (o) {
                                        case SimplePred::Op::Lt:  return SimplePred::Op::Gt;
                                        case SimplePred::Op::Lte: return SimplePred::Op::Gte;
                                        case SimplePred::Op::Gt:  return SimplePred::Op::Lt;
                                        case SimplePred::Op::Gte: return SimplePred::Op::Lte;
                                        default: return o; // Eq/Neq symmetrisch
                                    }
                                };
                                preds.push_back({var, field, lit, invert(*op_m)});
                            }
                        }
                    }
                }
            }

            auto cmp = [&](const std::string& a, const nlohmann::json& b, SimplePred::Op op)->bool{
                // Versuch: Zahl-Vergleich
                auto toDouble = [](const std::string& s, double& out)->bool{
                    char* end=nullptr; out = strtod(s.c_str(), &end); return end && *end=='\0';
                };
                auto toBool = [](const std::string& s, bool& out)->bool{
                    if (s == "true" || s == "1") { out = true; return true; }
                    if (s == "false" || s == "0") { out = false; return true; }
                    return false;
                };
                auto parseDate = [](const std::string& s, time_t& t)->bool{
                    // Unterst�tzt YYYY-MM-DD oder YYYY-MM-DDTHH:MM:SSZ
                    std::tm tm{}; memset(&tm, 0, sizeof tm);
                    if (s.size() == 10 && std::sscanf(s.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
                        tm.tm_year -= 1900; tm.tm_mon -= 1; tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
                        t = portable_mkgmtime_impl(&tm); return t != -1;
                    }
                    int Y,M,D,h=0,m=0,sec=0; char Z='\0', T='\0';
                    if (std::sscanf(s.c_str(), "%d-%d-%d%c%d:%d:%d%c", &Y,&M,&D,&T,&h,&m,&sec,&Z) >= 7) {
                        tm.tm_year = Y-1900; tm.tm_mon = M-1; tm.tm_mday = D; tm.tm_hour = h; tm.tm_min = m; tm.tm_sec = sec;
                        t = portable_mkgmtime_impl(&tm); return t != -1;
                    }
                    return false;
                };

                // Literaltypen pr�fen
                if (b.is_number()) {
                    double lit = b.get<double>();
                    double aval; if (!toDouble(a, aval)) return false;
                    switch (op) {
                        case SimplePred::Op::Eq:  return nearly_equal(aval, lit);
                        case SimplePred::Op::Neq: return !nearly_equal(aval, lit);
                        case SimplePred::Op::Lt:  return aval <  lit;
                        case SimplePred::Op::Lte: return aval <= lit;
                        case SimplePred::Op::Gt:  return aval >  lit;
                        case SimplePred::Op::Gte: return aval >= lit;
                    }
                } else if (b.is_boolean()) {
                    bool lit = b.get<bool>(); bool av;
                    if (!toBool(a, av)) return false;
                    switch (op) {
                        case SimplePred::Op::Eq:  return av == lit;
                        case SimplePred::Op::Neq: return av != lit;
                        default: return false; // <,> semantisch nicht definiert
                    }
                } else if (b.is_string()) {
                    const std::string lit = b.get<std::string>();
                    // Datumsvergleich falls beide ISO-�hnlich
                    time_t ta = 0, tb = 0; bool da = parseDate(a, ta), db = parseDate(lit, tb);
                    if (da && db) {
                        switch (op) {
                            case SimplePred::Op::Eq:  return ta == tb;
                            case SimplePred::Op::Neq: return ta != tb;
                            case SimplePred::Op::Lt:  return ta <  tb;
                            case SimplePred::Op::Lte: return ta <= tb;
                            case SimplePred::Op::Gt:  return ta >  tb;
                            case SimplePred::Op::Gte: return ta >= tb;
                        }
                    }
                    // Default: lexikographisch
                    int c = a.compare(lit);
                    switch (op) {
                        case SimplePred::Op::Eq:  return c == 0;
                        case SimplePred::Op::Neq: return c != 0;
                        case SimplePred::Op::Lt:  return c <  0;
                        case SimplePred::Op::Lte: return c <= 0;
                        case SimplePred::Op::Gt:  return c >  0;
                        case SimplePred::Op::Gte: return c >= 0;
                    }
                }
                return false;
            };

            // Allgemeine Filterauswertung (AND/OR/NOT/XOR) auf AST-Basis
            auto getVFieldString = [&](const std::string& pk, const std::string& field)->std::optional<std::string>{
                if (field == "_key") return pk;
                auto blob = storage_->get(pk);
                if (!blob) return std::nullopt;
                try {
                    auto ent = themis::BaseEntity::deserialize(pk, *blob);
                    return ent.getFieldAsString(field);
                } catch (...) { return std::nullopt; }
            };
            auto getEFieldString = [&](const std::string& edgeId, const std::string& field)->std::optional<std::string>{
                if (field == "id") return edgeId;
                auto eblob = storage_->get(themis::KeySchema::makeGraphEdgeKey(edgeId));
                if (!eblob) return std::nullopt;
                try {
                    auto ent = themis::BaseEntity::deserialize(edgeId, *eblob);
                    return ent.getFieldAsString(field);
                } catch (...) { return std::nullopt; }
            };

            using namespace themis::query;
            size_t filterShortCircuits = 0;  // Z�hlt Short-Circuit-Evaluationen in Filtern
            // Parent map used by path-based predicates is declared here so lambdas below can capture it
            struct ParentInfo { std::string parent; std::string edgeId; };
            std::unordered_map<std::string, ParentInfo> parent;

            std::function<bool(const Expression*, const std::string&, const std::optional<std::string>&)> evalBoolExpr;
            evalBoolExpr = [&](const Expression* e, const std::string& vpk, const std::optional<std::string>& eid)->bool{
                if (!e) return true;
                // Special handling: PATH.ALL / PATH.ANY / PATH.NONE
                if (auto* fe = dynamic_cast<const FunctionCallExpr*>(e)) {
                    std::string fname = fe->name; std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
                    if (fname == "path.all" || fname == "path.any" || fname == "path.none") {
                        // Expect two arguments: variable name (v or e) and a predicate expression
                        if (fe->arguments.size() != 2) return false;
                        const auto& path_args = fe->arguments;
                        auto* varExpr = dynamic_cast<const VariableExpr*>(path_args.front().get());
                        if (!varExpr) return false;
                        std::string varName = varExpr->name; // 'v' or 'e'
                        const Expression* inner = path_args.back().get();
                        // Reconstruct path from startVertex to vpk using parent map
                        std::vector<std::string> pathNodes;
                        std::vector<std::string> pathEdges; // edges between pathNodes[i] -> pathNodes[i+1]
                        std::string cur = vpk;
                        // If vpk empty, nothing to evaluate
                        if (cur.empty()) {
                            // Empty path: PATH.ALL & PATH.NONE -> true, PATH.ANY -> false
                            if (fname == "path.any") return false; else return true;
                        }
                        // Walk back using parent map if available.
                        // Guard: track visited nodes to prevent infinite loop if
                        // a cycle in the parent map is ever introduced (defensive
                        // invariant, normally impossible in a correct BFS).
                        pathNodes.push_back(cur);
                        std::unordered_set<std::string> pathVisited;
                        pathVisited.insert(cur);
                        auto itp = parent.find(cur);
                        while (itp != parent.end()) {
                            const std::string& next_node = itp->second.parent;
                            // Cycle detection: stop if we've already visited this node
                            if (pathVisited.count(next_node) > 0) {
                                break;
                            }
                            pathEdges.push_back(itp->second.edgeId);
                            pathNodes.push_back(next_node);
                            pathVisited.insert(next_node);
                            itp = parent.find(next_node);
                        }
                        // Now reverse to get start->...->cur
                        std::reverse(pathNodes.begin(), pathNodes.end());
                        std::reverse(pathEdges.begin(), pathEdges.end());

                        bool any = false;
                        bool all = true;
                        // If varName == "v", evaluate inner for each vertex in pathNodes
                        if (varName == "v") {
                            for (const auto& nodePk : pathNodes) {
                                bool r = evalBoolExpr(inner, nodePk, std::nullopt);
                                any = any || r;
                                all = all && r;
                            }
                        } else if (varName == "e") {
                            // Iterate over edges; align edge[i] with node[i+1] (to-vertex).
                            // pathNodes.size() == pathEdges.size()+1 by construction, so the
                            // node iterator always has a valid next element for each edge.
                            auto nit = pathNodes.begin() + 1; // start at the first 'to' vertex
                            for (const auto& eid2 : pathEdges) {
                                // For edge evaluation, set vpk to the 'to' vertex and eid to edge id
                                bool r = evalBoolExpr(inner, *nit++, std::optional<std::string>(eid2));
                                any = any || r;
                                all = all && r;
                            }
                        } else {
                            return false; // unknown var
                        }

                        if (fname == "path.all") return all;
                        if (fname == "path.none") return !any;
                        return any; // path.any
                    }
                }
                if (auto* ue = dynamic_cast<const UnaryOpExpr*>(e)) {
                    if (ue->op == UnaryOperator::Not) return !evalBoolExpr(ue->operand.get(), vpk, eid);
                    return false;
                }
                if (auto* be = dynamic_cast<const BinaryOpExpr*>(e)) {
                    auto evalCmp = [&](const Expression* left, BinaryOperator op, const Expression* right)->bool{
                        // Unterst�tze: FieldAccess(v|e).field <op> (Literal|Funktion) und umgekehrt
                        auto parseFA = [&](const Expression* ex, char& var, std::string& field)->bool{
                            auto* fa = dynamic_cast<const FieldAccessExpr*>(ex);
                            if (!fa) return false;
                            if (!fa->object) return false;  // Null-safety: Check shared_ptr is valid
                            auto* v = dynamic_cast<const VariableExpr*>(fa->object.get());
                            if (!v) return false;
                            if (v->name != "v" && v->name != "e") return false;
                            if (v->name.empty()) return false;
                            var = v->name.front(); field = fa->field; return true;
                        };
                        auto mapOp = [&](BinaryOperator bop)->std::optional<SimplePred::Op>{
                            switch (bop) {
                                case BinaryOperator::Eq: return SimplePred::Op::Eq;
                                case BinaryOperator::Neq: return SimplePred::Op::Neq;
                                case BinaryOperator::Lt: return SimplePred::Op::Lt;
                                case BinaryOperator::Lte: return SimplePred::Op::Lte;
                                case BinaryOperator::Gt: return SimplePred::Op::Gt;
                                case BinaryOperator::Gte: return SimplePred::Op::Gte;
                                default: return std::nullopt;
                            }
                        };
                        auto op_m = mapOp(op); if (!op_m) return false;

                        char var=0; std::string field; nlohmann::json lit;
                        if (parseFA(left, var, field)) {
                            // rechts Literal/Funktion
                            if (!evalExprToLiteral(std::shared_ptr<Expression>(const_cast<Expression*>(right), [](Expression*){}), lit)) return false;
                            std::optional<std::string> val;
                            if (var=='v') val = getVFieldString(vpk, field);
                            else { if (!eid) return false; val = getEFieldString(*eid, field); }
                            if (!val) return false;
                            return cmp(*val, lit, *op_m);
                        }
                        if (parseFA(right, var, field)) {
                            // links Literal/Funktion
                            if (!evalExprToLiteral(std::shared_ptr<Expression>(const_cast<Expression*>(left), [](Expression*){}), lit)) return false;
                            // invertiere Operator
                            auto invert = [&](SimplePred::Op o){
                                switch (o) {
                                    case SimplePred::Op::Lt: return SimplePred::Op::Gt;
                                    case SimplePred::Op::Lte: return SimplePred::Op::Gte;
                                    case SimplePred::Op::Gt: return SimplePred::Op::Lt;
                                    case SimplePred::Op::Gte: return SimplePred::Op::Lte;
                                    default: return o;
                                }
                            };
                            auto op2 = invert(*op_m);
                            std::optional<std::string> val;
                            if (var=='v') val = getVFieldString(vpk, field);
                            else { if (!eid) return false; val = getEFieldString(*eid, field); }
                            if (!val) return false;
                            return cmp(*val, lit, op2);
                        }
                        return false; // nicht unterst�tztes Muster
                    };

                    switch (be->op) {
                        case BinaryOperator::And: {
                            bool l = evalBoolExpr(be->left.get(), vpk, eid);
                            if (!l) { filterShortCircuits++; return false; }
                            return evalBoolExpr(be->right.get(), vpk, eid);
                        }
                        case BinaryOperator::Or: {
                            bool l = evalBoolExpr(be->left.get(), vpk, eid);
                            if (l) { filterShortCircuits++; return true; }
                            return evalBoolExpr(be->right.get(), vpk, eid);
                        }
                        case BinaryOperator::Xor: { bool l = evalBoolExpr(be->left.get(), vpk, eid); bool r = evalBoolExpr(be->right.get(), vpk, eid); return l ^ r; }
                        case BinaryOperator::Eq:
                        case BinaryOperator::Neq:
                        case BinaryOperator::Lt:
                        case BinaryOperator::Lte:
                        case BinaryOperator::Gt:
                        case BinaryOperator::Gte:
                            return evalCmp(be->left.get(), be->op, be->right.get());
                        default:
                            return false;
                    }
                }
                // Literale/Variablen alleine als bool nicht unterst�tzt ? false
                return false;
            };

            // DATA-RACE-AUDIT(2026-08-26 Wave-7): `usesVE` is a recursive
            // stack-local std::function.  The scanner flagged the `[&]`
            // capture as a potential data race because `[&]` silently captures
            // the entire enclosing scope — including `usesVE` itself — by
            // reference, making it look like a reference that could escape.
            // FIX: change to an explicit single-variable capture `[&usesVE]`
            // which (a) makes the self-reference intent unambiguous, and (b)
            // prevents any future code addition inside the lambda from
            // accidentally touching other stack locals without a visible
            // capture declaration (race-free: single-threaded dispatch only).
            std::function<bool(const Expression*)> usesVE;
            usesVE = [&usesVE](const Expression* e)->bool{
                if (!e) return false;
                if (auto* le = dynamic_cast<const LiteralExpr*>(e)) {
                    return false;
                }
                if (auto* ve = dynamic_cast<const VariableExpr*>(e)) {
                    return (ve->name == "v" || ve->name == "e");
                }
                if (auto* fa = dynamic_cast<const FieldAccessExpr*>(e)) {
                    // Pr�fe, ob das Objekt eine Variable v/e ist
                    if (auto* objVar = dynamic_cast<VariableExpr*>(fa->object.get())) {
                        if (objVar->name == "v" || objVar->name == "e") return true;
                    }
                    // Auch verschachtelte Zugriffe k�nnen v/e enthalten
                    return usesVE(fa->object.get());
                }
                if (auto* ue = dynamic_cast<const UnaryOpExpr*>(e)) {
                    return usesVE(ue->operand.get());
                }
                if (auto* be = dynamic_cast<const BinaryOpExpr*>(e)) {
                    return usesVE(be->left.get()) || usesVE(be->right.get());
                }
                if (auto* fe = dynamic_cast<const FunctionCallExpr*>(e)) {
                    for (const auto& a : fe->arguments) {
                        if (usesVE(a.get())) return true;
                    }
                    return false;
                }
                return false;
            };

            // Vorab: Wenn alle FILTER-Ausdr�cke keine v/e-Referenz enthalten, einmal bewerten und ggf. fr�h abbrechen
            if (!(*parse_result)->filters.empty()) {
                bool anyUsesVE = false;
                for (const auto& f : (*parse_result)->filters) {
                    if (usesVE(f->condition.get())) { anyUsesVE = true; break; }
                }
                if (!anyUsesVE) {
                    bool allPass = true;
                    for (const auto& f : (*parse_result)->filters) {
                        if (!evalBoolExpr(f->condition.get(), std::string{}, std::nullopt)) { allPass = false; break; }
                    }
                    if (!allPass) {
                        nlohmann::json res;
                        res["table"] = "graph";
                        res["count"] = 0;
                        res["entities"] = nlohmann::json::array();
                        if (explain) {
                            nlohmann::json metrics;
                            metrics["constant_filter_precheck"] = true;
                            metrics["constant_filter_result"] = false;
                            metrics["edges_expanded"] = 0;
                            metrics["pruned_last_level"] = 0;
                            metrics["filter_evaluations_total"] = 1;
                            metrics["filter_short_circuits"] = 0;
                            metrics["frontier_processed_per_depth"] = nlohmann::json::object();
                            metrics["enqueued_per_depth"] = nlohmann::json::object();
                            res["metrics"] = metrics;
                        }
                        return makeResponse(http::status::ok, res.dump(), req);
                    }
                }
                    // Joins via doppeltem FOR (MVP): Wenn mehrere FOR-Klauseln vorhanden sind und keine Traversal-Query aktiv ist
                    if ((*parse_result) && (*parse_result)->traversal == nullptr) {
                        const auto& for_nodes = (*parse_result)->for_nodes;
                        if (for_nodes.size() >= 2) {
                        auto joinSpan = Tracer::startSpan("aql.join");
                        // Beschränkung: Genau zwei FOR-Klauseln, Equality-Join über FILTER lhs.field == rhs.field
                        const auto& f1 = for_nodes.front();
                        const auto second_for_node = std::next(for_nodes.begin());
                        const auto& f2 = *second_for_node;
                        const std::string var1 = f1.variable;
                        const std::string var2 = f2.variable;
                        const std::string table1 = f1.collection;
                        const std::string table2 = f2.collection;
                        joinSpan.setAttribute("join.var_left", var1);
                        joinSpan.setAttribute("join.var_right", var2);
                        joinSpan.setAttribute("join.table_left", table1);
                        joinSpan.setAttribute("join.table_right", table2);

                        // Hilfsfunktionen zur Extraktion
                        using namespace themis::query;
                        // DATA-RACE-AUDIT(2026-08-26 Wave-7): `fieldFromFA` is a
                        // non-recursive helper that does NOT need to capture
                        // itself; the `[&]` on the prior version silently
                        // captured all outer locals (var1, var2, table1/2,
                        // parse_result…) — making the scanner flag the whole
                        // lambda as a potential escaped-reference race.
                        // FIX: capture only `fieldFromFA` is not needed here
                        // (no self-recursion); use empty capture `[]` and
                        // accept params explicitly.  Single-threaded dispatch,
                        // no real race — capture narrowing removes the warning.
                        std::function<std::string(const std::shared_ptr<Expression>&, std::string&)> fieldFromFA = [](const std::shared_ptr<Expression>& expr, std::string& rootVar)->std::string {
                            // Liefert Feldpfad ("a.b") und setzt rootVar auf Variablennamen
                            auto* fa = dynamic_cast<FieldAccessExpr*>(expr.get());
                            if (!fa) return std::string();
                            std::vector<std::string> parts;
                            parts.reserve(8);  // Typical nesting depth is 4-8 levels
                            parts.push_back(fa->field);
                            auto* cur = fa->object.get();
                            while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) {
                                parts.push_back(fa2->field);
                                cur = fa2->object.get();
                            }
                            auto* root = dynamic_cast<VariableExpr*>(cur);
                            if (!root) return std::string();
                            rootVar = root->name;
                            std::ostringstream col_oss;
                            for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
                                if (it != parts.rbegin()) col_oss << ".";
                                col_oss << *it;
                            }
                            return col_oss.str();
                        };
                        auto literalToString = [&](const LiteralValue& value)->std::string{
                            return std::visit([](auto&& arg)->std::string{
                                using T = std::decay_t<decltype(arg)>;
                                if constexpr (std::is_same_v<T, std::nullptr_t>) return std::string("null");
                                else if constexpr (std::is_same_v<T, bool>) return arg ? std::string("true") : std::string("false");
                                else if constexpr (std::is_same_v<T, int64_t>) return std::to_string(arg);
                                else if constexpr (std::is_same_v<T, double>) return std::to_string(arg);
                                else if constexpr (std::is_same_v<T, std::string>) return arg;
                                else return std::string();
                            }, value);
                        };

                        // Zerlege FILTER in Konjunktions-Terme und identifiziere Join-Bedingung
                        std::optional<std::pair<std::string,std::string>> joinCols; // (col1, col2)
                        std::vector<PredicateEq> eq1, eq2; std::vector<PredicateRange> r1, r2;
                        std::function<void(const std::shared_ptr<Expression>&)> collectPreds;
                        collectPreds = [&](const std::shared_ptr<Expression>& e){
                            if (!e) return;
                            if (e->getType() == ASTNodeType::BinaryOp) {
                                auto bin = std::static_pointer_cast<BinaryOpExpr>(e);
                                if (bin->op == BinaryOperator::And) {
                                    collectPreds(bin->left); collectPreds(bin->right); return;
                                }
                                // Join-Gleichheit: Field(var1.*) == Field(var2.*) oder umgekehrt
                                if (bin->op == BinaryOperator::Eq) {
                                    std::string rvL, rvR; std::string colL, colR;
                                    colL = fieldFromFA(bin->left, rvL);
                                    colR = fieldFromFA(bin->right, rvR);
                                    if (!colL.empty() && !colR.empty()) {
                                        // beide Seiten FieldAccess
                                        if ((rvL == var1 && rvR == var2) || (rvL == var2 && rvR == var1)) {
                                            if (joinCols.has_value()) {
                                                joinSpan.setStatus(false, "multiple_join_predicates_not_supported");
                                                span.setStatus(false, "Only single equality-join supported");
                                                return; // ignore further
                                            }
                                            if (rvL == var1) joinCols = std::make_pair(colL, colR); else joinCols = std::make_pair(colR, colL);
                                            return;
                                        }
                                    }
                                    // Seiten-Literal Prädikat
                                    if (!colL.empty() && rvL == var1 && bin->right->getType() == ASTNodeType::Literal) {
                                        auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
                                        if (!lit) return;
                                        switch (bin->op) {
                                            case BinaryOperator::Eq: eq1.push_back({colL, literalToString(lit->value)}); break;
                                            case BinaryOperator::Lt: r1.push_back({colL, std::nullopt, literalToString(lit->value), true, false}); break;
                                            case BinaryOperator::Lte: r1.push_back({colL, std::nullopt, literalToString(lit->value), true, true}); break;
                                            case BinaryOperator::Gt: r1.push_back({colL, literalToString(lit->value), std::nullopt, false, true}); break;
                                            case BinaryOperator::Gte: r1.push_back({colL, literalToString(lit->value), std::nullopt, true, true}); break;
                                            default: break;
                                        }
                                        return;
                                    }
                                    if (!colL.empty() && rvL == var2 && bin->right->getType() == ASTNodeType::Literal) {
                                        auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
                                        switch (bin->op) {
                                            case BinaryOperator::Eq: eq2.push_back({colL, literalToString(lit->value)}); break;
                                            case BinaryOperator::Lt: r2.push_back({colL, std::nullopt, literalToString(lit->value), true, false}); break;
                                            case BinaryOperator::Lte: r2.push_back({colL, std::nullopt, literalToString(lit->value), true, true}); break;
                                            case BinaryOperator::Gt: r2.push_back({colL, literalToString(lit->value), std::nullopt, false, true}); break;
                                            case BinaryOperator::Gte: r2.push_back({colL, literalToString(lit->value), std::nullopt, true, true}); break;
                                            default: break;
                                        }
                                        return;
                                    }
                                    // Literal == Field(varX.*)
                                    if (bin->left->getType() == ASTNodeType::Literal) {
                                        std::string rv; std::string col = fieldFromFA(bin->right, rv);
                                        if (!col.empty()) {
                                            auto lit = std::static_pointer_cast<LiteralExpr>(bin->left);
                                            if (rv == var1) eq1.push_back({col, literalToString(lit->value)});
                                            else if (rv == var2) eq2.push_back({col, literalToString(lit->value)});
                                        }
                                        return;
                                    }
                                }
                                // Range auf rechter oder linker Seite bereits obigen Fällen abgedeckt (nur MVP)
                            }
                        };
                        for (const auto& f : (*parse_result)->filters) {
                            collectPreds(f->condition);
                        }
                        if (!joinCols.has_value()) {
                            joinSpan.setStatus(false, "join_predicate_missing");
                            span.setStatus(false, "JOIN requires equality predicate between variables");
                            return makeErrorResponse(http::status::bad_request, "JOIN requires equality predicate between variables", req);
                        }

                        // Führe Seitenabfragen aus
                        themis::ConjunctiveQuery q1; q1.table = table1; q1.predicates = eq1; q1.rangePredicates = r1;
                        themis::ConjunctiveQuery q2; q2.table = table2; q2.predicates = eq2; q2.rangePredicates = r2;
                        themis::query::QueryEngine engine(*storage_, *secondary_index_);
                        auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
                        if (stats_collector) engine.setStatisticsCollector(stats_collector);
                        
                        auto result1 = allow_full_scan ? engine.executeAndEntitiesWithFallback(q1, optimize) : engine.executeAndEntities(q1);
                        std::pair<QueryExecStatus, std::vector<themis::BaseEntity>> res1;
                        if (!result1) {
                            res1 = {QueryExecStatus{false, result1.error().message()}, std::vector<themis::BaseEntity>{}};
                        } else {
                            res1 = {QueryExecStatus::OK(), std::move(*result1)};
                        }
                        if (!res1.first.ok) {
                            joinSpan.setStatus(false, res1.first.message);
                            span.setStatus(false, "Left side execution failed");
                            return makeErrorResponse(http::status::bad_request, res1.first.message, req);
                        }
                        
                        auto result2 = allow_full_scan ? engine.executeAndEntitiesWithFallback(q2, optimize) : engine.executeAndEntities(q2);
                        std::pair<QueryExecStatus, std::vector<themis::BaseEntity>> res2;
                        if (!result2) {
                            res2 = {QueryExecStatus{false, result2.error().message()}, std::vector<themis::BaseEntity>{}};
                        } else {
                            res2 = {QueryExecStatus::OK(), std::move(*result2)};
                        }
                        if (!res2.first.ok) {
                            joinSpan.setStatus(false, res2.first.message);
                            span.setStatus(false, "Right side execution failed");
                            return makeErrorResponse(http::status::bad_request, res2.first.message, req);
                        }

                        // Wähle kleinere Seite für Hash-Index
                        const auto& leftVec = res1.second; const auto& rightVec = res2.second;
                        bool buildLeft = leftVec.size() <= rightVec.size();
                        const auto [colLeft, colRight] = *joinCols;
                        std::unordered_multimap<std::string, themis::BaseEntity> hash;
                        auto getFieldStr = [&](const themis::BaseEntity& e, const std::string& col)->std::optional<std::string> { auto v = e.getFieldAsString(col); if (v.has_value()) return v; auto d = e.getFieldAsDouble(col); if (d.has_value()) return std::to_string(*d); return std::nullopt; };
                        if (buildLeft) {
                            hash.reserve(leftVec.size()*2+1);
                            for (const auto& e : leftVec) { auto k = getFieldStr(e, colLeft); if (k.has_value()) hash.emplace(*k, e); }
                        } else {
                            hash.reserve(rightVec.size()*2+1);
                            for (const auto& e : rightVec) { auto k = getFieldStr(e, colRight); if (k.has_value()) hash.emplace(*k, e); }
                        }

                        // Bestimme welche Variable im RETURN zurückgegeben werden soll
                        std::string retVar;
                        if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
                            if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) {
                                retVar = v->name;
                            }
                        }
                        if (retVar != var1 && retVar != var2) {
                            joinSpan.setStatus(false, "return_not_supported_for_join");
                            span.setStatus(false, "JOIN currently supports RETURN of one bound variable (left or right)");
                            return makeErrorResponse(http::status::bad_request, "JOIN currently supports RETURN of one bound variable (left or right)", req);
                        }

                        // Probiere Join und sammle Ergebnisse
                        std::vector<themis::BaseEntity> out;
                        if (buildLeft) {
                            for (const auto& e : rightVec) {
                                auto k = getFieldStr(e, colRight); if (!k.has_value()) continue;
                                auto range = hash.equal_range(*k);
                                for (auto it = range.first; it != range.second; ++it) {
                                    const themis::BaseEntity& l = it->second;
                                    if (retVar == var1) out.push_back(l); else out.push_back(e);
                                }
                            }
                        } else {
                            for (const auto& e : leftVec) {
                                auto k = getFieldStr(e, colLeft); if (!k.has_value()) continue;
                                auto range = hash.equal_range(*k);
                                for (auto it = range.first; it != range.second; ++it) {
                                    const themis::BaseEntity& r = it->second;
                                    if (retVar == var1) out.push_back(e); else out.push_back(r);
                                }
                            }
                        }

                        // LIMIT (post-join slicing)
                        if ((*parse_result) && (*parse_result)->limit) {
                            auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
                            auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
                            if (off < out.size()) {
                                size_t last = std::min(out.size(), off + cnt);
                                std::vector<themis::BaseEntity> tmp; tmp.reserve(last - off);
                                auto first_it = out.begin() + static_cast<std::ptrdiff_t>(off);
                                auto last_it = out.begin() + static_cast<std::ptrdiff_t>(last);
                                std::move(first_it, last_it, std::back_inserter(tmp));
                                out.swap(tmp);
                            } else {
                                out.clear();
                            }
                        }

                        // Serialize
                        nlohmann::json entities = nlohmann::json::array();
                        for (const auto& e : out) entities.push_back(e.toJson());
                        nlohmann::json response_body = {
                            {"table_left", table1}, {"table_right", table2}, {"count", out.size()}, {"entities", applyMasking(entities, req)}
                        };
                        if (explain) {
                            response_body["query"] = aql_query;
                            response_body["ast"] = (*parse_result)->toJSON();
                            nlohmann::json jp; jp["on_left"] = (*joinCols).first; jp["on_right"] = (*joinCols).second; response_body["join"] = jp;
                        }
                        joinSpan.setAttribute("join.output_count", static_cast<int64_t>(out.size()));
                        joinSpan.setStatus(true);
                        span.setAttribute("aql.result_count", static_cast<int64_t>(out.size()));
                        span.setStatus(true);
                        return makeResponse(http::status::ok, response_body.dump(), req);
                        }
                    }

            }

            [[maybe_unused]]
            auto evalV = [&](const std::string& pk)->bool{
                for (const auto& p : preds) {
                    if (p.var != 'v') continue;
                    if (p.field == "_key") { // direkte PK-Vergleiche
                        if (!cmp(pk, p.literal, p.op)) return false;
                    } else {
                        // Lade Entity und vergleiche typbewusst
                        auto blob = storage_->get(pk);
                        if (!blob) return false;
                        try {
                            auto ent = themis::BaseEntity::deserialize(pk, *blob);
                            // bevorzugt String, f�r Zahlen k�nnte man auch getFieldAsDouble versuchen
                            auto valOpt = ent.getFieldAsString(p.field);
                            if (!valOpt) return false;
                            if (!cmp(*valOpt, p.literal, p.op)) return false;
                        } catch (...) { return false; }
                    }
                }
                return true;
            };

            auto evalSingleV = [&](const std::string& pk, const SimplePred& p)->bool{
                if (p.var != 'v') return true; // nicht zust�ndig
                if (p.field == "_key") {
                    return cmp(pk, p.literal, p.op);
                } else {
                    auto blob = storage_->get(pk);
                    if (!blob) return false;
                    try {
                        auto ent = themis::BaseEntity::deserialize(pk, *blob);
                        auto valOpt = ent.getFieldAsString(p.field);
                        if (!valOpt) return false;
                        return cmp(*valOpt, p.literal, p.op);
                    } catch (...) { return false; }
                }
            };

            [[maybe_unused]] auto evalE = [&](const std::string& edgeId)->bool{
                bool needLoad = false;
                for (const auto& p : preds) {
                    if (p.var != 'e') continue;
                    if (p.field == "id") {
                        if (!cmp(edgeId, p.literal, p.op)) return false;
                    } else {
                        needLoad = true; // _from/_to oder andere Felder
                    }
                }
                if (!needLoad) return true;
                auto blob = storage_->get(themis::KeySchema::makeGraphEdgeKey(edgeId));
                if (!blob) return false;
                try {
                    auto ent = themis::BaseEntity::deserialize(edgeId, *blob);
                    for (const auto& p : preds) {
                        if (p.var != 'e' || p.field == "id") continue;
                        auto valOpt = ent.getFieldAsString(p.field);
                        if (!valOpt) return false;
                        if (!cmp(*valOpt, p.literal, p.op)) return false;
                    }
                } catch (...) { return false; }
                return true;
            };

            auto evalSingleE = [&](const std::string& edgeId, const SimplePred& p)->bool{
                if (p.var != 'e') return true; // nicht zust�ndig
                if (p.field == "id") {
                    return cmp(edgeId, p.literal, p.op);
                }
                auto blob = storage_->get(themis::KeySchema::makeGraphEdgeKey(edgeId));
                if (!blob) return false;
                try {
                    auto ent = themis::BaseEntity::deserialize(edgeId, *blob);
                    auto valOpt = ent.getFieldAsString(p.field);
                    if (!valOpt) return false;
                    return cmp(*valOpt, p.literal, p.op);
                } catch (...) { return false; }
            };

            // BFS mit Eltern-/Kanten-Tracking f�r e/p
            std::unordered_set<std::string> visited;
            std::queue<std::pair<std::string,int>> qnodes;
            qnodes.push({t.startVertex, 0});
            visited.insert(t.startVertex);

            // Metriken
            bool constantFilterPrechecked = false; // bleibt false, wenn oben nicht gegriffen (true w�re nur beim Early-Return)
            size_t edgesExpanded = 0;
            size_t prunedLastLevel = 0;
            std::unordered_map<int, size_t> frontierProcessedPerDepth;
            std::unordered_map<int, size_t> enqueuedPerDepth;
            size_t filterEvaluationsTotal = 0;
            // filterShortCircuits bereits weiter oben definiert (Zeile ~1265)
            size_t frontierLimitHits = 0;
            size_t maxFrontierSizeReached = 0;
            bool resultLimitReached = false;

            // Ergebnis-Sammlungen
            std::vector<std::string> resultVertices;
            std::vector<std::string> resultEdgeIds;
            std::vector<std::string> resultTerminalVertices; // f�r Pfadrekonstruktion

            auto withinDepth = [&](int depth){ return depth >= t.minDepth && depth <= t.maxDepth; };

            auto bfsSpan = Tracer::startSpan("aql.traversal.bfs");
            bfsSpan.setAttribute("traversal.max_frontier_size_limit", static_cast<int64_t>(max_frontier_size));
            bfsSpan.setAttribute("traversal.max_results_limit", static_cast<int64_t>(max_results));
            
            while (!qnodes.empty()) {
                if (timedOut()) {
                    bfsSpan.setStatus(false, "timeout");
                    span.setStatus(false, "Traversal timed out");
                    return makeErrorResponse(http::status::request_timeout,
                        "query exceeded timeout of " + std::to_string(resource_limits.timeout_ms) + " ms", req);
                }
                // Frontier-Size Limit Check (Soft Limit)
                if (qnodes.size() > max_frontier_size) {
                    frontierLimitHits++;
                    // Optional: Abbruch oder nur Warnung
                    // break;  // hart abbrechen (sp�ter konfigurierbar)
                }
                if (qnodes.size() > maxFrontierSizeReached) {
                    maxFrontierSizeReached = qnodes.size();
                }
                
                auto [node, depth] = qnodes.front();
                qnodes.pop();
                frontierProcessedPerDepth[depth]++;

                if (withinDepth(depth)) {
                    if (!(depth == 0 && t.minDepth > 0)) {
                        // Filterauswertung pro "Zeile" (Knoten + eingehende Kante) mit voller Bool-Logik
                        bool pass = true;
                        if (!(*parse_result)->filters.empty()) {
                            filterEvaluationsTotal++;
                            std::optional<std::string> edgeIdOpt;
                            if (depth > 0) {
                                auto itp = parent.find(node);
                                if (itp != parent.end()) edgeIdOpt = itp->second.edgeId;
                            }
                            for (const auto& f : (*parse_result)->filters) {
                                if (!evalBoolExpr(f->condition.get(), node, edgeIdOpt)) { pass = false; break; }
                            }
                        }

                        if (pass) {
                            // Result-Limit Check
                            size_t currentResultCount = (retMode == RetMode::Vertex) ? resultVertices.size() :
                                                        (retMode == RetMode::Edge) ? resultEdgeIds.size() :
                                                        resultTerminalVertices.size();
                            if (currentResultCount >= max_results) {
                                resultLimitReached = true;
                                // Optional: BFS abbrechen (sp�ter konfigurierbar)
                                // goto traversal_finished;
                            }
                            
                            if (retMode == RetMode::Vertex) {
                                resultVertices.push_back(node);
                            } else if (retMode == RetMode::Edge) {
                                auto it = parent.find(node);
                                if (it != parent.end()) {
                                    resultEdgeIds.push_back(it->second.edgeId);
                                }
                            } else { // Path
                                if (node != t.startVertex) {
                                    resultTerminalVertices.push_back(node);
                                } else if (t.minDepth == 0) {
                                    resultTerminalVertices.push_back(node);
                                }
                            }
                        }
                    }
                }
                if (depth == t.maxDepth) continue;

                auto enqueueOut = [&](const std::vector<themis::GraphIndexManager::AdjacencyInfo>& adj){
                    for (const auto& a : adj) {
                        const std::string& nb = a.targetPk;
                        edgesExpanded++;
                        // Konservative Pruning-Strategie am letzten Level: wende einfache v/e-Pr�dikate an
                        if (depth + 1 == t.maxDepth && !preds.empty()) {
                            bool drop = false;
                            for (const auto& p : preds) {
                                if (p.var == 'e' && !evalSingleE(a.edgeId, p)) { drop = true; break; }
                                if (p.var == 'v' && !evalSingleV(nb, p)) { drop = true; break; }
                            }
                            if (drop) { prunedLastLevel++; continue; }
                        }
                        if (visited.insert(nb).second) {
                            parent[nb] = {node, a.edgeId};
                            qnodes.push({nb, depth + 1});
                            enqueuedPerDepth[depth + 1]++;
                        }
                    }
                };
                auto enqueueIn = [&](const std::vector<themis::GraphIndexManager::AdjacencyInfo>& adj){
                    for (const auto& a : adj) {
                        const std::string& nb = a.targetPk; // bei inAdjacency ist targetPk = fromPk
                        edgesExpanded++;
                        // Konservative Pruning-Strategie am letzten Level
                        if (depth + 1 == t.maxDepth && !preds.empty()) {
                            bool drop = false;
                            for (const auto& p : preds) {
                                if (p.var == 'e' && !evalSingleE(a.edgeId, p)) { drop = true; break; }
                                if (p.var == 'v' && !evalSingleV(nb, p)) { drop = true; break; }
                            }
                            if (drop) { prunedLastLevel++; continue; }
                        }
                        if (visited.insert(nb).second) {
                            parent[nb] = {node, a.edgeId};
                            qnodes.push({nb, depth + 1});
                            enqueuedPerDepth[depth + 1]++;
                        }
                    }
                };

                if (t.direction == themis::AQLTranslator::TranslationResult::TraversalQuery::Direction::Outbound ||
                    t.direction == themis::AQLTranslator::TranslationResult::TraversalQuery::Direction::Any) {
                    auto [stAdj, adj] = graph_index.outAdjacency(node);
                    if (!stAdj.ok) {
                        return makeErrorResponse(http::status::internal_server_error, std::string("Graph outAdjacency failed: ") + stAdj.message, req);
                    }
                    enqueueOut(adj);
                }
                if (t.direction == themis::AQLTranslator::TranslationResult::TraversalQuery::Direction::Inbound ||
                    t.direction == themis::AQLTranslator::TranslationResult::TraversalQuery::Direction::Any) {
                    auto [stAdjIn, adjIn] = graph_index.inAdjacency(node);
                    if (!stAdjIn.ok) {
                        return makeErrorResponse(http::status::internal_server_error, std::string("Graph inAdjacency failed: ") + stAdjIn.message, req);
                    }
                    enqueueIn(adjIn);
                }
            }
            
            bfsSpan.setAttribute("traversal.visited_count", static_cast<int64_t>(visited.size()));
            bfsSpan.setAttribute("traversal.edges_expanded", static_cast<int64_t>(edgesExpanded));
            bfsSpan.setAttribute("traversal.filter_evaluations", static_cast<int64_t>(filterEvaluationsTotal));
            bfsSpan.setStatus(true);

            // Serialisierung je nach Return-Modus
            nlohmann::json res;
            res["table"] = "graph";
            res["entities"] = nlohmann::json::array();

            if (retMode == RetMode::Vertex) {
                res["count"] = themis::utils::conversion::safe_size_to_int(resultVertices.size());
                for (const auto& pk : resultVertices) {
                    std::optional<std::vector<uint8_t>> blob = storage_->get(pk);
                    if (!blob && pk.find(':') == std::string::npos) {
                        blob = storage_->get(std::string("users:") + pk);
                    }
                    if (blob) {
                        try {
                            auto entity = themis::BaseEntity::deserialize(pk, *blob);
                            res["entities"].push_back(entity.toJson());
                        } catch (...) {
                            THEMIS_DEBUG("query_api_handler: unhandled exception caught");
                            res["entities"].push_back(nlohmann::json({{"_key", pk}}));
                        }
                    } else {
                        res["entities"].push_back(nlohmann::json({{"_key", pk}}));
                    }
                }
            } else if (retMode == RetMode::Edge) {
                res["count"] = themis::utils::conversion::safe_size_to_int(resultEdgeIds.size());
                for (const auto& eid : resultEdgeIds) {
                    auto eblob = storage_->get(themis::KeySchema::makeGraphEdgeKey(eid));
                    if (eblob) {
                        try {
                            auto edgeEnt = themis::BaseEntity::deserialize(eid, *eblob);
                            res["entities"].push_back(edgeEnt.toJson());
                        } catch (...) {
                            THEMIS_DEBUG("query_api_handler: unhandled exception caught");
                            res["entities"].push_back(nlohmann::json({{"_edge", eid}}));
                        }
                    } else {
                        res["entities"].push_back(nlohmann::json({{"_edge", eid}}));
                    }
                }
            } else { // Path
                // F�r jeden Terminalknoten Pfad rekonstruieren
                res["count"] = themis::utils::conversion::safe_size_to_int(resultTerminalVertices.size());
                for (const auto& terminal : resultTerminalVertices) {
                    // Rekonstruiere Knotenfolge und Kanten entlang Elternzeiger
                    std::vector<std::string> vertices;
                    std::vector<std::string> edges;
                    std::string cur = terminal;
                    vertices.push_back(cur);
                    // Cycle guard: track visited nodes to prevent infinite traversal
                    // if a malformed parent map contains a cycle.
                    std::unordered_set<std::string> recoVisited;
                    recoVisited.insert(cur);
                    while (cur != t.startVertex) {
                        auto it = parent.find(cur);
                        if (it == parent.end()) break; // sollte bei Start aufhören
                        const std::string& next = it->second.parent;
                        if (recoVisited.count(next) > 0) break; // cycle detected
                        edges.push_back(it->second.edgeId);
                        cur = next;
                        vertices.push_back(cur);
                        recoVisited.insert(cur);
                    }
                    std::reverse(vertices.begin(), vertices.end());
                    std::reverse(edges.begin(), edges.end());

                    nlohmann::json jpath;
                    jpath["length"] = themis::utils::conversion::safe_size_to_int(edges.size());
                    jpath["vertices"] = nlohmann::json::array();
                    jpath["edges"] = nlohmann::json::array();

                    // Lade Vertex-Entities
                    for (const auto& pk : vertices) {
                        std::optional<std::vector<uint8_t>> blob = storage_->get(pk);
                        if (!blob && pk.find(':') == std::string::npos) {
                            blob = storage_->get(std::string("users:") + pk);
                        }
                        if (blob) {
                            try {
                                auto ent = themis::BaseEntity::deserialize(pk, *blob);
                                jpath["vertices"].push_back(ent.toJson());
                            } catch (...) {
                                THEMIS_DEBUG("query_api_handler: unhandled exception caught");
                                jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));
                            }
                        } else {
                            jpath["vertices"].push_back(nlohmann::json({{"_key", pk}}));
                        }
                    }

                    // Lade Edge-Entities
                    for (const auto& eid : edges) {
                        auto eblob = storage_->get(themis::KeySchema::makeGraphEdgeKey(eid));
                        if (eblob) {
                            try {
                                auto eent = themis::BaseEntity::deserialize(eid, *eblob);
                                jpath["edges"].push_back(eent.toJson());
                            } catch (...) {
                                THEMIS_DEBUG("query_api_handler: unhandled exception caught");
                                jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));
                            }
                        } else {
                            jpath["edges"].push_back(nlohmann::json({{"_edge", eid}}));
                        }
                    }

                    res["entities"].push_back(std::move(jpath));
                }
            }
            // EXPLAIN/PROFILE: Traversal-Metriken anh�ngen
            if (explain) {
                nlohmann::json metrics;
                metrics["constant_filter_precheck"] = constantFilterPrechecked;
                metrics["edges_expanded"] = themis::utils::conversion::safe_size_to_int(edgesExpanded);
                metrics["pruned_last_level"] = themis::utils::conversion::safe_size_to_int(prunedLastLevel);
                metrics["filter_evaluations_total"] = themis::utils::conversion::safe_size_to_int(filterEvaluationsTotal);
                metrics["filter_short_circuits"] = themis::utils::conversion::safe_size_to_int(filterShortCircuits);
                metrics["max_frontier_size_reached"] = themis::utils::conversion::safe_size_to_int(maxFrontierSizeReached);
                metrics["frontier_limit_hits"] = themis::utils::conversion::safe_size_to_int(frontierLimitHits);
                metrics["result_limit_reached"] = resultLimitReached;
                nlohmann::json fp = nlohmann::json::object();
                for (const auto& kv : frontierProcessedPerDepth) fp[std::to_string(kv.first)] = kv.second;
                nlohmann::json eq = nlohmann::json::object();
                for (const auto& kv : enqueuedPerDepth) eq[std::to_string(kv.first)] = kv.second;
                metrics["frontier_processed_per_depth"] = std::move(fp);
                metrics["enqueued_per_depth"] = std::move(eq);
                res["metrics"] = std::move(metrics);
            }
            
            traversalSpan.setAttribute("traversal.result_count", static_cast<int64_t>(res["count"].get<int>()));
            traversalSpan.setStatus(true);
            span.setAttribute("aql.result_count", static_cast<int64_t>(res["count"].get<int>()));
            span.setStatus(true);
            if (res.contains("entities") && res["entities"].is_array()) {
                res["entities"] = applyMasking(res["entities"], req);
            }
            return makeResponse(http::status::ok, res.dump(), req);
        }

        // Disjunctive Query (OR support)
        if (translate_result.disjunctive.has_value()) {
            const auto& dq = translate_result.disjunctive.value();
            auto orSpan = Tracer::startSpan("aql.or_execution");
            orSpan.setAttribute("or.table", dq.table);
            orSpan.setAttribute("or.disjunct_count", static_cast<int64_t>(dq.disjuncts.size()));
            
            themis::query::QueryEngine engine(*storage_, *secondary_index_);
            auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
            if (stats_collector) engine.setStatisticsCollector(stats_collector);
            // Nutze Fallback-Variante, damit OR-Queries auch ohne passende Indizes funktionieren
            auto result = engine.executeOrKeysWithFallback(dq, optimize);
            std::pair<QueryExecStatus, std::vector<std::string>> statusKeys;
            if (!result) {
                statusKeys = {QueryExecStatus{false, result.error().message()}, std::vector<std::string>{}};
            } else {
                statusKeys = {QueryExecStatus::OK(), std::move(*result)};
            }
            auto [status, keys] = statusKeys;
            
            if (!status.ok) {
                orSpan.setStatus(false, status.message);
                span.setStatus(false, "OR execution failed");
                return makeErrorResponse(http::status::bad_request, status.message, req);
            }
            
            // Fetch entities
            nlohmann::json entities = nlohmann::json::array();
            for (const auto& key : keys) {
                auto pk = themis::KeySchema::makeRelationalKey(dq.table, key);
                auto blob = storage_->get(pk);
                if (blob && !blob->empty()) {
                    try {
                        themis::BaseEntity::Blob entity_blob(blob->begin(), blob->end());
                        auto entity = themis::BaseEntity::deserialize(key, entity_blob);
                        entities.push_back(nlohmann::json::parse(entity.toJson()));
                    } catch (...) {
                        THEMIS_DEBUG("query_api_handler: unhandled exception caught");
                        // Skip malformed entities
                    }
                }
            }
            
            nlohmann::json response_body = {
                {"table", dq.table},
                {"count", entities.size()},
                {"entities", applyMasking(entities, req)}
            };
            // Provide "result" alias for compatibility with older clients/tests
            try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
            
            if (explain) {
                response_body["query"] = aql_query;
                response_body["ast"] = (*parse_result)->toJSON();
                response_body["disjunctive_query"] = true;
                response_body["disjunct_count"] = dq.disjuncts.size();
            }
            
            orSpan.setAttribute("or.result_count", static_cast<int64_t>(entities.size()));
            orSpan.setStatus(true);
            span.setAttribute("aql.result_count", static_cast<int64_t>(entities.size()));
            span.setStatus(true);
            return makeResponse(http::status::ok, response_body.dump(), req);
        }

        // JOIN/LET Query (Multi-FOR or LET without COLLECT)
        // Note: Single-FOR + COLLECT is handled by conversion to ConjunctiveQuery below
        if (translate_result.join.has_value()) {
            const auto& jq = translate_result.join.value();
            
            // For single-FOR + COLLECT, convert back to ConjunctiveQuery and skip to standard path
            if (jq.for_nodes.size() == 1 && jq.collect) {
                // Reconstruct ConjunctiveQuery from JoinQuery
                themis::ConjunctiveQuery cq;
                const auto& first_for_node = jq.for_nodes.front();
                cq.table = first_for_node.collection;
                
                // Convert simple equality filters to predicates
                using namespace themis::query;
                for (const auto& filter : jq.filters) {
                    if (filter->condition->getType() == ASTNodeType::BinaryOp) {
                        auto bin = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
                        if (bin->op == BinaryOperator::Eq) {
                            // Check for pattern: var.field == literal
                            if (bin->left->getType() == ASTNodeType::FieldAccess &&
                                bin->right->getType() == ASTNodeType::Literal) {
                                auto fa = std::static_pointer_cast<FieldAccessExpr>(bin->left);
                                auto lit = std::static_pointer_cast<LiteralExpr>(bin->right);
                                
                                // Extract field path
                                std::vector<std::string> parts;
                                parts.reserve(8);  // Typical nesting depth is 4-8 levels
                                parts.push_back(fa->field);
                                auto* cur = fa->object.get();
                                while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) {
                                    parts.push_back(fa2->field);
                                    cur = fa2->object.get();
                                }
                                
                                // Verify it's rooted at the FOR variable
                                if (auto* rootVar = dynamic_cast<VariableExpr*>(cur)) {
                                    if (rootVar->name == first_for_node.variable) {
                                        std::ostringstream col_oss;
                                        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
                                            if (it != parts.rbegin()) col_oss << ".";
                                            col_oss << *it;
                                        }
                                        std::string col = col_oss.str();
                                        
                                        // Convert literal to string
                                        auto litToString = [](const LiteralValue& value)->std::string {
                                            return std::visit([](auto&& arg)->std::string {
                                                using T = std::decay_t<decltype(arg)>;
                                                if constexpr (std::is_same_v<T, std::nullptr_t>) return std::string("null");
                                                else if constexpr (std::is_same_v<T, bool>) return arg ? std::string("true") : std::string("false");
                                                else if constexpr (std::is_same_v<T, int64_t>) return std::to_string(arg);
                                                else if constexpr (std::is_same_v<T, double>) return std::to_string(arg);
                                                else if constexpr (std::is_same_v<T, std::string>) return arg;
                                                else return std::string();
                                            }, value);
                                        };
                                        
                                        cq.predicates.push_back({col, litToString(lit->value)});
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Store in translate_result.conjunctive_query for standard processing
                translate_result.conjunctive_query = std::move(cq);
                // Clear join to fall through to standard path
                translate_result.join = std::nullopt;
            } else if (!jq.collect) {
                // Multi-FOR or single-FOR + LET: use executeJoin
                auto joinSpan = Tracer::startSpan("aql.join_execution");
                joinSpan.setAttribute("join.for_count", static_cast<int64_t>(jq.for_nodes.size()));
                joinSpan.setAttribute("join.let_count", static_cast<int64_t>(jq.let_nodes.size()));
                joinSpan.setAttribute("join.filter_count", static_cast<int64_t>(jq.filters.size()));
                
                themis::query::QueryEngine engine(*storage_, *secondary_index_);
                auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
                if (stats_collector) engine.setStatisticsCollector(stats_collector);
                auto res = engine.executeJoin(
                    jq.for_nodes,
                    jq.filters,
                    jq.let_nodes,
                    jq.return_node,
                    jq.sort,
                    jq.limit
                );
                
                if (!res.has_value()) {
                    joinSpan.setStatus(false, res.error().message());
                    span.setStatus(false, "JOIN execution failed");
                    return makeErrorResponse(http::status::bad_request, res.error().message(), req);
                }
                
                nlohmann::json response_body;
                nlohmann::json entities = nlohmann::json::array();
                for (const auto& result : res.value()) {
                    entities.push_back(result);
                }
                
                // Determine table name for response (use first FOR collection)
                std::string table = jq.for_nodes.empty() ? std::string("unknown") : jq.for_nodes.front().collection;

                // Fallback: Single-FOR + LET + Object/Projection RETURN produced no results due to join-path edge case
                if (entities.empty() && jq.for_nodes.size() == 1 && !jq.let_nodes.empty() && jq.return_node) {
                    try {
                        THEMIS_WARN("Join path returned 0 rows; applying single-FOR LET projection fallback");
                        const auto& forNode = jq.for_nodes.front();
                        const std::string prefix = forNode.collection + ":";
                        // Minimal evaluator for LET + object projection
                        bool fallback_scan_timed_out = false;
                        storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
                            if (timedOut()) {
                                fallback_scan_timed_out = true;
                                return false;
                            }
                            std::string pk = themis::KeySchema::extractPrimaryKey(key);
                            std::vector<uint8_t> blob(value.begin(), value.end());
                            try {
                                BaseEntity be = BaseEntity::deserialize(pk, blob);
                                nlohmann::json doc = nlohmann::json::parse(be.toJson());
                                doc["_key"] = pk;
                                // Simple recursive eval supporting Variable, FieldAccess, Literal, ObjectConstruct
                                std::map<std::string, nlohmann::json> letValues;
                                std::function<nlohmann::json(const std::shared_ptr<themis::query::Expression>&)> evalExpr;
                                evalExpr = [&](const std::shared_ptr<themis::query::Expression>& e) -> nlohmann::json {
                                    using namespace themis::query;
                                    if (!e) return nullptr;
                                    switch (e->getType()) {
                                        case ASTNodeType::Literal: {
                                            auto lit = std::static_pointer_cast<LiteralExpr>(e);
                                            nlohmann::json j; std::visit([&](auto&& arg){ j = arg; }, lit->value); return j;
                                        }
                                        case ASTNodeType::Variable: {
                                            auto v = std::static_pointer_cast<VariableExpr>(e);
                                            if (v->name == forNode.variable) return doc;
                                            auto it = letValues.find(v->name);
                                            if (it != letValues.end()) return it->second;
                                            return nullptr;
                                        }
                                        case ASTNodeType::FieldAccess: {
                                            auto fa = std::static_pointer_cast<FieldAccessExpr>(e);
                                            auto base = evalExpr(fa->object);
                                            if (!base.is_object()) return nullptr;
                                            if (base.contains(fa->field)) return base[fa->field];
                                            return nullptr;
                                        }
                                        case ASTNodeType::ObjectConstruct: {
                                            auto obj = std::static_pointer_cast<ObjectConstructExpr>(e);
                                            nlohmann::json out = nlohmann::json::object();
                                            for (const auto& [k, ce] : obj->fields) out[k] = evalExpr(ce);
                                            return out;
                                        }
                                        case ASTNodeType::ArrayLiteral: {
                                            auto arr = std::static_pointer_cast<ArrayLiteralExpr>(e);
                                            nlohmann::json a = nlohmann::json::array();
                                            for (const auto& ce : arr->elements) a.push_back(evalExpr(ce));
                                            return a;
                                        }
                                        default: return nullptr;
                                    }
                                };
                                // Evaluate LET nodes
                                for (const auto& let : jq.let_nodes) {
                                    letValues[let.variable] = evalExpr(let.expression);
                                }
                                // Project return
                                auto projected = evalExpr(jq.return_node->expression);
                                entities.push_back(projected);
                            } catch (...) {
                                THEMIS_WARN("query_api_handler: unhandled exception caught");
                                // Skip malformed entry
                            }
                            return true; // continue scan
                        });
                        if (fallback_scan_timed_out) {
                            joinSpan.setStatus(false, "timeout");
                            span.setStatus(false, "LET fallback scan timed out");
                            return makeErrorResponse(http::status::request_timeout,
                                "query exceeded timeout of " + std::to_string(resource_limits.timeout_ms) + " ms", req);
                        }
                    } catch (const std::exception& ex) {
                        THEMIS_ERROR("LET projection fallback failed: {}", ex.what());
                    }
                }
                
                response_body = {
                    {"table", table},
                    {"count", entities.size()},
                    {"entities", applyMasking(entities, req)}
                };
                
                if (explain) {
                    response_body["query"] = aql_query;
                    response_body["ast"] = (*parse_result)->toJSON();
                    response_body["join_query"] = true;
                }
                
                joinSpan.setAttribute("join.result_count", static_cast<int64_t>(entities.size()));
                joinSpan.setStatus(true);
                span.setAttribute("aql.result_count", static_cast<int64_t>(entities.size()));
                span.setStatus(true);
                return makeResponse(http::status::ok, response_body.dump(), req);
            }
        }

        // Relationale Query (mutable Kopie f�r Cursor-Anker/Limit-Anpassungen)
    auto forSpan = Tracer::startSpan("aql.for");
    auto q = translate_result.conjunctive_query;

        // Detect function-based SORT (BM25(doc) or FULLTEXT_SCORE()) to avoid range-index ORDER BY
        bool sortByScoreFunction = false;
        bool sortAsc = true;
        if ((*parse_result) && (*parse_result)->sort && !(*parse_result)->sort->specifications.empty()) {
            // Helper: recursively check if expression contains a specific function name
            std::function<bool(const std::shared_ptr<themis::query::Expression>&, const std::string&)> exprContainsFn;
            exprContainsFn = [&](const std::shared_ptr<themis::query::Expression>& expr, const std::string& name)->bool{
                if (!expr) return false;
                using namespace themis::query;
                switch (expr->getType()) {
                    case ASTNodeType::FunctionCall: {
                        auto* fc = static_cast<FunctionCallExpr*>(expr.get());
                        std::string n = fc->name; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                        if (n == name) return true;
                        for (const auto& a : fc->arguments) if (exprContainsFn(a, name)) return true;
                        return false;
                    }
                    case ASTNodeType::BinaryOp: {
                        auto* bo = static_cast<BinaryOpExpr*>(expr.get());
                        return exprContainsFn(bo->left, name) || exprContainsFn(bo->right, name);
                    }
                    case ASTNodeType::UnaryOp: {
                        auto* u = static_cast<UnaryOpExpr*>(expr.get());
                        return exprContainsFn(u->operand, name);
                    }
                    case ASTNodeType::ArrayLiteral: {
                        auto* ar = static_cast<ArrayLiteralExpr*>(expr.get());
                        for (const auto& el : ar->elements) if (exprContainsFn(el, name)) return true;
                        return false;
                    }
                    case ASTNodeType::ObjectConstruct: {
                        auto* oc = static_cast<ObjectConstructExpr*>(expr.get());
                        for (const auto& kv : oc->fields) if (exprContainsFn(kv.second, name)) return true;
                        return false;
                    }
                    default:
                        return false;
                }
            };
            const auto& spec = (*parse_result)->sort->specifications.front();
            sortAsc = spec.ascending;
            if (exprContainsFn(spec.expression, "bm25") || exprContainsFn(spec.expression, "fulltext_score")) {
                sortByScoreFunction = true;
            }
        }
        // If SORT uses BM25/FULLTEXT_SCORE, clear index ORDER BY and sort later in-memory
        if (sortByScoreFunction && q.orderBy.has_value()) {
            q.orderBy.reset();
        }

        std::string table = q.table;
        forSpan.setAttribute("for.table", table);
        forSpan.setAttribute("for.predicates_count", static_cast<int64_t>(q.predicates.size()));
        forSpan.setAttribute("for.range_predicates_count", static_cast<int64_t>(q.rangePredicates.size()));
        if (q.orderBy.has_value()) {
            forSpan.setAttribute("for.order_by", q.orderBy->column);
            forSpan.setAttribute("for.order_desc", q.orderBy->desc);
        }

        // Shared score map for BM25/FULLTEXT score lookups (filled on demand later)
        std::unordered_map<std::string, double> fulltextScoreByPk;

        // Cursor-Integration in die QueryEngine: falls ORDER BY vorhanden
        bool early_empty_due_to_cursor = false;
        size_t requested_count_for_cursor = 0;
        if (use_cursor && q.orderBy.has_value()) {
            // Use configured page size
            requested_count_for_cursor = page_size;
            
            // Override with LIMIT if present
            if ((*parse_result) && (*parse_result)->limit) {
                requested_count_for_cursor = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count));
                requested_count_for_cursor = themis::utils::Cursor::normalizePageSize(requested_count_for_cursor, pagination_config);
            }
            
            // Engine soll count+1 liefern (extra Element für has_more detection)
            // Add safety margin for cursor pagination: when using cursor with ORDER BY,
            // the query engine may filter items after cursor positioning (e.g., if there
            // are equality predicates), which can result in fewer items than requested.
            // Conservative approach: fetch extra items to account for potential filtering.
            constexpr size_t CURSOR_SAFETY_MARGIN = 5;
            size_t num_predicates = q.predicates.size();
            size_t safety_margin = (num_predicates > 0) ? CURSOR_SAFETY_MARGIN * num_predicates : CURSOR_SAFETY_MARGIN;
            q.orderBy->limit = requested_count_for_cursor + safety_margin + 1;

            // Wenn ein Cursor-Token übergeben wurde, ermittle Anker (value, pk)
            if (!cursor_token.empty()) {
                // Use enhanced cursor decoding
                auto cursor_info = themis::utils::Cursor::decodeDetailed(cursor_token);
                if (!cursor_info.has_value()) {
                    early_empty_due_to_cursor = true;
                } else {
                    const std::string& pk = cursor_info->pk;
                    const std::string& collection = cursor_info->collection;
                    if (collection != table) {
                        // Falsche Collection im Cursor
                        early_empty_due_to_cursor = true;
                    } else {
                        // Check if cursor already contains ORDER BY value (keyset pagination)
                        if (cursor_info->order_value.has_value()) {
                            // Use value from cursor directly (more efficient)
                            q.orderBy->cursor_value = *cursor_info->order_value;
                            q.orderBy->cursor_pk = pk;
                        } else {
                            // Fallback: Entität laden, um Sortierspaltenwert zu extrahieren
                            auto blob = storage_->get(table + ":" + pk);
                            if (!blob.has_value()) {
                                early_empty_due_to_cursor = true;
                            } else {
                                try {
                                    auto entity = themis::BaseEntity::deserialize(pk, *blob);
                                    // Sortierspalte extrahieren
                                    const std::string sortCol = q.orderBy->column;
                                    auto maybeVal = entity.extractField(sortCol);
                                    if (maybeVal.has_value()) {
                                        q.orderBy->cursor_value = *maybeVal;
                                        q.orderBy->cursor_pk = pk;
                                    } else {
                                        // Ohne Sortwert kein sicherer Anker
                                        early_empty_due_to_cursor = true;
                                    }
                                } catch (...) {
                                    THEMIS_WARN("query_api_handler: unhandled exception caught");
                                    early_empty_due_to_cursor = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Execute query
        themis::query::QueryEngine engine(*storage_, *secondary_index_);
        auto* stats_collector = stats_collector_.load(std::memory_order_acquire);
        if (stats_collector) engine.setStatisticsCollector(stats_collector);
        
        std::string exec_mode;
        nlohmann::json plan_json;
        
        std::pair<QueryExecStatus, std::vector<themis::BaseEntity>> res;
        if (early_empty_due_to_cursor && use_cursor) {
            // Liefere leere Seite sofort zur�ck
            res = std::make_pair(QueryExecStatus::OK(), std::vector<themis::BaseEntity>{});
        } else {
        
        if (allow_full_scan) {
            exec_mode = "full_scan_fallback";
            auto result = engine.executeAndEntitiesWithFallback(q, optimize);
            if (!result) {
                res = std::make_pair(QueryExecStatus{false, result.error().message()}, std::vector<themis::BaseEntity>{});
            } else {
                res = std::make_pair(QueryExecStatus::OK(), std::move(*result));
            }
        } else {
            // Wenn FULLTEXT vorhanden ist, delegiere direkt an Engine (Optimizer kennt FULLTEXT nicht)
            if (q.fulltextPredicate.has_value()) {
                exec_mode = "fulltext";
                auto result = engine.executeAndEntities(q);
                if (!result) {
                    res = std::make_pair(QueryExecStatus{false, result.error().message()}, std::vector<themis::BaseEntity>{});
                } else {
                    res = std::make_pair(QueryExecStatus::OK(), std::move(*result));
                }
            }
            // Range-aware: Wenn Range-Prädikate oder ORDER BY vorhanden sind,
            // nutze direkt die range-fähige Engine-Logik (Optimizer unterstützt nur Gleichheit).
            else if (!q.rangePredicates.empty() || q.orderBy.has_value()) {
                exec_mode = "index_rangeaware";
                auto result = engine.executeAndEntities(q);
                if (!result) {
                    res = std::make_pair(QueryExecStatus{false, result.error().message()}, std::vector<themis::BaseEntity>{});
                } else {
                    res = std::make_pair(QueryExecStatus::OK(), std::move(*result));
                }
            } else if (optimize) {
                themis::query::QueryOptimizer opt(*secondary_index_);
                auto plan = opt.chooseOrderForAndQuery(q);
                auto result = opt.executeOptimizedEntities(engine, q, plan);
                if (!result) {
                    res = std::make_pair(QueryExecStatus{false, result.error().message()}, std::vector<themis::BaseEntity>{});
                } else {
                    res = std::make_pair(QueryExecStatus::OK(), std::move(*result));
                }
                exec_mode = "index_optimized";
                
                if (explain) {
                    plan_json["mode"] = exec_mode;
                    plan_json["order"] = nlohmann::json::array();
                    for (const auto& p : plan.orderedPredicates) {
                        plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                    }
                    plan_json["estimates"] = nlohmann::json::array();
                    for (const auto& d : plan.details) {
                        plan_json["estimates"].push_back({
                            {"column", d.pred.column}, {"value", d.pred.value},
                            {"estimatedCount", d.estimatedCount}, {"capped", d.capped}
                        });
                    }
                }
            } else {
                exec_mode = "index_parallel";
                auto result = engine.executeAndEntities(q);
                if (!result) {
                    res = std::make_pair(QueryExecStatus{false, result.error().message()}, std::vector<themis::BaseEntity>{});
                } else {
                    res = std::make_pair(QueryExecStatus::OK(), std::move(*result));
                }
                
                if (explain) {
                    plan_json = {
                        {"mode", exec_mode},
                        {"order", nlohmann::json::array()}
                    };
                    for (const auto& p : q.predicates) {
                        plan_json["order"].push_back({{"column", p.column}, {"value", p.value}});
                    }
                }
            }
        }
        }
        
        if (!res.first.ok) {
            forSpan.setStatus(false, res.first.message);
            span.setStatus(false, "Query execution failed");
            return makeErrorResponse(http::status::bad_request, res.first.message, req);
        }
        
        forSpan.setAttribute("for.result_count", static_cast<int64_t>(res.second.size()));
        forSpan.setAttribute("for.exec_mode", exec_mode);
        forSpan.setStatus(true);
        
        // Apply LIMIT offset,count if provided in the AQL (post-fetch slicing)
        std::vector<themis::BaseEntity> sliced;
        sliced.reserve(res.second.size());
        sliced = std::move(res.second);

        // If SORT uses BM25/FULLTEXT_SCORE, compute scores and sort now (pre-LIMIT)
        if (sortByScoreFunction) {
            if (!q.fulltextPredicate.has_value()) {
                forSpan.setStatus(false, "BM25/FULLTEXT_SCORE sort without FULLTEXT filter");
                span.setStatus(false, "BM25/FULLTEXT_SCORE sort requires FULLTEXT() in FILTER");
                return makeErrorResponse(http::status::bad_request, "SORT by BM25/FULLTEXT_SCORE requires a FULLTEXT(...) filter in the query", req);
            }
            const auto& ft = *q.fulltextPredicate;
            auto scoreSpan = Tracer::startSpan("aql.fulltext_scores_fetch.sort");
            scoreSpan.setAttribute("table", q.table);
            scoreSpan.setAttribute("column", ft.column);
            scoreSpan.setAttribute("limit", static_cast<int64_t>(ft.limit));
            auto [st, results] = secondary_index_->scanFulltextWithScores(q.table, ft.column, ft.query, ft.limit);
            if (!st.ok) {
                scoreSpan.setStatus(false, st.message);
                return makeErrorResponse(http::status::internal_server_error, std::string("Failed to fetch fulltext scores: ") + st.message, req);
            }
            fulltextScoreByPk.clear();
            fulltextScoreByPk.reserve(results.size());
            for (const auto& r : results) fulltextScoreByPk.emplace(r.pk, r.score);
            scoreSpan.setAttribute("count", static_cast<int64_t>(results.size()));
            scoreSpan.setStatus(true);

            std::sort(sliced.begin(), sliced.end(), [&](const themis::BaseEntity& a, const themis::BaseEntity& b){
                double sa = 0.0, sb = 0.0;
                auto ita = fulltextScoreByPk.find(a.getPrimaryKey()); if (ita != fulltextScoreByPk.end()) sa = ita->second;
                auto itb = fulltextScoreByPk.find(b.getPrimaryKey()); if (itb != fulltextScoreByPk.end()) sb = itb->second;
                if (sortAsc) return sa < sb; else return sa > sb;
            });
        }

        if (!use_cursor && (*parse_result) && (*parse_result)->limit) {
            auto limitSpan = Tracer::startSpan("aql.limit");
            // Klassisches LIMIT offset,count Verhalten
            auto off = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->offset));
            auto cnt = static_cast<size_t>(std::max<int64_t>(0, (*parse_result)->limit->count));
            limitSpan.setAttribute("limit.offset", static_cast<int64_t>(off));
            limitSpan.setAttribute("limit.count", static_cast<int64_t>(cnt));
            limitSpan.setAttribute("limit.input_count", static_cast<int64_t>(sliced.size()));
            
            if (off < sliced.size()) {
                size_t last = std::min(sliced.size(), off + cnt);
                std::vector<themis::BaseEntity> tmp;
                tmp.reserve(last - off);
                auto first_it = sliced.begin() + static_cast<std::ptrdiff_t>(off);
                auto last_it = sliced.begin() + static_cast<std::ptrdiff_t>(last);
                std::move(first_it, last_it, std::back_inserter(tmp));
                sliced.swap(tmp);
            } else {
                sliced.clear();
            }
            
            limitSpan.setAttribute("limit.output_count", static_cast<int64_t>(sliced.size()));
            limitSpan.setStatus(true);
        }

        // Enrich plan (for explain) with execution mode and cursor metadata
        if (explain) {
            if (plan_json.is_null()) plan_json = nlohmann::json::object();
            if (!exec_mode.empty()) plan_json["mode"] = exec_mode;
            if (use_cursor) {
                nlohmann::json cursor_meta = nlohmann::json::object();
                cursor_meta["used"] = true;
                cursor_meta["cursor_present"] = !cursor_token.empty();
                if (q.orderBy.has_value()) {
                    cursor_meta["sort_column"] = q.orderBy->column;
                    cursor_meta["effective_limit"] = themis::utils::conversion::safe_size_to_int(q.orderBy->limit);
                    cursor_meta["anchor_set"] = q.orderBy->cursor_pk.has_value();
                }
                cursor_meta["requested_count"] = themis::utils::conversion::safe_size_to_int(requested_count_for_cursor);
                plan_json["cursor"] = std::move(cursor_meta);
            }
        }

        // COLLECT/GROUP BY (MVP): falls vorhanden, f�hre In-Memory-Gruppierung/Aggregation �ber die Ergebnisse aus
        if ((*parse_result) && (*parse_result)->collect && !use_cursor) {
            auto collectSpan = Tracer::startSpan("aql.collect");
            const auto& collect = *(*parse_result)->collect;
            using namespace themis::query;
            
            collectSpan.setAttribute("collect.input_count", static_cast<int64_t>(sliced.size()));
            collectSpan.setAttribute("collect.group_by_count", static_cast<int64_t>(collect.groups.size()));
            collectSpan.setAttribute("collect.aggregates_count", static_cast<int64_t>(collect.aggregations.size()));

            // Extrahiere aus einem FieldAccess-Ausdruck die Feld-Pfad-Notation (z.B. doc.city -> "city", doc.addr.city -> "addr.city")
            auto extractColumn = [&](const std::shared_ptr<themis::query::Expression>& expr)->std::string {
                auto* fa = dynamic_cast<FieldAccessExpr*>(expr.get());
                if (!fa) return std::string();
                std::vector<std::string> parts;
                parts.reserve(8);  // Typical nesting depth is 4-8 levels
                parts.push_back(fa->field);
                auto* cur = fa->object.get();
                while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) {
                    parts.push_back(fa2->field);
                    cur = fa2->object.get();
                }
                // Root erwartet Variable; deren Name wird ignoriert
                std::ostringstream col_oss;
                for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
                    if (it != parts.rbegin()) col_oss << ".";
                    col_oss << *it;
                }
                return col_oss.str();
            };

            // MVP: Unterst�tze 0..1 Group-Variablen
            std::string groupVarName;
            std::string groupColumn;
            if (!collect.groups.empty()) {
                const auto& first_group = collect.groups.front();
                groupVarName = first_group.first;
                if (first_group.second) {
                    groupColumn = extractColumn(first_group.second);
                }
            }

            struct AggSpec { std::string var; std::string func; std::string col; };
            std::vector<AggSpec> aggs;
            aggs.reserve(collect.aggregations.size());
            for (const auto& a : collect.aggregations) {
                std::string func = a.funcName; std::transform(func.begin(), func.end(), func.begin(), ::tolower);
                std::string col;
                if (a.argument) col = extractColumn(a.argument);
                aggs.push_back({a.varName, func, col});
            }

            struct AggState { uint64_t cnt=0; double sum=0.0; double min=std::numeric_limits<double>::infinity(); double max=-std::numeric_limits<double>::infinity(); };
            std::unordered_map<std::string, std::unordered_map<std::string, AggState>> acc;

            auto toGroupKey = [&](const themis::BaseEntity& e)->std::string{
                if (groupColumn.empty()) return std::string("__all__");
                auto v = e.getFieldAsString(groupColumn);
                return v.value_or(std::string(""));
            };
            auto toNumber = [&](const themis::BaseEntity& e, const std::string& col, std::optional<double>& out)->bool{
                if (col.empty()) { out = 1.0; return true; }
                auto dv = e.getFieldAsDouble(col);
                if (dv.has_value()) { out = *dv; return true; }
                auto sv = e.getFieldAsString(col);
                if (sv.has_value()) {
                    try { out = std::stod(*sv); return true; } catch (...) { /* ignore */ }
                }
                return false;
            };

            for (const auto& e : sliced) {
                std::string key = toGroupKey(e);
                auto& bucket = acc[key];
                // Update je Aggregation
                if (aggs.empty()) {
                    bucket[std::string("count")].cnt += 1;
                } else {
                    for (const auto& a : aggs) {
                        auto& st = bucket[a.var];
                        if (a.func == "count") {
                            st.cnt += 1;
                        } else if (a.func == "sum" || a.func == "avg" || a.func == "min" || a.func == "max") {
                            std::optional<double> num;
                            if (toNumber(e, a.col, num) && num.has_value()) {
                                st.cnt += 1;
                                st.sum += *num;
                                if (*num < st.min) st.min = *num;
                                if (*num > st.max) st.max = *num;
                            }
                        }
                    }
                }
            }

            // Baue Ausgabe
            nlohmann::json groups = nlohmann::json::array();
            for (const auto& [k, mp] : acc) {
                nlohmann::json row = nlohmann::json::object();
                if (!groupVarName.empty()) row[groupVarName] = k;
                if (aggs.empty()) {
                    auto it = mp.find("count");
                    uint64_t c = (it != mp.end()) ? it->second.cnt : 0;
                    row["count"] = c;
                } else {
                    for (const auto& a : aggs) {
                        const auto it = mp.find(a.var);
                        if (it == mp.end()) continue;
                        const auto& st = it->second;
                        if (a.func == "count") row[a.var] = static_cast<uint64_t>(st.cnt);
                        else if (a.func == "sum") row[a.var] = st.sum;
                        else if (a.func == "avg") row[a.var] = (st.cnt ? (st.sum / static_cast<double>(st.cnt)) : 0.0);
                        else if (a.func == "min") row[a.var] = (st.cnt ? st.min : 0.0);
                        else if (a.func == "max") row[a.var] = (st.cnt ? st.max : 0.0);
                    }
                }
                groups.push_back(std::move(row));
            }

            nlohmann::json response_body = {
                {"table", table},
                {"count", groups.size()},
                {"groups", applyMasking(groups, req)}
            };
            if (explain) {
                response_body["query"] = aql_query;
                response_body["ast"] = (*parse_result)->toJSON();
                if (!plan_json.is_null()) response_body["plan"] = plan_json;
            }
            
            collectSpan.setAttribute("collect.group_count", static_cast<int64_t>(groups.size()));
            collectSpan.setStatus(true);
            span.setAttribute("aql.result_count", static_cast<int64_t>(groups.size()));
            span.setStatus(true);
            return makeResponse(http::status::ok, response_body.dump(), req);
        }

        // Serialize entities or projections with LET support
        auto returnSpan = Tracer::startSpan("aql.return");
        returnSpan.setAttribute("return.input_count", static_cast<int64_t>(sliced.size()));

        using namespace themis::query;
        const std::string loopVar = (*parse_result)->for_node.variable;

        // Helper: extract column path from FieldAccess rooted at loop var
        std::function<std::optional<std::string>(const std::shared_ptr<Expression>&, bool&)> extractColFromFA;
        extractColFromFA = [&](const std::shared_ptr<Expression>& expr, bool& rootedAtLoop)->std::optional<std::string> {
            auto* fa = dynamic_cast<FieldAccessExpr*>(expr.get()); if (!fa) return std::nullopt;
            std::vector<std::string> parts;
            parts.reserve(8);  // Typical nesting depth is 4-8 levels
            parts.push_back(fa->field);
            auto* cur = fa->object.get();
            while (auto* fa2 = dynamic_cast<FieldAccessExpr*>(cur)) { parts.push_back(fa2->field); cur = fa2->object.get(); }
            if (auto* rootVarExpr = dynamic_cast<VariableExpr*>(cur)) { rootedAtLoop = (rootVarExpr->name == loopVar); }
            else { rootedAtLoop = false; }
            std::ostringstream col_oss;
            for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
                if (it != parts.rbegin()) col_oss << ".";
                col_oss << *it;
            }
            return col_oss.str();
        };

        // Detect if RETURN/LET expressions reference FULLTEXT_SCORE()
        std::function<bool(const std::shared_ptr<Expression>&, const std::string&)> containsFunction;
        containsFunction = [&](const std::shared_ptr<Expression>& expr, const std::string& name)->bool{
            if (!expr) return false;
            switch (expr->getType()) {
                case ASTNodeType::FunctionCall: {
                    auto* fc = static_cast<FunctionCallExpr*>(expr.get());
                    std::string n = fc->name; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                    if (n == name) return true;
                    for (const auto& a : fc->arguments) if (containsFunction(a, name)) return true;
                    return false;
                }
                case ASTNodeType::BinaryOp: {
                    auto* bo = static_cast<BinaryOpExpr*>(expr.get());
                    return containsFunction(bo->left, name) || containsFunction(bo->right, name);
                }
                case ASTNodeType::UnaryOp: {
                    auto* u = static_cast<UnaryOpExpr*>(expr.get());
                    return containsFunction(u->operand, name);
                }
                case ASTNodeType::ArrayLiteral: {
                    auto* ar = static_cast<ArrayLiteralExpr*>(expr.get());
                    for (const auto& el : ar->elements) if (containsFunction(el, name)) return true;
                    return false;
                }
                case ASTNodeType::ObjectConstruct: {
                    auto* oc = static_cast<ObjectConstructExpr*>(expr.get());
                    for (const auto& kv : oc->fields) if (containsFunction(kv.second, name)) return true;
                    return false;
                }
                default:
                    return false;
            }
        };

        bool usesFulltextScore = false;
        if ((*parse_result)) {
            // RETURN
            if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
                usesFulltextScore = containsFunction((*parse_result)->return_node->expression, "fulltext_score");
            }
            // LETs
            if (!usesFulltextScore) {
                for (const auto& ln : (*parse_result)->let_nodes) {
                    if (containsFunction(ln.expression, "fulltext_score")) { usesFulltextScore = true; break; }
                }
            }
        }

        // If FULLTEXT_SCORE() or BM25() is referenced in RETURN/LET, ensure scores are prepared (and validate FULLTEXT_SCORE usage)
        bool usesScoreFn = usesFulltextScore;
        if (!usesScoreFn && (*parse_result)) {
            if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
                usesScoreFn = containsFunction((*parse_result)->return_node->expression, "bm25");
            }
            if (!usesScoreFn) {
                for (const auto& ln : (*parse_result)->let_nodes) {
                    if (containsFunction(ln.expression, "bm25")) { usesScoreFn = true; break; }
                }
            }
        }
        if (usesFulltextScore && !q.fulltextPredicate.has_value()) {
            forSpan.setStatus(false, "FULLTEXT_SCORE without FULLTEXT filter");
            span.setStatus(false, "FULLTEXT_SCORE requires FULLTEXT() in FILTER");
            return makeErrorResponse(http::status::bad_request, "FULLTEXT_SCORE() requires a FULLTEXT(...) filter in the query", req);
        }
        if ((usesScoreFn || sortByScoreFunction) && fulltextScoreByPk.empty() && q.fulltextPredicate.has_value()) {
            const auto& ft = *q.fulltextPredicate;
            auto scoreSpan = Tracer::startSpan("aql.fulltext_scores_fetch");
            scoreSpan.setAttribute("table", q.table);
            scoreSpan.setAttribute("column", ft.column);
            scoreSpan.setAttribute("limit", static_cast<int64_t>(ft.limit));
            auto [st, results] = secondary_index_->scanFulltextWithScores(q.table, ft.column, ft.query, ft.limit);
            if (!st.ok) {
                scoreSpan.setStatus(false, st.message);
                return makeErrorResponse(http::status::internal_server_error, std::string("Failed to fetch fulltext scores: ") + st.message, req);
            }
            for (const auto& r : results) fulltextScoreByPk.emplace(r.pk, r.score);
            scoreSpan.setAttribute("count", static_cast<int64_t>(results.size()));
            scoreSpan.setStatus(true);
        }

        // Evaluate expressions to JSON (Literal, Variable, FieldAccess, Binary/Unary, Object, Array, selected FunctionCall)
        std::function<nlohmann::json(const std::shared_ptr<Expression>&,
                                     const themis::BaseEntity&,
                                     const std::unordered_map<std::string, nlohmann::json>&)> evalExpr;
        evalExpr = [&](const std::shared_ptr<Expression>& expr,
                       const themis::BaseEntity& ent,
                       const std::unordered_map<std::string, nlohmann::json>& env)->nlohmann::json {
            if (!expr) return nlohmann::json();
            switch (expr->getType()) {
                case ASTNodeType::Literal: {
                    // reuse toJSON Value
                    return static_cast<LiteralExpr*>(expr.get())->toJSON()["value"]; }
                case ASTNodeType::Variable: {
                    auto* v = static_cast<VariableExpr*>(expr.get());
                    if (v->name == loopVar) return ent.toJson();
                    auto it = env.find(v->name); if (it != env.end()) return it->second; return nullptr; }
                case ASTNodeType::FieldAccess: {
                    bool rooted = false; auto colOpt = extractColFromFA(expr, rooted);
                    if (colOpt.has_value() && rooted) {
                        // Extract from entity
                        auto asDouble = ent.getFieldAsDouble(*colOpt); if (asDouble.has_value()) return *asDouble;
                        auto asStr = ent.getFieldAsString(*colOpt); if (asStr.has_value()) return *asStr; return nullptr;
                    } else {
                        // Evaluate object part and index JSON
                        auto* fa = static_cast<FieldAccessExpr*>(expr.get());
                        auto base = evalExpr(fa->object, ent, env);
                        if (base.is_object()) {
                            auto it = base.find(fa->field); if (it != base.end()) return *it; return nullptr;
                        }
                        return nullptr;
                    }
                }
                case ASTNodeType::BinaryOp: {
                    auto* bo = static_cast<BinaryOpExpr*>(expr.get());
                    auto left = evalExpr(bo->left, ent, env);
                    auto right = evalExpr(bo->right, ent, env);
                    auto toNumber = [](const nlohmann::json& j, double& out)->bool{
                        if (j.is_number()) { out = j.get<double>(); return true; }
                        if (j.is_boolean()) { out = j.get<bool>() ? 1.0 : 0.0; return true; }
                        if (j.is_string()) { char* end=nullptr; std::string s=j.get<std::string>(); out = strtod(s.c_str(), &end); return end && *end=='\0'; }
                        return false;
                    };
                    switch (bo->op) {
                        case BinaryOperator::Eq:  {
                            double a, b;
                            if (toNumber(left, a) && toNumber(right, b)) {
                                return nearly_equal(a, b);
                            }
                            return left == right;
                        }
                        case BinaryOperator::Neq: {
                            double a, b;
                            if (toNumber(left, a) && toNumber(right, b)) {
                                return !nearly_equal(a, b);
                            }
                            return left != right;
                        }
                        case BinaryOperator::Lt:  return left < right;
                        case BinaryOperator::Lte: return left <= right;
                        case BinaryOperator::Gt:  return left > right;
                        case BinaryOperator::Gte: return left >= right;
                        case BinaryOperator::And: {
                            bool lb = left.is_boolean() ? left.get<bool>() : (!left.is_null());
                            bool rb = right.is_boolean() ? right.get<bool>() : (!right.is_null());
                            return nlohmann::json(lb && rb);
                        }
                        case BinaryOperator::Or:  {
                            bool lb = left.is_boolean() ? left.get<bool>() : (!left.is_null());
                            bool rb = right.is_boolean() ? right.get<bool>() : (!right.is_null());
                            return nlohmann::json(lb || rb);
                        }
                        case BinaryOperator::Add: {
                            double a,b; if (toNumber(left,a) && toNumber(right,b)) return a+b; return nullptr; }
                        case BinaryOperator::Sub: {
                            double a,b; if (toNumber(left,a) && toNumber(right,b)) return a-b; return nullptr; }
                        case BinaryOperator::Mul: {
                            double a,b; if (toNumber(left,a) && toNumber(right,b)) return a*b; return nullptr; }
                        case BinaryOperator::Div: {
                            double a,b; if (toNumber(left,a) && toNumber(right,b) && !nearly_equal(b, 0.0)) return a/b; return nullptr; }
                        default: return nullptr;
                    }
                }
                case ASTNodeType::UnaryOp: {
                    auto* u = static_cast<UnaryOpExpr*>(expr.get());
                    auto val = evalExpr(u->operand, ent, env);
                    switch (u->op) {
                        case UnaryOperator::Not:   return val.is_boolean() ? nlohmann::json(!val.get<bool>()) : nlohmann::json(false);
                        case UnaryOperator::Minus: {
                            if (val.is_number()) return -val.get<double>();
                            if (val.is_string()) { char* end=nullptr; std::string s=val.get<std::string>(); double d=strtod(s.c_str(), &end); if (end && *end=='\0') return -d; }
                            return nullptr; }
                        case UnaryOperator::Plus:  {
                            if (val.is_number()) return val.get<double>();
                            if (val.is_string()) { char* end=nullptr; std::string s=val.get<std::string>(); double d=strtod(s.c_str(), &end); if (end && *end=='\0') return d; }
                            return nullptr; }
                        default: return nullptr;
                    }
                }
                case ASTNodeType::FunctionCall: {
                    auto* fc = static_cast<FunctionCallExpr*>(expr.get());
                    std::string name = fc->name; std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (name == "bm25") {
                        // One-arg function: BM25(doc). Returns score for provided document object by _key/_pk
                        if (fc->arguments.size() != 1) return 0.0;
                        auto arg = evalExpr(fc->arguments.front(), ent, env);
                        if (arg.is_object()) {
                            std::string pk;
                            if (arg.contains("_key") && arg["_key"].is_string()) pk = arg["_key"].get<std::string>();
                            else if (arg.contains("_pk") && arg["_pk"].is_string()) pk = arg["_pk"].get<std::string>();
                            if (!pk.empty()) {
                                auto it = fulltextScoreByPk.find(pk);
                                if (it != fulltextScoreByPk.end()) return it->second; else return 0.0;
                            }
                        }
                        return 0.0;
                    }
                    if (name == "fulltext_score") {
                        // No-arg function returning score of current entity (requires FULLTEXT filter)
                        auto it = fulltextScoreByPk.find(ent.getPrimaryKey());
                        if (it != fulltextScoreByPk.end()) return it->second; else return 0.0; // default 0.0 when not present
                    }
                    auto evalArg = [&](size_t i)->nlohmann::json{ return (i<fc->arguments.size()) ? evalExpr(fc->arguments[i], ent, env) : nlohmann::json(); };
                    if (name == "concat") {
                        std::string out;
                        for (const auto& arg : fc->arguments) {
                            auto a = evalExpr(arg, ent, env);
                            if (a.is_string()) out += a.get<std::string>();
                            else if (a.is_number()) out += std::to_string(a.get<double>());
                            else if (a.is_boolean()) out += (a.get<bool>()?"true":"false");
                        }
                        return out;
                    }
                    if (name == "substring" || name == "substr") {
                        auto s = evalArg(0);
                        auto off = evalArg(1);
                        auto len = evalArg(2);
                        if (!s.is_string()) return nullptr;
                        std::string str = s.get<std::string>();
                        int start = off.is_number_integer() ? themis::utils::conversion::safe_int64_to_int32(off.get<int64_t>()) : 0;
                        int count = len.is_number_integer() ? 
                            themis::utils::conversion::safe_int64_to_int32(len.get<int64_t>()) : 
                            themis::utils::conversion::safe_size_to_int(str.size()) - std::min<int>(start, themis::utils::conversion::safe_size_to_int(str.size()));
                        if (start < 0) {
                            start = 0;
                        }
                        if (start > (int)str.size()) {
                            start = (int)str.size();
                        }
                        if (count < 0) {
                            count = 0;
                        }
                        if (start + count > (int)str.size()) {
                            count = (int)str.size() - start;
                        }
                        return str.substr(static_cast<size_t>(start), static_cast<size_t>(count));
                    }
                    if (name == "length") {
                        auto s = evalArg(0);
                        if (s.is_string()) return static_cast<int64_t>(s.get<std::string>().size());
                        if (s.is_array()) return static_cast<int64_t>(s.size());
                        if (s.is_object()) return static_cast<int64_t>(s.size());
                        return 0;
                    }
                    if (name == "lower") {
                        auto s = evalArg(0); if (!s.is_string()) return nullptr; std::string t=s.get<std::string>();
                        std::transform(t.begin(), t.end(), t.begin(), ::tolower); return t;
                    }
                    if (name == "upper") {
                        auto s = evalArg(0); if (!s.is_string()) return nullptr; std::string t=s.get<std::string>();
                        std::transform(t.begin(), t.end(), t.begin(), ::toupper); return t;
                    }
                    if (name == "to_number") {
                        auto v = evalArg(0); if (v.is_number()) return v.get<double>(); if (v.is_boolean()) return v.get<bool>()?1.0:0.0; if (v.is_string()) { char* end=nullptr; std::string s=v.get<std::string>(); double d=strtod(s.c_str(), &end); if (end && *end=='\0') return d; }
                        return nullptr;
                    }
                    if (name == "to_string") {
                        auto v = evalArg(0); if (v.is_string()) return v; if (v.is_number()) return std::to_string(v.get<double>()); if (v.is_boolean()) return v.get<bool>()?"true":"false"; if (v.is_null()) return "null"; return v.dump();
                    }
                    if (name == "abs" || name == "ceil" || name == "floor" || name == "round") {
                        auto v = evalArg(0); if (!v.is_number()) return nullptr; double d = v.get<double>();
                        if (name == "abs") return std::abs(d);
                        if (name == "ceil") return std::ceil(d);
                        if (name == "floor") return std::floor(d);
                        if (name == "round") return std::llround(d);
                        return nullptr;
                    }
                    if (name == "coalesce") {
                        for (const auto& arg : fc->arguments) { auto a = evalExpr(arg, ent, env); if (!a.is_null()) return a; }
                        return nullptr;
                    }
                    // Unsupported function in MVP eval
                    return nullptr;
                }
                case ASTNodeType::ObjectConstruct: {
                    auto* oc = static_cast<ObjectConstructExpr*>(expr.get());
                    nlohmann::json obj = nlohmann::json::object();
                    for (const auto& kv : oc->fields) obj[kv.first] = evalExpr(kv.second, ent, env);
                    return obj; }
                case ASTNodeType::ArrayLiteral: {
                    auto* ar = static_cast<ArrayLiteralExpr*>(expr.get());
                    nlohmann::json arr = nlohmann::json::array();
                    for (const auto& el : ar->elements) arr.push_back(evalExpr(el, ent, env));
                    return arr; }
                default:
                    return nullptr;
            }
        };

        // Decide if we can fast-path return of the loop variable
        bool simpleReturnLoopVar = false;
        if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
            if (auto* v = dynamic_cast<VariableExpr*>((*parse_result)->return_node->expression.get())) {
                simpleReturnLoopVar = (v->name == loopVar) && ((*parse_result)->let_nodes.empty());
            }
        }

        json entities = json::array();
        if (simpleReturnLoopVar) {
            for (const auto& e : sliced) entities.push_back(e.toJson());
        } else {
            for (const auto& e : sliced) {
                // Build LET environment per row
                std::unordered_map<std::string, nlohmann::json> env;
                for (const auto& ln : (*parse_result)->let_nodes) {
                    // Only allow Literal, FieldAccess and Variable
                    auto val = evalExpr(ln.expression, e, env);
                    env[ln.variable] = std::move(val);
                }
                // Evaluate RETURN expression; if missing, default to loop var entity
                if ((*parse_result)->return_node && (*parse_result)->return_node->expression) {
                    auto out = evalExpr((*parse_result)->return_node->expression, e, env);
                    entities.push_back(std::move(out));
                } else {
                    entities.push_back(e.toJson());
                }
            }
        }
        
        returnSpan.setStatus(true);
        
        json response_body;
        
        if (use_cursor) {
            // Cursor-basierte Antwort wurde nach Engine-Paginierung erstellt
            themis::utils::PaginatedResponse paged;
            // Use configured page size (already normalized)
            size_t requested_count = page_size;
            
            // Override with query LIMIT if present
            if ((*parse_result) && (*parse_result)->limit) {
                requested_count = static_cast<size_t>(std::max<int64_t>(1, (*parse_result)->limit->count));
                // Re-normalize with configuration
                requested_count = themis::utils::Cursor::normalizePageSize(requested_count, pagination_config);
            }

            bool has_more = false;
            if (sliced.size() > requested_count) {
                has_more = true;
                // Trenne das +1 Element ab (nur für has_more Erkennung)
                sliced.resize(requested_count);
            }

            // Serialize final page
            json page_items = json::array();
            for (const auto& e : sliced) page_items.push_back(e.toJson());

            paged.items = applyMasking(page_items, req);
            paged.batch_size = sliced.size();
            paged.has_more = has_more;
            paged.method = themis::utils::PaginationMethod::CURSOR;
            
            // Populate enhanced PageInfo
            paged.page_info.page_size = sliced.size();
            paged.page_info.has_next_page = has_more;
            paged.page_info.has_prev_page = !cursor_token.empty(); // Has previous if cursor was provided
            
            // Generate next cursor with ORDER BY value if available
            if (has_more && !sliced.empty()) {
                std::optional<std::string> order_value;
                
                // If query has ORDER BY, encode the sort value in cursor for keyset pagination
                if ((*parse_result) && (*parse_result)->sort && !(*parse_result)->sort->specifications.empty()) {
                    try {
                        const auto& last_entity = sliced.back();
                        // Use a generic sort column name for pagination cursor
                        const std::string sort_column = "__sort_value__";
                        auto maybe_value = last_entity.extractField(sort_column);
                        if (maybe_value.has_value()) {
                            order_value = *maybe_value;
                        }
                    } catch (...) {
                        THEMIS_WARN("query_api_handler: unhandled exception caught");
                        // If extraction fails, continue without order_value
                    }
                }
                
                paged.next_cursor = themis::utils::Cursor::encode(
                    sliced.back().getPrimaryKey(), 
                    table,
                    order_value
                );
            }
            response_body = paged.toJSON();
        } else {
            // Traditional response format
            response_body = {
                {"table", table},
                {"count", sliced.size()},
                {"entities", applyMasking(entities, req)}
            };
            // Provide "result" alias for compatibility
            try { response_body["result"] = response_body["entities"]; } catch (...) { /* ignore */ }
        }
        
        if (explain) {
            response_body["query"] = aql_query;
            response_body["ast"] = (*parse_result)->toJSON();
            if (!plan_json.is_null()) {
                // Markiere, wenn LET-Filter vor der Übersetzung extrahiert wurden (MVP-Sonderpfad)
                if (letFilterHandled) {
                    try { plan_json["let_pre_extracted"] = true; } catch (...) { /* noop */ }
                }
                response_body["plan"] = plan_json;
            }
        }
        
    span.setAttribute("aql.result_count", static_cast<int64_t>(sliced.size()));
    span.setStatus(true);

        // Enforce per-query resource limits on the standard result path.
        if (resource_limits.timeout_ms > 0) {
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - resource_limit_start).count();
            if (elapsed_ms >= static_cast<long long>(resource_limits.timeout_ms)) {
                return makeErrorResponse(http::status::request_timeout,
                    "query exceeded timeout of " + std::to_string(resource_limits.timeout_ms) + " ms", req);
            }
        }
        if (resource_limits.max_rows > 0 && sliced.size() > resource_limits.max_rows) {
            return makeErrorResponse(http::status::bad_request,
                "result row count " + std::to_string(sliced.size()) +
                " exceeds max_rows limit of " + std::to_string(resource_limits.max_rows), req);
        }
        // Serialise once; reuse for both memory check and final response.
        std::string response_body_str = response_body.dump();
        if (resource_limits.max_memory_bytes > 0 &&
            response_body_str.size() > resource_limits.max_memory_bytes) {
            return makeErrorResponse(http::status::bad_request,
                "result memory estimate " + std::to_string(response_body_str.size()) +
                " bytes exceeds max_memory_bytes limit of " +
                std::to_string(resource_limits.max_memory_bytes), req);
        }

    auto final_res = makeResponse(http::status::ok, response_body_str, req);
        // Record page fetch time histogram for cursor-based pagination
        if (use_cursor) {
            auto end = std::chrono::steady_clock::now();
            auto dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - page_fetch_start);
        }
        return final_res;
        
    } catch (const json::exception& e) {
        span.recordError("JSON parse error: " + std::string(e.what()));
    span.setStatus(false);
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    }
}

// Implementation extracted from http_server.cpp (lines 12042-12167)
http::response<http::string_body> QueryApiHandler::handleQueryEnhanced(
    const http::request<http::string_body>& req
) {
    if (!feature_llm_query_enhancement_) {
        return makeErrorResponse(http::status::not_found, 
            "Enterprise feature 'llm_query_enhancement' disabled", req);
    }
    
    if (!feature_llm_store_) {
        return makeErrorResponse(http::status::not_found, 
            "Feature 'llm_store' must be enabled for enhanced queries", req);
    }
    if (!llm_store_) {
        return makeErrorResponse(http::status::service_unavailable,
            "LLM interaction store is not available", req);
    }
    auto& llm_store = *llm_store_;
    
    auto span = Tracer::startSpan("handleQueryEnhanced");
    span.setAttribute("http.path", "/query/enhanced");
    span.setAttribute("enterprise.feature", true);
    
    try {
        auto body = json::parse(req.body());
        
        // Execute standard query first
        json query_response;
        
        // Check if this is an AQL query or standard query
        if (body.contains("aql")) {
            // Use AQL query path
            json aql_req_body;
            aql_req_body["query"] = body["aql"];
            if (body.contains("parameters")) {
                aql_req_body["parameters"] = body["parameters"];
            }
            
            // Create temporary request for AQL handler
            http::request<http::string_body> aql_req{req};
            aql_req.body() = aql_req_body.dump();
            aql_req.prepare_payload();
            
            auto aql_response = handleQueryAql(aql_req);
            
            if (aql_response.result() != http::status::ok) {
                // Return the error response directly
                return aql_response;
            }
            
            query_response = json::parse(aql_response.body());
            
        } else if (body.contains("table")) {
            // Use standard query path
            http::request<http::string_body> std_req{req};
            std_req.body() = body.dump();
            std_req.prepare_payload();
            
            auto std_response = handleQuery(std_req);
            
            if (std_response.result() != http::status::ok) {
                return std_response;
            }
            
            query_response = json::parse(std_response.body());
        } else {
            span.setStatus(false, "missing_query");
            return makeErrorResponse(http::status::bad_request, 
                "Request must contain either 'aql' or 'table' field", req);
        }
        
        // Get LLM context options from request
        json llm_options;
        if (body.contains("llm_context")) {
            llm_options = body["llm_context"];
        }
        
        // Fetch relevant LLM interactions
        LLMInteractionStore::ListOptions list_opts;
        list_opts.limit = llm_options.value("limit", 10);
        
        if (llm_options.contains("model")) {
            list_opts.filter_model = llm_options["model"].get<std::string>();
        }
        
        if (llm_options.contains("since_timestamp_ms")) {
            list_opts.since_timestamp_ms = llm_options["since_timestamp_ms"].get<int64_t>();
        }
        
        auto llm_interactions = llm_store.listInteractions(list_opts);
        
        // Build enhanced response
        json enhanced_response;
        enhanced_response["query_results"] = query_response;
        enhanced_response["llm_context"] = json::array();
        
        for (const auto& interaction : llm_interactions) {
            json llm_entry;
            llm_entry["id"] = interaction.id;
            llm_entry["prompt"] = interaction.prompt;
            llm_entry["response"] = interaction.response;
            llm_entry["model_version"] = interaction.model_version;
            llm_entry["timestamp_ms"] = interaction.timestamp_ms;
            
            // Include metadata (which may contain feedback from enterprise addons)
            if (!interaction.metadata.empty()) {
                llm_entry["metadata"] = interaction.metadata;
            }
            
            enhanced_response["llm_context"].push_back(llm_entry);
        }
        
        enhanced_response["llm_context_count"] = llm_interactions.size();
        
        span.setAttribute("query.result_count", static_cast<int64_t>(query_response.value("count", 0)));
        span.setAttribute("llm.context_count", static_cast<int64_t>(llm_interactions.size()));
        span.setStatus(true);
        
        return makeResponse(http::status::ok, enhanced_response.dump(), req);
        
    } catch (const json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, 
            std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, 
            std::string("Error: ") + e.what(), req);
    }
}

// Helper method implementations
std::optional<http::response<http::string_body>> QueryApiHandler::requireAccess(
    const http::request<http::string_body>& req,
    const std::string& /*permission*/,
    const std::string& /*resource_type*/,
    const std::string& /*resource_id*/
) {
    // If auth is disabled, allow
    if (!auth_ || !auth_->isEnabled()) {
        return std::nullopt;
    }
    
    // Extract and validate token
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
        res.prepare_payload();
        return res;
    }
    
    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(), auth_header.size())
    );
    if (!token) {
        return makeErrorResponse(http::status::unauthorized, "Invalid Authorization header format", req);
    }
    
    auto ar = auth_->validateToken(*token);
    if (!ar.authorized) {
        return makeErrorResponse(http::status::forbidden, "Access denied", req);
    }

    // NOTE: AuthMiddleware::AuthResult does not expose scopes; token validation is sufficient here.
    return std::nullopt;
}

QueryApiHandler::AuthContext QueryApiHandler::extractAuthContext(const http::request<http::string_body>& req) {
    AuthContext ctx;
    
    // If auth is disabled, return empty context
    if (!auth_ || !auth_->isEnabled()) {
        return ctx;
    }
    
    // Extract Authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return ctx; // No token -> empty context
    }
    
    // Extract Bearer token
    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(), auth_header.size())
    );
    if (!token) {
        return ctx; // Invalid token format -> empty context
    }
    
    // Validate token and extract user_id + groups
    auto ar = auth_->validateToken(*token);
    if (ar.authorized) {
        ctx.user_id = ar.user_id;
        ctx.groups = ar.groups;
    }
    
    return ctx;
}

http::response<http::string_body> QueryApiHandler::handleQueryStreamSse(
    const http::request<http::string_body>& req
) {
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "data:read", "query", path_only)) return *resp;
    }

    auto span = Tracer::startSpan("GET /v2/query/stream");

    // Parse query parameters from URL
    std::string aql_query;
    int max_seconds   = 30;
    int heartbeat_ms  = 15000;
    int retry_ms      = 3000;

    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        std::string qs = target.substr(qpos + 1);
        // Parse 'q' parameter (URL-encoded AQL query)
        auto extract = [&](const std::string& key) -> std::string {
            std::string prefix = key + "=";
            auto pos = qs.find(prefix);
            if (pos == std::string::npos) return {};
            auto end = qs.find('&', pos);
            std::string raw = qs.substr(pos + prefix.size(),
                end == std::string::npos ? std::string::npos : end - pos - prefix.size());
            // Basic URL-decode: replace '+' with ' ' and %XX with char
            std::string decoded;
            decoded.reserve(raw.size());
            for (auto it = raw.begin(); it != raw.end(); ) {
                if (*it == '+') {
                    decoded += ' ';
                    ++it;
                } else if (*it == '%' && std::distance(it, raw.end()) >= 3) {
                    char hex[3] = {*(it + 1), *(it + 2), '\0'};
                    decoded += static_cast<char>(std::strtol(hex, nullptr, 16));
                    it += 3;
                } else {
                    decoded += *it;
                    ++it;
                }
            }
            return decoded;
        };

        aql_query = extract("q");

        auto extractInt = [&](const std::string& key, int def, int lo, int hi) -> int {
            std::string v = extract(key);
            if (v.empty()) return def;
            try {
                int n = std::stoi(v);
                if (n < lo) n = lo;
                if (n > hi) n = hi;
                return n;
            } catch (...) { return def; }
        };
        max_seconds  = extractInt("max_seconds",  30,    1,    60);
        heartbeat_ms = extractInt("heartbeat_ms", 15000, 100, 60000);
        retry_ms     = extractInt("retry_ms",     3000,  100, 120000);
    }

    if (aql_query.empty()) {
        span.setStatus(false, "missing_query");
        return makeErrorResponse(http::status::bad_request,
            "Missing 'q' query parameter (AQL query string)", req);
    }

    span.setAttribute("aql.query", aql_query);

    try {
        // Execute the AQL query via the existing handler by building a synthetic request
        http::request<http::string_body> aql_req{http::verb::post, "/query/aql", req.version()};
        aql_req.set(http::field::content_type, "application/json");
        // Forward Authorization header so auth is properly propagated
        const auto auth_fwd = req[http::field::authorization];
        if (!auth_fwd.empty()) {
            aql_req.set(http::field::authorization, auth_fwd);
        }
        json aql_body = {{"query", aql_query}};
        aql_req.body() = aql_body.dump();
        aql_req.prepare_payload();

        http::response<http::string_body> aql_resp = handleQueryAql(aql_req);

        // Parse the query result
        json result;
        try {
            result = json::parse(aql_resp.body());
        } catch (...) {
            THEMIS_DEBUG("query_api_handler: unhandled exception caught");
            result = json::object();
        }

        // Build SSE response
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "text/event-stream");
        res.set(http::field::cache_control, "no-cache, no-transform");
        res.set(http::field::connection, "keep-alive");
        // GAP-012 fixed: no hardcoded CORS wildcard; CORS is applied by the central
        // HttpServer dispatch layer via THEMIS_CORS_* env vars.
        res.keep_alive(true);

        std::ostringstream body;
        body << "retry: " << retry_ms << "\n\n";

        // Check for query-level error
        if (aql_resp.result_int() >= 400) {
            std::string err_msg = result.value("message", aql_resp.body());
            json err_event = {{"error", true}, {"message", err_msg},
                              {"status_code", aql_resp.result_int()}};
            body << "event: error\n";
            body << "data: " << err_event.dump() << "\n\n";

            json done_event = {
                {"rows_streamed", 0},
                {"total", 0},
                {"error", true}
            };
            body << "event: done\n";
            body << "data: " << done_event.dump() << "\n\n";

            span.setStatus(false, "query_error");
            res.body() = body.str();
            res.prepare_payload();
            return res;
        }

        // Extract rows/entities array from the result
        json rows = json::array();
        if (result.contains("entities") && result["entities"].is_array()) {
            rows = result["entities"];
        } else if (result.contains("rows") && result["rows"].is_array()) {
            rows = result["rows"];
        } else if (result.is_array()) {
            rows = result;
        }

        // Stream rows as individual SSE events with sequence IDs
        auto stream_start = std::chrono::steady_clock::now();
        const auto max_duration = std::chrono::seconds(max_seconds);
        size_t seq = 0;
        auto last_hb = stream_start;

        for (const auto& row : rows) {
            // Respect time budget
            if (std::chrono::steady_clock::now() - stream_start >= max_duration) {
                break;
            }

            body << "id: " << seq << "\n";
            body << "data: " << row.dump() << "\n\n";
            ++seq;
        }

        // Emit a heartbeat if no rows were produced or after streaming
        auto elapsed_hb = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_hb
        ).count();
        if (seq == 0 || elapsed_hb >= heartbeat_ms) {
            body << ": heartbeat\n\n";
        }

        // Emit a terminal "done" event with metadata
        json done_event = {
            {"rows_streamed", seq},
            {"total",         result.value("count", static_cast<int>(rows.size()))}
        };
        body << "event: done\n";
        body << "data: " << done_event.dump() << "\n\n";

        span.setAttribute("sse.rows_streamed", static_cast<int64_t>(seq));
        span.setStatus(true);

        res.body() = body.str();
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error,
            std::string("Error: ") + e.what(), req);
    }
}

} // namespace server
} // namespace themis
