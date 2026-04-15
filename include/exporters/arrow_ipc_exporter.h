/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            arrow_ipc_exporter.h                               ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:09:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f7a4d85f3b  2026-02-26  Changes before error encountered        ║
    • c7c509d739  2026-02-22  feat(exporters): add Parquet export for training datasets ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/// Output format variant for Arrow IPC
enum class ArrowIPCFormat {
    /// Arrow IPC File format (.arrow / Feather v2).
    /// Produces a self-contained file with magic bytes, schema, record
    /// batches, and a footer.  Readable by pyarrow, Polars, DuckDB, etc.
    FILE,

    /// Arrow IPC Stream format (.arrows).
    /// Produces a streaming sequence of messages without file magic or
    /// footer.  Suitable for piped zero-copy IPC.
    STREAM,
};

/// Configuration for Arrow IPC export
struct ArrowIPCExportConfig {
    /// Output format variant (FILE or STREAM)
    ArrowIPCFormat format = ArrowIPCFormat::FILE;

    /// When true, schema is inferred from the first batch of entities.
    /// When false, schema is derived solely from column_hints (not yet
    /// implemented; effectively treated as true).
    bool auto_detect_schema = true;

    /// Columns to include (empty = include all fields from entities)
    std::vector<std::string> include_columns;

    /// Columns to always exclude
    std::vector<std::string> exclude_columns;

    /// Optional key/value metadata written to the Arrow schema
    std::map<std::string, std::string> schema_metadata;

    /// When true, null counts are recorded accurately per column.
    /// When false (default), null_count = 0 is reported for all columns
    /// (valid only when all values are non-null).
    bool track_nulls = false;
};

/// Arrow IPC exporter for zero-copy data pipelines.
///
/// Implements IExporter and writes entities to Apache Arrow IPC files
/// (.arrow) or streams (.arrows) with all entity fields represented as
/// UTF-8 string columns.
///
/// When compiled with ARROW_ENABLED (Apache Arrow C++ available),
/// actual Arrow IPC files are produced via arrow::ipc::RecordBatchWriter.
///
/// When ARROW_ENABLED is not defined, a minimal standards-conformant Arrow
/// IPC File (or Stream) is written using an internal FlatBuffer encoder —
/// no external libraries required.  Both paths produce files readable by
/// pyarrow, Polars, DuckDB, and any compliant Arrow IPC reader.
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

    /// Replace the current configuration
    void setConfig(const ArrowIPCExportConfig& config) { config_ = config; }
    const ArrowIPCExportConfig& getConfig() const { return config_; }

    /// Access live metrics (may be polled at any time)
    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

    /// Reset all collected metrics
    void resetMetrics() { if (metrics_) metrics_->reset(); }

    /// Returns true if the Apache Arrow C++ library was compiled in
    static bool isArrowAvailable();

private:
    ArrowIPCExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /// Determine the effective column set for this export
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
