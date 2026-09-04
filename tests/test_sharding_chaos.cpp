/**
 * @file test_sharding_chaos.cpp
 * @brief Chaos engineering tests for sharded database
 * 
 * Tests system resilience under chaotic conditions:
 * - Network partitions
 * - Cascading failures
 * - Split-brain scenarios
 * - Random failure injection
 * - Byzantine faults
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

using namespace std::chrono_literals;

namespace themis {
namespace test {

/**
 * @brief Mock distributed shard for chaos testing
 */
class ChaosShard {
public:
    enum class State {
        HEALTHY,
        SLOW,
        PARTITIONED,
        FAILED,
        RECOVERING
    };
    
    explicit ChaosShard(int id) 
        : shard_id_(id), state_(State::HEALTHY), 
          request_count_(0), error_count_(0),
          mutex_(std::make_unique<std::mutex>()) {}
    
    // Non-copyable but movable
    ChaosShard(const ChaosShard&) = delete;
    ChaosShard& operator=(const ChaosShard&) = delete;
    
    ChaosShard(ChaosShard&& other) noexcept
        : shard_id_(other.shard_id_),
          state_(other.state_),
          request_count_(other.request_count_.load()),
          error_count_(other.error_count_.load()),
          mutex_(std::move(other.mutex_)) {}
    
    ChaosShard& operator=(ChaosShard&& other) noexcept {
        if (this != &other) {
            shard_id_ = other.shard_id_;
            state_ = other.state_;
            request_count_.store(other.request_count_.load());
            error_count_.store(other.error_count_.load());
            mutex_ = std::move(other.mutex_);
        }
        return *this;
    }
    
    bool processRequest(const std::string& /*request*/) {
        std::lock_guard<std::mutex> lock(*mutex_);
        request_count_++;
        
        switch (state_) {
            case State::HEALTHY:
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                return true;
                
            case State::SLOW:
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return true;
                
            case State::PARTITIONED:
            case State::FAILED:
                error_count_++;
                return false;
                
            case State::RECOVERING:
                // 50% success rate during recovery
                if (request_count_ % 2 == 0) {
                    return true;
                }
                error_count_++;
                return false;
        }
        return false;
    }
    
    void setState(State new_state) {
        std::lock_guard<std::mutex> lock(*mutex_);
        state_ = new_state;
    }
    
    State getState() const {
        std::lock_guard<std::mutex> lock(*mutex_);
        return state_;
    }
    
    int getRequestCount() const { return request_count_; }
    int getErrorCount() const { return error_count_; }
    int getId() const { return shard_id_; }
    
private:
    int shard_id_;
    State state_;
    std::atomic<int> request_count_;
    std::atomic<int> error_count_;
    mutable std::unique_ptr<std::mutex> mutex_;
};

/**
 * @brief Test network partition between shards
 */
TEST(ShardingChaosTest, NetworkPartitionScenario) {
    constexpr int NUM_SHARDS = 6;
    std::vector<ChaosShard> shards;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Partition network: shards 0-2 in one partition, 3-5 in another
    for (int i = 0; i < 3; ++i) {
        shards[i].setState(ChaosShard::State::HEALTHY);
    }
    for (int i = 3; i < 6; ++i) {
        shards[i].setState(ChaosShard::State::PARTITIONED);
    }
    
    // Try operations on both partitions
    int partition_a_success = 0;
    int partition_b_success = 0;
    
    for (int i = 0; i < 10; ++i) {
        if (shards[0].processRequest("test")) {
          partition_a_success++;
        }
        if (shards[3].processRequest("test")) {
          partition_b_success++;
        }
    }
    
    // Partition A should work, B should fail
    EXPECT_GT(partition_a_success, 0);
    EXPECT_EQ(partition_b_success, 0);
    
    // Heal partition
    for (int i = 3; i < 6; ++i) {
        shards[i].setState(ChaosShard::State::RECOVERING);
    }
    
    // Operations should start working again
    partition_b_success = 0;
    for (int i = 0; i < 20; ++i) {
        if (shards[3].processRequest("test")) {
          partition_b_success++;
        }
    }
    EXPECT_GT(partition_b_success, 0);
}

/**
 * @brief Test cascading failure scenario
 */
