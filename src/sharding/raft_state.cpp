/**
 * @file raft_state.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/raft_state.h"
#include <algorithm>
#include <iostream>

namespace themisdb {
namespace sharding {

/** @brief Initialize Raft state with configuration and fresh timers. */
RaftState::RaftState(const RaftConfig& config)
    : config_(config) {
    resetElectionTimeout();
    last_heartbeat_time_ = std::chrono::steady_clock::now();
}

/** @brief Transition node to follower role for provided term. */
void RaftState::becomeFollower([[maybe_unused]] uint64_t term) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (term > current_term_) {
        current_term_ = term;
        voted_for_.clear();
    }
    
    state_ = RaftNodeState::FOLLOWER;
    votes_received_.clear();
    resetElectionTimeout();
}

/** @brief Transition node to candidate role and self-vote for election. */
void RaftState::becomeCandidate() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    state_ = RaftNodeState::CANDIDATE;
    current_term_++;
    voted_for_ = config_.node_id;  // Vote for self
    votes_received_.clear();
    votes_received_[config_.node_id] = true;  // Count own vote
    leader_id_.clear();
    
    resetElectionTimeout();
}

/** @brief Promote candidate to leader once quorum has been reached. */
void RaftState::becomeLeader() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ != RaftNodeState::CANDIDATE) {
        return;  // Can only become leader from candidate state
    }
    
    if (!hasQuorum()) {
        return;  // Need quorum to become leader
    }
    
    state_ = RaftNodeState::LEADER;
    leader_id_ = config_.node_id;
    votes_received_.clear();
    
    // Reset heartbeat timer
    last_heartbeat_time_ = std::chrono::steady_clock::now();
}

/** @brief Trigger new election round by entering candidate state. */
void RaftState::startElection() {
    becomeCandidate();
    
    // In a real implementation, would send VoteRequest RPCs to all peers here
    // For now, this is handled by the caller
}

/**
 * @brief Process vote request from remote candidate.
 * @param request Incoming vote request.
 * @return Vote response including decision and local term.
 */
VoteResponse RaftState::handleVoteRequest(const VoteRequest& request) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    VoteResponse response;
    response.term = current_term_;
    response.voter_id = config_.node_id;
    response.vote_granted = false;
    
    // If request term is older, reject
    if (request.term < current_term_) {
        return response;
    }
    
    // If request term is newer, update our term and become follower
    if (request.term > current_term_) {
        current_term_ = request.term;
        voted_for_.clear();
        state_ = RaftNodeState::FOLLOWER;
        leader_id_.clear();
        response.term = current_term_;
    }
    
    // Grant vote if:
    // 1. Haven't voted in this term, OR already voted for this candidate
    // 2. Candidate's log is at least as up-to-date as ours (simplified for now)
    bool can_vote = voted_for_.empty() || voted_for_ == request.candidate_id;
    
    if (can_vote) {
        voted_for_ = request.candidate_id;
        response.vote_granted = true;
        resetElectionTimeout();  // Reset timeout when granting vote
    }
    
    return response;
}

/** @brief Record vote response for active election and promote on quorum. */
void RaftState::receiveVote(const std::string& voter_id, bool granted) {
    std::unique_lock<std::mutex> lock(state_mutex_);
    
    if (state_ != RaftNodeState::CANDIDATE) {
        return;  // Only candidates care about votes
    }
    
    votes_received_[voter_id] = granted;
    
    // Check if we have quorum
    if (hasQuorum()) {
        // Become leader (unlock before calling to avoid deadlock).
        // Use unique_lock so that unlock/lock are tracked by the RAII guard,
        // preventing a double-unlock if becomeLeader() throws.
        lock.unlock();
        becomeLeader();
        lock.lock();
    }
}

/** @brief Reset election timeout deadline using randomized timeout window. */
void RaftState::resetElectionTimeout() {
    auto timeout = getRandomElectionTimeout();
    election_timeout_time_ = std::chrono::steady_clock::now() + timeout;
}

/** @brief Check whether follower/candidate election timeout has expired. */
bool RaftState::isElectionTimeout() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ == RaftNodeState::LEADER) {
        return false;  // Leaders don't have election timeouts
    }
    
    return std::chrono::steady_clock::now() >= election_timeout_time_;
}

/** @brief Emit heartbeat timestamp update when node is leader. */
void RaftState::sendHeartbeat() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ != RaftNodeState::LEADER) {
        return;  // Only leaders send heartbeats
    }
    
    last_heartbeat_time_ = std::chrono::steady_clock::now();
    
    // In a real implementation, would send AppendEntries RPCs to all peers here
    // For now, this is handled by the caller
}

/** @brief Return whether heartbeat interval elapsed for current leader. */
bool RaftState::shouldSendHeartbeat() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ != RaftNodeState::LEADER) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_heartbeat_time_);
    
    return elapsed.count() >= config_.heartbeat_interval_ms;
}

/** @brief Return current Raft node role. */
RaftNodeState RaftState::getState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_;
}

/** @brief Return current term atomically. */
uint64_t RaftState::getCurrentTerm() const {
    return current_term_.load();
}

