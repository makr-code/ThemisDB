/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cluster_coordinator.h                              ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     220                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Cluster configuration (new optional config block)
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for multi-node GPU cluster coordination.
 *
 * This config block is optional; when `enabled == false` the
 * GPUClusterCoordinator operates in STANDALONE mode and exposes a
 * single synthetic "local" node so the rest of the code can always
 * call it without conditional compilation.
 *
 * Typical YAML equivalent (coordinator node)
 * ------------------------------------------
 * ```yaml
 * cluster:
 *   enabled: true
 *   mode: coordinator
 *   coordinator_address: 192.168.1.100:9000
 *   nodes:
 *     - id: node-1
 *       address: 192.168.1.101:9000
 *       gpu_count: 4
 *     - id: node-2
 *       address: 192.168.1.102:9000
 *       gpu_count: 4
 * ```
 *
 * Typical YAML equivalent (worker node)
 * --------------------------------------
 * ```yaml
 * cluster:
 *   enabled: true
 *   mode: worker
 *   coordinator_address: 192.168.1.100:9000
 *   node_id: node-1
 * ```
 */
struct ClusterConfig {
    /// When false the coordinator runs in STANDALONE mode (single local node).
    bool enabled = false;

    enum class Mode {
        STANDALONE,   ///< Single-node operation (default)
        COORDINATOR,  ///< This node is the cluster coordinator
        WORKER,       ///< This node is a worker managed by a coordinator
    };
    Mode mode = Mode::STANDALONE;

    /// Address of the coordinator node ("host:port"); required in WORKER mode.
    std::string coordinator_address;

    /// Identity of this node in the cluster; required in WORKER mode.
    std::string node_id;

    /// Interval between heartbeat ticks, in milliseconds.
    uint32_t heartbeat_interval_ms = 5000;

    /// Time after which a node with no heartbeat is considered offline, in ms.
    uint32_t node_timeout_ms = 15000;

    /// Static cluster membership (used by the coordinator to seed node list).
    struct NodeEntry {
        std::string id;
        std::string address;   ///< "host:port"
        int         gpu_count = 0;
    };
    std::vector<NodeEntry> nodes;
};

// ---------------------------------------------------------------------------
// GPUClusterCoordinator
// ---------------------------------------------------------------------------

/**
 * @brief Multi-node GPU cluster coordinator.
 *
 * Manages the set of cluster nodes that expose GPU resources, tracks their
 * health via heartbeats, and selects the best node for incoming GPU work
 * requests.
 *
 * Node roles
 * ----------
 * - COORDINATOR — maintains the authoritative node registry and selects
 *   target nodes for work placement.
 * - WORKER      — registers with the coordinator and sends periodic heartbeats.
 *
 * Work placement strategy
 * -----------------------
 * `selectNode()` uses a LEAST_LOADED strategy: the online node with the
 * highest reported free VRAM is selected.  When all nodes are offline the
 * method returns nullptr so callers can fall back to local execution.
 *
 * Heartbeat / timeout
 * -------------------
 * `updateHeartbeat()` must be called by the transport layer whenever a
 * heartbeat message is received from a remote node.
 * `expireStaleNodes()` marks nodes whose last heartbeat exceeds
 * `ClusterConfig::node_timeout_ms` as OFFLINE; callers should invoke this
 * periodically (e.g. from a background thread or before `selectNode()`).
 *
 * Thread safety: all public methods are protected by an internal mutex.
 *
 * Integration notes
 * -----------------
 * The actual network transport (TCP / gRPC / RDMA) is external to this class
 * and is expected to call `registerNode()`, `updateHeartbeat()`, and
 * `markNodeOffline()` on behalf of remote peers.  This decoupling allows the
 * coordinator logic to be fully tested without network hardware.
 */
class GPUClusterCoordinator {
public:
    // -----------------------------------------------------------------------
    // Node state
    // -----------------------------------------------------------------------
    enum class NodeRole   { COORDINATOR, WORKER };
    enum class NodeStatus { ONLINE, OFFLINE, DEGRADED };

    struct NodeInfo {
        std::string id;
        std::string address;
        NodeRole    role              = NodeRole::WORKER;
        NodeStatus  status            = NodeStatus::OFFLINE;
        int         gpu_count         = 0;
        uint64_t    total_vram_bytes  = 0;
        uint64_t    free_vram_bytes   = 0;
        std::chrono::steady_clock::time_point last_heartbeat{};
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUClusterCoordinator& GetInstance() {
        static GPUClusterCoordinator inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Initialise the coordinator from @p config.
     *
     * Seeds the node list from `ClusterConfig::nodes` (COORDINATOR mode) or
     * registers the local node (STANDALONE / WORKER mode).
     *
     * @return true on success.
     */
    bool initialize(const ClusterConfig& config);

    // -----------------------------------------------------------------------
    // Node management
    // -----------------------------------------------------------------------

    /**
     * @brief Register or update a cluster node.
     *
     * If a node with `node.id` already exists its information is updated;
     * otherwise a new entry is created.
     */
    void registerNode(const NodeInfo& node);

    /**
     * @brief Remove a node from the cluster registry.
     *
     * @return false if no node with that ID exists.
     */
    bool deregisterNode(const std::string& node_id);

    /**
     * @brief Record a heartbeat from @p node_id and update its free VRAM.
     *
     * Sets the node's status to ONLINE and records the current timestamp.
     */
    void updateHeartbeat(const std::string& node_id,
                         uint64_t           free_vram_bytes);

    /**
     * @brief Immediately mark @p node_id as OFFLINE.
     */
    void markNodeOffline(const std::string& node_id);

    /**
     * @brief Scan the node list and mark any nodes whose last heartbeat
     *        exceeds the configured timeout as OFFLINE.
     *
     * Should be called periodically by a background health-check thread.
     */
    void expireStaleNodes();

    // -----------------------------------------------------------------------
    // Work placement
    // -----------------------------------------------------------------------

    /**
     * @brief Select the best online node for the next GPU work item.
     *
     * Uses LEAST_LOADED strategy: picks the ONLINE node with the highest
     * free VRAM that satisfies @p required_vram_bytes.
     *
     * @param required_vram_bytes  Minimum free VRAM required (0 = any).
     * @return Pointer to the selected NodeInfo (stable reference into the
     *         internal list), or nullptr if no suitable node is found.
     */
    const NodeInfo* selectNode(uint64_t required_vram_bytes = 0);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    std::vector<NodeInfo> getClusterNodes() const;
    std::vector<NodeInfo> getOnlineNodes()  const;

    size_t totalNodes()       const;
    size_t onlineNodeCount()  const;

    bool isCoordinator() const noexcept { return is_coordinator_; }
    bool isInitialized()  const noexcept { return initialized_; }

    const ClusterConfig& config() const noexcept { return config_; }

private:
    GPUClusterCoordinator() = default;

    mutable std::mutex        mutex_;
    bool                      initialized_    = false;
    bool                      is_coordinator_ = false;
    ClusterConfig             config_;
    std::vector<NodeInfo>     nodes_;

    // Internal helpers — called under mutex_.
    NodeInfo* findNode(const std::string& id);
};

} // namespace gpu
} // namespace themis
