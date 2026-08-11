// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file geo_placement.h
 * @brief Geographic Replica Placement Policy API for ThemisDB replication.
 *
 * Provides constraint-based placement policy evaluation for leader election
 * and failover candidate selection in multi-datacenter (multi-DC) topologies.
 *
 * ### Design constraints
 * - PlacementConstraints are evaluated read-only; they never mutate topology.
 * - selectLeaderCandidate() and selectFailoverCandidate() return the best
 *   matching ReplicaInfo according to the active constraints; callers remain
 *   responsible for actually performing promotion.
 * - All methods are thread-safe.
 *
 * ### Placement constraint semantics
 *
 * | Constraint field          | Meaning                                            |
 * |---------------------------|----------------------------------------------------|
 * | preferred_datacenters     | Ordered list; first matching DC wins.              |
 * | forbidden_datacenters     | Candidates in these DCs are rejected outright.     |
 * | required_datacenters      | At least one candidate must come from each entry.  |
 * | min_copies_per_dc         | Minimum healthy replicas per DC for valid placement|
 * | zone_affinity             | Prefer candidates in the same zone as the caller.  |
 * | zone_anti_affinity        | Reject candidates in the same zone as the caller.  |
 * | require_voter             | Only voting members are eligible.                  |
 *
 * @see include/replication/replication_manager.h — ReplicaInfo
 * @see src/replication/ROADMAP.md — §3.1 Geographic replica placement policies
 */

#pragma once

#include "replication/replication_manager.h"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace replication {

// ============================================================================
// PlacementConstraints — DSL for geo/zone-level replica placement
// ============================================================================

/**
 * @brief Placement constraints for geographic replica positioning.
 *
 * Consumed by GeoReplicaPlacementManager to filter and rank replica candidates
 * during leader election and failover.
 */
struct PlacementConstraints {
    /**
     * Ordered preference list of datacenter IDs.  Candidates in the first DC
     * that has an eligible replica are ranked highest.  An empty list means
     * "no DC preference".
     */
    std::vector<std::string> preferred_datacenters;

    /**
     * Datacenters that must never host the selected candidate.
     * Any candidate whose ReplicaInfo::datacenter appears here is excluded.
     */
    std::vector<std::string> forbidden_datacenters;

    /**
     * Every datacenter listed here must have at least one healthy replica in
     * the cluster.  validatePlacement() returns a violation when this is not
     * satisfied.
     */
    std::vector<std::string> required_datacenters;

    /**
     * Minimum number of healthy replicas required per datacenter.
     * Zero means "no minimum per DC" (default).
     */
    uint32_t min_copies_per_dc = 0;

    /**
     * When non-empty, candidates in the same zone as this value are preferred
     * (zone affinity).  Cannot be set simultaneously with zone_anti_affinity_zone.
     */
    std::string zone_affinity_zone;

    /**
     * When non-empty, candidates in the same zone as this value are excluded
     * (zone anti-affinity).
     */
    std::string zone_anti_affinity_zone;

    /**
     * When true, only voting members (ReplicaInfo::is_voting_member == true)
     * are eligible.
     */
    bool require_voter = true;

    /**
     * When true, only candidates with HealthStatus::HEALTHY are eligible.
     * Candidates with HealthStatus::UNKNOWN are excluded.  Defaults to true.
     */
    bool healthy_only = true;
};

// ============================================================================
// PlacementValidationResult
// ============================================================================

/**
 * @brief Result of a placement constraint validation check.
 */
struct PlacementValidationResult {
    bool                     is_valid = true;
    std::vector<std::string> violations;      ///< Non-empty when is_valid == false
    std::vector<std::string> recommendations; ///< Suggestions even when valid
};

// ============================================================================
// GeoReplicaPlacementManager
// ============================================================================

