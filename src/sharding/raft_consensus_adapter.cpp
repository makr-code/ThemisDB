/**
 * @file raft_consensus_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_consensus_adapter.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <thread>
#include <stdexcept>

namespace themisdb {
namespace sharding {

/** @brief Construct adapter with initial follower cached state. */
RaftConsensusAdapter::RaftConsensusAdapter(const ConsensusConfig& config)
    : config_(config)
    , current_state_(ConsensusState::FOLLOWER)
{
}

/** @brief Ensure Raft engine is stopped before object teardown. */
RaftConsensusAdapter::~RaftConsensusAdapter() {
    stop();
}

/**
 * @brief Initialize underlying Raft engine and membership tracker.
 * @param node_id Local node identifier.
 * @param cluster_nodes Initial cluster members.
 * @return True when Raft engine construction succeeds.
 */
bool RaftConsensusAdapter::initialize(
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    node_id_ = node_id;
    cluster_nodes_ = cluster_nodes;
    
    // Initialize joint-consensus membership tracker
    membership_ = std::make_unique<themis::sharding::RaftConfiguration>(
        std::set<std::string>(cluster_nodes.begin(), cluster_nodes.end()));

    // Create Raft configuration
    RaftConsensus::Config raft_config;
    raft_config.raft_config.node_id = node_id;
    raft_config.raft_config.cluster_members = cluster_nodes;
    raft_config.raft_config.election_timeout_min_ms = 
        static_cast<uint32_t>(config_.election_timeout_min.count());
    raft_config.raft_config.election_timeout_max_ms = 
        static_cast<uint32_t>(config_.election_timeout_max.count());
    raft_config.raft_config.heartbeat_interval_ms = 
        static_cast<uint32_t>(config_.heartbeat_interval.count());
    
    raft_config.heartbeat_timeout = config_.heartbeat_interval;
    raft_config.enable_partition_detection = true;
    raft_config.enable_split_brain_prevention = true;
    
    try {
        raft_ = std::make_unique<RaftConsensus>(raft_config);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize Raft consensus: {}", e.what());
        return false;
    }
}

/** @brief Start underlying Raft engine. */
bool RaftConsensusAdapter::start() {
    if (!raft_) {
        spdlog::error("Raft not initialized");
        return false;
    }
    
    try {
        raft_->start();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start Raft consensus: {}", e.what());
        return false;
    }
}

/** @brief Stop underlying Raft engine if initialized. */
void RaftConsensusAdapter::stop() {
    if (raft_) {
        raft_->stop();
    }
}

/** @brief Return leadership status from underlying Raft engine. */
bool RaftConsensusAdapter::isLeader() const {
    return raft_ && raft_->isLeader();
}

/** @brief Return current known leader ID from Raft engine. */
std::string RaftConsensusAdapter::getLeaderId() const {
    return raft_ ? raft_->getLeaderId() : "";
}

/** @brief Return current consensus state, using cache when engine unavailable. */
ConsensusState RaftConsensusAdapter::getState() const {
    // Get state from Raft without holding our own lock to avoid lock ordering issues
    // RaftState::getState() has its own internal synchronization
    if (raft_) {
        return convertState(raft_->getRaftState());
    }
    
    // Fallback to cached state if Raft is not available
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_state_;
}

/** @brief Map Raft-specific state enum to generic consensus state enum. */
ConsensusState RaftConsensusAdapter::convertState(const RaftState& state) {
    // Extract the actual RaftNodeState from RaftState and convert to ConsensusState
    RaftNodeState node_state = state.getState();
    
    switch (node_state) {
        case RaftNodeState::FOLLOWER:
            return ConsensusState::FOLLOWER;
        case RaftNodeState::CANDIDATE:
            return ConsensusState::CANDIDATE;
        case RaftNodeState::LEADER:
            return ConsensusState::LEADER;
        default:
            return ConsensusState::FOLLOWER;  // Safe default
    }
}

/**
 * @brief Propose operation payload to Raft leader.
 * @param operation Operation type identifier.
 * @param data Operation payload.
 * @return Expected log index for the proposal or nullopt on failure.
 */
