/**
 * @file raft_shard_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_shard_manager.h"
#include "sharding/raft_two_phase_commit_decision_store.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

namespace themisdb {
namespace sharding {

RaftShardManager::RaftShardManager(const Config& config)
    : config_(config) {
    spdlog::info("RaftShardManager initialized with replication_factor={}",
                 config_.replication_factor);
}

RaftShardManager::~RaftShardManager() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Stop all Raft instances
    for (auto& [shard_id, raft] : raft_instances_) {
        if (raft) {
            spdlog::info("Stopping Raft instance for shard: {}", shard_id);
            raft->stop();
        }
    }
    
    raft_instances_.clear();
    active_decision_stores_.clear();
}

/**
 * @brief Initialize Shard.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] replica_ids Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), spdlog::warn(), size(), spdlog::error(), createRaftConfig(), spdlog::info().
 */
bool RaftShardManager::initializeShard(const std::string& shard_id,
                                      const std::vector<std::string>& replica_ids) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto& local_node_id = config_.raft_config.raft_config.node_id;
    if (shard_id.empty() || local_node_id.empty()) {
        spdlog::error("Raft shard and local node IDs must be configured");
        return false;
    }

    std::set<std::string> unique_members;
    for (const auto& replica_id : replica_ids) {
        if (replica_id.empty() || !unique_members.insert(replica_id).second) {
            spdlog::error("Raft group {} has an empty or duplicate member ID", shard_id);
            return false;
        }
    }
    if (unique_members.find(local_node_id) == unique_members.end()) {
        spdlog::error("Local node {} is not a member of Raft group {}",
                      local_node_id, shard_id);
        return false;
    }
    
    // Check if already initialized
    if (raft_instances_.find(shard_id) != raft_instances_.end()) {
        spdlog::warn("Raft instance already exists for shard: {}", shard_id);
        return false;
    }
    
    // Validate replica count
    if (replica_ids.size() < config_.replication_factor) {
        spdlog::error("Insufficient replicas for shard {}: got {}, expected {}",
                     shard_id,replica_ids.size(), config_.replication_factor);
        return false;
    }
    
    // Create Raft configuration
    auto raft_config = createRaftConfig(shard_id, replica_ids);
    
    // Create Raft instance
    try {
        auto raft = std::make_shared<RaftConsensus>(raft_config);
        if (config_.raft_peer_config_resolver && replica_ids.size() > 1) {
            auto peer_clients =
                std::make_shared<std::map<
                    std::string, std::shared_ptr<themis::sharding::ShardRPCClient>>>();
            for (const auto& peer_node_id : replica_ids) {
                if (peer_node_id == local_node_id) {
                    continue;
                }
                auto peer_config = config_.raft_peer_config_resolver(peer_node_id);
                if (!peer_config ||
                    peer_config->endpoint.empty() ||
                    peer_config->local_node_id != local_node_id ||
                    peer_config->expected_peer_node_id != peer_node_id ||
                    peer_config->expected_peer_identity_property !=
                        "x509_subject_alternative_name" ||
                    peer_config->expected_peer_identity_value.empty() ||
                    !peer_config->enable_mtls || !peer_config->tls_verify_server ||
                    peer_config->allow_insecure || peer_config->timeout_ms <= 0) {
                    throw std::runtime_error(
                        "Raft peer configuration must bind local/remote IDs to verified mTLS");
                }
                if (peer_config->shard_id.empty()) {
                    peer_config->shard_id = shard_id;
                }
                peer_clients->emplace(
                    peer_node_id,
                    std::make_shared<themis::sharding::ShardRPCClient>(*peer_config));
            }
            if (peer_clients->size() != replica_ids.size() - 1) {
                throw std::runtime_error("Raft peer transport configuration is incomplete");
            }

            const auto group_id = shard_id;
            const auto send_append =
                [peer_clients, group_id](const std::string& peer_node_id,
                                         const AppendEntriesRequest& request) {
                    const auto client = peer_clients->find(peer_node_id);
                    if (client == peer_clients->end() || !client->second) {
                        throw std::runtime_error("Raft peer has no configured RPC client");
                    }
                    return client->second->appendRaftEntries(
                        group_id, peer_node_id, request);
                };
            raft->setVoteRequestCallback(
                [peer_clients, group_id](const std::string& peer_node_id,
                                         const VoteRequest& request) {
                    const auto client = peer_clients->find(peer_node_id);
                    if (client == peer_clients->end() || !client->second) {
                        throw std::runtime_error("Raft voter has no configured RPC client");
                    }
                    return client->second->requestRaftVote(
                        group_id, peer_node_id, request);
                });
            raft->setReplicationCallback(
                [send_append](const std::string& peer_node_id,
                              const AppendEntriesRequest& request) {
                    try {
                        return send_append(peer_node_id, request);
                    } catch (const std::exception& error) {
                        spdlog::warn("Raft AppendEntries to {} failed: {}",
                                     peer_node_id, error.what());
                        return AppendEntriesResponse(request.term, false, 0);
                    }
                });
            if (config_.raft_snapshot_manager_resolver) {
                const auto snapshot_manager_resolver =
                    config_.raft_snapshot_manager_resolver;
                if (config_.raft_snapshot_timeout_ms == 0 ||
                    config_.raft_snapshot_timeout_ms > 300000) {
                    throw std::runtime_error(
                        "Raft snapshot timeout must be between 1 ms and five minutes");
                }
                const auto snapshot_timeout_ms =
                    config_.raft_snapshot_timeout_ms;
                raft->setSnapshotInstallCallback(
                    [peer_clients, group_id, snapshot_manager_resolver,
                     snapshot_timeout_ms](
                        const std::string& peer_node_id,
                        uint64_t leader_term,
                        uint64_t snapshot_index,
                        uint64_t snapshot_term) {
                        const auto client = peer_clients->find(peer_node_id);
                        if (client == peer_clients->end() || !client->second) {
                            throw std::runtime_error(
                                "Raft snapshot peer has no configured RPC client");
                        }
                        auto snapshots = snapshot_manager_resolver(group_id);
                        if (!snapshots) {
                            throw std::runtime_error(
                                "Raft snapshot source is unavailable for the group");
                        }
                        const auto result = client->second->installRaftSnapshot(
                            group_id, peer_node_id, *snapshots, leader_term,
                            snapshot_index, snapshot_timeout_ms);
                        if (!result.installed ||
                            result.snapshot_index != snapshot_index ||
                            result.snapshot_term != snapshot_term) {
                            return AppendEntriesResponse(
                                result.term, false, result.snapshot_index);
                        }
                        return AppendEntriesResponse(
                            result.term, true, result.snapshot_index);
                    });
            }
            // RaftConsensus sends bounded AppendEntries batches from each
            // replica's tracked next_index on its heartbeat loop, allowing
            // the same authenticated transport to repair a lagging prefix.
        }
        raft_instances_[shard_id] = raft;
        
        spdlog::info("Initialized Raft instance for shard: {} with {} replicas",
                    shard_id,replica_ids.size());
        
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

/**
 * @brief Remove Shard.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: lock(), find(), end(), stop(), erase(), spdlog::info().
 */
void RaftShardManager::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it != raft_instances_.end()) {
        if (it->second) {
            it->second->stop();
        }
        raft_instances_.erase(it);
        active_decision_stores_.erase(shard_id);
        spdlog::info("Removed Raft instance for shard: {}", shard_id);
    }
}

