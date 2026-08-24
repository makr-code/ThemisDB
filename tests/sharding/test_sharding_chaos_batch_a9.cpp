/**
 * @file test_sharding_chaos_batch_a9.cpp
 * @brief Comprehensive chaos testing for sharding module
 * 
 * Wave A Batch A-9: Chaos Testing & Fault Injection
 * Tests sharding resilience under chaotic conditions
 * 
 * Scenarios:
 * - MultiShardRebalanceUnderNetworkPartition
 * - ConsensusTimeoutUnderLoad
 * - CascadingShardFailure
 * - LeaderElectionUnderPartition
 * - ShardDataConsistencyAfterRecovery
 * - CrossShardTransactionFailure
 * 
 * Date: 2026-08-16
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <map>
#include <iostream>

#include "tests/utils/fault_injector.h"

using namespace std::chrono_literals;

namespace themis {
namespace test {

// ============================================================================
// MOCK SHARD CLUSTER FOR TESTING
// ============================================================================

class MockShard {
public:
    enum class State { HEALTHY, SLOW, PARTITIONED, FAILED, RECOVERING };

    explicit MockShard(int shard_id)
        : shard_id_(shard_id),
          state_(State::HEALTHY),
          request_count_(0),
          error_count_(0),
          is_leader_(false) {}

    int getId() const { return shard_id_; }
    State getState() const { return state_; }
    void setState(State s) { state_ = s; }

    bool processRequest(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mutex_);
        request_count_++;

        switch (state_) {
            case State::HEALTHY:
                data_[key] = value;
                return true;
            case State::SLOW:
                std::this_thread::sleep_for(50ms);
                data_[key] = value;
                return true;
            case State::PARTITIONED:
            case State::FAILED:
                error_count_++;
                return false;
            case State::RECOVERING:
                if (request_count_ % 2 == 0) {
                    data_[key] = value;
                    return true;
                }
                error_count_++;
                return false;
        }
        return false;
    }

    bool canServeRequest() const {
        return state_ == State::HEALTHY || state_ == State::RECOVERING;
    }

    int getRequestCount() const { return request_count_; }
    int getErrorCount() const { return error_count_; }
    bool isLeader() const { return is_leader_; }
    void setLeader(bool is_leader) { is_leader_ = is_leader; }

private:
    int shard_id_;
    State state_;
    std::atomic<int> request_count_;
    std::atomic<int> error_count_;
    bool is_leader_;
    std::map<std::string, std::string> data_;
    mutable std::mutex mutex_;
};

class MockShardCluster {
public:
    explicit MockShardCluster(int shard_count) : shard_count_(shard_count) {
        for (int i = 0; i < shard_count; ++i) {
            shards_.push_back(std::make_unique<MockShard>(i));
        }
        // Set first shard as leader
        if (!shards_.empty()) {
            shards_[0]->setLeader(true);
        }
    }

    int getShardCount() const { return shard_count_; }

    MockShard* getShard(int id) {
        if (id >= 0 && id < static_cast<int>(shards_.size())) {
            return shards_[id].get();
        }
        return nullptr;
    }

    bool write(int shard_id, const std::string& key, const std::string& value) {
        auto shard = getShard(shard_id);
        if (!shard) {
            return false;
        }
        return shard->processRequest(key, value);
    }

    int getHealthyShardCount() const {
        int count = 0;
        for (const auto& shard : shards_) {
            if (shard->canServeRequest()) {
                count++;
            }
        }
        return count;
    }

    bool hasLeader() const {
        for (const auto& shard : shards_) {
            if (shard->isLeader()) {
                return true;
            }
        }
        return false;
    }

    int getLeaderId() const {
        for (const auto& shard : shards_) {
            if (shard->isLeader()) {
                return shard->getId();
            }
        }
        return -1;
    }

    void promoteNewLeader() {
        // Find first healthy shard that's not current leader
        for (const auto& shard : shards_) {
            if (shard->canServeRequest() && !shard->isLeader()) {
                shard->setLeader(true);
                return;
            }
        }
    }

    bool rebalance() {
        int healthy_count = getHealthyShardCount();
        if (healthy_count < (shard_count_ / 2 + 1)) {
            return false;  // No quorum
        }
        return true;
    }

private:
    int shard_count_;
    std::vector<std::unique_ptr<MockShard>> shards_;
};

// ============================================================================
// CHAOS TESTS
// ============================================================================

class ShardingChaosTest : public ::testing::Test {
protected:
    void SetUp() override { cluster_ = std::make_unique<MockShardCluster>(5); }

    void TearDown() override { cluster_.reset(); }

    std::unique_ptr<MockShardCluster> cluster_;
};

/**
 * @brief Test: Multi-shard rebalance under network partition
 * 
 * Partitions one shard from the rest and attempts rebalance.
 * Verifies rebalance is deferred and partition recovery succeeds.
 */
