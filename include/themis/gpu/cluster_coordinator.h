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
#include "themis/gpu/cluster_config.h"
#include "themis/gpu/cluster_topology.h"
#include "themis/gpu/device_discovery.h"

namespace themis {
namespace gpu {

/**
 * @brief Multi-node GPU cluster coordinator with NVLink/InfiniBand topology
 *        awareness.
 *
 * The coordinator owns a `GPUClusterTopology` snapshot and exposes methods
 * for topology-aware device selection, inter-node route planning, and cluster
 * health queries.
 *
 * Single-node fallback
 * --------------------
 * When constructed with an empty `ClusterConfig` (world_size == 1) the
 * coordinator operates in single-node mode: `selectDevice()` delegates to
 * the local NVLink topology and all inter-node methods return empty results.
 * This preserves backward compatibility with existing single-node deployments.
 *
 * Multi-node mode
 * ---------------
 * 1. Call `initialize(config, devices)` once at startup.
 * 2. Register peer nodes via `registerNode()` as they become known.
 * 3. Call `updateTopology()` after all peers have registered.
 * 4. Use `selectDevice()` / `selectNodeForTransfer()` for placement.
 *
 * Thread safety: all public methods are protected by an internal mutex.
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
    // Placement result
    // -----------------------------------------------------------------------
    struct Placement {
        std::string node_id;         ///< Target node (empty = local node)
        int         device_index = -1;///< Target GPU device ordinal (-1 = CPU)
        InterconnectType route   = InterconnectType::CPU;
        float       bandwidth_gbps = 0.0f;
        std::string reason;          ///< Human-readable placement rationale
    };

    // -----------------------------------------------------------------------
    // Cluster health summary
    // -----------------------------------------------------------------------
    struct ClusterHealth {
        int  total_nodes     = 0;
        int  healthy_nodes   = 0;
        int  total_gpus      = 0;
        bool nvlink_available      = false;
        bool infiniband_available  = false;
    };

    // -----------------------------------------------------------------------
    // Singleton (production path)
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
     * @brief Initialise the coordinator with a cluster config and the local
     *        device list.
     *
     * Detects intra-node NVLink topology and registers the local node.
     * Safe to call multiple times; re-initialises in place.
     *
     * @param config   Cluster configuration (empty = single-node mode).
     * @param devices  Local GPU device list from `DeviceDiscovery::Enumerate()`.
     */
    void initialize(const ClusterConfig&           config,
                    const std::vector<DeviceInfo>& devices);

    /**
     * @brief Register a remote peer node.
     *
     * Adds the node to the topology and, when InfiniBand is configured,
     * records an estimated inter-node link.
     *
     * @param node        Remote node descriptor.
     * @param ib_bw_gbps  Estimated InfiniBand bandwidth (0 = use default 100 Gb/s).
     */
    void registerNode(const ClusterNode& node, float ib_bw_gbps = 0.0f);

    /**
     * @brief Remove a peer node (e.g. on disconnect / failure).
     */
    void removeNode(const std::string& node_id);

    /**
     * @brief Refresh the topology snapshot after node additions/removals.
     *
     * Re-runs intra-node detection and rebuilds the inter-node link table.
     */
    void updateTopology();

    // -----------------------------------------------------------------------
    // Placement & scheduling
    // -----------------------------------------------------------------------

    /**
     * @brief Select the best local GPU for a new work item.
     *
     * Considers NVLink bandwidth when multiple GPUs are present and
     * `cluster_config_.enable_nvlink` is true.
     *
     * @param required_vram_bytes  Minimum free VRAM needed (0 = any).
     * @return Placement with device_index set; node_id is the local node.
     */
    Placement selectDevice(uint64_t required_vram_bytes = 0) const;

    /**
     * @brief Select the best destination node for a data transfer.
     *
     * Returns the node reachable via the highest-bandwidth InfiniBand link.
     * Falls back to the local node when no inter-node links are registered.
     *
     * @param src_node_id  Source node (empty = local node).
     * @return Placement with node_id and route filled in.
     */
    Placement selectNodeForTransfer(const std::string& src_node_id = "") const;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /** @brief Return a read-only reference to the current topology snapshot. */
    const GPUClusterTopology& topology() const noexcept { return topology_; }

    /** @brief Return the active cluster configuration. */
    const ClusterConfig& clusterConfig() const noexcept { return config_; }

    /** @brief Return a cluster-wide health summary. */
    ClusterHealth clusterHealth() const;

    /** @brief True when `initialize()` has been called successfully. */
    bool isInitialized() const noexcept { return initialized_; }
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
    bool                      initialized_ = false;
    ClusterConfig             config_;
    std::vector<DeviceInfo>   local_devices_;
    GPUClusterTopology        topology_;
    bool                      initialized_    = false;
    bool                      is_coordinator_ = false;
    ClusterConfig             config_;
    std::vector<NodeInfo>     nodes_;

    // Internal helpers — called under mutex_.
    NodeInfo* findNode(const std::string& id);
};

} // namespace gpu
} // namespace themis
