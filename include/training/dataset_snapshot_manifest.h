/**
 * @file dataset_snapshot_manifest.h
 * @brief Dataset snapshot manifest for reproducible RAG training data governance
 * @version 1.0
 * @date 2026-07-01
 *
 * Provides versioned, auditable dataset snapshots with lineage tracking,
 * eligibility policies, and deterministic train/val/test split management.
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cstdint>

namespace themis {
namespace training {

// Forward declarations
struct DataSample;
struct LoRADataSelectionConfig;

// ============================================================================
// Eligibility Policy
// ============================================================================

/**
 * @brief Defines which criteria must be met for a sample to be eligible for training.
 *
 * Captures the rules applied during dataset construction to ensure reproducibility
 * and auditability of training data selection.
 */
struct EligibilityPolicy {
    /// Policy version for change tracking
    std::string policy_version = "1.0";

    /// Minimum quality score [0..1] required for inclusion
    double min_quality_score = 0.5;

    /// Maximum difficulty score [0..1] allowed (samples above are excluded)
    double max_difficulty_score = 0.95;

    /// Required language(s) for samples (ISO 639-1 codes)
    std::vector<std::string> required_languages;

    /// Domains that are eligible (empty = all domains eligible)
    std::vector<std::string> eligible_domains;

    /// Deduplication threshold (Jaccard similarity above this → rejected)
    double dedup_threshold = 0.95;

    /// PII handling mode: "reject", "redact", "allow"
    std::string pii_handling = "reject";

    /// Toxicity check enabled
    bool toxicity_check_enabled = true;

    /// Maximum toxicity score [0..1]
    double max_toxicity_score = 0.3;

    /// Custom policy attributes for extensibility
    std::map<std::string, std::string> custom_attributes;

    EligibilityPolicy() = default;

    /**
     * @brief Serialize to JSON string.
     */
    std::string toJSON() const;

    /**
     * @brief Deserialize from JSON string.
     */
    static EligibilityPolicy fromJSON(const std::string& json_str);
};

// ============================================================================
// Lineage Information
// ============================================================================

/**
 * @brief Tracks the source and transformation history of a sample.
 */
struct SampleLineage {
    /// Unique sample identifier
    std::string sample_id;

    /// Source document URN or identifier
    std::string source_document_id;

    /// Timestamp when sample was extracted
    std::chrono::system_clock::time_point extraction_timestamp;

    /// Labeler/processing version that created this sample
    std::string processing_version;

    /// Content modality (e.g., "text", "table", "citation")
    std::string modality;

    /// Hash of enrichment queries applied (for reproducibility)
    std::vector<std::string> enrichment_query_hashes;

    /// Upstream transformation steps (previous samples this was derived from)
    std::vector<std::string> upstream_sample_ids;

    /// Custom metadata preserved from source
    std::map<std::string, std::string> metadata;

    SampleLineage() : extraction_timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief Serialize to JSON string.
     */
    std::string toJSON() const;

    /**
     * @brief Deserialize from JSON string.
     */
    static SampleLineage fromJSON(const std::string& json_str);
};

// ============================================================================
// Split Assignment
// ============================================================================

/**
 * @brief Assigns a sample to train, validation, or test set with metadata.
 */
struct SplitAssignment {
    /// Sample identifier
    std::string sample_id;

    /// Split destination: "train", "validation", "test"
    std::string split;

    /// Fold number (for cross-validation scenarios)
    uint32_t fold_index = 0;

    /// Seed used for deterministic split generation (for reproducibility)
    uint64_t determinism_seed = 0;

    /// Weight assigned to sample in this split (for stratified sampling)
    double sample_weight = 1.0;

    SplitAssignment() = default;
    SplitAssignment(std::string sample_id_, std::string split_)
        : sample_id(std::move(sample_id_)), split(std::move(split_)) {}

    /**
     * @brief Serialize to JSON string.
     */
    std::string toJSON() const;

    /**
     * @brief Deserialize from JSON string.
     */
    static SplitAssignment fromJSON(const std::string& json_str);
};

// ============================================================================
// Dataset Snapshot Manifest
// ============================================================================

/**
 * @brief Immutable manifest capturing a complete dataset snapshot with full provenance.
 *
 * Serves as the definitive record of training data used for a LoRA/AdaLoRA run.
 * Includes lineage, eligibility policies, split assignments, and audit trail
 * to enable reproducible, auditable training data governance.
 *
 * Serializable to JSON and YAML for portability and long-term retention.
 */
