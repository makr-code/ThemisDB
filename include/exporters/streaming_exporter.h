/**
 * @file streaming_exporter.h
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
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themis::exporters {

class ExportCursor {
public:
    /**
     * @brief Export Cursor.
     * @return Return value.
     */
    virtual ~ExportCursor() = default;

    [[nodiscard]] virtual bool hasNext() const = 0;

    [[nodiscard]] virtual std::vector<BaseEntity> nextPage() = 0;

    virtual size_t totalCount() const { return 0; }

    [[nodiscard]] virtual size_t currentOffset() const = 0;

    virtual bool seekTo([[maybe_unused]] size_t offset) { return false; }
};

class VectorExportCursor : public ExportCursor {
public:
    VectorExportCursor(const std::vector<BaseEntity>& entities, size_t page_size = 1000);

    bool hasNext() const override;
    std::vector<BaseEntity> nextPage() override;
    size_t totalCount() const override { return entities_.size(); }
    size_t currentOffset() const override { return offset_; }
    bool seekTo(size_t offset) override;

private:
    const std::vector<BaseEntity>& entities_;
    size_t offset_ = 0;
    size_t page_size_;
};

struct StreamingExportConfig {
    size_t page_size = 1000;

    size_t max_buffer_bytes = 256 * 1024 * 1024;

    std::string checkpoint_path;
};

class StreamingExporter : public IExporter {
public:
    explicit StreamingExporter(const StreamingExportConfig& config = {});

    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    /**
     * @brief Export From Cursor.
     * @param[in,out] cursor Input/output parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    ExportStats exportFromCursor(
        ExportCursor& cursor,
        const ExportOptions& options
    );

    std::vector<std::string> getSupportedFormats() const override {
        return {"jsonl", "streaming_jsonl"};
    }

    std::string getName() const override { return "streaming_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    StreamingExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /**
     * @brief Format Entity.
     * @param[in] entity Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    static std::string formatEntity(const BaseEntity& entity, const ExportOptions& options);

    /**
     * @brief Write Checkpoint.
     * @param[in] path Input parameter.
     * @param[in] offset Input parameter.
     */
    static void writeCheckpoint(const std::string& path, size_t offset);

    /**
     * @brief Read Checkpoint.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static size_t readCheckpoint(const std::string& path);

    /**
     * @brief Calculate ETA.
     * @param[in] processed Input parameter.
     * @param[in] total Input parameter.
     * @param[in] start_time Input parameter.
     * @return Return value.
     */
    static double calculateETA(
        size_t processed,
        size_t total,
        std::chrono::steady_clock::time_point start_time
    );
};

} // namespace themis::exporters
