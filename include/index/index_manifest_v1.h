/**
 * @file index_manifest_v1.h
 * @brief Index Manifest v1.0 - RocksDB Persistence Schema
 *
 * Defines the JSON schema for storing index metadata, version history, and
 * reindex tracking information in RocksDB. Implements the manifest structure
 * from EMBEDDING_VERSION_GOVERNANCE.md §Index Manifest Schema.
 *
 * **Storage Layout:**
 * - Column family: "default", Key: "index_version", Value: Full manifest JSON
 * - Column family: "version_history", Key: version_number (uint32 BE), Value: VersionEntry JSON
 * - Column family: "embeddings", Key: sha256(doc_id + content), Value: embedding JSON
 *
 * @see src/ingestion/EMBEDDING_VERSION_GOVERNANCE.md
 * @date 2026-09-24
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <ctime>
#include <nlohmann/json.hpp>

namespace themis::index {

/**
 * @struct IndexVersionInfo
 * @brief Current version information for an index
 *
 * Tracks the active index version number, timestamps, document count,
 * and the embedding/chunking/schema metadata for this version.
 */
struct IndexVersionInfo {
  uint32_t version_number = 0;         ///< Monotonically increasing version counter
  std::string timestamp;               ///< ISO 8601 creation timestamp
  std::string embedding_model_id;      ///< Model ID for current version
  uint32_t embedding_dim = 0;          ///< Embedding dimensionality
  std::string chunking_profile_id;     ///< Chunking profile in use
  std::string schema_version = "2.0";  ///< Index schema version
  uint64_t document_count = 0;         ///< Number of documents in index
  uint64_t index_size_bytes = 0;       ///< Approximate index size
  std::string last_indexed_at;         ///< Last write timestamp

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized IndexVersionInfo.
  static IndexVersionInfo from_json(const nlohmann::json& j);
};

/**
 * @struct VersionHistoryEntry
 * @brief Historical record of a single index version
 *
 * Stored in version_history column family for audit trail and rollback support.
 * Tracks what changed and why (reindex reason, incremental update, etc.).
 */
struct VersionHistoryEntry {
  uint32_t version_number = 0;
  std::string timestamp;
  std::string embedding_model_id;
  std::string chunking_profile_id;
  std::string schema_version = "2.0";
  std::string action;  ///< "incremental_update", "reindex_complete", etc.
  std::string reason;  ///< Why this version was created
  uint64_t documents_added = 0;
  uint64_t documents_removed = 0;

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized VersionHistoryEntry.
  static VersionHistoryEntry from_json(const nlohmann::json& j);
};

/**
 * @struct ReindexTrackingInfo
 * @brief Tracks the most recent reindex operation
 *
 * Captures reindex reason, timing, status, and rollback availability
 * for operational dashboards and troubleshooting.
 */
struct ReindexTrackingInfo {
  std::string last_reindex_reason;         ///< Why: "embedding_model_upgrade", etc.
  std::string last_reindex_start;          ///< ISO 8601 start timestamp
  std::string last_reindex_end;            ///< ISO 8601 end timestamp
  std::string last_reindex_status;         ///< "success", "in_progress", "failed"
  bool atomic_rollback_available = true;   ///< Can rollback to previous version?
  bool previous_version_accessible = true; ///< Is previous version still stored?
  std::string error_message;               ///< Error details if status = "failed"

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized ReindexTrackingInfo.
  static ReindexTrackingInfo from_json(const nlohmann::json& j);
};

/**
 * @struct IndexManifestGovernance
 * @brief Governance metadata for compliance and auditing
 *
 * Tracks which specification version controls this manifest,
 * audit trail enablement, and feature flags.
 */
struct IndexManifestGovernance {
  std::string contract_version;        ///< e.g., "EMBEDDING_VERSION_GOVERNANCE.md (2026-09-24)"
  bool audit_trail_enabled = true;     ///< Whether all operations are logged
  bool canary_deployments_enabled = false; ///< Whether canary phases are active

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized IndexManifestGovernance.
  static IndexManifestGovernance from_json(const nlohmann::json& j);
};

/**
 * @class IndexManifestV1
 * @brief Complete index manifest for RocksDB persistence
 *
 * Stores and retrieves the full index metadata including:
 * - Current version information (what's active now)
 * - Version history (for rollback decisions)
 * - Reindex tracking (for operational monitoring)
 * - Governance metadata (for compliance)
 *
 * **Serialization Format:** JSON (stored as string in RocksDB)
 * **Storage Key:** "index_version" in column family "default"
 *
 * **Thread Safety:** Not thread-safe; caller must synchronize access.
 *
 * **Example:**
 * ```cpp
 * IndexManifestV1 manifest;
 * manifest.index_id = "wiki-index-main";
 * manifest.current_version.version_number = 42;
 * manifest.current_version.embedding_model_id = "all-minilm-l6-v2-1.0";
 * // ... populate fields ...
 *
 * // Serialize to JSON
 * nlohmann::json manifest_json = manifest.to_json();
 *
 * // Store in RocksDB
 * rocksdb::Status status = db->Put(rocksdb::WriteOptions(),
 *     "index_version", manifest_json.dump());
 * ```
 */
class IndexManifestV1 {
 public:
  IndexManifestV1() = default;

  // Manifest identity
  std::string manifest_version = "1.0";  ///< Manifest schema version
  std::string index_id;                  ///< Unique index identifier
  std::string created_at;                ///< When index was first created
  std::string last_updated_at;           ///< When manifest was last updated

  // Current state
  IndexVersionInfo current_version;

  // Historical tracking
  std::vector<VersionHistoryEntry> version_history;  ///< Up to 30 entries, FIFO
  static constexpr size_t MAX_HISTORY_SIZE = 30;

  // Reindex state
  ReindexTrackingInfo reindex_tracking;

  // Governance
  IndexManifestGovernance governance;

  /**
   * @brief Serialize manifest to JSON
   * @return JSON object containing full manifest
   */
  nlohmann::json to_json() const;

  /**
   * @brief Deserialize manifest from JSON
   * @param j JSON object to parse
   * @return Reconstructed manifest
   */
  static IndexManifestV1 from_json(const nlohmann::json& j);

  /**
   * @brief Create JSON string suitable for RocksDB storage
   * @return Formatted JSON string (minified, no extra whitespace)
   */
  std::string to_rocksdb_value() const;

  /**
   * @brief Load manifest from RocksDB-stored JSON string
   * @param rocksdb_value JSON string from RocksDB
   * @return Reconstructed manifest
   */
  static IndexManifestV1 from_rocksdb_value(const std::string& rocksdb_value);

  /**
   * @brief Add a new entry to version history
   *
   * Automatically maintains FIFO rotation (drops oldest if exceeds MAX_HISTORY_SIZE).
   * Updates last_updated_at timestamp.
   *
   * @param entry New history entry to add
   */
  void add_version_history(const VersionHistoryEntry& entry);

  /**
   * @brief Get the previous version number (for rollback decisions)
   * @return Previous version number, or 0 if no history exists
   */
  uint32_t get_previous_version_number() const;

  /**
   * @brief Check if atomic rollback to previous version is available
   * @return true if rollback is safe, false otherwise
   */
  bool can_rollback_to_previous() const;
};

}  // namespace themis::index
