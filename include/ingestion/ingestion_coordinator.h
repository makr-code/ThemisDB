/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_coordinator.h                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ingestion/ingestion_manager.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <map>
#include <unordered_map>
#include <future>
#include <thread>

namespace themis {
namespace ingestion {

// ============================================================================
// NodeInfo — lightweight description of a coordinator-managed worker node
// ============================================================================

struct NodeInfo {
    std::string node_id;   ///< Unique node identifier
    std::string address;   ///< Network address for remote nodes (empty = in-process)
    bool is_local = true;  ///< true for in-process nodes
    bool is_leader = false; ///< true when this node currently holds the leader lease

    NodeInfo() = default;
    NodeInfo(const std::string& id,
             const std::string& addr = "",
             bool local = true)
        : node_id(id), address(addr), is_local(local) {}
};

// ============================================================================
// IIngestionWorkerNode — interface for coordinator-managed worker nodes
// ============================================================================

/**
 * @brief Abstract worker node that can execute an ingestion sub-run.
 *
 * Each node is responsible for ingesting a subset of sources assigned by
 * the `IngestionCoordinator` via consistent hashing.
 */
class IIngestionWorkerNode {
public:
    virtual ~IIngestionWorkerNode() = default;

    /// Return this node's unique identifier.
    virtual const std::string& nodeId() const = 0;

    /**
     * @brief Ingest the assigned set of sources and return a partial report.
     *
     * @param sources            Sources assigned to this node
     * @param target_collection  Collection to write ingested documents into
     * @param progress_callback  Optional progress callback forwarded to the
     *                           underlying IngestionManager
     * @return Partial IngestionReport that will be aggregated by the coordinator
     */
    virtual IngestionReport ingest(
        const std::vector<SourceConfig>& sources,
        const std::string& target_collection,
        ProgressCallback progress_callback) = 0;

    /// Returns true when the node is idle and ready to accept work.
    virtual bool isAvailable() const = 0;
};

// ============================================================================
// LeaderLease — TTL-based distributed lock record
// ============================================================================

/**
 * @brief Snapshot of the current leader lease.
 *
 * A lease is valid when `owner_node_id` is non-empty and `expires_at` is in
 * the future.  The `epoch` counter increments each time ownership changes,
 * allowing callers to detect stale leases that happen to re-acquire the same
 * node ID.
 */
struct LeaderLease {
    std::string owner_node_id;                        ///< Node that holds the lease
    std::chrono::steady_clock::time_point expires_at; ///< Absolute expiry instant
    uint64_t epoch = 0;                               ///< Monotone election counter

    /** @return true when the lease is currently valid */
    bool isValid() const {
        return !owner_node_id.empty() &&
               std::chrono::steady_clock::now() < expires_at;
    }
};

// ============================================================================
// ILeaderElection — pluggable backend for leader election
// ============================================================================

/**
 * @brief Strategy interface for leader election.
 *
 * In production the default `InProcessLeaderElection` stores the lease in
 * shared memory.  For multi-host deployments the caller can inject a Redis-
 * or database-backed implementation via
 * `IngestionCoordinator::setLeaderElectionForTesting()`.
 */
class ILeaderElection {
public:
    virtual ~ILeaderElection() = default;

    /**
     * @brief Try to acquire (or renew) the leader lease.
     *
     * @param node_id  Node requesting leadership
     * @param ttl      Lease duration; must be renewed before expiry
     * @return true if this node is now the leader
     */
    virtual bool tryAcquireLease(const std::string& node_id,
                                  std::chrono::milliseconds ttl) = 0;

    /// Return the current lease snapshot.
    virtual LeaderLease getCurrentLease() const = 0;

    /**
     * @brief Voluntarily release the lease held by `node_id`.
     *
     * A no-op when `node_id` does not currently hold the lease.
     */
    virtual void revokeLease(const std::string& node_id) = 0;
};

// ============================================================================
// InProcessLeaderElection — in-process TTL-based leader election
// ============================================================================

/**
 * @brief Default `ILeaderElection` implementation for single-process deployments.
 *
 * The lease is stored in a `LeaderLease` struct protected by a mutex.  A node
 * acquires the lease when:
 *   - No current lease exists, OR
 *   - The existing lease has expired, OR
 *   - The requesting node already holds the lease (renewal path).
 *
 * Thread-safety: fully thread-safe.
 */
class InProcessLeaderElection : public ILeaderElection {
public:
    bool tryAcquireLease(const std::string& node_id,
                          std::chrono::milliseconds ttl) override;

    LeaderLease getCurrentLease() const override;

    void revokeLease(const std::string& node_id) override;

private:
    mutable std::mutex mutex_;
    LeaderLease current_lease_;
    uint64_t epoch_ = 0;
};

// ============================================================================
// ConsistentHashRing — maps source_ids to worker nodes
// ============================================================================

/**
 * @brief Consistent hash ring for distributing source identifiers across nodes.
 *
 * Each physical node is represented by `virtual_nodes_per_node` virtual points
 * on a 64-bit ring to achieve a more uniform distribution.
 *
 * Thread-safety: NOT thread-safe; callers are responsible for external locking
 * if nodes are added/removed concurrently.
 */
class ConsistentHashRing {
public:
    static constexpr size_t kDefaultVirtualNodes = 150;

