/**
 * @file analytics_export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    /// @brief Move constructor for polymorphic analytics exporter base.
    /// @note Move semantics: abstract base carries no data members; derived classes must delegate here.
    IAnalyticsExporter(IAnalyticsExporter&&) noexcept noexcept = default;

    /// @brief Move assignment operator for polymorphic analytics exporter base.
    /// @note Move semantics: no-op on data-less abstract base; derived classes extend this.
    IAnalyticsExporter& operator=(IAnalyticsExporter&&) noexcept noexcept = default;

    IAnalyticsExporter(const IAnalyticsExporter&) = delete;
    IAnalyticsExporter& operator=(const IAnalyticsExporter&) = delete;

protected:
    IAnalyticsExporter() = default;

public:
    /**
    * @brief Export a RecordBatch to file
    * @param batch The record batch to export
    * @param output_path Output file path
    * @param options Export options
    * @return Export result
    */
    [[nodiscard]] virtual ExportResult exportToFile(
        const ArrowRecordBatch& batch,
        const std::string& output_path,
        const ExportOptions& options = ExportOptions()) = 0;

    /**
     * @brief Export a RecordBatch to string (for small datasets)
     * @param batch The record batch to export
     * @param options Export options
     * @return Exported data as string
     */
    [[nodiscard]] virtual std::string exportToString(
        const ArrowRecordBatch& batch,
        const ExportOptions& options = ExportOptions()) = 0;

    /**
     * @brief Export with streaming callback
     * @param batch The record batch to export
     * @param callback Function to handle exported chunks
     * @param options Export options
     * @return Export result
     */
    [[nodiscard]] virtual ExportResult exportWithCallback(
        const ArrowRecordBatch& batch,
        std::function<void(const std::vector<uint8_t>&)> callback,
        const ExportOptions& options = ExportOptions()) = 0;

    /**
     * @brief Check if format is supported
     */
    [[nodiscard]] virtual bool supportsFormat(ExportFormat format) const = 0;

    /**
     * @brief Get exporter name/version
     */
    [[nodiscard]] virtual std::string getExporterInfo() const = 0;
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
