/**
 * @file distributed_gateway.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "server/api_gateway.h"
#include "sharding/raft_consensus.h"
#include "sharding/raft_state.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::server {

// ---------------------------------------------------------------------------
// GatewayNode – peer descriptor
// ---------------------------------------------------------------------------

/**
 * @brief Description of a single gateway node in the cluster.
 * 
 * Represents one instance of the distributed API gateway participating in cluster-wide
 * routing decisions, rate limiting, and failover logic. Each node has a unique identifier,
 * network address, and port for inter-node communication and client routing.
 * 
 * ### Member fields:
 * - node_id: Unique identifier (e.g., "gw-1", "gateway-east-1"); used in Raft consensus
 * - address: Hostname or IP address (IPv4 or IPv6)
 * - port: HTTP(S) port for accepting client requests (default 8080)
 * 
 * ### Equality
 * Two nodes are equal if they have the same node_id, regardless of address or port.
 * This is important for Raft cluster membership comparisons.
 * 
 * @note All fields are used by distributed consensus (Raft) and cluster topology management
 * @note Address and port must be reachable from all cluster nodes for inter-node communication
 * @note node_id must be globally unique across the cluster
 */
struct GatewayNode {
    std::string node_id;   ///< Unique node identifier (e.g. "gw-1")
    std::string address;   ///< Host address (IP or hostname)
    uint16_t    port{8080}; ///< HTTP(S) port

    bool operator==(const GatewayNode& o) const noexcept {
        return node_id == o.node_id;
    }
};

// ---------------------------------------------------------------------------
// GatewayRouteConfig – replicated routing rule
// ---------------------------------------------------------------------------

