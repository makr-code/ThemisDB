/**
 * @file parquet_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class ParquetColumnType {
    AUTO,     ///< Auto-detect from entity field values
    INT64,    ///< 64-bit signed integer
    DOUBLE,   ///< 64-bit IEEE 754 floating point
    STRING,   ///< UTF-8 string (BYTE_ARRAY in Parquet)
    BOOLEAN,  ///< Boolean
};

struct ParquetColumnHint {
    std::string name;
    ParquetColumnType type = ParquetColumnType::AUTO;
    bool nullable = true;
};

struct ParquetExportConfig {
    size_t row_group_size = 65536;

    std::string compression = "snappy";

    bool auto_detect_schema = true;

    std::vector<ParquetColumnHint> column_hints;

    std::vector<std::string> include_columns;

    std::vector<std::string> exclude_columns;

    std::map<std::string, std::string> file_metadata;

    struct PIIConfig {
        bool enable_detection = false;
        bool enable_redaction = false;
        bool detect_email = true;
        bool detect_phone = true;
        bool detect_ssn = true;
        bool detect_credit_card = true;

        std::string redaction_strategy = "mask";

        std::vector<std::string> check_fields;

        bool fail_on_pii = false;
    } pii_config;
};

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

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const ParquetExportConfig& config) { config_ = config; }
    const ParquetExportConfig& getConfig() const { return config_; }

    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

    /**
     * @brief Reset Metrics.
     * @details Calls: reset().
     */
    void resetMetrics() { if (metrics_) metrics_->reset(); }

    /**
     * @brief Is Arrow Available.
     * @return True when the operation succeeds.
     */
    static bool isArrowAvailable();

private:
    ParquetExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /**
     * @brief Decide which include/exclude columns apply for this export
     * @param[in] entities Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<std::string> resolveColumns(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) const;

#ifdef ARROW_ENABLED
    /**
     * @brief Export With Arrow.
     * @param[in] entities Input parameter.
     * @param[in] options Input parameter.
     * @param[in] columns Input parameter.
     * @return Return value.
     */
    ExportStats exportWithArrow(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options,
        const std::vector<std::string>& columns
    );
#endif

    /**
     * @brief Export Fallback.
     * @param[in] entities Input parameter.
     * @param[in] options Input parameter.
     * @param[in] columns Input parameter.
     * @return Return value.
     */
    ExportStats exportFallback(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options,
        const std::vector<std::string>& columns
    );
};

} // namespace exporters
} // namespace themis
