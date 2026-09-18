/**
 * @file oracle_importer.h
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
#include <unordered_set>

namespace themis {
namespace importers {

/**
 * @brief Oracle Database expdp/SQL Dump Importer
 *
 * Imports data from Oracle Data Pump SQL dump files produced by `expdp`
 * with `SQLFILE=` or from Oracle SQL*Plus spool output.
 *
 * Supports:
 * - DDL parsing (CREATE TABLE with double-quoted identifiers)
 * - DML parsing (INSERT INTO … VALUES – single-row and multi-row)
 * - Oracle-style schema qualifiers ("OWNER"."TABLE")
 * - Oracle hint comment stripping (hints of the form: --+ ... --)
 * - Type mapping for 30+ Oracle built-in column types
 * - Batch processing with configurable chunk size
 * - Async import via importDataAsync()
 * - Structured error reporting (ImportErrorCode)
 * - Observability: metrics and tracing callbacks
 * - Permission-check callback (ACL enforcement)
 */
class OracleImporter : public IImporter {
public:
    OracleImporter();
    ~OracleImporter() override;

    // IImporter interface
    const char* getName() const override { return "Oracle Database Importer"; }
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
        std::string name = {};
        std::string schema;   ///< Owner/schema name (from CREATE TABLE "OWNER"."TABLE")
        std::vector<std::string> columns;
        std::map<std::string, std::string> column_types;
        std::vector<std::string> primary_keys;
    };

    std::atomic<bool> cancelled_{false};
    std::map<std::string, TableSchema> schemas_;

    /**
     * @brief Parsing methods
     * @param[in] file_path Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] callback Input/output parameter.
     * @return True on success.
     */
    bool parseDumpFile(const std::string& file_path, const ImportOptions& options,
                       ImportStats& stats, ProgressCallback& callback);
    /**
     * @brief TBD: Describe parseCreateTable.
     * @param[in] sql Input parameter.
     * @param[in,out] schema Input/output parameter.
     * @return True on success.
     */
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    /**
     * @brief TBD: Describe parseInsert.
     * @param[in] sql Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in] line_number Input parameter.
     * @return True on success.
     */
    bool parseInsert(const std::string& sql, const ImportOptions& options,
                     ImportStats& stats, size_t line_number);

    /**
     * @brief Schema mapping
     * @param[in] oracle_type Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string mapOracleTypeToThemis(const std::string& oracle_type,
                                      const ImportOptions& options) const;
    /**
     * @brief TBD: Describe shouldImportTable.
     * @param[in] table_name Input parameter.
     * @param[in] options Input parameter.
     * @return True on success.
     */
    bool shouldImportTable(const std::string& table_name, const ImportOptions& options) const;

    /**
     * @brief Data conversion
     * @param[in] schema Input parameter.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    json convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values);

    /**
     * @brief INSERT value parsing
     * @param[in] values_clause Input parameter.
     * @return Return value.
     */
    std::vector<std::string> parseInsertValues(const std::string& values_clause) const;

    /**
     * @brief Identifier unquoting (strips double-quotes or returns plain identifiers as-is)
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string unquoteIdentifier(const std::string& s);

    /**
     * @brief Strip Oracle hint comments (/*+ .
     * @param[in] sql Input parameter.
     * @return Return value.
     * @details .. */) and regular block comments (/* ... */)
     */
    static std::string stripOracleComments(const std::string& sql);

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

    /**
     * @brief Progress reporting
     * @param[in,out] callback Input/output parameter.
     * @param[in] stage Input parameter.
     * @param[in] current Input parameter.
     * @param[in] total Input parameter.
     */
    void reportProgress(ProgressCallback& callback, const std::string& stage,
                        size_t current, size_t total);
};

/**
 * @brief Oracle Database Importer Plugin
 *
 * Wraps OracleImporter as a ThemisDB plugin.
 */
class OracleImporterPlugin : public plugins::IThemisPlugin {
public:
    OracleImporterPlugin();
    ~OracleImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName() const override { return "oracle_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override { return plugins::PluginType::IMPORTER; }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<OracleImporter> importer_;
};

} // namespace importers
} // namespace themis
