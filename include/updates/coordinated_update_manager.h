/**
 * @file coordinated_update_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: coordinated_update_manager.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "updates/hot_reload_engine.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace updates {

/**
 * @brief Identifies one node that participates in a coordinated update.
 *
 * Nodes are updated strictly in ascending sequence_number order.  The
 * replication leader (highest sequence_number, or the node explicitly
 * marked is_leader) is updated last to preserve replication safety.
 */
struct NodeDescriptor {
    /// Stable unique identifier for this node (hostname, UUID, …).
    std::string node_id;

    /// 0-based position in the update sequence (lower = updated sooner).
    uint32_t sequence_number = 0;

    /// True for the replication leader.  When CoordinatedUpdateConfig::leader_last
    /// is true the leader is unconditionally assigned the highest sequence slot.
    bool is_leader = false;
};

/**
 * @brief Configuration for a coordinated multi-node update session.
 *
 * Constraints (from module ROADMAP):
 *  - HotReloadEngine is used for the actual file-level update on each node;
 *    callers must not run concurrent updates.
 *  - Actual inter-node transport (wait/signal) is injected via callbacks so
 *    that this class remains transport-agnostic and fully unit-testable.
 */
struct CoordinatedUpdateConfig {
    /// All nodes participating in this update.  The list is sorted internally
    /// by sequence_number; the caller may supply them in any order.
    std::vector<NodeDescriptor> nodes;

    /// Target version string (e.g., "1.7.0").
    std::string version;

    /// Node ID of *this* node (the one running this manager instance).
    std::string local_node_id;

    /// When true, automatically rollback the local update on failure.
    bool rollback_on_failure = true;

    /// When true, the replication leader is always assigned the last sequence
    /// slot, guaranteeing it is updated after all replicas.
    bool leader_last = true;

    /// Timeout for waiting on the previous node to complete its update before
    /// this node proceeds.  Zero means wait indefinitely.
    std::chrono::milliseconds wait_timeout{30000};
};

/**
 * @brief Per-node update state in a coordinated session.
 */
enum class NodeUpdateState {
    PENDING,      ///< Not yet started.
    IN_PROGRESS,  ///< Update is being applied.
    COMPLETED,    ///< Update successfully applied.
    FAILED,       ///< Update failed (or was not attempted).
    ROLLED_BACK,  ///< Update was rolled back after a failure.
};

/**
 * @brief Status snapshot for a single node in the coordinated update.
 */
struct NodeUpdateStatus {
    /// Node identifier.
    std::string node_id;

    /// Current state.
    NodeUpdateState state = NodeUpdateState::PENDING;

    /// Human-readable error description (non-empty only when state == FAILED).
    std::string error_message;

    /// Rollback ID returned by HotReloadEngine::applyHotReload (non-empty when
    /// state == COMPLETED or ROLLED_BACK).
    std::string rollback_id;

    /// Position in the update sequence.
    uint32_t sequence_number = 0;

    /// True when this descriptor refers to the local node.
    bool is_local = false;
};

/**
 * @brief Overall result of a coordinated update triggered by applyLocalUpdate().
 */
struct CoordinatedUpdateResult {
    bool success = false;

    /// Human-readable error description on failure.
    std::string error_message;

    /// Status of every node known to the manager.
    std::vector<NodeUpdateStatus> node_statuses;

    /// Number of nodes that reached COMPLETED state during this session.
    uint32_t nodes_updated = 0;

    /// Number of nodes that reached FAILED state.
    uint32_t nodes_failed = 0;

    /// Number of nodes that were rolled back.
    uint32_t nodes_rolled_back = 0;
};

