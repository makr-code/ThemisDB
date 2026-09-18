/**
 * @file schema_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class SchemaApiHandler {
public:
    SchemaApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        SchemaManager* schema_mgr
    );

    ~SchemaApiHandler();


    /**
     * @brief Set Statistics Collector.
     * @param[in,out] stats_collector Input/output parameter.
     */
    void setStatisticsCollector(StatisticsCollector* stats_collector);

    /**
     * @brief Set Schema Constraints.
     * @param[in,out] schema_constraints Input/output parameter.
     */
    void setSchemaConstraints(SchemaConstraints* schema_constraints);

    /**
     * @brief Set Schema Version Manager.
     * @param[in,out] version_mgr Input/output parameter.
     */
    void setSchemaVersionManager(SchemaVersionManager* version_mgr);

    /**
     * @brief Set Index Recommender.
     * @param[in,out] index_recommender Input/output parameter.
     */
    void setIndexRecommender(metadata::IndexRecommender* index_recommender);

    /**
     * @brief Set Audit Log.
     * @param[in,out] audit_log Input/output parameter.
     */
    void setAuditLog(SchemaAuditLog* audit_log);

    /**
     * @brief Set Column Lineage Tracker.
     * @param[in,out] tracker Input/output parameter.
     */
    void setColumnLineageTracker(themis::metadata::ColumnLineageTracker* tracker);

    // ========================================================================
    // Core schema endpoints
    // ========================================================================

    /**
     * @brief Handle Get Schema.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetSchema(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Tables.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetTables(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Table.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetTable(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Capabilities.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetCapabilities(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Put Schema.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePutSchema(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Patch Schema.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePatchSchema(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Information Schema endpoints
    // ========================================================================

    /**
     * @brief Handle Get Information Schema.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetInformationSchema(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Statistics endpoints
    // ========================================================================

    /**
     * @brief Handle Get Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStats(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Collect Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCollectStats(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Constraints endpoints
    // ========================================================================

    /**
     * @brief Handle Get Constraints.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetConstraints(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Index recommendations endpoint
    // ========================================================================

    /**
     * @brief Handle Get Index Recommendations.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetIndexRecommendations(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Schema audit endpoint
    // ========================================================================

    /**
     * @brief Handle Get Audit Log.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetAuditLog(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Schema import endpoint
    // ========================================================================

    /**
     * @brief Handle Schema Import.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaImport(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Batch constraint validation
    // ========================================================================

    /**
     * @brief Handle Batch Constraint Validation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBatchConstraintValidation(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Schema version endpoints
    // ========================================================================

    /**
     * @brief Handle Get Version History.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetVersionHistory(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Create Version.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateVersion(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Diff.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetDiff(
        const http::request<http::string_body>& req);

    // ========================================================================
    // Column lineage endpoints
    // ========================================================================

    /**
     * @brief Handle Get Column Lineage.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetColumnLineage(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Record Lineage Derivation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRecordLineageDerivation(
        const http::request<http::string_body>& req);

private:
    /**
     * @brief Extract And Validate Schema Table Name.
     * @param[in] target Input parameter.
     * @param[in,out] table_name Name of the table.
     * @return Return value.
     */
    std::string extractAndValidateSchemaTableName(
        const std::string& target,
        std::string& table_name) const;

    /**
     * @brief Extract Table Name.
     * @param[in] target Input parameter.
     * @param[in] prefix Input parameter.
     * @param[in,out] table_name Name of the table.
     * @return Return value.
     */
    std::string extractTableName(
        const std::string& target,
        const std::string& prefix,
        std::string& table_name) const;

    /**
     * @brief Make Error.
     * @param[in] req Input parameter.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
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


