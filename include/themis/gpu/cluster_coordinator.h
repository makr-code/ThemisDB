/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cluster_coordinator.h                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA                                         ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     210                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 In Progress (Phase 2 - Multi-node cluster coordination)  ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

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

private:
    GPUClusterCoordinator() = default;

    mutable std::mutex        mutex_;
    bool                      initialized_ = false;
    ClusterConfig             config_;
    std::vector<DeviceInfo>   local_devices_;
    GPUClusterTopology        topology_;
};

} // namespace gpu
} // namespace themis
