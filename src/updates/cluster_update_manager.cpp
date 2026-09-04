/**
 * @file cluster_update_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/cluster_update_manager.h"
#include "updates/batch5_safety_helpers.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ClusterUpdateManager::ClusterUpdateManager(const Config& config)
    : config_(config)
{
    if (config_.nodes.empty()) {
        throw std::invalid_argument(
            "ClusterUpdateManager: node list must not be empty");
    }

    // Build the update order: non-leaders first (in the order they were
    // supplied), leader(s) last.
    sorted_nodes_.reserve(config_.nodes.size());
    for (const auto& n : config_.nodes) {
        if (!n.is_leader) {
            sorted_nodes_.push_back(n);
        }
    }
    for (const auto& n : config_.nodes) {
        if (n.is_leader) {
            sorted_nodes_.push_back(n);
        }
    }

    // Initialise per-node status entries.
    node_statuses_.reserve(sorted_nodes_.size());
    for (const auto& n : sorted_nodes_) {
        ClusterNodeStatus s;
        s.node_id   = n.node_id;
        s.is_leader = n.is_leader;
        s.state     = ClusterNodeState::PENDING;
        node_statuses_.push_back(std::move(s));
    }

    LOG_INFO("ClusterUpdateManager initialised: {} nodes ({} leader(s))",
             sorted_nodes_.size(),
             static_cast<size_t>(std::count_if(
                 sorted_nodes_.begin(), sorted_nodes_.end(),
                 [](const ClusterNode& n) { return n.is_leader; })));
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/// Sentinel returned by findNodeIndex when the node is not found.
static constexpr int NODE_NOT_FOUND = -1;

int ClusterUpdateManager::findNodeIndex(const std::string& node_id) const {
    for (size_t i = 0; i < node_statuses_.size(); ++i) {
        if (node_statuses_[i].node_id == node_id) {
            return static_cast<int>(i);
        }
    }
    return NODE_NOT_FOUND;
}

void ClusterUpdateManager::emitProgress(const std::string& current_node,
                                        const std::string& status_msg) {
    ProgressCallback cb;
    ClusterUpdateProgress progress;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cb = progress_cb_;
        progress.total_nodes  = node_statuses_.size();
        progress.current_node = current_node;
        progress.status       = status_msg;
        for (const auto& s : node_statuses_) {
            if (s.state == ClusterNodeState::COMPLETED) {
              ++progress.nodes_updated;
            }
            if (s.state == ClusterNodeState::FAILED ||
                s.state == ClusterNodeState::ROLLED_BACK) ++progress.nodes_failed;
        }
        progress.node_statuses = node_statuses_;
    }
    if (cb) {
        try {
            cb(progress);
        } catch (const std::exception& e) {
            LOG_WARN("ClusterUpdateManager: progress callback threw: {}", e.what());
        }
    }
}

bool ClusterUpdateManager::updateSingleNode(const ClusterNode&          node,
                                             const std::string&          version,
                                             const ClusterUpdateOptions& opts) {
    // ---- DRAINING ----
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = findNodeIndex(node.node_id);
        if (idx >= 0) {
          node_statuses_[idx].state = ClusterNodeState::DRAINING;
        }
    }
    emitProgress(node.node_id, "Draining connections on node " + node.node_id);

    if (cancelled_.load()) {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = findNodeIndex(node.node_id);
        if (idx >= 0) {
            node_statuses_[idx].state         = ClusterNodeState::SKIPPED;
            node_statuses_[idx].error_message = "update cancelled";
        }
        return false;
    }

    // ---- APPLYING (download + install, delegated to NodeUpdateFunc) ----
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = findNodeIndex(node.node_id);
        if (idx >= 0) {
          node_statuses_[idx].state = ClusterNodeState::APPLYING;
        }
    }
    emitProgress(node.node_id, "Applying update on node " + node.node_id);

    NodeUpdateFunc update_fn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        update_fn = node_update_fn_;
    }

    bool update_ok = true;
    if (update_fn) {
        update_ok = update_fn(node, version, opts);
    }
    // When no NodeUpdateFunc is registered, the no-op default is true.

    if (!update_ok) {
        LOG_ERROR("ClusterUpdateManager: update failed on node={}", node.node_id);

        bool should_rollback = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            int idx = findNodeIndex(node.node_id);
            if (idx >= 0) {
                node_statuses_[idx].state         = ClusterNodeState::FAILED;
                node_statuses_[idx].error_message = "NodeUpdateFunc reported failure";
            }
            should_rollback = opts.rollback_on_failure;
        }

        if (should_rollback) {
            NodeRollbackFunc rollback_fn;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                rollback_fn = node_rollback_fn_;
            }
            if (rollback_fn) {
                // The update failed before applied_version could be set
                // (NodeUpdateFunc returned false before HEALTH_CHECK
                // transition).  Pass an empty string to indicate no version
                // was successfully installed on this node.
                rollback_fn(node, "");
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                int idx = findNodeIndex(node.node_id);
                if (idx >= 0) {
                  node_statuses_[idx].state = ClusterNodeState::ROLLED_BACK;
                }
            }
            LOG_WARN("ClusterUpdateManager: rolled back node={}", node.node_id);
        }

        emitProgress(node.node_id, "Update failed on node " + node.node_id);
        return false;
    }

    // ---- HEALTH CHECK ----
    // Record applied_version now (before health check) so that the
    // NodeRollbackFunc can reference it if the health check fails.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = findNodeIndex(node.node_id);
        if (idx >= 0) {
            node_statuses_[idx].state           = ClusterNodeState::HEALTH_CHECK;
            node_statuses_[idx].applied_version = version;
        }
    }
    emitProgress(node.node_id, "Health check on node " + node.node_id);

    NodeHealthCheckFunc hc_fn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        hc_fn = node_health_check_fn_;
    }

    bool healthy = true;
    if (hc_fn) {
        healthy = hc_fn(node, opts.health_check_timeout);
    }

    if (!healthy) {
        LOG_ERROR("ClusterUpdateManager: health check failed on node={}",
                  node.node_id);

        bool should_rollback = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            int idx = findNodeIndex(node.node_id);
            if (idx >= 0) {
                node_statuses_[idx].state         = ClusterNodeState::FAILED;
                node_statuses_[idx].error_message = "post-update health check failed";
            }
            should_rollback = opts.rollback_on_failure;
        }

        if (should_rollback) {
            NodeRollbackFunc rollback_fn;
            std::string applied_ver = {};
            {
                std::lock_guard<std::mutex> lock(mutex_);
                rollback_fn = node_rollback_fn_;
                int idx = findNodeIndex(node.node_id);
                if (idx >= 0) {
                  applied_ver = node_statuses_[idx].applied_version;
                }
            }
            if (rollback_fn) {
                rollback_fn(node, applied_ver);
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                int idx = findNodeIndex(node.node_id);
                if (idx >= 0) {
                  node_statuses_[idx].state = ClusterNodeState::ROLLED_BACK;
                }
            }
            LOG_WARN("ClusterUpdateManager: rolled back node={} after health check failure",
                     node.node_id);
        }

        emitProgress(node.node_id,
                     "Health check failed on node " + node.node_id);
        return false;
    }

    // ---- REJOINING → COMPLETED ----
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = findNodeIndex(node.node_id);
        if (idx >= 0) {
          node_statuses_[idx].state = ClusterNodeState::REJOINING;
        }
    }
    emitProgress(node.node_id, "Node " + node.node_id + " rejoining cluster");

    {
        std::lock_guard<std::mutex> lock(mutex_);
        int idx = findNodeIndex(node.node_id);
        if (idx >= 0) {
            node_statuses_[idx].state = ClusterNodeState::COMPLETED;
            // applied_version was set when transitioning to HEALTH_CHECK.
        }
    }

    LOG_INFO("ClusterUpdateManager: node={} successfully updated to version={}",
             node.node_id, version);
    emitProgress(node.node_id,
                 "Node " + node.node_id + " updated to " + version);
    return true;
}

// ---------------------------------------------------------------------------
// Core operation
// ---------------------------------------------------------------------------

ClusterUpdateResult ClusterUpdateManager::updateCluster(
    const std::string&          version,
    const ClusterUpdateOptions& opts)
{
    // Reset cancellation flag at the start of each new update run.
    cancelled_.store(false);

    // Reset all node statuses to PENDING.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& s : node_statuses_) {
            s.state           = ClusterNodeState::PENDING;
            s.error_message.clear();
            s.applied_version.clear();
        }
    }

    LOG_INFO("ClusterUpdateManager: starting cluster update to version={}",
             version);

    emitProgress("", "Starting cluster update to " + version);

    // Walk through sorted_nodes_ (non-leaders first, leader last).
    for (const ClusterNode& node : sorted_nodes_) {
        if (cancelled_.load()) {
            LOG_WARN("ClusterUpdateManager: update cancelled, skipping "
                     "remaining nodes");
            break;
        }

        updateSingleNode(node, version, opts);

        // Version-skew protection: abort if this non-leader failed and
        // rollback_on_failure is true (we do not update the leader if a
        // follower could not be brought up successfully).
        if (!node.is_leader && opts.rollback_on_failure) {
            std::lock_guard<std::mutex> lock(mutex_);
            int idx = findNodeIndex(node.node_id);
            if (idx >= 0) {
                ClusterNodeState st = node_statuses_[idx].state;
                if (st == ClusterNodeState::FAILED ||
                    st == ClusterNodeState::ROLLED_BACK) {
                    LOG_ERROR(
                        "ClusterUpdateManager: aborting cluster update – "
                        "follower node={} failed and rollback_on_failure=true",
                        node.node_id);
                    break;
                }
            }
        }
    }

    // Build result.
    ClusterUpdateResult result;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result.node_statuses = node_statuses_;
        for (const auto& s : node_statuses_) {
            if (s.state == ClusterNodeState::COMPLETED) {
              ++result.nodes_updated;
            }
            if (s.state == ClusterNodeState::FAILED) {
              ++result.nodes_failed;
            }
            if (s.state == ClusterNodeState::ROLLED_BACK) {
              ++result.nodes_rolled_back;
            }
        }
        result.success = (result.nodes_failed == 0 &&
                          result.nodes_rolled_back == 0 &&
                          !cancelled_.load());

        if (cancelled_.load() && !result.success) {
            result.error_message = "cluster update was cancelled";
        } else if (!result.success) {
            result.error_message = "one or more nodes failed to update";
        }
    }

    if (result.success) {
        LOG_INFO("ClusterUpdateManager: cluster update to version={} "
                 "completed successfully ({} nodes updated)",
                 version, result.nodes_updated);
    } else {
        LOG_ERROR("ClusterUpdateManager: cluster update to version={} "
                  "failed: {}", version, result.error_message);
    }

    emitProgress("", result.success
                         ? "Cluster update to " + version + " completed"
                         : "Cluster update failed: " + result.error_message);

    return result;
}

ClusterUpdateResult ClusterUpdateManager::updateCluster(
    const std::string& version)
{
    return updateCluster(version, config_.default_options);
}

// ---------------------------------------------------------------------------
// Cancellation / accessors
// ---------------------------------------------------------------------------

void ClusterUpdateManager::cancelUpdate() {
    cancelled_.store(true);
    LOG_WARN("ClusterUpdateManager: cancellation requested");
}

std::vector<ClusterNodeStatus> ClusterUpdateManager::nodeStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return node_statuses_;
}

bool ClusterUpdateManager::isCancelled() const {
    return cancelled_.load();
}

size_t ClusterUpdateManager::totalNodes() const {
    return sorted_nodes_.size();
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void ClusterUpdateManager::setNodeUpdateFunc(NodeUpdateFunc fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_update_fn_ = std::move(fn);
}

void ClusterUpdateManager::setNodeHealthCheckFunc(NodeHealthCheckFunc fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_health_check_fn_ = std::move(fn);
}

void ClusterUpdateManager::setNodeRollbackFunc(NodeRollbackFunc fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_rollback_fn_ = std::move(fn);
}

void ClusterUpdateManager::setProgressCallback([[maybe_unused]] ProgressCallback fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    progress_cb_ = std::move(fn);
}

} // namespace updates
} // namespace themis

