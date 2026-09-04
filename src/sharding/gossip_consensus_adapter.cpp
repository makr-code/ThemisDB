/**
 * @file gossip_consensus_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=5, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/gossip_consensus_adapter.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>

namespace themisdb {
namespace sharding {

GossipConsensusAdapter::GossipConsensusAdapter(const ConsensusConfig& config)
    : config_(config)
    , running_(false)
    , next_log_index_(1)
    , commit_index_(0)
    , total_operations_(0)
    , failed_operations_(0)
{
}

GossipConsensusAdapter::~GossipConsensusAdapter() {
    stop();
}

bool GossipConsensusAdapter::initialize(
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    node_id_ = node_id;
    cluster_nodes_ = cluster_nodes;
    
    try {
        // Initialize gossip protocol with default config
        themis::sharding::GossipConfig gossip_config;
        auto topology = std::make_shared<themis::sharding::ShardTopology>();
        gossip_ = std::make_unique<themis::sharding::GossipProtocol>(gossip_config, topology);
        
        // Initialize distributed coordinator
        themis::sharding::GossipConfigManagerConfig mgr_config;
        auto gossip_mgr = std::make_shared<themis::sharding::GossipConfigManager>(mgr_config, topology);
        coordinator_ = std::make_unique<themis::sharding::DistributedCoordinator>(
            node_id, topology, gossip_mgr);
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize Gossip consensus: {}", e.what());
        return false;
    }
}

bool GossipConsensusAdapter::start() {
    if (running_.load()) {
        spdlog::warn("Gossip consensus already running");
        return false;
    }
    
    running_.store(true);
    
    // Start gossip thread
    gossip_thread_ = std::thread(&GossipConsensusAdapter::gossipThread, this);
    
    spdlog::info("Gossip consensus started for node {}", node_id_);
    return true;
}

void GossipConsensusAdapter::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    gossip_cv_.notify_all();
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(gossip_thread_)) {
        THEMIS_WARN("[GossipConsensusAdapter] gossip thread did not finish within shutdown deadline; detaching.");
    }
    
    spdlog::info("Gossip consensus stopped for node {}", node_id_);
}

bool GossipConsensusAdapter::isLeader() const {
    // Gossip is leaderless, so we'll return true if we're the "coordinator"
    // based on deterministic selection
    if (cluster_nodes_.empty()) {
      return false;
    }
    
    std::string expected_coordinator = *std::min_element(
        cluster_nodes_.begin(), cluster_nodes_.end()
    );
    
    return node_id_ == expected_coordinator;
}

std::string GossipConsensusAdapter::getLeaderId() const {
    if (cluster_nodes_.empty()) {
      return "";
    }
    
    // Return the deterministic "leader" (lowest node ID)
    return *std::min_element(cluster_nodes_.begin(), cluster_nodes_.end());
}

ConsensusState GossipConsensusAdapter::getState() const {
    // Gossip doesn't have traditional states
    // We'll return LEADER if we're the coordinator, FOLLOWER otherwise
    return isLeader() ? ConsensusState::LEADER : ConsensusState::FOLLOWER;
}

std::optional<uint64_t> GossipConsensusAdapter::propose(
    const std::string& operation,
    const nlohmann::json& data
) {
    if (!running_.load()) {
        spdlog::error("Cannot propose: Gossip not running");
        return std::nullopt;
    }
    
    // Create log entry
    ConsensusLogEntry entry;
    entry.index = next_log_index_++;
    entry.term = 0;  // Gossip doesn't use terms
    entry.operation = operation;
    entry.data = data;
    entry.timestamp = std::chrono::system_clock::now();
    
    // Add to log
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_entries_[entry.index] = entry;
        
        // W2-S06: Consensus validation — validate node_id before recording self-ack
        if (node_id_.empty()) {
            spdlog::error("GossipConsensusAdapter::appendEntry: node_id_ is empty, rejecting ack");
            return 0;
        }
        log_acknowledgments_[entry.index].insert(node_id_);  // Self-acknowledge
    }
    
    // Wake up gossip thread to propagate
    gossip_cv_.notify_one();
    total_operations_++;
    
    return entry.index;
}

bool GossipConsensusAdapter::waitForCommit(
    uint64_t log_index,
    std::chrono::milliseconds timeout
) {
    auto start = std::chrono::steady_clock::now();
    
    while (running_.load()) {
        if (hasReachedQuorum(log_index)) {
            return true;
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return false;
}

std::vector<ConsensusLogEntry> GossipConsensusAdapter::readLog(
    uint64_t start_index,
    std::optional<uint64_t> end_index
) {
    std::vector<ConsensusLogEntry> result;
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    uint64_t end = end_index.value_or(commit_index_.load());
    
    // FIXED: Add bounds validation to prevent unbounded iteration and data races
    if (start_index > end) {
        return result;  // Invalid range
    }
    
    // Cap iteration to avoid excessive memory use
    uint64_t max_entries = 1000;
    if (end - start_index + 1 > max_entries) {
        end = start_index + max_entries - 1;
    }
    
    for (uint64_t i = start_index; i <= end; ++i) {
        auto it = log_entries_.find(i);
        if (it != log_entries_.end() && hasReachedQuorumUnlocked(i)) {  // FIXED: Use unlocked version
            result.push_back(it->second);
        }
    }
    
    return result;
}

uint64_t GossipConsensusAdapter::getCommitIndex() const {
    return commit_index_.load();
}

uint64_t GossipConsensusAdapter::getLastLogIndex() const {
    uint64_t next = next_log_index_.load();
    // Return last assigned log index (next - 1)
    // If next is 0, return 0 to avoid underflow
    return next > 0 ? next - 1 : 0;
}

bool GossipConsensusAdapter::addNode(
    const std::string& node_id,
    const std::string& /*endpoint*/
) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // W2-S06: Consensus validation — validate node_id before adding to cluster
    if (node_id.empty()) {
        spdlog::error("GossipConsensusAdapter::addNode: node_id is empty, rejecting");
        return false;
    }
    
    if (std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id) != cluster_nodes_.end()) {
        spdlog::warn("Node {} already in cluster", node_id);
        return false;
    }
    
    cluster_nodes_.push_back(node_id);
    spdlog::info("Added node {} to cluster", node_id);
    return true;
}

