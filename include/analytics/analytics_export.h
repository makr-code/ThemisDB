/**
 * @file analytics_export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "arrow_export.h"
#include "analytics/analytics_api_contract.h"
#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <memory>
#include <functional>

namespace themis {
namespace analytics {

enum class ExportStatus {
    SUCCESS,
    FAILED,
    PARTIAL,
    NOT_SUPPORTED,
    POLICY_REJECTED ///< Export rejected because a BoundedExecutionPolicy limit was exceeded
};

enum class ExportFormat {
    FMT_ARROW_IPC,      // Arrow IPC (Inter-Process Communication) format
    FMT_ARROW_PARQUET,  // Apache Parquet format
    FMT_ARROW_FEATHER,  // Apache Feather format
    CSV,            // Comma-separated values
    JSON            // JSON format
};

struct ExportOptions {
    ExportFormat format = ExportFormat::FMT_ARROW_IPC;
    bool compress = false;
    std::string compression_codec = "zstd";  // zstd, gzip, snappy, lz4
    int compression_level = 3;
    size_t batch_size = 10000;  // Rows per batch
    bool include_metadata = true;

    BoundedExecutionPolicy policy;
};

struct ExportResult {
    ExportStatus status;
    std::string message;
    size_t rows_exported = 0;
    size_t bytes_written = 0;
    double duration_ms = 0.0;
    std::string operation_id;
    std::string correlation_id;
    std::string failure_class = "none";
    std::vector<std::string> operator_hints;
};

class IAnalyticsExporter {
public:
    /**
     * @brief IAnalytics Exporter.
     * @return Return value.
     */
    virtual ~IAnalyticsExporter() = default;

    IAnalyticsExporter(IAnalyticsExporter&&) noexcept
        : inflight_export_count_(0u) {}

    IAnalyticsExporter& operator=(IAnalyticsExporter&&) noexcept {
        inflight_export_count_.store(0u, std::memory_order_relaxed);
        return *this;
    }

    IAnalyticsExporter(const IAnalyticsExporter&) = delete;
    IAnalyticsExporter& operator=(const IAnalyticsExporter&) = delete;

protected:
    IAnalyticsExporter() = default;

public:
    [[nodiscard]] virtual ExportResult exportToFile(
        const ArrowRecordBatch& batch,
        const std::string& output_path,
        const ExportOptions& options = ExportOptions()) = 0;

    [[nodiscard]] ExportResult exportToFile(
        const ArrowRecordBatch&              batch,
        const std::string&                   output_path,
        const ExportOptions&                 options,
        const BoundedExecutionPolicy&        policy);

private:
    mutable std::atomic<uint32_t> inflight_export_count_{0u};

public:

    [[nodiscard]] virtual std::string exportToString(
        const ArrowRecordBatch& batch,
        const ExportOptions& options = ExportOptions()) = 0;

    [[nodiscard]] virtual ExportResult exportWithCallback(
        const ArrowRecordBatch& batch,
        std::function<void(const std::vector<uint8_t>&)> callback,
        const ExportOptions& options = ExportOptions()) = 0;

    [[nodiscard]] virtual bool supportsFormat(ExportFormat format) const = 0;

    [[nodiscard]] virtual std::string getExporterInfo() const = 0;
};

class ExporterFactory {
public:
    /**
     * @brief Create Exporter.
     * @param[in] format Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<IAnalyticsExporter> createExporter(ExportFormat format);

    /**
     * @brief Create Default Exporter.
     * @return Return value.
     */
    static std::unique_ptr<IAnalyticsExporter> createDefaultExporter();
};

} // namespace analytics
} // namespace themis
