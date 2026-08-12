/**
 * @file raft_state.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>
#include "sharding/raft_log.h"
#include "sharding/consensus_module.h"  // For VoteRequest/VoteResponse

namespace themisdb {
namespace sharding {

/**
 * @brief Raft Consensus Protocol Implementation
 * 
 * Sources:
 * - Algorithm: Raft Consensus Protocol
 * - Paper: Ongaro, D., & Ousterhout, J. (2014)
 *          "In Search of an Understandable Consensus Algorithm"
 *          USENIX Annual Technical Conference (ATC '14)
 * - URL: https://raft.github.io/
 * - Extended Paper: https://raft.github.io/raft.pdf
 * - License: Algorithm is freely implementable (no license restrictions)
 * - ThemisDB Implementation: Custom implementation with:
 *   - Integration with RocksDB for persistent log storage
 *   - Support for ThemisDB's VCC-URN sharding scheme
 *   - mTLS support for secure cluster communication
 *   - Optimized for database replication workloads
 */

/**
 * @brief Raft node states
 */
enum class RaftNodeState {
    FOLLOWER,   ///< Receives heartbeats and votes in elections.
    CANDIDATE,  ///< Campaigns for leadership.
    LEADER      ///< Sends heartbeats and drives log replication.
};

// VoteRequest and VoteResponse are defined in consensus_module.h

/**
 * @brief Raft configuration
 */
struct RaftConfig {
    std::string node_id;                        ///< Local node identifier.
    std::vector<std::string> cluster_members;   ///< Current cluster member IDs.
    uint32_t election_timeout_min_ms = 150;     ///< Lower bound for randomized election timeout.
    uint32_t election_timeout_max_ms = 300;     ///< Upper bound for randomized election timeout.
    uint32_t heartbeat_interval_ms = 50;        ///< Leader heartbeat interval.
};

/**
 * @brief Raft consensus state machine
 * 
 * Implements the core Raft consensus algorithm for leader election.
 * Handles state transitions (FOLLOWER → CANDIDATE → LEADER) and
 * maintains term-based leadership.
 * 
 * Thread-safe for concurrent access.
 */
class RaftState {
public:
    /**
     * @brief Constructor
     * @param config Raft configuration
     */
    explicit RaftState(const RaftConfig& config);

    /**
     * @brief Destructor
     */
    ~RaftState() = default;

    // Prevent copying
    RaftState(const RaftState&) = delete;
    RaftState& operator=(const RaftState&) = delete;

    /**
     * @brief Become follower with given term
     * @param term Term number
     */
    void becomeFollower(uint64_t term);

    /**
     * @brief Become candidate and start election
     */
    void becomeCandidate();

    /**
     * @brief Become leader (after winning election)
     */
    void becomeLeader();

    /**
     * @brief Start a new election
     * 
     * Increments term, votes for self, and requests votes from peers.
     */
    void startElection();

    /**
     * @brief Handle vote request from candidate
     * @param request Vote request
     * @return Vote response
     */
    VoteResponse handleVoteRequest(const VoteRequest& request);

    /**
     * @brief Receive vote from a peer
     * @param voter_id ID of the voting node
     * @param granted Whether vote was granted
     */
    void receiveVote(const std::string& voter_id, bool granted);

    /**
     * @brief Reset election timeout
     * 
     * Called when receiving heartbeat from leader or granting vote.
     */
    void resetElectionTimeout();

    /**
     * @brief Check if election timeout has occurred
     * @return True if election timeout has passed
     */
    bool isElectionTimeout() const;

    /**
     * @brief Send heartbeat (as leader)
     * 
     * Resets election timeouts on followers.
     */
    void sendHeartbeat();

    /**
     * @brief Check if heartbeat interval has passed
     * @return True if should send heartbeat
     */
    bool shouldSendHeartbeat() const;

    /**
     * @brief Get current state
     * @return Current Raft state
     */
    RaftNodeState getState() const;

    /**
     * @brief Get current term
     * @return Current term number
     */
    uint64_t getCurrentTerm() const;

    /**
     * @brief Get current leader ID
     * @return Leader ID (empty if no known leader)
     */
    std::string getLeaderId() const;

