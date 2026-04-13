/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            training_interfaces.h                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:21:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     499                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1082cc00f9  2026-03-20  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file training_interfaces.h
 * @brief Abstract interfaces for the training module.
 *
 * Defines the pure-virtual contracts used by the orchestration layer,
 * test doubles, and future service implementations:
 *
 *   - ILoRACheckpointManager  – content-addressed checkpoint save/load/verify
 *   - ISampleProvenanceTracker – append-only sample lineage tracking
 *   - IKGEnrichmentInterface  – cached knowledge-graph enrichment
 *   - IConfidenceCalibrator   – threshold-only calibration (no weight mutation)
 *   - ITrainingPipeline       – fully async training job orchestration
 *   - ILineageQueryAPI        – read-only sample-to-model lineage traversal
 */

#include "training/provenance_tracker.h"

#include <string>
#include <vector>
#include <future>
#include <map>
#include <cstddef>
#include <ctime>
#include <stdexcept>

namespace themis {
namespace training {

// ============================================================================
// Common value types
// ============================================================================

/** @brief Opaque identifier returned by ILoRACheckpointManager::save(). */
using CheckpointId = std::string;

/** @brief Opaque identifier for a submitted training job. */
using JobId = std::string;

/** @brief Per-layer LoRA weight matrices exported for checkpoint storage. */
struct LoRAWeights {
    std::string model_id;                         ///< Base model identifier
    std::map<std::string, std::vector<float>> A;  ///< A matrices keyed by layer name
    std::map<std::string, std::vector<float>> B;  ///< B matrices keyed by layer name
    int rank = 0;
    float alpha = 1.0f;

    LoRAWeights() = default;
};

/** @brief Descriptor for a single stored checkpoint. */
struct CheckpointDescriptor {
    CheckpointId id;           ///< Content-addressed identifier (SHA-256 hex)
    std::string  model_id;     ///< Base model the weights belong to
    std::time_t  created_at = 0;
    size_t       size_bytes = 0;
    bool         signature_valid = false; ///< Ed25519 signature verified

    CheckpointDescriptor() = default;
};

/** @brief Preprocessing step recorded in a provenance entry. */
struct PreprocessingStep {
    std::string name;        ///< Step identifier (e.g., "tokenise", "dedup")
    std::string version;     ///< Component version string
    std::string parameters;  ///< Serialised step parameters (JSON or key=value)

    PreprocessingStep() = default;
};

/** @brief Immutable provenance record for a single training sample (interface view). */
struct SampleProvenance {
    std::string sample_id;                            ///< Stable opaque sample key
    std::string source_uri;                           ///< Opaque source URI (no raw content)
    std::time_t collected_at = 0;                     ///< Unix timestamp
    std::vector<PreprocessingStep> preprocessing_steps;
    std::string dataset_version;                      ///< Dataset snapshot identifier

    SampleProvenance() = default;
};

/** @brief Directed acyclic graph of sample transformations. */
struct LineageGraph {
    LineageNode root;  ///< Starting node (the training-ready sample); reuses LineageNode from provenance_tracker.h
    size_t      depth = 0;

    LineageGraph() = default;
};

/** @brief Cache hit/miss statistics for IKGEnrichmentInterface. */
struct CacheStats {
    size_t hits      = 0;
    size_t misses    = 0;
    size_t evictions = 0;
    size_t size      = 0;  ///< Current number of cached entries

    CacheStats() = default;
};

/** @brief Reference to a knowledge-graph entity used as an enrichment query key. */
struct EntityRef {
    std::string entity_key;           ///< Primary entity identifier
    std::string graph_schema_version; ///< Graph schema version at query time

    EntityRef() = default;
    explicit EntityRef(const std::string& key,
                       const std::string& schema_version = "")
        : entity_key(key), graph_schema_version(schema_version) {}
};

/** @brief A single relation returned by knowledge-graph enrichment. */
struct KGRelation {
    std::string relation_type;
    std::string target_entity_key;
    float       weight = 1.0f;

    KGRelation() = default;
};

/** @brief Result of enriching one entity with knowledge-graph context. */
struct EnrichmentResult {
    std::string              entity_id;
    std::vector<KGRelation>  relations;
    float                    confidence = 0.0f;
    bool                     cache_hit  = false;

    EnrichmentResult() = default;
};

/** @brief Per-category calibrated threshold. */
using ThresholdMap = std::map<std::string, float>;

/** @brief A labelled sample used as calibration input. */
struct CalibrationSample {
    std::string category;
    float       confidence = 0.0f;
    bool        model_correct = false;

    CalibrationSample() = default;
};

/** @brief Dataset fed into IConfidenceCalibrator::calibrate(). */
struct CalibrationDataset {
    std::vector<CalibrationSample> samples;

