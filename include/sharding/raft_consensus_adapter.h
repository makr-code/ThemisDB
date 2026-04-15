/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_consensus_adapter.h                           ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/consensus_module.h"
#include "sharding/raft_consensus.h"
#include "sharding/raft_configuration.h"
#include <memory>
#include <mutex>

namespace themisdb {
namespace sharding {

/**
 * @brief Adapter for existing RaftConsensus implementation
 * 
 * Adapts the existing Raft implementation to the ConsensusModule interface,
 * allowing it to be used interchangeably with other consensus algorithms.
 */
class RaftConsensusAdapter : public ConsensusModule {
public:
    explicit RaftConsensusAdapter(const ConsensusConfig& config);
    ~RaftConsensusAdapter() override;
    
    // ConsensusModule interface
    ConsensusType getType() const override { return ConsensusType::RAFT; }
    
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
     * @brief Convert RaftState to ConsensusState
     */
    static ConsensusState convertState(const RaftState& state);
    
    /**
     * @brief Convert LogEntry to ConsensusLogEntry
     */
    static ConsensusLogEntry convertLogEntry(const LogEntry& entry);
    
    ConsensusConfig config_;
    std::unique_ptr<RaftConsensus> raft_;
    std::string node_id_;
    
    // Cluster nodes with synchronization
    mutable std::mutex cluster_mutex_;
    std::vector<std::string> cluster_nodes_;
    
    // Callbacks
    mutable std::mutex callbacks_mutex_;
    std::function<void(const ConsensusLogEntry&)> on_commit_callback_;
    std::function<void(ConsensusState, ConsensusState)> on_state_change_callback_;
    std::function<void(const std::string&, const std::string&)> on_leader_change_callback_;
    
    // State tracking
    mutable std::mutex state_mutex_;
    mutable ConsensusState current_state_;
    std::string current_leader_;

    // Snapshot storage (in-adapter, index-tracked)
    mutable std::mutex snapshot_mutex_;
    nlohmann::json snapshot_data_;
    uint64_t snapshot_index_{0};
    uint64_t snapshot_term_{0};

    // Joint-consensus membership tracker (Raft v2)
    std::unique_ptr<themis::sharding::RaftConfiguration> membership_;
};

} // namespace sharding
} // namespace themisdb
