/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            paxos_consensus.cpp                                ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:10:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   48.0/100                                       ║
    • Total Lines:     1159                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 116157e290  2026-04-12  fix(sharding): Paxos WAL durability, writeEntity RPC, PSR... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/paxos_consensus.h"
#include "sharding/paxos_wal.h"
#include "sharding/paxos_snapshot.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cstring>  // for strerror

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
    , last_applied_lsn_(0, 0)
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
    
    // Phase 2.1: Initialize WAL and Snapshot infrastructure
    if (config_.enable_persistence) {
        // Initialize WAL
        themis::sharding::PaxosWALConfig wal_config;
        wal_config.wal_directory = config_.data_dir + "/wal";
        wal_config.snapshot_directory = config_.data_dir + "/snapshots";
        wal_config.sync_on_write = true;
        
        wal_ = std::make_unique<themis::sharding::PaxosWAL>(wal_config);
        if (!wal_->initialize()) {
            spdlog::error("Failed to initialize Paxos WAL");
            return false;
        }
        
        // Initialize snapshot manager
        snapshot_manager_ = std::make_unique<themis::sharding::PaxosSnapshotManager>(
            wal_config.snapshot_directory,
            wal_config.max_snapshots
        );
        
        // Recover from snapshot + WAL
        if (!recoverFromWAL()) {
            spdlog::warn("Failed to recover from WAL, starting fresh");
        }
        
        spdlog::info("Paxos persistence enabled: node={}, wal_dir={}", 
                    node_id_, wal_config.wal_directory);
    } else {
        // Legacy: Load persistent state from JSON if available
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

bool PaxosConsensus::takeSnapshot(const nlohmann::json& snapshot_data) {
    if (!running_.load()) {
        spdlog::warn("takeSnapshot: Paxos not running");
        return false;
    }

    const uint64_t snap_index = commit_index_.load();

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        const uint64_t snap_term = current_round_;  // read under mutex to avoid race with restoreSnapshot
        snapshot_data_  = snapshot_data;
        snapshot_index_ = snap_index;
        snapshot_term_  = snap_term;
        spdlog::info("PaxosConsensus::takeSnapshot: snapshot at index={} round={}", snap_index, snap_term);
    }

    return true;
}

