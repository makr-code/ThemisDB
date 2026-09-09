/**
 * @file consensus_factory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/consensus_factory.h"
#include "sharding/raft_consensus_adapter.h"
#include "sharding/gossip_consensus_adapter.h"
#include "sharding/paxos_consensus.h"
#include "sharding/raid_paxos_consensus.h"
#include "sharding/dual_consensus_orchestrator.h"
#include "sharding/raft_consensus.h"
#include <algorithm>
#include <cctype>

namespace themisdb {
namespace sharding {

std::unique_ptr<ConsensusModule> ConsensusFactory::create(const ConsensusConfig& config) {
    switch (config.type) {
        case ConsensusType::RAFT:
            return std::make_unique<RaftConsensusAdapter>(config);
            
        case ConsensusType::GOSSIP:
            return std::make_unique<GossipConsensusAdapter>(config);
            
        case ConsensusType::PAXOS:
        [[fallthrough]];
        case ConsensusType::MULTI_PAXOS:
            return std::make_unique<PaxosConsensus>(config);
        
        case ConsensusType::RAID_PAXOS: {
            // Create RAID-Paxos with default configuration
            // For custom RAID configuration, use createRAIDPaxos() directly
            RAIDPaxosConfig raid_config;
            raid_config.type = ConsensusType::RAID_PAXOS;
            raid_config.node_id = config.node_id;
            raid_config.cluster_nodes = config.cluster_nodes;
            raid_config.heartbeat_interval = config.heartbeat_interval;
            raid_config.election_timeout_min = config.election_timeout_min;
            raid_config.election_timeout_max = config.election_timeout_max;
            raid_config.data_dir = config.data_dir;
            raid_config.enable_persistence = config.enable_persistence;
            raid_config.paxos_quorum_size = config.paxos_quorum_size;
            raid_config.paxos_enable_fast_path = config.paxos_enable_fast_path;
            
            auto raid_paxos = std::make_unique<RAIDPaxosConsensus>(raid_config);
            if (!raid_paxos->initialize(config.node_id, config.cluster_nodes)) {
                return nullptr;
            }
            return raid_paxos;
        }
            
        case ConsensusType::DUAL_CONSENSUS:
            // DualConsensusOrchestrator is not a ConsensusModule - it orchestrates two.
            // Use createDualConsensus() directly to get a DualConsensusOrchestrator instance.
            return nullptr;
            
        default:
            return nullptr;
    }
}

std::unique_ptr<ConsensusModule> ConsensusFactory::create(
    ConsensusType type,
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    ConsensusConfig config;
    config.type = type;
    config.node_id = node_id;
    config.cluster_nodes = cluster_nodes;
    
    return create(config);
}

std::unique_ptr<RAIDPaxosConsensus> ConsensusFactory::createRAIDPaxos(
    const RAIDPaxosConfig& raid_config,
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    auto raid_paxos = std::make_unique<RAIDPaxosConsensus>(raid_config);
    if (!raid_paxos->initialize(node_id, cluster_nodes)) {
        return nullptr;
    }
    return raid_paxos;
}

std::unique_ptr<DualConsensusOrchestrator> ConsensusFactory::createDualConsensus(
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes,
    bool use_raid_paxos,
    const RAIDPaxosConfig& raid_config
) {
    std::unique_ptr<ConsensusModule> storage_consensus;
    std::unique_ptr<ConsensusModule> cache_consensus;
    
    // Create storage layer
    if (use_raid_paxos) {
        auto raid_paxos = std::make_unique<RAIDPaxosConsensus>(raid_config);
        if (!raid_paxos->initialize(node_id, cluster_nodes)) {
            return nullptr;
        }
        storage_consensus = std::move(raid_paxos);
    } else {
        // Use standard Paxos for storage
        ConsensusConfig paxos_config;
        paxos_config.type = ConsensusType::PAXOS;
        paxos_config.node_id = node_id;
        paxos_config.cluster_nodes = cluster_nodes;
        storage_consensus = std::make_unique<PaxosConsensus>(paxos_config);
        if (!storage_consensus->initialize(node_id, cluster_nodes)) {
            return nullptr;
        }
    }
    
    // Create cache layer (Raft for performance)
    ConsensusConfig raft_config;
    raft_config.type = ConsensusType::RAFT;
    raft_config.node_id = node_id;
    raft_config.cluster_nodes = cluster_nodes;
    cache_consensus = std::make_unique<RaftConsensusAdapter>(raft_config);
    if (!cache_consensus->initialize(node_id, cluster_nodes)) {
        return nullptr;
    }
    
    // Create Dual-Consensus Orchestrator
    auto orchestrator = std::make_unique<DualConsensusOrchestrator>(
        std::move(storage_consensus),
        std::move(cache_consensus)
    );
    
    if (!orchestrator->initialize(node_id, cluster_nodes)) {
        return nullptr;
    }
    
    return orchestrator;
}

std::string ConsensusFactory::getTypeName(ConsensusType type) {
    switch (type) {
        case ConsensusType::RAFT:
            return "Raft";
        case ConsensusType::GOSSIP:
            return "Gossip";
        case ConsensusType::PAXOS:
            return "Paxos";
        case ConsensusType::MULTI_PAXOS:
            return "Multi-Paxos";
        case ConsensusType::RAID_PAXOS:
            return "RAID-Paxos";
        case ConsensusType::DUAL_CONSENSUS:
            return "Dual-Consensus";
        default:
            return "Unknown";
    }
}

std::optional<ConsensusType> ConsensusFactory::parseType(const std::string& type_str) {
    std::string lower_str = type_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower_str == "raft") {
        return ConsensusType::RAFT;
    } else if (lower_str == "gossip") {
        return ConsensusType::GOSSIP;
    } else if (lower_str == "paxos") {
        return ConsensusType::PAXOS;
    } else if (lower_str == "multi-paxos" || lower_str == "multipaxos") {
        return ConsensusType::MULTI_PAXOS;
    } else if (lower_str == "raid-paxos" || lower_str == "raid_paxos" || lower_str == "raidpaxos") {
        return ConsensusType::RAID_PAXOS;
    } else if (lower_str == "dual-consensus" || lower_str == "dual_consensus" || lower_str == "dualconsensus") {
        return ConsensusType::DUAL_CONSENSUS;
    }
    
    return std::nullopt;
}

std::vector<ConsensusType> ConsensusFactory::getSupportedTypes() {
    return {
        ConsensusType::RAFT,
        ConsensusType::GOSSIP,
        ConsensusType::PAXOS,
        ConsensusType::MULTI_PAXOS,
        ConsensusType::RAID_PAXOS,
        ConsensusType::DUAL_CONSENSUS
    };
}

} // namespace sharding
} // namespace themisdb
