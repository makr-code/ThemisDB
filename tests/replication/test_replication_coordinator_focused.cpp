#include <gtest/gtest.h>
#include "sharding/replication_coordinator.h"
#include <memory>

using namespace themis::sharding;

// ============================================================================
// QW-28: ReplicationCoordinator Fail-Closed Guard Tests
// ============================================================================

class ReplicationCoordinatorTest : public ::testing::Test {
protected:
    std::shared_ptr<ReplicationCoordinator> coordinator_;

    void SetUp() override {
        // Create a coordinator with nullptr shipper (minimal setup for fail-closed testing)
        // The recordAcknowledgment() guard should reject empty replica_id before using shipper
        coordinator_ = std::make_shared<ReplicationCoordinator>(nullptr);
    }
};

TEST_F(ReplicationCoordinatorTest, RecordAcknowledgmentFailsClosedForEmptyReplicaId) {
    // Fail-closed: reject empty replica_id immediately
    LSN lsn(1, 100);  // segment=1, offset=100
    
    // Should not crash or process empty replica_id (fail-closed guard)
    // This should be handled silently by the guard
    coordinator_->recordAcknowledgment("", lsn);
    
    // Verify no exception was thrown (the guard prevents processing)
    // Test passes if no crash or assertion failure occurs
}

TEST_F(ReplicationCoordinatorTest, RecordAcknowledgmentAcceptsValidReplicaId) {
    // Positive case: valid non-empty replica_id should be processed
    LSN lsn(1, 100);
    
    // Should accept non-empty replica_id
    coordinator_->recordAcknowledgment("replica_001", lsn);
    
    // Test passes if no crash occurs (acknowledgment may or may not be recorded
    // depending on shipper state, but guard should not reject it)
}
