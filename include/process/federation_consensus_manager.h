/**
 * @file federation_consensus_manager.h
 * @brief Raft-inspired quorum-based consensus manager for federated deployments.
 * @version 2.1.0
 * @date 2026-08-06
 */

#ifndef THEMISDB_INCLUDE_PROCESS_FEDERATION_CONSENSUS_MANAGER_H
#define THEMISDB_INCLUDE_PROCESS_FEDERATION_CONSENSUS_MANAGER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>

namespace themis {
namespace process {

// Forward declarations
class FederationConsensusManagerImpl;

/**
 * @brief Configuration for consensus manager.
 */
struct FederationConsensusConfig {
  uint64_t network_latency_ms = 5;
  uint64_t heartbeat_interval_ms = 50;
  uint64_t election_timeout_ms = 300;
};

/**
 * @brief Server state machine state.
 */
enum class ServerState {
  FOLLOWER,   ///< Following leader
  CANDIDATE,  ///< Candidate in election
  LEADER      ///< Leader
};

/**
 * @class FederationConsensusManager
 * @brief Raft-inspired consensus orchestrator.
 *
 * Coordinates quorum-based voting, leader election, log replication,
 * and Byzantine fault tolerance across multiple Process Module replicas.
 */
class FederationConsensusManager {
 public:
  /**
   * @brief Factory method to create consensus manager.
   */
  static std::unique_ptr<FederationConsensusManager> Create(
      const FederationConsensusConfig& config,
      const std::string& node_id,
      size_t quorum_size);

  /**
   * @brief Constructor.
   */
  explicit FederationConsensusManager(
      std::unique_ptr<FederationConsensusManagerImpl> impl);

  /**
   * @brief Destructor.
   */
  ~FederationConsensusManager();

  /**
   * @brief Append entry to consensus log.
   */
  uint64_t AppendEntry(const std::string& data);

  /**
   * @brief Check if this node is leader.
   */
  bool IsLeader() const;

  /**
   * @brief Get current leader ID.
   */
  std::string GetLeader() const;

  /**
   * @brief Request vote (called by remote candidate).
   */
  bool RequestVote(const std::string& candidate_id, uint64_t candidate_term,
                   uint64_t candidate_last_log_index,
                   uint64_t candidate_last_log_term);

  /**
   * @brief Append entries RPC (heartbeat or replication).
   */
  bool AppendEntries(const std::string& leader_id, uint64_t leader_term,
                     uint64_t prev_log_index, uint64_t prev_log_term,
                     const std::vector<std::string>& entries,
                     uint64_t leader_commit);

  /**
   * @brief Tick state machine (called periodically).
   */
  void Tick();

 private:
  std::unique_ptr<FederationConsensusManagerImpl> impl_;
};

}  // namespace process
}  // namespace themis

#endif  // THEMISDB_INCLUDE_PROCESS_FEDERATION_CONSENSUS_MANAGER_H
