/**
 * @file distributed_transaction_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// DistributedTransactionManager — 2PC across multiple storage shards
//
// Implements a Two-Phase Commit (2PC) coordinator for ACID distributed
// transactions spanning multiple ThemisDB storage shards.  Each shard
// exposes an IDistributedShardParticipant interface; the coordinator drives
// PREPARE → COMMIT/ABORT in lock-step.
//
// This is the storage-layer distributed transaction facility introduced in
// v1.7.0 and is fully backward-compatible: existing single-node
// TransactionManager usage is unaffected.
//
// Usage:
// @code
//   DistributedTransactionManager dtx_mgr;
//   dtx_mgr.registerShard("shard1", &shard1Participant);
//   dtx_mgr.registerShard("shard2", &shard2Participant);
//
//   auto tx = dtx_mgr.beginDistributedTransaction();
//   tx->put("shard1:user:1", "alice");
//   tx->put("shard2:user:1", "alice");
//   bool ok = tx->commit();  // 2PC across both shards
// @endcode

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// Operation types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single write operation buffered inside a DistributedTransaction.
 */
struct DistributedOperation {
    enum class Type { PUT, DELETE };

    Type        type  = Type::PUT;
    std::string shard_id;
    std::string key;
    std::string value;  ///< Empty for DELETE operations
};

// ─────────────────────────────────────────────────────────────────────────────
// Participant interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Interface every shard participant must implement for 2PC coordination.
 *
 * Concrete implementations may wrap an in-process MVCCStore or a remote RPC
 * stub (gRPC).  Returning false from prepare() votes ABORT for the entire
 * transaction.
 */
class IDistributedShardParticipant {
public:
    virtual ~IDistributedShardParticipant() = default;

    /**
     * @brief Phase 1 — lock rows and record a durable PREPARE log entry.
     *
     * @param txn_id  Globally unique transaction identifier.
     * @param ops     Operations destined for this shard.
     * @return true → vote COMMIT; false → vote ABORT.
     */
    [[nodiscard]] virtual bool prepare(
        const std::string&                       txn_id,
        const std::vector<DistributedOperation>& ops
    ) = 0;

    /**
     * @brief Phase 2 (commit path) — apply the prepared operations.
     *
     * Called only when every participant voted COMMIT.
     *
     * @param txn_id  Transaction to commit.
     */
    virtual void commit(const std::string& txn_id) = 0;

    /**
     * @brief Phase 2 (abort path) — discard prepared operations and release locks.
     *
     * @param txn_id  Transaction to abort.
     */
    virtual void abort(const std::string& txn_id) = 0;

