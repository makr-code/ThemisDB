/**
 * @file raft_shard_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_shard_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themisdb {
namespace sharding {

/** @brief Initialize per-shard Raft manager and log startup configuration. */
RaftShardManager::RaftShardManager(const Config& config)
    : config_(config) {
    spdlog::info("RaftShardManager initialized with replication_factor={}",
                 config_.replication_factor);
}

/** @brief Stop all active shard Raft instances before destruction. */
RaftShardManager::~RaftShardManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Stop all Raft instances
    for (auto& [shard_id, raft] : raft_instances_) {
        if (raft) {
            spdlog::info("Stopping Raft instance for shard: {}", shard_id);
            raft->stop();
        }
    }
    
    raft_instances_.clear();
}

/**
 * @brief Create and optionally auto-start Raft instance for shard.
 * @param shard_id Shard identifier.
 * @param replica_ids Replica IDs in shard Raft group.
 * @return True when instance creation succeeds.
 */
bool RaftShardManager::initializeShard(const std::string& shard_id,
                                      const std::vector<std::string>& replica_ids) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already initialized
    if (raft_instances_.find(shard_id) != raft_instances_.end()) {
        spdlog::warn("Raft instance already exists for shard: {}", shard_id);
        return false;
    }
    
    // Validate replica count
    if (replica_ids.size() < config_.replication_factor) {
        spdlog::error("Insufficient replicas for shard {}: got {}, expected {}",
                     shard_id, replica_ids.size(), config_.replication_factor);
        return false;
    }
    
    // Create Raft configuration
    auto raft_config = createRaftConfig(shard_id, replica_ids);
    
    // Create Raft instance
    try {
        auto raft = std::make_shared<RaftConsensus>(raft_config);
        raft_instances_[shard_id] = raft;
        
        spdlog::info("Initialized Raft instance for shard: {} with {} replicas",
                    shard_id, replica_ids.size());
        
        // Auto-start if enabled
        if (config_.enable_auto_start) {
            raft->start();
            spdlog::info("Auto-started Raft instance for shard: {}", shard_id);
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize Raft for shard {}: {}", shard_id, e.what());
        return false;
    }
}

/** @brief Stop and erase Raft instance associated with shard ID. */
void RaftShardManager::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it != raft_instances_.end()) {
        if (it->second) {
            it->second->stop();
        }
        raft_instances_.erase(it);
        spdlog::info("Removed Raft instance for shard: {}", shard_id);
    }
}

/** @brief Start already initialized Raft instance for shard. */
bool RaftShardManager::startShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        spdlog::error("No Raft instance found for shard: {}", shard_id);
        return false;
    }
    
    try {
        it->second->start();
        spdlog::info("Started Raft instance for shard: {}", shard_id);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start Raft for shard {}: {}", shard_id, e.what());
        return false;
    }
}

/** @brief Stop running Raft instance for shard when present. */
void RaftShardManager::stopShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it != raft_instances_.end() && it->second) {
        it->second->stop();
        spdlog::info("Stopped Raft instance for shard: {}", shard_id);
    }
}

/** @brief Return whether local node is current leader for shard. */
bool RaftShardManager::isShardLeader(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return false;
    }
    
    return it->second->isLeader();
}

/** @brief Return known leader ID for shard or empty string if unavailable. */
std::string RaftShardManager::getShardLeader(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return "";
    }
    
    return it->second->getLeaderId();
}

/** @brief Propose replicated write command on shard Raft instance. */
std::future<bool> RaftShardManager::proposeWrite(const std::string& shard_id,
                                                 const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    
    return it->second->propose(command);
}

/** @brief Build Raft runtime info snapshot for one shard. */
std::optional<ShardRaftInfo> RaftShardManager::getShardRaftInfo(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return std::nullopt;
    }
    
    const auto& raft = it->second;
    const auto& raft_state = raft->getRaftState();
    const auto& raft_log = raft_state.getLog();
    
    ShardRaftInfo info;
    info.shard_id = shard_id;
    info.role = raft_state.getState();
    info.term = raft->getCurrentTerm();
    info.commit_index = raft_log.getCommitIndex();
    // Note: Using commit_index as proxy for last_applied since RaftLog doesn't expose last_applied separately
    // In practice, last_applied trails commit_index but this is sufficient for monitoring
    info.last_applied = raft_log.getCommitIndex();
    info.leader_id = raft->getLeaderId();
    info.has_quorum = raft->hasQuorum();
    info.is_healthy = raft->hasQuorum() && !raft->isReadOnly();
    info.last_heartbeat = std::chrono::steady_clock::now();
    
    // Get replica states
    auto replica_states = raft->getReplicaStates();
    for (const auto& replica : replica_states) {
        info.replica_ids.push_back(replica.node_id);
    }
    
    return info;
}

/** @brief Build Raft runtime info snapshots for all managed shards. */
std::map<std::string, ShardRaftInfo> RaftShardManager::getAllShardRaftInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::map<std::string, ShardRaftInfo> all_info;
    
    for (const auto& [shard_id, raft] : raft_instances_) {
        if (!raft) {
          continue;
        }
        
        const auto& raft_state = raft->getRaftState();
        const auto& raft_log = raft_state.getLog();
        
        ShardRaftInfo info;
        info.shard_id = shard_id;
        info.role = raft_state.getState();
        info.term = raft->getCurrentTerm();
        info.commit_index = raft_log.getCommitIndex();
        // Note: Using commit_index as proxy for last_applied since RaftLog doesn't expose last_applied separately
        // In practice, last_applied trails commit_index but this is sufficient for monitoring
        info.last_applied = raft_log.getCommitIndex();
        info.leader_id = raft->getLeaderId();
        info.has_quorum = raft->hasQuorum();
        info.is_healthy = raft->hasQuorum() && !raft->isReadOnly();
        info.last_heartbeat = std::chrono::steady_clock::now();
        
        // Get replica states
        auto replica_states = raft->getReplicaStates();
        for (const auto& replica : replica_states) {
            info.replica_ids.push_back(replica.node_id);
        }
        
        all_info[shard_id] = info;
    }
    
    return all_info;
}

/** @brief Return quorum availability for shard. */
bool RaftShardManager::hasQuorum(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return false;
    }
    
    return it->second->hasQuorum();
}

/** @brief Return shared pointer to shard Raft instance when available. */
std::shared_ptr<RaftConsensus> RaftShardManager::getRaftInstance(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it != raft_instances_.end()) {
        return it->second;
    }
    
    return nullptr;
}

/**
 * @brief Create shard-specific Raft config derived from manager defaults.
 * @param shard_id Shard identifier used as local Raft node ID.
 * @param replica_ids Replica members participating in shard consensus.
 * @return Ready-to-use Raft consensus configuration.
 */
RaftConsensus::Config RaftShardManager::createRaftConfig(
    const std::string& shard_id,
    const std::vector<std::string>& replica_ids) {
    
    RaftConsensus::Config raft_config = config_.raft_config;
    
    // Set node ID to shard ID
    raft_config.raft_config.node_id = shard_id;
    
    // Set cluster members (all replicas including self)
    raft_config.raft_config.cluster_members = replica_ids;
    
    // Enable partition detection for production use
    raft_config.enable_partition_detection = true;
    raft_config.enable_split_brain_prevention = true;
    
    return raft_config;
}

}  // namespace sharding
}  // namespace themisdb
