/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            replica_topology.cpp                               ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:40:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     99                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 62cab1172  2026-02-20  GEO_MIRROR: Configurable geo-quorums, locality-aware read... ║
    • 235d2ca7f  2026-02-10  Refactor tests and update dependencies   ║
    • b8699bedd  2026-01-04  Implement Replica Topology and Health Monitoring for Shar... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/replica_topology.h"

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

namespace themis::sharding {

bool ReplicaTopology::loadFromJson(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config.is_array()) {
        return false;
    }
    
    replica_sets_.clear();
    
    for (const auto& item : config) {
        ShardReplicaSet replica_set;
        
        if (!item.contains("shard_id") || !item.contains("primary_id")) {
            continue;
        }
        
        replica_set.shard_id = item["shard_id"].get<std::string>();
        replica_set.primary_id = item["primary_id"].get<std::string>();
        
        // Parse redundancy mode
        if (item.contains("redundancy")) {
            std::string mode_str = item["redundancy"].get<std::string>();
            std::string mode_upper = mode_str;
            std::transform(mode_upper.begin(), mode_upper.end(), mode_upper.begin(),
                [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            if (mode_upper == "NONE") {
                replica_set.redundancy = RedundancyMode::NONE;
            } else if (mode_upper == "RAID1" || mode_upper == "MIRROR") {
                replica_set.redundancy = RedundancyMode::MIRROR;
            } else if (mode_upper == "RAID10" || mode_upper == "STRIPE_MIRROR") {
                replica_set.redundancy = RedundancyMode::STRIPE_MIRROR;
            } else if (mode_upper == "RAID5" || mode_upper == "PARITY") {
                replica_set.redundancy = RedundancyMode::PARITY;
            } else if (mode_upper == "RAID6") {
                replica_set.redundancy = RedundancyMode::RAID6;
            } else if (mode_upper == "STRIPE") {
                replica_set.redundancy = RedundancyMode::STRIPE;
            } else if (mode_upper == "GEO_MIRROR") {
                replica_set.redundancy = RedundancyMode::GEO_MIRROR;
            }
        }
        
        // Parse replicas
        if (item.contains("replicas") && item["replicas"].is_array()) {
            for (const auto& replica : item["replicas"]) {
                if (replica.is_string()) {
                    replica_set.replicas.push_back(replica.get<std::string>());
                }
            }
        }
        
        // Parse stripe key (for STRIPE_MIRROR)
        if (item.contains("stripe_key")) {
            replica_set.stripe_key = item["stripe_key"].get<uint64_t>();
        }

        // Parse geo placement metadata (for GEO_MIRROR and Raft placement)
        if (item.contains("region")) {
            replica_set.region = item["region"].get<std::string>();
        }
        if (item.contains("zone")) {
            replica_set.zone = item["zone"].get<std::string>();
        }
        
        replica_sets_[replica_set.shard_id] = replica_set;
    }
    
    return !replica_sets_.empty();
}

} // namespace themis::sharding
