/**
 * @file gossip_protocol.h
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
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declarations
class ShardTopology;
class MTLSClient;

/**
 * @brief Gossip Protocol for Cluster Membership and Failure Detection
 * 
 * Sources:
 * - Algorithm: Gossip Protocol (Epidemic/Anti-Entropy)
 * - Inspired by: Apache Cassandra's Gossip Implementation
 * - Paper: van Renesse, R., Birman, K. P., & Vogels, W. (2003)
 *          "Astrolabe: A robust and scalable technology for distributed system monitoring,
 *           management, and data mining"
 *          ACM Transactions on Computer Systems, 21(2), 164-206
 * - Cassandra: https://cassandra.apache.org/doc/latest/architecture/gossip.html
 * - License: Apache 2.0 (Cassandra)
 * - ThemisDB Implementation: Custom gossip protocol with:
 *   - Integration with VCC-URN sharding
 *   - mTLS certificate-based peer validation
 *   - Datacenter/region-aware topology
 *   - Optimized for database cluster state synchronization
 */

/**
 * Peer Information
 * Contains metadata about a known peer in the gossip network.
 */
struct PeerInfo {
    std::string peer_id;                     // Unique peer identifier
    std::string endpoint;                    // HTTP endpoint (host:port)
    std::string datacenter;                  // Datacenter location
    std::string region;                      // Geographic region
    uint64_t version;                        // Version vector for anti-entropy
    std::chrono::system_clock::time_point last_seen;
    std::chrono::system_clock::time_point first_seen;
    bool is_healthy = true;
    std::string certificate_serial;          // For mTLS validation
    
    // Serialize to JSON
    nlohmann::json toJson() const {
        return {
            {"peer_id", peer_id},
            {"endpoint", endpoint},
            {"datacenter", datacenter},
            {"region", region},
            {"version", version},
            {"last_seen", std::chrono::duration_cast<std::chrono::milliseconds>(
                last_seen.time_since_epoch()).count()},
            {"is_healthy", is_healthy},
            {"certificate_serial", certificate_serial}
        };
    }
    
    // Deserialize from JSON
    static PeerInfo fromJson(const nlohmann::json& j) {
        PeerInfo info;
        info.peer_id = j.value("peer_id", "");
        info.endpoint = j.value("endpoint", "");
        info.datacenter = j.value("datacenter", "");
        info.region = j.value("region", "");
        info.version = j.value("version", 0ULL);
        info.is_healthy = j.value("is_healthy", true);
        info.certificate_serial = j.value("certificate_serial", "");
        
        auto last_seen_ms = j.value("last_seen", 0LL);
        info.last_seen = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(last_seen_ms));
        info.first_seen = std::chrono::system_clock::now();
        
        return info;
    }
};

/**
 * Gossip Message
 * Message exchanged between peers during gossip rounds.
 */
struct GossipMessage {
    std::string message_id;
    std::string sender_id;
    std::string message_type;  // "heartbeat", "peer_list", "peer_update", "peer_leave"
    nlohmann::json payload;
    uint64_t timestamp;
    std::string signature;     // RSA signature for authenticity
    
    nlohmann::json toJson() const {
        return {
            {"message_id", message_id},
            {"sender_id", sender_id},
            {"message_type", message_type},
            {"payload", payload},
            {"timestamp", timestamp},
            {"signature", signature}
        };
    }
    
    static GossipMessage fromJson(const nlohmann::json& j) {
        GossipMessage msg;
        msg.message_id = j.value("message_id", "");
        msg.sender_id = j.value("sender_id", "");
        msg.message_type = j.value("message_type", "");
        msg.payload = j.value("payload", nlohmann::json{});
        msg.timestamp = j.value("timestamp", 0ULL);
        msg.signature = j.value("signature", "");
        return msg;
    }
};

/**
 * Gossip Protocol Configuration
 */
struct GossipConfig {
    bool enabled = false;                    // Default: disabled
    uint32_t gossip_interval_sec = 30;       // Heartbeat interval
    uint32_t max_peers = 100;                // Maximum peers to track
    uint32_t fanout = 3;                     // Number of peers to gossip to each round
    uint32_t peer_timeout_sec = 120;         // Mark peer unhealthy after timeout
    uint32_t peer_remove_sec = 300;          // Remove peer after extended timeout
    std::vector<std::string> seed_nodes;     // Bootstrap nodes
    
