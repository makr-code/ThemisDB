/**
 * @file cluster_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Cluster Coordinator — multi-node GPU cluster management.
 *
 * Manages the node registry, health tracking via heartbeats, and
 * least-loaded node selection for multi-node GPU cluster coordination.
 */

#include "themis/gpu/cluster_coordinator.h"

#include <algorithm>
#include <chrono>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr float kDefaultInfiniBandBwGbps    = 25.0f; // HDR IB 200 Gb/s ÷ 8 = 25 GB/s
static constexpr float kDefaultInfiniBandLatencyUs = 2.0f;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void GPUClusterCoordinator::initialize(const ClusterConfig &config, const std::vector<DeviceInfo> &devices) {
    std::lock_guard<std::mutex> lock(mutex_);

    config_        = config;
    local_devices_ = devices;

    // Build initial intra-node topology.
    topology_ = GPUClusterTopology::detect(devices);

    // Register the local node.
    if (!config_.node_id.empty()) {
        ClusterNode local;
        local.node_id   = config_.node_id;
        local.rank      = config_.rank;
        local.ib_device = config_.ib_device;
        local.ib_port   = config_.ib_port;
        for (const auto &d : devices) {
            local.device_indices.push_back(d.index);
        }
        topology_.addNode(local);
    }

    initialized_ = true;
}

// ---------------------------------------------------------------------------
// Node management
// ---------------------------------------------------------------------------

void GPUClusterCoordinator::registerNode(const ClusterNode &node, float ib_bw_gbps) {
    std::lock_guard<std::mutex> lock(mutex_);

    topology_.addNode(node);

    // If InfiniBand is enabled and the local node has an IB device, add a
    // bidirectional link between the local node and the newly registered peer.
    if (config_.has_infiniband() && !config_.node_id.empty() && !node.node_id.empty()
        && node.node_id != config_.node_id) {
        const float bw = (ib_bw_gbps > 0.0f) ? ib_bw_gbps : kDefaultInfiniBandBwGbps;

        // src → dst
        TopologyLink fwd;
        fwd.type           = InterconnectType::INFINIBAND;
        fwd.bandwidth_gbps = bw;
        fwd.latency_us     = kDefaultInfiniBandLatencyUs;
        fwd.src_node_id    = config_.node_id;
        fwd.dst_node_id    = node.node_id;
        topology_.addLink(fwd);

        // dst → src (symmetric fabric)
        TopologyLink rev = fwd;
        rev.src_node_id  = node.node_id;
        rev.dst_node_id  = config_.node_id;
        topology_.addLink(rev);
    }
}

void GPUClusterCoordinator::removeNode(const std::string &node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    topology_.removeNode(node_id);
}

void GPUClusterCoordinator::updateTopology() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Re-detect intra-node topology and preserve existing inter-node links.
    GPUClusterTopology fresh = GPUClusterTopology::detect(local_devices_);

    // Re-add all previously registered nodes.
    for (const auto &n : topology_.nodes) {
        fresh.addNode(n);
    }

    // Re-add inter-node links.
    for (const auto &lnk : topology_.links) {
        if (lnk.is_inter_node()) {
            fresh.addLink(lnk);
        }
    }

    topology_ = std::move(fresh);
}

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

