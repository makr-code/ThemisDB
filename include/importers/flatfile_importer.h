/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            flatfile_importer.h                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-27                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>

namespace themis {
namespace importers {

/**
 * @brief Flat-file source format
 */
enum class FlatFileFormat {
    AUTO,   ///< Detect from file extension
    CSV,    ///< Comma-separated values (configurable delimiter)
    TSV,    ///< Tab-separated values
    JSONL   ///< JSON Lines (one JSON object per line)
};

/**
 * @brief Flat-File Importer
 *
 * Imports data from CSV, TSV, and JSON Lines (JSONL) files into ThemisDB.
 *
 * Supports:
 * - CSV / TSV with configurable delimiter, quote character, and header row
 * - JSONL (one JSON object per line)
 * - Format auto-detection from file extension
 * - Schema mapping to ThemisDB BaseEntity via column/table mappings
 * - Dry-run mode (validate without writing)
 * - Include / exclude table filtering
 * - Streaming row callback for large files
 * - Async import via importDataAsync()
 * - Metrics and distributed-tracing callbacks (Prometheus / OTel)
 * - Permission-check callback (ACL enforcement)
 * - Row-size limits and UTF-8 validation
 * - Structured error reporting (ImportErrorCode)
 *
 * Configuration (passed as JSON string to initialize()):
 * @code
 *   {
 *     "format":     "csv",     // "auto" (default), "csv", "tsv", "jsonl"
 *     "delimiter":  ",",       // single character; overrides format default
 *     "quote_char": "\"",      // single character (default: double-quote)
 *     "has_header": true,      // first row contains column names (CSV/TSV)
 *     "table_name": "data"     // logical table name (default: stem of filename)
 *   }
 * @endcode
 */
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

    // ---- Format helpers -------------------------------------------------------

    /// Detect format from file extension; returns AUTO if unknown.
    static FlatFileFormat detectFormat(const std::string& path);

    /// Resolve effective format: uses configured format_ or auto-detects.
    FlatFileFormat effectiveFormat(const std::string& path) const;

    /// Extract filename stem (basename without extension) for table name.
    static std::string filenameStem(const std::string& path);

    // ---- CSV / TSV parsing ---------------------------------------------------

    /**
     * @brief Parse a single CSV/TSV row into fields.
     *
     * Handles quoted fields (RFC 4180), embedded newlines inside quotes are
     * NOT supported (single-line records only).
     *
     * @param line      Input line (without the trailing newline).
     * @param delim     Field delimiter character.
     * @param quote     Quote character.
     * @return          Vector of field strings (un-quoted, unescaped).
     */
    static std::vector<std::string> parseCsvRow(const std::string& line,
                                                 char delim,
                                                 char quote);

    /**
     * @brief Import CSV or TSV file.
     *
     * @param path      File path.
     * @param fmt       Resolved format (CSV or TSV).
     * @param table     Logical table name.
     * @param options   Import options.
     * @param stats     Output statistics (updated in-place).
     * @param cb        Progress callback (may be null).
     * @return false on fatal I/O error; true otherwise.
     */
    bool importCsvFile(const std::string& path,
                       FlatFileFormat fmt,
                       const std::string& table,
                       const ImportOptions& options,
                       ImportStats& stats,
                       ProgressCallback& cb);

    // ---- JSONL parsing -------------------------------------------------------

    /**
     * @brief Import a JSONL file (one JSON object per line).
     *
     * @param path      File path.
     * @param table     Logical table name.
     * @param options   Import options.
     * @param stats     Output statistics (updated in-place).
     * @param cb        Progress callback (may be null).
     * @return false on fatal I/O error; true otherwise.
     */
    bool importJsonlFile(const std::string& path,
                         const std::string& table,
                         const ImportOptions& options,
                         ImportStats& stats,
                         ProgressCallback& cb);

    // ---- Utility helpers -----------------------------------------------------

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

    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current, size_t total);

    static bool isValidUtf8(const std::string& s);
};

/**
 * @brief FlatFile Importer Plugin
 *
 * Wraps FlatFileImporter as a ThemisDB plugin.
 */
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
