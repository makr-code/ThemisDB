/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cluster_coordinator.cpp                            ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     190                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

// ============================================================================
// Internal helper
// ============================================================================

GPUClusterCoordinator::NodeInfo*
GPUClusterCoordinator::findNode(const std::string& id)
{
    for (auto& n : nodes_) {
        if (n.id == id) return &n;
    }
    return nullptr;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool GPUClusterCoordinator::initialize(const ClusterConfig& cfg)
{
    std::lock_guard<std::mutex> lock(mutex_);

    config_         = cfg;
    is_coordinator_ = (cfg.mode == ClusterConfig::Mode::COORDINATOR ||
                       cfg.mode == ClusterConfig::Mode::STANDALONE);
    nodes_.clear();

    if (cfg.mode == ClusterConfig::Mode::STANDALONE || !cfg.enabled) {
        // Expose a single synthetic local node so callers always get a result.
        NodeInfo local;
        local.id               = cfg.node_id.empty() ? "local" : cfg.node_id;
        local.address          = "localhost";
        local.role             = NodeRole::COORDINATOR;
        local.status           = NodeStatus::ONLINE;
        local.last_heartbeat   = std::chrono::steady_clock::now();
        nodes_.push_back(std::move(local));

    } else if (cfg.mode == ClusterConfig::Mode::COORDINATOR) {
        // Seed with statically-configured cluster members.
        const auto now = std::chrono::steady_clock::now();
        for (const auto& entry : cfg.nodes) {
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

void GPUClusterCoordinator::registerNode(const NodeInfo& node)
{
    std::lock_guard<std::mutex> lock(mutex_);
    NodeInfo* existing = findNode(node.id);
    if (existing) {
        *existing = node;
    } else {
        nodes_.push_back(node);
    }
}

bool GPUClusterCoordinator::deregisterNode(const std::string& node_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(nodes_.begin(), nodes_.end(),
        [&node_id](const NodeInfo& n) { return n.id == node_id; });
    if (it == nodes_.end()) return false;
    nodes_.erase(it, nodes_.end());
    return true;
}

void GPUClusterCoordinator::updateHeartbeat(const std::string& node_id,
                                             uint64_t           free_vram_bytes)
{
    std::lock_guard<std::mutex> lock(mutex_);
    NodeInfo* n = findNode(node_id);
    if (!n) return;
    n->status          = NodeStatus::ONLINE;
    n->free_vram_bytes = free_vram_bytes;
    n->last_heartbeat  = std::chrono::steady_clock::now();
}

void GPUClusterCoordinator::markNodeOffline(const std::string& node_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    NodeInfo* n = findNode(node_id);
    if (n) n->status = NodeStatus::OFFLINE;
}

void GPUClusterCoordinator::expireStaleNodes()
{
    const auto now     = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(config_.node_timeout_ms);

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& n : nodes_) {
        if (n.status == NodeStatus::ONLINE &&
            (now - n.last_heartbeat) > timeout) {
            n.status = NodeStatus::OFFLINE;
        }
    }
}

// ============================================================================
// Work placement
// ============================================================================

const GPUClusterCoordinator::NodeInfo*
GPUClusterCoordinator::selectNode(uint64_t required_vram_bytes)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const NodeInfo* best = nullptr;
    for (const auto& n : nodes_) {
        if (n.status != NodeStatus::ONLINE) continue;
        if (required_vram_bytes > 0 &&
            n.free_vram_bytes < required_vram_bytes) continue;
        if (best == nullptr ||
            n.free_vram_bytes > best->free_vram_bytes) {
            best = &n;
        }
    }
    return best;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<GPUClusterCoordinator::NodeInfo>
GPUClusterCoordinator::getClusterNodes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_;
}

std::vector<GPUClusterCoordinator::NodeInfo>
GPUClusterCoordinator::getOnlineNodes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeInfo> result;
    for (const auto& n : nodes_) {
        if (n.status == NodeStatus::ONLINE) {
            result.push_back(n);
        }
    }
    return result;
}

size_t GPUClusterCoordinator::totalNodes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

size_t GPUClusterCoordinator::onlineNodeCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t n = 0;
    for (const auto& node : nodes_) {
        if (node.status == NodeStatus::ONLINE) ++n;
    }
    return n;
}

} // namespace gpu
} // namespace themis