    explicit ConsistentHashRing(size_t virtual_nodes_per_node = kDefaultVirtualNodes);

    /** @brief Add a node to the ring. */
    void addNode(const std::string& node_id);

    /** @brief Remove a node from the ring (no-op if node is not present). */
    void removeNode(const std::string& node_id);

    /**
     * @brief Look up the node responsible for `key`.
     * @return Node ID, or empty string when the ring is empty.
     */
    std::string getNode(const std::string& key) const;

    /** @return true when no nodes are registered. */
    bool empty() const { return ring_.empty(); }

    /** @return Number of distinct physical nodes registered. */
    size_t nodeCount() const { return node_ids_.size(); }

private:
    size_t virtual_nodes_per_node_;
    std::map<uint64_t, std::string> ring_;  ///< hash → node_id
    std::vector<std::string> node_ids_;     ///< distinct physical nodes

    uint64_t hashKey(const std::string& key) const;
};

// ============================================================================
// InProcessWorkerNode — in-process worker backed by IngestionManager
// ============================================================================

/**
 * @brief `IIngestionWorkerNode` implementation that uses `IngestionManager`.
 *
 * The node creates a fresh `IngestionManager` for each `ingest()` call,
 * registers the assigned sources, and returns the resulting partial report.
 *
 * Thread-safety: `ingest()` is thread-safe; concurrent calls are serialised
 * by the `busy_` flag — if the node is already busy the call blocks until
 * the previous run completes (the coordinator never double-dispatches to a
 * single node, so in practice this lock is uncontended).
 */
class InProcessWorkerNode : public IIngestionWorkerNode {
public:
    /**
     * @brief Construct a worker node.
     *
     * @param node_id        Unique identifier for this node
     * @param db_connection  Database connection string passed to IngestionManager
     */
    InProcessWorkerNode(const std::string& node_id,
                        const std::string& db_connection);

    const std::string& nodeId() const override { return node_id_; }

    IngestionReport ingest(
        const std::vector<SourceConfig>& sources,
        const std::string& target_collection,
        ProgressCallback progress_callback) override;

    bool isAvailable() const override;

private:
    std::string node_id_;
    std::string db_connection_;
    mutable std::mutex busy_mutex_;
    std::atomic<bool> busy_{false};
};

// ============================================================================
// IngestionCoordinatorConfig — coordinator configuration (free-standing struct)
// ============================================================================

/**
 * @brief Configuration for IngestionCoordinator.
 *
 * Defined outside the class to allow its use as a default argument in
 * IngestionCoordinator's constructor.
 */
struct IngestionCoordinatorConfig {
    /// Number of in-process worker nodes (0 = hardware_concurrency / 2, min 1).
    size_t num_nodes = 0;
    /// Leader lease TTL. The lease is renewed every `lease_ttl / 2`.
    std::chrono::milliseconds lease_ttl{5000};
    /// Per-worker ingest call timeout. Coordinator fails a worker on expiry.
    std::chrono::seconds worker_timeout{120};
    /// Target collection for ingested documents.
    std::string target_collection = "legal_documents";
    /// Database connection string passed to each worker's IngestionManager.
    std::string db_connection;

    IngestionCoordinatorConfig() = default;
};

// ============================================================================
// IngestionCoordinator — leader-based distributed ingestion orchestrator
// ============================================================================

/**
 * @brief Distributed ingestion coordinator using work-stealing across nodes.
 *
 * The coordinator acts as the **leader** in a group of worker nodes:
 *   1. It acquires a TTL-based leader lease to prevent split-brain.
 *   2. It partitions a set of `SourceConfig` objects across worker nodes using
 *      a consistent hash ring keyed on `source_id`.
 *   3. It dispatches each partition to the owning node concurrently
 *      (work-stealing: idle nodes may pick up tasks from busy nodes).
 *   4. It aggregates the partial `IngestionReport` results into one report.
 *
 * ### In-process usage
 * @code
 * IngestionCoordinatorConfig cfg;
 * cfg.num_nodes          = 4;
 * cfg.db_connection      = "my_db";
 * cfg.target_collection  = "legal_documents";
 *
 * IngestionCoordinator coordinator(cfg);
 * coordinator.start();
 *
 * std::vector<SourceConfig> sources = { ... };
 * auto report = coordinator.ingestAll(sources);
 * coordinator.stop();
 * @endcode
 *
 * ### Using the IngestionBuilder
 * @code
 * auto mgr = IngestionBuilder("my_db")
 *     .withFilesystemSource("fs1", "/data/docs1")
 *     .withFilesystemSource("fs2", "/data/docs2")
 *     .build();
 *
 * IngestionCoordinator coordinator({.num_nodes=2, .db_connection="my_db"});
 * coordinator.start();
 * auto sources = mgr->getRegisteredSources();
 * auto report  = coordinator.ingestAll(sources);
 * coordinator.stop();
 * @endcode
 */
class IngestionCoordinator {
public:
    /// Convenience alias.
    using Config = IngestionCoordinatorConfig;

