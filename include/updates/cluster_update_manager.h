/**
 * @file cluster_update_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

/**
 * @brief Per-node state within a cluster update.
 */
enum class ClusterNodeState {
    PENDING,       ///< Not yet started.
    DRAINING,      ///< Draining active connections before update.
    APPLYING,      ///< Applying the update on the node (download + install).
    HEALTH_CHECK,  ///< Running post-update health check.
    REJOINING,     ///< Node is rejoining the cluster after update.
    COMPLETED,     ///< Update successfully applied and node is healthy.
    FAILED,        ///< Update failed (or was not attempted).
    ROLLED_BACK,   ///< Node was rolled back to the previous version after failure.
    SKIPPED,       ///< Node was skipped (e.g., already at target version).
};

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/**
 * @brief Describes one node that participates in a cluster-wide update.
 */
struct ClusterNode {
    /// Stable unique identifier (hostname, UUID, …).
    std::string node_id;

    /// Human-readable address / endpoint (informational only).
    std::string address;

    /// When true this node is the Raft/replication leader and will be
    /// updated last in a rolling update.
    bool is_leader = false;

    /// Current version running on this node.
    std::string current_version;
};

/**
 * @brief Live status snapshot for a single node in the cluster update.
 */
struct ClusterNodeStatus {
    /// Node identifier.
    std::string node_id;

    /// Current state machine state.
    ClusterNodeState state = ClusterNodeState::PENDING;

    /// Non-empty on failure.
    std::string error_message;

    /// Version string that was applied to this node.  Set when the node
    /// transitions to HEALTH_CHECK state (immediately after the
    /// NodeUpdateFunc succeeds) and remains populated through COMPLETED or
    /// ROLLED_BACK.  Empty while the node is in PENDING, DRAINING, or
    /// APPLYING state, or if the NodeUpdateFunc itself failed.
    std::string applied_version;

    /// true when this status entry describes the coordinator node.
    bool is_leader = false;
};

/**
 * @brief Snapshot delivered to the progress callback after each node
 *        transitions to a new state.
 */
struct ClusterUpdateProgress {
    /// Total number of nodes participating in this update.
    size_t total_nodes = 0;

    /// Nodes that have reached COMPLETED state.
    size_t nodes_updated = 0;

    /// Nodes that have reached FAILED or ROLLED_BACK state.
    size_t nodes_failed = 0;

    /// Node ID currently being processed.
    std::string current_node;

    /// Human-readable status line.
    std::string status;

    /// Per-node detail.
    std::vector<ClusterNodeStatus> node_statuses;
};

/**
 * @brief Options controlling how a cluster-wide update is executed.
 */
struct ClusterUpdateOptions {
    /// Timeout for the per-node health check after the update is applied.
    std::chrono::seconds health_check_timeout{30};

    /// When true, invoke the NodeRollbackFunc (if registered) on a node
    /// that fails to update or fails its post-update health check, and abort
    /// updating any subsequent nodes.
    bool rollback_on_failure = true;
};

/**
 * @brief Final outcome of a cluster-wide update initiated by
 *        ClusterUpdateManager::updateCluster().
 */
struct ClusterUpdateResult {
    bool success = false;

    /// Human-readable description on failure.
    std::string error_message;

    /// Per-node outcome.
    std::vector<ClusterNodeStatus> node_statuses;

    /// Number of nodes that reached COMPLETED.
    size_t nodes_updated = 0;

    /// Number of nodes that reached FAILED.
    size_t nodes_failed = 0;

    /// Number of nodes that were rolled back.
    size_t nodes_rolled_back = 0;
};

// ---------------------------------------------------------------------------
// ClusterUpdateManager
// ---------------------------------------------------------------------------

/**
 * @brief Orchestrates a rolling update across all nodes of a ThemisDB cluster.
 *
 * The manager is transport-agnostic: actual per-node update execution,
 * health checks, and rollbacks are injected as callbacks.  This keeps
 * the class independently testable and decoupled from any particular
 * cluster transport (gRPC, Raft RPC, …).
 *
 * Update procedure (always sequential / rolling)
 * -----------------------------------------------
 *  1. For each non-leader node (in the order supplied), then for each
 *     leader node:
 *     a. Mark DRAINING  — emit progress.
 *     b. Invoke NodeUpdateFunc (→ APPLYING).
 *     c. Invoke NodeHealthCheckFunc (→ HEALTH_CHECK).
 *     d. On pass: REJOINING → COMPLETED.
 *     e. On fail (rollback_on_failure=true):
 *        - Invoke NodeRollbackFunc (if registered).
 *        - Mark ROLLED_BACK; abort update of remaining nodes.
 *  2. Emit final progress snapshot.
 *
 * Usage example
 * -------------
 * @code
 *   ClusterUpdateManager::Config cfg;
 *   cfg.nodes = {
 *       { "node-a", "host-a:6543", false, "1.6.0" },
 *       { "node-b", "host-b:6543", false, "1.6.0" },
 *       { "node-c", "host-c:6543", true,  "1.6.0" },  // leader
 *   };
 *
 *   ClusterUpdateManager mgr(cfg);
 *
 *   mgr.setNodeUpdateFunc([](const ClusterNode& node,
 *                             const std::string& version,
 *                             const ClusterUpdateOptions& opts)
 *   {
 *       return my_rpc_client.updateNode(node.node_id, version);
 *   });
 *
 *   mgr.setNodeRollbackFunc([](const ClusterNode& node,
 *                               const std::string& applied_version)
 *   {
 *       return my_rpc_client.rollbackNode(node.node_id, applied_version);
 *   });
 *
 *   mgr.setProgressCallback([](const ClusterUpdateProgress& p) {
 *       std::cout << p.nodes_updated << "/" << p.total_nodes
 *                 << " – " << p.status << "\n";
 *   });
 *
 *   auto result = mgr.updateCluster("1.7.0");
 *   if (!result.success) {
 *       LOG_ERROR("Cluster update failed: {}", result.error_message);
 *   }
 * @endcode
 */