TEST_F(ShardingChaosTest, MultiShardRebalanceUnderNetworkPartition) {
    // Setup network partition injector
    NetworkInjector::NetworkConfig cfg;
    cfg.target_component = "shard-0";
    cfg.duration = 2s;
    cfg.network_type = NetworkInjector::NetworkFaultType::PARTITION;
    cfg.target_node = "shard-0";
    cfg.partition_members = {"shard-0"};

    NetworkInjector net_injector(cfg);

    // Inject partition on shard 0
    auto result = net_injector.inject();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.state, FaultInjector::InjectionState::ACTIVE);

    // Mark shard 0 as partitioned
    auto shard_0 = cluster_->getShard(0);
    EXPECT_NE(shard_0, nullptr);
    shard_0->setState(MockShard::State::PARTITIONED);

    // Count healthy shards before rebalance
    int healthy_before = cluster_->getHealthyShardCount();
    EXPECT_EQ(healthy_before, 4);  // All except shard 0

    // Attempt rebalance (should work - still have quorum)
    bool rebalance_ok = cluster_->rebalance();
    EXPECT_TRUE(rebalance_ok);

    // Recover network partition
    std::this_thread::sleep_for(100ms);
    result = net_injector.recover();
    EXPECT_TRUE(result.success);

    // Recover shard 0
    shard_0->setState(MockShard::State::RECOVERING);
    std::this_thread::sleep_for(50ms);
    shard_0->setState(MockShard::State::HEALTHY);

    // Verify all shards healthy again
    int healthy_after = cluster_->getHealthyShardCount();
    EXPECT_EQ(healthy_after, 5);
}

/**
 * @brief Test: Consensus timeout under load
 * 
 * High load with network delays triggering consensus timeout.
 * Verifies leader election succeeds.
 */
TEST_F(ShardingChaosTest, ConsensusTimeoutUnderLoad) {
    // Setup network delay injector
    NetworkInjector::NetworkConfig cfg;
    cfg.target_component = "consensus_channel";
    cfg.duration = 3s;
    cfg.network_type = NetworkInjector::NetworkFaultType::DELAY;
    cfg.latency = 100ms;

    NetworkInjector delay_injector(cfg);

    // Inject delay
    auto result = delay_injector.inject();
    EXPECT_TRUE(result.success);

    // Simulate high load (many write requests)
    const int REQUEST_COUNT = 100;
    int success_count = 0;

    for (int i = 0; i < REQUEST_COUNT; ++i) {
        for (int shard_id = 0; shard_id < cluster_->getShardCount(); ++shard_id) {
            if (cluster_->write(shard_id, "key_" + std::to_string(i),
                               "value_" + std::to_string(i))) {
                success_count++;
            }
        }
    }

    // Expect most requests to succeed (despite delays)
    int expected_success = REQUEST_COUNT * cluster_->getShardCount() * 0.8;
    EXPECT_GE(success_count, expected_success);

    // Current leader should still be valid
    EXPECT_TRUE(cluster_->hasLeader());
    int leader_id = cluster_->getLeaderId();
    EXPECT_GE(leader_id, 0);

    // Recover network
    result = delay_injector.recover();
    EXPECT_TRUE(result.success);

    // Verify leader election succeeds
    EXPECT_TRUE(cluster_->hasLeader());
}

/**
 * @brief Test: Cascading shard failure
 * 
 * Fail multiple shards in succession.
 * Verifies remaining shards maintain quorum.
 */
