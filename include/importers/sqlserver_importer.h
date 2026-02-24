/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sqlserver_importer.h                               ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23 12:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 IN PROGRESS                                  ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • initial  2026-02-23  Add SQL Server importer (Issue #1845)      ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <unordered_set>

namespace themis {
namespace importers {

/**
 * @brief Microsoft SQL Server T-SQL Dump Importer
 *
 * Imports data from SQL Server Management Studio (SSMS) and sqlcmd export
 * files (T-SQL format). Supports:
 * - DDL parsing (CREATE TABLE with square-bracket-quoted identifiers)
 * - DML parsing (INSERT INTO … VALUES with N-prefixed Unicode string literals)
 * - GO batch separator
 * - Schema mapping to ThemisDB BaseEntity
 * - Type conversion for 30+ T-SQL column types
 * - Batch processing
 * - Async import via importDataAsync()
 * - Structured error reporting (ImportErrorCode)
 * - Observability: metrics and tracing callbacks
 * - Permission-check callback (ACL enforcement)
 */
class SQLServerImporter : public IImporter {
public:
    SQLServerImporter();
    ~SQLServerImporter() override;

    // IImporter interface
    const char* getName() const override { return "SQL Server Importer"; }
    std::vector<std::string> getSupportedTypes() const override;
    bool initialize(const std::string& config) override;
    bool validateSource(const std::string& source_path, std::vector<std::string>& errors) override;
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
        std::string schema;  ///< SQL Server schema name (e.g. "dbo")
        std::vector<std::string> columns;
        std::map<std::string, std::string> column_types;
        std::vector<std::string> primary_keys;
    };

    std::atomic<bool> cancelled_{false};
    std::map<std::string, TableSchema> schemas_;

    // Parsing methods
    bool parseDumpFile(const std::string& file_path, const ImportOptions& options,
                       ImportStats& stats, ProgressCallback& callback);
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    bool parseInsert(const std::string& sql, const ImportOptions& options,
                     ImportStats& stats, size_t line_number);

    // Schema mapping
    std::string mapSQLServerTypeToThemis(const std::string& sqlserver_type,
                                         const ImportOptions& options) const;
    bool shouldImportTable(const std::string& table_name, const ImportOptions& options) const;

    // Data conversion
    json convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values);

    // INSERT value parsing
    std::vector<std::string> parseInsertValues(const std::string& values_clause) const;

    // Identifier unquoting – strips square brackets: [name] -> name
    static std::string unquoteIdentifier(const std::string& s);

    // Error helpers
    void addError(ImportStats& stats, ImportErrorCode code, ImportErrorSeverity severity,
                  const std::string& message, const std::string& location = "") const;

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
    void reportProgress(ProgressCallback& callback, const std::string& stage,
                        size_t current, size_t total);
};

/**
 * @brief SQL Server Importer Plugin
 *
 * Wraps SQLServerImporter as a ThemisDB plugin.
 */
class SQLServerImporterPlugin : public plugins::IThemisPlugin {
public:
    SQLServerImporterPlugin();
    ~SQLServerImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName() const override { return "sqlserver_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override { return plugins::PluginType::IMPORTER; }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<SQLServerImporter> importer_;
};

} // namespace importers
} // namespace themis