/**
 * @brief A single routing rule that is replicated via Raft across the cluster.
 * 
 * Defines how requests matching a path prefix are forwarded to upstream services.
 * All instances of this struct in the cluster converge to the same state via Raft consensus,
 * ensuring consistent routing behavior across nodes.
 * 
 * ### Routing Logic
 * When a request arrives:
 * 1. Match request path against path_prefix
 * 2. If matched, forward to upstream_url
 * 3. Apply per-request timeout
 * 4. Retry on transient errors (up to retry_count times)
 * 5. If circuit breaker enabled, track failures and trip on threshold
 * 
 * ### Member fields:
 * - path_prefix: Longest-prefix matching; "/api" matches "/api/v1/entities"
 * - upstream_url: Target service URL (e.g., "http://query-service:8081")
 * - timeout_ms: Request deadline (default 30s); includes all retries
 * - retry_count: Transient error retry attempts (default 2; excludes initial attempt)
 * - circuit_breaker_enabled: Enable circuit-breaker logic for this route
 * - circuit_breaker_failure_threshold: Consecutive failures before tripping (default 5)
 * 
 * @note This config is replicated to all gateway nodes via Raft for consistency
 * @note Changes to configs are applied immediately to ongoing requests
 * @note Upstream URL must be reachable from all gateway nodes
 * 
 * @see ClusterGatewayConfig for the full configuration snapshot
 */
    struct GatewayRouteConfig {
    std::string path_prefix;        ///< Path prefix to match (e.g. "/api/v1/query")
    std::string upstream_url;       ///< Target upstream URL
    uint32_t    timeout_ms{30000};  ///< Per-request timeout (ms)
    uint32_t    retry_count{2};     ///< Retry attempts on transient errors
    bool        circuit_breaker_enabled{true};
    uint32_t    circuit_breaker_failure_threshold{5};

    nlohmann::json toJson() const;
    static GatewayRouteConfig fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// ClusterGatewayConfig – full replicated config snapshot
// ---------------------------------------------------------------------------

/**
 * @brief Complete gateway configuration snapshot that is replicated across the cluster via Raft.
 * 
 * Represents the full state of cluster-wide gateway configuration at a given version.
 * All nodes converge to the same config version through Raft consensus, ensuring
 * consistent routing, rate limiting, and cluster behavior.
 * 
 * ### Configuration Elements
 * - version: Monotonically increasing integer; incremented on every config change
 * - routes: Ordered list of routing rules (first match wins)
 * - rate_limits: Per-client-key rate limits (overrides global limit if set)
 * - global_rate_limit_rps: Default cluster-wide rate limit (requests per second)
 * - updated_by: Node ID of the peer that committed this version (for tracing)
 * - updated_at: Timestamp when config was committed to Raft log
 * 
 * ### Cluster Convergence
 * When one node updates the config:
 * 1. Change is appended to Raft log
 * 2. Leader broadcasts change to followers
 * 3. Followers apply change when it's committed (quorum confirmation)
 * 4. All nodes now have identical config (same version, routes, rate_limits)
 * 
 * ### Rate Limiting Rules
 * - If rate_limits[client_key] is set, use per-client limit
 * - Otherwise, use global_rate_limit_rps
 * - Enforcement is per-node; Redis backend provides cluster-wide consistency
 * 
 * @note All fields must be serializable to/from JSON for Raft replication
 * @note version is used for concurrency control and conflict detection
 * @note updated_at is purely informational; uses system clock (may drift)
 * 
 * @see GatewayRouteConfig for individual routing rules
 * @see DistributedGateway for runtime management
 */
    struct ClusterGatewayConfig {
    uint64_t                        version{0};       ///< Monotonically increasing config version
    std::vector<GatewayRouteConfig> routes;           ///< Ordered routing rules
    std::unordered_map<std::string, uint32_t> rate_limits; ///< per-client-key limit (req/s)
    uint32_t    global_rate_limit_rps{100000};        ///< Cluster-wide default req/s
    std::string updated_by;                           ///< Node that committed this version
    std::chrono::system_clock::time_point updated_at;

    nlohmann::json toJson() const;
    static ClusterGatewayConfig fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// ConsistentHashRing – session affinity
// ---------------------------------------------------------------------------

/**
 * @brief Consistent-hash ring for sticky session routing across gateway nodes.
 * 
 * Implements consistent hashing to route WebSocket and SSE (Server-Sent Events) sessions
 * to the same gateway node throughout their lifetime. This ensures session affinity
 * and allows stateful operations (e.g., context variables in streaming queries).
 * 
 * ### Consistent Hashing Algorithm
 * 1. Create virtual nodes (replicas) for each physical GatewayNode
 * 2. Hash each virtual node into a ring (0 to MAX_UINT64)
 * 3. For a new session, hash the session_id and find the next higher virtual node on the ring
 * 4. Route session to the physical node owning that virtual node
 * 5. If a node joins/leaves, only keys between old and new node positions are re-hashed
 * 
 * ### Virtual Node Replication
 * Multiple virtual nodes per physical node reduce the impact of node failures
 * and improve load distribution. Default is 150 virtual nodes per physical node.
 * 
 * ### Sticky Routing Benefits
 * - Session context remains local to one node (no cross-node session store needed)
 * - Connection state (subscriptions, temporary tables) is preserved
 * - Reduced latency for stateful operations
 * - Simplified operational management
 * 
 * @note Used only for stateful protocols (WebSocket, SSE); REST requests are not pinned
 * @note When nodes join/leave, sessions are rebalanced but not interrupted
 * @note Separate from TCP connection routing; a single connection may span multiple nodes
 * 
 * @see GatewayNode for physical node descriptors
 */
class ConsistentHashRing {
public:
    /**
     * @brief Construct ring with the given virtual-node replication factor.
     * @param virtual_nodes Number of virtual nodes per physical node (default 150).
     */
    explicit ConsistentHashRing(uint32_t virtual_nodes = 150);

    /**
     * @brief Add a physical node to the ring.
     * @param node Node to add.
     */
    void addNode(const GatewayNode& node);

    /**
     * @brief Remove a physical node from the ring.
     * @param node_id Node identifier to remove.
     */
    void removeNode(const std::string& node_id);

    /**
     * @brief Resolve the responsible node for a session key.
     * @param session_key Session key (e.g. client IP + path).
     * @return Node responsible for the key, or std::nullopt if ring is empty.
     */
    std::optional<GatewayNode> getNode(const std::string& session_key) const;

    /**
     * @brief Return the current number of physical nodes in the ring.
     */
    std::size_t nodeCount() const;

private:
    uint32_t virtual_nodes_;
    // Sorted map: hash → GatewayNode
    std::map<uint64_t, GatewayNode> ring_;
    mutable std::shared_mutex mutex_;

    static uint64_t hash(const std::string& key, uint32_t replica);
};

// ---------------------------------------------------------------------------
// DistributedGateway
// ---------------------------------------------------------------------------

/**
 * @brief Distributed API Gateway – multi-node extension of APIGateway.
 *
 * Wraps an APIGateway instance and augments it with:
 *  1. Raft-based cluster membership and config replication.
 *  2. Automatic leader failover (target ≤ 500 ms).
 *  3. Consistent-hash ring for WebSocket/SSE session affinity.
 *  4. Quorum-aware config mutation (write rejected if not leader).
 *  5. Graceful degradation: last-known config used on quorum loss.
 */
class DistributedGateway {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Configuration for the distributed gateway layer.
     */
    struct Config {
        // Cluster identity
        std::string node_id     = "gw-1";    ///< This node's ID
        std::string bind_address = "0.0.0.0"; ///< Address to bind on
        uint16_t    bind_port   = 8080;       ///< Port to listen on

        // Cluster peers (including this node)
        std::vector<GatewayNode> cluster_nodes;

        // Raft tuning
        uint32_t election_timeout_min_ms = 150;
        uint32_t election_timeout_max_ms = 300;
        uint32_t heartbeat_interval_ms   = 50;

        // Failover
        std::chrono::milliseconds leader_failover_timeout{500};

        // Consistent-hash ring
        uint32_t virtual_nodes_per_peer = 150;

        // Degraded-mode behaviour
        bool continue_on_quorum_loss = true; ///< Use last-known config when quorum is lost

        // Wire-protocol retry (P5-S01)
        /// Maximum number of retry attempts on transient errors (5xx).
        /// Total attempts = max_retries + 1.  Default: 2 retries (3 total).
        uint32_t max_retries = 2;
        /// Base delay (ms) for exponential backoff between retry attempts.
        /// Actual delay for attempt N = min(retry_base_delay_ms × 2^N, retry_max_delay_ms).
        uint32_t retry_base_delay_ms = 50;
        /// Upper bound (ms) for exponential backoff to prevent unbounded waits.
        uint32_t retry_max_delay_ms = 2000;
    };

    // -----------------------------------------------------------------------
    // Construction / lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Construct DistributedGateway.
     *
     * @param config    Distributed-gateway configuration.
     * @param gateway   Underlying single-node APIGateway (non-null).
     */
    DistributedGateway(
        const Config& config,
        std::shared_ptr<APIGateway> gateway
    );

    ~DistributedGateway();

    // Prevent copying
    DistributedGateway(const DistributedGateway&) = delete;
    DistributedGateway& operator=(const DistributedGateway&) = delete;

    /**
     * @brief Start the distributed gateway (Raft consensus + hash ring).
     */
    void start();

    /**
     * @brief Gracefully stop the distributed gateway.
     */
    void stop();

    // -----------------------------------------------------------------------
    // Request routing
    // -----------------------------------------------------------------------

    /**
     * @brief Route an HTTP request.
     *
     * For stateful protocols (WebSocket/SSE, detected from the Upgrade header),
     * session affinity is applied via the consistent-hash ring.  All other
     * requests are routed through the underlying APIGateway.
     *
     * @param req           Incoming HTTP request.
     * @param local_handler Handler for locally-served requests.
     * @return HTTP response.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(
            const http::request<http::string_body>&)> local_handler
    );

    /**
     * @brief Determine the affinity node for a session key.
     *
     * Exposed for testing and monitoring.
     *
     * @param session_key Key used for consistent hashing.
     * @return Responsible GatewayNode, or nullopt if ring is empty.
     */
    std::optional<GatewayNode> resolveAffinityNode(
        const std::string& session_key) const;

    // -----------------------------------------------------------------------
    // Config management (Raft-replicated)
    // -----------------------------------------------------------------------

    /**
     * @brief Propose a new cluster-wide gateway configuration.
     *
     * The mutation is serialised as a Raft log entry and applied only after
     * it is committed by a quorum.  If this node is not the leader, or if
     * quorum is unavailable, the call returns false immediately.
     *
     * @param new_config  New configuration to replicate.
     * @return true if the entry was successfully committed.
     */
    bool proposeConfig(const ClusterGatewayConfig& new_config);

    /**
     * @brief Return the currently active (last-committed) configuration.
     */
    ClusterGatewayConfig getCurrentConfig() const;

    // -----------------------------------------------------------------------
    // Extensibility
    // -----------------------------------------------------------------------

    /**
     * @brief Register a local request handler for a path pattern.
     *
     * Delegates to the underlying APIGateway::registerHandler so that callers
     * do not need to hold a reference to the wrapped single-node gateway.
     *
     * @param pattern Path pattern (e.g., "/api/v1/custom/{name}")
     * @param handler Handler function
     */
    void registerHandler(
        const std::string& pattern,
        std::function<http::response<http::string_body>(
            const http::request<http::string_body>&)> handler
    );

    /**
     * @brief Register a deprecated API endpoint.
     *
     * Delegates to the underlying APIGateway::registerDeprecation so that
     * callers can register endpoint deprecations without a direct reference to
     * the wrapped single-node gateway.
     *
     * @param endpoint Endpoint path (e.g., "/api/v1/old-endpoint")
     * @param info     Deprecation details
     */
    void registerDeprecation(
        const std::string& endpoint,
        const APIDeprecationInfo& info
    );

    // -----------------------------------------------------------------------
    // Cluster status
    // -----------------------------------------------------------------------

    /**
     * @brief Check whether this node is the current Raft leader.
     */
    bool isLeader() const;

    /**
     * @brief Check whether the cluster currently has a quorum.
     */
    bool hasQuorum() const;

    /**
     * @brief Return the node ID of the current Raft leader.
     *
     * Returns an empty string when no leader has been elected yet.
     */
    std::string getLeaderId() const;

    /**
     * @brief Return a JSON status snapshot for monitoring/admin endpoints.
     */
    nlohmann::json getClusterStatus() const;

    // -----------------------------------------------------------------------
    // Internal – exposed for testing
    // -----------------------------------------------------------------------

    /**
     * @brief Apply a serialised config entry received from the Raft log.
     *
     * Called by the Raft replication callback; exposed for unit testing.
     *
     * @param entry_json JSON-encoded ClusterGatewayConfig.
     * @return true on success.
     */
    bool applyConfigEntry(const std::string& entry_json);

    /**
     * @brief Return true if @p status is a transient HTTP error that should
     *        trigger a retry (HTTP 429, 500, 502, 503, 504).
     *
     * Exposed publicly for unit tests.
     */
    static bool isTransientError(unsigned status) noexcept;

    /**
     * @brief Compute the exponential-backoff delay for a given retry attempt.
     *
     * Exposed publicly for unit tests.
     */
    static std::chrono::milliseconds retryDelay(uint32_t attempt,
                                                uint32_t base_ms,
                                                uint32_t max_ms) noexcept;

private:
    Config     config_;
    std::shared_ptr<APIGateway> gateway_;

    // Raft consensus engine
    std::unique_ptr<themisdb::sharding::RaftConsensus> raft_;

    // Current replicated configuration
    mutable std::shared_mutex config_mutex_;
    ClusterGatewayConfig      current_config_;
    bool                      quorum_lost_{false};

    // Consistent-hash ring for session affinity
    ConsistentHashRing hash_ring_;

    // Running flag
    std::atomic<bool> running_{false};

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Build a RaftConsensus::Config from our own config.
     */
    themisdb::sharding::RaftConsensus::Config buildRaftConfig() const;

    /**
     * @brief Rebuild the hash ring from the current cluster_nodes list.
     */
    void rebuildHashRing();

    /**
     * @brief Return the session key for a request (used for affinity).
     */
    std::string sessionKey(const http::request<http::string_body>& req) const;

    /**
     * @brief Return true if the request requires session affinity
     *        (WebSocket upgrade or SSE Accept header).
     */
    bool needsSessionAffinity(const http::request<http::string_body>& req) const;

    // (moved to public section for unit test access)
};

} // namespace themis::server
