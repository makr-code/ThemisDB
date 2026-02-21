/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analytics_export.h                                 ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:43:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     157                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
    ARROW_IPC,      // Arrow IPC (Inter-Process Communication) format
    ARROW_PARQUET,  // Apache Parquet format
    ARROW_FEATHER,  // Apache Feather format
    CSV,            // Comma-separated values
    JSON            // JSON format
};

/**
 * @brief Export options
 */
struct ExportOptions {
    ExportFormat format = ExportFormat::ARROW_IPC;
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
     * @brief Get default exporter (stub implementation)
     */
    static std::unique_ptr<IAnalyticsExporter> createDefaultExporter();
};

} // namespace analytics
} // namespace themis
