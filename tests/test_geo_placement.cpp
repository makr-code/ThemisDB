// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_geo_placement.cpp
 * @brief Unit tests for GeoReplicaPlacementManager.
 */

#include "replication/geo_placement.h"

#include <gtest/gtest.h>
#include <algorithm>

namespace themisdb {
namespace replication {

// Test fixture
class GeoPlacementTest : public ::testing::Test {
protected:
    GeoReplicaPlacementManager placement_;

    // Helper to create a replica
    ReplicaInfo makeReplica(const std::string& node_id,
                            const std::string& datacenter,
                            HealthStatus       health = HealthStatus::HEALTHY,
                            bool               voting = true,
                            int                priority = 0,
                            uint64_t           seq = 0)
    {
        ReplicaInfo r;
        r.node_id = node_id;
        r.datacenter = datacenter;
        r.health_status = health;
        r.is_voting_member = voting;
        r.priority = priority;
        r.last_applied_sequence = seq;
        return r;
    }
};

// ============================================================================
// Basic candidate selection tests
// ============================================================================

TEST_F(GeoPlacementTest, SelectLeaderFromSingleReplica)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100)
    };

    PlacementConstraints constraints;
    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    EXPECT_EQ(candidate->node_id, "node-1");
}

TEST_F(GeoPlacementTest, SelectLeaderFromMultipleHealthyReplicas)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 5, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 3, 100),
        makeReplica("node-3", "eu-west-1", HealthStatus::HEALTHY, true, 7, 100),
    };

    PlacementConstraints constraints;
    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    // Highest priority should be selected (node-3 with priority 7)
    EXPECT_EQ(candidate->node_id, "node-3");
}

TEST_F(GeoPlacementTest, SelectLeaderPrefersBySequenceNumber)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 5, 50),
        makeReplica("node-2", "us-east-1", HealthStatus::HEALTHY, true, 5, 100),
        makeReplica("node-3", "us-east-1", HealthStatus::HEALTHY, true, 5, 75),
    };

    PlacementConstraints constraints;
    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    // Highest sequence should be selected (node-2 with seq 100)
    EXPECT_EQ(candidate->node_id, "node-2");
}

// ============================================================================
// Datacenter preference tests
// ============================================================================

TEST_F(GeoPlacementTest, PreferredDatacenterPriority)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 5, 100),
        makeReplica("node-3", "eu-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.preferred_datacenters = {"eu-west-1", "us-west-1", "us-east-1"};

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    // Should select from preferred DC (eu-west-1), not by priority
    EXPECT_EQ(candidate->node_id, "node-3");
}

TEST_F(GeoPlacementTest, ForbiddenDatacenterExclusion)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 5, 100),
    };

    PlacementConstraints constraints;
    constraints.forbidden_datacenters = {"us-west-1"};

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    EXPECT_EQ(candidate->node_id, "node-1");
}

// ============================================================================
// Health and voting status tests
// ============================================================================

TEST_F(GeoPlacementTest, RequireVoter)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, false, 0, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.require_voter = true;

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    EXPECT_EQ(candidate->node_id, "node-2");
}

TEST_F(GeoPlacementTest, HealthyOnlyFilter)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::UNHEALTHY, true, 0, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.healthy_only = true;

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    EXPECT_EQ(candidate->node_id, "node-2");
}

TEST_F(GeoPlacementTest, NoEligibleCandidate)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, false, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.require_voter = true;

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    EXPECT_FALSE(candidate.has_value());
}

// ============================================================================
// Zone affinity/anti-affinity tests
// ============================================================================

TEST_F(GeoPlacementTest, ZoneAffinity)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1a", HealthStatus::HEALTHY, true, 5, 100),
        makeReplica("node-2", "us-east-1b", HealthStatus::HEALTHY, true, 5, 100),
    };

    PlacementConstraints constraints;
    constraints.zone_affinity_zone = "us-east-1a";

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    EXPECT_EQ(candidate->node_id, "node-1");
}

TEST_F(GeoPlacementTest, ZoneAntiAffinity)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1a", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-2", "us-east-1b", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.zone_anti_affinity_zone = "us-east-1a";

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    EXPECT_EQ(candidate->node_id, "node-2");
}

// ============================================================================
// Failover candidate tests
// ============================================================================

TEST_F(GeoPlacementTest, SelectFailoverCandidateExcludesFailed)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 5, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 3, 100),
        makeReplica("node-3", "eu-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    auto candidate = placement_.selectFailoverCandidate(replicas, constraints, "node-1");

    ASSERT_TRUE(candidate.has_value());
    // node-1 should be excluded; node-2 has highest priority among remaining
    EXPECT_EQ(candidate->node_id, "node-2");
    EXPECT_NE(candidate->node_id, "node-1");
}

// ============================================================================
// Placement validation tests
// ============================================================================

TEST_F(GeoPlacementTest, ValidatePlacementHealthyReplicas)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-2", "us-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;

    auto result = placement_.validatePlacement(replicas, constraints);

    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.violations.empty());
}

TEST_F(GeoPlacementTest, ValidatePlacementRequiredDC)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.required_datacenters = {"us-east-1", "us-west-1"};

    auto result = placement_.validatePlacement(replicas, constraints);

    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.violations.size(), 0);
}

TEST_F(GeoPlacementTest, ValidatePlacementMinCopiesPerDC)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-2", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-3", "us-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    PlacementConstraints constraints;
    constraints.min_copies_per_dc = 2;

    auto result = placement_.validatePlacement(replicas, constraints);

    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.violations.size(), 0);
}

// ============================================================================
// Helper method tests
// ============================================================================

TEST_F(GeoPlacementTest, HealthyCountPerDC)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1", HealthStatus::HEALTHY, true, 0, 100),
        makeReplica("node-2", "us-east-1", HealthStatus::UNHEALTHY, true, 0, 100),
        makeReplica("node-3", "us-west-1", HealthStatus::HEALTHY, true, 0, 100),
    };

    auto counts = placement_.healthyCountPerDC(replicas);

    EXPECT_EQ(counts["us-east-1"], 1);
    EXPECT_EQ(counts["us-west-1"], 1);
}

// ============================================================================
// Complex scenarios
// ============================================================================

TEST_F(GeoPlacementTest, MultiDCMultiZoneScenario)
{
    std::vector<ReplicaInfo> replicas = {
        makeReplica("node-1", "us-east-1/az-1a", HealthStatus::HEALTHY, true, 10, 200),
        makeReplica("node-2", "us-east-1/az-1b", HealthStatus::HEALTHY, true, 8, 180),
        makeReplica("node-3", "us-west-1/az-2a", HealthStatus::HEALTHY, true, 5, 200),
        makeReplica("node-4", "eu-west-1/az-3a", HealthStatus::HEALTHY, true, 3, 150),
    };

    PlacementConstraints constraints;
    constraints.preferred_datacenters = {"eu-west-1", "us-west-1", "us-east-1"};
    constraints.zone_affinity_zone = "az-1a";

    auto candidate = placement_.selectLeaderCandidate(replicas, constraints);

    ASSERT_TRUE(candidate.has_value());
    // eu-west-1 is first in preference list
    EXPECT_EQ(candidate->datacenter, "eu-west-1/az-3a");
}

}  // namespace replication
}  // namespace themisdb
