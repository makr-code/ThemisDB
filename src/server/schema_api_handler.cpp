// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "server/schema_api_handler.h"
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
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
{
    spdlog::info("SchemaApiHandler initialized");
}

SchemaApiHandler::~SchemaApiHandler() {
    spdlog::info("SchemaApiHandler destroyed");
}

http::response<http::string_body> SchemaApiHandler::handleGetSchema(
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
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
        
        spdlog::debug("Schema API: Returned full schema");
        return res;
        
    } catch (const std::exception& e) {
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
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
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
        
        spdlog::debug("Schema API: Returned {} tables", tables.size());
        return res;
        
    } catch (const std::exception& e) {
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
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
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
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Table name is required";
            res.body() = error_resp.dump();
            res.result(http::status::bad_request);
            res.prepare_payload();
            return res;
        }
        
        // Get specific table schema
        auto table_opt = schema_mgr_->getTable(table_name);
        
        if (!table_opt.has_value()) {
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
        
        spdlog::debug("Schema API: Returned schema for table '{}'", table_name);
        return res;
        
    } catch (const std::exception& e) {
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
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
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
        
        spdlog::debug("Schema API: Returned capabilities");
        return res;
        
    } catch (const std::exception& e) {
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

http::response<http::string_body> SchemaApiHandler::handlePutSchema(
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Extract table name from path: /api/v1/schema/:tablename
        std::string target = std::string(req.target());
        std::string prefix = "/api/v1/schema/";
        
        if (target.find(prefix) != 0 || target == "/api/v1/schema/" || 
            target == "/api/v1/schema/tables" || target.find("/api/v1/schema/tables/") == 0) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Invalid URL format. Use: PUT /api/v1/schema/:tablename";
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
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Table name is required";
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
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        if (!schema_mgr_) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Schema manager not available";
            res.body() = error_resp.dump();
            res.result(http::status::service_unavailable);
            res.prepare_payload();
            return res;
        }

        // Extract table name from path: /api/v1/schema/:tablename
        std::string target = std::string(req.target());
        std::string prefix = "/api/v1/schema/";
        
        if (target.find(prefix) != 0 || target == "/api/v1/schema/" || 
            target == "/api/v1/schema/tables" || target.find("/api/v1/schema/tables/") == 0) {
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Invalid URL format. Use: PATCH /api/v1/schema/:tablename";
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
            json error_resp;
            error_resp["status"] = "error";
            error_resp["message"] = "Table name is required";
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

} // namespace server
} // namespace themis
