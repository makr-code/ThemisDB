/**
 * @file gossip_config_manager.h
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
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>
#include <chrono>
#include <thread>

namespace themis {
namespace sharding {
namespace proto {
class VectorClock;
class ConfigUpdate;
class ResourceSnapshot;
class GossipMessage;
} // namespace proto
} // namespace sharding
} // namespace themis

namespace themis {
namespace sharding {

// Forward declarations
class ShardTopology;
class MTLSClient;
class PrometheusMetrics;

/**
 * @brief Vector Clock for causality tracking in distributed systems
 * 
 * Used for anti-entropy and conflict resolution in the gossip protocol.
 * Based on Lamport's logical clocks and vector clocks.
 * 
 * Sources:
 * - Paper: Lamport, L. (1978). "Time, clocks, and the ordering of events in a distributed system"
 * - Paper: Fidge, C. J. (1988). "Timestamps in message-passing systems that preserve partial ordering"
 * - Implementation: Inspired by Apache Cassandra and Amazon Dynamo
 */
class VectorClock {
public:
    VectorClock() = default;
    
    // Increment local clock for a shard
    void increment(const std::string& shard_id);
    
    // Merge with another vector clock (for received messages)
    void merge(const VectorClock& other);
    
    // Compare two vector clocks
    enum class Ordering {
        BEFORE,      // This clock is before other
        AFTER,       // This clock is after other
        CONCURRENT,  // Clocks are concurrent (conflict)
        EQUAL        // Clocks are equal
    };
    
    Ordering compare(const VectorClock& other) const;
    
    // Get clock value for a shard
    uint64_t get(const std::string& shard_id) const;
    
    // Set clock value for a shard
    void set(const std::string& shard_id, uint64_t value);
    
    // Serialize to protobuf
    proto::VectorClock toProto() const;
    
    // Deserialize from protobuf
    static VectorClock fromProto(const proto::VectorClock& proto);
    
    // Get all clocks
    const std::map<std::string, uint64_t>& getClocks() const { return clocks_; }
    
private:
    std::map<std::string, uint64_t> clocks_;
};

/**
 * @brief Configuration Update with vector clock
 * 
 * Represents a configuration change that needs to be propagated
 * through the gossip network.
 */
struct ConfigUpdate {
    std::string update_id;
    std::string config_key;
    std::string config_value;
    uint64_t timestamp_ns;
    VectorClock vector_clock;
    std::string originator_shard_id;
    uint32_t ttl;
    
    // Convert to protobuf
    proto::ConfigUpdate toProto() const;
    
    // Convert from protobuf
    static ConfigUpdate fromProto(const proto::ConfigUpdate& proto);
};

/**
 * @brief Resource Snapshot (YARN-inspired)
 * 
 * Represents the current resource state of a shard, similar to
 * YARN NodeManager's resource tracking.
 */
struct ResourceSnapshot {
    std::string shard_id;
    uint64_t timestamp_ns;
    
    // Resource metrics
    uint64_t available_memory_bytes;
    uint64_t total_memory_bytes;
    uint32_t available_cpu_cores;
    uint32_t total_cpu_cores;
    
    // Storage metrics
    uint64_t available_disk_bytes;
    uint64_t total_disk_bytes;
    uint64_t rocksdb_sst_files_count;
    uint64_t rocksdb_total_size_bytes;
    
    // Load metrics
    double cpu_usage_percent;
    double memory_usage_percent;
    double disk_usage_percent;
    uint64_t requests_per_second;
    double avg_latency_ms;
    
    // Health status
    bool is_healthy;
    std::string status;
    std::vector<std::string> warnings;
    
    // Convert to protobuf
    proto::ResourceSnapshot toProto() const;
    
    // Convert from protobuf
    static ResourceSnapshot fromProto(const proto::ResourceSnapshot& proto);
};

/**
 * @brief Configuration for Gossip Config Manager
 */
struct GossipConfigManagerConfig {
    bool enabled = true;
    uint32_t gossip_interval_ms = 1000;      // Gossip round interval (1 second)
    uint32_t fanout = 3;                      // Number of peers per round
    uint32_t max_updates = 1000;              // Maximum config updates to track
    uint32_t update_ttl = 10;                 // Default TTL for updates (rounds)
    uint32_t anti_entropy_interval_ms = 5000; // Anti-entropy sync interval
    bool require_mtls = true;                 // Require mTLS for communication