/**
 * @brief Manages the coordinated, replication-safe update of a single node
 *        within a multi-node ThemisDB cluster.
 *
 * Each node in the cluster runs its own CoordinatedUpdateManager instance.
 * Nodes update in a deterministic, ascending sequence so that replicas are
 * always updated before the replication leader, preventing version skew.
 *
 * Inter-node synchronisation is injected via two callbacks:
 *  - WaitForPreviousFunc: blocks until the preceding node signals readiness.
 *  - SignalReadyFunc: notifies the next node that this node has completed.
 *
 * This design keeps the class transport-agnostic and easy to unit-test.
 *
 * Usage example:
 * @code
 *   CoordinatedUpdateConfig cfg;
 *   cfg.version        = "1.7.0";
 *   cfg.local_node_id  = hostname;
 *   cfg.nodes          = { {"node-a", 0, false},
 *                           {"node-b", 1, false},
 *                           {"node-c", 2, true} };   // node-c is leader
 *
 *   CoordinatedUpdateManager mgr(hot_reload_engine, cfg);
 *
 *   mgr.setWaitForPreviousFunc([&](const std::string& prev_id,
 *                                   std::chrono::milliseconds timeout) {
 *       return cluster_bus.waitForNodeReady(prev_id, timeout);
 *   });
 *   mgr.setSignalReadyFunc([&](const std::string& local_id, bool ok) {
 *       cluster_bus.signalReady(local_id, ok);
 *   });
 *
 *   auto result = mgr.applyLocalUpdate();
 *   if (!result.success) {
 *       LOG_ERROR("Coordinated update failed: {}", result.error_message);
 *   }
 * @endcode
 */
class CoordinatedUpdateManager {
public:
    /**
     * @brief Callback invoked to wait for the previous node in the sequence to
     *        complete its update before this node may proceed.
     *
     * @param prev_node_id  Node ID of the preceding node.
     * @param timeout       Maximum time to wait (zero = wait indefinitely).
     * @return true  when the previous node has completed successfully.
     * @return false on timeout or when the previous node failed.
     */
    using WaitForPreviousFunc =
        std::function<bool(const std::string& prev_node_id,
                           std::chrono::milliseconds timeout)>;

    /**
     * @brief Callback invoked after this node's update attempt to signal
     *        the next node in the sequence.
     *
     * @param local_node_id  Node ID of this node.
     * @param success        true when the update succeeded; false on failure.
     */
    using SignalReadyFunc =
        std::function<void(const std::string& local_node_id, bool success)>;

    /**
     * @brief Callback for progress reporting.
     *
     * @param nodes_done   Number of nodes whose update is complete.
     * @param nodes_total  Total nodes in the sequence.
     * @param message      Human-readable status message.
     */
    using ProgressCallback =
        std::function<void(uint32_t nodes_done,
                           uint32_t nodes_total,
                           const std::string& message)>;

    /**
     * @brief Construct a coordinated update manager for the local node.
     *
     * @param engine  Shared hot-reload engine (must outlive this object).
     * @param config  Session configuration including the full node list.
     * @throws std::invalid_argument if engine is null, local_node_id is
     *         unknown, or the node list is empty.
     */
    CoordinatedUpdateManager(std::shared_ptr<HotReloadEngine> engine,
                              const CoordinatedUpdateConfig& config);

    ~CoordinatedUpdateManager() = default;

    // Non-copyable
    CoordinatedUpdateManager(const CoordinatedUpdateManager&) = delete;
    CoordinatedUpdateManager& operator=(const CoordinatedUpdateManager&) = delete;

    // ---------- Core update operation ----------

    /**
     * @brief Wait for this node's turn in the sequence and apply the update.
     *
     * Steps:
     *  1. If this is not the first node, invoke WaitForPreviousFunc to block
     *     until the preceding node completes.
     *  2. Apply the update via HotReloadEngine::applyHotReload.
     *  3. Invoke SignalReadyFunc to unblock the next node.
     *  4. On failure, optionally rollback via HotReloadEngine::rollback.
     *
     * @return Overall result including per-node status snapshot.
     */
    CoordinatedUpdateResult applyLocalUpdate();

    /**
     * @brief Rollback the update previously applied by applyLocalUpdate().
     *
     * @param reason  Human-readable reason (optional).
     * @return true if the rollback succeeded (or no rollback was needed).
     */
    bool rollback(const std::string& reason = "");

    // ---------- Coordinated rollback enhancements (v1.8.1 – Q3 2026) ----------