bool GossipConsensusAdapter::removeNode(const std::string& node_id) {
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

bool GossipConsensusAdapter::transferLeadership(const std::string& /*target_node_id*/) {
    // Gossip is leaderless, so this is a no-op
    spdlog::info("Gossip is leaderless, leadership transfer not applicable");
    return true;
}

bool GossipConsensusAdapter::takeSnapshot(const nlohmann::json& snapshot_data) {
    if (!running_.load()) {
        spdlog::warn("takeSnapshot: Gossip not running");
        return false;
    }

    const uint64_t snap_index = commit_index_.load();
    // Gossip is termless; use 0 for term

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snapshot_data_  = snapshot_data;
        snapshot_index_ = snap_index;
        snapshot_term_  = 0;
    }

    spdlog::info("GossipConsensusAdapter::takeSnapshot: snapshot at index={}", snap_index);
    return true;
}

bool GossipConsensusAdapter::restoreSnapshot(const nlohmann::json& snapshot_data) {
    if (snapshot_data.is_null() || snapshot_data.empty()) {
        spdlog::error("GossipConsensusAdapter::restoreSnapshot: snapshot_data is null or empty");
        return false;
    }

    uint64_t restored_index = 0;
    if (snapshot_data.contains("_snapshot_index"))
        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snapshot_data_  = snapshot_data;
        snapshot_index_ = restored_index;
        snapshot_term_  = 0;  // Gossip has no term concept
    }

    spdlog::info("GossipConsensusAdapter::restoreSnapshot: restored at index={}", restored_index);
    return true;
}