    // Security settings
    bool require_mtls = true;                // Require mTLS for peer communication
    bool validate_certificates = true;       // Validate peer certificates
    uint32_t max_message_age_sec = 60;       // Reject old messages (replay protection)
    uint32_t rate_limit_per_peer = 10;       // Max messages per peer per interval
    
    // Local node settings
    std::string local_peer_id;
    std::string local_endpoint;
    std::string local_datacenter;
    std::string local_region;
    std::string private_key_path;            // For signing messages
    std::string peer_public_keys_dir;        // GOS-2: dir containing <peer_id>.pem public keys for signature verification
};

/**
 * Gossip Protocol
 * 
 * Implements a peer-to-peer gossip protocol for decentralized peer discovery
 * in ThemisDB clusters. Based on SWIM (Scalable Weakly-consistent Infection-style
 * process group Membership) protocol principles.
 * 
 * Features:
 * - Periodic heartbeat with random peer selection
 * - Peer list exchange with version vectors
 * - Anti-entropy for consistency
 * - mTLS-authenticated communication
 * - Rate limiting for DoS protection
 * 
 * Example:
 *   GossipConfig config;
 *   config.enabled = true;
 *   config.seed_nodes = {"peer1.example.com:8080"};
 *   
 *   auto gossip = std::make_unique<GossipProtocol>(config, topology);
 *   gossip->start();
 *   
 *   // ... later
 *   gossip->stop();
 */
class GossipProtocol {
public:
    using PeerDiscoveryCallback = std::function<void(const PeerInfo& peer)>;
    using PeerLostCallback = std::function<void(const std::string& peer_id)>;

    /**
     * @brief Raft membership gate callback type (CC-4).
     *
     * When set via @ref setRaftMembershipGateFn, this function is called
     * before any gossip-discovered peer is written to the ShardTopology.
     *
     * @param peer_id  Gossip-level peer identifier.
     * @param endpoint Network endpoint (host:port) of the peer.
     * @return true  — the peer has been admitted through the Raft
     *                 joint-consensus membership protocol and MAY be added
     *                 to the routing topology.
     * @return false — the peer has NOT been admitted; it stays in the gossip
     *                 `peers_` map for health tracking but MUST NOT appear in
     *                 the routing topology or influence quorum.
     *
     * **Exception safety:** The function must not throw.
     */
    using RaftMembershipGateFn = std::function<bool(const std::string& peer_id,
                                                     const std::string& endpoint)>;
    
    /**
     * Construct GossipProtocol with configuration
     * @param config Configuration parameters
     * @param topology ShardTopology to update with discovered peers
     */
    GossipProtocol(const GossipConfig& config, 
                   std::shared_ptr<ShardTopology> topology);
    
    ~GossipProtocol();
    
    /**
     * Start the gossip protocol
     * Begins periodic gossip rounds and listens for incoming messages
     */
    void start();
    
    /**
     * Stop the gossip protocol
     * Sends leave message to peers and stops all threads
     */
    void stop();
    
    /**
     * Check if gossip protocol is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * Get all known peers
     * @return Map of peer_id to PeerInfo
     */
    std::map<std::string, PeerInfo> getPeers() const;
    
    /**
     * Get healthy peers only
     * @return Vector of healthy peer endpoints
     */
    std::vector<PeerInfo> getHealthyPeers() const;
    
    /**
     * Get peer count
     */
    size_t getPeerCount() const;
    
    /**
     * Manually add a peer (for seed nodes or manual discovery)
     * @param peer Peer information
     */
    void addPeer(const PeerInfo& peer);
    
    /**
     * Remove a peer
     * @param peer_id Peer identifier to remove
     */
    void removePeer(const std::string& peer_id);
    
    /**
     * Handle incoming gossip message (called by HTTP endpoint)
     * @param message Received gossip message
     * @return Response message
     */
    GossipMessage handleMessage(const GossipMessage& message);
    
