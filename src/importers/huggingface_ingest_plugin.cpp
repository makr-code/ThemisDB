/**
 * @file huggingface_ingest_plugin.cpp
 * @brief HuggingFace dataset ingest plugin implementation.
 *
 * Implements the HuggingFace dataset importer that fetches dataset
 * records via the HuggingFace Datasets API and ingests them into ThemisDB.
 */

#include "importers/huggingface_ingest_plugin.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace themis::importers {

namespace {

std::string trim(std::string value) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&]([[maybe_unused]] unsigned char c) { return !is_space(c); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&]([[maybe_unused]] unsigned char c) { return !is_space(c); }).base(), value.end());
    return value;
}

std::string rowString(const json& row, std::string_view key, std::string default_value = {}) {
    if (!row.contains(std::string(key))) {
        return default_value;
    }
    const auto& v = row[std::string(key)];
    if (v.is_string()) {
        return v.get<std::string>();
    }
    return v.dump();
}

std::string stableHashHex(std::string_view input) {
    const auto hash_value = std::hash<std::string_view>{}(input);
    std::ostringstream oss = {};
    oss << std::hex << hash_value;
    return oss.str();
}

} // namespace

HuggingFaceIngestPlugin::HuggingFaceIngestPlugin(HuggingFaceIngestConfig config)
    : config_(std::move(config)) {}

bool HuggingFaceIngestPlugin::init() {
    initialized_ = true;
    return true;
}

void HuggingFaceIngestPlugin::shutdown() {
    initialized_ = false;
}

std::vector<json> HuggingFaceIngestPlugin::fetchRawRows(const HuggingFaceImportRequest& request) const {
    // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
    // Reason: User requested structured HuggingFace adapter scaffold before backend wiring.
    // Activation: Used when no external HuggingFace connector is injected.
    // Production Delta: Consumes provided seed_rows instead of live HuggingFace endpoints.
    // Approved By: User request in issue statement (2026-06-24).
    // Removal Target: Replace with production adapter in HuggingFace source-connector hardening phase.
    return request.seed_rows;
}

std::vector<json> HuggingFaceIngestPlugin::fetchRawRows(const HuggingFaceUpdateRequest& request) const {
    // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
    // Reason: User requested structured HuggingFace adapter scaffold before backend wiring.
    // Activation: Used when no external HuggingFace connector is injected.
    // Production Delta: Consumes provided changed_rows instead of live incremental stream.
    // Approved By: User request in issue statement (2026-06-24).
    // Removal Target: Replace with production update connector in HuggingFace delta hardening phase.
    return request.changed_rows;
}

HuggingFaceIngestPlugin::NormalizationResult HuggingFaceIngestPlugin::normalizeLegalRecord(
    const json& row,
    std::string_view dataset_name,
    std::string_view split,
    std::size_t row_index) const {
    NormalizationResult result;

    result.document.id = rowString(row, "id", std::string(dataset_name) + ":" + std::to_string(row_index));
    result.document.dataset_name = std::string(dataset_name);
    result.document.split = std::string(split);
    result.document.language = rowString(row, "language", "en");
    result.document.jurisdiction = rowString(row, "jurisdiction", "unknown");
    result.document.court = rowString(row, "court", "unknown");
    result.document.topic = rowString(row, "topic", rowString(row, "label", "legal"));
    result.document.issued_at = rowString(row, "issued_at");
    result.document.text = trim(rowString(row, "text"));

    if (result.document.text.empty()) {
        result.error = "missing legal document text";
        return result;
    }

    const std::string license = trim(rowString(row, "license", "unknown"));
    bool license_ok = config_.allowed_licenses.empty() || config_.allowed_licenses.count(license) > 0;
    if (config_.license_gate_hook) {
        license_ok = config_.license_gate_hook(license);
    }

    result.audit.document_id = result.document.id;
    result.audit.license_passed = license_ok;
    if (!license_ok) {
        result.error = "license gate rejected record: " + license;
        result.audit.notes = result.error;
        return result;
    }

    if (config_.pii_redaction_hook) {
        const std::string redacted = config_.pii_redaction_hook(result.document.text);
        result.audit.redaction_applied = (redacted != result.document.text);
        result.document.text = redacted;
    }

    const std::string label = rowString(row, "label");
    if (!label.empty()) {
        result.annotations.push_back(
            LegalAnnotation{result.document.id, "topic", label, row.value("label_confidence", 1.0)});
    }

    if (row.contains("annotations") && row["annotations"].is_array()) {
        for (const auto& annotation : row["annotations"]) {
            result.annotations.push_back(LegalAnnotation{
                result.document.id,
                rowString(annotation, "type", "generic"),
                rowString(annotation, "value"),
                annotation.value("confidence", 1.0)});
        }
    }

    result.ok = true;
    return result;
}

double HuggingFaceIngestPlugin::computeQualityScore(const json& raw_row, const LegalDocument& document) const {
    if (config_.quality_score_hook) {
        return std::clamp(config_.quality_score_hook(raw_row), 0.0, 1.0);
    }

    const auto length = document.text.size();
    if (length < config_.min_quality_text_length) {
        return 0.2;
    }
    if (length > 10000) {
        return 0.5;
    }
    return 0.9;
}

