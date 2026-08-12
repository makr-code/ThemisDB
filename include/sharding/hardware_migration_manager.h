/**
 * @file hardware_migration_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.14
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

/**
 * Hardware Migration Manager for ThemisDB Sharding
 *
 * Addresses the question: what happens to the consistent hash ring and
 * node identity when a ThemisDB shard node moves to new physical hardware?
 *
 * Key design decisions:
 * 1. Node identity (shard_id) is LOGICAL, not hardware-bound.
 *    The hash ring positions are derived from shard_id — moving to
 *    new hardware does NOT change ring positions as long as shard_id
 *    is preserved.
 * 2. A NodeIdentity file on disk decouples the logical identity from
 *    the physical endpoint. The file must be transferred to new hardware.
 * 3. Endpoint replacement is an atomic topology update: the topology
 *    record for a given shard_id gets a new primary_endpoint while
 *    all ring positions remain unchanged.
 * 4. During the migration window both old and new endpoints are tracked
 *    so in-flight requests are not lost.
 */

#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <condition_variable>
#include <optional>
#include <mutex>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// NodeIdentity — stable logical identity of a shard node
// Must be persisted to disk and transferred to new hardware before migration.
// ─────────────────────────────────────────────────────────────────────────────

struct NodeIdentity {
    std::string shard_id;        // Stable logical shard ID (never changes)
    std::string cluster_name;    // Cluster this shard belongs to
    uint64_t    token_start{0};  // Start of the assigned token range
    uint64_t    token_end{0};    // End of the assigned token range
    std::string created_at;      // ISO-8601 timestamp of original provisioning
    std::string identity_version; // Monotonically increasing version tag

    /** Serialise identity to JSON string (written to the identity file). */
    std::string toJson() const;

    /** Deserialise identity from a JSON string; returns nullopt on parse error. */
    static std::optional<NodeIdentity> fromJson(const std::string& json);

    /** Write identity to file. Returns false and logs on error. */
    bool saveTo(const std::string& path) const;

    /** Load identity from file. Returns nullopt if file absent or invalid. */
    static std::optional<NodeIdentity> loadFrom(const std::string& path);
};

// ─────────────────────────────────────────────────────────────────────────────
// HardwareMigrationConfig
// ─────────────────────────────────────────────────────────────────────────────

struct HardwareMigrationConfig {
    /**
     * Path to the node identity file (e.g. /var/lib/themisdb/node_identity.json).
     * This file MUST be copied to the new hardware before calling replaceEndpoint().
     */
    std::string identity_file_path = "/var/lib/themisdb/node_identity.json";

    /**
     * How long to keep the old endpoint registered as "draining" before
     * removing it from the topology (default: 60 s). Set to 0 to remove
     * the old endpoint immediately after the topology update.
     */
    std::chrono::seconds drain_period{60};

    /**
     * If true, perform a ring-stability assertion after the endpoint update:
     * verify that every shard_id maps to the same token positions as before.
     */
    bool verify_ring_stability = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// HardwareMigrationResult
// ─────────────────────────────────────────────────────────────────────────────

struct HardwareMigrationResult {
    bool success = false;

    /** Human-readable summary of what was done (or what went wrong). */
    std::string message;

    /** shard_id whose endpoint was updated. */
    std::string shard_id;

    /** Endpoint before the migration. */
    std::string old_endpoint;

    /** Endpoint after the migration. */
    std::string new_endpoint;

    /**
     * True when ring positions were verified unchanged after the endpoint
     * update. Only meaningful when HardwareMigrationConfig::verify_ring_stability
     * is true.
     */
    bool ring_stability_verified = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// HardwareMigrationManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Manages the process of migrating a shard node to new physical hardware.
 *
 * Thread-safe.
 */
class HardwareMigrationManager {
public:
    /**
     * Construct manager.
     *
     * @param config         Migration configuration.
     * @param topology       Shared shard topology (modified atomically).
     * @param ring           Shared consistent-hash ring (read-only during migration).
     */
    HardwareMigrationManager(
        const HardwareMigrationConfig& config,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ConsistentHashRing> ring
    );

    // ── Identity management ──────────────────────────────────────────────────

    /**
     * Load the node identity from the identity file.
     * Returns nullopt if the file does not exist.
     */
    std::optional<NodeIdentity> loadIdentity() const;

    /**
     * Create a new node identity for the given shard and persist it to disk.
     * Fails if an identity file already exists (use loadIdentity() first).
     *
     * @param shard_id      Logical shard identifier.
     * @param cluster_name  Cluster the shard belongs to.
     * @param token_start   Token range start.
     * @param token_end     Token range end.
     * @return The created identity, or nullopt on failure.
     */
    std::optional<NodeIdentity> createAndSaveIdentity(
        const std::string& shard_id,
        const std::string& cluster_name,
        uint64_t token_start,
        uint64_t token_end
    ) const;

    // ── Endpoint replacement ─────────────────────────────────────────────────

