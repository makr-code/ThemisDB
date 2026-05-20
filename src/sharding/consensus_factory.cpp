/*
 * ThemisDB | File: consensus_factory.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 95
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=8 | delta=5 | status=divergent
 * External Severity (v3): C=0, H=7, M=1
 * PR: #866 Distributed Sharding: Pluggable Consensus and Cross-Shard Transacti... (2026-03-11T21:56:07Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