    CalibrationDataset() = default;
};

/** @brief Result returned by IConfidenceCalibrator::calibrate(). */
struct CalibratorOutput {
    ThresholdMap updated_thresholds;
    double       calibration_error = 0.0;  ///< Mean squared error of isotonic fit
    size_t       samples_used      = 0;
    bool         success           = false;
    std::string  summary;

    CalibratorOutput() = default;
};

/** @brief Input specification for a single training job. */
struct TrainingJob {
    std::string  dataset_ref;     ///< Collection / snapshot identifier
    std::string  model_config;    ///< Serialised model configuration (JSON)
    std::string  lora_config;     ///< Optional serialised LoRA config (JSON)
    std::string  job_id;          ///< Optional caller-provided job ID; auto-assigned if empty

    TrainingJob() = default;
};

/** @brief Result returned when a training job completes. */
struct AsyncTrainingResult {
    bool        success = false;
    std::string job_id;
    double      training_loss     = 0.0;
    double      validation_loss   = 0.0;
    double      accuracy          = 0.0;
    std::string adapter_version;
    std::string error_message;

    AsyncTrainingResult() = default;
};

/** @brief Lifecycle state of a submitted training job. */
enum class JobStatus {
    Queued,
    Running,
    Completed,
    Failed,
    Cancelled
};

/** @brief Result of a cancel() call. */
enum class CancelResult {
    Cancelled,
    AlreadyCompleted,
    NotFound
};

/** @brief Reference to a training sample used in lineage queries. */
struct SampleRef {
    std::string sample_id;
    std::string source_uri;
    std::string dataset_version;

    SampleRef() = default;
};

/** @brief Origin record for a training sample. */
struct OriginRecord {
    std::string sample_id;
    std::string source_uri;
    std::time_t collected_at = 0;
    std::string dataset_version;
    bool        found = false;  ///< false when the sample is unknown

    OriginRecord() = default;
};

// ============================================================================
// ILoRACheckpointManager
// ============================================================================

/**
 * @brief Content-addressed LoRA checkpoint storage interface.
 *
 * Implementors must guarantee:
 *  - save() returns a deterministic SHA-256-derived CheckpointId.
 *  - load() is async to support large checkpoints from remote stores.
 *  - verify() re-derives the content hash; returns false for tampered files.
 *  - listCheckpoints() exposes the Ed25519 signature validity flag.
 *
 * @see LoRACheckpointManager (concrete implementation)
 */
class ILoRACheckpointManager {
public:
    virtual ~ILoRACheckpointManager() = default;

    /**
     * @brief Persist LoRA weights and return their content-addressed ID.
     * @param weights  Adapter weights to store.
     * @return SHA-256-derived checkpoint identifier.
     * @throws std::runtime_error on I/O failure.
     */
    virtual CheckpointId save(const LoRAWeights& weights) = 0;

    /**
     * @brief Asynchronously load adapter weights by their content ID.
     * @param checkpoint_id  Identifier returned by save().
     * @return Future resolving to the loaded weights.
     */
    virtual std::future<LoRAWeights> load(const CheckpointId& checkpoint_id) = 0;

    /**
     * @brief Re-derive the content hash and confirm integrity.
     * @param checkpoint_id  Identifier to validate.
     * @return true if the stored file matches its SHA-256 digest.
     */
    virtual bool verify(const CheckpointId& checkpoint_id) const = 0;

    /**
     * @brief List all checkpoints associated with a base model.
     * @param model_id  Base model identifier.
     * @return Descriptors sorted newest-first.
     */
    virtual std::vector<CheckpointDescriptor> listCheckpoints(
        const std::string& model_id) const = 0;
};

// ============================================================================
// ISampleProvenanceTracker
// ============================================================================

/**
 * @brief Append-only sample lineage tracker interface.
 *
 * Design constraints:
 *  - record() is append-only; no update or delete path exists.
 *  - Only opaque source URIs are stored; raw training content must not appear
 *    in provenance records.
 *  - queryLineage() returns the full transformation DAG, not raw content.
 *
 * @see ProvenanceTracker (concrete implementation)
 */
class ISampleProvenanceTracker {
public:
    virtual ~ISampleProvenanceTracker() = default;

    /**
     * @brief Append a provenance record.  Cannot overwrite an existing record.
     * @param provenance  Provenance data to store.
     * @throws std::invalid_argument if sample_id is already recorded.
     */
    virtual void record(const SampleProvenance& provenance) = 0;

    /**
     * @brief Retrieve the transformation DAG for a sample.
     * @param sample_id  Sample to trace.
     * @return Lineage graph rooted at the training-ready sample.
     */
    virtual LineageGraph queryLineage(const std::string& sample_id) const = 0;

    /**
     * @brief Total number of provenance records stored.
     */
    virtual size_t totalRecords() const = 0;

