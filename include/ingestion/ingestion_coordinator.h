/**
 * @file ingestion_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=0, Mock=2, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <deque>
#include <map>
#include <unordered_map>
#include <future>
#include <thread>

namespace themis {
namespace ingestion {

// ============================================================================
// ISharedCheckpointStore — pluggable shared-backend checkpoint interface
// ============================================================================

/**
 * @brief Strategy interface for a cluster-wide checkpoint store.
 *
 * All worker nodes in a distributed ingestion run share a single
 * `ISharedCheckpointStore` so that incremental progress is visible across
 * nodes and a failover worker can resume from the last committed offset.
 *
 * In production this is backed by Redis or the ThemisDB checkpoint
 * collection.  For single-process deployments the default
 * `InMemorySharedCheckpointStore` keeps state in a mutex-protected map
 * so that multiple in-process workers see each other's checkpoints.
 */
class ISharedCheckpointStore {
public:
    virtual ~ISharedCheckpointStore() = default;

    /**
     * @brief Atomically write (or overwrite) a checkpoint.
     * @return true on success
     */
    [[nodiscard]] virtual bool write(const IngestionCheckpoint& cp) = 0;

    /**
     * @brief Read the checkpoint for a source.
     * @param source_id  Source whose checkpoint to retrieve
     * @param out        Populated on success
     * @return true if a checkpoint was found and read successfully
     */
    [[nodiscard]] virtual bool read(const std::string& source_id,
                      IngestionCheckpoint& out) const = 0;

    /**
     * @brief Remove the checkpoint for a source.
     * @return true if the checkpoint existed and was deleted
     */
    [[nodiscard]] virtual bool clear(const std::string& source_id) = 0;

    /**
     * @brief Check whether a checkpoint exists for a source.
     */
    [[nodiscard]] virtual bool exists(const std::string& source_id) const = 0;
};

// ============================================================================
// InMemorySharedCheckpointStore — thread-safe in-process implementation
// ============================================================================

// STUB/SIMULATION NOTE:
// Purpose: Provide a fully functional ISharedCheckpointStore without requiring
//   an external store (Redis, DB) so that single-process and test scenarios
//   work out of the box.
// Activation: Default implementation used by IngestionCoordinator when no
//   external ISharedCheckpointStore is injected (e.g. via
//   setSharedCheckpointStore()).  Also the default in InProcessWorkerNode.
// Production Delta: State is process-local and is lost on restart.  No
//   cross-process coordination, no durable persistence, no TTL/expiry logic.
// Removal Plan: Not removed — retained for single-process deployments and
//   tests.  Multi-process / HA deployments must inject a Redis- or DB-backed
//   implementation via IngestionCoordinator::setSharedCheckpointStore().
// RESOLVED 2026-05-06 — public injection API `setSharedCheckpointStore(store)`
//   confirmed on IngestionCoordinator (throws std::logic_error when called while
//   running); getSharedCheckpointStore() accessor added; tests
//   IngestionCoordinatorCheckpointStoreTest::InjectedStoreIsUsed and
//   SetStoreWhileRunningThrows confirm injection semantics are correct.

/**
 * @brief `ISharedCheckpointStore` backed by a mutex-protected in-memory map.
 *
 * Suitable for single-process multi-worker deployments (e.g. tests and the
 * default `InProcessWorkerNode` mode).  Production deployments inject a Redis-
 * or database-backed implementation via
 * `IngestionCoordinator::setSharedCheckpointStoreForTesting()`.
 *
 * Thread-safety: fully thread-safe.
 */
class InMemorySharedCheckpointStore : public ISharedCheckpointStore {
public:
    bool write(const IngestionCheckpoint& cp) override;
    bool read(const std::string& source_id,
              IngestionCheckpoint& out) const override;
    bool clear(const std::string& source_id) override;
    bool exists(const std::string& source_id) const override;

    /** @return Number of checkpoints currently held in memory. */
    size_t size() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, IngestionCheckpoint> store_;
};

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
    [[nodiscard]] virtual const std::string& nodeId() const = 0;

    /**
     * @brief Ingest the assigned set of sources and return a partial report.
     *
     * @param sources            Sources assigned to this node
     * @param target_collection  Collection to write ingested documents into
     * @param progress_callback  Optional progress callback forwarded to the
     *                           underlying IngestionManager
     * @return Partial IngestionReport that will be aggregated by the coordinator
     */
    [[nodiscard]] virtual IngestionReport ingest(
        const std::vector<SourceConfig>& sources,
        const std::string& target_collection,
        ProgressCallback progress_callback) = 0;

    /// Returns true when the node is idle and ready to accept work.
    [[nodiscard]] virtual bool isAvailable() const = 0;
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
    [[nodiscard]] virtual bool tryAcquireLease(const std::string& node_id,
                                  std::chrono::milliseconds ttl) = 0;

    /// Return the current lease snapshot.
    [[nodiscard]] virtual LeaderLease getCurrentLease() const = 0;

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
// WorkStealingPool — per-source task pool with work stealing
// ============================================================================