    /**
     * @brief Optional point-in-time read of a single key.
     *
     * Used by DistributedTransaction::get() for cross-shard reads.
     * The default implementation returns std::nullopt (key not found).
     */
    [[nodiscard]] virtual std::optional<std::string> get(const std::string& /*key*/) {
        return std::nullopt;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Transaction state
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Lifecycle state of a distributed transaction. */
enum class DistributedTxnState {
    ACTIVE,     ///< Accepting put/get/del operations
    PREPARING,  ///< Phase 1 in progress
    COMMITTED,  ///< 2PC succeeded; all shards applied
    ABORTED,    ///< 2PC failed or explicit rollback
    FAILED      ///< Unrecoverable error
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Configuration for DistributedTransactionManager. */
struct DistributedTxnConfig {
    /// Key separator used to extract shard_id from "shard_id:key" notation.
    char shard_key_separator = ':';
};

// ─────────────────────────────────────────────────────────────────────────────
// ManagerSharedState — shared ownership between manager and live transactions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Internal state co-owned by DistributedTransactionManager and all
 *        active DistributedTransaction handles via std::shared_ptr.
 *
 * This ensures that a transaction can safely call rollback() in its destructor
 * even if the owning DistributedTransactionManager has already been destroyed.
 *
 * Participants are stored as std::shared_ptr with a no-op deleter, so the
 * caller retains ownership while still allowing safe reference-copying under
 * the lock to prevent use-after-free when a concurrent unregisterShard() races
 * with an in-flight prepare/commit/abort call.
 */
struct ManagerSharedState {
    mutable std::mutex shards_mutex;
    /// Best-effort statistics counters (all relaxed — approximate counts only).
    std::atomic<uint64_t> total_transactions{0};
    std::atomic<uint64_t> committed{0};
    std::atomic<uint64_t> aborted{0};
    std::atomic<uint64_t> active{0};

    std::atomic<uint64_t> txn_counter{0};

    /// Participants are stored as shared_ptr with a no-op deleter: the caller
    /// retains ownership of the underlying object, but copying the shared_ptr
    /// under the shards_mutex gives in-flight operations a stable reference
    /// that outlives a concurrent unregisterShard() call.
    std::map<std::string, std::shared_ptr<IDistributedShardParticipant>> shards;

    /// Monotonic per-shard registration versions (protected by shards_mutex).
    uint64_t next_shard_version{0};
    std::map<std::string, uint64_t> shard_versions;
};

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransaction — user-facing transaction handle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Handle returned by DistributedTransactionManager::beginDistributedTransaction().
 *
 * Operations are buffered locally and flushed to all participating shards
 * during 2PC commit.  Keys are expected in "shard_id:key" format; the
 * coordinator uses the configurable separator to route each operation to the
 * correct participant.
 *
 * Thread-safety: A DistributedTransaction must be used from a single thread.
 */
class DistributedTransaction {
public:
    ~DistributedTransaction();

    // Non-copyable, non-moveable
    DistributedTransaction(const DistributedTransaction&)            = delete;
    DistributedTransaction& operator=(const DistributedTransaction&) = delete;
    DistributedTransaction(DistributedTransaction&&)                 = delete;
    DistributedTransaction& operator=(DistributedTransaction&&)      = delete;

    /**
     * @brief Write a key-value pair to the appropriate shard.
     *
     * @param key   "shard_id:logical_key" — the prefix up to the first
     *              separator identifies the destination shard.
     * @param value Value bytes to store.
     * @throws std::invalid_argument if the transaction is not ACTIVE.
     * @throws std::invalid_argument if the shard_id is not registered.
     * @throws std::runtime_error if shard registration changes while active.
     */
    void put(std::string_view key, std::string_view value);

    /**
     * @brief Delete a key from the appropriate shard.
     *
     * @param key  "shard_id:logical_key".
     * @throws std::invalid_argument if the transaction is not ACTIVE.
     * @throws std::invalid_argument if the shard_id is not registered.
     * @throws std::runtime_error if shard registration changes while active.
     */
    void del(std::string_view key);

    /**
     * @brief Read a key from the appropriate shard (non-transactional snapshot).
     *
     * Reads are routed to the shard via IDistributedShardParticipant::get().
     * The read is NOT part of the write-set and will not be validated at commit.
     *
     * @param key  "shard_id:logical_key".
     * @return Value bytes, or std::nullopt if the key is not found in the shard.
     * @throws std::invalid_argument if the shard_id is not registered.
     * @throws std::runtime_error if shard registration changes while active.
     */
    std::optional<std::string> get(std::string_view key);

    /**
     * @brief Commit the transaction via two-phase commit.
     *
     * Runs Phase 1 (PREPARE to all participating shards) and then Phase 2
     * (COMMIT if all voted YES, ABORT otherwise).
     *
     * @return true if all shards committed successfully.
     */
    bool commit();

    /**
     * @brief Roll back the transaction without committing.
     *
     * Sends ABORT to any shards that have already received a PREPARE, then
     * marks the transaction ABORTED.  No-op if already committed or aborted.
     */
    void rollback();

    /** @return Current lifecycle state. */
    DistributedTxnState state() const { return state_; }

    /** @return Unique transaction identifier. */
    const std::string& id() const { return txn_id_; }

    /** @return Shards that have operations buffered in this transaction. */
    std::vector<std::string> participatingShards() const;

    /** @return Number of buffered write operations. */
    size_t operationCount() const;

    // ── Construction tag (use DistributedTransactionManager only) ─────────────

    /**
     * @brief Tag type that gates direct construction to the manager.
     *
     * Callers outside DistributedTransactionManager cannot construct
     * DistributedTransaction directly because they cannot create PrivateTag.
     */
    struct PrivateTag { explicit PrivateTag() = default; };

    DistributedTransaction(
        PrivateTag,
        std::string                         txn_id,
        char                                separator,
        std::shared_ptr<ManagerSharedState> state
    );

private:

    /// Parse "shard_id:logical_key" → (shard_id, logical_key)
    std::pair<std::string, std::string> parseKey(std::string_view composite) const;

    /// Look up participant/version under lock and return a reference-counted copy.
    /// Throws std::invalid_argument if shard is not registered.
    /// Throws std::runtime_error if shard version metadata is missing.
    std::pair<std::shared_ptr<IDistributedShardParticipant>, uint64_t>
    requireParticipant(const std::string& shard_id) const;

    std::string txn_id_;
    char        separator_;

    /// Shared ownership of coordinator state; safe to access after manager destruction.
    std::shared_ptr<ManagerSharedState> mgr_state_;

    DistributedTxnState state_ = DistributedTxnState::ACTIVE;

    /// Per-shard buffered operations (populated by put/del).
    std::map<std::string, std::vector<DistributedOperation>> pending_ops_;

    /// Shard references from Phase-1 PREPARE; stored as shared_ptr to outlive
    /// any concurrent unregisterShard() call during Phase-2 COMMIT/ABORT.
    std::vector<std::pair<std::string, std::shared_ptr<IDistributedShardParticipant>>>
        prepared_shards_;

    /// Expected shard registration versions captured while routing operations.
    std::map<std::string, uint64_t> expected_shard_versions_;
};

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransactionManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Coordinator for distributed storage transactions (v1.7.0).
 *
 * Manages shard registration and drives Two-Phase Commit (2PC) across
 * registered IDistributedShardParticipant instances.
 *
 * This class is thread-safe; individual DistributedTransaction handles are
 * single-threaded.
 *
 * @note Participants are stored as raw pointers (wrapped internally with a
 *       no-op deleter shared_ptr).  Callers must ensure that any registered
 *       participant object is not destroyed before it is either unregistered
 *       or the manager itself is destroyed.
 */
class DistributedTransactionManager {
public:
    /** @brief Per-shard registration entry used in the constructor. */
    struct ShardConfig {
        std::string                  shard_id;
        IDistributedShardParticipant* participant = nullptr;
    };

    /** @brief Best-effort runtime statistics (approximate, not serialized). */
    struct Statistics {
        uint64_t total_transactions = 0;
        uint64_t committed          = 0;
        uint64_t aborted            = 0;
        uint64_t active             = 0;
    };

    /**
     * @brief Construct a manager with an optional initial set of shards.
     */
    explicit DistributedTransactionManager(
        std::vector<ShardConfig> shards = {},
        DistributedTxnConfig     config = {}
    );

    ~DistributedTransactionManager() = default;

    // Non-copyable, non-moveable (shared state is referenced by active transactions)
    DistributedTransactionManager(const DistributedTransactionManager&)            = delete;
    DistributedTransactionManager& operator=(const DistributedTransactionManager&) = delete;
    DistributedTransactionManager(DistributedTransactionManager&&)                 = delete;
    DistributedTransactionManager& operator=(DistributedTransactionManager&&)      = delete;

    // ── Shard management ──────────────────────────────────────────────────────

    /**
     * @brief Register a shard participant.
     *
     * The participant is stored internally as a reference — the caller retains
     * ownership.  The caller must ensure the participant object remains valid
     * until it is either explicitly unregistered or this manager is destroyed.
     *
     * Safe to call concurrently with other shard management operations.
     *
     * @param shard_id    Logical shard identifier (prefix in "shard_id:key").
     * @param participant Non-null pointer to the shard's 2PC proxy.
     * @throws std::invalid_argument if shard_id is empty or participant is null.
     */
    void registerShard(
        const std::string&            shard_id,
        IDistributedShardParticipant* participant
    );

    /**
     * @brief Unregister a shard.
     *
     * Safe to call concurrently.  After this call returns, no new transactions
     * will route operations to the shard.  In-flight transactions that already
     * hold a reference to the participant will complete their current call
     * safely before releasing the reference.
     *
     * @return true if the shard was found and removed.
     */
    bool unregisterShard(const std::string& shard_id);

    /** @return Number of registered shards. */
    size_t shardCount() const;

    /** @return true if the given shard_id is registered. */
    bool hasShard(const std::string& shard_id) const;

    // ── Transaction lifecycle ─────────────────────────────────────────────────

    /**
     * @brief Begin a new distributed transaction.
     *
     * @return A non-null shared_ptr to the new DistributedTransaction.
     */
    std::shared_ptr<DistributedTransaction> beginDistributedTransaction();

    // ── Statistics ────────────────────────────────────────────────────────────

    /**
     * @return Approximate snapshot of coordinator statistics.
     *
     * Counters are updated with relaxed ordering and may not reflect the very
     * latest completed transactions on all cores.  Suitable for monitoring;
     * not suitable for synchronization.
     */
    Statistics statistics() const;

private:
    DistributedTxnConfig                config_;
    std::shared_ptr<ManagerSharedState> state_;

    std::string generateTransactionId();
};

} // namespace storage
} // namespace themis
