/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mysql_importer.h                                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-23 03:57:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     144                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ac1dacf6a  2026-02-22  Add MySQL/MariaDB importer: header, implementation, tests... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
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
 * @brief MySQL / MariaDB mysqldump Importer
 *
 * Imports data from MySQL/MariaDB mysqldump files (SQL format).
 * Supports:
 * - DDL parsing (CREATE TABLE with backtick-quoted identifiers)
 * - DML parsing (single-row and multi-row INSERT INTO … VALUES)
 * - LOCK TABLES / UNLOCK TABLES blocks
 * - MySQL conditional comments (bang-comments: version-gated directives)
 * - Schema mapping to ThemisDB BaseEntity
 * - Type conversion for 30+ MySQL/MariaDB column types
 * - Batch processing
 * - Async import via importDataAsync()
 * - Structured error reporting (ImportErrorCode)
 * - Observability: metrics and tracing callbacks
 * - Permission-check callback (ACL enforcement)
 */
class MySQLImporter : public IImporter {
public:
    MySQLImporter();
    ~MySQLImporter() override;

    // IImporter interface
    const char* getName() const override { return "MySQL/MariaDB Importer"; }
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
        std::string schema;   ///< Database name (from USE statement or dump header)
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
    std::string mapMySQLTypeToThemis(const std::string& mysql_type,
                                     const ImportOptions& options) const;
    bool shouldImportTable(const std::string& table_name, const ImportOptions& options) const;

    // Data conversion
    json convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values);

    // INSERT value parsing
    std::vector<std::string> parseInsertValues(const std::string& values_clause) const;

    // Identifier unquoting (strips backticks or double-quotes)
    static std::string unquoteIdentifier(const std::string& s);

    // Strip MySQL conditional comments: /*!... */ and /*! ... */
    static std::string stripMySQLComments(const std::string& sql);

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
 * @brief MySQL/MariaDB Importer Plugin
 *
 * Wraps MySQLImporter as a ThemisDB plugin.
 */
class MySQLImporterPlugin : public plugins::IThemisPlugin {
public:
    MySQLImporterPlugin();
    ~MySQLImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName() const override { return "mysql_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override { return plugins::PluginType::IMPORTER; }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<MySQLImporter> importer_;
};

} // namespace importers
} // namespace themis
