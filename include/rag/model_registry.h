/**
 * @file model_registry.h
 * @brief Model versioning and lifecycle management for RAG Phase 11
 *
 * Central model registry tracking trained model versions, metadata, and
 * deployment status. Enables model governance, rollback, and A/B testing.
 *
 * @version 0.1.0
 * @note Phase: 11 (Retraining Automation & Orchestration)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag::lifecycle {

/**
 * @brief Model deployment status lifecycle
 *
 * Models transition through states: draft → validated → candidate → deployed → retired
 */
enum class ModelStatus {
  kDraft,      ///< Model trained, not yet validated
  kValidated,  ///< Statistical validation passed
  kCandidate,  ///< Approved for deployment, awaiting traffic
  kDeployed,   ///< Currently serving production traffic
  kRetired,    ///< No longer in use, archived
  kFailed      ///< Validation or deployment failed
};

/**
 * @brief Convert ModelStatus to string representation
 */
std::string StatusToString(ModelStatus status);

/**
 * @brief Parse string to ModelStatus
 */
std::optional<ModelStatus> StringToStatus(const std::string& status_str);

/**
 * @brief Model metadata and versioning information
 */
struct ModelMetadata {
  /// Unique version identifier (auto-incremented, starting at 1)
  uint32_t version;

  /// Human-readable model identifier (e.g., "cost-v2", "embedding-v3")
  std::string model_id;

  /// Timestamp when model was trained (microseconds since epoch)
  uint64_t built_at_us;

  /// Timestamp when model status last changed
  uint64_t status_changed_at_us;

  /// Current deployment status
  ModelStatus status = ModelStatus::kDraft;

  /// Parent model version (for lineage tracking); 0 if root
  uint32_t parent_version = 0;

  /// Training dataset identifier or checksum
  std::string training_dataset_id;

  /// Performance metrics (JSON encoded)
  std::string metrics_json;  // {"ndcg@10": 0.75, "recall@10": 0.82, ...}

  /// Cost statistics (JSON encoded)
  std::string cost_stats_json;  // {"mean_latency_ms": 42.5, "p95_latency_ms": 150.0, ...}

  /// Notes on this version (deployment blockers, validation issues, etc.)
  std::string notes;

  /// Model serialization checksum for integrity verification
  std::string model_checksum;

  /// Persisted model path or S3 URI
  std::string model_location;
};

/**
 * @brief Model Registry — Central model versioning and lifecycle management
 *
 * Tracks all trained model versions with metadata, status transitions,
 * and audit trails. Supports querying, state transitions, and persistence.
 *
 * Thread-safe for concurrent access via internal locking.
 */
class ModelRegistry {
 public:
  /**
   * @brief Constructor
   *
   * @param persistence_path Path to store model registry (JSON or SQLite)
   */
  explicit ModelRegistry(const std::string& persistence_path);
  ~ModelRegistry();

  /**
   * @brief Register a newly trained model
   *
   * Creates a new model version in draft status. Model must pass validation
   * before it can be deployed.
   *
   * @param model_id Human-readable model identifier
   * @param training_dataset_id ID of dataset used for training
   * @param metrics_json Performance metrics in JSON format
   * @param cost_stats_json Cost and latency statistics in JSON format
   * @param model_location Path or URI where trained model is stored
   * @param parent_version Previous version (for lineage tracking), 0 if root
   * @return Newly assigned version number
   */
  uint32_t RegisterModel(const std::string& model_id,
                         const std::string& training_dataset_id,
                         const std::string& metrics_json,
                         const std::string& cost_stats_json,
                         const std::string& model_location,
                         uint32_t parent_version = 0);

  /**
   * @brief Update model status
   *
   * Transitions model through lifecycle states: draft → validated → candidate →
   * deployed → retired. Automatically records status change timestamp.
   *
   * @param version Model version to update
   * @param new_status Target status
   * @param notes Optional notes on state transition (validation results, deployment blockers)
   * @return true if transition succeeded, false if invalid transition
   */
  bool UpdateModelStatus(uint32_t version, ModelStatus new_status,
                        const std::string& notes = "");

  /**
   * @brief Get the latest deployed model
   *
   * Returns metadata for the model currently serving production traffic,
   * or std::nullopt if no deployed model exists.
   *
   * @return Latest deployed model metadata, or std::nullopt
   */
  std::optional<ModelMetadata> GetDeployedModel() const;

  /**
   * @brief Get model by version number
   *
   * @param version Target version
   * @return Model metadata if version exists, std::nullopt otherwise
   */
  std::optional<ModelMetadata> GetByVersion(uint32_t version) const;

  /**
   * @brief Get latest model (any status)
   *
   * Returns the most recently created model regardless of deployment status.
   *
   * @return Latest model metadata
   */
  std::optional<ModelMetadata> GetLatest() const;

  /**
   * @brief Get all models in a given status
   *
   * @param status Target status to filter by
   * @return List of matching model metadata
   */
  std::vector<ModelMetadata> GetByStatus(ModelStatus status) const;

  /**
   * @brief Get model lineage (ancestry chain)
   *
   * Traces parent versions back to the root model.
   *
   * @param version Starting version
   * @return List of metadata from given version back to root, newest first
   */
  std::vector<ModelMetadata> GetLineage(uint32_t version) const;

  /**
   * @brief Check if model exists
   *
   * @param version Model version
   * @return true if model registered, false otherwise
   */
  bool Exists(uint32_t version) const;

  /**
   * @brief Get total number of registered models
   * @return Count of models stored in this registry.
   */
  uint32_t Count() const;

  /**
   * @brief Persist registry to disk (JSON or SQLite backend)
   *
   * Automatically called on model registration, but can be invoked manually
   * for safety if needed.
   *
   * @return true if persistence succeeded, false on I/O error
   */
  bool Persist() const;

  /**
   * @brief Load registry from disk
   *
   * Called automatically in constructor. Can be used to reload if external
   * changes to persistence file are expected.
   *
   * @return true if load succeeded, false on I/O error or corrupt file
   */
  bool Load();

  /**
   * @brief Get registry as JSON for export/reporting
   *
   * @return JSON string representation of entire registry
   */
  std::string ToJson() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::lifecycle
