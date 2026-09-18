/**
 * @file arrow_ipc_exporter.h
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

enum class ArrowIPCFormat {
    FILE,

    STREAM,
};

struct ArrowIPCExportConfig {
    ArrowIPCFormat format = ArrowIPCFormat::FILE;

    bool auto_detect_schema = true;

    std::vector<std::string> include_columns;

    std::vector<std::string> exclude_columns;

    std::map<std::string, std::string> schema_metadata;

    bool track_nulls = false;
};

class ArrowIPCExporter : public IExporter {
public:
    explicit ArrowIPCExporter(const ArrowIPCExportConfig& config = {});

    // IExporter interface
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"arrow", "arrows", "arrow_ipc"};
    }
    std::string getName() const override { return "arrow_ipc_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const ArrowIPCExportConfig& config) { config_ = config; }
    const ArrowIPCExportConfig& getConfig() const { return config_; }

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
    ArrowIPCExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /**
     * @brief Resolve Columns.
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
