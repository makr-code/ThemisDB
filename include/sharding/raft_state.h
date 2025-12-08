#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace themisdb {
namespace sharding {

/**
 * @brief Raft node states
 */
enum class RaftNodeState {
    FOLLOWER,   // Receives heartbeats and votes in elections
    CANDIDATE,  // Campaigns for leadership
    LEADER      // Sends heartbeats and replicates log
};

/**
 * @brief Vote request structure
 */
struct VoteRequest {
    uint64_t term;                  // Candidate's term
    std::string candidate_id;        // Candidate requesting vote
    uint64_t last_log_index;        // Index of candidate's last log entry
    uint64_t last_log_term;         // Term of candidate's last log entry
};

/**
 * @brief Vote response structure
 */
struct VoteResponse {
    uint64_t term;          // Current term for candidate to update itself
    bool vote_granted;      // True if vote was granted
    std::string voter_id;   // ID of the node that voted
};

/**
 * @brief Raft configuration
 */
struct RaftConfig {
    std::string node_id;                        // This node's ID
    std::vector<std::string> cluster_members;   // All cluster member IDs
    uint32_t election_timeout_min_ms = 150;     // Min election timeout (ms)
    uint32_t election_timeout_max_ms = 300;     // Max election timeout (ms)
    uint32_t heartbeat_interval_ms = 50;        // Heartbeat interval (ms)
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

private:
    /**
     * @brief Calculate random election timeout
     * @return Timeout duration
     */
    std::chrono::milliseconds getRandomElectionTimeout();

    /**
     * @brief Check if have enough votes to become leader
     * @return True if have quorum
     */
    bool hasQuorum() const;

    // Configuration
    RaftConfig config_;

    // Persistent state (would be persisted to disk in production)
    std::atomic<uint64_t> current_term_{0};     // Latest term server has seen
    std::string voted_for_;                     // Candidate ID voted for in current term
    
    // Volatile state
    mutable std::mutex state_mutex_;            // Protects mutable state
    RaftNodeState state_{RaftNodeState::FOLLOWER};
    std::string leader_id_;                     // Current leader ID
    
    // Election state
    std::chrono::steady_clock::time_point election_timeout_time_;
    std::chrono::steady_clock::time_point last_heartbeat_time_;
    std::map<std::string, bool> votes_received_;  // Votes in current election
    
    // Random number generator for election timeout
    mutable std::random_device rd_;
    mutable std::mt19937 rng_{rd_()};
};

}  // namespace sharding
}  // namespace themisdb