    /**
     * @brief Per-run coordinator metrics.
     */
    struct CoordinatorMetrics {
        size_t nodes_active      = 0;  ///< Active worker nodes at last run
        size_t tasks_submitted   = 0;  ///< Source partitions dispatched
        size_t tasks_completed   = 0;  ///< Partitions completed (incl. empties)
        size_t leader_elections  = 0;  ///< Times leader lease was acquired
        double last_run_seconds  = 0.0;///< Wall-clock time of the most recent run
    };

    /**
     * @brief Construct a coordinator with the given configuration.
     *
     * Does not start worker threads; call `start()` explicitly.
     */
    explicit IngestionCoordinator(const IngestionCoordinatorConfig& config =
                                      IngestionCoordinatorConfig{});

    ~IngestionCoordinator();

    // Non-copyable, non-movable (owns threads)
    IngestionCoordinator(const IngestionCoordinator&) = delete;
    IngestionCoordinator& operator=(const IngestionCoordinator&) = delete;

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @brief Start the coordinator and all worker nodes.
     *
     * Idempotent: safe to call on an already-running coordinator.
     */
    void start();

    /**
     * @brief Gracefully stop the coordinator and wait for in-flight work.
     *
     * Revokes the leader lease so that another node may take over immediately.
     */
    void stop();

    /** @return true after `start()` and before `stop()`. */
    bool isRunning() const { return running_.load(); }

    // ── Node management ──────────────────────────────────────────────────────

    /**
     * @brief Register an external or mock worker node.
     *
     * The node is added to the consistent hash ring and will receive source
     * assignments on the next `ingestAll()` call.
     *
     * @param node  Worker node implementing `IIngestionWorkerNode`
     */
    void registerNode(std::shared_ptr<IIngestionWorkerNode> node);

    /**
     * @brief Return descriptive information for all registered nodes.
     */
    std::vector<NodeInfo> getNodes() const;

    /**
     * @brief Return the node ID that currently holds the leader lease.
     *
     * @return Non-empty string when a valid lease exists; empty otherwise.
     */
    std::string getLeaderNodeId() const;

    // ── Source partitioning ──────────────────────────────────────────────────

    /**
     * @brief Partition sources across worker nodes using consistent hashing.
     *
     * The `source_id` of each `SourceConfig` is hashed onto the ring.  Sources
     * that hash to the same node are grouped together.
     *
     * @param sources  Sources to distribute
     * @return Map of `node_id` → sources assigned to that node.
     *         If the ring is empty, all sources are grouped under an empty key.
     */
    std::unordered_map<std::string, std::vector<SourceConfig>>
    partitionSources(const std::vector<SourceConfig>& sources) const;

    // ── Distributed ingestion ────────────────────────────────────────────────

    /**
     * @brief Run distributed ingestion and return an aggregated report.
     *
     * Steps:
     *   1. Acquire (or renew) the leader lease — returns an empty report if
     *      this node cannot become leader within the configured TTL.
     *   2. Partition `sources` across worker nodes via `partitionSources()`.
     *   3. Dispatch each partition to the owning node concurrently.
     *   4. Aggregate partial reports and return the combined result.
     *
     * @param sources            Sources to ingest
     * @param progress_callback  Optional callback forwarded to each worker node
     * @return Aggregated `IngestionReport`
     */
    IngestionReport ingestAll(
        const std::vector<SourceConfig>& sources,
        ProgressCallback progress_callback = nullptr);

    // ── Metrics ──────────────────────────────────────────────────────────────

    /** @return Snapshot of coordinator runtime metrics. */
    CoordinatorMetrics getMetrics() const;

    // ── Testing hooks ────────────────────────────────────────────────────────

    /**
     * @brief Inject a custom leader election backend (testing / simulation only).
     *
     * Must be called before `start()`.
     */
    void setLeaderElectionForTesting(std::shared_ptr<ILeaderElection> election);

private:
    Config config_;
    std::atomic<bool> running_{false};
    std::string my_node_id_;  ///< Coordinator node ID used in leader election

    // Nodes and ring
    mutable std::mutex nodes_mutex_;
    std::vector<std::shared_ptr<IIngestionWorkerNode>> nodes_;
    ConsistentHashRing hash_ring_;

    // Leader election
    std::shared_ptr<ILeaderElection> leader_election_;

    // Metrics
    mutable std::mutex metrics_mutex_;
    CoordinatorMetrics metrics_;

    // Lease renewal background thread
    std::thread lease_renewal_thread_;
    std::atomic<bool> lease_renewal_running_{false};
    std::mutex lease_renewal_cv_mutex_;
    std::condition_variable lease_renewal_cv_;

    void leaseRenewalLoop();
    IngestionReport aggregateReports(
        const std::vector<IngestionReport>& partial) const;
};

} // namespace ingestion
} // namespace themis
