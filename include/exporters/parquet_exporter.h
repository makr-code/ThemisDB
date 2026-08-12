/**
 * @file parquet_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "exporter_interface.h"
#include "exporter_metrics.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace exporters {

/// Column data type hint for schema inference
enum class ParquetColumnType {
    AUTO,     ///< Auto-detect from entity field values
    INT64,    ///< 64-bit signed integer
    DOUBLE,   ///< 64-bit IEEE 754 floating point
    STRING,   ///< UTF-8 string (BYTE_ARRAY in Parquet)
    BOOLEAN,  ///< Boolean
};

/// Per-column schema hint
struct ParquetColumnHint {
    std::string name;
    ParquetColumnType type = ParquetColumnType::AUTO;
    bool nullable = true;
};

/// Configuration for Parquet export
struct ParquetExportConfig {
    /// Number of rows per row group (default: 65536 as recommended by Parquet spec)
    size_t row_group_size = 65536;

    /// Compression codec: "none", "snappy", "gzip", "zstd"
    /// Note: snappy/gzip/zstd require the corresponding library to be linked.
    /// Falls back to "none" if the requested codec is not available.
    std::string compression = "snappy";

    /// When true, schema is inferred from the first batch of entities.
    /// When false, schema is derived solely from column_hints.
    bool auto_detect_schema = true;

    /// Optional per-column type hints; any unlisted field is AUTO-detected
    std::vector<ParquetColumnHint> column_hints;

    /// Columns to include (empty = include all fields from entities)
    std::vector<std::string> include_columns;

    /// Columns to always exclude
    std::vector<std::string> exclude_columns;

    /// Metadata key/value pairs written to the Parquet file footer
    std::map<std::string, std::string> file_metadata;

    /// PII detection and redaction (mirrors JSONLLLMConfig::PIIConfig)
    struct PIIConfig {
        bool enable_detection = false;
        bool enable_redaction = false;
        bool detect_email = true;
        bool detect_phone = true;
        bool detect_ssn = true;
        bool detect_credit_card = true;

        /// "mask", "hash", "remove", "partial"
        std::string redaction_strategy = "mask";

        /// Fields to inspect (empty = all string columns)
        std::vector<std::string> check_fields;

        /// Abort the entire export if PII is found and redaction is disabled
        bool fail_on_pii = false;
    } pii_config;
};

/// Parquet exporter for training datasets
///
/// Implements IExporter and writes entities to columnar Parquet files.
///
/// When the library is compiled with ARROW_ENABLED (Apache Arrow C++ available),
/// actual Parquet files are produced using arrow::RecordBatchWriter.
///
/// When ARROW_ENABLED is not defined, a minimal standards-conformant Parquet
/// binary file is written using the internal MinimalParquetWriter — no external
/// libraries required.  Both paths produce files readable by pyarrow / Pandas.
class ParquetExporter : public IExporter {
public:
    explicit ParquetExporter(const ParquetExportConfig& config = {});

    // IExporter interface
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"parquet"};
    }
    std::string getName() const override { return "parquet_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /// Replace the current configuration
    void setConfig(const ParquetExportConfig& config) { config_ = config; }
    const ParquetExportConfig& getConfig() const { return config_; }

    /// Access live metrics (may be polled at any time)
    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

    /// Reset all collected metrics
    void resetMetrics() { if (metrics_) metrics_->reset(); }

    /// Returns true if the Apache Arrow C++ library was compiled in
    static bool isArrowAvailable();

private:
    ParquetExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    // Decide which include/exclude columns apply for this export
    std::vector<std::string> resolveColumns(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) const;

#ifdef ARROW_ENABLED
    ExportStats exportWithArrow(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options,
        const std::vector<std::string>& columns
    );
#endif

    ExportStats exportFallback(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options,
        const std::vector<std::string>& columns
    );
};

} // namespace exporters
} // namespace themis
