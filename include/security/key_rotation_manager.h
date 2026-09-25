/**
 * @file key_rotation_manager.h
 * @brief Encryption key rotation for versioned archived chunks
 *
 * Manages transparent encryption key rotation for archived index chunks.
 * Supports progressive key re-encryption without interrupting queries.
 *
 * **Key Rotation Process:**
 *
 * 1. **Trigger Key Rotation**
 *    - `StartKeyRotation()` creates new key derivation with CMAC-tag
 *    - Stores rotation intent + metadata
 *
 * 2. **Migrate Archived Chunks**
 *    - `MigrateChunk()` re-encrypts chunk with new key
 *    - Stores mapping of old_key_tag → new_key_tag in metadata
 *    - Old key kept in rotation_history until cleanup grace period
 *
 * 3. **Dual-Key Validation Period**
 *    - Queries compute both old and new key
 *    - Compare decryption results (both must match for CMAC consistency)
 *    - Grace period allows rollback if mismatches detected
 *
 * 4. **Cleanup Old Keys**
 *    - `FinalizeKeyRotation()` after validation window
 *    - Deletes old keys from rotation_history
 *    - Archives CMAC audit trail
 *
 * **Storage Layout:**
 * ```
 * RocksDB {
 *   "default" CF:
 *     "active_key_tag"        → current CMAC tag
 *     "key_rotation_state"    → RotationState JSON
 *   "key_metadata" CF:
 *     "key:<tag>"             → KeyMetadata JSON (derivation, salt, created_at)
 *   "chunk_mappings" CF:
 *     uint64_be(chunk_id)     → RotationMapping JSON (old_tag, new_tag, migrated_at)
 *   "rotation_history" CF:
 *     uint64_be(timestamp)    → HistoryEntry JSON (tag, action, details)
 * }
 * ```
 *
 * @date 2026-09-24
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>

namespace rocksdb {
class DB;
class ColumnFamilyHandle;
}

namespace themis::security {

/**
 * @struct KeyMetadata
 * @brief Encryption key derivation metadata
 */
struct KeyMetadata {
  std::string key_tag;  // CMAC tag for this key version
  std::string salt;     // KDF salt
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point expires_at;
  bool active{true};

  /**
   * @brief Derive encryption key from master key + metadata
   * @param master_key Master key material
   * @return Derived key (32 bytes)
   */
  std::string DeriveKey(const std::string& master_key) const;

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all KeyMetadata fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized KeyMetadata.
  static KeyMetadata from_json(const nlohmann::json& j);
};

/**
 * @struct RotationMapping
 * @brief Chunk migration record during key rotation
 */
struct RotationMapping {
  uint64_t chunk_id{0};
  std::string old_key_tag;
  std::string new_key_tag;
  std::chrono::system_clock::time_point migrated_at;
  bool validated{false};

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all RotationMapping fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized RotationMapping.
  static RotationMapping from_json(const nlohmann::json& j);
};

/**
 * @enum RotationPhase
 * @brief Stages of key rotation process
 */
enum class RotationPhase {
  Idle = 0,              // No rotation in progress
  InProgress = 1,        // New keys generated, migration starting
  DualKeyValidation = 2, // Both old/new keys active, validating
  Cleanup = 3,           // Old keys expired, cleanup scheduled
  Complete = 4,          // Rotation finished, history archived
};

/**
 * @struct RotationState
 * @brief Current state of key rotation process
 */
struct RotationState {
  RotationPhase phase{RotationPhase::Idle};
  uint32_t rotation_id{0};
  std::string old_key_tag;
  std::string new_key_tag;
  uint64_t chunks_migrated{0};
  uint64_t total_chunks{0};
  std::chrono::system_clock::time_point started_at;
  std::chrono::system_clock::time_point validation_deadline;

  /// @brief Serialize this struct to JSON.
  /// @return JSON object with all RotationState fields.
  nlohmann::json to_json() const;
  /// @brief Deserialize from JSON.
  /// @param j JSON object to parse.
  /// @return Deserialized RotationState.
  static RotationState from_json(const nlohmann::json& j);
};

