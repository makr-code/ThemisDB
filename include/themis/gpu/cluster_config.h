/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cluster_config.h                                   ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 In Progress (Phase 2 - Multi-node cluster coordination)  ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Configuration for the multi-node GPU cluster coordinator.
 *
 * This is the *optional* cluster configuration block embedded in `GPUConfig`
 * (see ROADMAP.md Breaking Changes).  The `enabled` flag defaults to `false`
 * which preserves single-node behaviour for existing deployments.
 *
 * Callers enable multi-node mode by setting `enabled = true`, providing a
 * unique `node_id`, and configuring `peer_nodes` / `ib_device`.
 */
struct ClusterConfig {
    // -----------------------------------------------------------------------
    // Enable flag (single-node default preserves backward compatibility)
    // -----------------------------------------------------------------------

    /**
     * @brief Set to true to activate multi-node cluster coordination.
     * When false the coordinator runs in single-node pass-through mode.
     */
    bool enabled = false;

    // -----------------------------------------------------------------------
    // Node identity
    // -----------------------------------------------------------------------

    /**
     * @brief Unique identifier for this node (e.g. hostname or ordinal).
     * Empty is allowed when enabled == false.
     */
    std::string node_id;

    /**
     * @brief Global rank in the cluster (0-based).  0 = primary/coordinator.
     */
    int rank        = 0;

    /**
     * @brief Total number of participating nodes.  1 = single-node mode.
     */
    int world_size  = 1;

    // -----------------------------------------------------------------------
    // Peer nodes
    // -----------------------------------------------------------------------

    /**
     * @brief Host:port or node-id strings for all peer nodes.
     *
     * Used to register remote cluster nodes in the topology and to set up
     * InfiniBand / RDMA routes when InfiniBand is available.
     */
    std::vector<std::string> peer_nodes;

    // -----------------------------------------------------------------------
    // InfiniBand / RDMA
    // -----------------------------------------------------------------------

    /** @brief InfiniBand device name on this node (e.g. "mlx5_0"). */
    std::string ib_device;

    /** @brief InfiniBand port (e.g. "1"). */
    std::string ib_port;

    // -----------------------------------------------------------------------
    // Feature toggles
    // -----------------------------------------------------------------------

    /** @brief Enable NVLink-aware intra-node scheduling. */
    bool enable_nvlink       = true;

    /** @brief Enable InfiniBand for inter-node data transfers. */
    bool enable_infiniband   = true;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /** @brief True when this config represents a real multi-node deployment. */
    bool is_multi_node() const noexcept { return enabled && world_size > 1; }

    /** @brief True when InfiniBand is configured and enabled. */
    bool has_infiniband() const noexcept {
        return enabled && enable_infiniband && !ib_device.empty();
    }
};

} // namespace gpu
} // namespace themis
