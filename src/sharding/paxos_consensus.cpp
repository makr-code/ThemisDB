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
    entry.index = next_slot_.fetch_add(1);
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
    
    uint64_t end = end_index.value_or(commit_index_.load());
    
    for (uint64_t i = start_index; i <= end; ++i) {
        auto it = committed_log_.find(i);
        if (it != committed_log_.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

uint64_t PaxosConsensus::getCommitIndex() const {
    return commit_index_.load();
}

uint64_t PaxosConsensus::getLastLogIndex() const {
    uint64_t next = next_slot_.load();
    return next > 0 ? next - 1 : 0;
}

bool PaxosConsensus::addNode(
    const std::string& node_id,
    const std::string& endpoint
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

bool PaxosConsensus::takeSnapshot(const nlohmann::json& snapshot_data) {
    spdlog::warn("Paxos snapshot not yet implemented");
    return false;
}

bool PaxosConsensus::restoreSnapshot(const nlohmann::json& snapshot_data) {
    spdlog::warn("Paxos snapshot restore not yet implemented");
    return false;
}

ConsensusStats PaxosConsensus::getStats() const {
    ConsensusStats stats{};
    stats.current_term = current_round_;
    stats.commit_index = commit_index_.load();
    stats.last_applied = commit_index_.load();
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
        
        // Process pending proposals with retry logic
        std::vector<uint64_t> failed_slots;
        
        for (auto it = pending_proposals_.begin(); it != pending_proposals_.end();) {
            auto& [slot, entry] = *it;
            
            lock.unlock();
            
            // Retry logic: attempt proposal up to 3 times
            bool success = false;
            const int max_retries = 3;
            
            for (int retry = 0; retry < max_retries && running_.load(); ++retry) {
                if (retry > 0) {
                    spdlog::debug("Node {} retrying proposal for slot {} (attempt {}/{})",
                                 node_id_, slot, retry + 1, max_retries);
                    // Exponential backoff: 100ms, 200ms, 400ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << retry)));
                }
                
                success = executePreparePhase(slot, entry);
                if (success) break;
            }
            
            lock.lock();
            
            if (success) {
                it = pending_proposals_.erase(it);
            } else {
                ++it;
                failed_proposals_++;
                failed_slots.push_back(slot);
            }
        }
        
        // Log failed proposals
        for (uint64_t slot : failed_slots) {
            spdlog::warn("Node {} failed to get consensus for slot {} after retries",
                        node_id_, slot);
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
        
        // Learner tracks accepted values and determines when consensus is reached
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        // Check for newly committed instances
        for (auto& [slot, instance] : instances_) {
            if (instance.is_committed && committed_log_.find(slot) == committed_log_.end()) {
                // This instance just reached consensus
                spdlog::debug("Learner detected committed value at slot {}", slot);
                
                // Update committed log (done via broadcastCommit)
                // Learner's job is to track and apply committed values
            }
        }
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
    // Phase 1a: Generate unique proposal number and send prepare requests
    auto proposal = generateProposalNumber();
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Get or create instance for this slot
    auto& instance = instances_[slot];
    instance.slot = slot;
    instance.prepare_promises.clear();
    
    spdlog::debug("Node {} executing prepare phase for slot {} with proposal {}/{}",
                 node_id_, slot, proposal.round, proposal.node_id);
    
    total_prepares_++;
    
    // In a single-node simulation, we always promise to ourselves
    // In multi-node: Send prepare(proposal) to all acceptors
    instance.prepare_promises.insert(node_id_);
    
    // Simulate timeout for collecting promises
    auto start_time = std::chrono::steady_clock::now();
    
    // For now, simulate other nodes accepting
    // In real implementation: Send RPC with timeout and wait for quorum of promises
    for (const auto& node : cluster_nodes_) {
        if (node != node_id_) {
            // Check if we've exceeded timeout
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > config_.paxos_prepare_timeout) {
                spdlog::warn("Node {} prepare phase timed out for slot {} after {}ms",
                           node_id_, slot,
                           std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                break;
            }
            
            // Simulate promise from other nodes
            // In production: Send RPC and wait for response
            instance.prepare_promises.insert(node);
        }
    }
    
    // Check if we have quorum
    if (!hasQuorum(instance.prepare_promises.size())) {
        spdlog::warn("Node {} failed to get quorum in prepare phase for slot {} ({}/{})",
                    node_id_, slot, instance.prepare_promises.size(), cluster_nodes_.size());
        return false;
    }
    
    spdlog::debug("Node {} got quorum ({}/{}) in prepare phase for slot {}",
                 node_id_, instance.prepare_promises.size(), 
                 cluster_nodes_.size(), slot);
    
    // Phase 1b complete: We have quorum of promises
    // If any acceptor returned a previously accepted value, use the one with highest ballot
    // For now, we use our proposed value since we're simulating
    
    // Move to accept phase
    return executeAcceptPhase(slot, proposal, value);
}

bool PaxosConsensus::executeAcceptPhase(
    uint64_t slot,
    const ProposalNumber& proposal,
    const ConsensusLogEntry& value
) {
    // Phase 2a: Send accept requests to all acceptors
    auto start_time = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        auto& instance = instances_[slot];
        instance.accept_acks.clear();
        
        spdlog::debug("Node {} executing accept phase for slot {} with proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
    }
    
    total_accepts_++;
    
    // Accept on self (we're also an acceptor)
    if (handleAccept(slot, proposal, value)) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        instances_[slot].accept_acks.insert(node_id_);
    }
    
    // In real implementation: Send accept(proposal, value) to all acceptors via RPC
    // For now, simulate other nodes accepting with timeout
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto& instance = instances_[slot];
        
        for (const auto& node : cluster_nodes_) {
            if (node != node_id_) {
                // Check if we've exceeded timeout
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed > config_.paxos_accept_timeout) {
                    spdlog::warn("Node {} accept phase timed out for slot {} after {}ms",
                               node_id_, slot,
                               std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                    break;
                }
                
                // Simulate acceptance from other nodes
                // In production: Send RPC and wait for response
                instance.accept_acks.insert(node);
            }
        }
        
        // Phase 2b: Check if we have quorum of accepts
        if (!hasQuorum(instance.accept_acks.size())) {
            spdlog::warn("Node {} failed to get quorum in accept phase for slot {} ({}/{})",
                        node_id_, slot, instance.accept_acks.size(), cluster_nodes_.size());
            return false;
        }
        
        spdlog::debug("Node {} got quorum ({}/{}) in accept phase for slot {}",
                     node_id_, instance.accept_acks.size(),
                     cluster_nodes_.size(), slot);
        
        // Quorum reached - value is chosen
        instance.is_committed = true;
        instance.accepted_value = value;
        instance.accepted_proposal = proposal;
    }
    
    // Broadcast commit to all learners
    broadcastCommit(slot, value);
    
    return true;
}

void PaxosConsensus::broadcastCommit(uint64_t slot, const ConsensusLogEntry& value) {
    {
        std::lock_guard<std::mutex> lock(proposal_mutex_);
        committed_log_[slot] = value;
        uint64_t current_commit = commit_index_.load();
        if (slot > current_commit) {
            commit_index_.store(slot);
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
    // Increment round and generate unique proposal number
    uint64_t round = ++current_round_;
    return ProposalNumber{round, node_id_};
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

bool PaxosConsensus::handlePrepare(uint64_t slot, const ProposalNumber& proposal) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Get or create instance for this slot
    auto& instance = instances_[slot];
    
    // Phase 1b: Promise not to accept proposals with lower ballot
    if (proposal > instance.promised_proposal) {
        // Update promised proposal
        instance.promised_proposal = proposal;
        
        spdlog::debug("Node {} promised slot {} to proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
        
        // Return true (promise granted)
        // In a real implementation, we would also return the previously accepted value
        return true;
    }
    
    // Reject if we've already promised to a higher proposal
    spdlog::debug("Node {} rejected prepare for slot {} - already promised to higher proposal",
                 node_id_, slot);
    return false;
}

bool PaxosConsensus::handleAccept(
    uint64_t slot,
    const ProposalNumber& proposal,
    const ConsensusLogEntry& value
) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Get or create instance for this slot
    auto& instance = instances_[slot];
    
    // Phase 2b: Accept proposal if it's >= our promised proposal
    if (proposal >= instance.promised_proposal) {
        // Accept the proposal
        instance.accepted_proposal = proposal;
        instance.accepted_value = value;
        
        spdlog::debug("Node {} accepted slot {} with proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
        
        return true;
    }
    
    // Reject if proposal number is lower than promised
    spdlog::debug("Node {} rejected accept for slot {} - proposal too low",
                 node_id_, slot);
    return false;
}

void PaxosConsensus::handleCommit(uint64_t slot, const ConsensusLogEntry& value) {
    broadcastCommit(slot, value);
}

} // namespace sharding
} // namespace themisdb
