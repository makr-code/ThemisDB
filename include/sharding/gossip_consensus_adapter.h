/**
 * @file gossip_consensus_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/consensus_module.h"
#include "sharding/gossip_protocol.h"
#include "sharding/distributed_coordinator.h"
#include <condition_variable>
#include <memory>
#include <mutex>

namespace themisdb {
namespace sharding {

/**
 * @brief Adapter for GossipProtocol and DistributedCoordinator
 * 
 * Adapts the existing Gossip protocol and distributed coordinator
 * to the ConsensusModule interface. Gossip is eventually consistent
 * and leaderless, but can be used for certain distributed scenarios.
 * 
 * Note: Gossip is not strongly consistent like Raft or Paxos.
 * It provides eventual consistency and is best suited for:
 * - Cluster membership detection
 * - Failure detection
 * - Configuration propagation
 * - Metrics aggregation
 */
class GossipConsensusAdapter : public ConsensusModule {
public:
    explicit GossipConsensusAdapter(const ConsensusConfig& config);
    ~GossipConsensusAdapter() override;
    
    // ConsensusModule interface
    ConsensusType getType() const override { return ConsensusType::GOSSIP; }
    
    bool initialize(
        const std::string& node_id,
        const std::vector<std::string>& cluster_nodes
    ) override;
    
    bool start() override;
    void stop() override;
    
    bool isLeader() const override;
    std::string getLeaderId() const override;
    ConsensusState getState() const override;
    
    std::optional<uint64_t> propose(
        const std::string& operation,
        const nlohmann::json& data
    ) override;
    
    bool waitForCommit(
        uint64_t log_index,
        std::chrono::milliseconds timeout
    ) override;
    
    std::vector<ConsensusLogEntry> readLog(
        uint64_t start_index,
        std::optional<uint64_t> end_index = std::nullopt
    ) override;
    
    uint64_t getCommitIndex() const override;
    uint64_t getLastLogIndex() const override;
    
    bool addNode(
        const std::string& node_id,
        const std::string& endpoint
    ) override;
    
    bool removeNode(const std::string& node_id) override;
    
    bool transferLeadership(const std::string& target_node_id) override;
    
    bool takeSnapshot(const nlohmann::json& snapshot_data) override;
    bool restoreSnapshot(const nlohmann::json& snapshot_data) override;
    
    ConsensusStats getStats() const override;
    nlohmann::json getStatus() const override;
    
    void onCommit(
        std::function<void(const ConsensusLogEntry&)> callback
    ) override;
    
    void onStateChange(
        std::function<void(ConsensusState, ConsensusState)> callback
    ) override;
    
    void onLeaderChange(
        std::function<void(const std::string&, const std::string&)> callback
    ) override;
    
private:
    /**
     * @brief Background thread for gossip propagation
     */
    void gossipThread();
    
    /**
     * @brief Check if a log entry has reached quorum
     */
    bool hasReachedQuorum(uint64_t log_index) const;
    
    ConsensusConfig config_;
    std::unique_ptr<themis::sharding::GossipProtocol> gossip_;
    std::unique_ptr<themis::sharding::DistributedCoordinator> coordinator_;
    
    std::string node_id_;
    std::vector<std::string> cluster_nodes_;
    
    // State
    mutable std::mutex state_mutex_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> next_log_index_;
    std::atomic<uint64_t> commit_index_;
    
    // Log storage (eventually consistent)
    mutable std::mutex log_mutex_;
    std::map<uint64_t, ConsensusLogEntry> log_entries_;
    std::map<uint64_t, std::set<std::string>> log_acknowledgments_;  // Nodes that have the entry
    
    // Callbacks
    mutable std::mutex callbacks_mutex_;
    std::function<void(const ConsensusLogEntry&)> on_commit_callback_;
    std::function<void(ConsensusState, ConsensusState)> on_state_change_callback_;
    std::function<void(const std::string&, const std::string&)> on_leader_change_callback_;
    
    // Background thread
    std::thread gossip_thread_;
    std::condition_variable gossip_cv_;
    
    // Statistics
    std::atomic<uint64_t> total_operations_;
    std::atomic<uint64_t> failed_operations_;

    // Snapshot storage (protected by state_mutex_)
    nlohmann::json snapshot_data_;
    uint64_t snapshot_index_{0};
    uint64_t snapshot_term_{0};
};

} // namespace sharding
} // namespace themisdb
