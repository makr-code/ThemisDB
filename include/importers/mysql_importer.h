/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mysql_importer.h                                   ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-14 11:25:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9bccf09a7c  2026-03-16  Changes before error encountered        ║
    • 786e4a8dfe  2026-03-15  feat(importers): incremental import, MySQL benchmark, Mon... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 25e8cec733  2026-02-28  Implement JDBC-compatible config for MySQL/MariaDB importer ║
    • ac1dacf6a6  2026-02-22  Add MySQL/MariaDB importer: header, implementation, tests... ║
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

    /// JDBC-compatible connection parameters stored from initialize().
    struct JdbcConfig {
        std::string host;
        int         port     = 3306;
        std::string database;
        std::string user;
        bool        ssl                 = false;
        bool        tinyint1_as_boolean = false;  ///< Map TINYINT(1) -> boolean (JDBC default)
    };

    std::atomic<bool> cancelled_{false};
    std::map<std::string, TableSchema> schemas_;
    JdbcConfig jdbc_config_;                               ///< Parsed JDBC config from initialize()
    std::map<std::string, std::string> config_type_overrides_; ///< Type overrides from initialize()

    // Parsing methods
    bool parseDumpFile(const std::string& file_path, const ImportOptions& options,
                       ImportStats& stats, ProgressCallback& callback);
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    bool parseInsert(const std::string& sql, const ImportOptions& options,
                     ImportStats& stats, size_t line_number,
                     std::unordered_set<uint64_t>& delta_hashes);

    // Schema mapping
    std::string mapMySQLTypeToThemis(const std::string& mysql_type,
                                     const ImportOptions& options) const;
    bool shouldImportTable(const std::string& table_name, const ImportOptions& options) const;

    // JDBC URL parsing: "jdbc:mysql://host:port/database?param=val&..."
    // Returns true if @p url is a valid JDBC URL; populates @p out on success.
    static bool parseJdbcUrl(const std::string& url, JdbcConfig& out);

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

    // Delta / incremental import helpers (same pattern as PostgreSQL importer).
    // When ImportOptions::delta_hash_file is set, rows whose FNV-1a hash is already
    // in the file are skipped; new hashes are persisted at end of import.
    // Setting delta_key_columns = {"updated_at"} is the recommended high-watermark
    // configuration for MySQL sources.
    static uint64_t computeRowHash(const std::string& tuple_str,
                                   const std::vector<std::string>& values,
                                   const std::vector<std::string>& key_columns,
                                   const std::vector<std::string>& schema_columns);
    static std::unordered_set<uint64_t> loadDeltaHashes(const std::string& delta_hash_file);
    static void saveDeltaHashes(const std::string& delta_hash_file,
                                const std::unordered_set<uint64_t>& hashes);
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

/**
 * @brief URI-scheme plugin registering MySQLImporter with ImporterSchemeRegistry.
 *
 * Handles "mysql://" and "mariadb://" source URIs.  Registered at static-init
 * time via REGISTER_IMPORTER_PLUGIN so that the process-wide
 * IImporterPluginRegistry::instance() can resolve MySQL/MariaDB sources
 * without any manual wiring.
 *
 * The admin import API route POST /api/v1/import/mysql uses this plugin to
 * create a fresh MySQLImporter for each request.
 */
namespace themis {
namespace importers {

class MySQLImporterSchemePlugin : public IImporterPlugin {
public:
    const char* pluginId() const override { return "mysql_plugin"; }

    std::vector<std::string> supportedSchemes() const override {
        return {"mysql", "mariadb"};
    }

    std::unique_ptr<IImporter> createImporter(
            const ImportConfig& config) const override {
        auto imp = std::make_unique<MySQLImporter>();
        imp->initialize(config.json_config.empty() ? "{}" : config.json_config);
        return imp;
    }
};

} // namespace importers
} // namespace themis
