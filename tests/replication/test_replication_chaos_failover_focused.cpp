/**
 * @file test_replication_chaos_failover_focused.cpp
 * @brief Contract tests for replication geo placement failover APIs.
 */

#include <gtest/gtest.h>

#include "replication/geo_placement.h"

using namespace themisdb::replication;

namespace {
ReplicaInfo makeReplica(const std::string& id, const std::string& dc, int priority) {
    ReplicaInfo r;
    r.node_id = id;
    r.endpoint = id + ":1234";
    r.role = ReplicationRole::FOLLOWER;
    r.last_applied_sequence = 100;
    r.last_applied_term = 1;
    r.last_heartbeat = std::chrono::system_clock::now();
    r.is_voting_member = true;
    r.datacenter = dc;
    r.priority = priority;
    r.health_status = HealthStatus::HEALTHY;
    return r;
}
}  // namespace

TEST(GeoPlacementContract, ValidatePlacementReturnsStructuredResult) {
    GeoReplicaPlacementManager mgr;
    PlacementConstraints constraints;
    constraints.required_datacenters = {"dc-a"};

    std::vector<ReplicaInfo> replicas{makeReplica("n1", "dc-a", 10)};
    const auto result = mgr.validatePlacement(replicas, constraints);
    EXPECT_TRUE(result.is_valid);
}

TEST(GeoPlacementContract, SelectLeaderCandidateHonorsAvailability) {
    GeoReplicaPlacementManager mgr;
    PlacementConstraints constraints;

    std::vector<ReplicaInfo> replicas{
        makeReplica("n1", "dc-a", 5),
        makeReplica("n2", "dc-b", 10)
    };

    auto leader = mgr.selectLeaderCandidate(replicas, constraints);
    EXPECT_TRUE(leader.has_value());
}