std::string HuggingFaceIngestPlugin::buildLeakageSensitiveSplit(const LegalDocument& document) const {
    const std::string leakage_group = document.jurisdiction + "|" + document.court + "|" + document.issued_at;
    const auto bucket = std::hash<std::string>{}(leakage_group) % 10;
    if (bucket < 8) {
        return "train";
    }
    if (bucket == 8) {
        return "val";
    }
    return "test";
}

void HuggingFaceIngestPlugin::projectDocument(
    const LegalDocument& document,
    const std::vector<LegalAnnotation>& annotations) {
    projections_.graph_edges.emplace_back(document.id, document.jurisdiction, "IN_JURISDICTION");
    projections_.graph_edges.emplace_back(document.id, document.court, "IN_COURT");
    projections_.graph_edges.emplace_back(document.id, document.topic, "HAS_TOPIC");
    for (const auto& annotation : annotations) {
        projections_.graph_edges.emplace_back(document.id, annotation.label_value, "HAS_LABEL");
    }

    const std::int64_t now_bucket = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() / 60);
    auto& metric = projections_.timeseries_metrics[now_bucket];
    metric.bucket_epoch_seconds = now_bucket;
    metric.ingested += 1;

    projections_.process_events.push_back(ProcessEvent{
        "ingestion_normalized",
        document.id,
        "dataset=" + document.dataset_name + ",split=" + document.split});

    if (config_.embedding_sink_hook) {
        config_.embedding_sink_hook(document.id, std::vector<float>{});
    }
}

bool HuggingFaceIngestPlugin::isDuplicate(const LegalDocument& document) const {
    const std::string new_hash = stableHashHex(document.text);
    for (const auto& [id, existing_hash] : text_hash_by_document_) {
        if (existing_hash == new_hash && id != document.id) {
            return true;
        }
        if (config_.near_duplicate_hook && id != document.id) {
            const auto it = legal_documents_.find(id);
            if (it != legal_documents_.end() && config_.near_duplicate_hook(it->second.text, document.text)) {
                return true;
            }
        }
    }
    return false;
}

std::size_t HuggingFaceIngestPlugin::upsertCanonical(
    const NormalizationResult& normalized,
    const std::string& split,
    bool* inserted) {
    auto it = legal_documents_.find(normalized.document.id);
    const bool exists = (it != legal_documents_.end());
    if (!exists) {
        legal_documents_[normalized.document.id] = normalized.document;
        *inserted = true;
    } else {
        if (it->second.text != normalized.document.text || it->second.topic != normalized.document.topic) {
            dirty_record_ids_.insert(normalized.document.id);
            it->second = normalized.document;
        }
        *inserted = false;
    }

    legal_annotations_[normalized.document.id] = normalized.annotations;
    compliance_audits_[normalized.document.id] = normalized.audit;
    text_hash_by_document_[normalized.document.id] = stableHashHex(normalized.document.text);

    TrainingExample example;
    example.example_id = normalized.document.id + "::" + split;
    example.document_id = normalized.document.id;
    example.split = split;
    example.instruction = "Analyze the legal text and answer the request.";
    example.input = normalized.document.text;
    example.target = normalized.document.topic;
    example.quality_score = computeQualityScore(json::object(), normalized.document);
    training_examples_[example.example_id] = std::move(example);

    return 1;
}

void HuggingFaceIngestPlugin::updateCheckpoint(std::size_t processed_records) {
    if (config_.checkpoint_every == 0 || processed_records == 0) {
        return;
    }
    if (processed_records % config_.checkpoint_every == 0) {
        ++checkpoint_counter_;
        checkpoint_token_ = "checkpoint:" + std::to_string(checkpoint_counter_);
    }
}

IngestionReport HuggingFaceIngestPlugin::runFullImport(const HuggingFaceImportRequest& request) {
    if (!initialized_) {
        throw std::logic_error("HuggingFaceIngestPlugin must be initialized before import");
    }

    legal_documents_.clear();
    legal_annotations_.clear();
    training_examples_.clear();
    compliance_audits_.clear();
    dirty_record_ids_.clear();
    dead_letter_records_.clear();
    projections_ = ProjectionState{};

    IngestionReport report;
    const std::string dataset = request.dataset_name.empty() ? config_.dataset_name : request.dataset_name;
    if (dataset.empty()) {
        report.errors.push_back("dataset_name is required");
        return report;
    }

    const auto rows = fetchRawRows(request);
    hf_dataset_catalog_.insert(dataset + "@" + request.snapshot_id);

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto normalized = normalizeLegalRecord(rows[i], dataset, request.split, i);
        if (!normalized.ok || isDuplicate(normalized.document)) {
            ++report.failed_records;
            dead_letter_records_.push_back(DeadLetterRecord{
                dataset,
                normalized.ok ? "duplicate_or_near_duplicate" : normalized.error,
                rows[i]});
            if (config_.strict_mode) {
                report.errors.push_back(dead_letter_records_.back().reason);
                return report;
            }
            continue;
        }

        bool inserted = false;
        report.imported_documents += upsertCanonical(normalized, buildLeakageSensitiveSplit(normalized.document), &inserted);
        projectDocument(normalized.document, normalized.annotations);
        if (!inserted) {
            report.dirty_records += 1;
        }
        updateCheckpoint(i + 1);
    }

    report.dirty_records += dirty_record_ids_.size();
    report.checkpoint_token = checkpoint_token_;
    report.success = report.errors.empty();
    return report;
}

