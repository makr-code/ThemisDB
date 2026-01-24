// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_consensus_adapter.h"
#include <spdlog/spdlog.h>

namespace themisdb {
namespace sharding {

RaftConsensusAdapter::RaftConsensusAdapter(const ConsensusConfig& config)
    : config_(config)
    , current_state_(ConsensusState::FOLLOWER)
{
}

RaftConsensusAdapter::~RaftConsensusAdapter() {
    stop();
}

bool RaftConsensusAdapter::initialize(
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    node_id_ = node_id;
    cluster_nodes_ = cluster_nodes;
    
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

void RaftConsensusAdapter::stop() {
    if (raft_) {
        raft_->stop();
    }
}

bool RaftConsensusAdapter::isLeader() const {
    return raft_ && raft_->isLeader();
}

std::string RaftConsensusAdapter::getLeaderId() const {
    return raft_ ? raft_->getLeaderId() : "";
}

ConsensusState RaftConsensusAdapter::getState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_state_;
}

ConsensusState RaftConsensusAdapter::convertState(RaftState state) {
    // Note: RaftState is a class, not enum in the actual implementation
    // This is a placeholder - actual implementation would need to check
    // the state from raft_->getRaftState()
    return ConsensusState::FOLLOWER;  // Placeholder
}

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
        
        // Propose to Raft
        auto future = raft_->propose(command);
        
        // For now, we'll use a simple counter for log index
        // In production, this should come from Raft
        static std::atomic<uint64_t> log_index_counter{1};
        return log_index_counter++;
    } catch (const std::exception& e) {
        spdlog::error("Failed to propose operation: {}", e.what());
        return std::nullopt;
    }
}

bool RaftConsensusAdapter::waitForCommit(
    uint64_t log_index,
    std::chrono::milliseconds timeout
) {
    // Placeholder implementation
    // In production, this should wait for the specific log index to be committed
    std::this_thread::sleep_for(std::min(timeout, std::chrono::milliseconds(100)));
    return true;
}

std::vector<ConsensusLogEntry> RaftConsensusAdapter::readLog(
    uint64_t start_index,
    std::optional<uint64_t> end_index
) {
    // Placeholder implementation
    // In production, this should read from Raft's log
    return {};
}

uint64_t RaftConsensusAdapter::getCommitIndex() const {
    // Placeholder implementation
    return 0;
}

bool RaftConsensusAdapter::addNode(
    const std::string& node_id,
    const std::string& endpoint
) {
    // Raft membership changes would go here
    spdlog::warn("Dynamic membership changes not yet implemented in adapter");
    return false;
}

bool RaftConsensusAdapter::removeNode(const std::string& node_id) {
    spdlog::warn("Dynamic membership changes not yet implemented in adapter");
    return false;
}

bool RaftConsensusAdapter::transferLeadership(const std::string& target_node_id) {
    spdlog::warn("Leadership transfer not yet implemented in adapter");
    return false;
}

bool RaftConsensusAdapter::takeSnapshot(const nlohmann::json& snapshot_data) {
    spdlog::warn("Snapshot not yet implemented in adapter");
    return false;
}

bool RaftConsensusAdapter::restoreSnapshot(const nlohmann::json& snapshot_data) {
    spdlog::warn("Snapshot restore not yet implemented in adapter");
    return false;
}

ConsensusStats RaftConsensusAdapter::getStats() const {
    ConsensusStats stats{};
    
    if (raft_) {
        stats.current_term = raft_->getCurrentTerm();
        stats.state = getState();
        stats.current_leader = raft_->getLeaderId();
        stats.cluster_size = cluster_nodes_.size();
        
        auto partition_status = raft_->getPartitionStatus();
        stats.reachable_nodes = partition_status.reachable_nodes.size();
    }
    
    return stats;
}

nlohmann::json RaftConsensusAdapter::getStatus() const {
    auto stats = getStats();
    
    return {
        {"type", "Raft"},
        {"node_id", node_id_},
        {"is_leader", isLeader()},
        {"leader_id", stats.current_leader},
        {"state", static_cast<int>(stats.state)},
        {"current_term", stats.current_term},
        {"cluster_size", stats.cluster_size},
        {"reachable_nodes", stats.reachable_nodes}
    };
}

void RaftConsensusAdapter::onCommit(
    std::function<void(const ConsensusLogEntry&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_commit_callback_ = std::move(callback);
}

void RaftConsensusAdapter::onStateChange(
    std::function<void(ConsensusState, ConsensusState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

void RaftConsensusAdapter::onLeaderChange(
    std::function<void(const std::string&, const std::string&)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_leader_change_callback_ = std::move(callback);
}

ConsensusLogEntry RaftConsensusAdapter::convertLogEntry(const LogEntry& entry) {
    ConsensusLogEntry consensus_entry;
    consensus_entry.index = entry.index;
    consensus_entry.term = entry.term;
    
    try {
        // Parse command JSON
        auto command_json = nlohmann::json::parse(entry.command);
        consensus_entry.operation = command_json.value("operation", "");
        consensus_entry.data = command_json.value("data", nlohmann::json{});
    } catch (const std::exception& e) {
        // If parsing fails, store raw command
        consensus_entry.operation = "raw";
        consensus_entry.data = {{"command", entry.command}};
    }
    
    return consensus_entry;
}

} // namespace sharding
} // namespace themisdb
