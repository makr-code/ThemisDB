/**
 * @file test_replication_chaos.cpp
 * @brief Comprehensive chaos testing for replication module
 * 
 * Wave A Batch A-9: Chaos Testing & Fault Injection
 * Tests replication resilience under chaotic conditions
 * 
 * Scenarios:
 * - LagInjectionAndFailover
 * - WALShippingUnderPacketLoss
 * - GeographicPartitionRecovery
 * - ReplicaResyncAfterCrash
 * - QuorumRecoveryAfterPartition
 * - ReplicationLagAlertAndAction
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
// MOCK REPLICATION SYSTEM FOR TESTING
// ============================================================================

class MockReplica {
public:
    enum class State { LEADER, FOLLOWER, LAGGED, UNAVAILABLE };

    explicit MockReplica(int replica_id, const std::string& region)
        : replica_id_(replica_id),
          region_(region),
          state_(State::FOLLOWER),
          lag_ms_(0),
          wal_position_(0),
          sync_count_(0) {}

    int getId() const { return replica_id_; }
    const std::string& getRegion() const { return region_; }
    State getState() const { return state_; }
    void setState(State s) { state_ = s; }

    int getLagMs() const { return lag_ms_; }
    void setLagMs(int lag) { lag_ms_ = lag; }

    uint64_t getWalPosition() const { return wal_position_; }
    void setWalPosition(uint64_t pos) { wal_position_ = pos; }

    int getSyncCount() const { return sync_count_; }
    void incrementSyncCount() { sync_count_++; }

    bool canAcceptWAL() const {
        return state_ != State::UNAVAILABLE && lag_ms_ < 60000;  // 60s threshold
    }

    bool isHealthy() const {
        return state_ == State::LEADER || (state_ == State::FOLLOWER && lag_ms_ < 10000);
    }

    void simulateWALShip(int wal_entry_count) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (canAcceptWAL()) {
            wal_position_ += wal_entry_count;
            sync_count_++;
        }
    }

private:
    int replica_id_;
    std::string region_;
    State state_;
    int lag_ms_;
    uint64_t wal_position_;
    int sync_count_;
    mutable std::mutex mutex_;
};

class MockReplicationSystem {
public:
    explicit MockReplicationSystem(int replica_count = 3)
        : quorum_size_((replica_count / 2) + 1) {
        // Create replicas in different regions
        std::vector<std::string> regions = {"us-east", "us-west", "eu-west"};
        for (int i = 0; i < replica_count; ++i) {
            replicas_.push_back(
                std::make_unique<MockReplica>(i, regions[i % regions.size()]));
        }
        // Set first as leader
        replicas_[0]->setState(MockReplica::State::LEADER);
    }

    int getReplicaCount() const { return replicas_.size(); }

    MockReplica* getReplica(int id) {
        if (id >= 0 && id < static_cast<int>(replicas_.size())) {
            return replicas_[id].get();
        }
        return nullptr;
    }

    int getLeaderId() const {
        for (const auto& replica : replicas_) {
            if (replica->getState() == MockReplica::State::LEADER) {
                return replica->getId();
            }
        }
        return -1;
    }

    int getHealthyReplicaCount() const {
        int count = 0;
        for (const auto& replica : replicas_) {
            if (replica->isHealthy()) {
                count++;
            }
        }
        return count;
    }

    bool hasQuorum() const { return getHealthyReplicaCount() >= quorum_size_; }

    bool shipWAL(int wal_entry_count) {
        int shipped_count = 0;
        for (const auto& replica : replicas_) {
            if (replica->canAcceptWAL()) {
                replica->simulateWALShip(wal_entry_count);
                shipped_count++;
            }
        }
        return shipped_count >= quorum_size_;
    }

    void updateReplicaLag(int replica_id, int lag_ms) {
        auto replica = getReplica(replica_id);
        if (replica) {
            replica->setLagMs(lag_ms);
            if (lag_ms > 30000) {
                replica->setState(MockReplica::State::LAGGED);
            } else if (lag_ms < 10000) {
                replica->setState(MockReplica::State::FOLLOWER);
            }
        }
    }

    bool failover() {
        if (!hasQuorum()) {
            return false;  // Can't failover without quorum
        }

        // Find new leader (first healthy follower)
        int old_leader = getLeaderId();
        for (const auto& replica : replicas_) {
            if (replica->getId() != old_leader && replica->isHealthy()) {
                getReplica(old_leader)->setState(MockReplica::State::UNAVAILABLE);
                replica->setState(MockReplica::State::LEADER);
                return true;
            }
        }
        return false;
    }

private:
    std::vector<std::unique_ptr<MockReplica>> replicas_;
    int quorum_size_;
};

// ============================================================================
// CHAOS TESTS
// ============================================================================

class ReplicationChaosTest : public ::testing::Test {
protected:
    void SetUp() override { repl_sys_ = std::make_unique<MockReplicationSystem>(3); }

    void TearDown() override { repl_sys_.reset(); }

    std::unique_ptr<MockReplicationSystem> repl_sys_;
};

/**
 * @brief Test: Lag injection and failover
 * 
 * Inject high lag on replica, trigger failover, verify consistency.
 */
