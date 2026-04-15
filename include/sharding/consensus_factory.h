/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consensus_factory.h                                ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     86                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/consensus_module.h"
#include <memory>
#include <string>

namespace themisdb {
namespace sharding {

/**
 * @brief Factory for creating consensus module instances
 * 
 * Provides a centralized way to create different types of consensus
 * modules based on configuration. Supports Raft, Gossip, and Paxos.
 */
class ConsensusFactory {
public:
    /**
     * @brief Create a consensus module based on configuration
     * @param config Consensus configuration
     * @return Unique pointer to the consensus module, or nullptr if creation failed
     */
    static std::unique_ptr<ConsensusModule> create(const ConsensusConfig& config);
    
    /**
     * @brief Create a consensus module by type
     * @param type Type of consensus module to create
     * @param node_id This node's unique identifier
     * @param cluster_nodes List of all nodes in the cluster
     * @return Unique pointer to the consensus module, or nullptr if creation failed
     */
    static std::unique_ptr<ConsensusModule> create(
        ConsensusType type,
        const std::string& node_id,
        const std::vector<std::string>& cluster_nodes
    );
    
    /**
     * @brief Get the name of a consensus type
     * @param type Consensus type
     * @return Human-readable name
     */
    static std::string getTypeName(ConsensusType type);
    
    /**
     * @brief Parse consensus type from string
     * @param type_str String representation (e.g., "raft", "gossip", "paxos")
     * @return Consensus type, or nullopt if invalid
     */
    static std::optional<ConsensusType> parseType(const std::string& type_str);
    
    /**
     * @brief Get list of supported consensus types
     */
    static std::vector<ConsensusType> getSupportedTypes();
};

} // namespace sharding
} // namespace themisdb