/**
 * @brief Start Shard.
 * @param[in] shard_id Identifier of the shard.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), spdlog::error(), start(), spdlog::info(), what().
 */
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

/**
 * @brief Stop Shard.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: lock(), find(), end(), stop(), spdlog::info().
 */
void RaftShardManager::stopShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it != raft_instances_.end() && it->second) {
        it->second->stop();
        spdlog::info("Stopped Raft instance for shard: {}", shard_id);
    }
}

bool RaftShardManager::isShardLeader(const std::string& shard_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return false;
    }
    
    return it->second->isLeader();
}

std::string RaftShardManager::getShardLeader(const std::string& shard_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return "";
    }
    
    return it->second->getLeaderId();
}

/**
 * @brief Propose Write.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] command Input parameter.
 * @return Return value.
 * @details Calls: lock(), find(), end(), set_value(), get_future(), propose().
 */
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

std::optional<ShardRaftInfo> RaftShardManager::getShardRaftInfo(const std::string& shard_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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

std::map<std::string, ShardRaftInfo> RaftShardManager::getAllShardRaftInfo() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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

bool RaftShardManager::hasQuorum(const std::string& shard_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it == raft_instances_.end() || !it->second) {
        return false;
    }
    
    return it->second->hasQuorum();
}

std::shared_ptr<RaftConsensus> RaftShardManager::getRaftInstance(const std::string& shard_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = raft_instances_.find(shard_id);
    if (it != raft_instances_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::shared_ptr<themis::sharding::RaftTwoPhaseCommitDecisionStore>
RaftShardManager::createTwoPhaseCommitDecisionStore(
    const std::string& shard_id,
    const RaftSnapshotManager& snapshot_manager) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto raft_it = raft_instances_.find(shard_id);
    if (raft_it == raft_instances_.end() || !raft_it->second) {
        throw std::invalid_argument(
            "cannot create a 2PC decision store for an uninitialized Raft shard");
    }
    auto store = std::make_shared<
        themis::sharding::RaftTwoPhaseCommitDecisionStore>(
            raft_it->second, shard_id, snapshot_manager);
    auto& active_stores = active_decision_stores_[shard_id];
    active_stores.erase(
        std::remove_if(active_stores.begin(), active_stores.end(),
                       [](const auto& weak_store) {
                           return weak_store.expired();
                       }),
        active_stores.end());
    active_stores.emplace_back(store);
    return store;
}

std::size_t RaftShardManager::restoreActiveTwoPhaseCommitDecisionStores(
    const std::string& shard_id,
    const RaftSnapshot& snapshot) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto raft_it = raft_instances_.find(shard_id);
    if (raft_it == raft_instances_.end() || !raft_it->second) {
        throw std::invalid_argument(
            "cannot restore 2PC decision stores for an uninitialized Raft shard");
    }

    const auto& state = raft_it->second->getRaftState();
    if (state.getSnapshotIndex() != snapshot.snapshot_index ||
        state.getSnapshotTerm() != snapshot.snapshot_term) {
        throw std::runtime_error(
            "2PC snapshot refresh does not match the installed Raft boundary");
    }

    auto stores_it = active_decision_stores_.find(shard_id);
    if (stores_it == active_decision_stores_.end()) {
        return 0;
    }
    auto& active_stores = stores_it->second;
    std::vector<std::shared_ptr<
        themis::sharding::RaftTwoPhaseCommitDecisionStore>> stores;
    stores.reserve(active_stores.size());
    active_stores.erase(
        std::remove_if(active_stores.begin(), active_stores.end(),
                       [&stores](const auto& weak_store) {
                           if (auto store = weak_store.lock()) {
                               stores.emplace_back(std::move(store));
                               return false;
                           }
                           return true;
                       }),
        active_stores.end());
    if (stores.empty()) {
        return 0;
    }

    const auto extension = snapshot.extensions.find(
        std::string(themis::sharding::RaftTwoPhaseCommitDecisionStore::
                        kSnapshotExtensionName));
    if (extension == snapshot.extensions.end()) {
        throw std::runtime_error(
            "installed snapshot is missing the active 2PC decision-state extension");
    }
    for (const auto& store : stores) {
        store->restoreSnapshotState(snapshot.snapshot_index,
                                    snapshot.snapshot_term,
                                    extension->second);
    }
    return stores.size();
}

/**
 * @brief Create Raft Config.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] replica_ids Input parameter.
 * @return Return value.
 * @details Implements createRaftConfig without additional internal calls.
 */
RaftConsensus::Config RaftShardManager::createRaftConfig(
    const std::string& shard_id,
    const std::vector<std::string>& replica_ids) {
    
    RaftConsensus::Config raft_config = config_.raft_config;
    
    // The map key identifies the Raft group; node_id identifies this process's member.
    raft_config.raft_config.cluster_members = replica_ids;
    
    // Enable partition detection for production use
    raft_config.enable_partition_detection = true;
    raft_config.enable_split_brain_prevention = true;
    
    return raft_config;
}

}  // namespace sharding
}  // namespace themisdb
