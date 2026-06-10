#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "replication/replication_manager.h"

/**
 * Simplified focused tests for ReplicationManager::addReplica fail-closed guard
 * 
 * Strategy: Test the guard via public API (getReplicas) without mocking
 * or complex fixture setup. Verifies that empty node_id/endpoint are rejected.
 */

using namespace themisdb::replication;

class ReplicationManagerAddReplicaTest : public ::testing::Test {
protected:
    std::shared_ptr<ReplicationManager> manager_;

    void SetUp() override {
        // Create with default ReplicationConfig
        ReplicationConfig config;
        manager_ = std::make_shared<ReplicationManager>(config);
    }
};

/**
 * Test 1: Reject empty node_id
 */
TEST_F(ReplicationManagerAddReplicaTest, RejectsEmptyNodeId) {
    ReplicaInfo replica;
    replica.node_id = "";  // Empty!
    replica.endpoint = "localhost:5432";
    replica.role = ReplicationRole::FOLLOWER;

    // Before call
    auto replicas_before = manager_->getReplicas();
    size_t size_before = replicas_before.size();

    // Call with empty node_id
    manager_->addReplica(replica);

    // After call
    auto replicas_after = manager_->getReplicas();
    size_t size_after = replicas_after.size();

    // Verify: size unchanged (guard rejected the call)
    EXPECT_EQ(size_before, size_after);
}

/**
 * Test 2: Reject empty endpoint
 */
TEST_F(ReplicationManagerAddReplicaTest, RejectsEmptyEndpoint) {
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "";  // Empty!
    replica.role = ReplicationRole::FOLLOWER;

    auto replicas_before = manager_->getReplicas();
    size_t size_before = replicas_before.size();

    manager_->addReplica(replica);

    auto replicas_after = manager_->getReplicas();
    size_t size_after = replicas_after.size();

    // Guard rejected (endpoint is empty)
    EXPECT_EQ(size_before, size_after);
}

/**
 * Test 3: Accept valid node_id and endpoint
 */
TEST_F(ReplicationManagerAddReplicaTest, AcceptsValidReplicaInfo) {
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:5432";
    replica.role = ReplicationRole::FOLLOWER;
    replica.is_voting_member = true;

    auto replicas_before = manager_->getReplicas();
    size_t size_before = replicas_before.size();

    manager_->addReplica(replica);

    auto replicas_after = manager_->getReplicas();
    size_t size_after = replicas_after.size();

    // Guard accepted (valid node_id and endpoint)
    EXPECT_EQ(size_after, size_before + 1);
    
    // Verify the replica was actually added
    bool found = false;
    for (const auto& r : replicas_after) {
        if (r.node_id == "replica-1" && r.endpoint == "localhost:5432") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

/**
 * Test 4: Multiple valid replicas can be added
 */
TEST_F(ReplicationManagerAddReplicaTest, MultipleReplicasCanBeAdded) {
    ReplicaInfo r1, r2, r3;
    
    r1.node_id = "replica-1";
    r1.endpoint = "host1:5432";
    r1.role = ReplicationRole::FOLLOWER;
    
    r2.node_id = "replica-2";
    r2.endpoint = "host2:5432";
    r2.role = ReplicationRole::FOLLOWER;
    
    r3.node_id = "replica-3";
    r3.endpoint = "host3:5432";
    r3.role = ReplicationRole::OBSERVER;

    auto size_before = manager_->getReplicas().size();

    manager_->addReplica(r1);
    manager_->addReplica(r2);
    manager_->addReplica(r3);

    auto replicas_after = manager_->getReplicas();
    EXPECT_EQ(replicas_after.size(), size_before + 3);
}

/**
 * Test 5: Guard is independent across multiple calls
 */
TEST_F(ReplicationManagerAddReplicaTest, FailClosedGuardsAreIndependent) {
    ReplicaInfo r_invalid1, r_valid, r_invalid2;

    r_invalid1.node_id = "";  // Empty
    r_invalid1.endpoint = "host1:5432";
    r_invalid1.role = ReplicationRole::FOLLOWER;

    r_valid.node_id = "replica-valid";
    r_valid.endpoint = "host-valid:5432";
    r_valid.role = ReplicationRole::FOLLOWER;

    r_invalid2.node_id = "replica-2";
    r_invalid2.endpoint = "";  // Empty
    r_invalid2.role = ReplicationRole::FOLLOWER;

    // First invalid call (empty node_id)
    manager_->addReplica(r_invalid1);
    
    // Valid call (should succeed)
    manager_->addReplica(r_valid);
    
    // Second invalid call (empty endpoint)
    manager_->addReplica(r_invalid2);

    auto replicas = manager_->getReplicas();
    
    // Expect only r_valid was added
    EXPECT_EQ(replicas.size(), 1);
    EXPECT_EQ(replicas[0].node_id, "replica-valid");
    EXPECT_EQ(replicas[0].endpoint, "host-valid:5432");
}