    /**
     * @brief Perform coordinated rollback across all nodes in reverse sequence.
     *
     * The leader node rolls back first (reverse of the update order), then
     * replicas in reverse sequence. This prevents replication skew during
     * rollback.
     *
     * Uses implicit retry logic when intermediate node rollbacks fail;
     * collects per-node results and returns aggregate success.
     *
     * @param reason Human-readable reason for rollback
     * @return true if all nodes rolled back successfully; false if any failed
     * @since 1.8.1
     */
    bool coordinatedRollback(const std::string& reason = "");

    /**
     * @brief Perform coordinated rollback with isolation on per-node failures.
     *
     * When a replica fails to rollback, that node is isolated (marked FAILED)
     * and the rollback continues with remaining nodes.  No cascading failures.
     *
     * @param reason Human-readable reason for rollback
     * @return CoordinatedUpdateResult with per-node status and aggregate result
     * @since 1.8.1
     */
    CoordinatedUpdateResult coordinatedRollbackWithIsolation(const std::string& reason = "");

    /**
     * @brief Check if any node is isolated due to rollback failure.
     *
     * When coordinatedRollbackWithIsolation() encounters a failure, that node
     * remains isolated to prevent cascade. This method checks if any nodes
     * are in that state.
     *
     * @return true if at least one node is isolated
     * @since 1.8.1
     */
    bool hasIsolatedNodes() const;

    /**
     * @brief Return count of nodes that failed rollback and are isolated.
     *
     * @return Number of isolated nodes
     * @since 1.8.1
     */
    uint32_t isolatedNodeCount() const;

    // ---------- Accessors ----------

    /**
     * @brief Return the sequence number assigned to the local node.
     */
    uint32_t localSequenceNumber() const;

    /**
     * @brief Return the total number of nodes in the update sequence.
     */
    uint32_t totalNodes() const;

    /**
     * @brief Return true when the local node is the replication leader.
     */
    bool isLeader() const;

    /**
     * @brief Return a snapshot of all known node statuses.
     */
    std::vector<NodeUpdateStatus> nodeStatuses() const;

    // ---------- Callback registration ----------

    /**
     * @brief Register the callback used to wait on the preceding node.
     *
     * When not set, the manager assumes it is the first node in the sequence
     * (or that there is no predecessor) and proceeds immediately.
     */
    void setWaitForPreviousFunc(WaitForPreviousFunc fn);

    /**
     * @brief Register the callback used to signal the next node.
     *
     * When not set, signalling is a no-op (useful when this is the last node).
     */
    void setSignalReadyFunc(SignalReadyFunc fn);

    /**
     * @brief Register a progress callback.
     */
    void setProgressCallback(ProgressCallback fn);

private:
    /// Returns a pointer to the local node's status entry (never null after
    /// successful construction).
    NodeUpdateStatus* localStatus();
    const NodeUpdateStatus* localStatus() const;

    /// Returns the node descriptor for the local node.
    const NodeDescriptor* localDescriptor() const;

    /// Returns the node descriptor for the predecessor of the local node,
    /// or nullptr when the local node is first in the sequence.
    const NodeDescriptor* predecessorDescriptor() const;

    /// Helper for coordinated rollback in reverse sequence
    bool performNodeRollback(const NodeDescriptor& node, const std::string& reason);

    /// Mark a node as isolated due to rollback failure
    void isolateNode(const std::string& node_id, const std::string& reason);

    void reportProgress(const std::string& message);

    mutable std::mutex mutex_;

    std::shared_ptr<HotReloadEngine> engine_;
    CoordinatedUpdateConfig config_;

    /// Sorted (by sequence_number) node descriptors.
    std::vector<NodeDescriptor> sorted_nodes_;

    /// Per-node status (same order as sorted_nodes_).
    std::vector<NodeUpdateStatus> node_statuses_;

    /// Rollback ID for the local update (set after a successful apply).
    std::string local_rollback_id_;

    /// True once rollback() has been called.
    bool is_rolled_back_{false};

    /// Track nodes isolated due to rollback failures
    std::vector<std::string> isolated_nodes_;

    WaitForPreviousFunc wait_for_previous_fn_;
    SignalReadyFunc     signal_ready_fn_;
    ProgressCallback    progress_cb_;
};

} // namespace updates
} // namespace themis
