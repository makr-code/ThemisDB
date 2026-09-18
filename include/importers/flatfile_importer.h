/**
 * @file flatfile_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "importers/schema_validator.h"
#include "plugins/plugin_interface.h"
#include <atomic>

namespace themis {
namespace importers {

enum class FlatFileFormat {
    AUTO,    ///< Detect from file extension
    CSV,     ///< Comma-separated values (configurable delimiter)
    TSV,     ///< Tab-separated values
    JSONL,   ///< JSON Lines (one JSON object per line)
    PARQUET  ///< Apache Parquet columnar format (requires ARROW_ENABLED)
};

class FlatFileImporter : public IImporter {
public:
    FlatFileImporter();
    ~FlatFileImporter() override;

    // IImporter interface
    const char* getName() const override { return "FlatFile Importer"; }
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
    // ---- Configuration -------------------------------------------------------
    FlatFileFormat format_   = FlatFileFormat::AUTO;
    char           delimiter_ = ',';
    char           quote_char_ = '"';
    bool           has_header_ = true;
    std::string    table_name_;   ///< Explicit table name (empty = use filename stem)

    std::atomic<bool> cancelled_{false};
    
    // Phase 2A Data Race Protection: Mutex guards for concurrent access
    mutable std::mutex column_options_mutex_;     ///< Protects column_options_map_ concurrent access
    mutable std::mutex validator_state_mutex_;    ///< Protects field_validator_state_ concurrent access
    mutable std::mutex schema_cache_mutex_;       ///< Protects schema_inference_cache_ concurrent access
    
    std::map<std::string, std::map<std::string, std::string>> column_options_map_;  ///< Column validation options (Phase 2A)
    std::map<std::string, std::string> field_validator_state_;  ///< Per-field validation state (Phase 2A)
    std::map<std::string, std::string> schema_inference_cache_;  ///< Schema type hints cache (Phase 2A)

    /**
     * @brief ---- Format helpers -------------------------------------------------------
     * @param[in] path Input parameter.
     * @return Return value.
     */

    static FlatFileFormat detectFormat(const std::string& path);

    /**
     * @brief Effective Format.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    FlatFileFormat effectiveFormat(const std::string& path) const;

    /**
     * @brief Filename Stem.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string filenameStem(const std::string& path);

    /**
     * @brief ---- CSV / TSV parsing ---------------------------------------------------
     * @param[in] line Input parameter.
     * @param[in] delim Input parameter.
     * @param[in] quote Input parameter.
     * @return Return value.
     */

    static std::vector<std::string> parseCsvRow(const std::string& line,
                                                 char delim,
                                                 char quote);

    /**
     * @brief Import Csv File.
     * @param[in] path Input parameter.
     * @param[in] fmt Input parameter.
     * @param[in] table Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] cb Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool importCsvFile(const std::string& path,
                       FlatFileFormat fmt,
                       const std::string& table,
                       const ImportOptions& options,
                       ImportStats& stats,
                       ProgressCallback& cb);

    /**
     * @brief Detect Csv Schema.
     * @param[in,out] file Input/output parameter.
     * @param[in] data_start_pos Input parameter.
     * @param[in] columns Input parameter.
     * @param[in] delim Input parameter.
     * @param[in] line_limit Input parameter.
     * @param[in] sample_limit Input parameter.
     * @param[in] table Input parameter.
     * @return Return value.
     */
    DetectedSchema detectCsvSchema(std::ifstream& file,
                                   std::streampos data_start_pos,
                                   const std::vector<std::string>& columns,
                                   char delim,
                                   size_t line_limit,
                                   size_t sample_limit,
                                   const std::string& table);

    /**
     * @brief ---- JSONL parsing -------------------------------------------------------
     * @param[in] path Input parameter.
     * @param[in] table Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] cb Input/output parameter.
     * @return True when the operation succeeds.
     */

    bool importJsonlFile(const std::string& path,
                         const std::string& table,
                         const ImportOptions& options,
                         ImportStats& stats,
                         ProgressCallback& cb);

    /**
     * @brief Import Parquet File.
     * @param[in] path Input parameter.
     * @param[in] table Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] cb Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool importParquetFile(const std::string& path,
                           const std::string& table,
                           const ImportOptions& options,
                           ImportStats& stats,
                           ProgressCallback& cb);

    /**
     * @brief ---- Utility helpers -----------------------------------------------------
     * @param[in] table_name Name of the table.
     * @param[in] options Input parameter.
     * @return True when the operation succeeds.
     */

    bool shouldImportTable(const std::string& table_name,
                           const ImportOptions& options) const;

    void addError(ImportStats& stats, ImportErrorCode code,
                  ImportErrorSeverity severity,
                  const std::string& message,
                  const std::string& location = "") const;

    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;

    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;

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

    /**
     * @brief Is Valid Utf8.
     * @param[in] s Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isValidUtf8(const std::string& s);
};

class FlatFileImporterPlugin : public plugins::IThemisPlugin {
public:
    FlatFileImporterPlugin();
    ~FlatFileImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName() const override { return "flatfile_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override {
        return plugins::PluginType::IMPORTER;
    }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<FlatFileImporter> importer_;
};

} // namespace importers
} // namespace themis