class DatasetSnapshotManifest {
public:
    /// Manifest format version
    static constexpr int MANIFEST_VERSION = 1;

    // --- Metadata ---

    /// Unique identifier for this snapshot (typically UUID)
    std::string snapshot_id;

    /// Human-readable name/label for the snapshot
    std::string name;

    /// Creation timestamp
    std::chrono::system_clock::time_point created_at;

    /// SHA-256 checksum of snapshot content (for integrity verification)
    std::string content_checksum;

    /// Description of what this snapshot represents
    std::string description;

    /// Source data selection configuration hash (for reproducibility)
    std::string selection_config_hash;

    // --- Policy & Governance ---

    /// Eligibility policy applied when building this snapshot
    EligibilityPolicy eligibility_policy;

    /// Data governance policy identifier or version
    std::string governance_policy_id;

    /// Timestamp of last modification
    std::chrono::system_clock::time_point last_modified;

    // --- Data Statistics ---

    /// Total number of samples in the snapshot
    size_t total_samples = 0;

    /// Samples assigned to training split
    size_t train_samples = 0;

    /// Samples assigned to validation split
    size_t validation_samples = 0;

    /// Samples assigned to test split
    size_t test_samples = 0;

    /// Average quality score across all samples
    double avg_quality_score = 0.0;

    /// Average difficulty score across all samples
    double avg_difficulty_score = 0.0;

    /// Samples filtered out during quality check
    size_t filtered_by_quality = 0;

    /// Samples removed during deduplication
    size_t filtered_by_dedup = 0;

    /// Distribution of samples by domain (domain -> count)
    std::map<std::string, size_t> domain_distribution;

    /// Distribution of samples by detected language
    std::map<std::string, size_t> language_distribution;

    // --- Lineage & Audit ---

    /// Complete lineage for each sample
    std::vector<SampleLineage> sample_lineages;

    /// Split assignments for each sample
    std::vector<SplitAssignment> split_assignments;

    /// Audit entries from data selection process
    std::vector<std::string> audit_log_entries;

    /// References to external audit files or logs
    std::vector<std::string> external_audit_references;

    // --- Constructor & Serialization ---

    DatasetSnapshotManifest() : created_at(std::chrono::system_clock::now()),
                                last_modified(std::chrono::system_clock::now()) {}

    /**
     * @brief Serialize manifest to JSON string.
     * @return JSON representation of the full manifest.
     */
    std::string toJSON() const;

    /**
     * @brief Serialize manifest to YAML string.
     * @return YAML representation of the manifest.
     */
    std::string toYAML() const;

    /**
     * @brief Deserialize manifest from JSON string.
     * @param json_str JSON representation of the manifest.
     * @return Deserialized manifest object.
     * @throws std::runtime_error on parse errors.
     */
    static DatasetSnapshotManifest fromJSON(const std::string& json_str);

    /**
     * @brief Deserialize manifest from YAML string.
     * @param yaml_str YAML representation of the manifest.
     * @return Deserialized manifest object.
     * @throws std::runtime_error on parse errors.
     */
    static DatasetSnapshotManifest fromYAML(const std::string& yaml_str);

    /**
     * @brief Save manifest to file (JSON format).
     * @param file_path Path where manifest will be saved.
     * @return true if successful, false otherwise.
     */
    bool saveToFile(const std::string& file_path) const;

    /**
     * @brief Load manifest from file (JSON format).
     * @param file_path Path to manifest file.
     * @return Loaded manifest object.
     * @throws std::runtime_error if file cannot be read or parsed.
     */
    static DatasetSnapshotManifest loadFromFile(const std::string& file_path);

    /**
     * @brief Compute and verify integrity checksum.
     * @return true if content checksum matches recomputed value.
     */
    bool verifyIntegrity() const;

    /**
     * @brief Recompute content checksum based on current manifest state.
     * Updates the content_checksum field.
     */
    void updateChecksum();

    /**
     * @brief Get split statistics as a human-readable string.
     */
    std::string getSplitStatistics() const;

    /**
     * @brief Get domain distribution as a human-readable string.
     */
    std::string getDomainStatistics() const;
};

} // namespace training
} // namespace themis
