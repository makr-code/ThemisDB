/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cluster_coordinator.cpp                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 In Progress (Phase 2 - Multi-node cluster coordination)  ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Cluster Coordinator — multi-node GPU cluster coordination with
 * NVLink / InfiniBand topology awareness.
 *
 * The coordinator integrates with GPUClusterTopology to provide placement
 * decisions that minimise data movement latency and maximise bandwidth
 * utilisation across the cluster.
 */

#include "themis/gpu/cluster_coordinator.h"

#include <algorithm>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr float kDefaultInfiniBandBwGbps = 100.0f; // HDR IB ~200 Gb/s ÷ 8 ≈ 25 GB/s; use 25 GB/s for a 200 Gb/s fabric
static constexpr float kDefaultInfiniBandLatencyUs = 2.0f;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void GPUClusterCoordinator::initialize(
    const ClusterConfig&           config,
    const std::vector<DeviceInfo>& devices)
{
    std::lock_guard<std::mutex> lock(mutex_);

    config_        = config;
    local_devices_ = devices;

    // Build initial intra-node topology.
    topology_ = GPUClusterTopology::detect(devices);

    // Register the local node.
    if (!config_.node_id.empty()) {
        ClusterNode local;
        local.node_id  = config_.node_id;
        local.rank     = config_.rank;
        local.ib_device = config_.ib_device;
        local.ib_port   = config_.ib_port;
        for (const auto& d : devices) {
            local.device_indices.push_back(d.index);
        }
        topology_.addNode(local);
    }

    initialized_ = true;
}

// ---------------------------------------------------------------------------
// Node management
// ---------------------------------------------------------------------------

void GPUClusterCoordinator::registerNode(const ClusterNode& node,
                                          float ib_bw_gbps)
{
    std::lock_guard<std::mutex> lock(mutex_);

    topology_.addNode(node);

    // If InfiniBand is enabled and the local node has an IB device, add a
    // bidirectional link between the local node and the newly registered peer.
    if (config_.has_infiniband() && !config_.node_id.empty() &&
        !node.node_id.empty() && node.node_id != config_.node_id)
    {
        const float bw = (ib_bw_gbps > 0.0f) ? ib_bw_gbps
                                              : kDefaultInfiniBandBwGbps;

        // src → dst
        TopologyLink fwd;
        fwd.type          = InterconnectType::INFINIBAND;
        fwd.bandwidth_gbps = bw;
        fwd.latency_us    = kDefaultInfiniBandLatencyUs;
        fwd.src_node_id   = config_.node_id;
        fwd.dst_node_id   = node.node_id;
        topology_.addLink(fwd);

        // dst → src (symmetric fabric)
        TopologyLink rev = fwd;
        rev.src_node_id  = node.node_id;
        rev.dst_node_id  = config_.node_id;
        topology_.addLink(rev);
    }
}

void GPUClusterCoordinator::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    topology_.removeNode(node_id);
}

void GPUClusterCoordinator::updateTopology() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Re-detect intra-node topology and preserve existing inter-node links.
    GPUClusterTopology fresh = GPUClusterTopology::detect(local_devices_);

    // Re-add all previously registered nodes.
    for (const auto& n : topology_.nodes) {
        fresh.addNode(n);
    }

    // Re-add inter-node links.
    for (const auto& lnk : topology_.links) {
        if (lnk.is_inter_node()) {
            fresh.addLink(lnk);
        }
    }

    topology_ = std::move(fresh);
}

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

GPUClusterCoordinator::Placement
GPUClusterCoordinator::selectDevice(uint64_t required_vram_bytes) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    Placement result;
    result.node_id = config_.node_id;

    if (local_devices_.empty()) {
        result.device_index = -1;
        result.route        = InterconnectType::CPU;
        result.reason       = "no local GPU devices";
        return result;
    }

    // Prefer the NVLink-connected device with the highest total bandwidth
    // when NVLink-aware scheduling is enabled.
    if (config_.enable_nvlink && topology_.has_nvlink) {
        // Sum outgoing NVLink bandwidths per device to find the best-connected
        // device on the NVLink fabric.
        int   best_dev = 0;
        float best_sum = -1.0f;

        for (int i = 0; i < topology_.num_gpus; ++i) {
            if (i >= static_cast<int>(local_devices_.size())) break;
            const auto& dev = local_devices_[static_cast<size_t>(i)];
            if (!dev.is_healthy) continue;
            if (required_vram_bytes > 0 &&
                dev.free_vram_bytes < required_vram_bytes) continue;

            float bw_sum = 0.0f;
            for (int j = 0; j < topology_.num_gpus; ++j) {
                if (j == i) continue;
                bw_sum += topology_.bandwidthBetween(i, j);
            }
            if (bw_sum > best_sum) {
                best_sum = bw_sum;
                best_dev = i;
            }
        }

        result.device_index    = best_dev;
        result.route           = InterconnectType::NVLINK;
        result.bandwidth_gbps  = best_sum;
        result.reason          = "NVLink-aware placement";
        return result;
    }

    // Fallback: pick the first healthy device that meets VRAM requirements.
    for (int i = 0; i < static_cast<int>(local_devices_.size()); ++i) {
        const auto& dev = local_devices_[static_cast<size_t>(i)];
        if (!dev.is_healthy) continue;
        if (required_vram_bytes > 0 &&
            dev.free_vram_bytes < required_vram_bytes) continue;

        result.device_index   = dev.index;
        result.route          = InterconnectType::PCIE_P2P;
        result.bandwidth_gbps = topology_.bandwidthBetween(i, 0);
        result.reason         = "first healthy device";
        return result;
    }

    // No eligible device found.
    result.device_index = -1;
    result.route        = InterconnectType::CPU;
    result.reason       = "no eligible GPU found; using CPU fallback";
    return result;
}

GPUClusterCoordinator::Placement
GPUClusterCoordinator::selectNodeForTransfer(
    const std::string& src_node_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string src = src_node_id.empty() ? config_.node_id
                                                 : src_node_id;

    Placement result;
    result.node_id = src;  // default: stay local

    if (!topology_.has_infiniband) {
        result.route  = InterconnectType::CPU;
        result.reason = "no InfiniBand links registered";
        return result;
    }

    // Find the highest-bandwidth InfiniBand link from src.
    float best_bw = -1.0f;
    for (const auto& lnk : topology_.links) {
        if (lnk.type == InterconnectType::INFINIBAND &&
            lnk.is_inter_node() &&
            lnk.src_node_id == src &&
            lnk.bandwidth_gbps > best_bw) {
            best_bw           = lnk.bandwidth_gbps;
            result.node_id    = lnk.dst_node_id;
            result.route      = InterconnectType::INFINIBAND;
            result.bandwidth_gbps = lnk.bandwidth_gbps;
            result.reason     = "highest-bandwidth InfiniBand link";
        }
    }

    if (best_bw < 0.0f) {
        result.node_id = src;
        result.route   = InterconnectType::ETHERNET;
        result.reason  = "no InfiniBand link from this node; using Ethernet";
    }

    return result;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

GPUClusterCoordinator::ClusterHealth
GPUClusterCoordinator::clusterHealth() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    ClusterHealth h;
    h.total_nodes        = static_cast<int>(topology_.nodes.size());
    h.healthy_nodes      = h.total_nodes;  // all registered nodes assumed healthy
    h.total_gpus         = static_cast<int>(local_devices_.size());
    h.nvlink_available   = topology_.has_nvlink;
    h.infiniband_available = topology_.has_infiniband;
    return h;
}

} // namespace gpu
} // namespace themis
