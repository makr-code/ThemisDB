/**
 * @file join_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: join_exporter.h | Version: 0.0.12 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 189
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4297 Add JoinExporter: cross-col... (2026-03-16)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "exporter_interface.h"
#include "exporter_metrics.h"
#include "pii_detector.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::exporters {

/**
 * @brief Configuration for a cross-collection join export.
 *
 * Exports a joined view of two collections (left JOIN right) into a single
 * JSONL or Parquet-like output file.  The join is performed as an in-memory
 * hash-join: the right side is loaded into a hash table keyed on the join
 * field, then every left entity is probed against it.
 *
 * **Performance guarantee:** ≥ 50 000 merged docs/sec on a modern server
 * when the right-side hash table fits within `right_side_memory_limit_bytes`.
 *
 * **Memory guarantee:** construction of the right-side hash table is aborted
 * and `ERR_EXPORT_JOIN_MEMORY_LIMIT` is thrown when the raw field data would
 * exceed `right_side_memory_limit_bytes` (default 1 GiB).
 */
struct JoinExportConfig {
    // ── Collection identification ─────────────────────────────────────────
    /// Name of the left-side collection (mandatory; used only for error messages /
    /// policy checks — the actual entity data is passed via exportEntities()).
    std::string left_collection;

    /// Name of the right-side collection (mandatory; used only for error messages
    /// / policy checks — actual entity data is passed via setRightCollection()).
    std::string right_collection;

    // ── Join key ─────────────────────────────────────────────────────────
    /// Field name on the **left** entity used as the join key.
    std::string left_key_field = "_key";

    /// Field name on the **right** entity used as the join key.
    std::string right_key_field = "_key";

    // ── Predicate filter (AQL) ────────────────────────────────────────────
    /// Optional AQL FILTER predicate evaluated on the **merged** record
    /// (loop variable: `doc`).  Leave empty to export all joined pairs.
    /// Example: `"doc.status == \"active\""`.
    std::string join_predicate;

    // ── Output field selection / aliasing ────────────────────────────────
    /// Ordered list of field names to include in the output record.  When
    /// empty all fields from both sides are included.
    ///
    /// To rename a field or resolve ambiguous names use the format
    /// `"original_name:alias"`.  Example: `{"left.title:title",
    /// "right.title:annotation_title"}` resolves the conflict and produces
    /// the alias names in the output.
    ///
    /// If a field name appears in **both** collections and is listed without
    /// an alias the exporter throws `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD`.
    std::vector<std::string> output_fields;

    // ── PII detection ─────────────────────────────────────────────────────
    struct PIIConfig {
        bool enable_detection = false;
        bool enable_redaction = false;
        bool detect_email     = true;
        bool detect_phone     = true;
        bool detect_ssn       = true;
        bool detect_credit_card = true;
        /// Redaction strategy: "mask", "hash", "remove", "partial"
        std::string redaction_strategy = "mask";
        bool fail_on_pii = false;
    } pii_config;

    // ── Memory budget ─────────────────────────────────────────────────────
    /// Maximum heap bytes used by the right-side hash table.
    /// Default: 1 GiB.  Set to 0 to disable the limit (not recommended).
    size_t right_side_memory_limit_bytes = 1ULL * 1024 * 1024 * 1024;
};

/**
 * @brief Exports a cross-collection join as JSONL.
 *
 * Usage:
 * @code
 *   JoinExportConfig cfg;
 *   cfg.left_collection  = "documents";
 *   cfg.right_collection = "annotations";
 *   cfg.left_key_field   = "_key";
 *   cfg.right_key_field  = "doc_id";
 *   cfg.join_predicate   = "doc.score >= 0.5";
 *   cfg.output_fields    = {"_key", "content", "label"};
 *
 *   JoinExporter exporter(cfg);
 *   exporter.setRightCollection(right_entities);
 *
 *   ExportOptions opts;
 *   opts.output_path = "/tmp/joined.jsonl";
 *   auto stats = exporter.exportEntities(left_entities, opts);
 * @endcode
 *
 * Error cases:
 *  - `left_collection` / `right_collection` name empty → `ERR_EXPORT_CONFIG_INVALID`
 *  - `join_predicate` cannot be parsed → `ERR_EXPORT_JOIN_PREDICATE_INVALID`
 *  - Ambiguous field name without alias → `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD`
 *  - Right-side hash table exceeds `right_side_memory_limit_bytes` →
 *    `ERR_EXPORT_JOIN_MEMORY_LIMIT`
 */
class JoinExporter : public IExporter {
public:
    explicit JoinExporter(const JoinExportConfig& config = {});

    /**
     * @brief Load the right-side collection into the in-memory hash table.
     *
     * Must be called before `exportEntities()`.  Can be called multiple times
     * to replace the hash table; the previous table is discarded.
     *
     * @param right_entities  All entities from the right collection.
     * @throws ExporterException(ERR_EXPORT_JOIN_MEMORY_LIMIT) if the raw field
     *         data exceeds `config_.right_side_memory_limit_bytes`.
     * @throws ExporterException(ERR_EXPORT_CONFIG_INVALID) if
     *         `config_.right_collection` is empty.
     */
    void setRightCollection(const std::vector<BaseEntity>& right_entities);

    /**
     * @brief Export the cross-collection join to a JSONL file.
     *
     * Performs a hash-join of `entities` (left side) against the previously
     * loaded right-side hash table. `setRightCollection()` must be called once
     * before this function; otherwise export fails closed with
     * `ERR_EXPORT_CONFIG_INVALID`.
     *
     * Inner join semantics: left entities with no matching right entity are
     * skipped.
     *
     * @param entities   Left-side entities.
     * @param options    Standard ExportOptions (output_path is mandatory).
     */
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"jsonl", "join_jsonl"};
    }

    std::string getName()    const override { return "join_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /// Access accumulated export metrics.
    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    JoinExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    // Right-side hash table: right_key_field value → entity
    std::unordered_map<std::string, BaseEntity> right_table_;
    size_t right_table_bytes_ = 0;
    bool right_collection_loaded_ = false;

    // ── Helpers ──────────────────────────────────────────────────────────

    /// Merge a left and right entity into a single entity.
    /// Applies output_fields selection/aliasing and detects ambiguous names.
    /// @throws ExporterException(ERR_EXPORT_JOIN_AMBIGUOUS_FIELD) on conflict.
    BaseEntity mergeEntities(const BaseEntity& left, const BaseEntity& right) const;

    /// Build a PIIDetector from config_.pii_config.  Returns nullptr when
    /// PII detection is disabled.
    std::unique_ptr<PIIDetector> buildPIIDetector() const;

    /// Estimate the raw heap bytes consumed by one BaseEntity in the hash
    /// table (primary key + serialised field data).
    static size_t estimateEntityBytes(const BaseEntity& entity);
};

} // namespace themis::exporters