/**
 * @brief Work-stealing thread pool for per-source ingestion tasks.
 *
 * Sources are initially assigned to workers via consistent hashing.  When a
 * worker exhausts its local deque, it steals individual sources from the back
 * of another worker's deque, providing automatic load balancing when
 * partitions are uneven.
 *
 * ### Lifecycle
 * 1. Construct with the set of worker nodes.
 * 2. Call `submitTo(worker_idx, source)` for each source.
 * 3. Call `run(callback)` — starts one thread per worker, returns when all
 *    sources are processed.
 * 4. Aggregate the returned partial reports.
 *
 * Thread-safety: `submitTo()` is NOT thread-safe; call it before `run()`.
 *                `run()` is safe to call exactly once per instance.
 */
class WorkStealingPool {
public:
    /**
     * @brief Construct a work-stealing pool.
     *
     * @param nodes            Worker nodes (one thread per node).
     * @param target_collection Collection written to by each worker.
     * @param worker_timeout   Per-source time limit (0 = disabled).
     */
    WorkStealingPool(
        std::vector<std::shared_ptr<IIngestionWorkerNode>> nodes,
        std::string target_collection,
        std::chrono::seconds worker_timeout);

    /**
     * @brief Queue a source for a specific worker's initial deque.
     *
     * Must be called before `run()`.
     *
     * @param worker_idx  Index into the node list (must be < nodeCount()).
     * @param source      Source to enqueue.
     */
    void submitTo(size_t worker_idx, SourceConfig source);

    /** @return Number of nodes (and threads) in the pool. */
    size_t nodeCount() const { return nodes_.size(); }

    /** @return Total sources currently queued across all workers. */
    size_t pendingCount() const {
        return remaining_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Execute all queued sources with work stealing.
     *
     * Spawns one thread per worker node.  Each thread processes sources from
     * its own deque first; when empty it steals from the back of another
     * worker's deque.  Threads exit when no sources remain in any deque.
     *
     * @param cb  Optional progress callback forwarded to each worker.
     * @return    One `IngestionReport` per processed source (order unspecified).
     */
    std::vector<IngestionReport> run(ProgressCallback cb);

private:
    std::vector<std::shared_ptr<IIngestionWorkerNode>> nodes_;
    std::string target_collection_;
    std::chrono::seconds worker_timeout_;

    /// Per-worker task deque (padded struct avoids false sharing on x86-64).
    struct WorkerDeque {
        std::deque<SourceConfig> tasks;
        std::mutex mtx;
    };
    std::vector<WorkerDeque> deques_;

    /// Shared result buffer — written by worker threads, read after join.
    std::mutex results_mtx_;
    std::vector<IngestionReport> results_;

    /// Count of sources still sitting in deques (decremented when stolen/popped).
    std::atomic<size_t> remaining_{0};

    /// Worker thread body.
    void workerFn(size_t my_idx, ProgressCallback cb);

    /// Pop a source from this worker's own deque (front).
    bool tryPopOwn(size_t idx, SourceConfig& out);

    /// Steal a source from another worker's deque (back).
    bool trySteal(size_t thief_idx, SourceConfig& out);
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

    // ── Checkpoint store configuration ──────────────────────────────────────

    /**
     * @brief Replace the shared checkpoint store.
     *
     * Must be called before `start()`.  The default store is an
     * `InMemorySharedCheckpointStore`, which is sufficient for single-process
     * deployments.  Multi-host deployments should provide a Redis- or
     * database-backed implementation before starting the coordinator.
     *
     * @throws std::logic_error if called while the coordinator is running.
     */
    void setSharedCheckpointStore(std::shared_ptr<ISharedCheckpointStore> store);

    /**
     * @brief Return the shared checkpoint store currently in use.
     *
     * Useful for test assertions (e.g. verifying a checkpoint was written
     * after ingestion).
     */
    std::shared_ptr<ISharedCheckpointStore> getSharedCheckpointStore() const;

    // ── Testing hooks ────────────────────────────────────────────────────────

    /**
     * @brief Inject a custom leader election backend (testing / simulation only).
     *
     * Must be called before `start()`.
     */
    void setLeaderElectionForTesting(std::shared_ptr<ILeaderElection> election);

    /**
     * @brief Alias for `setSharedCheckpointStore()` — test code only.
     *
     * Prefer `setSharedCheckpointStore()` in production.  This alias is kept
     * so that existing test code continues to compile.
     *
     * @throws std::logic_error if called while the coordinator is running.
     */
    void setSharedCheckpointStoreForTesting(
        std::shared_ptr<ISharedCheckpointStore> store);

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

    // Shared checkpoint store (visible to all worker nodes)
    std::shared_ptr<ISharedCheckpointStore> checkpoint_store_;

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
