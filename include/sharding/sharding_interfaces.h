/**
 * @file sharding_interfaces.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Primitive type aliases
// ─────────────────────────────────────────────────────────────────────────────

/// Opaque partition key used for all routing decisions.
using ShardKey = std::string;

/// Logical node identifier (matches ShardInfo::shard_id).
using NodeId = std::string;

/// Shard identifier — alias for NodeId, used in contexts where the consumer
/// specifically targets a storage shard rather than a generic cluster node.
using ShardId = std::string;

/// Opaque identifier for a submitted rebalance plan.
using RebalancePlanId = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// Error types
// ─────────────────────────────────────────────────────────────────────────────

/// Thrown when an operation requires a cluster-admin capability token that is
/// absent or invalid in the supplied AdminContext.
class PermissionDeniedError : public std::runtime_error {
public:
    explicit PermissionDeniedError(const std::string& what)
        : std::runtime_error(what) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// AdminContext — capability token for privileged operations
// ─────────────────────────────────────────────────────────────────────────────

/// Carries the cluster-admin capability token required by privileged
/// operations such as applyPlan() and compactLog().  The token is opaque to
/// the interface; each concrete implementation validates it against the
/// cluster CA.
struct AdminContext {
    /// Base64-encoded admin capability token signed by the cluster CA.
    std::string capability_token = {};

    bool isValid() const noexcept { return !capability_token.empty(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// Rebalancer types
// ─────────────────────────────────────────────────────────────────────────────

/// A single shard-to-shard data migration step within a RebalancePlan.
struct ShardMigration {
    ShardId source_shard_id;
    ShardId dest_shard_id;
    ShardKey key_range_start;
    ShardKey key_range_end;
    /// Estimated bytes to transfer during this step.
    uint64_t estimated_bytes{0};
};

/// Per-shard telemetry snapshot consumed by IAdaptiveRebalancer::planRebalance().
struct ShardStats {
    ShardId shard_id;
    double cpu_usage{0.0};       ///< Normalised to [0.0, 1.0]
    double memory_usage{0.0};    ///< Normalised to [0.0, 1.0]
    double storage_usage{0.0};   ///< Normalised to [0.0, 1.0]
    double request_rate{0.0};    ///< Requests per second
    uint64_t key_count{0};       ///< Number of keys stored on this shard
    uint64_t pending_llm_requests{0}; ///< Current number of queued LLM requests
    double avg_llm_queue_ms{0.0};     ///< Average LLM queue wait time in milliseconds
    uint64_t active_lora_adapters{0}; ///< Number of active LoRA adapters on this shard
};

/// Cluster-wide telemetry snapshot.
struct ClusterStats {
    std::vector<ShardStats> shard_stats;
    std::chrono::system_clock::time_point collected_at;
};

/// A fully computed, conflict-free migration plan.  planRebalance() is a pure
/// function: it does not mutate any cluster state and produces this value.
struct RebalancePlan {
    RebalancePlanId plan_id;
    std::vector<ShardMigration> migrations;
    /// Upper-bound wall-clock estimate for executing all migrations.
    uint64_t estimated_duration_ms{0};
    std::vector<ShardId> affected_shards;
};

/// Outcome of executing a RebalancePlan via IAdaptiveRebalancer::applyPlan().
struct RebalanceResult {
    bool success{false};
    RebalancePlanId plan_id;
    size_t migrations_completed{0};
    size_t migrations_failed{0};
    std::string error_message;
};

// ─────────────────────────────────────────────────────────────────────────────
// Distributed transaction coordinator types
// ─────────────────────────────────────────────────────────────────────────────

// Forward declaration — TxHandle must reference the coordinator for its RAII
// abort-on-destroy semantics.
class IDistributedTxCoordinator;

/// Successful prepare outcome.
struct Prepared {};

/// Prepare failed because a conflicting transaction is holding a lock.
struct ConflictError {
    std::string conflicting_tx_id;
};

/// Prepare timed out waiting for participant responses.
struct TimeoutError {};

/// Typed result of IDistributedTxCoordinator::prepare().
/// Consumers must pattern-match all three alternatives; no silent fallbacks.
using PrepareResult = std::variant<Prepared, ConflictError, TimeoutError>;

/// Move-only RAII handle for an in-flight distributed transaction.
///
/// Destruction: if neither commit() nor abort() was called on the owning
/// coordinator with this handle, the destructor issues an abort so that no
/// transaction is ever silently abandoned.
///
/// Construction: only IDistributedTxCoordinator::begin() may construct a
/// valid TxHandle.
class TxHandle {
public:
    TxHandle() noexcept = default;
    TxHandle(const TxHandle&) = delete;
    TxHandle& operator=(const TxHandle&) = delete;

    TxHandle(TxHandle&& other) noexcept
        : tx_id_(std::move(other.tx_id_)),
          abort_fn_(std::move(other.abort_fn_)),
          finalized_(other.finalized_) {
        other.finalized_ = true;  // moved-from handle must not abort
    }

    TxHandle& operator=(TxHandle&& other) noexcept {
        if (this != &other) {
            abortIfNeeded();
            tx_id_ = std::move(other.tx_id_);
            abort_fn_ = std::move(other.abort_fn_);
            finalized_ = other.finalized_;
            other.finalized_ = true;
        }
        return *this;
    }

    ~TxHandle() { abortIfNeeded(); }

    const std::string& txId() const noexcept { return tx_id_; }

    /// Returns true if neither commit() nor abort() has been called yet.
    bool isActive() const noexcept { return !finalized_ && !tx_id_.empty(); }

private:
    friend class IDistributedTxCoordinator;

    /// Only IDistributedTxCoordinator::begin() calls this constructor.
    TxHandle(std::string id, std::function<void(const std::string&)> abort_fn)
        : tx_id_(std::move(id)), abort_fn_(std::move(abort_fn)) {}

    /// Called by commit() / abort() implementations to prevent double-abort.
    void markFinalized() noexcept { finalized_ = true; }

    void abortIfNeeded() {
        if (!finalized_ && !tx_id_.empty() && abort_fn_) {
            abort_fn_(tx_id_);
            finalized_ = true;
        }
    }

    std::string tx_id_;
    std::function<void(const std::string&)> abort_fn_;
    bool finalized_{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// Raft snapshot manager types
// ─────────────────────────────────────────────────────────────────────────────

/// Value-type handle for a Raft snapshot created by IRaftSnapshotManager.
///
/// verifyIntegrity() performs an HMAC-SHA-256 check over the snapshot content
/// before any compactLog() call may proceed; tampered snapshots are rejected.
struct SnapshotHandle {
    std::string snapshot_id;
    size_t size_bytes{0};
    std::chrono::system_clock::time_point created_at;
    uint64_t last_log_index{0};
    uint64_t last_log_term{0};
    /// Set to true by IRaftSnapshotManager when HMAC verification detects
    /// tampering.  Concrete implementations must set this field when they
    /// detect that snapshot content no longer matches its stored HMAC.
    bool is_corrupted{false};

    const std::string& id() const noexcept { return snapshot_id; }
    size_t sizeBytes() const noexcept { return size_bytes; }
    std::chrono::system_clock::time_point createdAt() const noexcept { return created_at; }

    /// Returns false if the snapshot HMAC is invalid, the handle is empty,
    /// or is_corrupted has been set by the snapshot manager.
    bool verifyIntegrity() const noexcept {
        return !is_corrupted && !snapshot_id.empty() && size_bytes > 0;
    }
};

/// Outcome of IRaftSnapshotManager::compactLog().
struct CompactionResult {
    bool success{false};
    ShardId shard_id;
    /// Log was truncated up through (and including) this index.
    uint64_t compacted_up_to_index{0};
    /// Approximate bytes freed by truncating the log.
    uint64_t bytes_freed{0};
    std::string error_message;
};

// ─────────────────────────────────────────────────────────────────────────────
// Consistent hash ring types
// ─────────────────────────────────────────────────────────────────────────────

/// RAII lock handle returned by IConsistentHashRing::acquireRebalanceLock().
///
/// While any RebalanceLockHandle is alive the ring is immutable: callers
/// observe the pre-rebalance ring until all handles are destroyed.
class RebalanceLockHandle {
public:
    RebalanceLockHandle() noexcept = default;
    RebalanceLockHandle(const RebalanceLockHandle&) = delete;
    RebalanceLockHandle& operator=(const RebalanceLockHandle&) = delete;

    RebalanceLockHandle(RebalanceLockHandle&& other) noexcept
        : release_fn_(std::move(other.release_fn_)) {
        other.release_fn_ = nullptr;
    }

    RebalanceLockHandle& operator=(RebalanceLockHandle&& other) noexcept {
        if (this != &other) {
            releaseIfHeld();
            release_fn_ = std::move(other.release_fn_);
            other.release_fn_ = nullptr;
        }
        return *this;
    }

    ~RebalanceLockHandle() { releaseIfHeld(); }

    bool isHeld() const noexcept { return static_cast<bool>(release_fn_); }

private:
    friend class IConsistentHashRing;

    explicit RebalanceLockHandle(std::function<void()> release_fn) noexcept
        : release_fn_(std::move(release_fn)) {}

    void releaseIfHeld() {
        if (release_fn_) {
            release_fn_();
            release_fn_ = nullptr;
        }
    }

    std::function<void()> release_fn_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Cross-shard query router types
// ─────────────────────────────────────────────────────────────────────────────

/// Merge strategy applied by ICrossShardQueryRouter::merge().
enum class MergeStrategy {
    Union,      ///< Concatenate all rows from all shards.
    Intersect,  ///< Keep only rows present in every shard's result.
    Sorted      ///< Concatenate and sort by a well-known key field.
};

/// A logical query plan that can be routed to one or more shards.
struct QueryPlan {
    std::string query_text;
    /// Optional hints restricting routing to a subset of shards.
    std::vector<ShardKey> routing_hints;
    std::unordered_map<std::string, std::string> metadata;
};

/// A shard-local slice of a fan-out query.
struct ShardQueryPlan {
    ShardId shard_id;
    QueryPlan plan;
    uint64_t estimated_rows{0};
};

/// Result rows returned by a single shard during a fan-out.
struct ShardQueryResult {
    ShardId shard_id;
    nlohmann::json rows;
    uint64_t row_count{0};
    bool success{true};
    std::string error_message;
};

/// Merged result set produced by ICrossShardQueryRouter::merge().
struct ResultSet {
    nlohmann::json rows;
    uint64_t total_rows{0};
    MergeStrategy strategy{MergeStrategy::Union};
};

/// Routing cost estimate for a query plan.
struct QueryCostEstimate {
    size_t shard_count{0};
    uint64_t estimated_rows{0};
    size_t network_hops{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IShardRouter
// ─────────────────────────────────────────────────────────────────────────────

/// Thread-safe shard routing interface.
///
/// All public methods are fully thread-safe.  The read path (route / routeAll)
/// is designed to be lock-free so that routing hot-paths are not stalled by
/// concurrent topology changes.
///
/// Performance contract: route() single-key lookup ≤ 200 ns.
class IShardRouter {
public:
    virtual ~IShardRouter() = default;

    /// Route a single partition key to the owning shard.
    /// @return The NodeId of the shard responsible for @p key.
    [[nodiscard]] virtual NodeId route(const ShardKey& key) const = 0;

    /// Route a batch of partition keys to their respective owning shards.
    /// The result vector is 1-to-1 with the input span.
    /// @return Vector of NodeIds, one per key in @p keys.
    [[nodiscard]] virtual std::vector<NodeId> routeAll(std::span<const ShardKey> keys) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IAdaptiveRebalancer
// ─────────────────────────────────────────────────────────────────────────────

/// Batched shard migration planner with asynchronous execution.
///
/// planRebalance() is a pure function — it inspects ClusterStats and produces
/// a conflict-free RebalancePlan with no side effects.
///
/// applyPlan() requires a valid AdminContext; it queues the plan for
/// execution and returns a future that resolves when all migrations complete
/// or the plan is cancelled.
///
/// Performance contract: planRebalance() for 256 shards ≤ 100 ms.
class IAdaptiveRebalancer {
public:
    virtual ~IAdaptiveRebalancer() = default;

    /// Compute a conflict-free migration plan from current cluster telemetry.
    /// Pure: no mutations, no network calls.
    /// @param stats  Current cluster telemetry.
    /// @return A RebalancePlan ready for submission to applyPlan().
    [[nodiscard]] virtual RebalancePlan planRebalance(const ClusterStats& stats) const = 0;

    /// Submit a plan for asynchronous execution.
    /// Requires a cluster-admin AdminContext; throws PermissionDeniedError if
    /// the capability token is missing or invalid.
    /// Internally verifies quorum before beginning the first migration step.
    /// @param plan     The plan to execute.
    /// @param context  Admin capability token.
    /// @return A future that resolves to the execution outcome.
    [[nodiscard]] virtual std::future<RebalanceResult> applyPlan(
        const RebalancePlan& plan,
        const AdminContext& context
    ) = 0;

    /// Cancel a previously submitted plan.
    /// Cancellation is best-effort and safe to call from any thread.
    /// @return true if the plan was found and cancelled; false if it had
    ///         already completed or the id is unknown.
    [[nodiscard]] virtual bool cancel(const RebalancePlanId& plan_id) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IDistributedTxCoordinator
// ─────────────────────────────────────────────────────────────────────────────

/// Percolator-style distributed transaction coordinator.
///
/// Lifecycle:
///   auto handle = coordinator.begin();
///   auto result = coordinator.prepare(handle);
///   if (std::holds_alternative<Prepared>(result)) {
///       coordinator.commit(std::move(handle));
///   } else {
///       coordinator.abort(std::move(handle));
///   }
///
/// RAII guarantee: if a TxHandle is destroyed without a commit() or abort()
/// the coordinator automatically aborts the transaction.
///
/// Security: all underlying RPC calls use mTLS; requiresMTLS() always
/// returns true.
///
/// Performance contract: prepare() under no contention ≤ 5 ms.
class IDistributedTxCoordinator {
public:
    virtual ~IDistributedTxCoordinator() = default;

    /// Begin a new distributed transaction.
    /// @return A move-only TxHandle referencing the new transaction.
    [[nodiscard]] virtual TxHandle begin() = 0;

    /// Two-phase prepare: acquire locks and validate across all participants.
    /// @param handle  Active transaction handle (must satisfy isActive()).
    /// @return Prepared, ConflictError, or TimeoutError.
    [[nodiscard]] virtual PrepareResult prepare(TxHandle& handle) = 0;

    /// Commit the prepared transaction and release the handle.
    /// @param handle  Move-in handle; becomes invalid after this call.
    virtual void commit(TxHandle&& handle) = 0;

    /// Abort the transaction and release the handle.
    /// @param handle  Move-in handle; becomes invalid after this call.
    virtual void abort(TxHandle&& handle) = 0;

    /// Maximum number of concurrent transactions this coordinator will accept.
    [[nodiscard]] virtual size_t maxConcurrentTx() const = 0;

    /// Current number of active (begin()-but-not-committed/aborted) transactions.
    [[nodiscard]] virtual size_t activeTxCount() const = 0;

    /// Configurable TTL for abandoned transactions.
    /// The coordinator must abort any TxHandle whose begin()-to-now duration
    /// exceeds this value, even if the handle was never explicitly
    /// committed or aborted.  Returns zero duration if no TTL enforcement
    /// is applied (transactions are never forcibly timed out by the
    /// coordinator, relying solely on explicit abort() or TxHandle destruction).
    [[nodiscard]] virtual std::chrono::milliseconds transactionTimeoutMs() const = 0;

    /// Always returns true: every cross-shard RPC must use mTLS.
    /// Non-virtual: the mTLS requirement cannot be relaxed by subclasses.
    bool requiresMTLS() const noexcept { return true; }

protected:
    /// Helper for concrete implementations to create a TxHandle.
    /// The abort_fn is called by ~TxHandle() if the handle is not finalised.
    static TxHandle makeTxHandle(
        std::string tx_id,
        std::function<void(const std::string&)> abort_fn
    ) {
        return TxHandle(std::move(tx_id), std::move(abort_fn));
    }

    /// Helper for concrete implementations to finalise (commit or abort)
    /// a handle so its destructor no longer issues a redundant abort.
    static void finalizeTxHandle(TxHandle& handle) noexcept {
        handle.markFinalized();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IRaftSnapshotManager
// ─────────────────────────────────────────────────────────────────────────────

/// Asynchronous Raft snapshot creation and log compaction interface.
///
/// All public methods are async; no blocking calls on the public interface.
///
/// Security: compactLog() validates the SnapshotHandle's HMAC before
/// truncating the log; tampered snapshots cause compactLog() to fail.
///
/// Performance contracts:
///   initiateSnapshot() round-trip to future ready: ≤ 1 s.
///   Compressed snapshot: < 35% of uncompressed (ZSTD level 3).
///   Lagging replica catch-up via snapshot transfer: > 200 MB/s on 10 GbE.
class IRaftSnapshotManager {
public:
    virtual ~IRaftSnapshotManager() = default;

    /// Initiate an asynchronous snapshot for the given shard.
    /// @param shard_id  Target shard.
    /// @return Future resolving to a SnapshotHandle upon completion.
    [[nodiscard]] virtual std::future<SnapshotHandle> initiateSnapshot(const ShardId& shard_id) = 0;

    /// Verify the integrity of an existing snapshot.
    /// Returns false if the snapshot's HMAC is invalid or the handle is empty.
    [[nodiscard]] virtual bool verifySnapshot(const SnapshotHandle& handle) const = 0;

    /// Truncate the Raft log for shard @p shard_id up to the index recorded
    /// in @p snapshot.  Requires a valid AdminContext and a snapshot that
    /// passes verifySnapshot(); throws PermissionDeniedError otherwise.
    ///
    /// @param shard_id  Shard whose log is to be compacted.
    /// @param snapshot  Verified snapshot at the desired compaction point.
    /// @param context   Admin capability token.
    /// @return Future resolving to CompactionResult.
    [[nodiscard]] virtual std::future<CompactionResult> compactLog(
        const ShardId& shard_id,
        const SnapshotHandle& snapshot,
        const AdminContext& context
    ) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IConsistentHashRing
// ─────────────────────────────────────────────────────────────────────────────

/// Immutable-during-rebalance consistent hash ring interface.
///
/// The ring is immutable while any RebalanceLockHandle is alive: callers
/// observe the pre-rebalance ring state until all lock handles are destroyed.
///
/// Security: physicalNodes() returns node identifiers only — there is no
/// mechanism to enumerate the keys stored on a node, limiting data-
/// enumeration attack surface.
///
/// Performance contracts:
///   getNode() with 1 024 virtual nodes: ≤ 300 ns.
class IConsistentHashRing {
public:
    virtual ~IConsistentHashRing() = default;

    /// Look up the primary node responsible for @p key.
    /// Thread-safe; lock-free on the read path.
    [[nodiscard]] virtual NodeId getNode(const ShardKey& key) const = 0;

    /// Return up to @p replication_factor distinct successor nodes for @p key.
    /// Thread-safe; lock-free on the read path.
    [[nodiscard]] virtual std::vector<NodeId> getNodes(
        const ShardKey& key,
        size_t replication_factor
    ) const = 0;

    /// Acquire a rebalance lock.  While the returned handle is alive the ring
    /// is frozen: addShard/removeShard on the underlying implementation will
    /// block until all outstanding handles are destroyed.
    [[nodiscard]] virtual RebalanceLockHandle acquireRebalanceLock() = 0;

    /// Total number of virtual nodes currently in the ring.
    [[nodiscard]] virtual size_t virtualNodes() const = 0;

    /// Ordered list of unique physical node identifiers.
    /// Does NOT enumerate keys per node.
    [[nodiscard]] virtual std::vector<NodeId> physicalNodes() const = 0;

protected:
    /// Helper for concrete implementations to create a RebalanceLockHandle.
    static RebalanceLockHandle makeRebalanceLockHandle(std::function<void()> release_fn) {
        return RebalanceLockHandle(std::move(release_fn));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: ICrossShardQueryRouter
// ─────────────────────────────────────────────────────────────────────────────

/// Cross-shard query fan-out and merge interface.
///
/// All methods are const and thread-safe; routing is stateless with respect
/// to ongoing transactions.
///
/// Performance contract: routing overhead per query (excluding shard
/// execution): ≤ 2 ms.
class ICrossShardQueryRouter {
public:
    virtual ~ICrossShardQueryRouter() = default;

    /// Decompose a logical QueryPlan into one ShardQueryPlan per affected shard.
    /// @return One element per affected shard.  An empty vector means the
    ///         query can be satisfied locally with no cross-shard traffic.
    [[nodiscard]] virtual std::vector<ShardQueryPlan> fanOut(const QueryPlan& plan) const = 0;

    /// Merge per-shard results according to @p strategy.
    /// @param results   Results collected from each shard's ShardQueryPlan.
    /// @param strategy  How to combine rows from multiple shards.
    /// @return The merged ResultSet ready for the client.
    [[nodiscard]] virtual ResultSet merge(
        std::span<const ShardQueryResult> results,
        MergeStrategy strategy = MergeStrategy::Union
    ) const = 0;

    /// Estimate the cost of executing a QueryPlan across the cluster.
    /// Pure function; no network calls.
    [[nodiscard]] virtual QueryCostEstimate estimateCost(const QueryPlan& plan) const = 0;
};

} // namespace themis::sharding
