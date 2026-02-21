/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consensus_factory.h                                ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_CONSENSUS_FACTORY_H
#define THEMISDB_SHARDING_CONSENSUS_FACTORY_H

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

#endif // THEMISDB_SHARDING_CONSENSUS_FACTORY_H