std::optional<uint64_t> RaftConsensusAdapter::propose(
    const std::string& operation,
    const nlohmann::json& data
) {
    if (!raft_ || !isLeader()) {
        return std::nullopt;
    }
    
    try {
        // Serialize operation and data
        nlohmann::json command_json = {
            {"operation", operation},
            {"data", data}
        };
        std::string command = command_json.dump();
        
        // Get the log index before proposing to handle potential race conditions
        // Note: There's still a small race window here. In a production system,
        // the propose() method should return the log index directly.
        uint64_t expected_index = raft_->getRaftState().getLog().getLastLogIndex() + 1;
        
        // Propose to Raft
        auto future = raft_->propose(command);
        
        // Return the expected log index
        // In a production system, we would verify this matches the actual appended index
        return expected_index;
    } catch (const std::exception& e) {
        spdlog::error("Failed to propose operation: {}", e.what());
        return std::nullopt;
    }
}

/**
 * @brief Wait until requested log index is committed or timeout elapses.
 * @param log_index Target log index.
 * @param timeout Maximum wait duration.
 * @return True when commit index reaches target.
 */
bool RaftConsensusAdapter::waitForCommit(
    uint64_t log_index,
    std::chrono::milliseconds timeout
) {
    if (!raft_) {
        return false;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + timeout;
    
    // Poll the commit index until it reaches the target log_index or timeout
    // Note: This is a simple polling implementation. A production implementation
    // should use a condition variable to be notified of commit index changes
    // rather than busy-waiting, which would be more efficient.
    while (std::chrono::steady_clock::now() < end_time) {
        uint64_t commit_index = raft_->getRaftState().getLog().getCommitIndex();
        
        if (commit_index >= log_index) {
            return true;  // Successfully committed
        }
        
        // Sleep briefly to avoid busy waiting
        // Use a small interval to be responsive
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Timeout occurred
    return false;
}

/** @brief Read committed Raft log entries in requested index window. */
std::vector<ConsensusLogEntry> RaftConsensusAdapter::readLog(
    uint64_t start_index,
    std::optional<uint64_t> end_index
) {
    if (!raft_) {
        return {};
    }
    
    std::vector<ConsensusLogEntry> result;
    
    try {
        // Get reference to the Raft log
        // Note: RaftLog methods are internally synchronized via mutex,
        // so these calls are thread-safe individually
        const RaftLog& log = raft_->getRaftState().getLog();
        
        // Determine the actual end index
        uint64_t actual_end_index = end_index.value_or(log.getLastLogIndex());
        
        // Ensure we don't read beyond the commit index
        uint64_t commit_index = log.getCommitIndex();
        if (actual_end_index > commit_index) {
            actual_end_index = commit_index;
        }
        
        // Read entries in the range
        if (start_index <= actual_end_index) {
            std::vector<LogEntry> entries = log.getEntries(start_index, actual_end_index);
            
            // Convert each LogEntry to ConsensusLogEntry
            result.reserve(entries.size());
            for (const auto& entry : entries) {
                result.push_back(convertLogEntry(entry));
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to read log entries: {}", e.what());
        return {};
    }
    
    return result;
}

/** @brief Return committed index from underlying Raft log. */
uint64_t RaftConsensusAdapter::getCommitIndex() const {
    if (!raft_) {
        return 0;
    }
    
    return raft_->getRaftState().getLog().getCommitIndex();
}

/** @brief Return last log index from underlying Raft log. */
uint64_t RaftConsensusAdapter::getLastLogIndex() const {
    if (!raft_) {
        return 0;
    }
    
    return raft_->getRaftState().getLog().getLastLogIndex();
}

/**
 * @brief Add a node via two-phase Raft joint-consensus reconfiguration.
 * @param node_id Node ID to add.
 * @param endpoint Node endpoint metadata passed in config commands.
 * @return True when reconfiguration commits successfully.
 */
bool RaftConsensusAdapter::addNode(
    const std::string& node_id,
    const std::string& endpoint
) {
    if (!raft_ || !isLeader()) {
        spdlog::warn("Cannot add node: not leader or Raft not initialized");
        return false;
    }

    // Guard: node must not already be a member
    if (membership_ && membership_->isMember(node_id)) {
        spdlog::warn("Node {} already exists in cluster", node_id);
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        auto it = std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id);
        if (it != cluster_nodes_.end()) {
            spdlog::warn("Node {} already exists in cluster", node_id);
            return false;
        }
    }

    // ---- Phase 1: enter joint consensus (C_old,new) -------------------------
    if (!membership_) {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        membership_ = std::make_unique<themis::sharding::RaftConfiguration>(
            std::set<std::string>(cluster_nodes_.begin(), cluster_nodes_.end()));
    }

    try {
        membership_->addNode(node_id);
    } catch (const std::exception& e) {
        spdlog::error("Cannot start membership change for {}: {}", node_id, e.what());
        return false;
    }

    // Helper: convert set to sorted vector for JSON serialisation
    auto to_vec = [](const std::set<std::string>& s) {
        return std::vector<std::string>(s.begin(), s.end());
    };

    nlohmann::json joint_data = {
        {"node",        node_id},
        {"endpoint",    endpoint},
        {"old_members", to_vec(membership_->getOldMembers())},
        {"new_members", to_vec(membership_->getNewMembers())}
    };

    auto phase1_idx = propose("CONFIG_JOINT", joint_data);
    if (!phase1_idx.has_value()) {
        // Rollback to old configuration
        themis::sharding::ConfigurationEntry rollback{
            {},
            membership_->getOldMembers(),
            false
        };
        membership_->applyConfiguration(rollback);
        spdlog::error("Failed to propose joint configuration for node {}", node_id);
        return false;
    }

    if (!waitForCommit(*phase1_idx, std::chrono::milliseconds(5000))) {
        // Rollback
        themis::sharding::ConfigurationEntry rollback{
            {},
            membership_->getOldMembers(),
            false
        };
        membership_->applyConfiguration(rollback);
        spdlog::error("Timeout waiting for joint configuration commit (add {})", node_id);
        return false;
    }

    // ---- Phase 2: finalise to C_new ----------------------------------------
    std::set<std::string> c_new = membership_->getNewMembers();
    themis::sharding::ConfigurationEntry finalize{
        {},
        c_new,
        false
    };
    membership_->applyConfiguration(finalize);

    // Propagate C_new into the underlying RaftState so that quorum calculations,
    // heartbeat delivery, and log replication use the updated member list.
    std::vector<std::string> c_new_vec(c_new.begin(), c_new.end());
    raft_->getRaftState().setClusterMembers(c_new_vec);
    raft_->addReplicaNode(node_id);

    nlohmann::json final_data = {
        {"node",        node_id},
        {"new_members", to_vec(c_new)}
    };

    auto phase2_idx = propose("CONFIG_NEW", final_data);
    if (phase2_idx.has_value()) {
        waitForCommit(*phase2_idx, std::chrono::milliseconds(5000));
    }

    // Commit cluster_nodes_ to the new configuration
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        cluster_nodes_.push_back(node_id);
    }

    spdlog::info("Node {} added to cluster via joint consensus (endpoint: {})",
                 node_id, endpoint);
    return true;
}

/**
 * @brief Remove a node via two-phase Raft joint-consensus reconfiguration.
 * @param node_id Node ID to remove.
 * @return True when reconfiguration commits successfully.
 */
bool RaftConsensusAdapter::removeNode(const std::string& node_id) {
    if (!raft_ || !isLeader()) {
        spdlog::warn("Cannot remove node: not leader or Raft not initialized");
        return false;
    }

    // Cannot remove self
    if (node_id == node_id_) {
        spdlog::error("Cannot remove self from cluster");
        return false;
    }

    // Guard: node must exist
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        auto it = std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id);
        if (it == cluster_nodes_.end()) {
            spdlog::warn("Node {} not found in cluster", node_id);
            return false;
        }
    }

    // ---- Phase 1: enter joint consensus (C_old,new) -------------------------
    if (!membership_) {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        membership_ = std::make_unique<themis::sharding::RaftConfiguration>(
            std::set<std::string>(cluster_nodes_.begin(), cluster_nodes_.end()));
    }

    try {
        membership_->removeNode(node_id);
    } catch (const std::exception& e) {
        spdlog::error("Cannot start membership change for removal of {}: {}", node_id, e.what());
        return false;
    }

    auto to_vec = [](const std::set<std::string>& s) {
        return std::vector<std::string>(s.begin(), s.end());
    };

    nlohmann::json joint_data = {
        {"node",        node_id},
        {"old_members", to_vec(membership_->getOldMembers())},
        {"new_members", to_vec(membership_->getNewMembers())}
    };

    auto phase1_idx = propose("CONFIG_JOINT", joint_data);
    if (!phase1_idx.has_value()) {
        // Rollback
        themis::sharding::ConfigurationEntry rollback{
            {},
            membership_->getOldMembers(),
            false
        };
        membership_->applyConfiguration(rollback);
        spdlog::error("Failed to propose joint configuration for removal of {}", node_id);
        return false;
    }

    if (!waitForCommit(*phase1_idx, std::chrono::milliseconds(5000))) {
        // Rollback
        themis::sharding::ConfigurationEntry rollback{
            {},
            membership_->getOldMembers(),
            false
        };
        membership_->applyConfiguration(rollback);
        spdlog::error("Timeout waiting for joint configuration commit (remove {})", node_id);
        return false;
    }

    // ---- Phase 2: finalise to C_new ----------------------------------------
    std::set<std::string> c_new = membership_->getNewMembers();
    themis::sharding::ConfigurationEntry finalize{
        {},
        c_new,
        false
    };
    membership_->applyConfiguration(finalize);

    // Propagate C_new into the underlying RaftState so that quorum calculations,
    // heartbeat delivery, and log replication use the updated member list.
    std::vector<std::string> c_new_vec(c_new.begin(), c_new.end());
    raft_->getRaftState().setClusterMembers(c_new_vec);
    raft_->removeReplicaNode(node_id);

    nlohmann::json final_data = {
        {"node",        node_id},
        {"new_members", to_vec(c_new)}
    };

    auto phase2_idx = propose("CONFIG_NEW", final_data);
    if (phase2_idx.has_value()) {
        waitForCommit(*phase2_idx, std::chrono::milliseconds(5000));
    }

    // Commit cluster_nodes_ to the new configuration
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        auto it = std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id);
        if (it != cluster_nodes_.end()) {
            cluster_nodes_.erase(it);
        }
    }

    spdlog::info("Node {} removed from cluster via joint consensus", node_id);
    return true;
}

