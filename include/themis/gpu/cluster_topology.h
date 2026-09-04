/**
 * @file cluster_topology.h
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
#include <unordered_map>
#include "themis/gpu/device_discovery.h"

namespace themis {
namespace gpu {

/**
 * @brief Interconnect type between two GPU devices or cluster nodes.
 */
enum class InterconnectType {
    NVLINK,       ///< NVIDIA NVLink (intra-node, high bandwidth)
    XGMI,         ///< AMD Infinity Fabric / XGMI (intra-node)
    PCIE_P2P,     ///< PCIe peer-to-peer (intra-node)
    INFINIBAND,   ///< InfiniBand (inter-node, RDMA)
    ETHERNET,     ///< Standard Ethernet (inter-node, fallback)
    CPU,          ///< Transfer via host CPU memory (fallback)
    UNKNOWN,      ///< Interconnect type not determined
};

/**
 * @brief Returns a human-readable name for an InterconnectType.
 */
const char* interconnectTypeName(InterconnectType t) noexcept;

/**
 * @brief Directed link between two GPU devices (intra-node) or two nodes.
 *
 * For intra-node links src_device_index and dst_device_index are driver
 * device ordinals.  For inter-node links src_node_id / dst_node_id identify
 * the participating cluster nodes; device indices are set to -1.
 */
struct TopologyLink {
    InterconnectType type             = InterconnectType::UNKNOWN;
    float            bandwidth_gbps   = 0.0f;  ///< Peak measured or estimated bandwidth
    float            latency_us       = 0.0f;  ///< Round-trip latency in microseconds

    // Intra-node fields
    int src_device_index = -1;
    int dst_device_index = -1;

    // Inter-node fields
    std::string src_node_id = {};
    std::string dst_node_id;

    bool is_inter_node() const noexcept { return !src_node_id.empty(); }
};

/**
 * @brief Per-cluster-node descriptor (one entry per participating machine).
 */
struct ClusterNode {
    std::string              node_id;         ///< Unique identifier (hostname or ordinal)
    int                      rank         = 0;///< MPI/NCCL-style global rank
    std::vector<int>         device_indices;  ///< Local GPU device ordinals on this node
    std::string              ib_device;       ///< InfiniBand device name (e.g. "mlx5_0")
    std::string              ib_port;         ///< InfiniBand port (e.g. "1")
};

/**
 * @brief GPU cluster topology snapshot.
 *
 * Captures the full NVLink / InfiniBand / PCIe topology of a multi-node GPU
 * cluster so that the scheduler can make bandwidth-aware placement decisions.
 *
 * The topology is represented as:
 *  - A per-device NVLink bandwidth matrix (intra-node).
 *  - A per-node InfiniBand bandwidth matrix (inter-node).
 *  - A flat list of all directed `TopologyLink` records for detailed queries.
 *
 * Typical usage
 * -------------
 * ```cpp
 * auto devices = DeviceDiscovery::Enumerate();
 * auto topology = GPUClusterTopology::detect(devices);
 *
 * if (topology.has_nvlink) {
 *     // pick GPUs with highest NVLink bandwidth for data-parallel jobs
 *     auto pair = topology.bestNVLinkPair();
 * }
 * ```
 *
 * Thread safety: `GPUClusterTopology` is an immutable value object after
 * construction.  All read methods are const and thread-safe.
 */
class GPUClusterTopology {
public:
    // -----------------------------------------------------------------------
    // Construction / detection
    // -----------------------------------------------------------------------

    /**
     * @brief Detect intra-node GPU topology from an already-enumerated device
     *        list (single-node or local-node view of a multi-node cluster).
     *
     * Populates the NVLink bandwidth matrix and `has_nvlink` flag.  InfiniBand
     * inter-node links are populated via `addNode()` / `addLink()` by the
     * cluster coordinator once remote node information is available.
     *
     * @param devices  Output of `DeviceDiscovery::Enumerate()`.
     * @return Topology object with intra-node links filled in.
     */
    static GPUClusterTopology detect(const std::vector<DeviceInfo>& devices);

    /**
     * @brief Default-construct an empty topology (zero devices, zero links).
     */
    GPUClusterTopology() = default;

    // -----------------------------------------------------------------------
    // Cluster node management
    // -----------------------------------------------------------------------

    /**
     * @brief Register a cluster node.  Duplicate node_ids are ignored.
     */
    void addNode(const ClusterNode& node);

    /**
     * @brief Remove a cluster node by id (also removes its links).
     */
    void removeNode(const std::string& node_id);

    /**
     * @brief Add a directed inter-node InfiniBand link.
     *
     * Both src and dst node_ids must have been added via addNode() first;
     * if either is unknown the call is silently ignored.
     */
    void addLink(const TopologyLink& link);

    // -----------------------------------------------------------------------
    // Topology queries
    // -----------------------------------------------------------------------

    /** @brief True when at least one NVLink connection is present. */
    bool has_nvlink     = false;

    /** @brief True when at least one PCIe peer-to-peer path exists. */
    bool has_pcie_p2p   = false;

    /** @brief True when at least one InfiniBand path exists. */
    bool has_infiniband = false;

    /** @brief Number of GPUs included in the local topology snapshot. */
    int  num_gpus       = 0;

    /**
     * @brief Symmetric bandwidth matrix: bandwidth_matrix[i][j] is the
     *        estimated one-way GB/s from device i to device j.
     *
     * Diagonal entries are 0.  Uses estimated values when real NVML / ROCM-SMI
     * data are unavailable (NVLINK: 300 GB/s, PCIe P2P: 16 GB/s, CPU: 8 GB/s).
     */
    std::vector<std::vector<float>> bandwidth_matrix;

    /** @brief All topology links (intra-node and inter-node). */
    std::vector<TopologyLink> links;

    /** @brief Registered cluster nodes. */
    std::vector<ClusterNode> nodes;

    /**
     * @brief Return the pair of device indices with the highest NVLink
     *        bandwidth.  Returns {-1, -1} when no NVLink is present.
     */
    std::pair<int, int> bestNVLinkPair() const;

    /**
     * @brief Return the pair of nodes with the highest InfiniBand bandwidth.
     *
     * Returns {"", ""} when no inter-node InfiniBand links are registered.
     */
    std::pair<std::string, std::string> bestInfiniBandPair() const;

    /**
     * @brief Return the estimated bandwidth in GB/s between two local GPU
     *        device indices.  Returns 0 when either index is out of range.
     */
    float bandwidthBetween(int device_a, int device_b) const;

    /**
     * @brief Return the interconnect type used on the highest-bandwidth path
     *        between two local device indices.
     */
    InterconnectType preferredInterconnect(int device_a, int device_b) const;

    /**
     * @brief Return all registered node ids.
     */
    std::vector<std::string> nodeIds() const;

    /**
     * @brief Return the node descriptor for a given id (empty node_id if not
     *        found).
     */
    ClusterNode getNode(const std::string& node_id) const;

private:
    std::unordered_map<std::string, ClusterNode> node_map_;
};

} // namespace gpu
} // namespace themis