/** @brief Return currently known leader identifier. */
std::string RaftState::getLeaderId() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return leader_id_;
}

/** @brief Return true if current role is leader. */
bool RaftState::isLeader() const {
    return getState() == RaftNodeState::LEADER;
}

/** @brief Return true if current role is follower. */
bool RaftState::isFollower() const {
    return getState() == RaftNodeState::FOLLOWER;
}

/** @brief Return true if current role is candidate. */
bool RaftState::isCandidate() const {
    return getState() == RaftNodeState::CANDIDATE;
}

/** @brief Return configured local node identifier. */
std::string RaftState::getNodeId() const {
    return config_.node_id;
}

/** @brief Return configured cluster members snapshot. */
std::vector<std::string> RaftState::getClusterMembers() const {
    return config_.cluster_members;
}

/** @brief Replace cluster membership list under state lock. */
void RaftState::setClusterMembers(const std::vector<std::string>& members) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    config_.cluster_members = members;
}

/** @brief Compute majority quorum size from current member count. */
size_t RaftState::getQuorumSize() const {
    return (config_.cluster_members.size() / 2) + 1;
}

/** @brief Return candidate voted for in current term, if any. */
std::string RaftState::getVotedFor() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return voted_for_;
}

/** @brief Return count of granted votes in active election map. */
size_t RaftState::getVotesReceived() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    size_t count = 0;
    for (const auto& vote : votes_received_) {
        if (vote.second) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Process AppendEntries RPC and update log/commit state.
 * @param request Incoming append request from leader.
 * @return AppendEntries response with success and match index.
 */
AppendEntriesResponse RaftState::handleAppendEntries(const AppendEntriesRequest& request) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    AppendEntriesResponse response;
    response.term = current_term_;
    response.success = false;
    response.match_index = 0;
    
    // 1. Reply false if term < currentTerm
    if (request.term < current_term_) {
        return response;
    }
    
    // If RPC request or response contains term T > currentTerm:
    // set currentTerm = T, convert to follower
    if (request.term > current_term_) {
        current_term_ = request.term;
        voted_for_.clear();
        state_ = RaftNodeState::FOLLOWER;
        votes_received_.clear();
    }
    
    // Valid leader, reset election timeout
    leader_id_ = request.leader_id;
    resetElectionTimeout();
    
    // If candidate or leader, step down
    if (state_ != RaftNodeState::FOLLOWER) {
        state_ = RaftNodeState::FOLLOWER;
    }
    
    // 2. Reply false if log doesn't contain an entry at prevLogIndex
    //    whose term matches prevLogTerm
    if (!log_.hasEntry(request.prev_log_index, request.prev_log_term)) {
        response.match_index = log_.getLastLogIndex();
        return response;
    }
    
    // 3. If an existing entry conflicts with a new one (same index but different terms),
    //    delete the existing entry and all that follow it
    for (const auto& entry : request.entries) {
        auto existing = log_.getEntry(entry.index);
        if (existing.has_value() && existing->term != entry.term) {
            log_.truncateFrom(entry.index);
            break;
        }
    }
    
    // 4. Append any new entries not already in the log
    for (const auto& entry : request.entries) {
        if (!log_.getEntry(entry.index).has_value()) {
            log_.append(entry);
        }
    }
    
    // 5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
    if (request.leader_commit > log_.getCommitIndex()) {
        uint64_t new_commit = std::min(request.leader_commit, log_.getLastLogIndex());
        log_.setCommitIndex(new_commit);
    }
    
    response.success = true;
    response.match_index = log_.getLastLogIndex();
    response.term = current_term_;
    
    return response;
}

/** @brief Return mutable access to underlying Raft log object. */
RaftLog& RaftState::getLog() {
    return log_;
}

/** @brief Return const access to underlying Raft log object. */
const RaftLog& RaftState::getLog() const {
    return log_;
}

/** @brief Generate randomized election timeout in configured interval. */
std::chrono::milliseconds RaftState::getRandomElectionTimeout() {
    std::uniform_int_distribution<uint32_t> dist(
        config_.election_timeout_min_ms,
        config_.election_timeout_max_ms);
    return std::chrono::milliseconds(dist(rng_));
}

/** @brief Check whether granted votes satisfy majority quorum. */
bool RaftState::hasQuorum() const {
    size_t votes = 0;
    for (const auto& vote : votes_received_) {
        if (vote.second) {
            votes++;
        }
    }
    return votes >= getQuorumSize();
}

/** @brief Persist snapshot index/term metadata and sync log snapshot base. */
void RaftState::setSnapshotMeta(uint64_t index, uint64_t term) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    snapshot_index_ = index;
    snapshot_term_  = term;
    // Keep the log's snapshot meta in sync so hasEntry() works after compaction
    log_.setSnapshotMeta(index, term);
}

/** @brief Return index covered by most recently installed snapshot. */
uint64_t RaftState::getSnapshotIndex() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return snapshot_index_;
}

/** @brief Return term covered by most recently installed snapshot. */
uint64_t RaftState::getSnapshotTerm() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return snapshot_term_;
}

}  // namespace sharding
}  // namespace themisdb
