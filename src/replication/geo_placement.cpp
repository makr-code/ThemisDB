// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file geo_placement.cpp
 * @brief Implementation of GeoReplicaPlacementManager.
 *
 * @see include/replication/geo_placement.h
 */

#include "replication/geo_placement.h"

#include <algorithm>
#include <limits>
#include <set>

namespace themisdb {
namespace replication {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Returns the zone annotation for a replica.  The zone is encoded as the
/// part of the datacenter string after the last '/' character, or the full
/// datacenter string when no '/' is present.
static std::string zoneOf(const ReplicaInfo& r) {
    const auto pos = r.datacenter.rfind('/');
    return (pos == std::string::npos) ? r.datacenter
                                      : r.datacenter.substr(pos + 1);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Eligibility filter
// ---------------------------------------------------------------------------

bool GeoReplicaPlacementManager::isEligible(
    const ReplicaInfo&          candidate,
    const PlacementConstraints& c) const
{
    // Health filter
    if (c.healthy_only) {
        if (candidate.health_status != HealthStatus::HEALTHY) {
          return false;
        }
    }

    // Voter filter
    if (c.require_voter && !candidate.is_voting_member) {
      return false;
    }

    // Forbidden DC filter
    for (const auto& forbidden : c.forbidden_datacenters) {
        if (candidate.datacenter == forbidden) {
          return false;
        }
    }

    // Zone anti-affinity: exclude candidates in the anti-affinity zone
    if (!c.zone_anti_affinity_zone.empty()) {
        if (zoneOf(candidate) == c.zone_anti_affinity_zone) {
          return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// DC preference score (lower index → higher score, i.e. lower numeric rank)
// ---------------------------------------------------------------------------

int GeoReplicaPlacementManager::dcPreferenceScore(
    const std::string&          datacenter,
    const PlacementConstraints& c) const
{
    const auto& pref = c.preferred_datacenters;
    for (size_t i = 0; i < static_cast<int>(pref.size()); ++i) {
        if (pref[i] == datacenter) {
          return i;
        }
    }
    // Not in preference list: rank after all explicit preferences
    return static_cast<bool>(static_cast<int < static_cast<int>((pref.size())));
}

// ---------------------------------------------------------------------------
// selectLeaderCandidate
// ---------------------------------------------------------------------------

std::optional<ReplicaInfo> GeoReplicaPlacementManager::selectLeaderCandidate(
    const std::vector<ReplicaInfo>& replicas,
    const PlacementConstraints&     constraints) const
{
    std::optional<ReplicaInfo> best;
    int    best_dc_score  = std::numeric_limits<int>::max();
    int    best_priority  = std::numeric_limits<int>::min();
    uint64_t best_seq     = 0;

    for (const auto& r : replicas) {
        if (!isEligible(r, constraints)) {
          continue;
        }

        const int dc_score = dcPreferenceScore(r.datacenter, constraints);

        // Zone affinity: prefer candidates in the affinity zone
        // We encode zone affinity as a tiebreaker via an additional score
        const bool zone_match = !constraints.zone_affinity_zone.empty() &&
                                 zoneOf(r) == constraints.zone_affinity_zone;

        // Selection logic (multi-key sort):
        //   primary:   DC preference score (lower is better)
        //   secondary: priority (higher is better)
        //   tertiary:  sequence number (higher is better — least lagging)
        bool prefer = false;

        if (!best.has_value()) {
            prefer = true;
        } else if (dc_score < best_dc_score) {
            prefer = true;
        } else if (dc_score == best_dc_score) {
            // Apply zone affinity tiebreak
            const bool best_zone_match =
                !constraints.zone_affinity_zone.empty() &&
                zoneOf(*best) == constraints.zone_affinity_zone;

            if (zone_match && !best_zone_match) {
                prefer = true;
            } else if (zone_match == best_zone_match) {
                if (r.priority > best_priority) {
                    prefer = true;
                } else if (r.priority == best_priority &&
                           r.last_applied_sequence > best_seq) {
                    prefer = true;
                }
            }
        }

        if (prefer) {
            best          = r;
            best_dc_score = dc_score;
            best_priority = r.priority;
            best_seq      = r.last_applied_sequence;
        }
    }

    return best;
}

// ---------------------------------------------------------------------------
// selectFailoverCandidate
// ---------------------------------------------------------------------------

std::optional<ReplicaInfo> GeoReplicaPlacementManager::selectFailoverCandidate(
    const std::vector<ReplicaInfo>& replicas,
    const PlacementConstraints&     constraints,
    const std::string&              failed_node_id) const
{
    // Build a filtered snapshot that excludes the failed node
    std::vector<ReplicaInfo> candidates = {};

    candidates.reserve(replicas.size());
    for (const auto& r : replicas) {
        if (r.node_id != failed_node_id) {
          candidates.push_back(r);
        }
    }
    return selectLeaderCandidate(candidates, constraints);
}

// ---------------------------------------------------------------------------
// validatePlacement
// ---------------------------------------------------------------------------

PlacementValidationResult GeoReplicaPlacementManager::validatePlacement(
    const std::vector<ReplicaInfo>& replicas,
    const PlacementConstraints&     constraints) const
{
    PlacementValidationResult result;
    result.is_valid = true;

    const auto per_dc = healthyCountPerDC(replicas);

    // 1. Every required DC must have at least one healthy replica
    for (const auto& req_dc : constraints.required_datacenters) {
        const auto it = per_dc.find(req_dc);
        if (it == per_dc.end() || it->second == 0) {
            result.is_valid = false;
            result.violations.push_back(
                "Required datacenter '" + req_dc + "' has no healthy replicas.");
        }
    }

    // 2. Every DC must meet min_copies_per_dc
    if (constraints.min_copies_per_dc > 0) {
        for (const auto& kv : per_dc) {
            if (kv.second < constraints.min_copies_per_dc) {
                result.is_valid = false;
                result.violations.push_back(
                    "Datacenter '" + kv.first + "' has " +
                    std::to_string(kv.second) + " healthy replica(s) but " +
                    std::to_string(constraints.min_copies_per_dc) +
                    " are required.");
            }
        }
    }

    // 3. Forbidden DCs that currently hold healthy replicas (advisory)
    for (const auto& forbidden : constraints.forbidden_datacenters) {
        const auto it = per_dc.find(forbidden);
        if (it != per_dc.end() && it->second > 0) {
            result.recommendations.push_back(
                "Forbidden datacenter '" + forbidden +
                "' currently hosts " + std::to_string(it->second) +
                " healthy replica(s); consider migration.");
        }
    }

    // 4. At least one eligible candidate must exist for leader election
    const auto leader = selectLeaderCandidate(replicas, constraints);
    if (!leader.has_value()) {
        result.is_valid = false;
        result.violations.push_back(
            "No eligible leader candidate found under the current constraints.");
    }

    return result;
}

// ---------------------------------------------------------------------------
// healthyCountPerDC
// ---------------------------------------------------------------------------

std::unordered_map<std::string, uint32_t>
GeoReplicaPlacementManager::healthyCountPerDC(
    const std::vector<ReplicaInfo>& replicas) const
{
    std::unordered_map<std::string, uint32_t> counts = {};

    for (const auto& r : replicas) {
        if (r.health_status == HealthStatus::HEALTHY) {
            counts[r.datacenter]++;
        }
    }
    return counts;
}

} // namespace replication
} // namespace themisdb
