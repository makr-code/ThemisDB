/**
 * @file join_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct JoinExportConfig {
    // ── Collection identification ─────────────────────────────────────────
    std::string left_collection;

    std::string right_collection;

    // ── Join key ─────────────────────────────────────────────────────────
    std::string left_key_field = "_key";

    std::string right_key_field = "_key";

    // ── Predicate filter (AQL) ────────────────────────────────────────────
    std::string join_predicate;

    // ── Output field selection / aliasing ────────────────────────────────
    std::vector<std::string> output_fields;

    // ── PII detection ─────────────────────────────────────────────────────
    struct PIIConfig {
        bool enable_detection = false;
        bool enable_redaction = false;
        bool detect_email     = true;
        bool detect_phone     = true;
        bool detect_ssn       = true;
        bool detect_credit_card = true;
        std::string redaction_strategy = "mask";
        bool fail_on_pii = false;
    } pii_config;

    // ── Memory budget ─────────────────────────────────────────────────────
    size_t right_side_memory_limit_bytes = 1ULL * 1024 * 1024 * 1024;
};

class JoinExporter : public IExporter {
public:
    explicit JoinExporter(const JoinExportConfig& config = {});

    /**
     * @brief Set Right Collection.
     * @param[in] right_entities Input parameter.
     */
    void setRightCollection(const std::vector<BaseEntity>& right_entities);

    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"jsonl", "join_jsonl"};
    }

    std::string getName()    const override { return "join_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    JoinExportConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    // Right-side hash table: right_key_field value → entity
    std::unordered_map<std::string, BaseEntity> right_table_;
    size_t right_table_bytes_ = 0;
    bool right_collection_loaded_ = false;

    /**
     * @brief ── Helpers ──────────────────────────────────────────────────────────
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */

    BaseEntity mergeEntities(const BaseEntity& left, const BaseEntity& right) const;

    /**
     * @brief Build PIIDetector.
     * @return Return value.
     */
    std::unique_ptr<PIIDetector> buildPIIDetector() const;

    /**
     * @brief Estimate Entity Bytes.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    static size_t estimateEntityBytes(const BaseEntity& entity);
};

} // namespace themis::exporters