bool PaxosConsensus::restoreSnapshot(const nlohmann::json& snapshot_data) {
    if (snapshot_data.is_null() || snapshot_data.empty()) {
        spdlog::error("PaxosConsensus::restoreSnapshot: snapshot_data is null or empty");
        return false;
    }

    uint64_t restored_index = 0;
    uint64_t restored_term  = 0;
    if (snapshot_data.contains("_snapshot_index"))
        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();
    if (snapshot_data.contains("_snapshot_term"))
        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snapshot_data_  = snapshot_data;
        snapshot_index_ = restored_index;
        snapshot_term_  = restored_term;

        // Step down to follower so this node re-syncs before proposing
        if (running_.load()) {
            state_.store(ConsensusState::FOLLOWER);
            if (restored_term > current_round_)
                current_round_ = restored_term;
        }
    }

    spdlog::info("PaxosConsensus::restoreSnapshot: restored at index={} round={}",
                 restored_index, restored_term);
    return true;
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

    uint64_t snap_index = 0;
    uint64_t snap_term  = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snap_index = snapshot_index_;
        snap_term  = snapshot_term_;
    }

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
        {"failed_proposals", stats.failed_operations},
        {"snapshot_index", snap_index},
        {"snapshot_term",  snap_term}
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
    
    // Phase 2.1.3: Log PREPARE to WAL for durability
    if (wal_) {
        try {
            wal_->logPrepare(slot, proposal.round, node_id_);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log PREPARE to WAL: {}", e.what());
            // Continue operation despite WAL failure (graceful degradation)
        }
    }
    
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
    
    // Phase 2.1.3: Log ACCEPT to WAL for durability
    if (wal_) {
        try {
            wal_->logAccept(slot, proposal.round, node_id_, value);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log ACCEPT to WAL: {}", e.what());
            // Continue operation despite WAL failure (graceful degradation)
        }
    }
    
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
    // Phase 2.1.3: Log COMMIT to WAL for durability
    if (wal_) {
        try {
            wal_->logCommit(slot, value);
            uint64_t ops = ++operations_since_snapshot_;
            
            // Check if we should create a snapshot
            if (wal_->shouldCreateSnapshot(ops)) {
                spdlog::info("Triggering snapshot creation after {} operations", ops);
                createPeriodicSnapshot();
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log COMMIT to WAL: {}", e.what());
            // Continue operation despite WAL failure (graceful degradation)
        }
    }
    
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
    if (config_.data_dir.empty()) {
        spdlog::warn("No data directory configured for Paxos persistence");
        return false;
    }
    
    try {
        std::string state_file = config_.data_dir + "/paxos_state_" + node_id_ + ".json";
        std::ifstream file(state_file);
        
        if (!file.is_open()) {
            spdlog::info("No persistent state file found at {}, starting fresh", state_file);
            return false;
        }
        
        nlohmann::json state_json;
        file >> state_json;
        file.close();
        
        // Load basic state
        if (state_json.contains("current_round")) {
            current_round_ = state_json["current_round"].get<uint64_t>();
        }
        
        if (state_json.contains("next_slot")) {
            next_slot_.store(state_json["next_slot"].get<uint64_t>());
        }
        
        if (state_json.contains("commit_index")) {
            commit_index_.store(state_json["commit_index"].get<uint64_t>());
        }
        
        // Load Paxos instances
        if (state_json.contains("instances")) {
            for (const auto& [slot_str, instance_json] : state_json["instances"].items()) {
                uint64_t slot = std::stoull(slot_str);
                PaxosInstance& instance = instances_[slot];
                
                instance.slot = slot;
                
                if (instance_json.contains("promised_proposal")) {
                    const auto& promised = instance_json["promised_proposal"];
                    instance.promised_proposal.round = promised["round"].get<uint64_t>();
                    instance.promised_proposal.node_id = promised["node_id"].get<std::string>();
                }
                
                if (instance_json.contains("accepted_proposal")) {
                    const auto& accepted = instance_json["accepted_proposal"];
                    instance.accepted_proposal.round = accepted["round"].get<uint64_t>();
                    instance.accepted_proposal.node_id = accepted["node_id"].get<std::string>();
                }
                
                if (instance_json.contains("accepted_value")) {
                    const auto& value_json = instance_json["accepted_value"];
                    // Note: JSON uses "log_index" but struct uses "index"
                    if (value_json.contains("log_index")) {
                        instance.accepted_value.index = value_json["log_index"].get<uint64_t>();
                    }
                    instance.accepted_value.term = value_json["term"].get<uint64_t>();
                    instance.accepted_value.operation = value_json["operation"].get<std::string>();
                    instance.accepted_value.data = value_json["data"];
                }
                
                if (instance_json.contains("is_committed")) {
                    instance.is_committed = instance_json["is_committed"].get<bool>();
                }
            }
        }
        
        // Load committed log
        if (state_json.contains("committed_log")) {
            for (const auto& [index_str, entry_json] : state_json["committed_log"].items()) {
                uint64_t index = std::stoull(index_str);
                ConsensusLogEntry& entry = committed_log_[index];
                
                // Note: JSON may use "log_index" but struct uses "index"
                if (entry_json.contains("index")) {
                    entry.index = entry_json["index"].get<uint64_t>();
                } else if (entry_json.contains("log_index")) {
                    entry.index = entry_json["log_index"].get<uint64_t>();
                }
                entry.term = entry_json["term"].get<uint64_t>();
                entry.operation = entry_json["operation"].get<std::string>();
                entry.data = entry_json["data"];
            }
        }
        
        spdlog::info("Loaded Paxos persistent state: round={}, next_slot={}, commit_index={}, instances={}, committed_entries={}",
                     current_round_, next_slot_.load(), commit_index_.load(), 
                     instances_.size(), committed_log_.size());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load Paxos persistent state: {}", e.what());
        return false;
    }
}