    /**
     * @brief Check if this node is the leader
     * @return True if this node is the leader
     */
    bool isLeader() const;

    /**
     * @brief Check if this node is a follower
     * @return True if this node is a follower
     */
    bool isFollower() const;

    /**
     * @brief Check if this node is a candidate
     * @return True if this node is a candidate
     */
    bool isCandidate() const;

    /**
     * @brief Handle AppendEntries RPC from leader
     * @param request AppendEntries request
     * @return AppendEntries response
     */
    AppendEntriesResponse handleAppendEntries(const AppendEntriesRequest& request);

    /**
     * @brief Get reference to the Raft log
        * @return Mutable reference to RaftLog storage state.
     */
    RaftLog& getLog();

    /**
     * @brief Get reference to the Raft log (const)
        * @return Const reference to RaftLog storage state.
     */
    const RaftLog& getLog() const;

    /**
     * @brief Get node ID
     * @return This node's ID
     */
    std::string getNodeId() const;

    /**
     * @brief Get cluster members
     * @return List of all cluster member IDs
     */
    std::vector<std::string> getClusterMembers() const;

    /**
     * @brief Update the cluster member list after a committed membership change
        * @param members New authoritative list of cluster member IDs.
        * @note Caller is responsible for applying this only after consensus commit.
     */
    void setClusterMembers(const std::vector<std::string>& members);

    /**
     * @brief Get quorum size
     * @return Number of nodes needed for quorum (n/2 + 1)
     */
    size_t getQuorumSize() const;

    /**
     * @brief Get who this node voted for in current term
     * @return Candidate ID (empty if haven't voted)
     */
    std::string getVotedFor() const;

    /**
     * @brief Get votes received in current election
     * @return Number of votes received
     */
    size_t getVotesReceived() const;

    // ------------------------------------------------------------------
    // Snapshot state (persistent – survives restarts / log compaction)
    // ------------------------------------------------------------------

    /**
     * @brief Record the index and term of the most recently installed snapshot
     *
     * Called by RaftSnapshotManager after a snapshot has been successfully
     * persisted and the log has been compacted up to snapshot_index.
     *
     * @param index Index of the last log entry covered by the snapshot
     * @param term  Term of the last log entry covered by the snapshot
     */
    void setSnapshotMeta(uint64_t index, uint64_t term);

    /**
     * @brief Get the index of the last installed snapshot
     * @return Snapshot index (0 if no snapshot has been taken)
     */
    uint64_t getSnapshotIndex() const;

    /**
     * @brief Get the term of the last installed snapshot
     * @return Snapshot term (0 if no snapshot has been taken)
     */
    uint64_t getSnapshotTerm() const;

private:
    /**
     * @brief Calculate random election timeout
        * @return Random timeout in configured [min,max] range.
     */
    std::chrono::milliseconds getRandomElectionTimeout();

    /**
     * @brief Check if have enough votes to become leader
        * @return True when granted votes satisfy current quorum size.
     */
    bool hasQuorum() const;

    // Configuration
    RaftConfig config_;

    // Persistent state (would be persisted to disk in production)
    std::atomic<uint64_t> current_term_{0};     ///< Latest term observed by this node.
    std::string voted_for_;                     ///< Candidate ID voted for in current term.
    RaftLog log_;                               ///< Durable replicated log abstraction.

    // Persistent snapshot state (§7 of the Raft paper)
    uint64_t snapshot_index_{0};   ///< Index of the last installed snapshot.
    uint64_t snapshot_term_{0};    ///< Term of the last installed snapshot.

    // Volatile state
    mutable std::mutex state_mutex_;            ///< Protects mutable consensus state.
    RaftNodeState state_{RaftNodeState::FOLLOWER};
    std::string leader_id_;                     ///< Current known leader ID.
    
    // Election state
    std::chrono::steady_clock::time_point election_timeout_time_;
    std::chrono::steady_clock::time_point last_heartbeat_time_;
    std::map<std::string, bool> votes_received_;  ///< Votes collected in active election.
    
    // Random number generator for election timeout
    mutable std::random_device rd_;
    mutable std::mt19937 rng_{rd_()};
};

}  // namespace sharding
}  // namespace themisdb
