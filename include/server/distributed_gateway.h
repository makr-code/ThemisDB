/**
 * @file distributed_gateway.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

    struct GatewayRouteConfig {
    std::string path_prefix;        ///< Path prefix to match (e.g. "/api/v1/query")
    std::string upstream_url;       ///< Target upstream URL
    uint32_t    timeout_ms{30000};  ///< Per-request timeout (ms)
    uint32_t    retry_count{2};     ///< Retry attempts on transient errors
    bool        circuit_breaker_enabled{true};
    uint32_t    circuit_breaker_failure_threshold{5};

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static GatewayRouteConfig fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// ClusterGatewayConfig – full replicated config snapshot
// ---------------------------------------------------------------------------

    struct ClusterGatewayConfig {
    uint64_t                        version{0};       ///< Monotonically increasing config version
    std::vector<GatewayRouteConfig> routes;           ///< Ordered routing rules
    std::unordered_map<std::string, uint32_t> rate_limits; ///< per-client-key limit (req/s)
    uint32_t    global_rate_limit_rps{100000};        ///< Cluster-wide default req/s
    std::string updated_by;                           ///< Node that committed this version
    std::chrono::system_clock::time_point updated_at;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ClusterGatewayConfig fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// ConsistentHashRing – session affinity
// ---------------------------------------------------------------------------

class ConsistentHashRing {
public:
    explicit ConsistentHashRing(uint32_t virtual_nodes = 150);

    /**
     * @brief Add Node.
     * @param[in] node Input parameter.
     */
    void addNode(const GatewayNode& node);

    /**
     * @brief Remove Node.
     * @param[in] node_id Identifier of the node.
     */
    void removeNode(const std::string& node_id);

    /**
     * @brief Get Node.
     * @param[in] session_key Input parameter.
     * @return Return value.
     */
    std::optional<GatewayNode> getNode(const std::string& session_key) const;

    /**
     * @brief Node Count.
     * @return Return value.
     */
    std::size_t nodeCount() const;

private:
    uint32_t virtual_nodes_;
    // Sorted map: hash → GatewayNode
    std::map<uint64_t, GatewayNode> ring_;
    mutable std::shared_mutex mutex_;

    /**
     * @brief Hash.
     * @param[in] key Input parameter.
     * @param[in] replica Input parameter.
     * @return Return value.
     */
    static uint64_t hash(const std::string& key, uint32_t replica);
};

// ---------------------------------------------------------------------------
// DistributedGateway
// ---------------------------------------------------------------------------

class DistributedGateway {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

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
        uint32_t max_retries = 2;
        uint32_t retry_base_delay_ms = 50;
        uint32_t retry_max_delay_ms = 2000;
    };

    // -----------------------------------------------------------------------
    // Construction / lifecycle
    // -----------------------------------------------------------------------

    DistributedGateway(
        const Config& config,
        std::shared_ptr<APIGateway> gateway
    );

    ~DistributedGateway();

    // Prevent copying
    DistributedGateway(const DistributedGateway&) = delete;
    DistributedGateway& operator=(const DistributedGateway&) = delete;

    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Stop.
     */
    void stop();

    // -----------------------------------------------------------------------
    // Request routing
    // -----------------------------------------------------------------------

    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(
            const http::request<http::string_body>&)> local_handler
    );

    /**
     * @brief Resolve Affinity Node.
     * @param[in] session_key Input parameter.
     * @return Return value.
     */
    std::optional<GatewayNode> resolveAffinityNode(
        const std::string& session_key) const;

    // -----------------------------------------------------------------------
    // Config management (Raft-replicated)
    // -----------------------------------------------------------------------

    /**
     * @brief Propose Config.
     * @param[in] new_config Input parameter.
     * @return True when the operation succeeds.
     */
    bool proposeConfig(const ClusterGatewayConfig& new_config);

    /**
     * @brief Get Current Config.
     * @return Return value.
     */
    ClusterGatewayConfig getCurrentConfig() const;

    // -----------------------------------------------------------------------
    // Extensibility
    // -----------------------------------------------------------------------

    void registerHandler(
        const std::string& pattern,
        std::function<http::response<http::string_body>(
            const http::request<http::string_body>&)> handler
    );

    /**
     * @brief Register Deprecation.
     * @param[in] endpoint Input parameter.
     * @param[in] info Input parameter.
     */
    void registerDeprecation(
        const std::string& endpoint,
        const APIDeprecationInfo& info
    );

    // -----------------------------------------------------------------------
    // Cluster status
    // -----------------------------------------------------------------------

    /**
     * @brief Is Leader.
     * @return True when the operation succeeds.
     */
    bool isLeader() const;

    /**
     * @brief Has Quorum.
     * @return True when the operation succeeds.
     */
    bool hasQuorum() const;

    /**
     * @brief Get Leader Id.
     * @return Return value.
     */
    std::string getLeaderId() const;

    /**
     * @brief Get Cluster Status.
     * @return Return value.
     */
    nlohmann::json getClusterStatus() const;

    // -----------------------------------------------------------------------
    // Internal – exposed for testing
    // -----------------------------------------------------------------------

    /**
     * @brief Apply Config Entry.
     * @param[in] entry_json Input parameter.
     * @return True when the operation succeeds.
     */
    bool applyConfigEntry(const std::string& entry_json);

    /**
     * @brief Is Transient Error.
     * @param[in] status Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isTransientError(unsigned status) noexcept;

    /**
     * @brief Retry Delay.
     * @param[in] attempt Input parameter.
     * @param[in] base_ms Input parameter.
     * @param[in] max_ms Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
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
     * @brief Build Raft Config.
     * @return Return value.
     */
    themisdb::sharding::RaftConsensus::Config buildRaftConfig() const;

    /**
     * @brief Rebuild Hash Ring.
     */
    void rebuildHashRing();

    /**
     * @brief Session Key.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    std::string sessionKey(const http::request<http::string_body>& req) const;

    /**
     * @brief Needs Session Affinity.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     */
    bool needsSessionAffinity(const http::request<http::string_body>& req) const;

    // (moved to public section for unit test access)
};

} // namespace themis::server
