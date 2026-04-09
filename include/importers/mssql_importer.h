/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mssql_importer.h                                   ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          ThemisDB Contributors                              ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~180                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/importer_interfaces.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <cstdint>
#include <unordered_set>

namespace themis {
namespace importers {

/**
 * @brief Microsoft SQL Server T-SQL Dump Importer
 *
 * Imports data from SQL Server script files produced by SSMS
 * ("Script Database As…"), sqlcmd, or BCP text output.
 *
 * Supported features:
 * - DDL parsing (CREATE TABLE with square-bracket or double-quote identifiers)
 * - DML parsing (single-row and multi-row INSERT INTO … VALUES)
 * - GO batch separator handling
 * - SET IDENTITY_INSERT [table] ON/OFF blocks
 * - N'unicode string' literals
 * - T-SQL line comments (-- …) and block comments (/* … *\/)
 * - Schema-qualified names: [dbo].[TableName] or dbo.TableName
 * - Type mapping for 40+ T-SQL built-in column types
 * - Batch processing with configurable chunk size
 * - Async import via importDataAsync()
 * - Structured error reporting (ImportErrorCode)
 * - Observability: metrics and tracing callbacks
 * - Permission-check callback (ACL enforcement)
 * - Incremental / delta import (FNV-1a row-hash file)
 *
 * Optional live ODBC connectivity is compile-time gated by
 * THEMIS_ENABLE_ODBC; when not enabled every importData() call that
 * receives a connection string (not a file path) returns
 * ImportErrorCode::CONNECTOR_NOT_SUPPORTED.
 */
class MSSQLImporter : public IImporter {
public:
    MSSQLImporter();
    ~MSSQLImporter() override;

    // IImporter interface
    const char* getName() const override { return "SQL Server Importer"; }
    std::vector<std::string> getSupportedTypes() const override;
    bool initialize(const std::string& config) override;
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) override;
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) override;
    void cancel() override;
    json getSourceSchema(const std::string& source_path) override;

private:
    struct TableSchema {
        std::string name;
        std::string schema;   ///< Schema name from [schema].[table]
        std::vector<std::string> columns;
        std::map<std::string, std::string> column_types;
        std::vector<std::string> primary_keys;
        bool identity_insert_active = false;
    };

    std::atomic<bool> cancelled_{false};
    std::map<std::string, TableSchema> schemas_;

    // Parsing methods
    bool parseDumpFile(const std::string& file_path, const ImportOptions& options,
                       ImportStats& stats, ProgressCallback& callback);
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    bool parseInsert(const std::string& sql, const ImportOptions& options,
                     ImportStats& stats, size_t line_number,
                     std::unordered_set<uint64_t>& delta_hashes);

    // Schema mapping
    std::string mapMSSQLTypeToThemis(const std::string& mssql_type,
                                     const ImportOptions& options) const;
    bool shouldImportTable(const std::string& table_name,
                           const ImportOptions& options) const;

    // Identifier unquoting: strips [] or "" delimiters
    static std::string unquoteIdentifier(const std::string& s);

    // Strip T-SQL block comments (/* … */)
    static std::string stripBlockComments(const std::string& sql);

    // Data conversion
    json convertRowToEntity(const TableSchema& schema,
                            const std::vector<std::string>& values);

    // INSERT value parsing: handles N'…', '…', NULL, numeric, function calls
    std::vector<std::string> parseInsertValues(const std::string& values_clause) const;

    // Error helpers
    void addError(ImportStats& stats, ImportErrorCode code, ImportErrorSeverity severity,
                  const std::string& message,
                  const std::string& location = "") const;

    // Metrics emission helper
    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;

    // Distributed tracing / OTel span emission helper
    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;

    // Progress reporting
    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current, size_t total);

    // Delta / incremental import helpers (FNV-1a hash, same pattern as MySQL importer)
    static uint64_t computeRowHash(const std::string& tuple_str,
                                   const std::vector<std::string>& values,
                                   const std::vector<std::string>& key_columns,
                                   const std::vector<std::string>& schema_columns);
    static std::unordered_set<uint64_t> loadDeltaHashes(const std::string& delta_hash_file);
    static void saveDeltaHashes(const std::string& delta_hash_file,
                                const std::unordered_set<uint64_t>& hashes);
};

/**
 * @brief SQL Server Importer Plugin
 *
 * Wraps MSSQLImporter as a ThemisDB plugin.
 */
class MSSQLImporterPlugin : public plugins::IThemisPlugin {
public:
    MSSQLImporterPlugin();
    ~MSSQLImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName() const override { return "mssql_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override { return plugins::PluginType::IMPORTER; }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<MSSQLImporter> importer_;
};

} // namespace importers
} // namespace themis

/**
 * @brief URI-scheme plugin registering MSSQLImporter with ImporterSchemeRegistry.
 *
 * Handles "sqlserver://" and "mssql://" source URIs.  Registered at
 * static-init time via REGISTER_IMPORTER_PLUGIN so that the process-wide
 * IImporterPluginRegistry::instance() can resolve SQL Server sources without
 * any manual wiring.
 *
 * The admin import API route POST /api/v1/import/sqlserver uses this plugin
 * to create a fresh MSSQLImporter for each request.
 */
namespace themis {
namespace importers {

class MSSQLImporterSchemePlugin : public IImporterPlugin {
public:
    const char* pluginId() const override { return "mssql_plugin"; }

    std::vector<std::string> supportedSchemes() const override {
        return {"sqlserver", "mssql"};
    }

    std::unique_ptr<IImporter> createImporter(
            const ImportConfig& config) const override {
        auto imp = std::make_unique<MSSQLImporter>();
        imp->initialize(config.json_config.empty() ? "{}" : config.json_config);
        return imp;
    }
};

} // namespace importers
} // namespace themis
