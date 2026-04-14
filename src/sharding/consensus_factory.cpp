/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consensus_factory.cpp                              ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:06:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     109                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/consensus_factory.h"
#include "sharding/raft_consensus_adapter.h"
#include "sharding/gossip_consensus_adapter.h"
#include "sharding/paxos_consensus.h"
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
        case ConsensusType::MULTI_PAXOS:
            return std::make_unique<PaxosConsensus>(config);
            
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
    }
    
    return std::nullopt;
}

std::vector<ConsensusType> ConsensusFactory::getSupportedTypes() {
    return {
        ConsensusType::RAFT,
        ConsensusType::GOSSIP,
        ConsensusType::PAXOS,
        ConsensusType::MULTI_PAXOS
    };
}

} // namespace sharding
} // namespace themisdb
