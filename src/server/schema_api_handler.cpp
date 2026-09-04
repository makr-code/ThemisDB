/**
 * @file schema_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=23, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "server/schema_api_handler.h"
#include <stdexcept>
#include "metadata/schema_manager.h"
#include "metadata/information_schema.h"
#include "metadata/statistics_collector.h"
#include "metadata/schema_constraints.h"
#include "metadata/schema_version_manager.h"
#include "metadata/index_recommender.h"
#include "metadata/schema_audit_log.h"
#include "metadata/column_lineage.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "utils/tracing.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

SchemaApiHandler::SchemaApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    SchemaManager* schema_mgr
)
    : storage_(storage)
    , secondary_index_(secondary_index)
    , schema_mgr_(schema_mgr)
    , stats_collector_(nullptr)
    , schema_constraints_(nullptr)
    , version_mgr_(nullptr)
    , index_recommender_(nullptr)
    , audit_log_(nullptr)
    , column_lineage_tracker_(nullptr)
{
    spdlog::info([[maybe_unused]] "SchemaApiHandler initialized");
}

SchemaApiHandler::~SchemaApiHandler() {
    spdlog::info([[maybe_unused]] "SchemaApiHandler destroyed");
}

http::response<http::string_body> SchemaApiHandler::handleGetSchema(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("GET /schema");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            span.setStatus(false, "Schema manager not available");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Get full schema from SchemaManager
        auto schema_json = schema_mgr_->toJSON();
        res.body() = schema_json.dump(2);  // Pretty print with 2-space indent
        res.prepare_payload();
        
        span.setStatus(true);
        spdlog::debug("Schema API: Returned full schema");
        return res;
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, e.what());
        spdlog::error("Schema API error: {}", e.what());
        
        json error_resp;
        error_resp["status"] = "error";
        error_resp["message"] = std::string("Failed to retrieve schema: ") + e.what();
        res.body() = error_resp.dump();
        res.result(http::status::internal_server_error);
        res.prepare_payload();
        return res;
    }
}

http::response<http::string_body> SchemaApiHandler::handleGetTables(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("GET /schema/tables");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            span.setStatus(false, "Schema manager not available");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Get all tables from SchemaManager
        auto tables = schema_mgr_->getAllTables();
        span.setAttribute("schema.table_count", static_cast<int64_t>(tables.size()));
        
        json response;
        response["status"] = "success";
        response["tables"] = json::array();
        
        for (const auto& table : tables) {
            json table_info;
            table_info["name"] = table.name;
            table_info["type"] = table.type;
            table_info["estimated_row_count"] = table.estimated_row_count;
            table_info["property_count"] = table.properties.size();
            table_info["index_count"] = table.indexes.size();
            response["tables"].push_back(table_info);
        }
        
        res.body() = response.dump(2);
        res.prepare_payload();
        
        span.setStatus(true);
        spdlog::debug("Schema API: Returned {} tables", tables.size());
        return res;
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, e.what());
        spdlog::error("Schema API error (get tables): {}", e.what());
        
        json error_resp;
        error_resp["status"] = "error";
        error_resp["message"] = std::string("Failed to retrieve tables: ") + e.what();
        res.body() = error_resp.dump();
        res.result(http::status::internal_server_error);
        res.prepare_payload();
        return res;
    }
}

http::response<http::string_body> SchemaApiHandler::handleGetTable(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("GET /schema/tables/:name");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            span.setStatus(false, "Schema manager not available");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Extract table name from path: /api/v1/schema/tables/:name
        std::string target = std::string(req.target());
        std::string prefix = "/api/v1/schema/tables/";
        
        if (target.find(prefix) != 0) {
            span.setStatus(false, "Invalid URL format");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Invalid URL format";
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }
        
        std::string table_name = target.substr(prefix.length());
        
        // Remove query string if present
        size_t query_pos = table_name.find('?');
        if (query_pos != std::string::npos) {
            table_name = table_name.substr(0, query_pos);
        }
        
        if (table_name.empty()) {
            span.setStatus(false, "Table name is required");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Table name is required";
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        span.setAttribute("schema.table_name", table_name);
        
        // Get specific table schema
        auto table_opt = schema_mgr_->getTable(table_name);
        
        if (!table_opt.has_value()) {
            span.setStatus(false, "Table not found");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Table not found: " + table_name;
            res.body() = error_resp.dump();
            res.result(http::status::not_found);
            res.prepare_payload();
            return res;
        }
        
        json response;
        response["status"] = "success";
        response["table"] = table_opt->toJSON();
        
        res.body() = response.dump(2);
        res.prepare_payload();
        
        span.setStatus(true);
        spdlog::debug("Schema API: Returned schema for table '{}'", table_name);
        return res;
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, e.what());
        spdlog::error("Schema API error (get table): {}", e.what());
        
        json error_resp;
        error_resp["status"] = "error";
        error_resp["message"] = std::string("Failed to retrieve table schema: ") + e.what();
        res.body() = error_resp.dump();
        res.result(http::status::internal_server_error);
        res.prepare_payload();
        return res;
    }
}

http::response<http::string_body> SchemaApiHandler::handleGetCapabilities(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("GET /schema/capabilities");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            span.setStatus(false, "Schema manager not available");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Get capabilities from SchemaManager
        auto capabilities_json = schema_mgr_->getCapabilitiesJSON();
        res.body() = capabilities_json.dump(2);
        res.prepare_payload();
        
        span.setStatus(true);
        spdlog::debug("Schema API: Returned capabilities");
        return res;
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, e.what());
        spdlog::error("Schema API error (capabilities): {}", e.what());
        
        json error_resp;
        error_resp["status"] = "error";
        error_resp["message"] = std::string("Failed to retrieve capabilities: ") + e.what();
        res.body() = error_resp.dump();
        res.result(http::status::internal_server_error);
        res.prepare_payload();
        return res;
    }
}

std::string SchemaApiHandler::extractAndValidateSchemaTableName(
    const std::string& target,
    std::string& table_name) const
{
    const std::string prefix = "/api/v1/schema/";
    
    // Check URL format
    if (target.find(prefix) != 0 || target == "/api/v1/schema/" || 
        target == "/api/v1/schema/tables" || target.find("/api/v1/schema/tables/") == 0) {
        return "Invalid URL format. Use: /api/v1/schema/:tablename";
    }
    
    // Extract table name
    table_name = target.substr(prefix.length());
    
    // Remove query string if present
    size_t query_pos = table_name.find('?');
    if (query_pos != std::string::npos) {
        table_name = table_name.substr(0, query_pos);
    }
    
    // Validate table name is not empty
    if (table_name.empty()) {
        return "Table name is required";
    }
    
    return "";  // Success
}

http::response<http::string_body> SchemaApiHandler::handlePutSchema(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("PUT /schema/tables/:name");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            span.setStatus(false, "Schema manager not available");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Extract and validate table name from URL
        std::string table_name;
        std::string url_error = extractAndValidateSchemaTableName(std::string(req.target()), table_name);
        
        if (!url_error.empty()) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = url_error;
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        // Parse request body
        if (req.body().empty()) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Request body is required";
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        json schema_json;
        try {
            schema_json = json::parse(req.body());
        } catch (const json::exception& e) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = std::string("Invalid JSON: ") + e.what();
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        // Ensure name matches URL
        if (schema_json.contains("name") && schema_json["name"].is_string()) {
            std::string schema_name = schema_json["name"].get<std::string>();
            if (schema_name != table_name) {
                json error_resp;
                error_resp["status"] = "error";
                error_resp["message"] = "Schema name in body must match table name in URL";
                res.body() = error_resp.dump();
                res.result(http::status::bad_request);
                res.prepare_payload();
                return res;
            }
        } else {
            // Add name if not present
            schema_json["name"] = table_name;
        }

        // Parse schema
        SchemaManager::TableSchema schema;
        try {
            schema = SchemaManager::parseTableSchema(schema_json);
        } catch (const std::exception& e) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = std::string("Failed to parse schema: ") + e.what();
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        // Store schema
        bool success = schema_mgr_->setTableSchema(table_name, schema);
        
        if (!success) {
            // Get validation error
            std::string validation_error = schema_mgr_->validateSchema(schema);
            
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = validation_error.empty() ? 
                "Failed to store schema" : validation_error;
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        json response;
        response["status"] = "success";
        response["message"] = "Schema stored successfully";
        response["table_name"] = table_name;
        
        res.body() = response.dump(2);
        res.result(http::status::created);
        res.prepare_payload();
        
        spdlog::info("Schema API: Stored schema for table '{}'", table_name);
        return res;
        
    } catch (const std::exception& e) {
        spdlog::error("Schema API error (put schema): {}", e.what());
        
        json error_resp;
        error_resp["status"] = "error";
        error_resp["message"] = std::string("Internal error: ") + e.what();
        res.body() = error_resp.dump();
        res.result(http::status::internal_server_error);
        res.prepare_payload();
        return res;
    }
}

http::response<http::string_body> SchemaApiHandler::handlePatchSchema(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("PATCH /schema/tables/:name");
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            span.setStatus(false, "Schema manager not available");
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Extract and validate table name from URL
        std::string table_name;
        std::string url_error = extractAndValidateSchemaTableName(std::string(req.target()), table_name);
        
        if (!url_error.empty()) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = url_error;
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        // Parse request body
        if (req.body().empty()) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Request body is required";
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        json updates;
        try {
            updates = json::parse(req.body());
        } catch (const json::exception& e) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = std::string("Invalid JSON: ") + e.what();
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        // Apply patch
        bool success = schema_mgr_->patchTableSchema(table_name, updates);
        
        if (!success) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Failed to patch schema. Table may not exist or updates are invalid.";
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }

        json response;
        response["status"] = "success";
        response["message"] = "Schema updated successfully";
        response["table_name"] = table_name;
        
        res.body() = response.dump(2);
        res.prepare_payload();
        
        spdlog::info("Schema API: Patched schema for table '{}'", table_name);
        return res;
        
    } catch (const std::exception& e) {
        spdlog::error("Schema API error (patch schema): {}", e.what());
        
        json error_resp;
        error_resp["status"] = "error";
        error_resp["message"] = std::string("Internal error: ") + e.what();
        res.body() = error_resp.dump();
        res.result(http::status::internal_server_error);
        res.prepare_payload();
        return res;
    }
}

// ============================================================================
// Setter methods
// ============================================================================

void SchemaApiHandler::setStatisticsCollector([[maybe_unused]] StatisticsCollector* stats_collector) {
    stats_collector_ = stats_collector;
}

void SchemaApiHandler::setSchemaConstraints([[maybe_unused]] SchemaConstraints* schema_constraints) {
    schema_constraints_ = schema_constraints;
}

void SchemaApiHandler::setSchemaVersionManager([[maybe_unused]] SchemaVersionManager* version_mgr) {
    version_mgr_ = version_mgr;
}

void SchemaApiHandler::setIndexRecommender([[maybe_unused]] metadata::IndexRecommender* index_recommender) {
    index_recommender_ = index_recommender;
}

void SchemaApiHandler::setAuditLog([[maybe_unused]] SchemaAuditLog* audit_log) {
    audit_log_ = audit_log;
}

void SchemaApiHandler::setColumnLineageTracker([[maybe_unused]] themis::metadata::ColumnLineageTracker* tracker) {
    column_lineage_tracker_ = tracker;
}

// ============================================================================
// Helper methods
// ============================================================================

http::response<http::string_body> SchemaApiHandler::makeError(
    const http::request<http::string_body>& req,
    http::status status,
    const std::string& message) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    json j;
    j["status"]  = "error";
    j["message"] = message;
    res.body() = j.dump();
    res.prepare_payload();
    return res;
}

std::string SchemaApiHandler::extractTableName(
    const std::string& target,
    const std::string& prefix,
    std::string& table_name) const
{
    if (target.find(prefix) != 0) {
        return "Invalid URL prefix";
    }
    table_name = target.substr(prefix.size());
    // Strip query string
    auto qpos = table_name.find('?');
    if (qpos != std::string::npos) {
        table_name = table_name.substr(0, qpos);
    }
    if (table_name.empty()) {
        return "Table name is required";
    }
    return "";
}

// ============================================================================
// Information Schema endpoints
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetInformationSchema(
    const http::request<http::string_body>& req)
{
    if (!schema_mgr_) {
        return makeError(req, http::status::service_unavailable,
                         "Schema manager not available");
    }

    try {
        std::string target = std::string(req.target());
        InformationSchema is(*schema_mgr_);
        json response;
        response["status"] = "success";

        // Route to sub-views
        if (target == "/api/v1/information_schema" ||
            target == "/api/v1/information_schema/")
        {
            response["data"] = is.toJSON();

        } else if (target == "/api/v1/information_schema/tables") {
            response["tables"] = is.tablesToJSON();

        } else if (target.find("/api/v1/information_schema/columns") == 0) {
            std::string table_name;
            std::string err = extractTableName(
                target, "/api/v1/information_schema/columns/", table_name);
            if (err.empty() && !table_name.empty()) {
                response["columns"] = is.columnsToJSON(table_name);
            } else {
                response["columns"] = [&]() {
                    json arr = json::array();
                    for (const auto& col : is.getColumns()) {
                        arr.push_back(col.toJSON());
                    }
                    return arr;
                }();
            }

        } else if (target.find("/api/v1/information_schema/statistics") == 0) {
            std::string table_name;
            std::string err = extractTableName(
                target, "/api/v1/information_schema/statistics/", table_name);
            json stats_arr = json::array();
            if (err.empty() && !table_name.empty()) {
                for (const auto& s : is.getStatistics(std::string_view(table_name))) {
                    stats_arr.push_back(s.toJSON());
                }
            } else {
                for (const auto& s : is.getStatistics()) {
                    stats_arr.push_back(s.toJSON());
                }
            }
            response["statistics"] = stats_arr;

        } else {
            return makeError(req, http::status::not_found,
                             "Unknown information_schema endpoint: " + target);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = response.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        spdlog::error("Schema API error (information_schema): {}", e.what());
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Statistics endpoints
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetStats(
    const http::request<http::string_body>& req)
{
    if (!stats_collector_) {
        return makeError(req, http::status::service_unavailable,
                         "Statistics collector not available");
    }
    auto& stats_collector = *stats_collector_;

    std::string table_name;
    std::string err = extractTableName(
        std::string(req.target()), "/api/v1/metadata/stats/", table_name);
    if (!err.empty()) {
        return makeError(req, http::status::bad_request, err);
    }

    try {
        auto result = stats_collector.getStats(table_name);
        if (!result.ok) {
            return makeError(req, http::status::not_found, result.error_message);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        json j;
        j["status"] = "success";
        j["stats"]  = result.value.toJSON();
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

http::response<http::string_body> SchemaApiHandler::handleCollectStats(
    const http::request<http::string_body>& req)
{
    if (!stats_collector_) {
        return makeError(req, http::status::service_unavailable,
                         "Statistics collector not available");
    }
    auto& stats_collector = *stats_collector_;

    std::string table_name;
    std::string err = extractTableName(
        std::string(req.target()), "/api/v1/metadata/stats/", table_name);
    if (!err.empty()) {
        return makeError(req, http::status::bad_request, err);
    }

    try {
        auto result = stats_collector.collectStats(table_name);
        if (!result.ok) {
            return makeError(req, http::status::internal_server_error,
                             result.error_message);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        json j;
        j["status"]  = "success";
        j["message"] = "Statistics collected for table '" + table_name + "'";
        j["stats"]   = result.value.toJSON();
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Constraints endpoints
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetConstraints(
    const http::request<http::string_body>& req)
{
    if (!schema_constraints_) {
        return makeError(req, http::status::service_unavailable,
                         "Schema constraints not available");
    }
    auto& schema_constraints = *schema_constraints_;

    std::string table_name;
    std::string err = extractTableName(
        std::string(req.target()), "/api/v1/metadata/constraints/", table_name);
    if (!err.empty()) {
        return makeError(req, http::status::bad_request, err);
    }

    try {
        auto constraints = schema_constraints.getTableConstraints(table_name);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        json j;
        j["status"]     = "success";
        j["table_name"] = table_name;
        json c_arr = json::array();
        for (const auto& c : constraints) {
            c_arr.push_back(c.toJSON());
        }
        j["constraints"] = c_arr;
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Schema version endpoints
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetVersionHistory(
    const http::request<http::string_body>& req)
{
    if (!version_mgr_) {
        return makeError(req, http::status::service_unavailable,
                         "Schema version manager not available");
    }
    auto& version_mgr = *version_mgr_;

    std::string table_name;
    std::string err = extractTableName(
        std::string(req.target()), "/api/v1/schema/versions/", table_name);
    if (!err.empty()) {
        return makeError(req, http::status::bad_request, err);
    }

    try {
        auto result = version_mgr.getChangeHistory(table_name);
        if (!result.ok) {
            return makeError(req, http::status::not_found, result.error_message);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        json j;
        j["status"]     = "success";
        j["table_name"] = table_name;
        j["history"]    = version_mgr.historyToJSON(table_name);
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

http::response<http::string_body> SchemaApiHandler::handleCreateVersion(
    const http::request<http::string_body>& req)
{
    if (!version_mgr_) {
        return makeError(req, http::status::service_unavailable,
                         "Schema version manager not available");
    }
    auto& version_mgr = *version_mgr_;

    std::string table_name;
    std::string err = extractTableName(
        std::string(req.target()), "/api/v1/schema/versions/", table_name);
    if (!err.empty()) {
        return makeError(req, http::status::bad_request, err);
    }

    try {
        // Parse optional body for author/description
        std::string author, description;
        if (!req.body().empty()) {
            try {
                json body = json::parse(req.body());
                author      = body.value("author",      std::string{});
                description = body.value("description", std::string{});
            } catch (...) {}
        }

        auto result = version_mgr.createSchemaVersion(table_name, author, description);
        if (!result.ok) {
            return makeError(req, http::status::unprocessable_entity,
                             result.error_message);
        }

        http::response<http::string_body> res{http::status::created, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        json j;
        j["status"]     = "success";
        j["table_name"] = table_name;
        j["version"]    = result.value;
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

http::response<http::string_body> SchemaApiHandler::handleGetDiff(
    const http::request<http::string_body>& req)
{
    if (!version_mgr_) {
        return makeError(req, http::status::service_unavailable,
                         "Schema version manager not available");
    }

    std::string table_name;
    std::string err = extractTableName(
        std::string(req.target()), "/api/v1/schema/diff/", table_name);
    if (!err.empty()) {
        return makeError(req, http::status::bad_request, err);
    }

    // Parse query params ?from=V&to=V
    // table_name may contain "?from=1&to=2" — split it
    uint64_t version_a = 0, version_b = 0;
    auto qpos = table_name.find('?');
    if (qpos != std::string::npos) {
        std::string query = table_name.substr(qpos + 1);
        table_name = table_name.substr(0, qpos);

        // Very simple query string parser
        auto parse_param = [&]([[maybe_unused]] const std::string& name) -> uint64_t {
            std::string key = name + "=";
            auto pos = query.find(key);
            if (pos == std::string::npos) return 0;
            pos += key.size();
            auto end = query.find('&', pos);
            std::string val = (end == std::string::npos)
                ? query.substr(pos)
                : query.substr(pos, end - pos);
            try { return std::stoull(val); } catch (...) { return 0; }
        };

        version_a = parse_param("from");
        version_b = parse_param("to");
    }

    if (version_a == 0 || version_b == 0) {
        return makeError(req, http::status::bad_request,
                         "Query parameters 'from' and 'to' (version numbers) are required");
    }

    try {
        auto result = version_mgr_->diffVersions(table_name, version_a, version_b);
        if (!result.ok) {
            return makeError(req, http::status::not_found, result.error_message);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        json j;
        j["status"] = "success";
        j["diff"]   = result.value;
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Index recommendations endpoint
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetIndexRecommendations(
    const http::request<http::string_body>& req)
{
    if (!index_recommender_) {
        return makeError(req, http::status::service_unavailable,
                         "Index recommender not available");
    }
    auto& index_recommender = *index_recommender_;

    try {
        std::string target = std::string(req.target());
        std::string base   = "/api/v1/metadata/index_recommendations";
        std::string prefix = base + "/";

        json j;
        j["status"] = "success";

        if (target == base || target == base + "/") {
            // All tables
            auto all_recs = index_recommender.recommendAll();
            json rec_obj = json::object();
            for (const auto& [table_name, recs] : all_recs) {
                json rec_arr = json::array();
                for (const auto& r : recs) {
                    rec_arr.push_back(r.toJSON());
                }
                rec_obj[table_name] = rec_arr;
            }
            j["recommendations"] = rec_obj;

        } else if (target.find(prefix) == 0) {
            // Single table
            std::string table_name = target.substr(prefix.size());
            // Strip query string
            auto qpos = table_name.find('?');
            if (qpos != std::string::npos) table_name = table_name.substr(0, qpos);

            if (table_name.empty()) {
                return makeError(req, http::status::bad_request,
                                 "Table name is required");
            }

            auto recs = index_recommender.recommend(table_name);
            json rec_arr = json::array();
            for (const auto& r : recs) {
                rec_arr.push_back(r.toJSON());
            }
            j["table_name"]      = table_name;
            j["recommendations"] = rec_arr;

        } else {
            return makeError(req, http::status::not_found,
                             "Unknown endpoint: " + target);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Audit log endpoint
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetAuditLog(
    const http::request<http::string_body>& req)
{
    if (!audit_log_) {
        return makeError(req, http::status::service_unavailable,
                         "Audit log not available");
    }
    auto& audit_log = *audit_log_;
    try {
        std::string target = std::string(req.target());
        std::string base   = "/api/v1/metadata/audit";
        std::string prefix = base + "/";

        json j;
        j["status"] = "success";

        if (target == base || target == base + "/") {
            j["audit"] = audit_log.fullHistoryToJSON();
        } else if (target.find(prefix) == 0) {
            std::string table_name = target.substr(prefix.size());
            auto qpos = table_name.find('?');
            if (qpos != std::string::npos) table_name = table_name.substr(0, qpos);
            if (table_name.empty()) {
                return makeError(req, http::status::bad_request, "Table name required");
            }
            j["table_name"] = table_name;
            j["audit"]      = audit_log.historyToJSON(table_name);
        } else {
            return makeError(req, http::status::not_found,
                             "Unknown endpoint: " + target);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Schema import endpoint
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleSchemaImport(
    const http::request<http::string_body>& req)
{
    if (!schema_mgr_) {
        return makeError(req, http::status::service_unavailable,
                         "Schema manager not available");
    }
    auto& schema_mgr = *schema_mgr_;
    try {
        auto body = json::parse(req.body());
        if (!body.contains("tables") || !body["tables"].is_array()) {
            return makeError(req, http::status::bad_request,
                             "Body must contain a 'tables' array");
        }

        std::vector<std::string> imported;
        std::vector<json>        errors;

        for (const auto& schema_json : body["tables"]) {
            try {
                auto schema = SchemaManager::parseTableSchema(schema_json);
                if (schema.name.empty()) {
                    errors.push_back({{"error", "Table schema missing 'name' field"}});
                    continue;
                }

                bool ok = schema_mgr.setTableSchema(schema.name, schema);
                if (!ok) {
                    errors.push_back({{"table", schema.name},
                                      {"error", "Failed to register schema"}});
                    continue;
                }

                // Audit
                if (audit_log_) {
                    audit_log_->record(schema.name, "import", "", "bulk schema import", 0,
                                       {{"source", "schema_import_api"}});
                }
                imported.push_back(schema.name);

            } catch (const std::exception& ex) {
                errors.push_back({{"error", std::string("Parse error: ") + ex.what()}});
            }
        }

        json j;
        j["status"]   = errors.empty() ? "success" : "partial";
        j["imported"] = imported;
        j["errors"]   = errors;
        j["imported_count"] = imported.size();
        j["error_count"]    = errors.size();

        http::status status = errors.empty() ? http::status::ok
                                             : http::status::multi_status;
        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const json::parse_error& e) {
        return makeError(req, http::status::bad_request,
                         std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Batch constraint validation
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleBatchConstraintValidation(
    const http::request<http::string_body>& req)
{
    if (!schema_constraints_) {
        return makeError(req, http::status::service_unavailable,
                         "Constraint engine not available");
    }
    auto& schema_constraints = *schema_constraints_;
    try {
        // Extract table name from /api/v1/metadata/constraints/validate/:table
        std::string target = std::string(req.target());
        std::string prefix = "/api/v1/metadata/constraints/validate/";
        std::string table_name;
        std::string err = extractTableName(target, prefix, table_name);
        if (!err.empty()) {
            return makeError(req, http::status::bad_request, err);
        }

        auto body = json::parse(req.body());
        if (!body.contains("rows") || !body["rows"].is_array()) {
            return makeError(req, http::status::bad_request,
                             "Body must contain a 'rows' array");
        }

        json valid_rows    = json::array();
        json invalid_rows  = json::array();

        size_t row_index = 0;
        for (const auto& row_json : body["rows"]) {
            // Convert JSON object to string map
            std::map<std::string, themis::ColumnValue> row;
            if (row_json.is_object()) {
                for (auto& [k, v] : row_json.items()) {
                    if (v.is_string()) {
                        row[k] = v.get<std::string>();
                    } else if (v.is_boolean()) {
                        row[k] = v.get<bool>();
                    } else if (v.is_number_integer()) {
                        row[k] = v.get<int64_t>();
                    } else if (v.is_number_float()) {
                        row[k] = v.get<double>();
                    } else if (v.is_null()) {
                        row[k] = std::monostate{};
                    } else {
                        row[k] = v.dump();
                    }
                }
            }

            auto violations = schema_constraints.enforce(table_name, row);
            if (violations.empty()) {
                valid_rows.push_back({{"index", row_index}, {"row", row_json}});
            } else {
                json viol_arr = json::array();
                for (const auto& v : violations) viol_arr.push_back(v.toJSON());
                invalid_rows.push_back({{"index", row_index}, {"row", row_json},
                                        {"violations", viol_arr}});
            }
            ++row_index;
        }

        json j;
        j["status"]        = invalid_rows.empty() ? "success" : "violations_found";
        j["table_name"]    = table_name;
        j["total_rows"]    = row_index;
        j["valid_count"]   = valid_rows.size();
        j["invalid_count"] = invalid_rows.size();
        j["invalid_rows"]  = invalid_rows;

        http::status http_st = invalid_rows.empty() ? http::status::ok
                                                     : http::status::unprocessable_entity;
        http::response<http::string_body> res{http_st, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = j.dump(2);
        res.prepare_payload();
        return res;

    } catch (const json::parse_error& e) {
        return makeError(req, http::status::bad_request,
                         std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// Column Lineage endpoints
// ============================================================================

http::response<http::string_body> SchemaApiHandler::handleGetColumnLineage(
    const http::request<http::string_body>& req)
{
    if (!column_lineage_tracker_) {
        return makeError(req, http::status::service_unavailable,
            "Column lineage tracker not available");
    }
    auto& column_lineage_tracker = *column_lineage_tracker_;

    try {
        const std::string base    = "/api/v1/metadata/lineage/";
        std::string       target  = std::string(req.target());
        // Strip query string
        auto qpos = target.find('?');
        if (qpos != std::string::npos) target = target.substr(0, qpos);

        if (target.rfind(base, 0) != 0) {
            return makeError(req, http::status::bad_request, "Invalid lineage path");
        }

        std::string rest = target.substr(base.size());  // e.g. "users" or "users/full_name"
        auto sep = rest.find('/');

        json body;
        if (sep == std::string::npos) {
            // GET /api/v1/metadata/lineage/:table
            if (rest.empty()) {
                return makeError(req, http::status::bad_request,
                    "Table name required");
            }
            body = column_lineage_tracker.exportTableLineage(rest);
        } else {
            // GET /api/v1/metadata/lineage/:table/:column
            std::string table_name  = rest.substr(0, sep);
            std::string column_name = rest.substr(sep + 1);
            if (table_name.empty() || column_name.empty()) {
                return makeError(req, http::status::bad_request,
                    "Table and column name required");
            }
            themis::metadata::ColumnRef col{table_name, column_name};
            body = column_lineage_tracker.getColumnProvenance(col);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = body.dump(2);
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

http::response<http::string_body> SchemaApiHandler::handleRecordLineageDerivation(
    const http::request<http::string_body>& req)
{
    if (!column_lineage_tracker_) {
        return makeError(req, http::status::service_unavailable,
            "Column lineage tracker not available");
    }
    auto& column_lineage_tracker = *column_lineage_tracker_;

    try {
        json body = json::parse(req.body());

        themis::metadata::ColumnLineageEntry entry;
        entry.entry_id = body.value("entry_id", std::string{});
        entry.timestamp_ms = body.value("timestamp_ms", int64_t{0});
        entry.performed_by = body.value("performed_by", std::string{});
        entry.transformation_expression =
            body.value("transformation_expression", std::string{});

        if (body.contains("metadata") && !body["metadata"].is_null()) {
            entry.metadata = body["metadata"];
        }

        // target column
        if (!body.contains("target") || !body["target"].is_object()) {
            return makeError(req, http::status::bad_request,
                "'target' field (object with 'table' and 'column') is required");
        }
        entry.target_column = themis::metadata::ColumnRef::fromJSON(body["target"]);

        // source columns (optional)
        if (body.contains("source_columns") && body["source_columns"].is_array()) {
            for (const auto& src : body["source_columns"]) {
                entry.source_columns.push_back(
                    themis::metadata::ColumnRef::fromJSON(src));
            }
        }

        // transformation type
        entry.transformation = themis::metadata::TransformationType::DIRECT_COPY;
        if (body.contains("transformation") && body["transformation"].is_string()) {
            entry.transformation = themis::metadata::transformationTypeFromString(
                body["transformation"].get<std::string>());
        }

        column_lineage_tracker.recordDerivation(std::move(entry));

        json resp_body;
        resp_body["status"]      = "recorded";
        resp_body["total_entries"] = column_lineage_tracker.totalEntryCount();

        http::response<http::string_body> res{http::status::created, req.version()};
        res.set(http::field::server, "ThemisDB");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = resp_body.dump(2);
        res.prepare_payload();
        return res;

    } catch (const json::parse_error& e) {
        return makeError(req, http::status::bad_request,
                         std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        return makeError(req, http::status::internal_server_error,
                         std::string("Internal error: ") + e.what());
    }
}

} // namespace server
} // namespace themis

