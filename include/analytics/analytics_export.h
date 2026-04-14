/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analytics_export.h                                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     162                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8d9f388865  2026-03-16  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 28a4b23b94  2026-02-23  Refactor tests and update error handling ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "arrow_export.h"
#include <string>
#include <memory>
#include <functional>

namespace themis {
namespace analytics {

/**
 * @brief Export result status
 */
enum class ExportStatus {
    SUCCESS,
    FAILED,
    PARTIAL,
    NOT_SUPPORTED
};

/**
 * @brief Export format
 */
enum class ExportFormat {
    FMT_ARROW_IPC,      // Arrow IPC (Inter-Process Communication) format
    FMT_ARROW_PARQUET,  // Apache Parquet format
    FMT_ARROW_FEATHER,  // Apache Feather format
    CSV,            // Comma-separated values
    JSON            // JSON format
};

/**
 * @brief Export options
 */
struct ExportOptions {
    ExportFormat format = ExportFormat::FMT_ARROW_IPC;
    bool compress = false;
    std::string compression_codec = "zstd";  // zstd, gzip, snappy, lz4
    int compression_level = 3;
    size_t batch_size = 10000;  // Rows per batch
    bool include_metadata = true;
};

/**
 * @brief Export result
 */
struct ExportResult {
    ExportStatus status;
    std::string message;
    size_t rows_exported = 0;
    size_t bytes_written = 0;
    double duration_ms = 0.0;
};

/**
 * @brief Interface for Analytics Data Export
 * 
 * This interface defines the contract for exporting analytics data
 * to various formats, including Apache Arrow formats.
 * 
 * Implementations can provide real Arrow integration or other export mechanisms.
 */
class IAnalyticsExporter {
public:
    virtual ~IAnalyticsExporter() = default;

    /**
     * @brief Export a RecordBatch to file
     * @param batch The record batch to export
     * @param output_path Output file path
     * @param options Export options
     * @return Export result
     */
    virtual ExportResult exportToFile(
        const ArrowRecordBatch& batch,
        const std::string& output_path,
        const ExportOptions& options = ExportOptions()) = 0;

    /**
     * @brief Export a RecordBatch to string (for small datasets)
     * @param batch The record batch to export
     * @param options Export options
     * @return Exported data as string
     */
    virtual std::string exportToString(
        const ArrowRecordBatch& batch,
        const ExportOptions& options = ExportOptions()) = 0;

    /**
     * @brief Export with streaming callback
     * @param batch The record batch to export
     * @param callback Function to handle exported chunks
     * @param options Export options
     * @return Export result
     */
    virtual ExportResult exportWithCallback(
        const ArrowRecordBatch& batch,
        std::function<void(const std::vector<uint8_t>&)> callback,
        const ExportOptions& options = ExportOptions()) = 0;

    /**
     * @brief Check if format is supported
     */
    virtual bool supportsFormat(ExportFormat format) const = 0;

    /**
     * @brief Get exporter name/version
     */
    virtual std::string getExporterInfo() const = 0;
};

/**
 * @brief Factory for creating exporters
 */
class ExporterFactory {
public:
    /**
     * @brief Create an exporter for the specified format
     * @param format Export format
     * @return Exporter instance or nullptr if not supported
     */
    static std::unique_ptr<IAnalyticsExporter> createExporter(ExportFormat format);

    /**
     * @brief Get the default JSON/CSV exporter
     *
     * Returns a @c JSONCSVExporter that handles @c ExportFormat::JSON and
     * @c ExportFormat::CSV.  For Arrow-based formats (IPC, Parquet, Feather)
     * use @c createExporter(format) which returns the appropriate concrete
     * type, or throws @c std::runtime_error when the format requires Arrow
     * support that was not compiled in.
     */
    static std::unique_ptr<IAnalyticsExporter> createDefaultExporter();
};

} // namespace analytics
} // namespace themis
