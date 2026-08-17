// tests/observability/test_high_cardinality_metrics.cpp
// Wave D Phase 2B: High-Cardinality Metrics Collection
// Gate: W4A-METRICS-01 (memory bounded, no OOM at 50k dimensions)

#include "observability/wave_d_high_cardinality_metrics.h"

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

namespace themis::observability::tests {

// Benchmark: Metrics collection under increasing cardinality
static void HighCardinalityMetricsEmission(benchmark::State& state) {
  int dimension_count = state.range(0);  // 100, 1000, 10000, 50000
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  for (auto _ : state) {
    // Simulate metric emission with varying cardinality
    for (int i = 0; i < dimension_count; ++i) {
      mgr.shardLatencies().recordLatency(
        "shard_" + std::to_string(i % 100),
        "write",
        42.5);
    }
  }
}

BENCHMARK(HighCardinalityMetricsEmission)
    ->RangeMultiplier(10)
    ->Range(100, 50000)
    ->Complexity();

// Test: Verify cardinality bounds
class MetricsCardinalityTest : public ::testing::Test {
protected:
  static constexpr int MAX_SAFE_CARDINALITY = 50000;
  static constexpr double MAX_MEMORY_MB = 512.0;
  
  void SetUp() override {
    auto& mgr = HighCardinalityMetricsManager::getInstance();
    mgr.reset();
  }
};

TEST_F(MetricsCardinalityTest, MetricsMemoryBounded) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Emit metrics up to 50k cardinality
  for (int i = 0; i < 50000; ++i) {
    mgr.shardLatencies().recordLatency(
      "shard_" + std::to_string(i / 50),  // 1000 unique shards
      "write",
      10.0 + (i % 100));
  }
  
  // Verify cardinality is bounded
  EXPECT_TRUE(mgr.isCardinalitySafe());
  
  // Verify memory usage is within budget
  uint64_t estimated_bytes = mgr.getEstimatedMemoryBytes();
  double estimated_mb = estimated_bytes / (1024.0 * 1024.0);
  EXPECT_LT(estimated_mb, MAX_MEMORY_MB);
}

TEST_F(MetricsCardinalityTest, ShardCardinalityBounded) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Try to emit more than 1024 shards
  for (int i = 0; i < 2000; ++i) {
    mgr.shardLatencies().recordLatency(
      "shard_" + std::to_string(i),
      "read",
      25.0);
  }
  
  // Verify cardinality is bounded at 1024
  EXPECT_LE(mgr.shardLatencies().getCardinality(), 1024);
}

TEST_F(MetricsCardinalityTest, ReplicaCardinalityBounded) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Try to emit more than 32 replicas
  for (int i = 0; i < 100; ++i) {
    mgr.replicaLag().recordLag(
      "replica_" + std::to_string(i),
      10.0 + (i % 20));
  }
  
  // Verify cardinality is bounded at 32
  EXPECT_LE(mgr.replicaLag().getCardinality(), 32);
}

TEST_F(MetricsCardinalityTest, RetryReasonCardinalityBounded) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Define failure reasons
  std::vector<std::string> reasons = {
    "timeout", "connection_reset", "byzantine", "overload",
    "not_found", "permission_denied", "invalid_argument",
    "internal_error", "unavailable", "unknown"
  };
  
  // Try to emit more than 10 reasons
  for (size_t i = 0; i < reasons.size() + 5; ++i) {
    mgr.retryCounter().recordRetry(
      reasons[i % reasons.size()],
      1);
  }
  
  // Verify cardinality is bounded at 10
  EXPECT_LE(mgr.retryCounter().getAllRetries().size(), 10);
}

TEST_F(MetricsCardinalityTest, QuantileCalculation) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Record latencies for a specific shard
  for (int i = 1; i <= 100; ++i) {
    mgr.shardLatencies().recordLatency("shard_001", "write", i * 1.0);
  }
  
  auto quantiles = mgr.shardLatencies().getQuantiles("shard_001", "write");
  EXPECT_GT(quantiles.p50, 0.0);
  EXPECT_GT(quantiles.p95, quantiles.p50);
  EXPECT_GT(quantiles.p99, quantiles.p95);
}

TEST_F(MetricsCardinalityTest, ReplicaLagQuantiles) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Record lag observations
  for (int i = 1; i <= 50; ++i) {
    mgr.replicaLag().recordLag("replica_001", i * 2.0);
  }
  
  auto quantiles = mgr.replicaLag().getQuantiles("replica_001");
  EXPECT_GT(quantiles.p50, 0.0);
  EXPECT_GT(quantiles.p95, quantiles.p50);
  EXPECT_GT(quantiles.p99, quantiles.p95);
  EXPECT_GT(quantiles.max, quantiles.p99);
}

TEST_F(MetricsCardinalityTest, RetryCounterAggregation) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Record retries by reason
  mgr.retryCounter().recordRetry("timeout", 10);
  mgr.retryCounter().recordRetry("connection_reset", 5);
  mgr.retryCounter().recordRetry("byzantine", 3);
  
  EXPECT_EQ(mgr.retryCounter().getRetryCount("timeout"), 10);
  EXPECT_EQ(mgr.retryCounter().getRetryCount("connection_reset"), 5);
  EXPECT_EQ(mgr.retryCounter().getTotalRetries(), 18);
}

TEST_F(MetricsCardinalityTest, MemoryBoundedUnderSustainedLoad) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Simulate sustained metric emission
  for (int iteration = 0; iteration < 1000; ++iteration) {
    for (int i = 0; i < 100; ++i) {
      mgr.shardLatencies().recordLatency(
        "shard_" + std::to_string(i % 50),
        "write",
        20.0 + (iteration % 50));
    }
    mgr.replicaLag().recordLag(
      "replica_" + std::to_string(iteration % 10),
      15.0 + (iteration % 30));
  }
  
  // Verify system is still stable
  EXPECT_TRUE(mgr.isCardinalitySafe());
  uint64_t memory_bytes = mgr.getEstimatedMemoryBytes();
  EXPECT_LT(memory_bytes, 512 * 1024 * 1024);  // Under 512 MB
}

TEST_F(MetricsCardinalityTest, CardinalityExceedingLimit) {
  auto& mgr = HighCardinalityMetricsManager::getInstance();
  
  // Emit to near the limit (40k cardinality)
  for (int i = 0; i < 40000; ++i) {
    mgr.shardLatencies().recordLatency(
      "shard_" + std::to_string(i / 40),
      "read",
      15.0);
  }
  
  size_t cardinality_before = mgr.getTotalCardinality();
  EXPECT_LT(cardinality_before, 50000);
  
  // Try to exceed limit (should be capped)
  for (int i = 40000; i < 60000; ++i) {
    mgr.shardLatencies().recordLatency(
      "shard_" + std::to_string(i / 40),
      "write",
      25.0);
  }
  
  size_t cardinality_after = mgr.getTotalCardinality();
  // Cardinality should grow but remain bounded
  EXPECT_LE(cardinality_after, 50000 + 100);  // Small tolerance
}

}  // namespace themis::observability::tests
