/**
 * @file huggingface_ingest_plugin.h
 * @brief HuggingFace dataset ingestion plugin for legal training data pipelines.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::importers {

using json = nlohmann::json;

enum class AdaLoraExportFormat {
    INSTRUCTION_TUNING,
    PROMPT_RESPONSE
};

struct HuggingFaceIngestConfig {
    std::string dataset_name;
    std::string default_split = "train";
    std::set<std::string> allowed_licenses = {"cc-by-4.0", "mit", "apache-2.0"};
    bool strict_mode = false;
    bool deterministic_export = true;
    std::size_t min_quality_text_length = 64;
    std::size_t checkpoint_every = 500;

    std::function<bool(std::string_view)> license_gate_hook;
    std::function<std::string(std::string_view)> pii_redaction_hook;
    std::function<double(const json&)> quality_score_hook;
    std::function<bool(const std::string&, const std::string&)> near_duplicate_hook;
    std::function<void(std::string_view, const std::vector<float>&)> embedding_sink_hook;
};

struct HuggingFaceImportRequest {
    std::string dataset_name;
    std::string split = "train";
    std::string snapshot_id;
    std::vector<json> seed_rows;
    bool resume_from_checkpoint = true;
};

struct HuggingFaceUpdateRequest {
    std::string dataset_name;
    std::string split = "train";
    std::string update_token;
    std::vector<json> changed_rows;
    bool resume_from_checkpoint = true;
};

struct IngestionReport {
    bool success = false;
    std::size_t imported_documents = 0;
    std::size_t failed_records = 0;
    std::size_t dirty_records = 0;
    std::string checkpoint_token;
    std::vector<std::string> errors;
};

struct ValidationReport {
    bool ok = true;
    std::size_t checked_examples = 0;
    std::size_t failed_examples = 0;
    std::vector<std::string> errors;
};

struct AdaLoraExportRequest {
    std::string output_path;
    AdaLoraExportFormat format = AdaLoraExportFormat::INSTRUCTION_TUNING;
    bool include_system_field = false;
    std::optional<std::string> system_prompt;
    bool deterministic = true;
};

struct AdaLoraExportReport {
    bool success = false;
    std::string output_path;
    std::size_t exported_examples = 0;
    std::vector<std::string> errors;
};

/**
 * @brief HuggingFace legal ingestion plugin with canonicalization and AdaLoRA export.
 *
 * Pipeline:
 * - Raw ingest (HuggingFace adapter hook)
 * - Canonical relational core normalization
 * - Multi-model projection hooks (graph/vector/process/timeseries)
 * - Training export (AdaLoRA-compatible JSONL)
 */
class HuggingFaceIngestPlugin {
public:
    explicit HuggingFaceIngestPlugin(HuggingFaceIngestConfig config = {});

    /**
     * @brief Initialize plugin state and lifecycle resources.
     * @return true when initialization was successful.
     */
    [[nodiscard]] bool init();

    /**
     * @brief Shutdown plugin and release in-memory lifecycle state.
     */
    void shutdown();

    /**
     * @brief Execute full dataset snapshot import.
     * @param request Full import request containing dataset and raw seed payload.
     * @return Ingestion report including dirty tracking and checkpoint metadata.
     */
    [[nodiscard]] IngestionReport runFullImport(const HuggingFaceImportRequest& request);

    /**
     * @brief Execute incremental update/refresh import.
     * @param request Update request with changed records and resume behavior.
     * @return Ingestion report including idempotent upsert and dead-letter outcomes.
     */
    [[nodiscard]] IngestionReport runIncrementalUpdate(const HuggingFaceUpdateRequest& request);

    /**
     * @brief Run quality and compliance validation over canonical/training records.
     * @return Validation report with failing examples and detailed error strings.
     */
    [[nodiscard]] ValidationReport validateQuality() const;

