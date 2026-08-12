/**
 * @file cluster_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * @brief Multi-node GPU cluster coordinator.
 *
 * Combines topology-aware intra-node device scheduling (NVLink / InfiniBand)
 * with a node registry that tracks health via heartbeats and selects the best
 * node for incoming GPU work requests.
 *
 * Modes of operation
 * ------------------
 * - STANDALONE — single-node pass-through; exposes a synthetic "local" node
 *   so callers never need conditional compilation.
 * - COORDINATOR — maintains the authoritative node registry and handles
 *   placement requests from workers.
 * - WORKER — registers with the coordinator and sends periodic heartbeats.
 *
 * Typical usage (topology-aware, multi-node)
 * ------------------------------------------
 * 1. Call `initialize(config, devices)` once at startup.
 * 2. Register peer nodes via `registerNode(ClusterNode, bw)`.
 * 3. Call `updateTopology()` after all peers have registered.
 * 4. Use `selectDevice()` / `selectNodeForTransfer()` for placement.
 *
 * Typical usage (registry, multi-node)
 * -------------------------------------
 * 1. Call `initialize(config)` once at startup.
 * 2. Transport layer calls `registerNode(NodeInfo)` / `updateHeartbeat()`.
 * 3. Call `selectNode()` to pick the least-loaded online node.
 * 4. Call `expireStaleNodes()` periodically from a health-check thread.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 *
 * Integration notes
 * -----------------
 * The actual network transport (TCP / gRPC / RDMA) is external to this class.
 * The transport layer is expected to call `registerNode()`, `updateHeartbeat()`,
 * and `markNodeOffline()` on behalf of remote peers, allowing the coordinator
 * to be fully tested without GPU hardware or a live network.
 */
class GPUClusterCoordinator {
public:
    // -----------------------------------------------------------------------
    // Node state (registry path)
    // -----------------------------------------------------------------------
    enum class NodeRole   { COORDINATOR, WORKER };
    enum class NodeStatus { ONLINE, OFFLINE, DEGRADED };

    struct NodeInfo {
        std::string id;
        std::string address;
        NodeRole    role             = NodeRole::WORKER;
        NodeStatus  status           = NodeStatus::OFFLINE;
        int         gpu_count        = 0;
        uint64_t    total_vram_bytes = 0;
        uint64_t    free_vram_bytes  = 0;
        std::chrono::steady_clock::time_point last_heartbeat{};
    };

    // -----------------------------------------------------------------------
    // Placement result (topology-aware path)
    // -----------------------------------------------------------------------
    struct Placement {
        std::string      node_id;                          ///< Target node (empty = local)
        int              device_index    = -1;             ///< Target GPU ordinal (-1 = CPU)
        InterconnectType route           = InterconnectType::CPU;
        float            bandwidth_gbps  = 0.0f;
        std::string      reason;                           ///< Human-readable rationale
    };

    // -----------------------------------------------------------------------
    // Cluster health summary
    // -----------------------------------------------------------------------
    struct ClusterHealth {
        int  total_nodes           = 0;
        int  healthy_nodes         = 0;
        int  total_gpus            = 0;
        bool nvlink_available      = false;
        bool infiniband_available  = false;
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUClusterCoordinator& GetInstance() {
        static GPUClusterCoordinator inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Lifecycle — topology-aware path
    // -----------------------------------------------------------------------

    /**
     * @brief Initialise with a cluster config and the local device list.
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
     * @brief Register a remote peer node (topology-aware path).
     *
     * Adds the node to the topology and, when InfiniBand is configured,
     * records an estimated inter-node link.
     *
     * @param node        Remote node descriptor.
     * @param ib_bw_gbps  Estimated InfiniBand bandwidth (0 = use default).
     */
    void registerNode(const ClusterNode& node, float ib_bw_gbps = 0.0f);

    /**
     * @brief Remove a peer node (topology-aware path).
     */
    void removeNode(const std::string& node_id);

    /**
     * @brief Rebuild the topology snapshot after node additions/removals.
     */
    void updateTopology();

    // -----------------------------------------------------------------------
    // Lifecycle — registry path
    // -----------------------------------------------------------------------

    /**
     * @brief Initialise the coordinator from a cluster config alone.
     *
     * Seeds the node list from `ClusterConfig::nodes` (COORDINATOR mode) or
     * registers the local node (STANDALONE / WORKER mode).
     *
     * @return true on success.
     */
    bool initialize(const ClusterConfig& config);

    /**
     * @brief Register or update a cluster node (registry path).
     *
     * If a node with `node.id` already exists its information is updated;
     * otherwise a new entry is created.
     */
    void registerNode(const NodeInfo& node);

    /**
     * @brief Remove a node from the cluster registry (registry path).
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
     * @brief Scan the node list and mark nodes whose last heartbeat exceeds
     *        the configured timeout as OFFLINE.
     *
     * Should be called periodically by a background health-check thread.
     */
    void expireStaleNodes();

    // -----------------------------------------------------------------------
    // Placement & scheduling — topology-aware path
    // -----------------------------------------------------------------------

    /**
     * @brief Select the best local GPU for a new work item.
     *
     * Uses NVLink bandwidth when multiple GPUs are present and
     * `config.enable_nvlink` is true; otherwise falls back to the first
     * healthy device.
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
    // Placement & scheduling — registry path
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
    // Diagnostics & queries
    // -----------------------------------------------------------------------

    /** @brief Return a read-only reference to the current topology snapshot. */
    const GPUClusterTopology& topology() const noexcept { return topology_; }

    /** @brief Return the active cluster configuration. */
    const ClusterConfig& clusterConfig() const noexcept { return config_; }

    /** @brief Return a cluster-wide health summary. */
    ClusterHealth clusterHealth() const;

    std::vector<NodeInfo> getClusterNodes() const;
    std::vector<NodeInfo> getOnlineNodes()  const;

    size_t totalNodes()      const;
    size_t onlineNodeCount() const;

    bool isCoordinator() const noexcept { return is_coordinator_; }
    bool isInitialized()  const noexcept { return initialized_; }

    const ClusterConfig& config() const noexcept { return config_; }

    GPUClusterCoordinator() = default;

private:
    mutable std::mutex      mutex_;
    bool                    initialized_    = false;
    bool                    is_coordinator_ = false;
    ClusterConfig           config_;
    std::vector<DeviceInfo> local_devices_;
    GPUClusterTopology      topology_;
    std::vector<NodeInfo>   nodes_;

    // Internal helper — called under mutex_.
    NodeInfo* findNode(const std::string& id);
};

} // namespace gpu
} // namespace themis
