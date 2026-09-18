/**
 * @file incremental_exporter.h
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
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace themis::exporters {

struct IncrementalExportConfig {
    std::string sequence_field = "_seq";

    std::string watermark_path;

    bool export_missing_sequence = true;
};

class IncrementalExporter : public IExporter {
public:
    explicit IncrementalExporter(const IncrementalExportConfig& config = {});

    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"jsonl", "incremental_jsonl"};
    }

    std::string getName() const override { return "incremental_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /**
     * @brief Read Watermark.
     * @return Return value.
     */
    int64_t readWatermark() const;

    /**
     * @brief Write Watermark.
     * @param[in] sequence Input parameter.
     * @param[in] exported_count Input parameter.
     * @param[in] timestamp Input parameter.
     * @return True when the operation succeeds.
     */
    bool writeWatermark(int64_t sequence,
                        size_t exported_count,
                        const std::string& timestamp) const;

    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    IncrementalExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /**
     * @brief Extract Sequence.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    int64_t extractSequence(const BaseEntity& entity) const;

    /**
     * @brief Format Entity.
     * @param[in] entity Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    static std::string formatEntity(const BaseEntity& entity,
                                    const ExportOptions& options);
};

} // namespace themis::exporters
