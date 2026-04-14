/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            streaming_exporter.h                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:38:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 384a0bfa56  2026-02-26  Implement streaming export for large collections with pro... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "exporter_interface.h"
#include "exporter_metrics.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themis::exporters {

/// Abstract cursor for paginated entity access.
/// Enables streaming export without loading all entities into memory.
class ExportCursor {
public:
    virtual ~ExportCursor() = default;

    /// Returns true if more pages are available.
    virtual bool hasNext() const = 0;

    /// Fetches the next page of entities. Must only be called when hasNext() is true.
    virtual std::vector<BaseEntity> nextPage() = 0;

    /// Total number of entities in the collection (0 if unknown).
    virtual size_t totalCount() const { return 0; }

    /// Number of entities already consumed from the cursor.
    virtual size_t currentOffset() const = 0;

    /// Seek to a specific offset (for checkpoint resume). Returns false if unsupported.
    virtual bool seekTo(size_t offset) { return false; }
};

/// Concrete cursor that pages over an in-memory std::vector<BaseEntity>.
/// Suitable for testing and for wrapping small collections.
class VectorExportCursor : public ExportCursor {
public:
    /// Construct a cursor over @p entities using @p page_size entities per page.
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

/// Configuration for the StreamingExporter.
struct StreamingExportConfig {
    /// Number of entities fetched per cursor page (default: 1 000).
    size_t page_size = 1000;

    /// Maximum bytes buffered in the StreamWriter before a forced flush (default: 256 MB).
    size_t max_buffer_bytes = 256 * 1024 * 1024;

    /// Path to the checkpoint file for resumable exports.
    /// Empty string disables checkpointing. When set, the last committed cursor
    /// offset is persisted after each page so that a restart can resume from
    /// the last completed page.
    std::string checkpoint_path;
};

/// Streaming exporter for collections that exceed available memory.
///
/// Unlike JSONLLLMExporter (which requires all entities in a vector),
/// StreamingExporter operates on an ExportCursor and advances one page at a
/// time so that peak resident memory is bounded by
/// StreamingExportConfig::max_buffer_bytes.
///
/// Progress callbacks (ExportOptions::progress_callback) are invoked with
/// updated ExportStats after every progress_interval entities; the
/// ExportStats::estimated_eta_seconds field is populated when the cursor
/// reports a non-zero totalCount().
class StreamingExporter : public IExporter {
public:
    explicit StreamingExporter(const StreamingExportConfig& config = {});

    /// IExporter interface: wraps entities in a VectorExportCursor and
    /// delegates to exportFromCursor().
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    /// Primary streaming API: export entities from a cursor page by page.
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

    /// Serialize a single entity to a JSONL line (all fields, field filtering applied).
    static std::string formatEntity(const BaseEntity& entity, const ExportOptions& options);

    /// Persist the cursor offset to @p path (atomic write via temp-file + rename).
    static void writeCheckpoint(const std::string& path, size_t offset);

    /// Read the last committed offset from @p path. Returns 0 if the file does not exist.
    static size_t readCheckpoint(const std::string& path);

    /// Compute estimated remaining seconds given progress and elapsed time.
    static double calculateETA(
        size_t processed,
        size_t total,
        std::chrono::steady_clock::time_point start_time
    );
};

} // namespace themis::exporters
