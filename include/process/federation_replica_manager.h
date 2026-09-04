/**
 * @file federation_replica_manager.h
 * @brief Replica state machine executor and recovery orchestrator.
 * @version 2.1.0
 * @date 2026-08-06
 */

#ifndef THEMISDB_INCLUDE_PROCESS_FEDERATION_REPLICA_MANAGER_H
#define THEMISDB_INCLUDE_PROCESS_FEDERATION_REPLICA_MANAGER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace process {

// Forward declarations
class FederationReplicaManagerImpl;

/**
 * @brief Configuration for replica manager.
 */
struct FederationReplicaConfig {
  uint64_t snapshot_interval_entries = 1000;
  uint32_t batch_apply_size = 100;
};

/**
 * @brief Replica statistics.
 */
struct ReplicaStats {
  uint64_t entries_applied = 0;
  uint64_t snapshots_taken = 0;
  uint64_t divergence_detected = 0;
  uint64_t last_applied = 0;
  uint64_t state_size_bytes = 0;
};

/**
 * @brief Snapshot representation.
 */
struct Snapshot {
  uint64_t snapshot_index = 0;
  uint64_t snapshot_term;
  std::string state_blob;
  uint32_t checksum;
  uint64_t created_at_ms;
  uint64_t duration_ms;
};

/**
 * @class FederationReplicaManager
 * @brief State machine executor and recovery coordinator.
 *
 * Applies replicated log entries to state machine, manages snapshots,
 * verifies consistency, and handles automatic recovery from failures.
 */
class FederationReplicaManager {
 public:
  /**
   * @brief Factory method to create replica manager.
   */
  static std::unique_ptr<FederationReplicaManager> Create(
      const FederationReplicaConfig& config,
      const std::string& node_id);

  /**
   * @brief Constructor.
   */
  explicit FederationReplicaManager(
      std::unique_ptr<FederationReplicaManagerImpl> impl);

  /**
   * @brief Destructor.
   */
  ~FederationReplicaManager();

  /**
   * @brief Apply committed log entry to state machine.
   */
  std::string ApplyEntry(uint64_t log_index, uint64_t log_term,
                         const std::string& data);

  /**
   * @brief Verify replica consistency via state hash.
   */
  bool VerifyConsistency(const std::string& expected_state_hash,
                         uint64_t at_log_index) const;

  /**
   * @brief Take snapshot of current state.
   */
  std::shared_ptr<Snapshot> TakeSnapshot();

  /**
   * @brief Restore from snapshot and replay tail entries.
   */
  bool RestoreFromSnapshot(const Snapshot* snapshot,
                           const std::vector<std::string>& tail_entries);

  /**
   * @brief Get current state hash.
   */
  std::string GetStateHash() const;

  /**
   * @brief Get last applied log index.
   */
  uint64_t GetLastApplied() const;

  /**
   * @brief Get replica statistics.
   */
  ReplicaStats GetStats() const;

 private:
  std::unique_ptr<FederationReplicaManagerImpl> impl_;
};

}  // namespace process
}  // namespace themis

#endif  // THEMISDB_INCLUDE_PROCESS_FEDERATION_REPLICA_MANAGER_H
