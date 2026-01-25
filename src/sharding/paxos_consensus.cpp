// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/paxos_consensus.h"
#include <spdlog/spdlog.h>
#include <fstream>

namespace themisdb {
namespace sharding {

PaxosConsensus::PaxosConsensus(const ConsensusConfig& config)
    : config_(config)
    , state_(ConsensusState::FOLLOWER)
    , running_(false)
    , current_round_(0)
    , next_slot_(1)
    , commit_index_(0)
    , total_proposals_(0)
    , failed_proposals_(0)
    , total_prepares_(0)
    , total_accepts_(0)
{
}

PaxosConsensus::~PaxosConsensus() {
    stop();
}

bool PaxosConsensus::initialize(
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    node_id_ = node_id;
    cluster_nodes_ = cluster_nodes;
    
    // Load persistent state if available
    if (config_.enable_persistence) {
        if (!loadPersistentState()) {
            spdlog::warn("No persistent state found, starting fresh");
        }
    }
    
    return true;
}

bool PaxosConsensus::start() {
    if (running_.load()) {
        spdlog::warn("Paxos consensus already running");
        return false;
    }
    
    running_.store(true);
    
    // Start background threads
    proposer_thread_ = std::thread(&PaxosConsensus::runProposer, this);
    acceptor_thread_ = std::thread(&PaxosConsensus::runAcceptor, this);
    learner_thread_ = std::thread(&PaxosConsensus::runLearner, this);
    election_thread_ = std::thread(&PaxosConsensus::leaderElectionThread, this);
    
    spdlog::info("Paxos consensus started for node {}", node_id_);
    return true;
}

void PaxosConsensus::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    // Wake up threads
    proposal_cv_.notify_all();
    
    // Join threads
    if (proposer_thread_.joinable()) proposer_thread_.join();
    if (acceptor_thread_.joinable()) acceptor_thread_.join();
    if (learner_thread_.joinable()) learner_thread_.join();
    if (election_thread_.joinable()) election_thread_.join();
    
    // Save state
    if (config_.enable_persistence) {
        savePersistentState();
    }
    
    spdlog::info("Paxos consensus stopped for node {}", node_id_);
}

bool PaxosConsensus::isLeader() const {
    return state_.load() == ConsensusState::LEADER;
}

std::string PaxosConsensus::getLeaderId() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_leader_;
}

ConsensusState PaxosConsensus::getState() const {
    return state_.load();
}

std::optional<uint64_t> PaxosConsensus::propose(
    const std::string& operation,
    const nlohmann::json& data
) {
    if (!running_.load()) {
        spdlog::error("Cannot propose: Paxos not running");
        return std::nullopt;
    }
    
    // Create log entry
    ConsensusLogEntry entry;
    entry.index = next_slot_++;
    entry.term = current_round_;
    entry.operation = operation;
    entry.data = data;
    entry.timestamp = std::chrono::system_clock::now();
    
    // Add to pending proposals
    {
        std::lock_guard<std::mutex> lock(proposal_mutex_);
        pending_proposals_[entry.index] = entry;
    }
    
    proposal_cv_.notify_one();
    total_proposals_++;
    
    return entry.index;
}