GPUClusterCoordinator::Placement GPUClusterCoordinator::selectDevice([[maybe_unused]] uint64_t required_vram_bytes) const {
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
        int best_dev   = -1;
        float best_sum = -1.0f;

        for (int i = 0; i < topology_.num_gpus; ++i) {
            if (i >= static_cast<int>(local_devices_.size())) {
                break;
            }
            const auto &dev = local_devices_[static_cast<size_t>(i)];
            if (!dev.is_healthy) {
                continue;
            }
            if (required_vram_bytes > 0 && dev.free_vram_bytes < required_vram_bytes) {
                continue;
            }

            float bw_sum = 0.0f;
            for (int j = 0; j < topology_.num_gpus; ++j) {
                if (j == i) {
                    continue;
                }
                bw_sum += topology_.bandwidthBetween(i, j);
            }
            if (bw_sum > best_sum) {
                best_sum = bw_sum;
                best_dev = i;
            }
        }

        // Only return an NVLink placement when at least one eligible device was found.
        if (best_dev >= 0) {
            result.device_index   = best_dev;
            result.route          = InterconnectType::NVLINK;
            result.bandwidth_gbps = best_sum;
            result.reason         = "NVLink-aware placement";
            return result;
        }
        // Fall through to the regular fallback when no device qualified.
    }

    // Fallback: pick the first healthy device that meets VRAM requirements.
    for (size_t i = 0; i < local_devices_.size(); ++i) {
        const auto &dev = local_devices_[static_cast<size_t>(i)];
        if (!dev.is_healthy) {
            continue;
        }
        if (required_vram_bytes > 0 && dev.free_vram_bytes < required_vram_bytes) {
            continue;
        }

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

GPUClusterCoordinator::Placement GPUClusterCoordinator::selectNodeForTransfer(const std::string &src_node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string src = src_node_id.empty() ? config_.node_id : src_node_id;

    Placement result;
    result.node_id = src; // default: stay local

    if (!topology_.has_infiniband) {
        result.route  = InterconnectType::CPU;
        result.reason = "no InfiniBand links registered";
        return result;
    }

    // Find the highest-bandwidth InfiniBand link from src.
    float best_bw = -1.0f;
    for (const auto &lnk : topology_.links) {
        if (lnk.type == InterconnectType::INFINIBAND && lnk.is_inter_node() && lnk.src_node_id == src
            && lnk.bandwidth_gbps > best_bw) {
            best_bw               = lnk.bandwidth_gbps;
            result.node_id        = lnk.dst_node_id;
            result.route          = InterconnectType::INFINIBAND;
            result.bandwidth_gbps = lnk.bandwidth_gbps;
            result.reason         = "highest-bandwidth InfiniBand link";
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

GPUClusterCoordinator::ClusterHealth GPUClusterCoordinator::clusterHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);

    ClusterHealth h;
    h.total_nodes          = static_cast<int>(topology_.nodes.size());
    h.healthy_nodes        = h.total_nodes; // all registered nodes assumed healthy
    h.total_gpus           = static_cast<int>(local_devices_.size());
    h.nvlink_available     = topology_.has_nvlink;
    h.infiniband_available = topology_.has_infiniband;
    return h;
}

// ============================================================================
// Internal helper
// ============================================================================

GPUClusterCoordinator::NodeInfo *GPUClusterCoordinator::findNode(const std::string &id) {
    for (auto &n : nodes_) {
        if (n.id == id) {
            return &n;
        }
    }
    return nullptr;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool GPUClusterCoordinator::initialize(const ClusterConfig &cfg) {
    std::lock_guard<std::mutex> lock(mutex_);

    config_         = cfg;
    is_coordinator_ = (cfg.mode == ClusterConfig::Mode::COORDINATOR || cfg.mode == ClusterConfig::Mode::STANDALONE);
    nodes_.clear();

    if (cfg.mode == ClusterConfig::Mode::STANDALONE || !cfg.enabled) {
        // Expose a single synthetic local node so callers always get a result.
        NodeInfo local;
        local.id             = cfg.node_id.empty() ? "local" : cfg.node_id;
        local.address        = "localhost";
        local.role           = NodeRole::COORDINATOR;
        local.status         = NodeStatus::ONLINE;
        local.last_heartbeat = std::chrono::steady_clock::now();
        nodes_.push_back(std::move(local));

    } else if (cfg.mode == ClusterConfig::Mode::COORDINATOR) {
        // Seed with statically-configured cluster members.
        const auto now = std::chrono::steady_clock::now();
        for (const auto &entry : cfg.nodes) {
            NodeInfo n;
            n.id             = entry.id;
            n.address        = entry.address;
            n.role           = NodeRole::WORKER;
            n.status         = NodeStatus::OFFLINE; // until first heartbeat
            n.gpu_count      = entry.gpu_count;
            n.last_heartbeat = now;
            nodes_.push_back(std::move(n));
        }

    } else {
        // WORKER mode: register this node itself; the coordinator endpoint is
        // stored in config_ for the transport layer to use.
        NodeInfo self;
        self.id             = cfg.node_id.empty() ? "worker" : cfg.node_id;
        self.address        = cfg.coordinator_address;
        self.role           = NodeRole::WORKER;
        self.status         = NodeStatus::OFFLINE; // until first heartbeat
        self.last_heartbeat = std::chrono::steady_clock::now();
        nodes_.push_back(std::move(self));
    }

    initialized_ = true;
    return true;
}

// ============================================================================
// Node management
// ============================================================================

void GPUClusterCoordinator::registerNode(const NodeInfo &node) {
    std::lock_guard<std::mutex> lock(mutex_);
    NodeInfo *existing = findNode(node.id);
    if (existing) {
        *existing = node;
    } else {
        nodes_.push_back(node);
    }
}

bool GPUClusterCoordinator::deregisterNode(const std::string &node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(nodes_.begin(), nodes_.end(), [&node_id](const NodeInfo &n) { return n.id == node_id; });
    if (it == nodes_.end()) {
        return false;
    }
    nodes_.erase(it, nodes_.end());
    return true;
}

void GPUClusterCoordinator::updateHeartbeat(const std::string &node_id, uint64_t free_vram_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    NodeInfo *n = findNode(node_id);
    if (!n) {
        return;
    }
    n->status          = NodeStatus::ONLINE;
    n->free_vram_bytes = free_vram_bytes;
    n->last_heartbeat  = std::chrono::steady_clock::now();
}

void GPUClusterCoordinator::markNodeOffline(const std::string &node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    NodeInfo *n = findNode(node_id);
    if (n) {
        n->status = NodeStatus::OFFLINE;
    }
}

void GPUClusterCoordinator::expireStaleNodes() {
    const auto now     = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(config_.node_timeout_ms);

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &n : nodes_) {
        if (n.status == NodeStatus::ONLINE && (now - n.last_heartbeat) > timeout) {
            n.status = NodeStatus::OFFLINE;
        }
    }
}

// ============================================================================
// Work placement
// ============================================================================

const GPUClusterCoordinator::NodeInfo *GPUClusterCoordinator::selectNode([[maybe_unused]] uint64_t required_vram_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    const NodeInfo *best = nullptr;
    for (const auto &n : nodes_) {
        if (n.status != NodeStatus::ONLINE) {
            continue;
        }
        if (required_vram_bytes > 0 && n.free_vram_bytes < required_vram_bytes) {
            continue;
        }
        if (best == nullptr || n.free_vram_bytes > best->free_vram_bytes) {
            best = &n;
        }
    }
    return best;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<GPUClusterCoordinator::NodeInfo> GPUClusterCoordinator::getClusterNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_;
}

std::vector<GPUClusterCoordinator::NodeInfo> GPUClusterCoordinator::getOnlineNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeInfo> result = {};

    for (const auto &n : nodes_) {
        if (n.status == NodeStatus::ONLINE) {
            result.push_back(n);
        }
    }
    return result;
}

size_t GPUClusterCoordinator::totalNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(nodes_.size());
}

size_t GPUClusterCoordinator::onlineNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t n = 0;
    for (const auto &node : nodes_) {
        if (node.status == NodeStatus::ONLINE) {
            ++n;
        }
    }
    return n;
}

} // namespace gpu
} // namespace themis
