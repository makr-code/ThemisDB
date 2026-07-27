/**
 * @file schema_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: schema_api_handler.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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
class StatisticsCollector;
class SchemaConstraints;
class SchemaVersionManager;
class SchemaAuditLog;

namespace metadata {
class IndexRecommender;
class ColumnLineageTracker;
} // namespace metadata

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Schema API Handler for HTTP Server
 *
 * Provides REST endpoints for database schema introspection and management.
 * Enables external tools and LLM agents to understand database structure.
 *
 * Endpoints:
 * - GET /api/v1/schema                       – Complete schema with all tables
 * - GET /api/v1/schema/tables                – List of all table names
 * - GET /api/v1/schema/tables/:name          – Detailed schema for specific table
 * - GET /api/v1/capabilities                 – Database capabilities
 * - PUT /api/v1/schema/:name                 – Create/replace table schema
 * - PATCH /api/v1/schema/:name               – Partial update of table schema
 *
 * Information Schema endpoints (require InformationSchema to be set):
 * - GET /api/v1/information_schema/tables    – INFORMATION_SCHEMA.TABLES view
 * - GET /api/v1/information_schema/columns   – INFORMATION_SCHEMA.COLUMNS view
 * - GET /api/v1/information_schema/columns/:table – Columns for one table
 * - GET /api/v1/information_schema/statistics – INFORMATION_SCHEMA.STATISTICS
 * - GET /api/v1/information_schema           – All views as one JSON object
 *
 * Statistics endpoints (require StatisticsCollector to be set):
 * - GET /api/v1/metadata/stats/:table        – Statistics for a table
 * - POST /api/v1/metadata/stats/:table       – Trigger stats collection for a table
 *
 * Constraints endpoints (require SchemaConstraints to be set):
 * - GET /api/v1/metadata/constraints/:table  – Constraints for a table
 *
 * Schema version endpoints (require SchemaVersionManager to be set):
 * - GET /api/v1/schema/versions/:table       – Version history for a table
 * - POST /api/v1/schema/versions/:table      – Snapshot current schema as new version
 * - GET /api/v1/schema/diff/:table?from=V&to=V – Diff between two versions
 *
 * Column lineage endpoints (require ColumnLineageTracker to be set):
 * - GET  /api/v1/metadata/lineage/:table             – All lineage entries for a table
 * - GET  /api/v1/metadata/lineage/:table/:column     – Provenance for one column
 * - POST /api/v1/metadata/lineage                    – Record a derivation entry
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

    // ========================================================================
    // Optional component injection (called after construction)
    // ========================================================================

    /// Attach a StatisticsCollector to enable /api/v1/metadata/stats/* endpoints.
    void setStatisticsCollector(StatisticsCollector* stats_collector);

    /// Attach a SchemaConstraints instance to enable /api/v1/metadata/constraints/* endpoints.
    void setSchemaConstraints(SchemaConstraints* schema_constraints);

    /// Attach a SchemaVersionManager to enable /api/v1/schema/versions/* endpoints.
    void setSchemaVersionManager(SchemaVersionManager* version_mgr);

    /// Attach an IndexRecommender to enable /api/v1/metadata/index_recommendations/* endpoints.
    void setIndexRecommender(metadata::IndexRecommender* index_recommender);

    /// Attach a SchemaAuditLog to enable /api/v1/metadata/audit/* endpoints.
    void setAuditLog(SchemaAuditLog* audit_log);

    /// Attach a ColumnLineageTracker to enable /api/v1/metadata/lineage/* endpoints.
    void setColumnLineageTracker(themis::metadata::ColumnLineageTracker* tracker);

    // ========================================================================
    // Core schema endpoints
    // ========================================================================

    /// GET /api/v1/schema
    http::response<http::string_body> handleGetSchema(
        const http::request<http::string_body>& req);

    /// GET /api/v1/schema/tables
    http::response<http::string_body> handleGetTables(
        const http::request<http::string_body>& req);

    /// GET /api/v1/schema/tables/:name
    http::response<http::string_body> handleGetTable(
        const http::request<http::string_body>& req);

    /// GET /api/v1/capabilities
    http::response<http::string_body> handleGetCapabilities(
        const http::request<http::string_body>& req);

    /// PUT /api/v1/schema/:name
    http::response<http::string_body> handlePutSchema(
        const http::request<http::string_body>& req);

    /// PATCH /api/v1/schema/:name
    http::response<http::string_body> handlePatchSchema(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Information Schema endpoints
    // ========================================================================

    /// GET /api/v1/information_schema  (full dump)
    /// GET /api/v1/information_schema/tables
    /// GET /api/v1/information_schema/columns[/:table]
    /// GET /api/v1/information_schema/statistics[/:table]
    http::response<http::string_body> handleGetInformationSchema(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Statistics endpoints
    // ========================================================================

    /// GET  /api/v1/metadata/stats/:table  – return cached statistics
    /// POST /api/v1/metadata/stats/:table  – trigger collection
    http::response<http::string_body> handleGetStats(
        const http::request<http::string_body>& req);

    http::response<http::string_body> handleCollectStats(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Constraints endpoints
    // ========================================================================

    /// GET /api/v1/metadata/constraints/:table
    http::response<http::string_body> handleGetConstraints(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Index recommendations endpoint
    // ========================================================================

    /// GET /api/v1/metadata/index_recommendations         – all tables
    /// GET /api/v1/metadata/index_recommendations/:table  – single table
    http::response<http::string_body> handleGetIndexRecommendations(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Schema audit endpoint
    // ========================================================================

    /// GET /api/v1/metadata/audit              – full audit history
    /// GET /api/v1/metadata/audit/:table       – per-table audit history
    http::response<http::string_body> handleGetAuditLog(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Schema import endpoint
    // ========================================================================

    /// PUT /api/v1/metadata/schema_import
    /// Bulk-import multiple table schemas from a JSON array.
    /// Body: { "tables": [ <TableSchema JSON>, … ] }
    http::response<http::string_body> handleSchemaImport(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Batch constraint validation
    // ========================================================================

    /// POST /api/v1/metadata/constraints/validate/:table
    /// Validate a batch of rows against the table's registered constraints.
    /// Body: { "rows": [ { <column>: <value>, … }, … ] }
    http::response<http::string_body> handleBatchConstraintValidation(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Schema version endpoints
    // ========================================================================

    /// GET  /api/v1/schema/versions/:table
    http::response<http::string_body> handleGetVersionHistory(
        const http::request<http::string_body>& req);

    /// POST /api/v1/schema/versions/:table
    http::response<http::string_body> handleCreateVersion(
        const http::request<http::string_body>& req);

    /// GET /api/v1/schema/diff/:table?from=V&to=V
    http::response<http::string_body> handleGetDiff(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Column lineage endpoints
    // ========================================================================

    /// GET  /api/v1/metadata/lineage/:table           – export all lineage for a table
    /// GET  /api/v1/metadata/lineage/:table/:column   – provenance for one column
    http::response<http::string_body> handleGetColumnLineage(
        const http::request<http::string_body>& req);

    /// POST /api/v1/metadata/lineage – record a derivation entry
    /// Body: ColumnLineageEntry JSON object
    http::response<http::string_body> handleRecordLineageDerivation(
        const http::request<http::string_body>& req);

private:
    /// Extract and validate table name from schema URL
    std::string extractAndValidateSchemaTableName(
        const std::string& target,
        std::string& table_name) const;

    /// Extract table name from a path with given prefix
    /// e.g. prefix="/api/v1/metadata/stats/", target="/api/v1/metadata/stats/users" → "users"
    std::string extractTableName(
        const std::string& target,
        const std::string& prefix,
        std::string& table_name) const;

    /// Build a standard error response
    http::response<http::string_body> makeError(
        const http::request<http::string_body>& req,
        http::status status,
        const std::string& message) const;

    std::shared_ptr<RocksDBWrapper>        storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    SchemaManager*          schema_mgr_          = nullptr;  ///< Non-owning
    StatisticsCollector*    stats_collector_     = nullptr;  ///< Non-owning
    SchemaConstraints*      schema_constraints_  = nullptr;  ///< Non-owning
    SchemaVersionManager*   version_mgr_         = nullptr;  ///< Non-owning
    metadata::IndexRecommender*       index_recommender_   = nullptr;  ///< Non-owning
    SchemaAuditLog*         audit_log_           = nullptr;  ///< Non-owning
    themis::metadata::ColumnLineageTracker* column_lineage_tracker_ = nullptr;  ///< Non-owning
};

} // namespace server
} // namespace themis