IngestionReport HuggingFaceIngestPlugin::runIncrementalUpdate(const HuggingFaceUpdateRequest& request) {
    if (!initialized_) {
        throw std::logic_error("HuggingFaceIngestPlugin must be initialized before update");
    }

    IngestionReport report;
    const std::string dataset = request.dataset_name.empty() ? config_.dataset_name : request.dataset_name;
    if (dataset.empty()) {
        report.errors.push_back("dataset_name is required");
        return report;
    }

    if (!request.resume_from_checkpoint) {
        checkpoint_token_ = "checkpoint:0";
        checkpoint_counter_ = 0;
    }

    const auto rows = fetchRawRows(request);
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto normalized = normalizeLegalRecord(rows[i], dataset, request.split, i);
        if (!normalized.ok || isDuplicate(normalized.document)) {
            ++report.failed_records;
            dead_letter_records_.push_back(DeadLetterRecord{
                dataset,
                normalized.ok ? "duplicate_or_near_duplicate" : normalized.error,
                rows[i]});
            if (config_.strict_mode) {
                report.errors.push_back(dead_letter_records_.back().reason);
                return report;
            }
            continue;
        }

        bool inserted = false;
        report.imported_documents += upsertCanonical(normalized, buildLeakageSensitiveSplit(normalized.document), &inserted);
        report.dirty_records += inserted ? 0 : 1;
        projectDocument(normalized.document, normalized.annotations);
        updateCheckpoint(i + 1);
    }

    report.dirty_records += dirty_record_ids_.size();
    report.checkpoint_token = checkpoint_token_;
    report.success = report.errors.empty();
    return report;
}

ValidationReport HuggingFaceIngestPlugin::validateQuality() const {
    ValidationReport report;
    report.checked_examples = training_examples_.size();

    for (const auto& [id, example] : training_examples_) {
        (void)id;
        if (static_cast<int>(example.input.size()) < config_.min_quality_text_length) {
            report.ok = false;
            ++report.failed_examples;
            report.errors.push_back("training example below min text length: " + example.example_id);
        }
        if (example.target.empty()) {
            report.ok = false;
            ++report.failed_examples;
            report.errors.push_back("training example missing target: " + example.example_id);
        }
    }

    for (const auto& [doc_id, audit] : compliance_audits_) {
        if (!audit.license_passed) {
            report.ok = false;
            report.errors.push_back("compliance license gate failed: " + doc_id);
        }
    }

    return report;
}

AdaLoraExportReport HuggingFaceIngestPlugin::exportAdaLoraJsonl(const AdaLoraExportRequest& request) const {
    AdaLoraExportReport report;
    report.output_path = request.output_path;

    if (request.output_path.empty()) {
        report.errors.push_back("output_path is required");
        return report;
    }

    std::vector<std::reference_wrapper<const TrainingExample>> ordered_examples;
    ordered_examples.reserve(training_examples_.size());
    for (const auto& [_, example] : training_examples_) {
        ordered_examples.emplace_back(std::cref(example));
    }

    const bool deterministic = request.deterministic && config_.deterministic_export;
    if (deterministic) {
        std::sort(ordered_examples.begin(), ordered_examples.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.get().example_id < rhs.get().example_id;
        });
    }

    std::ofstream out(request.output_path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        report.errors.push_back("failed to open output file: " + request.output_path);
        return report;
    }

    for (const auto& entry : ordered_examples) {
        const auto& example = entry.get();
        json line = {};
        if (request.format == AdaLoraExportFormat::PROMPT_RESPONSE) {
            line["prompt"] = example.input;
            line["response"] = example.target;
        } else {
            line["instruction"] = example.instruction;
            line["input"] = example.input;
            line["target"] = example.target;
            if (request.include_system_field || request.system_prompt.has_value()) {
                line["system"] = request.system_prompt.value_or("");
            }
        }
        line["split"] = example.split;
        line["quality_score"] = example.quality_score;
        line["document_id"] = example.document_id;
        out << line.dump() << '\n';
        ++report.exported_examples;
    }

    report.success = report.errors.empty();
    return report;
}

std::vector<std::string> HuggingFaceIngestPlugin::canonicalTableNames() const {
    return {
        "hf_dataset_catalog",
        "legal_document",
        "legal_annotation",
        "training_example",
        "compliance_audit"};
}

} // namespace themis::importers
