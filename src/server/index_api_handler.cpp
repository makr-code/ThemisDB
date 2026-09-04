/**
 * @file index_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/index_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/adaptive_index.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/input_validator.h"
#include <sstream>

using json = nlohmann::json;

namespace themis {
namespace server {

IndexApiHandler::IndexApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<AdaptiveIndexManager> adaptive_index,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , secondary_index_(std::move(secondary_index))
    , adaptive_index_(std::move(adaptive_index))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> IndexApiHandler::handleCreate(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        if (!body.contains("table")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table'", req);
        }
        std::string table = body["table"].get<std::string>();
        
        // QW-46 Guard: Fail-closed collection name validation
        {
            utils::InputValidator validator;
            if (!validator.validateStringLength(table, 256) || !validator.validatePathSegment(table)) {
                THEMIS_ERROR("QW-46 Guard: Invalid table name in handleCreate; only alphanumeric, underscore, and hyphen allowed");
                return makeErrorResponse(http::status::bad_request,
                    "Invalid table name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
            }
        }
        
        bool unique = false;
        if (body.contains("unique")) {
            unique = body["unique"].get<bool>();
        }

        // Support range index creation via type = "range"
        if (body.contains("type")) {
            std::string type = body["type"].get<std::string>();
            if (type == "range") {
                if (!body.contains("column")) {
                    return makeErrorResponse(http::status::bad_request, "Missing 'column' for range index", req);
                }
                std::string column = body["column"].get<std::string>();
                auto st = secondary_index_->createRangeIndex(table, column);
                if (!st.ok) {
                    return makeErrorResponse(http::status::bad_request, st.message, req);
                }
                json resp = {{"success", true}, {"table", table}, {"column", column}, {"type", "range"}};
                return makeResponse(http::status::ok, resp.dump(), req);
            } else if (type == "fulltext") {
                if (!body.contains("column")) {
                    return makeErrorResponse(http::status::bad_request, "Missing 'column' for fulltext index", req);
                }
                std::string column = body["column"].get<std::string>();
                
                // Parse optional config
                SecondaryIndexManager::FulltextConfig config;
                if (body.contains("config") && body["config"].is_object()) {
                    auto configObj = body["config"];
                    config.stemming_enabled = configObj.value("stemming_enabled", false);
                    config.language = configObj.value("language", "none");
                    config.stopwords_enabled = configObj.value("stopwords_enabled", false);
                    if (configObj.contains("stopwords") && configObj["stopwords"].is_array()) {
                        for (const auto& s : configObj["stopwords"]) {
                            if (s.is_string()) {
                              config.stopwords.push_back(s.get<std::string>());
                            }
                        }
                    }
                    config.normalize_umlauts = configObj.value("normalize_umlauts", false);
                } else {
                    config.stemming_enabled = false;
                    config.language = "none";
                    config.stopwords_enabled = false;
                    config.normalize_umlauts = false;
                }
                
                auto st = secondary_index_->createFulltextIndex(table, column, config);
                if (!st.ok) {
                    return makeErrorResponse(http::status::bad_request, st.message, req);
                }
                
                json resp = {
                    {"success", true}, 
                    {"table", table}, 
                    {"column", column}, 
                    {"type", "fulltext"},
                    {"config", {
                        {"stemming_enabled", config.stemming_enabled},
                        {"language", config.language},
                        {"stopwords_enabled", config.stopwords_enabled},
                        {"stopwords", config.stopwords},
                        {"normalize_umlauts", config.normalize_umlauts}
                    }}
                };
                return makeResponse(http::status::ok, resp.dump(), req);
            }
        }

        // Support single-column (column) and composite (columns)
        if (body.contains("columns")) {
            if (!body["columns"].is_array() || body["columns"].empty()) {
                return makeErrorResponse(http::status::bad_request, "'columns' must be a non-empty array of strings", req);
            }
            std::vector<std::string> columns = {};

            for (const auto& c : body["columns"]) {
                columns.push_back(c.get<std::string>());
            }
            auto st = secondary_index_->createCompositeIndex(table, columns, unique);
            if (!st.ok) {
                return makeErrorResponse(http::status::bad_request, st.message, req);
            }
            json resp = {{"success", true}, {"table", table}, {"columns", columns}, {"unique", unique}};
            return makeResponse(http::status::ok, resp.dump(), req);
        }
        
        if (!body.contains("column")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'column' or 'columns'", req);
        }
        std::string column = body["column"].get<std::string>();
        auto st = secondary_index_->createIndex(table, column, unique);
        if (!st.ok) {
            return makeErrorResponse(http::status::bad_request, st.message, req);
        }
        json resp = {{"success", true}, {"table", table}, {"column", column}, {"unique", unique}};
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleDrop(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        if (!body.contains("table") || !body.contains("column")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' or 'column'", req);
        }
        std::string table = body["table"].get<std::string>();
        
        // QW-46 Guard: Fail-closed collection name validation
        {
            utils::InputValidator validator;
            if (!validator.validateStringLength(table, 256) || !validator.validatePathSegment(table)) {
                THEMIS_ERROR("QW-46 Guard: Invalid table name in handleDrop; only alphanumeric, underscore, and hyphen allowed");
                return makeErrorResponse(http::status::bad_request,
                    "Invalid table name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
            }
        }
        
        std::string column = body["column"].get<std::string>();

        // Optional type for dropping range indexes
        if (body.contains("type") && body["type"].is_string() && body["type"].get<std::string>() == "range") {
            auto st = secondary_index_->dropRangeIndex(table, column);
            if (!st.ok) {
                return makeErrorResponse(http::status::bad_request, st.message, req);
            }
            json resp = {{"success", true}, {"table", table}, {"column", column}, {"type", "range"}};
            return makeResponse(http::status::ok, resp.dump(), req);
        }

        auto st = secondary_index_->dropIndex(table, column);
        if (!st.ok) {
            return makeErrorResponse(http::status::bad_request, st.message, req);
        }
        json resp = {{"success", true}, {"table", table}, {"column", column}};
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    try {
        std::string table;
        std::string column;

        // Try parsing JSON body first
        if (!req.body().empty()) {
            json body = json::parse(req.body());
            if (body.contains("table")) {
                table = body["table"];
            }
            if (body.contains("column")) {
                column = body["column"];
            }
        }

        // If no JSON, try query parameters from target
        if (table.empty()) {
            std::string target = std::string(req.target());
            size_t query_start = target.find('?');
            if (query_start != std::string::npos) {
                std::string query = target.substr(query_start + 1);
                // Simple query parser: table=X&column=Y
                size_t pos = 0;
                while (pos < query.size()) {
                    size_t eq = query.find('=', pos);
                    if (eq == std::string::npos) {
                      break;
                    }
                    size_t amp = query.find('&', eq);
                    if (amp == std::string::npos) {
                      amp = query.size();
                    }
                    
                    std::string key = query.substr(pos, eq - pos);
                    std::string value = query.substr(eq + 1, amp - eq - 1);
                    
                    if (key == "table") {
                      table = value;
                    }
                    else if (key == "column") column = value;
                    
                    pos = amp + 1;
                }
            }
        }

        if (table.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' parameter", req);
        }
        
        // QW-46 Guard: Fail-closed collection name validation
        {
            utils::InputValidator validator;
            if (!validator.validateStringLength(table, 256) || !validator.validatePathSegment(table)) {
                THEMIS_ERROR("QW-46 Guard: Invalid table name in handleStats; only alphanumeric, underscore, and hyphen allowed");
                return makeErrorResponse(http::status::bad_request,
                    "Invalid table name: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
            }
        }

        // If column specified, get single index stats
        if (!column.empty()) {
            auto stats = secondary_index_->getIndexStats(table, column);
            json resp = {
                {"type", stats.type},
                {"table", stats.table},
                {"column", stats.column},
                {"entry_count", stats.entry_count},
                {"estimated_size_bytes", stats.estimated_size_bytes},
                {"unique", stats.unique}
            };
            if (!stats.additional_info.empty()) {
                resp["additional_info"] = stats.additional_info;
            }
            return makeResponse(http::status::ok, resp.dump(), req);
        } else {
            // Get all index stats for table
            auto all_stats = secondary_index_->getAllIndexStats(table);
            json resp = json::array();
            for (const auto& stats : all_stats) {
                json stat_obj = {
                    {"type", stats.type},
                    {"table", stats.table},
                    {"column", stats.column},
                    {"entry_count", stats.entry_count},
                    {"estimated_size_bytes", stats.estimated_size_bytes},
                    {"unique", stats.unique}
                };
                if (!stats.additional_info.empty()) {
                    stat_obj["additional_info"] = stats.additional_info;
                }
                resp.push_back(stat_obj);
            }
            return makeResponse(http::status::ok, resp.dump(), req);
        }
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleRebuild(
    const http::request<http::string_body>& req
) {
    try {
        json body = json::parse(req.body());
        
        if (!body.contains("table") || !body.contains("column")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' or 'column'", req);
        }

        std::string table = body["table"];
        std::string column = body["column"];

        // Rebuild the index – online mode keeps the live index readable throughout
        bool online = body.value("online", false);
        if (online) {
            uint32_t throttle_us = body.value("throttle_us", 0u);
            secondary_index_->rebuildIndexOnline(table, column, throttle_us);
        } else {
            secondary_index_->rebuildIndex(table, column);
        }

        // Get updated stats
        auto stats = secondary_index_->getIndexStats(table, column);
        
        json resp = {
            {"success", true},
            {"table", table},
            {"column", column},
            {"online", online},
            {"entry_count", stats.entry_count},
            {"estimated_size_bytes", stats.estimated_size_bytes}
        };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleReindex(
    const http::request<http::string_body>& req
) {
    try {
        json body = json::parse(req.body());
        
        if (!body.contains("table")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table'", req);
        }

        std::string table = body["table"];

        // Reindex the entire table
        secondary_index_->reindexTable(table);

        // Get all index stats
        auto all_stats = secondary_index_->getAllIndexStats(table);
        
        json resp = {
            {"success", true},
            {"table", table},
            {"indexes_rebuilt", all_stats.size()}
        };
        
        // Include stats for each index
        json stats_array = json::array();
        for (const auto& stats : all_stats) {
            stats_array.push_back({
                {"column", stats.column},
                {"type", stats.type},
                {"entry_count", stats.entry_count}
            });
        }
        resp["indexes"] = stats_array;
        
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleSuggestions(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexSuggestions");
    
    try {
        auto target = std::string(req.target());
        
        // Parse query parameters
        std::string collection;
        double min_score = 0.5;
        size_t limit = 10;
        
        // Extract query params from URL
        auto query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = target.substr(query_pos + 1);
            std::istringstream iss(query_string);
            std::string param;
            
            while (std::getline(iss, param, '&')) {
                auto eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    
                    if (key == "collection") {
                        collection = value;
                    } else if (key == "min_score") {
                        min_score = std::stod(value);
                    } else if (key == "limit") {
                        limit = std::stoull(value);
                    }
                }
            }
        }
        
        span.setAttribute("collection", collection);
        span.setAttribute("min_score", min_score);
        span.setAttribute("limit", static_cast<int64_t>(limit));
        
        auto suggestions = adaptive_index_->getSuggestions(collection, min_score, limit);
        
        json response = json::array();
        for (const auto& suggestion : suggestions) {
            response.push_back(suggestion.toJson());
        }
        
        span.setAttribute("suggestions.count", static_cast<int64_t>(suggestions.size()));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> IndexApiHandler::handlePatterns(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexPatterns");
    
    try {
        auto target = std::string(req.target());
        
        // Parse collection from query params
        std::string collection;
        auto query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = target.substr(query_pos + 1);
            auto coll_pos = query_string.find("collection=");
            if (coll_pos != std::string::npos) {
                collection = query_string.substr(coll_pos + 11);
                auto amp_pos = collection.find('&');
                if (amp_pos != std::string::npos) {
                    collection = collection.substr(0, amp_pos);
                }
            }
        }
        
        span.setAttribute("collection", collection);
        
        auto patterns = adaptive_index_->getPatterns(collection);
        
        json response = json::array();
        for (const auto& pattern : patterns) {
            response.push_back(pattern.toJson());
        }
        
        span.setAttribute("patterns.count", static_cast<int64_t>(patterns.size()));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleRecordPattern(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexRecordPattern");
    
    try {
        json body = json::parse(req.body());
        
        std::string collection = body.value("collection", "");
        std::string field = body.value("field", "");
        std::string operation = body.value("operation", "eq");
        int64_t execution_time_ms = body.value("execution_time_ms", int64_t(0));
        
        // Validate required fields
        if (collection.empty() || field.empty()) {
            span.setStatus(false, "missing_fields");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required fields: collection, field", req);
        }
        
        span.setAttribute("collection", collection);
        span.setAttribute("field", field);
        span.setAttribute("operation", operation);
        
        adaptive_index_->getPatternTracker()->recordPattern(
            collection, field, operation, execution_time_ms
        );
        
        json response = {
            {"status", "recorded"},
            {"collection", collection},
            {"field", field},
            {"operation", operation}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> IndexApiHandler::handleClearPatterns(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexClearPatterns");
    
    try {
        size_t count_before = adaptive_index_->getPatternTracker()->size();
        
        adaptive_index_->getPatternTracker()->clear();
        
        json response = {
            {"status", "cleared"},
            {"patterns_removed", count_before}
        };
        
        span.setAttribute("patterns.removed", static_cast<int64_t>(count_before));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> IndexApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> IndexApiHandler::makeResponse(
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

