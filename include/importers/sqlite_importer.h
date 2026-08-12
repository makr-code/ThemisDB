/**
 * @file sqlite_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>

namespace themis {
namespace importers {

/**
 * @brief SQLite .dump Importer
 *
 * Imports data from SQLite database dump files produced by the
 * `.dump` command in the sqlite3 CLI.
 *
 * Supports:
 * - DDL parsing (CREATE TABLE, CREATE INDEX, CREATE VIRTUAL TABLE)
 * - DML parsing (INSERT INTO … VALUES)
 * - BEGIN TRANSACTION / COMMIT wrappers (ignored gracefully)
 * - Schema mapping to ThemisDB BaseEntity
 * - Type conversion for SQLite column types (type affinity rules)
 * - Batch processing
 * - Async import via importDataAsync()
 * - Structured error reporting (ImportErrorCode)
 * - Observability: metrics and tracing callbacks
 * - Permission-check callback (ACL enforcement)
 * - Dry-run mode
 * - include/exclude table filtering
 */
class SQLiteImporter : public IImporter {
public:
    SQLiteImporter();
    ~SQLiteImporter() override;

    // IImporter interface
    const char* getName() const override { return "SQLite Importer"; }
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
        std::vector<std::string> columns;
        std::map<std::string, std::string> column_types;
        std::vector<std::string> primary_keys;
    };

    std::atomic<bool> cancelled_{false};
    std::map<std::string, TableSchema> schemas_;

    // Parsing methods
    bool parseDumpFile(const std::string& file_path,
                       const ImportOptions& options,
                       ImportStats& stats,
                       ProgressCallback& callback);
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    bool parseInsert(const std::string& sql,
                     const ImportOptions& options,
                     ImportStats& stats,
                     size_t line_number);

    // Schema mapping
    std::string mapSQLiteTypeToThemis(const std::string& sqlite_type,
                                      const ImportOptions& options) const;
    bool shouldImportTable(const std::string& table_name,
                           const ImportOptions& options) const;

    // Data conversion
    json convertRowToEntity(const TableSchema& schema,
                            const std::vector<std::string>& values);

    // INSERT value parsing
    std::vector<std::string> parseInsertValues(
        const std::string& values_clause) const;

    // Error helpers
    void addError(ImportStats& stats, ImportErrorCode code,
                  ImportErrorSeverity severity,
                  const std::string& message,
                  const std::string& location = "") const;

    // Metrics / tracing helpers
    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;
    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;

    // Progress reporting
    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current, size_t total);
};

/**
 * @brief SQLite Importer Plugin
 *
 * Wraps SQLiteImporter as a ThemisDB plugin.
 */
class SQLiteImporterPlugin : public plugins::IThemisPlugin {
public:
    SQLiteImporterPlugin();
    ~SQLiteImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName() const override { return "sqlite_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override {
        return plugins::PluginType::IMPORTER;
    }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<SQLiteImporter> importer_;
};

} // namespace importers
} // namespace themis
