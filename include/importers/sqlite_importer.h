/**
 * @file sqlite_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>

namespace themis {
namespace importers {

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
    /**
     * @brief Parse Dump File.
     * @param[in] file_path Path to the file.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] callback Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseDumpFile(const std::string& file_path,
                       const ImportOptions& options,
                       ImportStats& stats,
                       ProgressCallback& callback);
    /**
     * @brief Parse Create Table.
     * @param[in] sql Input parameter.
     * @param[in,out] schema Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    /**
     * @brief Parse Insert.
     * @param[in] sql Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in] line_number Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseInsert(const std::string& sql,
                     const ImportOptions& options,
                     ImportStats& stats,
                     size_t line_number);

    // Schema mapping
    /**
     * @brief Map SQLite Type To Themis.
     * @param[in] sqlite_type Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string mapSQLiteTypeToThemis(const std::string& sqlite_type,
                                      const ImportOptions& options) const;
    /**
     * @brief Should Import Table.
     * @param[in] table_name Name of the table.
     * @param[in] options Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldImportTable(const std::string& table_name,
                           const ImportOptions& options) const;

    // Data conversion
    /**
     * @brief Convert Row To Entity.
     * @param[in] schema Input parameter.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    json convertRowToEntity(const TableSchema& schema,
                            const std::vector<std::string>& values);

    // INSERT value parsing
    /**
     * @brief Parse Insert Values.
     * @param[in] values_clause Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Report Progress.
     * @param[in,out] callback Input/output parameter.
     * @param[in] stage Input parameter.
     * @param[in] current Input parameter.
     * @param[in] total Input parameter.
     */
    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current, size_t total);
};

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