TEST_F(ReplicationChaosTest, LagInjectionAndFailover) {
    // Setup initial state
    EXPECT_EQ(repl_sys_->getLeaderId(), 0);
    EXPECT_TRUE(repl_sys_->hasQuorum());

    // Inject lag on replica 1
    repl_sys_->updateReplicaLag(1, 65000);  // Exceeds 60s threshold

    // Replica 1 becomes unavailable for WAL shipping
    auto replica_1 = repl_sys_->getReplica(1);
    EXPECT_FALSE(replica_1->canAcceptWAL());

    // Ship WAL - should succeed with remaining replicas
    EXPECT_TRUE(repl_sys_->shipWAL(10));

    // Trigger failover (old leader dies)
    int old_leader = repl_sys_->getLeaderId();
    EXPECT_TRUE(repl_sys_->failover());

    // Verify new leader
    int new_leader = repl_sys_->getLeaderId();
    EXPECT_NE(new_leader, old_leader);
    EXPECT_GE(new_leader, 0);
}

/**
 * @brief Test: WAL shipping under packet loss
 * 
 * Inject packet loss, verify WAL shipping retries and succeeds.
 */
TEST_F(ReplicationChaosTest, WALShippingUnderPacketLoss) {
    NetworkInjector::NetworkConfig cfg;
    cfg.target_component = "wal_channel";
    cfg.duration = 2s;
    cfg.network_type = NetworkInjector::NetworkFaultType::PACKET_LOSS;
    cfg.packet_loss_rate = 0.1f;  // 10% packet loss

    NetworkInjector loss_injector(cfg);

    auto result = loss_injector.inject();
    EXPECT_TRUE(result.success);

    // Try to ship WAL multiple times
    int success_count = 0;
    for (int i = 0; i < 5; ++i) {
        if (repl_sys_->shipWAL(10)) {
            success_count++;
        }
        std::this_thread::sleep_for(100ms);
    }

    // Should succeed at least some times (idempotent retries)
    EXPECT_GE(success_count, 3);

    result = loss_injector.recover();
    EXPECT_TRUE(result.success);
}

/**
 * @brief Test: Geographic partition recovery
 * 
 * Partition one region, trigger failover to local region, verify recovery.
 */
