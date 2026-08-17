/**
 * @file catalog_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "metadata/schema_manager.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>

namespace themis {

/// CatalogExporter - Integration with External Data Catalogs
///
/// Publishes ThemisDB schema metadata to external data governance and
/// catalog systems: Apache Atlas and DataHub.
///
/// Supported targets:
/// - Apache Atlas (v2 REST API): entity creation via /api/atlas/v2/entity/bulk
/// - DataHub GMS (REST API):     metadata ingestion via /aspects?action=ingestProposal
///
/// Architecture:
///   CatalogExporter
///   ├─→ SchemaManager::TableSchema (input)
///   ├─→ Apache Atlas entity serializer (buildAtlasEntities)
///   └─→ DataHub MetadataChangeProposal serializer (buildDataHubProposals)
///
/// Thread-Safety:
/// - publishSchema() and publishTable() are thread-safe when called with
///   different Config instances; do not share a single CatalogExporter
///   instance across threads without external synchronization.
///
/// Usage:
///   CatalogExporter::Config cfg;
///   cfg.type     = CatalogExporter::CatalogType::APACHE_ATLAS;
///   cfg.endpoint = "http://atlas-host:21000";
///   cfg.username = "admin";
///   cfg.password = "admin";
///
///   CatalogExporter exporter(cfg);
///   auto result = exporter.publishSchema(schema_mgr.getAllTables());
///   if (!result.success) { /* handle error */ }
///
/// Issue: makr-code/ThemisDB#2414
class CatalogExporter {
public:
    /// Catalog target type
    enum class CatalogType {
        APACHE_ATLAS,  ///< Apache Atlas v2 REST API
        DATAHUB        ///< DataHub GMS REST API
    };

    /// Configuration for the catalog connection
    struct Config {
        CatalogType type = CatalogType::APACHE_ATLAS;

        /// Base URL of the catalog service
        /// Atlas example:   "http://atlas-host:21000"
        /// DataHub example: "http://datahub-gms:8080"
        std::string endpoint;

        /// Basic-auth username (Apache Atlas only)
        std::string username;

        /// Basic-auth password (Apache Atlas only)
        std::string password;

        /// Bearer token (DataHub only)
        std::string token;

        /// Logical database / instance name used as the top-level catalog entity
        std::string database_name = "ThemisDB";

        /// HTTP request timeout in milliseconds
        int timeout_ms = 10000;
    };

    /// Result returned by publish operations
    struct PublishResult {
        bool        success          = false;
        int         entities_published = 0;
        std::string error;
    };

    // =========================================================================
    // HTTP injection (for unit testing without network access)
    // =========================================================================

    /// Signature of the injectable HTTP POST function.
    /// Returns the HTTP status code; writes the response body to @p response_body.
    using HttpPostFn = std::function<int(
        const std::string& url,
        const std::string& body,
        const std::string& auth_header,
        std::string&       response_body
    )>;

    // =========================================================================
    // Construction
    // =========================================================================

    /// @param config  Catalog connection parameters
    explicit CatalogExporter(Config config);

    ~CatalogExporter() = default;

    // Non-copyable; movable
    CatalogExporter(const CatalogExporter&) = delete;
    CatalogExporter& operator=(const CatalogExporter&) = delete;
    CatalogExporter(CatalogExporter&&) noexcept = default;
    CatalogExporter& operator=(CatalogExporter&&) noexcept = default;

    // =========================================================================
    // Public API
    // =========================================================================

    /// Publish all tables to the configured catalog.
    ///
    /// For Apache Atlas: creates/updates `rdbms_db` + `rdbms_table` + `rdbms_column`
    /// entities in a single bulk call.
    ///
    /// For DataHub: emits one `MetadataChangeProposal` per table with
    /// `datasetProperties` and `schemaMetadata` aspects.
    ///
    /// @param tables  Tables to export (typically from SchemaManager::getAllTables())
    /// @return        PublishResult with success flag and entity count
    PublishResult publishSchema(const std::vector<SchemaManager::TableSchema>& tables);

    /// Publish a single table to the configured catalog.
    /// @param table  Table schema to export
    /// @return       PublishResult with success flag and entity count
    PublishResult publishTable(const SchemaManager::TableSchema& table);

    /// Replace the real libcurl implementation with a test double.
    /// Pass an empty HttpPostFn{} to restore the real implementation.
    void setHttpPostForTesting(HttpPostFn fn);

private:
    // =========================================================================
    // Apache Atlas helpers
    // =========================================================================

    /// Build the Atlas bulk-entity JSON payload for a list of tables.
    json buildAtlasPayload(const std::vector<SchemaManager::TableSchema>& tables) const;

    /// Publish entity payload to Atlas; returns the publish result.
    PublishResult sendToAtlas(const json& payload);

    // =========================================================================
    // DataHub helpers
    // =========================================================================

    /// Build an array of MetadataChangeProposal JSON objects (one per table).
    json buildDataHubProposals(const std::vector<SchemaManager::TableSchema>& tables) const;

    /// Post each proposal to the DataHub GMS ingest endpoint.
    PublishResult sendToDataHub(const json& proposals);

    // =========================================================================
    // HTTP helper
    // =========================================================================

    /// Execute an HTTP POST request.
    /// Delegates to the injected test function when set; otherwise uses libcurl.
    /// @returns HTTP status code (0 on transport error)
    int httpPost(const std::string& url,
                 const std::string& body,
                 const std::string& auth_header,
                 std::string&       response_body);

    // =========================================================================
    // Members
    // =========================================================================

    Config     config_;
    HttpPostFn http_post_fn_;   ///< Optional test double; empty = use libcurl
};

} // namespace themis