/**
 * @class KeyRotationManager
 * @brief Manages encryption key rotation lifecycle
 *
 * Handles transparent key rotation for archived chunks with
 * atomic semantics and validation windows.
 */
class KeyRotationManager {
 public:
  /**
   * @brief Open or create key rotation manager
   * @param db_path Path to RocksDB directory
   * @return Initialized manager
   */
  static std::unique_ptr<KeyRotationManager> Open(const std::string& db_path);

  ~KeyRotationManager();

  // Delete copy; move allowed
  KeyRotationManager(const KeyRotationManager&) = delete;
  KeyRotationManager& operator=(const KeyRotationManager&) = delete;
  KeyRotationManager(KeyRotationManager&&) = default;
  KeyRotationManager& operator=(KeyRotationManager&&) = default;

  /**
   * @brief Get current active key tag
   * @return Active key CMAC tag
   */
  std::string GetActiveKeyTag() const;

  /**
   * @brief Get key metadata
   * @param key_tag Key CMAC tag
   * @return KeyMetadata, or empty if not found
   */
  std::optional<KeyMetadata> GetKeyMetadata(const std::string& key_tag) const;

  /**
   * @brief Start new key rotation
   * @param master_key Master key for KDF
   * @return New rotation ID
   */
  uint32_t StartKeyRotation(const std::string& master_key);

  /**
   * @brief Get current rotation state
   * @return RotationState
   */
  RotationState GetRotationState() const;

  /**
   * @brief Migrate chunk with new key (re-encrypt)
   * @param chunk_id Chunk identifier
   * @param master_key Master key for KDF
   * @return Success
   */
  bool MigrateChunk(uint64_t chunk_id, const std::string& master_key);

  /**
   * @brief Get rotation mapping for chunk
   * @param chunk_id Chunk identifier
   * @return Mapping (empty if not migrated)
   */
  std::optional<RotationMapping> GetChunkMapping(uint64_t chunk_id) const;

  /**
   * @brief Check if chunk needs dual-key validation
   * @param chunk_id Chunk identifier
   * @return True if in validation window
   */
  bool IsInValidationWindow(uint64_t chunk_id) const;

  /**
   * @brief Mark chunk as validated (decryption consistent)
   * @param chunk_id Chunk identifier
   * @return Success
   */
  bool ValidateChunk(uint64_t chunk_id);

  /**
   * @brief Finalize rotation (cleanup grace period expired)
   * @return Success
   */
  bool FinalizeKeyRotation();

  /**
   * @brief Rollback rotation to previous state
   * @return Success
   */
  bool RollbackKeyRotation();

  /**
   * @brief Get rotation history entries
   * @param max_count Max entries to return
   * @return History records
   */
  std::vector<nlohmann::json> GetRotationHistory(size_t max_count = 100) const;

 private:
  KeyRotationManager(rocksdb::DB* db);

  std::unique_ptr<rocksdb::DB> db_;
  rocksdb::ColumnFamilyHandle* cf_default_{nullptr};
  rocksdb::ColumnFamilyHandle* cf_key_metadata_{nullptr};
  rocksdb::ColumnFamilyHandle* cf_chunk_mappings_{nullptr};
  rocksdb::ColumnFamilyHandle* cf_rotation_history_{nullptr};

  /**
   * @brief Generate CMAC tag for new key version
   * @param key_material Raw key material
   * @param salt Salt for tagging
   * @return CMAC hex tag
   */
  static std::string GenerateKeyTag(const std::string& key_material,
                                     const std::string& salt);

  /**
   * @brief Encode chunk ID as big-endian
   * @param chunk_id Chunk ID
   * @return Encoded key
   */
  static std::string EncodeChunkId(uint64_t chunk_id);

  /**
   * @brief Decode chunk ID from bytes
   * @param encoded Encoded key
   * @return Chunk ID
   */
  static uint64_t DecodeChunkId(const std::string& encoded);
};

}  // namespace themis::security