/**
 * @brief Geographic replica placement policy manager.
 *
 * Evaluates PlacementConstraints against a live replica topology to:
 * - Select the best leader candidate for initial election.
 * - Select the best failover candidate when the current leader fails.
 * - Validate whether the current topology satisfies a given constraint set.
 *
 * ### Thread safety
 * All public methods are stateless with respect to internal mutable state
 * (the topology snapshot is provided by the caller on each call).  The class
 * is safe to call from multiple threads concurrently.
 *
 * ### Usage example
 * @code
 *   GeoReplicaPlacementManager placement;
 *
 *   PlacementConstraints constraints;
 *   constraints.preferred_datacenters = {"eu-west-1", "eu-central-1"};
 *   constraints.forbidden_datacenters = {"ap-southeast-1"};
 *   constraints.min_copies_per_dc = 1;
 *   constraints.require_voter = true;
 *
 *   auto leader = placement.selectLeaderCandidate(replicas, constraints);
 *   if (leader)
 *       std::cout << "Elect: " << leader->node_id << "\n";
 *
 *   auto validation = placement.validatePlacement(replicas, constraints);
 *   if (!validation.is_valid)
 *       for (const auto& v : validation.violations)
 *           std::cerr << "Violation: " << v << "\n";
 * @endcode
 */
class GeoReplicaPlacementManager {
public:
    GeoReplicaPlacementManager()  = default;
    ~GeoReplicaPlacementManager() = default;

    // Non-copyable, movable
    GeoReplicaPlacementManager(const GeoReplicaPlacementManager&)            = delete;
    GeoReplicaPlacementManager& operator=(const GeoReplicaPlacementManager&) = delete;
    GeoReplicaPlacementManager(GeoReplicaPlacementManager&&)                 = default;
    GeoReplicaPlacementManager& operator=(GeoReplicaPlacementManager&&)      = default;

    // -----------------------------------------------------------------------
    // Candidate selection
    // -----------------------------------------------------------------------

    /**
     * @brief Select the best leader candidate from a list of replicas.
     *
     * Ranking order (highest priority first):
     * 1. Candidate is in a preferred DC (ordered by preference list index).
     * 2. Candidate has the highest priority (ReplicaInfo::priority).
     * 3. Candidate has the lowest replication lag (last_applied_sequence desc).
     *
     * @param replicas    Current replica topology snapshot.
     * @param constraints Placement constraints to apply.
     * @return The best eligible candidate, or std::nullopt when no candidate
     *         satisfies all constraints.
     */
    std::optional<ReplicaInfo> selectLeaderCandidate(
        const std::vector<ReplicaInfo>& replicas,
        const PlacementConstraints&     constraints) const;

    /**
     * @brief Select the best failover candidate excluding a failed node.
     *
     * Identical ranking logic as selectLeaderCandidate() but additionally
     * skips the node identified by @p failed_node_id.
     *
     * @param replicas       Current replica topology snapshot.
     * @param constraints    Placement constraints to apply.
     * @param failed_node_id Node ID of the failed leader to exclude.
     * @return The best eligible candidate, or std::nullopt when no candidate
     *         satisfies all constraints.
     */
    std::optional<ReplicaInfo> selectFailoverCandidate(
        const std::vector<ReplicaInfo>& replicas,
        const PlacementConstraints&     constraints,
        const std::string&              failed_node_id) const;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate whether the current topology satisfies the constraints.
     *
     * Checks performed:
     * 1. Every required DC has at least one healthy replica.
     * 2. Every DC meets min_copies_per_dc healthy replica count.
     * 3. No forbidden DC hosts a healthy replica (recommendation only — not
     *    a hard violation since placement is advisory).
     * 4. At least one eligible candidate exists for leader election.
     *
     * @param replicas    Current topology snapshot.
     * @param constraints Constraints to evaluate.
     * @return PlacementValidationResult with violations and recommendations.
     */
    PlacementValidationResult validatePlacement(
        const std::vector<ReplicaInfo>& replicas,
        const PlacementConstraints&     constraints) const;

    // -----------------------------------------------------------------------
    // Topology helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Return a per-DC count of healthy replicas.
     *
     * @param replicas Current topology snapshot.
     * @return Map of datacenter ID → healthy replica count.
     */
    std::unordered_map<std::string, uint32_t> healthyCountPerDC(
        const std::vector<ReplicaInfo>& replicas) const;

private:
    /// Returns true when the candidate is eligible under the given constraints.
    bool isEligible(const ReplicaInfo&          candidate,
                    const PlacementConstraints& constraints) const;

    /**
     * Rank score for a candidate given its DC's position in the preference
     * list.  Lower index → higher score.  DC not in list → lowest score.
     */
    int dcPreferenceScore(const std::string&          datacenter,
                          const PlacementConstraints& constraints) const;
};

} // namespace replication
} // namespace themisdb
