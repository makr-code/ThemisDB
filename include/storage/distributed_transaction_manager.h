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
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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
    virtual bool prepare(
        const std::string&                    txn_id,
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
     * The default implementation returns std::nullopt (shard does not support reads).
     */
    virtual std::optional<std::string> get(const std::string& /*key*/) {
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
    /// Timeout for each participant's prepare() call (milliseconds).
    uint32_t prepare_timeout_ms = 5000;
    /// Timeout for each participant's commit()/abort() call (milliseconds).
    uint32_t commit_timeout_ms  = 5000;
    /// Key separator used to extract shard_id from "shard_id:key" notation.
    char shard_key_separator = ':';
};

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

class DistributedTransactionManager;

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

    // Disable copy and move (holds raw pointers to manager state)
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
     */
    void put(std::string_view key, std::string_view value);

    /**
     * @brief Delete a key from the appropriate shard.
     *
     * @param key  "shard_id:logical_key".
     * @throws std::invalid_argument if the transaction is not ACTIVE.
     */
    void del(std::string_view key);

    /**
     * @brief Read a key from the appropriate shard (non-transactional snapshot).
     *
     * Reads are routed to the shard via IDistributedShardParticipant::get().
     * The read is NOT part of the write-set and will not be validated at commit.
     *
     * @param key  "shard_id:logical_key".
     * @return Value bytes, or std::nullopt if not found.
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

    /** @return Shards that have operations in this transaction. */
    std::vector<std::string> participatingShards() const;

    /** @return Number of buffered write operations. */
    size_t operationCount() const;

    // ── Construction (use DistributedTransactionManager::beginDistributedTransaction()) ──

    /**
     * @brief Internal constructor — use DistributedTransactionManager instead.
     *
     * A tag type (PrivateTag) ensures that callers outside of the manager
     * cannot accidentally construct a bare DistributedTransaction.
     */
    struct PrivateTag { explicit PrivateTag() = default; };

    DistributedTransaction(
        PrivateTag,
        std::string                                                      txn_id,
        char                                                             separator,
        std::map<std::string, IDistributedShardParticipant*>*            shards,
        std::mutex*                                                      shards_mutex,
        DistributedTransactionManager*                                   manager
    );

    friend class DistributedTransactionManager;

private:

    /// Parse "shard_id:logical_key" → (shard_id, logical_key)
    std::pair<std::string, std::string> parseKey(std::string_view composite) const;

    std::string                                          txn_id_;
    char                                                 separator_;
    std::map<std::string, IDistributedShardParticipant*>* shards_;   ///< Non-owning
    std::mutex*                                           shards_mutex_;
    DistributedTransactionManager*                        manager_;  ///< Non-owning, for stats callbacks

    DistributedTxnState                                  state_ = DistributedTxnState::ACTIVE;

    /// Per-shard buffered operations (populated by put/del)
    std::map<std::string, std::vector<DistributedOperation>> pending_ops_;

    /// Shards that have received and acknowledged PREPARE (used for abort on partial prepare)
    std::vector<std::string> prepared_shards_;
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
 */
class DistributedTransactionManager {
public:
    /** @brief Per-shard registration entry used in the constructor. */
    struct ShardConfig {
        std::string                  shard_id;
        IDistributedShardParticipant* participant = nullptr;
    };

    /** @brief Runtime statistics. */
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

    // Disable copy; allow move
    DistributedTransactionManager(const DistributedTransactionManager&)            = delete;
    DistributedTransactionManager& operator=(const DistributedTransactionManager&) = delete;

    // ── Shard management ──────────────────────────────────────────────────────

    /**
     * @brief Register a shard participant.
     *
     * The participant must remain valid for the lifetime of this manager (or
     * until it is explicitly unregistered).
     *
     * @param shard_id    Logical shard identifier (prefix used in "shard_id:key").
     * @param participant Non-null pointer to the shard's 2PC proxy.
     * @throws std::invalid_argument if shard_id is empty or participant is null.
     */
    void registerShard(
        const std::string&            shard_id,
        IDistributedShardParticipant* participant
    );

    /**
     * @brief Unregister a shard.
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
     * Operations (put/del/get) are then issued on the returned handle.
     *
     * @return A non-null shared_ptr to the new DistributedTransaction.
     */
    std::shared_ptr<DistributedTransaction> beginDistributedTransaction();

    // ── Statistics ────────────────────────────────────────────────────────────

    /** @return Snapshot of coordinator statistics. */
    Statistics statistics() const;

private:
    friend class DistributedTransaction;

    DistributedTxnConfig                                config_;
    mutable std::mutex                                  shards_mutex_;
    std::map<std::string, IDistributedShardParticipant*> shards_;

    std::atomic<uint64_t>       total_transactions_{0};
    std::atomic<uint64_t>       committed_{0};
    std::atomic<uint64_t>       aborted_{0};
    std::atomic<uint64_t>       active_{0};

    std::atomic<uint64_t>       txn_counter_{0};

    void notifyCommitted();
    void notifyAborted();

    std::string generateTransactionId();
};

} // namespace storage
} // namespace themis