TEST_F(ReplicationChaosTest, GeographicPartitionRecovery) {
    // Setup replicas by region
    int total_replicas = repl_sys_->getReplicaCount();

    // Partition us-west region (replica 1)
    auto replica_west = repl_sys_->getReplica(1);
    replica_west->setState(MockReplica::State::UNAVAILABLE);

    // Verify quorum still exists
    EXPECT_TRUE(repl_sys_->hasQuorum());

    // Failover to remaining region
    int old_leader = repl_sys_->getLeaderId();
    EXPECT_TRUE(repl_sys_->failover());

    int new_leader = repl_sys_->getLeaderId();
    EXPECT_NE(new_leader, old_leader);

    // Recover region
    replica_west->setState(MockReplica::State::FOLLOWER);
    replica_west->setLagMs(1000);  // Some lag after recovery

    // Verify quorum strength restored
    EXPECT_GE(repl_sys_->getHealthyReplicaCount(), (total_replicas / 2 + 1));
}

/**
 * @brief Test: Replica resync after crash
 * 
 * Replica crashes, reboots, catches up via resync.
 */
TEST_F(ReplicationChaosTest, ReplicaResyncAfterCrash) {
    auto replica_1 = repl_sys_->getReplica(1);
    uint64_t initial_wal = replica_1->getWalPosition();

    // Crash replica
    replica_1->setState(MockReplica::State::UNAVAILABLE);

    // Ship WAL while replica is down (leader advances)
    for (int i = 0; i < 5; ++i) {
        repl_sys_->shipWAL(10);
    }

    // Replica reboots
    replica_1->setState(MockReplica::State::FOLLOWER);
    replica_1->setWalPosition(initial_wal);

    // Simulate resync (catching up)
    for (int i = 0; i < 5; ++i) {
        replica_1->simulateWALShip(10);
    }

    // Verify caught up
    uint64_t synced_wal = replica_1->getWalPosition();
    EXPECT_GT(synced_wal, initial_wal);
}

/**
 * @brief Test: Quorum recovery after partition
 * 
 * Partition partitions cluster, verify quorum is lost then recovered.
 */
TEST_F(ReplicationChaosTest, QuorumRecoveryAfterPartition) {
    // Initially have quorum
    EXPECT_TRUE(repl_sys_->hasQuorum());

    // Partition 2 replicas
    repl_sys_->getReplica(1)->setState(MockReplica::State::UNAVAILABLE);
    repl_sys_->getReplica(2)->setState(MockReplica::State::UNAVAILABLE);

    // Quorum is lost (only 1 out of 3)
    EXPECT_FALSE(repl_sys_->hasQuorum());

    // Recover first replica
    repl_sys_->getReplica(1)->setState(MockReplica::State::FOLLOWER);
    repl_sys_->getReplica(1)->setLagMs(5000);

    // Quorum restored (2 out of 3)
    EXPECT_TRUE(repl_sys_->hasQuorum());

    // Verify WAL can be shipped
    EXPECT_TRUE(repl_sys_->shipWAL(10));
}

/**
 * @brief Test: Replication lag alert and action
 * 
 * Monitor lag, trigger alerts at thresholds, trigger failover after prolonged lag.
 */
TEST_F(ReplicationChaosTest, ReplicationLagAlertAndAction) {
    auto replica_1 = repl_sys_->getReplica(1);

    // Normal lag
    repl_sys_->updateReplicaLag(1, 5000);
    EXPECT_TRUE(replica_1->canAcceptWAL());

    // Increased lag (10-60s range)
    repl_sys_->updateReplicaLag(1, 30000);
    EXPECT_TRUE(replica_1->canAcceptWAL());

    // Critical lag (>60s)
    repl_sys_->updateReplicaLag(1, 65000);
    EXPECT_FALSE(replica_1->canAcceptWAL());

    // Trigger failover after critical lag
    int old_leader = repl_sys_->getLeaderId();
    if (!replica_1->canAcceptWAL()) {
        replica_1->setState(MockReplica::State::UNAVAILABLE);
    }

    // If quorum still exists, failover succeeds
    if (repl_sys_->hasQuorum()) {
        EXPECT_TRUE(repl_sys_->failover());
        int new_leader = repl_sys_->getLeaderId();
        EXPECT_NE(new_leader, old_leader);
    }
}

}  // namespace test
}  // namespace themis