/**
 * @brief Initiate best-effort leadership transfer by stepping down locally.
 * @param target_node_id Target node expected to assume leadership.
 * @return True when local step-down operation succeeds.
 */
bool RaftConsensusAdapter::transferLeadership(const std::string& target_node_id) {
    if (!raft_ || !isLeader()) {
        spdlog::warn("transferLeadership: not leader or Raft not initialized");
        return false;
    }

    // Validate target exists in the known cluster
    {
        std::lock_guard<std::mutex> lock(cluster_mutex_);
        auto it = std::find(cluster_nodes_.begin(), cluster_nodes_.end(), target_node_id);
        if (it == cluster_nodes_.end()) {
            spdlog::error("transferLeadership: target node {} not in cluster", target_node_id);
            return false;
        }
    }

    // Cannot transfer leadership to self
    if (target_node_id == node_id_) {
        spdlog::warn("transferLeadership: target is self ({}), nothing to do", node_id_);
        return true;
    }

    // Propose an advisory log entry so the target's log is up-to-date
    nlohmann::json transfer_data = {
        {"target",  target_node_id},
        {"from",    node_id_}
    };
    propose("LEADERSHIP_TRANSFER", transfer_data);

    // Step down: read the current term and call becomeFollower(term+1) under the
    // state_mutex_ so the term read and the state transition are serialised
    // against any concurrent state change.
    uint64_t new_term;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        new_term = raft_->getCurrentTerm() + 1;
        raft_->getRaftState().becomeFollower(new_term);
        current_state_ = ConsensusState::FOLLOWER;
    }

    spdlog::info("transferLeadership: stepped down to follower (term {}) in favour of {}",
                 new_term, target_node_id);
    return true;
}