    /**
     * @brief Estimated storage footprint in bytes.
     */
    virtual size_t storageEstimateBytes() const = 0;
};

// ============================================================================
// IKGEnrichmentInterface
// ============================================================================

/**
 * @brief Cached knowledge-graph enrichment interface.
 *
 * Design constraints:
 *  - enrich() is read-only; it must never issue AQL write operations.
 *  - Cache key is derived deterministically from EntityRef fields.
 *  - Cache entries have TTL enforcement; stale data is evicted before use.
 *
 * @see KnowledgeGraphEnricher (concrete implementation)
 */
class IKGEnrichmentInterface {
public:
    virtual ~IKGEnrichmentInterface() = default;

    /**
     * @brief Enrich an entity, serving from cache when available.
     * @param entity  Entity reference identifying the enrichment query.
     * @return Enrichment result; result.cache_hit is true on cache hit.
     */
    virtual EnrichmentResult enrich(const EntityRef& entity) = 0;

    /**
     * @brief Invalidate the cached result for a specific entity.
     * @param entity  Entity whose cache entry should be evicted.
     */
    virtual void invalidateCache(const EntityRef& entity) = 0;

    /**
     * @brief Evict all cached enrichment results.
     */
    virtual void clearCache() = 0;

    /**
     * @brief Return hit/miss/eviction counters.
     */
    virtual CacheStats cacheStats() const = 0;
};

// ============================================================================
// IConfidenceCalibrator
// ============================================================================

/**
 * @brief Threshold-only confidence calibration interface.
 *
 * Design constraints:
 *  - calibrate() operates exclusively on threshold scalars.
 *  - It must never access gradient state, weight tensors, or optimizer state.
 *  - applyThresholds() updates only the internal threshold store.
 *  - resetToDefaults() restores factory thresholds without touching any model.
 *
 * @see ConfidenceCalibrator (concrete implementation)
 */
class IConfidenceCalibrator {
public:
    virtual ~IConfidenceCalibrator() = default;

    /**
     * @brief Asynchronously compute calibrated thresholds.
     * @param dataset  Labelled (confidence, correct) pairs.
     * @return Future resolving to the calibration output.
     */
    virtual std::future<CalibratorOutput> calibrate(
        const CalibrationDataset& dataset) = 0;

    /**
     * @brief Return the currently active threshold map.
     */
    virtual const ThresholdMap& currentThresholds() const = 0;

    /**
     * @brief Replace the active threshold map.
     * @param thresholds  New per-category thresholds to apply.
     */
    virtual void applyThresholds(const ThresholdMap& thresholds) = 0;

    /**
     * @brief Reset all thresholds to their factory defaults (0.5 per category).
     */
    virtual void resetToDefaults() = 0;
};

// ============================================================================
// ITrainingPipeline
// ============================================================================

/**
 * @brief Fully-async training job orchestration interface.
 *
 * Design constraints:
 *  - submit() returns a future immediately; no blocking calls on the public API.
 *  - cancel() is best-effort; returns AlreadyCompleted if the job has finished.
 *  - status() is safe to call from any thread.
 *
 * @see TrainingPipeline (concrete implementation)
 */
class ITrainingPipeline {
public:
    virtual ~ITrainingPipeline() = default;

    /**
     * @brief Submit a training job for asynchronous execution.
     * @param job  Job specification (dataset, model config, optional LoRA config).
     * @return Future resolving to the training result.
     */
    virtual std::future<AsyncTrainingResult> submit(const TrainingJob& job) = 0;

    /**
     * @brief Request cancellation of a running or queued job.
     * @param job_id  Identifier returned by submit (or job.job_id if set).
     * @return Cancelled, AlreadyCompleted, or NotFound.
     */
    virtual CancelResult cancel(const JobId& job_id) = 0;

    /**
     * @brief Query the current lifecycle state of a job.
     * @param job_id  Job identifier.
     * @return Current JobStatus.
     */
    virtual JobStatus status(const JobId& job_id) const = 0;
};

// ============================================================================
// ILineageQueryAPI
// ============================================================================

/**
 * @brief Read-only sample-to-model lineage query interface.
 *
 * Design constraint: no write methods exist on this interface.
 * Unauthorized access must throw std::runtime_error with a descriptive message.
 *
 * @see ProvenanceTracker::queryLineage (concrete traversal)
 */
class ILineageQueryAPI {
public:
    virtual ~ILineageQueryAPI() = default;

    /**
     * @brief Return all training samples that contributed to a deployed model.
     * @param model_id  Adapter version or model identifier.
     * @return Sample references sorted by contribution weight (descending).
     */
    virtual std::vector<SampleRef> getProvenance(
        const std::string& model_id) const = 0;

    /**
     * @brief Return the origin record for a specific training sample.
     * @param sample_id  Sample key.
     * @return Origin record; result.found is false when the sample is unknown.
     */
    virtual OriginRecord getSampleOrigin(
        const std::string& sample_id) const = 0;
};

} // namespace training
} // namespace themis
