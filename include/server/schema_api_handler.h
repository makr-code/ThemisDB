// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class SchemaManager;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Schema API Handler for HTTP Server
 * 
 * Provides REST endpoints for database schema introspection and self-awareness.
 * Enables external tools and LLM agents to understand database structure.
 * 
 * Endpoints:
 * - GET /api/v1/schema - Complete schema with all tables and relationships
 * - GET /api/v1/schema/tables - List of all table names
 * - GET /api/v1/schema/tables/:name - Detailed schema for specific table
 * - GET /api/v1/capabilities - Database capabilities and features
 * 
 * Features:
 * - Automatic table discovery from RocksDB
 * - Property type detection
 * - Index metadata
 * - Relationship discovery (graph edges)
 * - Cached responses for performance
 * 
 * @see SchemaManager for core implementation
 */
class SchemaApiHandler {
public:
    /**
     * @brief Construct handler with database and schema manager
     * @param storage Database wrapper
     * @param secondary_index Secondary index manager
     * @param schema_mgr Schema manager (must outlive this handler)
     */
    SchemaApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        SchemaManager* schema_mgr
    );

    ~SchemaApiHandler();

    /**
     * @brief Get complete database schema
     * GET /api/v1/schema
     * 
     * Returns all tables, relationships, and metadata in a single response.
     * Suitable for comprehensive schema analysis.
     * 
     * Response: JSON with full schema
     * {
     *   "status": "success",
     *   "metadata": { "version": "1.5.0", "table_count": 5, ... },
     *   "tables": [ ... ],
     *   "relationships": [ ... ]
     * }
     */
    http::response<http::string_body> handleGetSchema(
        const http::request<http::string_body>& req);

    /**
     * @brief Get list of all tables
     * GET /api/v1/schema/tables
     * 
     * Returns lightweight list of table names and basic metadata.
     * Suitable for populating dropdowns or quick overview.
     * 
     * Response: JSON array of table info
     * {
     *   "status": "success",
     *   "tables": [
     *     {"name": "users", "type": "relational", "row_count": 1000},
     *     ...
     *   ]
     * }
     */
    http::response<http::string_body> handleGetTables(
        const http::request<http::string_body>& req);

    /**
     * @brief Get detailed schema for specific table
     * GET /api/v1/schema/tables/:name
     * 
     * Returns complete schema for a single table including:
     * - Property definitions with types
     * - Secondary indexes
     * - Row count estimate
     * 
     * Response: JSON with table schema
     * {
     *   "status": "success",
     *   "table": {
     *     "name": "users",
     *     "type": "relational",
     *     "properties": [ ... ],
     *     "indexes": [ ... ],
     *     "estimated_row_count": 1000
     *   }
     * }
     */
    http::response<http::string_body> handleGetTable(
        const http::request<http::string_body>& req);

    /**
     * @brief Get database capabilities
     * GET /api/v1/capabilities
     * 
     * Returns list of enabled features based on build configuration.
     * Useful for feature detection and compatibility checks.
     * 
     * Response: JSON with capabilities
     * {
     *   "status": "success",
     *   "version": "1.5.0",
     *   "capabilities": [
     *     "graph", "vector_search", "timeseries", ...
     *   ]
     * }
     */
    http::response<http::string_body> handleGetCapabilities(
        const http::request<http::string_body>& req);

    /**
     * @brief Create or update table schema
     * PUT /api/v1/schema/:tablename
     * 
     * Stores custom schema definition for a table.
     * Validates schema structure and persists to database.
     * 
     * Request: JSON schema definition
     * {
     *   "name": "users",
     *   "type": "relational",
     *   "properties": [
     *     {"name": "id", "type": "integer", "indexed": true},
     *     {"name": "name", "type": "string", "nullable": true}
     *   ],
     *   "indexes": [
     *     {"name": "id", "type": "regular", "unique": true, "columns": ["id"]}
     *   ]
     * }
     * 
     * Response: Success or validation error
     */
    http::response<http::string_body> handlePutSchema(
        const http::request<http::string_body>& req);

    /**
     * @brief Partially update table schema
     * PATCH /api/v1/schema/:tablename
     * 
     * Updates specific fields of existing schema without replacing entire schema.
     * Only provided fields are updated, others remain unchanged.
     * 
     * Request: Partial JSON schema
     * {
     *   "properties": [
     *     {"name": "email", "type": "string", "indexed": true}
     *   ]
     * }
     * 
     * Response: Success or validation error
     */
    http::response<http::string_body> handlePatchSchema(
        const http::request<http::string_body>& req);

private:
    /// Extract and validate table name from schema URL
    /// @param target URL target path
    /// @param table_name Output parameter for extracted table name
    /// @return Empty string on success, error message on failure
    std::string extractAndValidateSchemaTableName(
        const std::string& target,
        std::string& table_name) const;

    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    SchemaManager* schema_mgr_;  // Non-owning pointer
};

} // namespace server
} // namespace themis
