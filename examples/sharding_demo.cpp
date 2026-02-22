/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sharding_demo.cpp                                  ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-22 11:29:17                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c97d719  2026-02-22  Add parallel multi-source BFS/DFS implementation (graph/p... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/urn.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include <iostream>
#include <memory>

using namespace themis::sharding;

int main() {
    std::cout << "=== ThemisDB Horizontal Sharding Demo ===" << std::endl;
    
    // Create URN
    auto urn = URN::parse("urn:themis:relational:customers:users:550e8400-e29b-41d4-a716-446655440000");
    std::cout << "URN: " << urn->toString() << std::endl;
    
    // Setup hash ring
    auto hash_ring = std::make_shared<ConsistentHashRing>();
    hash_ring->addShard("shard_001", 150);
    hash_ring->addShard("shard_002", 150);
    
    std::cout << "Shards: " << hash_ring->getShardCount() << std::endl;
    std::cout << "Target shard: " << hash_ring->getShardForURN(*urn) << std::endl;
    
    return 0;
}