class ClusterUpdateManager {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Injected callback that performs the actual update on a single
     *        remote (or local) node.
     *
     * @param node     Description of the node to update.
     * @param version  Target version string.
     * @param opts     Update options.
     * @return true if the update succeeded on that node.
     */
    using NodeUpdateFunc =
        std::function<bool(const ClusterNode&        node,
                           const std::string&        version,
                           const ClusterUpdateOptions& opts)>;

    /**
     * @brief Injected callback that checks whether a node is healthy.
     *
     * @param node     Node to health-check.
     * @param timeout  Maximum time to wait for a healthy response.
     * @return true when the node passes the health check.
     */
    using NodeHealthCheckFunc =
        std::function<bool(const ClusterNode&              node,
                           std::chrono::seconds             timeout)>;

    /**
     * @brief Injected callback that rolls back an update on a single node.
     *
     * Invoked by updateCluster() when a node fails its update or post-update
     * health check and @c rollback_on_failure is true.
     *
     * @param node             Node to roll back.
     * @param applied_version  The version string that was applied (from
     *                         ClusterNodeStatus::applied_version), which can
     *                         be used to identify what to undo.
     * @return true if the rollback succeeded.
     */
    using NodeRollbackFunc =
        std::function<bool(const ClusterNode& node,
                           const std::string& applied_version)>;

    /**
     * @brief Progress callback invoked after each significant state change.
     */
    using ProgressCallback =
        std::function<void(const ClusterUpdateProgress&)>;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Static configuration supplied at construction time.
     */
    struct Config {
        /// All nodes that participate in the update.
        std::vector<ClusterNode> nodes;

        /// Default options applied when none are passed to updateCluster().
        ClusterUpdateOptions default_options;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a ClusterUpdateManager.
     *
     * @param config  Cluster topology and default options.
     * @throws std::invalid_argument if the node list is empty.
     */
    explicit ClusterUpdateManager(const Config& config);

    ~ClusterUpdateManager() = default;

    ClusterUpdateManager(const ClusterUpdateManager&)            = delete;
    ClusterUpdateManager& operator=(const ClusterUpdateManager&) = delete;

    // -----------------------------------------------------------------------
    // Core operation
    // -----------------------------------------------------------------------

    /**
     * @brief Initiate a cluster-wide update to @p version.
     *
     * This call blocks until all nodes have been updated (or failed).
     * Use setProgressCallback() to receive incremental progress.
     *
     * @param version  Target version string (e.g. "1.7.0").
     * @param opts     Override the per-update options.
     * @return ClusterUpdateResult describing the final outcome.
     */
    ClusterUpdateResult updateCluster(
        const std::string&          version,
        const ClusterUpdateOptions& opts);

    /**
     * @brief Initiate a cluster-wide update using default options.
     */
    ClusterUpdateResult updateCluster(const std::string& version);

    /**
     * @brief Cancel an in-progress cluster update.
     *
     * Sets an internal cancellation flag.  The current node's update is
     * allowed to finish; subsequent nodes will be skipped.
     */
    void cancelUpdate();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Return a snapshot of all known node statuses.
     */
    std::vector<ClusterNodeStatus> nodeStatuses() const;

    /**
     * @brief Return true when a cancellation has been requested.
     */
    bool isCancelled() const;

    /**
     * @brief Return the total number of nodes in the cluster.
     */
    size_t totalNodes() const;

    // -----------------------------------------------------------------------
    // Callback registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register the callback that performs the per-node update.
     *
     * When not set, the manager uses a no-op that returns true (useful
     * for dry-run or unit-test scenarios).
     */
    void setNodeUpdateFunc(NodeUpdateFunc fn);

    /**
     * @brief Register the callback that performs per-node health checks.
     *
     * When not set, the manager assumes the node is healthy immediately
     * after the NodeUpdateFunc succeeds.
     */
    void setNodeHealthCheckFunc(NodeHealthCheckFunc fn);

    /**
     * @brief Register the callback that performs per-node rollbacks.
     *
     * Called when a node's update or health check fails and
     * @c rollback_on_failure is true.  When not set, the node is marked
     * ROLLED_BACK in state but no remote action is taken.
     */
    void setNodeRollbackFunc(NodeRollbackFunc fn);

    /**
     * @brief Register a progress callback.
     */
    void setProgressCallback(ProgressCallback fn);

private:
    // Internal helpers

    /// Returns index into node_statuses_ for the given node_id, or -1.
    int findNodeIndex(const std::string& node_id) const;

    /// Emit a progress snapshot to the registered callback (if any).
    void emitProgress(const std::string& current_node,
                      const std::string& status_msg);

    /// Apply the update to a single node and update its status entry.
    /// Returns true on success.
    bool updateSingleNode(const ClusterNode&          node,
                          const std::string&          version,
                          const ClusterUpdateOptions& opts);

    mutable std::mutex mutex_;

    Config                        config_;
    std::vector<ClusterNode>      sorted_nodes_; ///< leader last
    std::vector<ClusterNodeStatus> node_statuses_;

    std::atomic<bool> cancelled_{false};

    NodeUpdateFunc      node_update_fn_;
    NodeHealthCheckFunc node_health_check_fn_;
    NodeRollbackFunc    node_rollback_fn_;
    ProgressCallback    progress_cb_;
};

} // namespace updates
} // namespace themis