ConsensusStats GossipConsensusAdapter::getStats() const {
    ConsensusStats stats{};
    stats.current_term = 0;  // Gossip doesn't use terms
    stats.commit_index = commit_index_.load();
    stats.last_applied = commit_index_.load();
    stats.state = getState();
    stats.current_leader = getLeaderId();
    stats.cluster_size = cluster_nodes_.size();
    stats.reachable_nodes = cluster_nodes_.size();  // Simplified
    stats.total_operations = total_operations_.load();
    stats.failed_operations = failed_operations_.load();
    return stats;
}

nlohmann::json GossipConsensusAdapter::getStatus() const {
    auto stats = getStats();

    uint64_t snap_index = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snap_index = snapshot_index_;
    }

    return {
        {"type", "Gossip"},
        {"node_id", node_id_},
        {"is_coordinator", isLeader()},
        {"coordinator_id", stats.current_leader},
        {"state", static_cast<int>(stats.state)},
        {"commit_index", stats.commit_index},
        {"cluster_size", stats.cluster_size},
        {"total_operations", stats.total_operations},
        {"snapshot_index", snap_index},
        {"snapshot_term",  0}
    };
}

void GossipConsensusAdapter::onCommit(
    std::function<void(const ConsensusLogEntry&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_commit_callback_ = std::move(callback);
}

void GossipConsensusAdapter::onStateChange(
    std::function<void(ConsensusState, ConsensusState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

void GossipConsensusAdapter::onLeaderChange(
    std::function<void(const std::string&, const std::string&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_leader_change_callback_ = std::move(callback);
}

// Private methods

void GossipConsensusAdapter::gossipThread() {
    spdlog::debug("Gossip thread started");
    
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(state_mutex_);
        
        // Wait for gossip interval
        gossip_cv_.wait_for(lock, config_.gossip_interval, [this] {
            return !running_.load();
        });
        
        if (!running_.load()) {
          break;
        }
        
        lock.unlock();
        
        // Simulate gossip propagation
        // FIXED: Collect entries first, release lock, then invoke callbacks to avoid deadlock
        std::vector<std::pair<uint64_t, ConsensusLogEntry>> ready_to_commit;
        {
            std::lock_guard<std::mutex> log_lock(log_mutex_);
            
            // Collect entries that have reached quorum (avoid iterator invalidation
            // and deadlock by not calling hasReachedQuorum which re-acquires lock)
            for (auto& [index, entry] : log_entries_) {
                if (index > commit_index_.load() && hasReachedQuorumUnlocked(index)) {
                    // W2-S06: Consensus validation — validate entry before marking for commit
                    if (entry.index == 0 || entry.data.empty()) {
                        spdlog::warn("gossipThread: entry {} has invalid index or data, skipping commit", entry.index);
                        continue;
                    }
                    ready_to_commit.push_back({index, entry});
                }
            }
        }  // Lock released here
        
        // Update commit index and invoke callbacks outside lock
        for (auto& [index, entry] : ready_to_commit) {
            commit_index_ = index;
            
            // Call commit callback without holding log_mutex_
            std::lock_guard<std::mutex> cb_lock(callbacks_mutex_);
            if (on_commit_callback_) {
                on_commit_callback_(entry);
            }
        }
    }
    
    spdlog::debug("Gossip thread stopped");
}

bool GossipConsensusAdapter::hasReachedQuorum(uint64_t log_index) const {
    std::lock_guard<std::mutex> lock(log_mutex_);
    return hasReachedQuorumUnlocked(log_index);
}

bool GossipConsensusAdapter::hasReachedQuorumUnlocked(uint64_t log_index) const {
    // FIXED: Helper method that doesn't acquire lock (caller must hold log_mutex_)
    auto it = log_acknowledgments_.find(log_index);
    if (it == log_acknowledgments_.end()) {
        return false;
    }
    
    // Calculate quorum (majority)
    size_t quorum_size = (cluster_nodes_.size() / 2) + 1;
    return static_cast<bool>(it- < static_cast<int>(second.size())) >= quorum_size;
}

} // namespace sharding
} // namespace themisdb