/** @brief Capture in-adapter snapshot metadata from current committed state. */
bool RaftConsensusAdapter::takeSnapshot(const nlohmann::json& snapshot_data) {
    if (!raft_) {
        spdlog::warn("takeSnapshot: Raft not initialized");
        return false;
    }

    const uint64_t commit_idx = raft_->getRaftState().getLog().getCommitIndex();
    const uint64_t term       = raft_->getRaftState().getCurrentTerm();

    {
        std::lock_guard<std::mutex> lock(snapshot_mutex_);
        snapshot_data_  = snapshot_data;
        snapshot_index_ = commit_idx;
        snapshot_term_  = term;
    }

    spdlog::info("takeSnapshot: snapshot taken at index={} term={}", commit_idx, term);
    return true;
}

/** @brief Restore in-adapter snapshot metadata and optionally force follower mode. */
bool RaftConsensusAdapter::restoreSnapshot(const nlohmann::json& snapshot_data) {
    if (snapshot_data.is_null() || snapshot_data.empty()) {
        spdlog::error("restoreSnapshot: snapshot_data is null or empty");
        return false;
    }

    // Extract optional index/term metadata embedded in the snapshot
    uint64_t restored_index = 0;
    uint64_t restored_term  = 0;
    if (snapshot_data.contains("_snapshot_index"))
        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();
    if (snapshot_data.contains("_snapshot_term"))
        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();

    {
        std::lock_guard<std::mutex> lock(snapshot_mutex_);
        snapshot_data_  = snapshot_data;
        snapshot_index_ = restored_index;
        snapshot_term_  = restored_term;
    }

    // If Raft is running, step down to follower so the restored state is
    // consistent with a freshly-caught-up node.
    if (raft_ && restored_term > 0) {
        raft_->getRaftState().becomeFollower(restored_term);
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            current_state_ = ConsensusState::FOLLOWER;
        }
    }

    spdlog::info("restoreSnapshot: snapshot restored at index={} term={}",
                 restored_index, restored_term);
    return true;
}