    /**
     * @brief Export canonical training examples to AdaLoRA-compatible JSONL.
     * @param request Export format and output path configuration.
     * @return Export report with deterministic record count and failure diagnostics.
     */
    [[nodiscard]] AdaLoraExportReport exportAdaLoraJsonl(const AdaLoraExportRequest& request) const;

    /**
     * @brief Return canonical relational core table names used by the pipeline.
     * @return Stable table list for integration checks and documentation.
     */
    [[nodiscard]] std::vector<std::string> canonicalTableNames() const;

    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
    [[nodiscard]] std::size_t deadLetterCount() const noexcept { return dead_letter_records_.size(); }
    [[nodiscard]] std::string checkpointToken() const { return checkpoint_token_; }

private:
    struct LegalDocument {
        std::string id;
        std::string dataset_name;
        std::string split;
        std::string text;
        std::string jurisdiction;
        std::string court;
        std::string topic;
        std::string language;
        std::string issued_at;
    };

    struct LegalAnnotation {
        std::string document_id;
        std::string label_type;
        std::string label_value;
        double confidence = 1.0;
    };

    struct TrainingExample {
        std::string example_id;
        std::string document_id;
        std::string split;
        std::string instruction;
        std::string input;
        std::string target;
        std::optional<std::string> system;
        double quality_score = 0.0;
    };

    struct ComplianceAudit {
        std::string document_id;
        bool license_passed = false;
        bool redaction_applied = false;
        std::string notes;
    };

    struct DeadLetterRecord {
        std::string dataset_name;
        std::string reason;
        json record;
    };

    struct ProcessEvent {
        std::string event_type;
        std::string document_id;
        std::string details;
    };

    struct TimeseriesMetric {
        std::int64_t bucket_epoch_seconds = 0;
        std::size_t ingested = 0;
        std::size_t rejected = 0;
    };

    struct ProjectionState {
        std::vector<std::tuple<std::string, std::string, std::string>> graph_edges;
        std::vector<ProcessEvent> process_events;
        std::unordered_map<std::int64_t, TimeseriesMetric> timeseries_metrics;
    };

    struct NormalizationResult {
        bool ok = false;
        LegalDocument document;
        std::vector<LegalAnnotation> annotations;
        ComplianceAudit audit;
        std::string error;
    };

    std::vector<json> fetchRawRows(const HuggingFaceImportRequest& request) const;
    std::vector<json> fetchRawRows(const HuggingFaceUpdateRequest& request) const;
    NormalizationResult normalizeLegalRecord(
        const json& row,
        std::string_view dataset_name,
        std::string_view split,
        std::size_t row_index) const;
    double computeQualityScore(const json& raw_row, const LegalDocument& document) const;
    std::string buildLeakageSensitiveSplit(const LegalDocument& document) const;
    void projectDocument(const LegalDocument& document, const std::vector<LegalAnnotation>& annotations);
    std::size_t upsertCanonical(const NormalizationResult& normalized, const std::string& split, bool* inserted);
    void updateCheckpoint(std::size_t processed_records);
    bool isDuplicate(const LegalDocument& document) const;

    HuggingFaceIngestConfig config_;
    bool initialized_ = false;
    std::string checkpoint_token_ = "checkpoint:0";
    std::size_t checkpoint_counter_ = 0;

    std::unordered_map<std::string, LegalDocument> legal_documents_;
    std::unordered_map<std::string, std::vector<LegalAnnotation>> legal_annotations_;
    std::unordered_map<std::string, TrainingExample> training_examples_;
    std::unordered_map<std::string, ComplianceAudit> compliance_audits_;
    std::set<std::string> hf_dataset_catalog_;
    std::set<std::string> dirty_record_ids_;
    std::vector<DeadLetterRecord> dead_letter_records_;
    ProjectionState projections_;
    mutable std::unordered_map<std::string, std::string> text_hash_by_document_;
};

} // namespace themis::importers