TEST(ShardingChaosTest, CascadingFailureHandling) {
    constexpr int NUM_SHARDS = 5;
    std::vector<ChaosShard> shards;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Initial state: all healthy
    int initial_success = 0;
    for (auto& shard : shards) {
        if (shard.processRequest("test")) {
          initial_success++;
        }
    }
    EXPECT_EQ(initial_success, NUM_SHARDS);
    
    // Simulate cascading failures
    std::vector<int> failure_order = {0, 2, 4};
    
    for (int shard_id : failure_order) {
        shards[shard_id].setState(ChaosShard::State::FAILED);
        
        // Check remaining healthy shards
        int healthy_count = 0;
        for (const auto& shard : shards) {
            if (shard.getState() == ChaosShard::State::HEALTHY) {
                healthy_count++;
            }
        }
        
        // System should still be operational with remaining shards
        if (healthy_count >= NUM_SHARDS / 2) {
            int success_count = 0;
            for (auto& shard : shards) {
                if (shard.processRequest("test")) {
                  success_count++;
                }
            }
            EXPECT_GT(success_count, 0);
        }
    }
}

/**
 * @brief Test random failure injection
 */
TEST(ShardingChaosTest, RandomFailureInjection) {
    constexpr int NUM_SHARDS = 8;
    constexpr int NUM_ITERATIONS = 50;
    constexpr double FAILURE_PROBABILITY = 0.2;
    
    std::vector<ChaosShard> shards = {};

    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::bernoulli_distribution failure_dis(FAILURE_PROBABILITY);
    
    int total_requests = 0;
    int successful_requests = 0;
    
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        // Randomly fail/recover shards
        for (auto& shard : shards) {
            if (failure_dis(gen)) {
                if (shard.getState() == ChaosShard::State::HEALTHY) {
                    shard.setState(ChaosShard::State::FAILED);
                } else if (shard.getState() == ChaosShard::State::FAILED) {
                    shard.setState(ChaosShard::State::HEALTHY);
                }
            }
        }
        
        // Send requests to all shards
        for (auto& shard : shards) {
            total_requests++;
            if (shard.processRequest("test")) {
                successful_requests++;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Some requests should succeed despite chaos
    EXPECT_GT(successful_requests, 0);
    EXPECT_LT(successful_requests, total_requests); // Some should fail
    
    double success_rate = static_cast<double>(successful_requests) / total_requests;
    EXPECT_GT(success_rate, 0.3); // At least 30% success rate
}

/**
 * @brief Test split-brain scenario
 */
TEST(ShardingChaosTest, SplitBrainScenario) {
    constexpr int NUM_SHARDS = 6;
    std::vector<ChaosShard> shards;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Create 3-3 split
    std::vector<bool> partition_a = {true, true, true, false, false, false};
    
    // Set partition states
    for (size_t i = 0; i < shards.size(); ++i) {
        if (partition_a[i]) {
            shards[i].setState(ChaosShard::State::HEALTHY);
        } else {
            shards[i].setState(ChaosShard::State::PARTITIONED);
        }
    }
    
    // Count operational shards in each partition
    int partition_a_count = 0;
    int partition_b_count = 0;
    
    for (size_t i = 0; i < shards.size(); ++i) {
        if (partition_a[i] && shards[i].getState() == ChaosShard::State::HEALTHY) {
            partition_a_count++;
        } else if (!partition_a[i] && shards[i].getState() == ChaosShard::State::PARTITIONED) {
            partition_b_count++;
        }
    }
    
    EXPECT_EQ(partition_a_count, 3);
    EXPECT_EQ(partition_b_count, 3);
    
    // Neither partition has majority (need 4/6)
    EXPECT_LT(partition_a_count, NUM_SHARDS / 2 + 1);
    EXPECT_LT(partition_b_count, NUM_SHARDS / 2 + 1);
}

/**
 * @brief Test recovery from multiple simultaneous failures
 */
TEST(ShardingChaosTest, MultipleSimultaneousFailures) {
    constexpr int NUM_SHARDS = 10;
    std::vector<ChaosShard> shards;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Fail multiple shards simultaneously
    std::vector<int> failed_shards = {1, 3, 5, 7};
    for (int shard_id : failed_shards) {
        shards[shard_id].setState(ChaosShard::State::FAILED);
    }
    
    // System should still be operational
    int successful_ops = 0;
    for (auto& shard : shards) {
        if (shard.processRequest("test")) {
            successful_ops++;
        }
    }
    
    EXPECT_GT(successful_ops, 0);
    EXPECT_EQ(successful_ops, NUM_SHARDS - failed_shards.size());
    
    // Start recovery
    for (int shard_id : failed_shards) {
        shards[shard_id].setState(ChaosShard::State::RECOVERING);
    }
    
    // Count operations during recovery
    int recovery_ops = 0;
    for (int i = 0; i < 20; ++i) {
        for (auto& shard : shards) {
            if (shard.processRequest("test")) {
                recovery_ops++;
            }
        }
    }
    
    EXPECT_GT(recovery_ops, successful_ops);
}

/**
 * @brief Test slow shard performance degradation
 */
TEST(ShardingChaosTest, SlowShardPerformanceDegradation) {
    constexpr int NUM_SHARDS = 4;
    std::vector<ChaosShard> shards;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Measure baseline performance
    auto baseline_start = std::chrono::steady_clock::now();
    for (auto& shard : shards) {
        shard.processRequest("test");
    }
    auto baseline_duration = std::chrono::steady_clock::now() - baseline_start;
    
    // Make one shard slow
    shards[1].setState(ChaosShard::State::SLOW);
    
    // Measure degraded performance
    auto degraded_start = std::chrono::steady_clock::now();
    for (auto& shard : shards) {
        shard.processRequest("test");
    }
    auto degraded_duration = std::chrono::steady_clock::now() - degraded_start;
    
    // Degraded should be slower
    EXPECT_GT(degraded_duration, baseline_duration);
}

/**
 * @brief Test Byzantine fault tolerance
 */
TEST(ShardingChaosTest, ByzantineFaultTolerance) {
    constexpr int NUM_SHARDS = 7;
    constexpr int NUM_BYZANTINE = 2; // Can tolerate up to (n-1)/3 Byzantine faults
    
    struct ByzantineShard : public ChaosShard {
        bool is_byzantine = false;
        
        explicit ByzantineShard(int id) : ChaosShard(id) {}
        
        bool processRequestWithVerification(const std::string& request) {
            bool result = processRequest(request);
            
            // Byzantine shard returns false information
            if (is_byzantine) {
                return !result;
            }
            return result;
        }
    };
    
    std::vector<ByzantineShard> shards = {};

    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Mark some shards as Byzantine
    shards[2].is_byzantine = true;
    shards[5].is_byzantine = true;
    
    // Collect responses
    std::vector<bool> responses = {};

    for (auto& shard : shards) {
        responses.push_back(shard.processRequestWithVerification("test"));
    }
    
    // Count true/false responses
    int true_count = 0;
    int false_count = 0;
    for (bool resp : responses) {
        if (resp) {
          true_count++;
        }
        else false_count++;
    }
    
    // Majority should still be correct (5 honest vs 2 Byzantine)
    EXPECT_GT(true_count, false_count);
    EXPECT_GE(true_count, NUM_SHARDS - NUM_BYZANTINE);
}

/**
 * @brief Test partial network connectivity
 */
TEST(ShardingChaosTest, PartialNetworkConnectivity) {
    constexpr int NUM_SHARDS = 5;
    
    struct ConnectedShard {
        ChaosShard shard;
        std::vector<bool> can_reach; // Which other shards it can reach
        
        explicit ConnectedShard(int id) : shard(id), can_reach(NUM_SHARDS, true) {}
    };
    
    std::vector<ConnectedShard> shards = {};

    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    // Create partial connectivity: shard 2 can't reach shards 3 and 4
    shards[2].can_reach[3] = false;
    shards[2].can_reach[4] = false;
    shards[3].can_reach[2] = false;
    shards[4].can_reach[2] = false;
    
    // Test communication
    int successful_communications = 0;
    int failed_communications = 0;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        for (int j = 0; j < NUM_SHARDS; ++j) {
            if (i != j) {
                if (shards[i].can_reach[j]) {
                    successful_communications++;
                } else {
                    failed_communications++;
                }
            }
        }
    }
    
    EXPECT_GT(successful_communications, 0);
    EXPECT_GT(failed_communications, 0);
    EXPECT_EQ(failed_communications, 4); // 2->3, 2->4, 3->2, 4->2
}

} // namespace test
} // namespace themis
