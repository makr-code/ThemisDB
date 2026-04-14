/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hardware_migration_manager.h                       ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 06:56:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     247                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 6d2b1af70b  2026-03-01  feat(sharding): hardware migration support - NodeIdentity... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

private:
    HardwareMigrationConfig         config_;
    std::shared_ptr<ShardTopology>  topology_;
    std::shared_ptr<ConsistentHashRing> ring_;
    mutable std::mutex              mutex_;

    /**
     * Internal helper — captures ring snapshot while mutex_ is already held.
     * Must only be called by methods that hold mutex_.
     */
    std::map<std::string, size_t> captureRingSnapshotLocked() const;
};

} // namespace themis::sharding
