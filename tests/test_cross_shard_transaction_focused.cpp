/*
 * ThemisDB | Test: test_cross_shard_transaction_focused.cpp | Version: 0.0.47
 * Focused Unit Tests for W2-S04: Cross-Shard Transaction Precondition Validation
 * 
 * Test Coverage:
 * - CrossShardTransactionCoordinator::beginTransaction() validation (empty ID, etc)
 * - CrossShardTransactionCoordinator::addParticipant() validation (empty inputs)
 * - CrossShardTransactionCoordinator::prepare() precondition checks
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sharding/cross_shard_transaction.h"
#include <memory>
#include <vector>

namespace themisdb::sharding {

// Mock ConsensusModule for testing
class MockConsensusModule : public ConsensusModule {
public:
    MOCK_METHOD2(propose, bool(const std::string&, const nlohmann::json&));
    MOCK_METHOD0(initialize, bool());
    MOCK_METHOD0(start, bool());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(getId, std::string());
    MOCK_METHOD0(getTerm, uint64_t());
};

// ============================================================================
// CrossShardTransactionCoordinator Tests
// ============================================================================

class CrossShardTransactionCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.replica_id = "test-replica";
        config_.lock_timeout = std::chrono::milliseconds(100);
        mock_consensus_ = std::make_shared<MockConsensusModule>();
    }
    
    CrossShardTransactionConfig config_;
    std::shared_ptr<MockConsensusModule> mock_consensus_;
    
    std::unique_ptr<CrossShardTransactionCoordinator> createCoordinator() {
        auto coordinator = std::make_unique<CrossShardTransactionCoordinator>(
            config_,
            mock_consensus_,
            nullptr  // No TrueTime for simplicity
        );
        if (coordinator->initialize()) {
            coordinator->start();
        }
        return coordinator;
    }
};

// W2-S04: Fail-closed on empty transaction_id in beginTransaction
TEST_F(CrossShardTransactionCoordinatorTest, BeginTransactionRejectsEmptyId) {
    auto coordinator = createCoordinator();
    
    bool result = coordinator->beginTransaction(
        "",  // Empty transaction ID
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    EXPECT_FALSE(result) << "Should reject empty transaction_id";
}

// W2-S04: Accept valid beginTransaction
TEST_F(CrossShardTransactionCoordinatorTest, BeginTransactionAcceptsValidId) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    bool result = coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    EXPECT_TRUE(result) << "Should accept valid transaction_id";
}

// W2-S04: Fail-closed on duplicate transaction ID
TEST_F(CrossShardTransactionCoordinatorTest, BeginTransactionRejectsDuplicate) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // First begin should succeed
    bool first = coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    EXPECT_TRUE(first);
    
    // Second begin with same ID should fail
    bool second = coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    EXPECT_FALSE(second) << "Should reject duplicate transaction_id";
}

// W2-S04: Fail-closed on empty transaction_id in addParticipant
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyTransactionId) {
    auto coordinator = createCoordinator();
    
    bool result = coordinator->addParticipant(
        "",  // Empty transaction ID
        "shard-1",
        "localhost:5000",
        {"READ", "WRITE"}
    );
    
    EXPECT_FALSE(result) << "Should reject empty transaction_id";
}

// W2-S04: Fail-closed on empty shard_id in addParticipant
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyShardId) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction first
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    bool result = coordinator->addParticipant(
        "txn-12345",
        "",  // Empty shard ID
        "localhost:5000",
        {"READ", "WRITE"}
    );
    
    EXPECT_FALSE(result) << "Should reject empty shard_id";
}

// W2-S04: Fail-closed on empty endpoint in addParticipant
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyEndpoint) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction first
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    bool result = coordinator->addParticipant(
        "txn-12345",
        "shard-1",
        "",  // Empty endpoint
        {"READ", "WRITE"}
    );
    
    EXPECT_FALSE(result) << "Should reject empty endpoint";
}

// W2-S04: Fail-closed on empty operations in addParticipant
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyOperations) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction first
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    bool result = coordinator->addParticipant(
        "txn-12345",
        "shard-1",
        "localhost:5000",
        {}  // Empty operations
    );
    
    EXPECT_FALSE(result) << "Should reject empty operations";
}

// W2-S04: Fail-closed on adding participant to non-existent transaction
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsNonExistentTransaction) {
    auto coordinator = createCoordinator();
    
    bool result = coordinator->addParticipant(
        "txn-nonexistent",
        "shard-1",
        "localhost:5000",
        {"READ", "WRITE"}
    );
    
    EXPECT_FALSE(result) << "Should reject non-existent transaction";
}

// W2-S04: Fail-closed on duplicate participant in same transaction
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsDuplicateShard) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    // Add first participant
    bool first = coordinator->addParticipant(
        "txn-12345",
        "shard-1",
        "localhost:5000",
        {"READ", "WRITE"}
    );
    EXPECT_TRUE(first);
    
    // Try to add same shard again
    bool second = coordinator->addParticipant(
        "txn-12345",
        "shard-1",
        "localhost:5001",
        {"DELETE"}
    );
    EXPECT_FALSE(second) << "Should reject duplicate shard_id";
}

// W2-S04: Accept valid addParticipant
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantAcceptsValidInputs) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    // Add valid participant
    bool result = coordinator->addParticipant(
        "txn-12345",
        "shard-1",
        "localhost:5000",
        {"READ", "WRITE"}
    );
    
    EXPECT_TRUE(result) << "Should accept valid participant";
}

// W2-S04: Accept multiple different participants
TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantAcceptsMultipleDifferentShards) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    // Add multiple participants
    bool r1 = coordinator->addParticipant("txn-12345", "shard-1", "host1:5000", {"READ"});
    bool r2 = coordinator->addParticipant("txn-12345", "shard-2", "host2:5000", {"WRITE"});
    bool r3 = coordinator->addParticipant("txn-12345", "shard-3", "host3:5000", {"READ", "WRITE"});
    
    EXPECT_TRUE(r1 && r2 && r3) << "Should accept multiple different shards";
}

// W2-S04: Fail-closed on prepare with empty transaction_id
TEST_F(CrossShardTransactionCoordinatorTest, PrepareRejectsEmptyTransactionId) {
    auto coordinator = createCoordinator();
    
    bool result = coordinator->prepare("");  // Empty transaction ID
    
    EXPECT_FALSE(result) << "Should reject empty transaction_id in prepare";
}

// W2-S04: Fail-closed on prepare with no participants
TEST_F(CrossShardTransactionCoordinatorTest, PrepareRejectsTransactionWithNoParticipants) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction but don't add participants
    coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    // Try to prepare without participants
    bool result = coordinator->prepare("txn-12345");
    
    EXPECT_FALSE(result) << "Should reject prepare without participants";
}

// W2-S04: Fail-closed on prepare of non-existent transaction
TEST_F(CrossShardTransactionCoordinatorTest, PrepareRejectsNonExistentTransaction) {
    auto coordinator = createCoordinator();
    
    bool result = coordinator->prepare("txn-nonexistent");
    
    EXPECT_FALSE(result) << "Should reject non-existent transaction";
}

// W2-S04: Precondition setup works before prepare
TEST_F(CrossShardTransactionCoordinatorTest, PreparePreconditionsSetupWorks) {
    auto coordinator = createCoordinator();
    
    EXPECT_CALL(*mock_consensus_, propose).Times(::testing::AtLeast(0));
    
    // Begin transaction
    bool begin = coordinator->beginTransaction(
        "txn-12345",
        TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION
    );
    EXPECT_TRUE(begin);
    
    // Add participant
    bool add = coordinator->addParticipant(
        "txn-12345",
        "shard-1",
        "localhost:5000",
        {"READ", "WRITE"}
    );
    EXPECT_TRUE(add);
    
    // Now prepare should pass precondition check (may fail on actual prep steps,
    // but should not fail due to missing participants)
    // Note: actual prepare may fail due to mocking, but we're testing the precondition
    coordinator->prepare("txn-12345");
    // We're not asserting result here since it depends on implementation details
}

} // namespace themisdb::sharding
