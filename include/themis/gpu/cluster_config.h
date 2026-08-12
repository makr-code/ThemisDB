/**
 * @file cluster_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
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
 * unique `node_id`, and selecting a `mode`.
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
    // Node role
    // -----------------------------------------------------------------------

    /**
     * @brief Operating mode for this node in the cluster.
     */
    enum class Mode {
        STANDALONE,   ///< Single-node operation (default, backward-compatible)
        COORDINATOR,  ///< This node is the authoritative cluster coordinator
        WORKER,       ///< This node is a worker managed by a coordinator
    };
    Mode mode = Mode::STANDALONE;

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
    // Coordinator connectivity
    // -----------------------------------------------------------------------

    /**
     * @brief Address of the coordinator node ("host:port").
     * Required when mode == WORKER.
     */
    std::string coordinator_address;

    // -----------------------------------------------------------------------
    // Heartbeat / health-check
    // -----------------------------------------------------------------------

    /** @brief Interval between heartbeat ticks, in milliseconds. */
    uint32_t heartbeat_interval_ms = 5000;

    /** @brief Time after which a node with no heartbeat is considered offline, in ms. */
    uint32_t node_timeout_ms = 15000;

    // -----------------------------------------------------------------------
    // Static cluster membership (seeded by the coordinator)
    // -----------------------------------------------------------------------

    /**
     * @brief Per-node entry used to seed the coordinator's node registry at
     *        startup.  Only relevant when mode == COORDINATOR.
     */
    struct NodeEntry {
        std::string id;
        std::string address;   ///< "host:port"
        int         gpu_count = 0;
    };

    /** @brief Static list of cluster members (coordinator mode). */
    std::vector<NodeEntry> nodes;

    // -----------------------------------------------------------------------
    // Peer nodes (topology-aware path)
    // -----------------------------------------------------------------------

    /**
     * @brief Host:port or node-id strings for all peer nodes.
     * Used to register remote cluster nodes in the topology.
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