bool PaxosConsensus::waitForCommit(
    uint64_t log_index,
    std::chrono::milliseconds timeout
) {
    auto start = std::chrono::steady_clock::now();
    
    while (running_.load()) {
        {
            std::lock_guard<std::mutex> lock(proposal_mutex_);
            if (committed_log_.find(log_index) != committed_log_.end()) {
                return true;
            }
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return false;
}

std::vector<ConsensusLogEntry> PaxosConsensus::readLog(
    uint64_t start_index,
    std::optional<uint64_t> end_index
) {
    std::vector<ConsensusLogEntry> result;
    std::lock_guard<std::mutex> lock(proposal_mutex_);
    
    uint64_t end = end_index.value_or(commit_index_);
    
    for (uint64_t i = start_index; i <= end; ++i) {
        auto it = committed_log_.find(i);
        if (it != committed_log_.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

uint64_t PaxosConsensus::getCommitIndex() const {
    return commit_index_;
}

bool PaxosConsensus::addNode(
    const std::string& node_id,
    const std::string& /*endpoint*/
) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Check if node already exists
    if (std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id) != cluster_nodes_.end()) {
        spdlog::warn("Node {} already in cluster", node_id);
        return false;
    }
    
    cluster_nodes_.push_back(node_id);
    spdlog::info("Added node {} to cluster", node_id);
    return true;
}

bool PaxosConsensus::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id);
    if (it == cluster_nodes_.end()) {
        spdlog::warn("Node {} not found in cluster", node_id);
        return false;
    }
    
    cluster_nodes_.erase(it);
    spdlog::info("Removed node {} from cluster", node_id);
    return true;
}

bool PaxosConsensus::transferLeadership(const std::string& target_node_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!isLeader()) {
        spdlog::warn("Cannot transfer leadership: not the leader");
        return false;
    }
    
    // In Paxos, we don't have a persistent leader
    // Just step down and let target become proposer
    state_.store(ConsensusState::FOLLOWER);
    current_leader_ = target_node_id;
    
    spdlog::info("Transferred leadership to {}", target_node_id);
    return true;
}

bool PaxosConsensus::takeSnapshot(const nlohmann::json& /*snapshot_data*/) {
    spdlog::warn("Paxos snapshot not yet implemented");
    return false;
}

bool PaxosConsensus::restoreSnapshot(const nlohmann::json& /*snapshot_data*/) {
    spdlog::warn("Paxos snapshot restore not yet implemented");
    return false;
}

ConsensusStats PaxosConsensus::getStats() const {
    ConsensusStats stats{};
    stats.current_term = current_round_;
    stats.commit_index = commit_index_;
    stats.last_applied = commit_index_;
    stats.state = state_.load();
    stats.current_leader = current_leader_;
    stats.cluster_size = cluster_nodes_.size();
    stats.reachable_nodes = cluster_nodes_.size();  // Simplified
    stats.total_operations = total_proposals_.load();
    stats.failed_operations = failed_proposals_.load();
    return stats;
}

nlohmann::json PaxosConsensus::getStatus() const {
    auto stats = getStats();
    
    return {
        {"type", "Paxos"},
        {"node_id", node_id_},
        {"is_leader", isLeader()},
        {"leader_id", stats.current_leader},
        {"state", static_cast<int>(stats.state)},
        {"current_round", current_round_},
        {"commit_index", stats.commit_index},
        {"cluster_size", stats.cluster_size},
        {"total_proposals", stats.total_operations},
        {"failed_proposals", stats.failed_operations}
    };
}

