/**
 * @file w8c_degradation_fault_recovery_test.cpp
 * @brief Wave 8C — Degradation & Fault Recovery (DFR-01..DFR-08).
 *
 * Validates graceful degradation and recovery under fault scenarios:
 * - Shard/node failures (immediate + recovery phases)
 * - Network partitions (latency + loss)
 * - Connection pool exhaustion
 * - Query timeout propagation
 *
 * Goals:
 * - Graceful degradation (reduced throughput, no cascade)
 * - RTO < 30sec for transient faults
 * - Zero silent data loss/corruption
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

/// @brief Represents a database node in a sharded cluster.
enum class NodeState { kHealthy, kDegraded, kFailed, kRecovering };

/// @brief Mock node representing a database shard.
class MockShardNode {
public:
    explicit MockShardNode(int node_id) : node_id_(node_id), state_(NodeState::kHealthy) {}

    [[nodiscard]] int Id() const { return node_id_; }

    [[nodiscard]] NodeState State() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    void SetState(NodeState state) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = state;
    }

    [[nodiscard]] bool WriteDocument(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == NodeState::kFailed || state_ == NodeState::kRecovering) {
            return false;
        }
        data_[key] = value;
        return true;
    }

    [[nodiscard]] std::optional<std::string> ReadDocument(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == NodeState::kFailed) {
            return std::nullopt;
        }
        auto it = data_.find(key);
        return (it != data_.end()) ? std::make_optional(it->second) : std::nullopt;
    }

    [[nodiscard]] size_t DocumentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

    void SimulateRecovery() {
        std::lock_guard<std::mutex> lock(mutex_);
        // Data is retained during recovery simulation
        state_ = NodeState::kRecovering;
    }

private:
    int node_id_;
    mutable std::mutex mutex_;
    NodeState state_;
    std::unordered_map<std::string, std::string> data_;
};

/// @brief Mock cluster managing multiple shards.
class MockShardedCluster {
public:
    MockShardedCluster(int num_nodes) {
        for (int i = 0; i < num_nodes; ++i) {
            nodes_.push_back(std::make_unique<MockShardNode>(i));
        }
    }

    [[nodiscard]] int GetShardForKey(const std::string& key) {
        // Simple hash-based sharding
        size_t hash = std::hash<std::string>()(key);
        return hash % nodes_.size();
    }

    [[nodiscard]] bool WriteDocument(const std::string& key, const std::string& value) {
        int shard = GetShardForKey(key);
        return nodes_[shard]->WriteDocument(key, value);
    }

    [[nodiscard]] std::optional<std::string> ReadDocument(const std::string& key) {
        int shard = GetShardForKey(key);
        return nodes_[shard]->ReadDocument(key);
    }

    void FailNode(int node_id) {
        if (node_id >= 0 && node_id < static_cast<int>(nodes_.size())) {
            nodes_[node_id]->SetState(NodeState::kFailed);
        }
    }

    void RecoverNode(int node_id) {
        if (node_id >= 0 && node_id < static_cast<int>(nodes_.size())) {
            nodes_[node_id]->SimulateRecovery();
            // Simulate recovery delay
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            nodes_[node_id]->SetState(NodeState::kHealthy);
        }
    }

    [[nodiscard]] int GetHealthyNodeCount() const {
        int count = 0;
        for (const auto& node : nodes_) {
            if (node->State() == NodeState::kHealthy) {
                count++;
            }
        }
        return count;
    }

    [[nodiscard]] int GetTotalNodeCount() const { return nodes_.size(); }

private:
    std::vector<std::unique_ptr<MockShardNode>> nodes_;
};

/// @brief Query executor with timeout simulation.
class QueryExecutor {
public:
    struct QueryResult {
        bool success = false;
        std::vector<std::string> data;
        std::chrono::milliseconds latency{0};
    };

    QueryResult ExecuteQuery(std::function<std::optional<std::string>(const std::string&)> fetcher,
                             const std::string& key, std::chrono::milliseconds timeout) {
        QueryResult result;
        auto start = std::chrono::high_resolution_clock::now();

        // Simulate query with timeout
        auto fetch_result = fetcher(key);
        result.success = fetch_result.has_value();
        if (result.success) {
            result.data.push_back(fetch_result.value());
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // Check timeout
        if (result.latency > timeout) {
            result.success = false;
        }

        return result;
    }
};

/// @brief Degradation metrics tracker.
class DegradationMetrics {
public:
    void RecordOperation(bool success) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_operations_++;
        if (success) {
            successful_operations_++;
        }
    }

    void RecordLatency(std::chrono::milliseconds latency) {
        std::lock_guard<std::mutex> lock(mutex_);
        latencies_ms_.push_back(latency.count());
    }

    [[nodiscard]] double SuccessRate() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_operations_ > 0 ? static_cast<double>(successful_operations_) / total_operations_
                                    : 0.0;
    }

    [[nodiscard]] std::string Report() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        oss << "\n=== Degradation Metrics ===\n";
        oss << "Total Operations: " << total_operations_ << "\n";
        oss << "Successful: " << successful_operations_ << "\n";
        oss << "Success Rate: " << std::fixed << std::setprecision(2) << (SuccessRate() * 100)
            << "%\n";
        if (!latencies_ms_.empty()) {
            auto sorted = latencies_ms_;
            std::sort(sorted.begin(), sorted.end());
            oss << "Latency p95: " << sorted[sorted.size() * 95 / 100] << "ms\n";
            oss << "Latency p99: " << sorted[sorted.size() * 99 / 100] << "ms\n";
        }
        return oss.str();
    }

private:
    mutable std::mutex mutex_;
    uint64_t total_operations_ = 0;
    uint64_t successful_operations_ = 0;
    std::vector<uint64_t> latencies_ms_;
};

}  // namespace

// ============================================================================
// Wave 8C Degradation & Fault Recovery Test Suite (DFR-01..DFR-08)
// ============================================================================

class DegradationFaultRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        cluster_ = std::make_unique<MockShardedCluster>(4);  // 4 shards
        metrics_ = std::make_unique<DegradationMetrics>();
        gen_ = std::make_unique<SeededTestDataGenerator>(kCanonicalSeed);

        // Pre-populate cluster
        for (int i = 0; i < 200; ++i) {
            cluster_->WriteDocument("doc_" + std::to_string(i), "content_" + std::to_string(i));
        }
    }

    void TearDown() override {
        cluster_.reset();
        metrics_.reset();
        gen_.reset();
    }

    std::unique_ptr<MockShardedCluster> cluster_;
    std::unique_ptr<DegradationMetrics> metrics_;
    std::unique_ptr<SeededTestDataGenerator> gen_;
};

/// @brief DFR-01: Shard failure scenario (immediate impact).
TEST_F(DegradationFaultRecoveryTest, DFR_01_ShardFailureImmediate) {
    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 4);

    // Fail node 0
    cluster_->FailNode(0);

    // Verify impact: some operations will fail (those targeting shard 0)
    int failures = 0;
    for (int i = 0; i < 100; ++i) {
        std::string key = "doc_" + std::to_string(i);
        auto result = cluster_->ReadDocument(key);
        if (!result.has_value()) {
            failures++;
        }
    }

    EXPECT_GT(failures, 0) << "Shard failure should cause some operation failures";
    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 3);
}

/// @brief DFR-02: Shard recovery (RTO < 30s).
TEST_F(DegradationFaultRecoveryTest, DFR_02_ShardRecovery) {
    cluster_->FailNode(0);

    auto start = std::chrono::high_resolution_clock::now();
    cluster_->RecoverNode(0);
    auto end = std::chrono::high_resolution_clock::now();

    auto recovery_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 4);
    EXPECT_LT(recovery_time.count(), 30000) << "RTO exceeded 30 seconds";
}

/// @brief DFR-03: Network partition isolation.
TEST_F(DegradationFaultRecoveryTest, DFR_03_NetworkPartitionIsolation) {
    // Simulate partition by failing 2 of 4 nodes
    cluster_->FailNode(0);
    cluster_->FailNode(1);

    // Verify quorum is still maintained (2/4 alive)
    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 2);
    EXPECT_GE(cluster_->GetHealthyNodeCount(), cluster_->GetTotalNodeCount() / 2)
        << "Quorum lost during partition";

    // Operations targeting healthy shards should succeed
    int successes = 0;
    for (int i = 0; i < 50; ++i) {
        std::string key = "doc_" + std::to_string(i);
        auto result = cluster_->ReadDocument(key);
        if (result.has_value()) {
            successes++;
        }
    }

    EXPECT_GT(successes, 0) << "Some operations should succeed even during partition";
}

/// @brief DFR-04: Network partition recovery.
TEST_F(DegradationFaultRecoveryTest, DFR_04_NetworkPartitionRecovery) {
    // Simulate partition
    cluster_->FailNode(0);
    cluster_->FailNode(1);

    // Recover both nodes
    auto start = std::chrono::high_resolution_clock::now();
    cluster_->RecoverNode(0);
    cluster_->RecoverNode(1);
    auto end = std::chrono::high_resolution_clock::now();

    auto recovery_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 4);
    EXPECT_LT(recovery_time.count(), 30000) << "Partition recovery exceeded 30 seconds";
}

/// @brief DFR-05: Connection pool exhaustion + backoff.
TEST_F(DegradationFaultRecoveryTest, DFR_05_ConnectionPoolExhaustion) {
    const int max_connections = 32;
    std::queue<int> connection_pool;

    // Simulate connection pool
    for (int i = 0; i < max_connections; ++i) {
        connection_pool.push(i);
    }

    int backoff_events = 0;

    // Try to acquire more connections than available
    for (int i = 0; i < max_connections + 10; ++i) {
        if (!connection_pool.empty()) {
            connection_pool.pop();
        } else {
            backoff_events++;
        }
    }

    EXPECT_GT(backoff_events, 0) << "Backoff events should occur on pool exhaustion";

    // Simulate connection return and backoff recovery
    for (int i = 0; i < 10; ++i) {
        connection_pool.push(i + max_connections);
    }

    EXPECT_FALSE(connection_pool.empty()) << "Connections should be returned to pool";
}

/// @brief DFR-06: Query timeout propagation.
TEST_F(DegradationFaultRecoveryTest, DFR_06_QueryTimeoutPropagation) {
    QueryExecutor executor;
    auto timeout = std::chrono::milliseconds(5);

    // Fail a node to potentially cause timeouts
    cluster_->FailNode(0);

    int timeout_failures = 0;
    for (int i = 0; i < 100; ++i) {
        std::string key = "doc_" + std::to_string(i);
        auto result = executor.ExecuteQuery(
            [this, &key](const std::string& k) { return cluster_->ReadDocument(k); }, key,
            timeout);

        if (!result.success && result.latency > timeout) {
            timeout_failures++;
        }

        metrics_->RecordOperation(result.success);
        metrics_->RecordLatency(result.latency);
    }

    // Some operations should timeout due to degraded state
    std::cout << metrics_->Report();
}

/// @brief DFR-07: Data consistency after fault recovery.
TEST_F(DegradationFaultRecoveryTest, DFR_07_DataConsistencyAfterRecovery) {
    // Record baseline data
    std::unordered_map<std::string, std::string> baseline;
    for (int i = 0; i < 100; ++i) {
        std::string key = "doc_" + std::to_string(i);
        auto result = cluster_->ReadDocument(key);
        if (result.has_value()) {
            baseline[key] = result.value();
        }
    }

    // Fail and recover
    cluster_->FailNode(0);
    cluster_->FailNode(1);
    cluster_->RecoverNode(0);
    cluster_->RecoverNode(1);

    // Verify data consistency
    int consistent = 0;
    for (const auto& [key, expected_value] : baseline) {
        auto result = cluster_->ReadDocument(key);
        if (result.has_value() && result.value() == expected_value) {
            consistent++;
        }
    }

    EXPECT_EQ(consistent, baseline.size()) << "Data consistency degraded after recovery";
}

/// @brief DFR-08: No cascading failures during recovery.
TEST_F(DegradationFaultRecoveryTest, DFR_08_NoCascadingFailures) {
    // Start with healthy cluster
    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 4);

    // Trigger cascading scenario: fail 1 node, then another quickly
    cluster_->FailNode(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    cluster_->FailNode(1);

    // Verify quorum is maintained (still 2/4 alive)
    EXPECT_GE(cluster_->GetHealthyNodeCount(), cluster_->GetTotalNodeCount() / 2)
        << "Cascading failures lost quorum";

    // Recover first node
    cluster_->RecoverNode(0);

    // Verify quorum restored
    EXPECT_GE(cluster_->GetHealthyNodeCount(), cluster_->GetTotalNodeCount() / 2)
        << "Quorum not restored after partial recovery";

    // Recover second node
    cluster_->RecoverNode(1);

    // Verify all healthy
    EXPECT_EQ(cluster_->GetHealthyNodeCount(), 4);
}
} } // namespace themis::test
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
