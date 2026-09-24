/**
 * @file audit_log_store.h
 * @brief Persistent audit trail storage for query decisions
 *
 * Logs all retrieval queries and policy enforcement decisions
 * to RocksDB for compliance and forensic analysis.
 *
 * **Audit Entry Contents:**
 * - Query ID, principal, resource, action
 * - Policy enforcement decision (allowed/denied, reason)
 * - Timestamp, execution time, result count
 * - Encryption/audit flags applied
 *
 * **Storage Layout:**
 * ```
 * RocksDB {
 *   "default" CF:
 *     "audit_counter" → current entry count
 *   "audit_logs" CF:
 *     uint64_be(entry_id) → AuditEntry JSON
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

namespace rocksdb {
class DB;
class ColumnFamilyHandle;
}

namespace themis::security {

/**
 * @struct AuditEntry
 * @brief Single audit log entry
 */
struct AuditEntry {
  uint64_t entry_id{0};
  std::string query_id;
  std::string principal_id;
  std::string resource_id;
  int action{0};  // PolicyAction enum
  std::string query_text;
  std::chrono::system_clock::time_point timestamp;

  // Decision
  bool allowed{false};
  std::string decision_reason;
  bool encryption_applied{false};
  bool audit_required{false};

  // Execution
  uint32_t execution_time_ms{0};
  uint32_t result_count{0};
  bool success{true};
  std::string error_message;

  /**
   * @brief Serialize to JSON
   * @return JSON representation
   */
  nlohmann::json to_json() const;

  /**
   * @brief Deserialize from JSON
   * @param j JSON object
   * @return AuditEntry
   */
  static AuditEntry from_json(const nlohmann::json& j);
};

/**
 * @class AuditLogStore
 * @brief Persistent audit trail in RocksDB
 *
 * Appends audit entries to immutable log for compliance.
 * Supports querying/filtering historical entries.
 */
class AuditLogStore {
 public:
  /**
   * @brief Open or create audit log store
   * @param db_path Path to RocksDB directory
   * @return Initialized store
   */
  static std::unique_ptr<AuditLogStore> Open(const std::string& db_path);

  ~AuditLogStore();

  // Delete copy; move allowed
  AuditLogStore(const AuditLogStore&) = delete;
  AuditLogStore& operator=(const AuditLogStore&) = delete;
  AuditLogStore(AuditLogStore&&) = default;
  AuditLogStore& operator=(AuditLogStore&&) = default;

  /**
   * @brief Append audit entry
   * @param entry Entry to append
   * @return Assigned entry ID
   */
  uint64_t AppendEntry(const AuditEntry& entry);

  /**
   * @brief Get audit entry by ID
   * @param entry_id Entry ID
   * @return AuditEntry, or empty if not found
   */
  AuditEntry GetEntry(uint64_t entry_id) const;

  /**
   * @brief List entries in range
   * @param start_id First entry ID (inclusive)
   * @param end_id Last entry ID (inclusive)
   * @return Vector of entries in range
   */
  std::vector<AuditEntry> ListEntries(uint64_t start_id, uint64_t end_id) const;

  /**
   * @brief Get total entry count
   * @return Number of entries in audit log
   */
  uint64_t GetEntryCount() const;

  /**
   * @brief Query entries by principal
   * @param principal_id Principal to filter
   * @param max_results Max entries to return
   * @return Entries matching principal
   */
  std::vector<AuditEntry> QueryByPrincipal(const std::string& principal_id,
                                            size_t max_results = 1000) const;

  /**
   * @brief Query entries by resource
   * @param resource_id Resource to filter
   * @param max_results Max entries to return
   * @return Entries matching resource
   */
  std::vector<AuditEntry> QueryByResource(const std::string& resource_id,
                                           size_t max_results = 1000) const;

  /**
   * @brief Query denied decisions
   * @param max_results Max entries to return
   * @return Denied entries
   */
  std::vector<AuditEntry> QueryDeniedDecisions(size_t max_results = 1000) const;

 private:
  AuditLogStore(rocksdb::DB* db);

  std::unique_ptr<rocksdb::DB> db_;
  rocksdb::ColumnFamilyHandle* cf_default_{nullptr};
  rocksdb::ColumnFamilyHandle* cf_audit_logs_{nullptr};

  /**
   * @brief Encode entry ID as big-endian bytes
   * @param entry_id Entry ID
   * @return Encoded key
   */
  static std::string EncodeEntryId(uint64_t entry_id);

  /**
   * @brief Decode entry ID from bytes
   * @param encoded Encoded key
   * @return Entry ID
   */
  static uint64_t DecodeEntryId(const std::string& encoded);
};

}  // namespace themis::security
