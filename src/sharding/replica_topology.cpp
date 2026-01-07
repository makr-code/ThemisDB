#include "sharding/replica_topology.h"
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
            if (mode_str == "RAID1") {
                replica_set.redundancy = RedundancyMode::RAID1;
            } else if (mode_str == "RAID10") {
                replica_set.redundancy = RedundancyMode::RAID10;
            } else if (mode_str == "RAID5") {
                replica_set.redundancy = RedundancyMode::RAID5;
            } else if (mode_str == "RAID6") {
                replica_set.redundancy = RedundancyMode::RAID6;
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
        
        // Parse stripe key (for RAID 10)
        if (item.contains("stripe_key")) {
            replica_set.stripe_key = item["stripe_key"].get<uint64_t>();
        }
        
        replica_sets_[replica_set.shard_id] = replica_set;
    }
    
    return !replica_sets_.empty();
}

} // namespace themis::sharding
