/**
 * @file raft_consensus.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_consensus.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <algorithm>
#include <iostream>
#include <limits>

namespace themisdb {
namespace sharding {

RaftConsensus::RaftConsensus(const Config& config)
    : config_(config), raft_state_(config.raft_config) {
    initializeReplicaStates();
    partition_status_.is_partitioned = false;
    partition_status_.has_quorum = true;
}

RaftConsensus::~RaftConsensus() {
    stop();
}

/**
 * @brief Start.
 * @details Calls: exchange(), std::thread().
 */
void RaftConsensus::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    // Start background threads
    heartbeat_thread_ = std::thread(&RaftConsensus::heartbeatLoop, this);
    election_thread_ = std::thread(&RaftConsensus::electionLoop, this);
    
    if (config_.enable_partition_detection) {
        partition_detector_thread_ = std::thread(&RaftConsensus::partitionDetectionLoop, this);
    }
}

/**
 * @brief Stop.
 * @details Calls: exchange(), notify_all(), themis::utils::joinThreadWithin(), THEMIS_WARN().
 */
void RaftConsensus::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    // Wake up all threads
    cv_.notify_all();
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(heartbeat_thread_)) {
        THEMIS_WARN("[RaftConsensus] heartbeat thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(election_thread_)) {
        THEMIS_WARN("[RaftConsensus] election thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(partition_detector_thread_)) {
        THEMIS_WARN("[RaftConsensus] partition detector thread did not finish within shutdown deadline; detaching.");
    }
}

/**
 * @brief Propose.
 * @param[in] command Input parameter.
 * @return Return value.
 * @details Delegates to proposeWithIndex() and resolves the future synchronously.
 */
std::future<bool> RaftConsensus::propose(const std::string& command) {
    std::promise<bool> promise;
    auto future = promise.get_future();
    promise.set_value(proposeWithIndex(command).has_value());
    return future;
}

std::optional<uint64_t> RaftConsensus::proposeWithIndex(const std::string& command) {

    ReplicationCallback cb;
    AppendEntriesRequest request;
    LogEntry captured_entry;
    {
        std::lock_guard<std::mutex> lock(replica_mutex_);
        if (isReadOnly()) {
            return std::nullopt;
        }

        try {
            auto proposal = raft_state_.appendProposalIfLeader(command);
            if (!proposal) {
                return std::nullopt;
            }
            request = std::move(*proposal);
            captured_entry = request.entries.back();
            cb = replication_callback_;
        } catch (const std::exception& error) {
            THEMIS_ERROR("[RaftConsensus] durable append failed: {}", error.what());
            read_only_mode_.store(true);
            return std::nullopt;
        }
    }

    int acks = 1;  // Leader counts as one acknowledgment
    int required = static_cast<int>(raft_state_.getQuorumSize());

    if (cb) {
        for (const auto& member : raft_state_.getClusterMembers()) {
            if (member == raft_state_.getNodeId()) {
                continue;  // Skip self
            }

            const auto response = replicateToFollower(member, request, cb);
            if (response.term > request.term) {
                try {
                    raft_state_.becomeFollower(response.term);
                } catch (const std::exception& error) {
                    THEMIS_ERROR("[RaftConsensus] failed to persist follower term {}: {}",
                                 response.term, error.what());
                    read_only_mode_.store(true);
                }
                return std::nullopt;
            }
            if (response.term == request.term && response.success &&
                response.match_index >= captured_entry.index) {
                acks++;
                if (acks >= required) {
                    break;
                }
            }
        }
    }

    if (acks < required) {
        THEMIS_WARN("[RaftConsensus] proposal {} did not reach quorum", captured_entry.index);
        return std::nullopt;
    }

    {
        std::lock_guard<std::mutex> lock(replica_mutex_);
        try {
            if (!raft_state_.commitProposalIfLeader(captured_entry)) {
                THEMIS_WARN("[RaftConsensus] proposal {} lost leadership or its log entry",
                            captured_entry.index);
                return std::nullopt;
            }
        } catch (const std::exception& error) {
            THEMIS_ERROR("[RaftConsensus] durable commit update failed: {}", error.what());
            read_only_mode_.store(true);
            return std::nullopt;
        }
    }

    return captured_entry.index;
}

bool RaftConsensus::isLeader() const {
    return raft_state_.isLeader();
}

bool RaftConsensus::hasQuorum() const {
    return checkQuorum();
}

PartitionStatus RaftConsensus::getPartitionStatus() const {
    /**
     * @brief Lock.
     * @param[in] partition_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(partition_mutex_);
    return partition_status_;
}

bool RaftConsensus::isReadOnly() const {
    return read_only_mode_.load();
}

std::string RaftConsensus::getLeaderId() const {
    return raft_state_.getLeaderId();
}

uint64_t RaftConsensus::getCurrentTerm() const {
    return raft_state_.getCurrentTerm();
}

std::vector<ReplicaState> RaftConsensus::getReplicaStates() const {
    /**
     * @brief Lock.
     * @param[in] replica_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(replica_mutex_);
    std::vector<ReplicaState> states = {};

    states.reserve(replica_states_.size());
    for (const auto& pair : replica_states_) {
        states.push_back(pair.second);
    }
    return states;
}

/**
 * @brief Set Replication Callback.
 * @param[in] callback Input parameter.
 * @details Calls: lock(), std::move().
 */
void RaftConsensus::setReplicationCallback(ReplicationCallback callback) {
    // RAFT-2: Protect the write side under the same mutex used by propose()
    // to read the callback, preventing a data race on std::function.
    std::lock_guard<std::mutex> lock(replica_mutex_);
    replication_callback_ = std::move(callback);
}

void RaftConsensus::setVoteRequestCallback(VoteRequestCallback callback) {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    vote_request_callback_ = std::move(callback);
}

/**
 * @brief Set Heartbeat Callback.
 * @param[in] callback Input parameter.
 * @details Calls: lock().
 */
void RaftConsensus::setHeartbeatCallback(HeartbeatCallback callback) {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    heartbeat_callback_ = callback;
}

/**
 * @brief Receive Heartbeat.
 * @param[in] heartbeat Input parameter.
 * @details Calls: getCurrentTerm(), getState(), becomeFollower(), resetElectionTimeout(), store().
 */
void RaftConsensus::receiveHeartbeat(const Heartbeat& heartbeat) {
    // Update leader info
    if (heartbeat.term >= raft_state_.getCurrentTerm()) {
        if (heartbeat.term > raft_state_.getCurrentTerm() ||
            raft_state_.getState() != RaftNodeState::FOLLOWER) {
            try {
                raft_state_.becomeFollower(heartbeat.term);
            } catch (const std::exception& error) {
                THEMIS_ERROR("[RaftConsensus] failed to persist heartbeat term: {}", error.what());
                read_only_mode_.store(true);
                return;
            }
        }
        raft_state_.resetElectionTimeout();
        
        // Clear read-only mode if we're following a leader
        read_only_mode_.store(false);
    }
}

/**
 * @brief Receive Append Entries Response.
 * @param[in] node_id Identifier of the node.
 * @param[in] response Input parameter.
 * @details Rejects unknown peers, persists higher terms before stepping down, and
 *          advances replica progress only for current-term successful responses.
 */
void RaftConsensus::receiveAppendEntriesResponse(const std::string& node_id,
                                                 const AppendEntriesResponse& response) {
    {
        std::lock_guard<std::mutex> lock(replica_mutex_);
        const auto it = replica_states_.find(node_id);
        if (it == replica_states_.end()) {
            return;
        }
        it->second.last_contact = std::chrono::steady_clock::now();
    }

    if (response.term > raft_state_.getCurrentTerm()) {
        try {
            raft_state_.becomeFollower(response.term);
        } catch (const std::exception& error) {
            THEMIS_ERROR("[RaftConsensus] failed to persist higher AppendEntries response term {}: {}",
                         response.term, error.what());
            read_only_mode_.store(true);
        }
        return;
    }

    std::lock_guard<std::mutex> lock(replica_mutex_);
    auto it = replica_states_.find(node_id);
    if (it == replica_states_.end()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (response.term != raft_state_.getCurrentTerm() || !response.success) {
        updateReplicaHealthLocked(it->second, false, now);
        return;
    }

    it->second.match_index = std::max(it->second.match_index, response.match_index);
    const auto next_index = response.match_index == std::numeric_limits<uint64_t>::max()
        ? response.match_index
        : response.match_index + 1;
    it->second.next_index = std::max(it->second.next_index, next_index);
    updateReplicaHealthLocked(it->second, true, now);
}

/**
 * @brief Heartbeat Loop.
 * @details Calls: isLeader(), sendHeartbeats(), lock(), wait_for(), std::chrono::milliseconds().
 */
void RaftConsensus::heartbeatLoop() {
    while (running_) {
        if (isLeader()) {
            sendHeartbeats();
        }
        
        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(config_.raft_config.heartbeat_interval_ms),
                    [this] { return !running_; });
    }
}

/**
 * @brief Election Loop.
 * @details Calls: isFollower(), isCandidate(), isElectionTimeout(), isReadOnly(), startElection(), lock(), wait_for(), std::chrono::milliseconds().
 */
void RaftConsensus::electionLoop() {
    while (running_) {
        if ((raft_state_.isFollower() || raft_state_.isCandidate()) &&
            raft_state_.isElectionTimeout()) {
            if (!isReadOnly()) {  // Don't start election if in minority partition
                bool election_started = false;
                const auto members = raft_state_.getClusterMembers();
                const bool requires_durable_journal = members.size() > 1;
                if (requires_durable_journal &&
                    (config_.raft_config.persistence_path.empty() ||
                     !raft_state_.getLog().isPersistenceHealthy())) {
                    THEMIS_ERROR("[RaftConsensus] refusing multi-node election without a healthy durable journal");
                    read_only_mode_.store(true);
                } else {
                    try {
                        raft_state_.startElection();
                        election_started = true;
                    } catch (const std::exception& error) {
                        THEMIS_ERROR("[RaftConsensus] failed to persist election state: {}",
                                     error.what());
                        read_only_mode_.store(true);
                    }
                }

                if (election_started) {
                    VoteRequestCallback vote_callback;
                    {
                        std::lock_guard<std::mutex> lock(replica_mutex_);
                        vote_callback = vote_request_callback_;
                    }

                    if (vote_callback) {
                        VoteRequest request;
                        request.candidate_id = raft_state_.getNodeId();
                        request.term = raft_state_.getCurrentTerm();
                        request.last_log_index = raft_state_.getLog().getLastLogIndex();
                        request.last_log_term = raft_state_.getLog().getLastLogTerm();

                        for (const auto& member : raft_state_.getClusterMembers()) {
                            if (member == request.candidate_id) {
                                continue;
                            }
                            if (!running_ || !raft_state_.isCandidate() ||
                                raft_state_.getCurrentTerm() != request.term) {
                                break;
                            }

                            try {
                                const auto response = vote_callback(member, request);
                                if (!raft_state_.isCandidate() ||
                                    raft_state_.getCurrentTerm() != request.term) {
                                    break;
                                }
                                if (response.voter_id != member) {
                                    updateReplicaHealth(member, false);
                                    continue;
                                }
                                if (response.term > request.term) {
                                    try {
                                        raft_state_.becomeFollower(response.term);
                                    } catch (const std::exception& error) {
                                        THEMIS_ERROR(
                                            "[RaftConsensus] failed to persist higher vote-response term {}: {}",
                                            response.term, error.what());
                                        read_only_mode_.store(true);
                                    }
                                    break;
                                }
                                if (response.term != request.term) {
                                    updateReplicaHealth(member, false);
                                    continue;
                                }

                                updateReplicaHealth(member, true);
                                raft_state_.receiveVote(member, response.vote_granted);
                            } catch (const std::exception& error) {
                                THEMIS_WARN("[RaftConsensus] vote request to peer {} failed: {}",
                                            member, error.what());
                                updateReplicaHealth(member, false);
                            }
                        }
                    }
                }
            }
        }
        
        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(50),
                    [this] { return !running_; });
    }
}

/**
 * @brief Partition Detection Loop.
 * @details Calls: detectPartition(), lock(), store(), wait_for().
 */
void RaftConsensus::partitionDetectionLoop() {
    while (running_) {
        auto status = detectPartition();
        
        {
            std::lock_guard<std::mutex> lock(partition_mutex_);
            partition_status_ = status;
        }
        
        // Enter read-only mode if we're in minority partition and configured to do so
        if (config_.read_only_on_partition && status.is_partitioned && !status.has_quorum) {
            read_only_mode_.store(true);
        } else {
            read_only_mode_.store(false);
        }
        
        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.wait_for(lock, config_.partition_detection_interval,
                    [this] { return !running_; });
    }
}

/**
 * @brief Send Heartbeats.
 * @details Calls: getNodeId(), getCurrentTerm(), getLog(), getCommitIndex(), std::chrono::steady_clock::now(), lock(), reserve(), size().
 */
void RaftConsensus::sendHeartbeats() {
    HeartbeatCallback heartbeat_cb;
    Heartbeat hb;
    hb.leader_id = raft_state_.getNodeId();
    hb.term = raft_state_.getCurrentTerm();
    hb.commit_index = raft_state_.getLog().getCommitIndex();
    hb.timestamp = std::chrono::steady_clock::now();
    
    // Collect reachable nodes
    {
        std::lock_guard<std::mutex> lock(replica_mutex_);
        heartbeat_cb = heartbeat_callback_;
        if (!heartbeat_cb) {
            return;
        }
        hb.reachable_nodes.reserve(replica_states_.size());
        for (const auto& pair : replica_states_) {
            if (pair.second.health == ReplicaHealth::HEALTHY ||
                pair.second.health == ReplicaHealth::DEGRADED) {
                hb.reachable_nodes.push_back(pair.first);
            }
        }
    }
    
    // Send to all followers
    for (const auto& member : raft_state_.getClusterMembers()) {
        if (member == raft_state_.getNodeId()) {
            continue;  // Skip self
        }
        if (!raft_state_.isLeader() || raft_state_.getCurrentTerm() != hb.term) {
            break;
        }
        
        bool success = heartbeat_cb(member, hb);
        updateReplicaHealth(member, success);
        if (!raft_state_.isLeader() || raft_state_.getCurrentTerm() != hb.term) {
            break;
        }
    }
}

/**
 * @brief Replicate To Follower.
 * @param[in] node_id Identifier of the node.
 * @param[in] entry Input parameter.
 * @param[in] callback Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: callback(), updateReplicaHealth().
 */
AppendEntriesResponse RaftConsensus::replicateToFollower(
                                        const std::string& node_id,
                                        const AppendEntriesRequest& request,
                                        const ReplicationCallback& callback) {
    if (!callback) {
        return AppendEntriesResponse(raft_state_.getCurrentTerm(), false, 0);
    }
    
    auto response = callback(node_id, request);
    const uint64_t required_match_index =
        request.entries.empty() ? request.prev_log_index : request.entries.back().index;
    const bool success = response.term == request.term && response.success &&
                         response.match_index >= required_match_index;
    updateReplicaHealth(node_id, success);
    return response;
}

/**
 * @brief Detect Partition.
 * @return Return value.
 * @details Calls: std::chrono::steady_clock::now(), lock(), reserve(), size(), push_back(), getQuorumSize(), std::sort(), begin().
 */
PartitionStatus RaftConsensus::detectPartition() {
    PartitionStatus status;
    status.is_partitioned = false;
    status.detected_at = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(replica_mutex_);
    
    // Count healthy nodes
    int healthy_count = 1;  // Count self
    status.reachable_nodes.reserve(replica_states_.size());
    status.unreachable_nodes.reserve(replica_states_.size());
    for (const auto& pair : replica_states_) {
        if (pair.second.health == ReplicaHealth::HEALTHY ||
            pair.second.health == ReplicaHealth::DEGRADED) {
            status.reachable_nodes.push_back(pair.first);
            healthy_count++;
        } else {
            status.unreachable_nodes.push_back(pair.first);
        }
    }
    
    // Check if we have quorum
    size_t quorum_size = raft_state_.getQuorumSize();
    status.has_quorum = (static_cast<size_t>(healthy_count) >= quorum_size);
    
    // Partition detected if we can't reach quorum
    if (!status.has_quorum) {
        status.is_partitioned = true;
    }
    
    // Generate partition ID based on reachable nodes
    std::sort(status.reachable_nodes.begin(), status.reachable_nodes.end());
    status.partition_id = raft_state_.getNodeId();
    for (const auto& node : status.reachable_nodes) {
        status.partition_id += ":" + node;
    }
    
    return status;
}

/**
 * @brief Update Replica Health.
 * @param[in] node_id Identifier of the node.
 * @param[in] success Input parameter.
 * @details Calls: lock(), find(), end(), updateReplicaHealthLocked(), std::chrono::steady_clock::now().
 */
void RaftConsensus::updateReplicaHealth(const std::string& node_id, bool success) {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    auto it = replica_states_.find(node_id);
    if (it == replica_states_.end()) {
        return;
    }

    updateReplicaHealthLocked(it->second, success, std::chrono::steady_clock::now());
}

bool RaftConsensus::checkQuorum() const {
    /**
     * @brief Lock.
     * @param[in] replica_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(replica_mutex_);
    
    int healthy_count = 1;  // Count self
    for (const auto& pair : replica_states_) {
        if (pair.second.health == ReplicaHealth::HEALTHY ||
            pair.second.health == ReplicaHealth::DEGRADED) {
            healthy_count++;
        }
    }
    
    return static_cast<size_t>(healthy_count) >= raft_state_.getQuorumSize();
}

/**
 * @brief Update Replica Health Locked.
 * @param[in,out] state Input/output parameter.
 * @param[in] success Input parameter.
 * @param[in] now Input parameter.
 * @details Implements updateReplicaHealthLocked without additional internal calls.
 */
void RaftConsensus::updateReplicaHealthLocked(ReplicaState& state,
                                              bool success,
                                              std::chrono::steady_clock::time_point now) {
    if (success) {
        state.consecutive_failures = 0;
        state.last_contact = now;
        state.health = ReplicaHealth::HEALTHY;
        return;
    }

    state.consecutive_failures++;
    const auto time_since_contact = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - state.last_contact);

    if (state.consecutive_failures >= config_.max_consecutive_failures ||
        time_since_contact > config_.heartbeat_timeout * 3) {
        state.health = ReplicaHealth::UNREACHABLE;
    } else {
        state.health = ReplicaHealth::DEGRADED;
    }
}

/**
 * @brief Initialize Replica States.
 * @details Calls: lock(), std::chrono::steady_clock::now().
 */
void RaftConsensus::initializeReplicaStates() {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    
    for (const auto& member : config_.raft_config.cluster_members) {
        if (member == config_.raft_config.node_id) {
            continue;  // Don't track self
        }
        
        ReplicaState state;
        state.node_id = member;
        state.health = ReplicaHealth::HEALTHY;
        state.next_index = 1;
        state.match_index = 0;
        state.last_contact = std::chrono::steady_clock::now();
        state.consecutive_failures = 0;
        
        replica_states_[member] = state;
    }
}

/**
 * @brief Add Replica Node.
 * @param[in] node_id Identifier of the node.
 * @details Calls: lock(), find(), end(), getLog(), getLastLogIndex(), std::chrono::steady_clock::now().
 */
void RaftConsensus::addReplicaNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    if (replica_states_.find(node_id) == replica_states_.end()) {
        ReplicaState state;
        state.node_id = node_id;
        state.health = ReplicaHealth::HEALTHY;
        state.next_index = raft_state_.getLog().getLastLogIndex() + 1;
        state.match_index = 0;
        state.last_contact = std::chrono::steady_clock::now();
        state.consecutive_failures = 0;
        replica_states_[node_id] = state;
    }
}

/**
 * @brief Remove Replica Node.
 * @param[in] node_id Identifier of the node.
 * @details Calls: lock(), erase().
 */
void RaftConsensus::removeReplicaNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    replica_states_.erase(node_id);
}

/**
 * @brief Update Peer Address.
 * @param[in] node_id Identifier of the node.
 * @param[in] new_endpoint Input parameter.
 * @details Calls: lock(), find(), end().
 */
void RaftConsensus::updatePeerAddress(const std::string& node_id,
                                       const std::string& new_endpoint) {
    std::lock_guard<std::mutex> lock(replica_mutex_);
    auto it = replica_states_.find(node_id);
    if (it == replica_states_.end()) {
        std::cerr << "[RaftConsensus] updatePeerAddress: unknown peer '"
                  << node_id << "' — ignored\n";
        return;
    }
    it->second.endpoint = new_endpoint;
}

}  // namespace sharding
}  // namespace themisdb
