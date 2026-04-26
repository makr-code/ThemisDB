/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_gateway.h                              ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:46:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     432                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c4f67f41ce  2026-03-11  fix(server): audit gaps – quorum detection CRITICAL log, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file distributed_gateway.h
 * @brief Distributed API Gateway with Raft-based configuration synchronisation.
 *
 * Implements a multi-node gateway cluster (3 or 5 nodes) where routing
 * rules and rate-limit configuration are replicated through Raft log entries.
 *
 * Key properties:
 *  - Leader-failover in ≤ 500 ms.
 *  - Config propagation across 5 LAN nodes in ≤ 100 ms.
 *  - Session affinity for WebSocket/SSE connections via a consistent-hash ring.
 *  - Quorum-loss resilience: gateway continues with last-known config and emits
 *    a CRITICAL-level alert; config writes are rejected until quorum is restored.
 *  - Split-brain safety: config mutations are refused when the node is not the
 *    Raft leader.
 *
 * Architecture:
 * ```
 *  Client → Load Balancer → [GatewayNode A]
 *                         → [GatewayNode B]  ← Raft cluster
 *                         → [GatewayNode C]
 *                               ↓
 *                         (config replicated via Raft log)
 *                               ↓
 *                         Backend services / shards
 * ```
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
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
 * @brief A single routing rule that is replicated via Raft.
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
 * @brief Complete gateway configuration that is replicated across the cluster.
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
 * @brief Consistent-hash ring for sticky routing of WebSocket/SSE sessions.
 *
 * Each virtual node is mapped to a physical GatewayNode; clients are routed
 * to the same node for the lifetime of their session.
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
};

} // namespace themis::server
