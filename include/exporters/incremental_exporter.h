/**
 * @file incremental_exporter.h
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
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace themis::exporters {

/// Configuration for incremental / delta export.
///
/// The incremental exporter tracks the highest sequence value seen in each
/// export run and persists it as a "watermark" so that the next run only
/// exports documents whose sequence field exceeds the stored value.
struct IncrementalExportConfig {
    /// Field name used as the monotonically increasing sequence number.
    /// The field must contain an integer (int64_t) or floating-point (double)
    /// value.  Documents whose field value is absent or cannot be parsed as a
    /// number are always exported (fail-open: conservative correctness).
    /// Default: "_seq"
    std::string sequence_field = "_seq";

    /// Path to the watermark file.  The file contains a JSON object with
    /// metadata about the last completed export run.  If the path is empty,
    /// watermark persistence is disabled and every call behaves as a full
    /// export.
    ///
    /// Example file content:
    /// {
    ///   "last_sequence": 1234567890,
    ///   "last_export_time": "2026-02-27T07:00:00Z",
    ///   "exported_count": 500
    /// }
    std::string watermark_path;

    /// When true, entities without the sequence field (or with an
    /// unparseable value) are always exported.  When false, such entities
    /// are skipped.
    /// Default: true  (fail-open)
    bool export_missing_sequence = true;
};

/// Incremental / delta exporter.
///
/// Only documents whose sequence field value is strictly greater than the
/// stored watermark are exported.  After a successful run the watermark is
/// atomically updated to the highest sequence value observed in the exported
/// batch, ensuring that re-runs are idempotent and do not miss or double-count
/// records.
///
/// Watermark file writes use a tmp-then-rename strategy to prevent corrupt
/// state on crash, matching the behaviour of StreamingExporter's checkpoint
/// mechanism.
///
/// The exporter emits `exporter_delta_docs_skipped_total` via ExporterMetrics
/// so that Prometheus / Grafana can distinguish full-export from delta-export
/// runs and track change rates over time.
class IncrementalExporter : public IExporter {
public:
    explicit IncrementalExporter(const IncrementalExportConfig& config = {});

    /// IExporter interface.
    ///
    /// Filters @p entities to those with a sequence value strictly above the
    /// watermark, exports them to @p options.output_path (JSONL, one entity
    /// per line), and atomically updates the watermark file.
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"jsonl", "incremental_jsonl"};
    }

    std::string getName() const override { return "incremental_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /// Read the current watermark value.  Returns std::numeric_limits<int64_t>::min() when no
    /// watermark file exists or the path is empty (full-export mode).
    int64_t readWatermark() const;

    /// Write the watermark atomically.  Exposed for testing.
    bool writeWatermark(int64_t sequence,
                        size_t exported_count,
                        const std::string& timestamp) const;

    /// Get the exporter metrics object.
    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    IncrementalExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /// Extract the sequence value from an entity.  Returns std::numeric_limits<int64_t>::min()
    /// if the field is absent or unparseable.
    int64_t extractSequence(const BaseEntity& entity) const;

    /// Serialize a single entity to a JSONL line with field filtering applied.
    static std::string formatEntity(const BaseEntity& entity,
                                    const ExportOptions& options);
};

} // namespace themis::exporters
