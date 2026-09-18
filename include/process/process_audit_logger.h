/**
 * @file process_audit_logger.h
 * @brief Immutable append-only audit trail for model mutations.
 * @version 2.1.0
 * @date 2026-08-06
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_AUDIT_LOGGER_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_AUDIT_LOGGER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace process {

// Forward declarations
class ProcessAuditLoggerImpl;

/**
 * @brief Configuration for audit logger.
 */
struct AuditLoggerConfig {
  std::string backend_type = "rocksdb";  // rocksdb, s3, file
  uint64_t snapshot_interval_entries = 1000;
  std::string storage_path = "./audit";
};

/**
 * @brief Audit trail entry.
 */
struct AuditTrailEntry {
  uint64_t entry_id = 0;
  std::string model_id;
  std::string operation_type;  // insert, update, delete
  std::string delta_patch;
  std::string before_hash;
  std::string after_hash;
  uint64_t timestamp_ms = 0;
};

/**
 * @brief Audit logger statistics.
 */
struct AuditLoggerStats {
  uint64_t entries_appended = 0;
  uint64_t snapshots_created = 0;
  uint64_t storage_size_bytes = 0;
  uint64_t last_entry_id = 0;
};

/**
 * @class ProcessAuditLogger
 * @brief Immutable audit trail storage and retrieval engine.
 *
 * Provides append-only audit trail storage with delta encoding (RFC 7386),
 * temporal snapshot indexing, and CRC32 integrity verification.
 */
class ProcessAuditLogger {
 public:
  /**
   * @brief Factory method to create audit logger.
   */
  static std::unique_ptr<ProcessAuditLogger> Create(
      const AuditLoggerConfig& config);

  /**
   * @brief Constructor.
   */
  explicit ProcessAuditLogger(
      std::unique_ptr<ProcessAuditLoggerImpl> impl);

  /**
   * @brief Destructor.
   */
  ~ProcessAuditLogger();

  /**
   * @brief Append immutable entry to audit trail.
   */
  uint64_t AppendEntry(const std::string& model_id,
                       const std::string& operation,
                       const std::string& before_state,
                       const std::string& after_state);

  /**
   * @brief Verify audit trail integrity via CRC32 chain.
   */
  bool VerifyIntegrity() const;

  /**
   * @brief Query audit trail for entries of a specific model.
   */
  std::vector<AuditTrailEntry> QueryByModelId(const std::string& model_id) const;

  /**
   * @brief Query audit trail for entries in a time range.
   */
  std::vector<AuditTrailEntry> QueryByTimeRange(uint64_t start_ms,
                                                  uint64_t end_ms) const;

  /**
   * @brief Reconstruct model state at specific point in time.
   */
  std::string GetModelStateAt(const std::string& model_id,
                              uint64_t timestamp_ms) const;

  /**
   * @brief Get audit logger statistics.
   */
  AuditLoggerStats GetStats() const;

  /**
   * @brief Create snapshot of current audit trail state.
   */
  uint64_t CreateSnapshot();

 private:
  std::unique_ptr<ProcessAuditLoggerImpl> impl_;
};

}  // namespace process
}  // namespace themis

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_AUDIT_LOGGER_H