    /**
     * Replace the primary endpoint of a shard without altering its ring
     * positions or any other topology metadata.
     *
     * Preconditions (all must hold; failure logged and returned in result):
     *  - shard_id must exist in the topology.
     *  - new_endpoint must not be empty.
     *  - If verify_ring_stability is true, the shard must appear in the ring.
     *
     * Postconditions:
     *  - ShardInfo::primary_endpoint updated to new_endpoint.
     *  - Ring positions (virtual nodes) for shard_id are UNCHANGED.
     *
     * @param shard_id     The logical shard to update.
     * @param new_endpoint New network endpoint (host:port or URL).
     * @return Result describing what happened.
     */
    HardwareMigrationResult replaceEndpoint(
        const std::string& shard_id,
        const std::string& new_endpoint
    );

    // ── Ring-stability validation ────────────────────────────────────────────

    /**
     * Verify that every shard listed in shard_ids has the same set of virtual-
     * node token positions in the ring before and after a topology change.
     *
     * Compares the ring snapshot taken at construction time of this call (the
     * "before" state, passed as parameter) against the current ring state.
     *
     * @param shard_ids         Shards to validate.
     * @param before_vnode_counts Map of shard_id → virtual-node count captured
     *                           before the topology change.
     * @return true if all shards have the same virtual-node count as before.
     */
    bool validateRingStability(
        const std::vector<std::string>& shard_ids,
        const std::map<std::string, size_t>& before_vnode_counts
    ) const;

    /**
     * Capture a ring snapshot: shard_id → virtual-node count for all
     * currently known shards. Useful to record the "before" state prior to
     * calling replaceEndpoint().
     */
    std::map<std::string, size_t> captureRingSnapshot() const;

    // ── Drain-period enforcement ─────────────────────────────────────────────

    /**
     * @brief RAII guard that tracks one in-flight request for a shard.
     *
     * Create one guard per request that is active during a migration drain.
     * The guard increments the in-flight counter on construction and decrements
     * it on destruction.
     *
     * Usage:
     * @code
     *   auto guard = mgr.makeRequestGuard("shard-1");
     *   // … serve the request …
     * @endcode
     */
    class DrainGuard {
    public:
        DrainGuard() = default;
        ~DrainGuard();
        DrainGuard(const DrainGuard&)            = delete;
        DrainGuard& operator=(const DrainGuard&) = delete;
        DrainGuard(DrainGuard&&) noexcept;
        DrainGuard& operator=(DrainGuard&&) noexcept;

    private:
        friend class HardwareMigrationManager;
        DrainGuard(HardwareMigrationManager* mgr, std::string shard_id);

        HardwareMigrationManager* mgr_{nullptr};
        std::string shard_id_;
        bool active_{false};
    };

    /**
     * @brief Increment the in-flight request counter for @p shard_id.
     *
     * Call once per new in-flight request arriving on the old endpoint during
     * the drain window.  Must be paired with `releaseInFlightRequest()`.
     * Prefer `makeRequestGuard()` to avoid mismatched calls.
     */
    void addInFlightRequest(const std::string& shard_id);

    /**
     * @brief Decrement the in-flight request counter for @p shard_id.
     *
     * Notifies `waitForDrain()` callers when the counter reaches zero.
     */
    void releaseInFlightRequest(const std::string& shard_id);

    /**
     * @brief Return the current in-flight request count for @p shard_id.
     */
    size_t inFlightCount(const std::string& shard_id) const;

    /**
     * @brief Create a RAII DrainGuard that counts one in-flight request.
     *
     * @param shard_id  Shard whose counter to increment/decrement.
     */
    DrainGuard makeRequestGuard(const std::string& shard_id);

    /**
     * @brief Block until all in-flight requests for @p shard_id complete,
     *        or until `timeout` elapses (whichever comes first).
     *
     * This is called by `replaceEndpoint()` when `config_.drain_period > 0`
     * and is also available for callers that want to wait before removing an
     * old-endpoint registration from DNS / load balancer.
     *
     * @param shard_id  Shard to wait for.
     * @param timeout   Maximum time to wait.  Pass `std::chrono::seconds{0}` to
     *                  skip waiting (proceed immediately regardless of in-flight
     *                  count).
     * @return `true` if all requests drained within the timeout,
     *         `false` if the timeout expired while requests were still active.
     */
    bool waitForDrain(const std::string& shard_id,
                      std::chrono::seconds timeout) const;

private:
    HardwareMigrationConfig         config_;
    std::shared_ptr<ShardTopology>  topology_;
    std::shared_ptr<ConsistentHashRing> ring_;
    mutable std::mutex              mutex_;

    // ── Drain-tracking state ─────────────────────────────────────────────────
    mutable std::mutex              drain_mutex_;
    mutable std::condition_variable drain_cv_;
    /// shard_id → number of in-flight requests on the old endpoint
    std::map<std::string, size_t>   in_flight_counts_;

    /**
     * Internal helper — captures ring snapshot while mutex_ is already held.
     * Must only be called by methods that hold mutex_.
     */
    std::map<std::string, size_t> captureRingSnapshotLocked() const;
};

} // namespace themis::sharding