    // Cross-shard RPC connection pool configuration.
    // Propagated via gossip so all shards converge on the same pool limit.
    uint32_t rpc_max_pool_connections = 50;   // Per-endpoint connection pool size

    std::string local_shard_id;
    std::string local_endpoint;
};

/**
 * @brief Gossip-Enhanced Configuration Manager
 * 
 * Implements a decentralized configuration management system using
 * gossip protocols with vector clocks for anti-entropy. Inspired by
 * YARN's NodeManager heartbeat pattern and Dynamo's anti-entropy design.
 * 
 * Features:
 * - Decentralized config propagation (reduces etcd dependency)
 * - Vector clock-based conflict resolution
 * - Anti-entropy for eventual consistency
 * - YARN-inspired resource tracking
 * - Periodic gossip rounds with peer selection
 * - Prometheus metrics integration
 * 
 * Sources:
 * - YARN Architecture: NodeManager heartbeat and resource tracking
 * - Dynamo: Vector clocks and anti-entropy
 * - Cassandra: Gossip protocol implementation
 * 
 * Example:
 *   GossipConfigManagerConfig config;
 *   config.local_shard_id = "shard-1";
 *   config.local_endpoint = "localhost:8080";
 *   
 *   auto manager = std::make_unique<GossipConfigManager>(config, topology);
 *   manager->start();
 *   
 *   // Publish a config update
 *   manager->publishConfigUpdate("shard.replication_factor", "3");
 *   
 *   // Register listener for updates
 *   manager->onConfigUpdate([](const ConfigUpdate& update) {
 *       std::cout << "Config updated: " << update.config_key << std::endl;
 *   });
 */
class GossipConfigManager {
public:
    using ConfigUpdateCallback = std::function<void(const ConfigUpdate&)>;
    using ResourceSnapshotCallback = std::function<void(const ResourceSnapshot&)>;

    /**
     * Transport function type for gossip message delivery.
     * @param peer_endpoint  Destination endpoint string (e.g. "https://shard-2:8080")
     * @param message        Gossip message to deliver
     * @return true if the message was accepted by the peer, false on error
     */
    using GossipSendFn = std::function<bool(const std::string& peer_endpoint,
                                            const proto::GossipMessage& message)>;
    
    /**
     * Construct GossipConfigManager
     * @param config Configuration parameters
     * @param topology ShardTopology for peer discovery
     * @param metrics Optional Prometheus metrics exporter
     */
    GossipConfigManager(const GossipConfigManagerConfig& config,
                       std::shared_ptr<ShardTopology> topology,
                       std::shared_ptr<PrometheusMetrics> metrics = nullptr);
    
    ~GossipConfigManager();
    
    /**
     * Start the gossip config manager
     */
    void start();
    
    /**
     * Stop the gossip config manager
     */
    void stop();
    
    /**
     * Check if manager is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * Publish a configuration update
     * @param config_key Configuration key
     * @param config_value Configuration value (JSON-encoded)
     * @return Update ID
     */
    std::string publishConfigUpdate(const std::string& config_key, 
                                    const std::string& config_value);
    
    /**
     * Publish a resource snapshot
     * @param snapshot Resource snapshot to publish
     */
    void publishResourceSnapshot(const ResourceSnapshot& snapshot);
    
    /**
     * Handle incoming gossip message
     * @param message Received gossip message
     * @return Response message
     */
    proto::GossipMessage handleGossipMessage(const proto::GossipMessage& message);
    
    /**
     * Register callback for config updates
     * @param callback Function called when config update is received
     */
    void onConfigUpdate(ConfigUpdateCallback callback);
    
    /**
     * Register callback for resource snapshots
     * @param callback Function called when resource snapshot is received
     */
    void onResourceSnapshot(ResourceSnapshotCallback callback);

    /**
     * Inject a custom gossip transport function.
     *
     * When set, every outbound gossip message is delivered via @p fn instead
     * of the default MTLSClient HTTP path.  Intended for testing or for
     * integrating alternative transports (e.g. gRPC gossip service).
     *
     * @param fn  Callable with signature
     *            `bool(const std::string& endpoint, const proto::GossipMessage&)`.
     *            Returning true counts as a successful delivery; false increments
     *            the error counter.  Must be thread-safe.
     */
    void setGossipSendFunction(GossipSendFn fn);
    