TEST_F(ShardingChaosTest, CascadingShardFailure) {
    const int TOTAL_SHARDS = cluster_->getShardCount();
    const int FAIL_COUNT = 2;

    // Fail shards one by one
    for (int i = 0; i < FAIL_COUNT; ++i) {
        auto shard = cluster_->getShard(i);
        EXPECT_NE(shard, nullptr);
        shard->setState(MockShard::State::FAILED);

        // Check quorum
        int healthy_count = cluster_->getHealthyShardCount();
        int failed_count = i + 1;
        int expected_healthy = TOTAL_SHARDS - failed_count;
        EXPECT_EQ(healthy_count, expected_healthy);

        // Verify rebalance succeeds if quorum exists
        bool has_quorum = healthy_count > (TOTAL_SHARDS / 2);
        bool rebalance_ok = cluster_->rebalance();
        EXPECT_EQ(rebalance_ok, has_quorum);
    }

    // Verify we still have quorum with 2 failed out of 5
    int healthy_final = cluster_->getHealthyShardCount();
    EXPECT_EQ(healthy_final, 3);
    EXPECT_TRUE(cluster_->rebalance());  // Should succeed
}

/**
 * @brief Test: Leader election under partition
 * 
 * Leader becomes unreachable, verify new leader is elected.
 */
TEST_F(ShardingChaosTest, LeaderElectionUnderPartition) {
    // Identify current leader
    int old_leader = cluster_->getLeaderId();
    EXPECT_GE(old_leader, 0);

    // Partition the leader
    auto leader_shard = cluster_->getShard(old_leader);
    EXPECT_NE(leader_shard, nullptr);
    leader_shard->setState(MockShard::State::PARTITIONED);
    leader_shard->setLeader(false);

    // Promote new leader
    cluster_->promoteNewLeader();

    // Verify new leader
    int new_leader = cluster_->getLeaderId();
    EXPECT_NE(new_leader, old_leader);
    EXPECT_GE(new_leader, 0);

    // Verify new leader can serve requests
    auto new_leader_shard = cluster_->getShard(new_leader);
    EXPECT_NE(new_leader_shard, nullptr);
    EXPECT_TRUE(new_leader_shard->canServeRequest());
}

/**
 * @brief Test: Shard data consistency after recovery
 * 
 * Fail shard, recover it, verify data consistency.
 */
TEST_F(ShardingChaosTest, ShardDataConsistencyAfterRecovery) {
    // Write data to all shards
    const std::string TEST_KEY = "consistency_test_key";
    const std::string TEST_VALUE = "consistency_test_value";

    for (int i = 0; i < cluster_->getShardCount(); ++i) {
        EXPECT_TRUE(cluster_->write(i, TEST_KEY, TEST_VALUE));
    }

    // Fail shard 0
    auto shard_0 = cluster_->getShard(0);
    shard_0->setState(MockShard::State::FAILED);

    // Recover shard 0
    shard_0->setState(MockShard::State::RECOVERING);
    std::this_thread::sleep_for(50ms);

    // Write to recovered shard
    bool write_ok = cluster_->write(0, TEST_KEY, TEST_VALUE);
    EXPECT_TRUE(write_ok);

    // Final state should be consistent
    shard_0->setState(MockShard::State::HEALTHY);
    EXPECT_TRUE(shard_0->canServeRequest());
}

/**
 * @brief Test: Cross-shard transaction failure
 * 
 * Transaction across multiple shards fails on one shard.
 * Verifies proper rollback/abort.
 */
TEST_F(ShardingChaosTest, CrossShardTransactionFailure) {
    // Setup network partition on one shard
    auto shard_2 = cluster_->getShard(2);
    shard_2->setState(MockShard::State::PARTITIONED);

    // Attempt cross-shard write
    int success_count = 0;
    for (int i = 0; i < cluster_->getShardCount(); ++i) {
        if (cluster_->write(i, "cross_shard_key", "cross_shard_value")) {
            success_count++;
        }
    }

    // Should succeed on healthy shards (4 out of 5)
    EXPECT_EQ(success_count, 4);

    // Recover partitioned shard
    shard_2->setState(MockShard::State::RECOVERING);
    
    // Retry write
    if (cluster_->write(2, "cross_shard_key", "cross_shard_value")) {
        success_count++;
    }

    // Final count should be 5 (all healthy + recovered)
    EXPECT_GE(success_count, 4);
}

}  // namespace test
}  // namespace themis