    /**
     * Register a custom handler for a specific gossip message type.
     *
     * The handler is invoked inside `handleMessage()` before the built-in
     * type dispatch (heartbeat / peer_list / peer_leave).  This enables
     * external modules such as the distributed_knowledge layer to extend the
     * gossip bus without modifying the transport protocol.
     *
     * Thread-safety: handler map is protected by the internal peers mutex.
     * Duplicate registration: the new handler replaces the previous one and
     * a warning is written to stderr.
     *
     * @param message_type  Payload type string, e.g. "adapter_capability"
     * @param handler       Callable invoked with the full `GossipMessage`
     */
    void registerCustomHandler(
        const std::string& message_type,
        std::function<void(const GossipMessage&)> handler
    );

    /**
     * Register callback for peer discovery
     * @param callback Function called when new peer is discovered
     */
    void onPeerDiscovered(PeerDiscoveryCallback callback);
    
    /**
     * Register callback for peer loss
     * @param callback Function called when peer is lost
     */
    void onPeerLost(PeerLostCallback callback);

    /**
     * @brief Register a Raft membership gate for gossip-driven topology mutations (CC-4).
     *
     * When a gate function is registered, any gossip-discovered peer that is
     * NOT yet in the ShardTopology is admitted only if the gate returns `true`.
     * A peer rejected by the gate is still tracked in the internal `peers_` map
     * so that it participates in health-monitoring rounds, but it is NEVER
     * written to the routing topology and NEVER influences quorum calculations.
     *
     * When no gate is registered (the default), gossip-discovered peers are
     * added to the topology with a one-time warning — preserving backward
     * compatibility while clearly marking the unprotected path.
     *
     * @param fn  Gate function.  Pass `nullptr` to remove a previously
     *            registered gate and revert to the legacy warn+add behaviour.
     *
     * Thread-safety: the function pointer is stored under `peers_mutex_`.
     */
    void setRaftMembershipGateFn(RaftMembershipGateFn fn);
    
    /**
     * Get gossip statistics
     * @return JSON with gossip metrics
     */
    nlohmann::json getStatistics() const;
    
    /**
     * Get current configuration
     */
    const GossipConfig& getConfig() const { return config_; }

private:
    GossipConfig config_;
    std::shared_ptr<ShardTopology> topology_;
    std::unique_ptr<MTLSClient> client_;
    
    // Peer tracking
    std::map<std::string, PeerInfo> peers_;
    mutable std::mutex peers_mutex_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread gossip_thread_;
    std::thread cleanup_thread_;
    
    // Custom handlers registered via registerCustomHandler()
    std::map<std::string, std::function<void(const GossipMessage&)>> custom_handlers_;

    // Callbacks
    PeerDiscoveryCallback on_peer_discovered_;
    PeerLostCallback on_peer_lost_;
    // CC-4: Raft membership gate — guarded by peers_mutex_
    RaftMembershipGateFn raft_membership_gate_fn_;
    
    // Statistics
    std::atomic<uint64_t> messages_sent_{0};
    std::atomic<uint64_t> messages_received_{0};
    std::atomic<uint64_t> peers_discovered_{0};
    std::atomic<uint64_t> peers_lost_{0};
    std::atomic<uint64_t> gossip_rounds_{0};
    
    // Rate limiting
    std::map<std::string, std::vector<std::chrono::system_clock::time_point>> rate_limit_map_;
    std::mutex rate_limit_mutex_;
    
    // Internal methods
    void gossipLoop();
    void cleanupLoop();
    void performGossipRound();
    void sendHeartbeat(const PeerInfo& peer);
    void sendPeerList(const PeerInfo& peer);
    void sendLeaveMessage();
    void bootstrapFromSeedNodes();
    
    std::vector<PeerInfo> selectRandomPeers(size_t count);
    void mergePeerList(const std::vector<PeerInfo>& peers);
    void updatePeerHealth();
    void syncWithTopology();
    // Requires peers_mutex_ already held by the calling thread.
    void syncWithTopologyLocked();
    
    std::string generateMessageId() const;
    std::string signMessage(const GossipMessage& message) const;
    bool verifyMessage(const GossipMessage& message) const;
    bool checkRateLimit(const std::string& peer_id);
    
    GossipMessage createHeartbeatMessage() const;
    GossipMessage createPeerListMessage() const;
    GossipMessage createLeaveMessage() const;
};

} // namespace sharding
} // namespace themis