    /**
     * Get current configuration value
     * @param config_key Configuration key
     * @return Configuration value (empty if not found)
     */
    std::string getConfig(const std::string& config_key) const;
    
    /**
     * Get all configurations
     * @return Map of config key to value
     */
    std::map<std::string, std::string> getAllConfigs() const;
    
    /**
     * Get resource snapshot for a shard
     * @param shard_id Shard identifier
     * @return Resource snapshot (empty if not found)
     */
    ResourceSnapshot getResourceSnapshot(const std::string& shard_id) const;
    
    /**
     * Get all resource snapshots
     * @return Map of shard_id to ResourceSnapshot
     */
    std::map<std::string, ResourceSnapshot> getAllResourceSnapshots() const;
    
    /**
     * Get local vector clock
     */
    VectorClock getVectorClock() const;
    
    /**
     * Get statistics for monitoring
     */
    struct Statistics {
        uint64_t gossip_rounds = 0;
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t config_updates_sent;
        uint64_t config_updates_received;
        uint64_t resource_snapshots_sent;
        uint64_t resource_snapshots_received;
        uint64_t conflicts_resolved;
        uint64_t anti_entropy_syncs;
        double avg_propagation_latency_ms;
    };
    
    Statistics getStatistics() const;

private:
    GossipConfigManagerConfig config_;
    std::shared_ptr<ShardTopology> topology_;
    std::unique_ptr<MTLSClient> client_;
    std::shared_ptr<PrometheusMetrics> metrics_;  // Prometheus metrics exporter
    
    // Vector clock for this shard
    VectorClock local_clock_;
    mutable std::mutex clock_mutex_;
    
    // Configuration store
    std::map<std::string, ConfigUpdate> config_updates_;
    std::map<std::string, std::string> current_config_;
    mutable std::mutex config_mutex_;
    
    // Resource snapshots
    std::map<std::string, ResourceSnapshot> resource_snapshots_;
    mutable std::mutex resource_mutex_;
    
    // Threading
    std::atomic<bool> running_{false};
    mutable std::mutex lifecycle_mutex_;
    std::thread gossip_thread_;
    std::thread anti_entropy_thread_;
    
    // Callbacks
    ConfigUpdateCallback config_update_callback_;
    ResourceSnapshotCallback resource_snapshot_callback_;
    mutable std::mutex callback_mutex_;
    std::optional<GossipSendFn> gossip_send_fn_;  // injected transport; nullopt → use client_
    mutable std::mutex gossip_send_fn_mutex_;
    
    // Statistics
    std::atomic<uint64_t> gossip_rounds_{0};
    std::atomic<uint64_t> messages_sent_{0};
    std::atomic<uint64_t> messages_received_{0};
    std::atomic<uint64_t> config_updates_sent_{0};
    std::atomic<uint64_t> config_updates_received_{0};
    std::atomic<uint64_t> resource_snapshots_sent_{0};
    std::atomic<uint64_t> resource_snapshots_received_{0};
    std::atomic<uint64_t> conflicts_resolved_{0};
    std::atomic<uint64_t> anti_entropy_syncs_{0};
    
    // Latency tracking
    std::vector<double> propagation_latencies_ms_;
    mutable std::mutex latency_mutex_;
    
    // Internal methods
    void gossipLoop();
    void antiEntropyLoop();
    void performGossipRound();
    void performAntiEntropyScan();
    
    std::vector<std::string> selectRandomPeers(size_t count);
    void sendGossipMessage(const std::string& peer_endpoint, 
                          const proto::GossipMessage& message);
    
    void handleConfigUpdate(const ConfigUpdate& update);
    void handleResourceSnapshot(const ResourceSnapshot& snapshot);
    
    bool shouldAcceptUpdate(const ConfigUpdate& update);
    void mergeVectorClock(const VectorClock& other);
    
    std::string generateUpdateId() const;
    proto::GossipMessage createHeartbeatMessage();
    proto::GossipMessage createConfigUpdateMessage(const ConfigUpdate& update);
    proto::GossipMessage createResourceSnapshotMessage(const ResourceSnapshot& snapshot);
    proto::GossipMessage createAntiEntropyMessage();
};

} // namespace sharding
} // namespace themis
