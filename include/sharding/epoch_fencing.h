/**
 * @file epoch_fencing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

class EpochFencingManager;
class LeaseManager;

// ─────────────────────────────────────────────────────────────────────────────
// Epoch types
// ─────────────────────────────────────────────────────────────────────────────

/// Monotonically-increasing epoch number.  Zero is reserved as "invalid".
using EpochNumber = uint64_t;

/// Identifies a resource for which a lease may be held (e.g. "shard-3-leader").
using LeaseKey = std::string;

/// Identifies a cluster node.
using NodeId = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// EpochToken — carry-along fencing credential
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Immutable fencing token carried with every write RPC.
 *
 * A peer rejects any request whose epoch is strictly less than its own
 * current epoch.  The token is cheaply copyable and trivially serialisable.
 */
struct EpochToken {
    EpochNumber epoch{0};
    NodeId      issuer;       ///< Node that issued this token
    std::string shard_id;     ///< Target shard scope
    std::chrono::system_clock::time_point issued_at;

    /// Returns true if this token is usable with the given current epoch.
    [[nodiscard]] bool isValid(EpochNumber current_epoch) const noexcept {
        return epoch != 0 && epoch >= current_epoch;
    }

    bool operator==(const EpochToken& o) const noexcept {
        return epoch == o.epoch && issuer == o.issuer && shard_id == o.shard_id;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FencingResult — outcome of a fence check
// ─────────────────────────────────────────────────────────────────────────────

enum class FencingResult {
    ALLOWED,            ///< Token epoch matches; operation may proceed
    STALE_EPOCH,        ///< Token epoch is older than current; reject
    INVALID_TOKEN,      ///< Token is structurally invalid (epoch==0)
    STONITH_ISSUED,     ///< Stale node was fenced via STONITH
    STONITH_FAILED,     ///< STONITH attempt failed; manual intervention required
};

inline const char* toString(FencingResult r) noexcept {
    switch (r) {
        case FencingResult::ALLOWED:         return "ALLOWED";
        case FencingResult::STALE_EPOCH:     return "STALE_EPOCH";
        case FencingResult::INVALID_TOKEN:   return "INVALID_TOKEN";
        case FencingResult::STONITH_ISSUED:  return "STONITH_ISSUED";
        case FencingResult::STONITH_FAILED:  return "STONITH_FAILED";
    }
    return "UNKNOWN";
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseState — lifecycle of a distributed lease
// ─────────────────────────────────────────────────────────────────────────────

enum class LeaseState {
    AVAILABLE,   ///< No holder; any node may acquire
    HELD,        ///< Actively held and within its TTL
    EXPIRED,     ///< TTL elapsed without renewal; treated as AVAILABLE
    REVOKING,    ///< STONITH in progress; not yet safe to re-acquire
};

inline const char* toString(LeaseState s) noexcept {
    switch (s) {
        case LeaseState::AVAILABLE: return "AVAILABLE";
        case LeaseState::HELD:      return "HELD";
        case LeaseState::EXPIRED:   return "EXPIRED";
        case LeaseState::REVOKING:  return "REVOKING";
    }
    return "UNKNOWN";
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseRecord — persisted lease entry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief In-memory representation of a lease.  Serialised to WAL on every
 *        acquire / renew / release for crash safety.
 */
struct LeaseRecord {
    LeaseKey    key;
    NodeId      holder;
    EpochNumber epoch{0};
    std::chrono::system_clock::time_point acquired_at;
    std::chrono::system_clock::time_point expires_at;
    uint64_t    generation{0};   ///< Incremented on each acquisition
    LeaseState  state{LeaseState::AVAILABLE};

    /// True if the lease is currently held and has not expired.
    [[nodiscard]] bool isActive() const noexcept {
        return state == LeaseState::HELD &&
               std::chrono::system_clock::now() < expires_at;
    }

    /// Remaining TTL; negative means already expired.
    [[nodiscard]] std::chrono::milliseconds remainingTtl() const noexcept {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            expires_at - now);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// LeaseAcquireResult
// ─────────────────────────────────────────────────────────────────────────────

struct LeaseAcquireResult {
    bool        success{false};
    LeaseRecord record;
    std::string error_message;   ///< Non-empty only on failure
};

// ─────────────────────────────────────────────────────────────────────────────
// IStonithProvider — pluggable hard-fencing backend
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for STONITH (power-cycle / evict) backends.
 *
 * Concrete implementations:
 * - **NullStonithProvider** — no-op; used in tests and single-node setups.
 * - **IpmiStonithProvider** — sends `ipmitool chassis power off` to the BMC.
 * - **MtlsRevocationProvider** — revokes the node's mTLS certificate so it
 *   can no longer issue authenticated RPCs.
 */
class IStonithProvider {
public:
    virtual ~IStonithProvider() = default;

    /**
     * @brief Issue a fencing command against @p node_id.
     *
     * The implementation should ensure the target node can no longer write
     * to the cluster before returning.  If it cannot guarantee this within
     * the given @p deadline, it should return false.
     *
     * @param node_id    Node to fence.
     * @param reason     Human-readable reason (for audit log).
     * @param deadline   Latest point at which fencing must be confirmed.
     * @return true if the node was successfully fenced, false otherwise.
     */
    [[nodiscard]] virtual bool fence(const NodeId& node_id,
                       const std::string& reason,
                       std::chrono::steady_clock::time_point deadline) = 0;

    /**
     * @brief Check whether a previously fenced node has been confirmed offline.
     * @return true if the node is known to be offline / fenced.
     */
    [[nodiscard]] virtual bool isFenced(const NodeId& node_id) const = 0;

    /// Symbolic name of this provider (for logging / metrics).
    [[nodiscard]] virtual std::string providerName() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// NullStonithProvider — test / single-node stub
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief No-op STONITH provider.  Records fence calls in memory for test
 *        assertions; never actually kills anything.
 */
class NullStonithProvider final : public IStonithProvider {
public:
    bool fence(const NodeId& node_id,
               const std::string& /*reason*/,
               std::chrono::steady_clock::time_point /*deadline*/) override;

    bool isFenced(const NodeId& node_id) const override;

    [[nodiscard]] std::string providerName() const override {
        return "NullStonithProvider";
    }

    /// Returns all nodes that have been fenced (for test assertions).
    [[nodiscard]] std::vector<NodeId> fencedNodes() const;

    /// Clears the fenced-node set (for test teardown).
    void reset();

private:
    mutable std::mutex        mutex_;
    std::vector<NodeId>       fenced_;
};

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingConfig
// ─────────────────────────────────────────────────────────────────────────────

struct EpochFencingConfig {
    /// Shard scope this manager guards.
    std::string shard_id{"default"};

    /// Local node identity.
    NodeId node_id;

    /// If true, automatically issue STONITH when a stale write is detected.
    bool auto_stonith{true};

    /// Maximum time allowed for a STONITH operation to complete.
    std::chrono::milliseconds stonith_timeout_ms{5000};

    /// Minimum epoch delta before a stale-epoch log warning is emitted.
    uint64_t log_warn_epoch_delta{1};

    bool validate() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Epoch-based fencing manager.
 *
 * Maintains the authoritative current epoch for a shard and exposes a
 * single check-and-fence entry point used by the Raft leader and all
 * write-path hot spots.
 *
 * ### Epoch Increment Protocol
 * 1. A new Raft term begins (or leader election completes).
 * 2. The elected leader calls `bumpEpoch()`, which atomically increments
 *    the counter and returns the new EpochToken.
 * 3. The leader attaches the token to every subsequent write RPC.
 * 4. Followers call `checkToken()` on every incoming write RPC.
 * 5. If the token's epoch is stale, `checkToken()` returns STALE_EPOCH and
 *    — if `auto_stonith` is set — triggers STONITH via the provider.
 *
 * @note This class is entirely in-process.  Across processes the epoch is
 *       propagated through the Raft log; each node reconstructs it on replay.
 */
class EpochFencingManager {
public:
    /**
     * @brief Construct a fencing manager for the given shard.
     * @param config   Configuration.
     * @param stonith  STONITH backend (ownership transferred).
     */
    explicit EpochFencingManager(EpochFencingConfig config,
                                 std::shared_ptr<IStonithProvider> stonith =
                                     std::make_shared<NullStonithProvider>());

    virtual ~EpochFencingManager();

    // ── Epoch management ────────────────────────────────────────────────────

    /**
     * @brief Atomically increment the epoch and return a fresh token.
     *
     * Only the elected leader should call this.  On epoch bump all
     * in-flight tokens from the previous epoch become stale immediately.
     *
     * @param reason  Log message describing why the epoch was bumped.
     * @return New EpochToken with `epoch == currentEpoch()`.
     * @note Virtual to enable test doubles that return controlled epoch values
     *       (e.g. epoch=0 to exercise the invalid-epoch guard in failover paths).
     */
    [[nodiscard]] virtual EpochToken bumpEpoch(const std::string& reason);

    /**
     * @brief Return the current epoch without modifying it.
     */
    [[nodiscard]] EpochNumber currentEpoch() const noexcept;

    /**
     * @brief Produce a token for the current epoch (leader use only).
     *
     * Does NOT bump the epoch — call this when the epoch is already correct
     * and a new write just needs a fresh credential.
     */
    [[nodiscard]] EpochToken makeToken() const;

    // ── Fencing check ───────────────────────────────────────────────────────

    /**
     * @brief Check whether @p token authorises a write in the current epoch.
     *
     * - ALLOWED        → proceed with the write
     * - STALE_EPOCH    → reject; if auto_stonith the STONITH call may follow
     * - INVALID_TOKEN  → reject; structurally invalid token
     * - STONITH_ISSUED → stale token AND STONITH successfully issued
     * - STONITH_FAILED → stale token AND STONITH failed
     *
     * @param token       Token attached to the incoming write RPC.
     * @param source_node Node that sent the request (for STONITH targeting).
     */
    [[nodiscard]] FencingResult checkToken(const EpochToken& token,
                                           const NodeId& source_node);

    // ── Metrics ─────────────────────────────────────────────────────────────

    struct Metrics {
        uint64_t epoch_bumps{0};
        uint64_t allowed_writes{0};
        uint64_t stale_rejections{0};
        uint64_t stonith_issued{0};
        uint64_t stonith_failed{0};
    };

    [[nodiscard]] Metrics metrics() const noexcept;

    // ── Lifecycle ───────────────────────────────────────────────────────────

    [[nodiscard]] const EpochFencingConfig& config() const noexcept {
        return config_;
    }

    [[nodiscard]] std::shared_ptr<IStonithProvider> stonithProvider() const noexcept {
        return stonith_;
    }

private:
    EpochFencingConfig               config_;
    std::shared_ptr<IStonithProvider> stonith_;
    std::atomic<EpochNumber>          current_epoch_{1};   // Start at 1; 0 is invalid

    mutable std::mutex mutex_;
    Metrics            metrics_;

    FencingResult issueStonith(const NodeId& node, const std::string& reason);
};

// ─────────────────────────────────────────────────────────────────────────────
// LeaseConfig
// ─────────────────────────────────────────────────────────────────────────────

struct LeaseConfig {
    /// Default lease TTL.
    std::chrono::milliseconds ttl_ms{10'000};

    /// How early before expiry the holder should renew.
    std::chrono::milliseconds renew_before_ms{3'000};

    /// Maximum time to wait for a competing lease to expire.
    std::chrono::milliseconds acquire_wait_ms{15'000};

    /// Path prefix for WAL persistence (empty ⇒ in-memory only).
    std::string wal_path;

    bool validate() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Distributed lease manager.
 *
 * Provides exclusive, time-bounded leases for cluster resources.  Leases
 * are backed by the EpochFencingManager so that an expired-lease holder
 * whose epoch is also stale can be fenced via STONITH.
 *
 * ### Acquire Protocol
 * 1. Caller calls `acquire(key, node_id)`.
 * 2. If the key is AVAILABLE (or EXPIRED), the lease is granted immediately
 *    with a fresh EpochToken from the associated `EpochFencingManager`.
 * 3. If the key is HELD, the manager waits up to `acquire_wait_ms` for the
 *    current holder to release or expire; then retries once.
 * 4. If the holder does not release in time, STONITH is optionally issued
 *    and the lease transitions to REVOKING while fencing completes.
 *
 * ### Crash Recovery
 * On construction, if a non-empty `wal_path` is configured, the manager
 * replays existing lease records from WAL.  Expired leases are discarded;
 * active leases are restored so a restarted node does not accidentally
 * seize a lease still held by a live peer.
 */
class LeaseManager {
public:
    /**
     * @param config   Lease configuration.
     * @param fencing  Epoch fencing manager used for token issuance and STONITH.
     */
    explicit LeaseManager(LeaseConfig config,
                          std::shared_ptr<EpochFencingManager> fencing);

    ~LeaseManager();

    // ── Acquire ─────────────────────────────────────────────────────────────

    /**
     * @brief Try to acquire the lease for @p key on behalf of @p node_id.
     *
     * Blocking call; returns when the lease is acquired, already held by
     * another node and the wait timed out, or an error occurred.
     *
     * @param key     Lease identifier (e.g. "shard-3-leader").
     * @param node_id Requesting node.
     * @return LeaseAcquireResult with `success == true` on grant.
     */
    [[nodiscard]] LeaseAcquireResult acquire(const LeaseKey& key,
                                             const NodeId&   node_id);

    // ── Renew ───────────────────────────────────────────────────────────────

    /**
     * @brief Renew an existing lease.
     *
     * Only the current holder may renew.  Returns false if the lease has
     * already expired or is held by a different node.
     *
     * @param key     Lease key.
     * @param node_id Must match the current holder.
     * @return Updated LeaseRecord on success, or nullopt on failure.
     */
    [[nodiscard]] std::optional<LeaseRecord> renew(const LeaseKey& key,
                                                   const NodeId&   node_id);

    // ── Release ─────────────────────────────────────────────────────────────

    /**
     * @brief Voluntarily release a held lease.
     *
     * Idempotent: releasing a lease that is not held by @p node_id is a
     * no-op and returns false.
     *
     * @param key     Lease key.
     * @param node_id Must match the current holder.
     * @return true if the lease was released.
     */
    bool release(const LeaseKey& key, const NodeId& node_id);

    // ── Query ───────────────────────────────────────────────────────────────

    /**
     * @brief Look up a lease record.
     * @return Current record, or nullopt if the key has never been acquired.
     */
    [[nodiscard]] std::optional<LeaseRecord> get(const LeaseKey& key) const;

    /**
     * @brief Check whether @p node_id currently holds the lease for @p key.
     */
    [[nodiscard]] bool isHolder(const LeaseKey& key,
                                const NodeId&   node_id) const;

    /**
     * @brief List all lease keys currently tracked by this manager.
     */
    [[nodiscard]] std::vector<LeaseKey> listLeases() const;

    // ── Maintenance ─────────────────────────────────────────────────────────

    /**
     * @brief Expire and clean up all leases whose TTL has elapsed.
     *
     * Called automatically on acquire/renew; also callable from a
     * background maintenance thread.
     */
    void evictExpired();

    // ── Metrics ─────────────────────────────────────────────────────────────

    struct Metrics {
        uint64_t acquires{0};
        uint64_t acquire_failures{0};
        uint64_t renewals{0};
        uint64_t renewal_failures{0};
        uint64_t releases{0};
        uint64_t evictions{0};
        uint64_t stonith_triggered{0};
    };

    [[nodiscard]] Metrics metrics() const noexcept;

    [[nodiscard]] const LeaseConfig& config() const noexcept { return config_; }

private:
    LeaseConfig                          config_;
    std::shared_ptr<EpochFencingManager> fencing_;

    mutable std::mutex                               mutex_;
    std::unordered_map<LeaseKey, LeaseRecord>        leases_;
    Metrics                                          metrics_;

    // ── Helpers ─────────────────────────────────────────────────────────────

    LeaseRecord& getOrCreate(const LeaseKey& key);
    void         persistToWal(const LeaseRecord& record);
    void         loadFromWal();
    bool         waitForExpiry(const LeaseKey& key,
                               std::chrono::milliseconds timeout);
};

} // namespace sharding
} // namespace themis
