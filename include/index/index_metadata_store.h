/**
 * @file index_metadata_store.h
 * @brief RocksDB-backed persistent storage for index manifests
 *
 * Provides atomic version management, history rotation, and rollback
 * tracking for embedding versioning governance.
 *
 * **Column Families:**
 * - "default": Key "index_version" → IndexManifestV1 JSON
 * - "version_history": Key version_number (uint32 BE) → VersionHistoryEntry JSON
 * - "embeddings": Key sha256(doc_id + content) → embedding vector JSON
 *
 * **Storage Layout:**
 * ```
 * RocksDB {
 *   "default" CF:
 *     "index_version" → IndexManifestV1 manifest JSON (minified)
 *   "version_history" CF:
 *     uint32_be(40) → VersionHistoryEntry{version_number:40, ...}
 *     uint32_be(41) → VersionHistoryEntry{version_number:41, ...}
 *   "embeddings" CF:
 *     sha256_hex(doc_id+content) → {"vector": [...], "metadata": {...}}
 * }
 * ```
 *
 * @date 2026-09-24
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace rocksdb {
class DB;
class ColumnFamilyHandle;
}

namespace themis::index {

class IndexVersionInfo;
class VersionHistoryEntry;
class IndexManifestV1;

/**
 * @class IndexMetadataStore
 * @brief Persistent storage for index manifest and version history
 *
 * Manages RocksDB column families for:
 * - Current manifest state (atomic updates)
 * - Version history (append-only, rotated at MAX_HISTORY_SIZE)
 * - Embedding metadata (keyed by content SHA256)
 *
 * **Thread Safety:** All operations are serialized on a single writer thread.
 * Concurrent readers may access the same DB instance.
 */
class IndexMetadataStore {
 public:
  /**
   * @brief Open or create an IndexMetadataStore
   *
   * @param db_path Path to RocksDB directory
   * @param index_id Unique identifier for this index (used for logging)
   * @return Initialized store, ready for read/write operations
   * @throws rocksdb::Status on DB open failure
   */
  static std::unique_ptr<IndexMetadataStore> Open(const std::string& db_path,
                                                    const std::string& index_id);

  /**
   * @brief Close the underlying RocksDB instance
   */
  ~IndexMetadataStore();

  // Delete copy operations; move is allowed for future async I/O
  IndexMetadataStore(const IndexMetadataStore&) = delete;
  IndexMetadataStore& operator=(const IndexMetadataStore&) = delete;
  IndexMetadataStore(IndexMetadataStore&&) = default;
  IndexMetadataStore& operator=(IndexMetadataStore&&) = default;

  /**
   * @brief Load current manifest from RocksDB
   *
   * @return Current manifest state, or empty manifest if not yet written
   * @throws rocksdb::Status on read failure
   */
  IndexManifestV1 LoadManifest() const;

  /**
   * @brief Write manifest and add history entry atomically
   *
   * Updates the manifest and appends to version history in a single transaction.
   * **Atomicity:** Both operations complete or both roll back; no partial state.
   *
   * @param manifest New manifest to store
   * @param history_entry Entry to append to version_history CF
   * @throws rocksdb::Status on write failure
   */
  void WriteManifestAtomic(const IndexManifestV1& manifest,
                           const VersionHistoryEntry& history_entry);

  /**
   * @brief Get a specific version from history
   *
   * @param version_number Version to retrieve
   * @return VersionHistoryEntry if found, empty entry if not found
   * @throws rocksdb::Status on read failure
   */
  VersionHistoryEntry GetVersionHistory(uint32_t version_number) const;

  /**
   * @brief List all version history entries (in version order)
   *
   * @return Vector of all VersionHistoryEntry objects in version_history CF
   * @throws rocksdb::Status on read failure
   */
  std::vector<VersionHistoryEntry> ListVersionHistory() const;

  /**
   * @brief Check if a previous version is available for rollback
   *
   * @param version_number Target version to check
   * @return true if version exists and is accessible, false otherwise
   * @throws rocksdb::Status on read failure
   */
  bool CanRollbackToVersion(uint32_t version_number) const;

  /**
   * @brief Atomically rollback to previous version (N-1)
   *
   * Retrieves the second-to-last version from history and restores it
   * as current manifest. **Atomic:** Both read and write complete together.
   *
   * @return true if rollback succeeded, false if no previous version
   * @throws rocksdb::Status on failure
   */
  bool RollbackToPreviousVersion();

  /**
   * @brief Increment and return next version number
   *
   * Uses a dedicated "version_counter" key in RocksDB to atomically
   * generate unique version numbers. **Atomic:** Counter is incremented
   * and returned under ACID guarantees.
   *
   * @return Next unused version number (incremented from current)
   * @throws rocksdb::Status on failure
   */
  uint32_t GetNextVersionNumber();

 private:
  IndexMetadataStore(rocksdb::DB* db, const std::string& index_id);

  std::unique_ptr<rocksdb::DB> db_;
  rocksdb::ColumnFamilyHandle* cf_default_{nullptr};
  rocksdb::ColumnFamilyHandle* cf_version_history_{nullptr};
  rocksdb::ColumnFamilyHandle* cf_embeddings_{nullptr};
  std::string index_id_;

  /**
   * @brief Encode a uint32 version number as big-endian bytes
   * @param version_number Version number to encode
   * @return String of 4 bytes in big-endian order
   */
  static std::string EncodeVersionNumber(uint32_t version_number);

  /**
   * @brief Decode a big-endian byte string to uint32 version number
   * @param encoded Encoded version bytes
   * @return Decoded version number, or 0 if malformed
   */
  static uint32_t DecodeVersionNumber(const std::string& encoded);
};

}  // namespace themis::index