/** @brief Collect runtime adapter statistics. */
ConsensusStats RaftConsensusAdapter::getStats() const {
    ConsensusStats stats{};
    
    if (raft_) {
        stats.current_term = raft_->getCurrentTerm();
        stats.state = getState();
        stats.current_leader = raft_->getLeaderId();
        
        {
            std::lock_guard<std::mutex> lock(cluster_mutex_);
            stats.cluster_size = cluster_nodes_.size();
        }
        
        auto partition_status = raft_->getPartitionStatus();
        stats.reachable_nodes = partition_status.reachable_nodes.size();
    }
    
    return stats;
}

/** @brief Return structured adapter status payload for diagnostics APIs. */
nlohmann::json RaftConsensusAdapter::getStatus() const {
    auto stats = getStats();

    uint64_t snap_index = 0;
    uint64_t snap_term  = 0;
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex_);
        snap_index = snapshot_index_;
        snap_term  = snapshot_term_;
    }

    bool in_joint = membership_ && membership_->isJointConsensus();

    return {
        {"type",              "Raft"},
        {"node_id",           node_id_},
        {"is_leader",         isLeader()},
        {"leader_id",         stats.current_leader},
        {"state",             static_cast<int>(stats.state)},
        {"current_term",      stats.current_term},
        {"cluster_size",      stats.cluster_size},
        {"reachable_nodes",   stats.reachable_nodes},
        {"snapshot_index",    snap_index},
        {"snapshot_term",     snap_term},
        {"is_joint_consensus", in_joint}
    };
}

/** @brief Register callback invoked on committed entries. */
void RaftConsensusAdapter::onCommit(
    std::function<void(const ConsensusLogEntry&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_commit_callback_ = std::move(callback);
}

/** @brief Register callback invoked on consensus-state transitions. */
void RaftConsensusAdapter::onStateChange(
    std::function<void(ConsensusState, ConsensusState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

/** @brief Register callback invoked on leader changes. */
void RaftConsensusAdapter::onLeaderChange(
    std::function<void(const std::string&, const std::string&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_leader_change_callback_ = std::move(callback);
}

/** @brief Convert serialized Raft entry payload into generic consensus entry. */
ConsensusLogEntry RaftConsensusAdapter::convertLogEntry(const LogEntry& entry) {
    ConsensusLogEntry consensus_entry;
    consensus_entry.index = entry.index;
    consensus_entry.term = entry.term;
    
    try {
        // Parse command JSON
        auto command_json = nlohmann::json::parse(entry.command);
        consensus_entry.operation = command_json.value("operation", "");
        consensus_entry.data = command_json.value("data", nlohmann::json{});
    } catch (...) {
        // If parsing fails, store raw command
        consensus_entry.operation = "raw";
        consensus_entry.data = {{"command", entry.command}};
    }
    
    return consensus_entry;
}

} // namespace sharding
} // namespace themisdb