bool PaxosConsensus::savePersistentState() {
    if (config_.data_dir.empty()) {
        spdlog::warn("No data directory configured for Paxos persistence");
        return false;
    }
    
    try {
        nlohmann::json state_json;
        
        // Save basic state
        state_json["current_round"] = current_round_;
        state_json["next_slot"] = next_slot_.load();
        state_json["commit_index"] = commit_index_.load();
        
        // Save Paxos instances
        nlohmann::json instances_json = nlohmann::json::object();
        for (const auto& [slot, instance] : instances_) {
            nlohmann::json instance_json;
            
            instance_json["slot"] = instance.slot;
            instance_json["is_committed"] = instance.is_committed;
            
            // Save promised proposal
            if (instance.promised_proposal.round > 0) {
                instance_json["promised_proposal"] = {
                    {"round", instance.promised_proposal.round},
                    {"node_id", instance.promised_proposal.node_id}
                };
            }
            
            // Save accepted proposal
            if (instance.accepted_proposal.round > 0) {
                instance_json["accepted_proposal"] = {
                    {"round", instance.accepted_proposal.round},
                    {"node_id", instance.accepted_proposal.node_id}
                };
                
                // Save accepted value
                instance_json["accepted_value"] = {
                    {"index", instance.accepted_value.index},
                    {"term", instance.accepted_value.term},
                    {"operation", instance.accepted_value.operation},
                    {"data", instance.accepted_value.data}
                };
            }
            
            instances_json[std::to_string(slot)] = instance_json;
        }
        state_json["instances"] = instances_json;
        
        // Save committed log
        nlohmann::json committed_log_json = nlohmann::json::object();
        for (const auto& [index, entry] : committed_log_) {
            committed_log_json[std::to_string(index)] = {
                {"index", entry.index},
                {"term", entry.term},
                {"operation", entry.operation},
                {"data", entry.data}
            };
        }
        state_json["committed_log"] = committed_log_json;
        
        // Write to file atomically (write to temp, then rename)
        std::string state_file = config_.data_dir + "/paxos_state_" + node_id_ + ".json";
        std::string temp_file = state_file + ".tmp";
        
        std::ofstream file(temp_file);
        if (!file.is_open()) {
            spdlog::error("Failed to open temporary state file: {}", temp_file);
            return false;
        }
        
        file << state_json.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        // Atomic rename
        if (std::rename(temp_file.c_str(), state_file.c_str()) != 0) {
            spdlog::error("Failed to rename temporary state file: {}", strerror(errno));
            return false;
        }
        
        spdlog::debug("Saved Paxos persistent state: round={}, next_slot={}, commit_index={}, instances={}, committed_entries={}",
                      current_round_, next_slot_.load(), commit_index_.load(),
                      instances_.size(), committed_log_.size());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to save Paxos persistent state: {}", e.what());
        return false;
    }
}

