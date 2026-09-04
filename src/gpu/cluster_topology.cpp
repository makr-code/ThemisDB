/**
 * @file cluster_topology.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Cluster Topology — NVLink / InfiniBand topology detection.
 *
 * Real NVML / ROCM-SMI calls are gated behind THEMIS_ENABLE_CUDA /
 * THEMIS_ENABLE_HIP.  When neither is defined the class falls back to
 * estimated bandwidth values derived from well-known hardware specifications
 * so that the coordinator can still make topology-aware scheduling decisions
 * in pure-CPU CI environments.
 */

#include "themis/gpu/cluster_topology.h"

#include <algorithm>
#include <cstring>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Free helpers
// ---------------------------------------------------------------------------

const char *interconnectTypeName(InterconnectType t) noexcept {
    switch (t) {
        case InterconnectType::NVLINK:
            return "NVLink";
        case InterconnectType::XGMI:
            return "XGMI";
        case InterconnectType::PCIE_P2P:
            return "PCIe_P2P";
        case InterconnectType::INFINIBAND:
            return "InfiniBand";
        case InterconnectType::ETHERNET:
            return "Ethernet";
        case InterconnectType::CPU:
            return "CPU";
        default:
            return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// GPUClusterTopology::detect
// ---------------------------------------------------------------------------

GPUClusterTopology GPUClusterTopology::detect(const std::vector<DeviceInfo> &devices) {
    GPUClusterTopology topo;
    topo.num_gpus = static_cast<int>(devices.size());

    if (devices.empty()) {
        return topo;
    }

    // Initialise bandwidth matrix to 0.
    topo.bandwidth_matrix.assign(topo.num_gpus, std::vector<float>(topo.num_gpus, 0.0f));

#ifdef THEMIS_ENABLE_CUDA
    // -----------------------------------------------------------------------
    // CUDA path: query peer access capability via cudaDeviceCanAccessPeer and
    // estimate interconnect type based on available capability flags.
    // Real NVLink detection requires NVML; we fall back to PCIe P2P heuristics
    // when the NVML library is not linked.
    // -----------------------------------------------------------------------
    for (int i = 0; i < topo.num_gpus; ++i) {
        for (int j = 0; j < topo.num_gpus; ++j) {
            if (i == j)
                continue;

            int can_access  = 0;
            cudaError_t err = cudaDeviceCanAccessPeer(&can_access, devices[i].device_index, devices[j].device_index);

            if (err != cudaSuccess || !can_access) {
                // No direct path; route via CPU host memory.
                topo.bandwidth_matrix[i][j] = 8.0f;
                TopologyLink lnk;
                lnk.type             = InterconnectType::CPU;
                lnk.bandwidth_gbps   = 8.0f;
                lnk.latency_us       = 5.0f;
                lnk.src_device_index = i;
                lnk.dst_device_index = j;
                topo.links.push_back(lnk);
                continue;
            }

            // Peer access available — try to distinguish NVLink from PCIe.
            // Without NVML we rely on the heuristic that high-end data-centre
            // GPUs (A100, H100, V100) typically expose NVLink when peer access
            // is possible.  The caller can override the bandwidth values after
            // construction if real NVML data are available.
            TopologyLink lnk;
            lnk.src_device_index = i;
            lnk.dst_device_index = j;

            // Treat compute capability >= 7.0 (Volta+) as potentially NVLink
            // capable when peer access is available.
            if (devices[i].compute_major >= 7) {
                lnk.type           = InterconnectType::NVLINK;
                lnk.bandwidth_gbps = 300.0f; // NVLink 3.0 / 4.0 estimate
                lnk.latency_us     = 1.0f;
                topo.has_nvlink    = true;
            } else {
                lnk.type           = InterconnectType::PCIE_P2P;
                lnk.bandwidth_gbps = 16.0f;
                lnk.latency_us     = 2.5f;
                topo.has_pcie_p2p  = true;
            }

            topo.bandwidth_matrix[i][j] = lnk.bandwidth_gbps;
            topo.links.push_back(lnk);
        }
    }
#else
    // -----------------------------------------------------------------------
    // CPU-fallback path: assume all devices communicate via CPU memory unless
    // the device name or backend hints at a known interconnect.
    // -----------------------------------------------------------------------
    for (int i = 0; i < topo.num_gpus; ++i) {
        for (int j = 0; j < topo.num_gpus; ++j) {
            if (i == j) {
                continue;
            }

            TopologyLink lnk;
            lnk.src_device_index = i;
            lnk.dst_device_index = j;

            // If multiple CUDA or ROCm devices are present and their indices
            // differ, assume at least PCIe P2P is available as a best-effort
            // estimate (real detection requires hardware).
            if ((devices[i].backend == "CUDA" && devices[j].backend == "CUDA")
                || (devices[i].backend == "ROCm" && devices[j].backend == "ROCm")) {
                lnk.type           = InterconnectType::PCIE_P2P;
                lnk.bandwidth_gbps = 16.0f;
                lnk.latency_us     = 2.5f;
                topo.has_pcie_p2p  = true;
            } else {
                lnk.type           = InterconnectType::CPU;
                lnk.bandwidth_gbps = 8.0f;
                lnk.latency_us     = 5.0f;
            }

            topo.bandwidth_matrix[i][j] = lnk.bandwidth_gbps;
            topo.links.push_back(lnk);
        }
    }
#endif

    return topo;
}

// ---------------------------------------------------------------------------
// Cluster node management
// ---------------------------------------------------------------------------

void GPUClusterTopology::addNode(const ClusterNode &node) {
    if (node.node_id.empty()) {
        return;
    }
    node_map_.emplace(node.node_id, node);

    // Keep nodes list in sync with the map.
    for (const auto &n : nodes) {
        if (n.node_id == node.node_id) {
            return;
        }
    }
    nodes.push_back(node);
}

void GPUClusterTopology::removeNode(const std::string &node_id) {
    node_map_.erase(node_id);

    nodes.erase(
        std::remove_if(nodes.begin(), nodes.end(), [&node_id](const ClusterNode &n) { return n.node_id == node_id; }),
        nodes.end());

    links.erase(std::remove_if(
                    links.begin(), links.end(),
                    [&node_id](const TopologyLink &l) { return l.src_node_id == node_id || l.dst_node_id == node_id; }),
                links.end());
}

void GPUClusterTopology::addLink(const TopologyLink &link) {
    if (link.is_inter_node()) {
        if (node_map_.find(link.src_node_id) == node_map_.end()) {
            return;
        }
        if (node_map_.find(link.dst_node_id) == node_map_.end()) {
            return;
        }
        if (link.type == InterconnectType::INFINIBAND) {
            has_infiniband = true;
        }
    } else {
        // Intra-node link: keep bandwidth_matrix in sync so that
        // bandwidthBetween() reflects any manually-added links.
        const int si = link.src_device_index;
        const int di = link.dst_device_index;
        if (si >= 0 && di >= 0 && si < num_gpus && di < num_gpus) {
            if (link.bandwidth_gbps > bandwidth_matrix[static_cast<size_t>(si)][static_cast<size_t>(di)]) {
                bandwidth_matrix[static_cast<size_t>(si)][static_cast<size_t>(di)] = link.bandwidth_gbps;
            }
        }
    }
    links.push_back(link);
}

// ---------------------------------------------------------------------------
// Topology queries
// ---------------------------------------------------------------------------

std::pair<int, int> GPUClusterTopology::bestNVLinkPair() const {
    float best_bw = -1.0f;
    std::pair<int, int> best{-1, -1};

    for (const auto &lnk : links) {
        if (lnk.type == InterconnectType::NVLINK && !lnk.is_inter_node() && lnk.bandwidth_gbps > best_bw) {
            best_bw = lnk.bandwidth_gbps;
            best    = {lnk.src_device_index, lnk.dst_device_index};
        }
    }
    return best;
}

std::pair<std::string, std::string> GPUClusterTopology::bestInfiniBandPair() const {
    float best_bw = -1.0f;
    std::pair<std::string, std::string> best{"", ""};

    for (const auto &lnk : links) {
        if (lnk.type == InterconnectType::INFINIBAND && lnk.is_inter_node() && lnk.bandwidth_gbps > best_bw) {
            best_bw = lnk.bandwidth_gbps;
            best    = {lnk.src_node_id, lnk.dst_node_id};
        }
    }
    return best;
}

float GPUClusterTopology::bandwidthBetween(int device_a, int device_b) const {
    if (device_a < 0 || device_b < 0) {
        return 0.0f;
    }
    if (device_a >= num_gpus || device_b >= num_gpus) {
        return 0.0f;
    }
    if (device_a == device_b) {
        return 0.0f;
    }
    return bandwidth_matrix[static_cast<size_t>(device_a)][static_cast<size_t>(device_b)];
}

InterconnectType GPUClusterTopology::preferredInterconnect(int device_a, int device_b) const {
    if (device_a < 0 || device_b < 0 || device_a >= num_gpus || device_b >= num_gpus || device_a == device_b) {
        return InterconnectType::UNKNOWN;
    }

    // Find the highest-bandwidth intra-node link from a to b.
    float best_bw              = -1.0f;
    InterconnectType best_type = InterconnectType::CPU;

    for (const auto &lnk : links) {
        if (!lnk.is_inter_node() && lnk.src_device_index == device_a && lnk.dst_device_index == device_b
            && lnk.bandwidth_gbps > best_bw) {
            best_bw   = lnk.bandwidth_gbps;
            best_type = lnk.type;
        }
    }
    return best_type;
}

std::vector<std::string> GPUClusterTopology::nodeIds() const {
    std::vector<std::string> ids = {};

    ids.reserve(nodes.size());
    for (const auto &n : nodes) {
        ids.push_back(n.node_id);
    }
    return ids;
}

ClusterNode GPUClusterTopology::getNode(const std::string &node_id) const {
    auto it = node_map_.find(node_id);
    if (it == node_map_.end()) {
        return ClusterNode{};
    }
    return it->second;
}

} // namespace gpu
} // namespace themis
