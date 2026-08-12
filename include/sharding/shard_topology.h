/**
 * @file shard_topology.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <optional>
#include <algorithm>
#include "sharding/shard_capabilities.h"

namespace themis::sharding {

/**
 * Shard Information
 * Contains all metadata about a shard including network endpoints,
 * location, and health status.
 */
struct ShardInfo {
    std::string shard_id;                    // shard_001, shard_002, ...
    std::string primary_endpoint;            // themis-shard001.dc1.example.com:8080
    std::vector<std::string> replica_endpoints; // replica nodes
    std::string datacenter;                  // dc1, dc2, us-east-1, eu-west-1
    std::string region;                      // us-east, eu-west, ap-south (geo region)
    std::string zone;                        // us-east-1a, eu-west-1b (availability zone)
    std::string rack;                        // rack01, rack02 (locality awareness)
    uint64_t token_start;                    // Consistent Hash Range Start
    uint64_t token_end;                      // Consistent Hash Range End
    bool is_healthy;                         // Health check status
    
    // PKI/Security fields
    std::string certificate_serial;          // X.509 certificate serial number
    std::vector<std::string> capabilities;   // read, write, replicate, admin
    
    // Domain capability for adaptive routing
    DomainCapability domain_capability;      // Domain specialization info
    
    // Raft consensus state (optional, populated when Raft is enabled)
    std::string raft_role;                   // "LEADER", "FOLLOWER", "CANDIDATE", or empty
    uint64_t raft_term;                      // Current Raft term (0 if not using Raft)
    uint64_t raft_commit_index;              // Raft commit index
    std::string raft_leader_id;              // Current leader shard ID (empty if unknown)
    bool raft_has_quorum;                    // Does this shard have Raft quorum?
    
    /** @brief Check whether shard advertises specific capability token. */
    bool hasCapability(const std::string& cap) const {
        return std::find(capabilities.begin(), capabilities.end(), cap) != capabilities.end();
    }
    
    /** @brief Check whether shard currently reports Raft leader role. */
    bool isRaftLeader() const {
        return raft_role == "LEADER";
    }
};

/**
 * Shard Topology Manager
 * 
 * Manages the cluster topology including shard locations, health status,
 * and metadata. Integrates with metadata store (etcd) for distributed
 * configuration.
 * 
 * Thread-safe for concurrent access.
 */
class ShardTopology {
public:
    /**
     * Configuration for ShardTopology
     */
    struct Config {
        std::string metadata_endpoint;  // etcd endpoint (e.g., "http://localhost:2379")
        std::string cluster_name;       // Cluster identifier
        uint32_t refresh_interval_sec;  // Auto-refresh interval (0 = manual only)
        bool enable_health_checks;      // Enable periodic health checks
    };
    
    /**
     * Construct ShardTopology with configuration
     * @param config Configuration parameters
     */
    explicit ShardTopology(const Config& config);

    /** @brief Convenience default constructor for tests/benchmarks. */
    ShardTopology();
    
    /**
     * Add or update shard information
     * @param shard Shard information
     */
    void addShard(const ShardInfo& shard);
    
    /**
     * Remove shard from topology
     * @param shard_id Shard identifier
     */
    void removeShard(const std::string& shard_id);
    
    /**
     * Get shard information by ID
     * @param shard_id Shard identifier
     * @return ShardInfo if found, nullopt otherwise
     */
    std::optional<ShardInfo> getShard(const std::string& shard_id) const;
    
    /**
     * Get all shards in the cluster
     * @return Vector of all shard information
     */
    std::vector<ShardInfo> getAllShards() const;
    
    /**
     * Get healthy shards only
     * @return Vector of healthy shards
     */
    std::vector<ShardInfo> getHealthyShards() const;
    
    /**
     * Update shard health status
     * @param shard_id Shard identifier
     * @param is_healthy Health status
     */
    void updateHealth(const std::string& shard_id, bool is_healthy);
    
    /**
     * Refresh topology from metadata store
     * Loads latest shard configuration from etcd
     */
    void refresh();
    
    /**
     * Save topology to metadata store
     * Persists current topology to etcd
     */
    void save();
    
    /** @brief Return total shard count. */
    size_t getShardCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shards_.size();
    }
    
    /** @brief Return whether shard id is present in topology map. */
    bool hasShard(const std::string& shard_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shards_.find(shard_id) != shards_.end();
    }
    
    /** @brief Remove all shard entries (test helper). */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        shards_.clear();
    }
    
    /**
     * Update Raft status for a shard
     * @param shard_id Shard identifier
     * @param role Raft role (LEADER, FOLLOWER, CANDIDATE)
     * @param term Current Raft term
     * @param commit_index Raft commit index
     * @param leader_id Current leader shard ID
     * @param has_quorum Does shard have quorum?
     */
    void updateRaftStatus(const std::string& shard_id,
                         const std::string& role,
                         uint64_t term,
                         uint64_t commit_index,
                         const std::string& leader_id,
                         bool has_quorum);
    
    /**
     * Get shards that are Raft leaders
     * @return Vector of shard IDs that are leaders
     */
    std::vector<std::string> getRaftLeaders() const;

    /**
     * Get shards in a specific region
     * @param region Region name (e.g. "us-east", "eu-west")
     * @return Vector of ShardInfo for all shards in that region
     */
    std::vector<ShardInfo> getShardsInRegion(const std::string& region) const;

    /**
     * Get healthy shards in a specific region
     * @param region Region name
     * @return Vector of healthy ShardInfo for that region
     */
    std::vector<ShardInfo> getHealthyShardsInRegion(const std::string& region) const;

    /**
     * Get all distinct regions present in the topology
     * @return Sorted list of unique region names
     */
    std::vector<std::string> getRegions() const;

    /**
     * Check if a region has enough healthy shards to meet a quorum requirement
     * @param region Region name
     * @param required Minimum number of healthy shards required
     * @return true if region meets quorum
     */
    bool regionHasQuorum(const std::string& region, uint32_t required) const;

private:
    Config config_;
    std::map<std::string, ShardInfo> shards_;
    mutable std::mutex mutex_;
    
    /** @brief Load topology snapshot from configured metadata store backend. */
    void loadFromMetadataStore();
    
    /** @brief Persist current topology snapshot to metadata store backend. */
    void saveToMetadataStore();
};

} // namespace themis::sharding