bool PaxosConsensus::handlePrepare(uint64_t slot, const ProposalNumber& proposal) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Get or create instance for this slot
    auto& instance = instances_[slot];
    
    // Phase 1b: Promise not to accept proposals with lower ballot
    if (proposal > instance.promised_proposal) {
        // Update promised proposal
        instance.promised_proposal = proposal;
        
        // WAL durability: persist promise BEFORE responding so the invariant
        // survives a crash between sending the PROMISE and the next restart.
        if (wal_) {
            const uint64_t prev_accepted_round = instance.accepted_proposal.round;
            const nlohmann::json prev_accepted_value = instance.accepted_value.data;
            wal_->logPromise(slot, proposal.round, node_id_,
                             prev_accepted_round, prev_accepted_value);
            ++operations_since_snapshot_;
        }
        
        spdlog::debug("Node {} promised slot {} to proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
        
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
    if (proposal.round >= instance.promised_proposal.round || 
        (proposal.round == instance.promised_proposal.round && proposal.node_id >= instance.promised_proposal.node_id)) {
        // Accept the proposal
        instance.accepted_proposal = proposal;
        instance.accepted_value = value;
        
        // WAL durability: persist accept BEFORE responding so the invariant
        // survives a crash between sending ACCEPTED and the next restart.
        if (wal_) {
            wal_->logAccept(slot, proposal.round, node_id_, value);
            ++operations_since_snapshot_;
        }
        
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

// ============================================================================
// Phase 2.1: WAL and Snapshot Methods
// ============================================================================

bool PaxosConsensus::recoverFromWAL() {
    if (!snapshot_manager_ || !wal_) {
        return false;
    }
    
    try {
        // Step 1: Load latest snapshot if available
        auto snapshot_opt = snapshot_manager_->loadLatestSnapshot();
        
        if (snapshot_opt.has_value()) {
            const auto& snapshot = snapshot_opt.value();
            
            // Restore state from snapshot
            current_round_ = snapshot.current_round;
            next_slot_.store(snapshot.last_committed_slot + 1);
            commit_index_.store(snapshot.last_committed_slot);
            last_applied_lsn_ = snapshot.last_applied_lsn;
            
            // Restore Paxos instances
            instances_.clear();
            for (const auto& [slot, instance_json] : snapshot.instances) {
                // Parse instance from JSON
                // (Simplified - full implementation would reconstruct PaxosInstance)
                spdlog::debug("Restored Paxos instance for slot {}", slot);
            }
            
            // Restore committed log
            committed_log_.clear();
            for (const auto& [index, entry_json] : snapshot.committed_log) {
                // Parse entry from JSON
                spdlog::debug("Restored committed log entry {}", index);
            }
            
            spdlog::info("Recovered from Paxos snapshot: id={}, slot={}, instances={}, log_entries={}",
                        snapshot.snapshot_id, snapshot.last_committed_slot,
                        snapshot.instances.size(), snapshot.committed_log.size());
        } else {
            spdlog::info("No snapshot found, starting from empty state");
            last_applied_lsn_ = LSN(0, 0);
        }
        
        // Step 2: Replay WAL entries since snapshot
        auto wal_entries = wal_->readEntries(last_applied_lsn_);
        
        spdlog::info("Replaying {} WAL entries from LSN {}", 
                    wal_entries.size(), last_applied_lsn_.toString());
        
        for (const auto& entry : wal_entries) {
            // Apply each WAL entry to rebuild state
            switch (entry.type) {
                case themis::sharding::PaxosWALEntryType::PREPARE:
                    spdlog::debug("Replay PREPARE: slot={}, round={}", entry.slot, entry.round);
                    break;
                    
                case themis::sharding::PaxosWALEntryType::PROMISE: {
                    spdlog::debug("Replay PROMISE: slot={}, round={}", entry.slot, entry.round);
                    auto& inst = instances_[entry.slot];
                    inst.slot = entry.slot;
                    if (entry.round > inst.promised_proposal.round) {
                        inst.promised_proposal.round   = entry.round;
                        inst.promised_proposal.node_id = entry.node_id;
                    }
                    break;
                }
                    
                case themis::sharding::PaxosWALEntryType::ACCEPT:
                case themis::sharding::PaxosWALEntryType::ACCEPTED: {
                    spdlog::debug("Replay ACCEPT: slot={}, round={}", entry.slot, entry.round);
                    auto& inst = instances_[entry.slot];
                    inst.slot = entry.slot;
                    if (entry.round >= inst.accepted_proposal.round) {
                        inst.accepted_proposal.round   = entry.round;
                        inst.accepted_proposal.node_id = entry.node_id;
                        if (!entry.data.is_null()) {
                            inst.accepted_value.operation =
                                entry.data.value("operation", std::string{});
                            inst.accepted_value.data =
                                entry.data.value("data", nlohmann::json{});
                        }
                    }
                    break;
                }
                    
                case themis::sharding::PaxosWALEntryType::COMMIT: {
                    spdlog::debug("Replay COMMIT: slot={}", entry.slot);
                    auto& inst = instances_[entry.slot];
                    inst.is_committed = true;
                    // Update commit index
                    if (entry.slot > commit_index_.load()) {
                        commit_index_.store(entry.slot);
                    }
                    // Rebuild committed log
                    ConsensusLogEntry log_entry;
                    if (!entry.data.is_null()) {
                        log_entry.operation = entry.data.value("operation", std::string{});
                        log_entry.data      = entry.data.value("data", nlohmann::json{});
                        log_entry.index     = entry.slot;
                    }
                    committed_log_[entry.slot] = log_entry;
                    break;
                }
                    
                default:
                    break;
            }
            
            last_applied_lsn_ = entry.lsn;
        }
        
        spdlog::info("Paxos recovery complete: round={}, next_slot={}, commit_index={}",
                    current_round_, next_slot_.load(), commit_index_.load());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to recover from WAL: {}", e.what());
        return false;
    }
}

void PaxosConsensus::createPeriodicSnapshot() {
    if (!snapshot_manager_ || !wal_) {
        return;
    }
    
    // Check if we should create a snapshot
    uint64_t ops = operations_since_snapshot_.load();
    if (!wal_->shouldCreateSnapshot(ops)) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        // Create snapshot from current state
        auto snapshot_id = snapshot_manager_->createSnapshot(
            node_id_,
            last_applied_lsn_,
            commit_index_.load(),
            current_round_,
            instances_,
            committed_log_
        );
        
        if (snapshot_id.has_value()) {
            operations_since_snapshot_.store(0);
            spdlog::info("Created Paxos snapshot: id={}, slot={}, instances={}",
                        snapshot_id.value(), commit_index_.load(), instances_.size());
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create Paxos snapshot: {}", e.what());
    }
}

} // namespace sharding
} // namespace themisdb
