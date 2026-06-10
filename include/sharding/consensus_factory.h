/**
 * @file consensus_factory.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: consensus_factory.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