void PaxosConsensus::onCommit(
    std::function<void(const ConsensusLogEntry&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_commit_callback_ = std::move(callback);
}

void PaxosConsensus::onStateChange(
    std::function<void(ConsensusState, ConsensusState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

void PaxosConsensus::onLeaderChange(
    std::function<void(const std::string&, const std::string&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_leader_change_callback_ = std::move(callback);
}

// Private methods

void PaxosConsensus::runProposer() {
    spdlog::debug("Paxos proposer thread started");
    
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(proposal_mutex_);
        
        // Wait for proposals
        proposal_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !pending_proposals_.empty() || !running_.load();
        });
        
        if (!running_.load()) break;
        
        // Process pending proposals
        for (auto it = pending_proposals_.begin(); it != pending_proposals_.end();) {
            auto& [slot, entry] = *it;
            
            lock.unlock();
            bool success = executePreparePhase(slot, entry);
            lock.lock();
            
            if (success) {
                it = pending_proposals_.erase(it);
            } else {
                ++it;
                failed_proposals_++;
            }
        }
    }
    
    spdlog::debug("Paxos proposer thread stopped");
}

void PaxosConsensus::runAcceptor() {
    spdlog::debug("Paxos acceptor thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Acceptor logic would process incoming prepare/accept requests
    }
    
    spdlog::debug("Paxos acceptor thread stopped");
}

void PaxosConsensus::runLearner() {
    spdlog::debug("Paxos learner thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Learner logic would track accepted values
    }
    
    spdlog::debug("Paxos learner thread stopped");
}

void PaxosConsensus::leaderElectionThread() {
    spdlog::debug("Paxos leader election thread started");
    
    while (running_.load()) {
        // Simplified leader election
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (cluster_nodes_.empty()) continue;
        
        // Simple deterministic leader selection based on node ID
        std::string expected_leader = *std::min_element(
            cluster_nodes_.begin(), cluster_nodes_.end()
        );
        
        bool should_be_leader = (node_id_ == expected_leader);
        bool is_current_leader = (state_.load() == ConsensusState::LEADER);
        
        if (should_be_leader && !is_current_leader) {
            state_.store(ConsensusState::LEADER);
            current_leader_ = node_id_;
            spdlog::info("Node {} became leader", node_id_);
        } else if (!should_be_leader && is_current_leader) {
            state_.store(ConsensusState::FOLLOWER);
            current_leader_ = expected_leader;
        }
    }
    
    spdlog::debug("Paxos leader election thread stopped");
}

bool PaxosConsensus::executePreparePhase(uint64_t slot, const ConsensusLogEntry& value) {
    // Simplified prepare phase
    auto proposal = generateProposalNumber();
    
    // In a real implementation, this would send prepare requests to all nodes
    // and wait for a quorum of promises
    
    total_prepares_++;
    
    // Execute accept phase
    return executeAcceptPhase(slot, proposal, value);
}

bool PaxosConsensus::executeAcceptPhase(
    uint64_t slot,
    const ProposalNumber& proposal,
    const ConsensusLogEntry& value
) {
    // Simplified accept phase
    // In a real implementation, this would send accept requests to all nodes
    // and wait for a quorum of acceptances
    
    total_accepts_++;
    
    // Mark as committed
    broadcastCommit(slot, value);
    
    return true;
}

void PaxosConsensus::broadcastCommit(uint64_t slot, const ConsensusLogEntry& value) {
    {
        std::lock_guard<std::mutex> lock(proposal_mutex_);
        committed_log_[slot] = value;
        if (slot > commit_index_) {
            commit_index_ = slot;
        }
    }
    
    // Call commit callback
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        if (on_commit_callback_) {
            on_commit_callback_(value);
        }
    }
    
    spdlog::debug("Committed entry at slot {}", slot);
}

size_t PaxosConsensus::getQuorumSize() const {
    if (config_.paxos_quorum_size > 0) {
        return config_.paxos_quorum_size;
    }
    return (cluster_nodes_.size() / 2) + 1;
}

bool PaxosConsensus::hasQuorum(size_t count) const {
    return count >= getQuorumSize();
}

ProposalNumber PaxosConsensus::generateProposalNumber() {
    return ProposalNumber{current_round_++, node_id_};
}

bool PaxosConsensus::loadPersistentState() {
    // TODO: Complete implementation
    // Should load current_round_, next_slot_, commit_index_,
    // instances_, and committed_log_ from persistent storage (e.g., RocksDB)
    // For now, returns false indicating no state to load
    return false;  // Placeholder - no persistent state loaded
}

bool PaxosConsensus::savePersistentState() {
    // TODO: Complete implementation
    // Should save current_round_, next_slot_, commit_index_,
    // instances_, and committed_log_ to persistent storage (e.g., RocksDB)
    // For now, returns true indicating save succeeded (no-op)
    return true;  // Placeholder - no persistent state saved
}

bool PaxosConsensus::handlePrepare(uint64_t /*slot*/, const ProposalNumber& /*proposal*/) {
    // Simplified prepare handler
    return true;
}

bool PaxosConsensus::handleAccept(
    uint64_t /*slot*/,
    const ProposalNumber& /*proposal*/,
    const ConsensusLogEntry& /*value*/
) {
    // Simplified accept handler
    return true;
}

void PaxosConsensus::handleCommit(uint64_t slot, const ConsensusLogEntry& value) {
    broadcastCommit(slot, value);
}

} // namespace sharding
} // namespace themisdb
