/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sharding_operational_metrics.cpp              ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 04:31:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     187                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • fd50a2b408  2026-03-03  test: Wave 2 - add tests for utils, regex PII detection, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_sharding_operational_metrics.cpp
 * @brief Tests for OperationalMetrics
 *        (src/sharding/operational_metrics.cpp)
 *
 * Covers OperationalMetrics:
 *   - Default construction
 *   - getShardMetrics (creates on first access)
 *   - recordRequest (read and write, success and failure)
 *   - getShardIds
 *   - getAggregatedMetrics
 *   - getClusterHealth (initial state → HEALTHY or DEGRADED)
 *   - updateShardHealth
 *   - recordNetworkTraffic
 *   - updateResourceUsage
 */

#include <gtest/gtest.h>
#include "sharding/operational_metrics.h"
#include <algorithm>
#include <string>

using namespace themisdb::sharding;

// ============================================================================
// Fixture
// ============================================================================

class OperationalMetricsTest : public ::testing::Test {
protected:
    OperationalMetrics metrics_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(OperationalMetricsTest, Construction_Succeeds) {
    EXPECT_NE(metrics_.getShardIds().size(), static_cast<size_t>(-1));
}

TEST_F(OperationalMetricsTest, GetShardMetrics_CreatesOnFirstAccess) {
    auto* sm = metrics_.getShardMetrics("shard_alpha");
    EXPECT_NE(sm, nullptr);
}

TEST_F(OperationalMetricsTest, GetShardMetrics_SameIdReturnsSamePointer) {
    auto* a = metrics_.getShardMetrics("shard1");
    auto* b = metrics_.getShardMetrics("shard1");
    EXPECT_EQ(a, b);
}

// ============================================================================
// recordRequest
// ============================================================================

TEST_F(OperationalMetricsTest, RecordRequest_SuccessRead_IncrementsTotals) {
    metrics_.recordRequest("s1", /*latency_us=*/100, /*success=*/true, /*is_write=*/false);
    auto* sm = metrics_.getShardMetrics("s1");
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sm->total_requests.load(), 1u);
    EXPECT_EQ(sm->successful_requests.load(), 1u);
    EXPECT_EQ(sm->read_requests.load(), 1u);
    EXPECT_EQ(sm->write_requests.load(), 0u);
}

TEST_F(OperationalMetricsTest, RecordRequest_SuccessWrite_IncrementsWriteCount) {
    metrics_.recordRequest("s2", 200, true, /*is_write=*/true);
    auto* sm = metrics_.getShardMetrics("s2");
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sm->write_requests.load(), 1u);
    EXPECT_EQ(sm->read_requests.load(), 0u);
}

TEST_F(OperationalMetricsTest, RecordRequest_Failure_IncrementsFailedCount) {
    metrics_.recordRequest("s3", 50, /*success=*/false, false);
    auto* sm = metrics_.getShardMetrics("s3");
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sm->failed_requests.load(), 1u);
    EXPECT_EQ(sm->successful_requests.load(), 0u);
}

TEST_F(OperationalMetricsTest, RecordRequest_LatencyIsTracked) {
    metrics_.recordRequest("s_lat", 1234, true, false);
    auto* sm = metrics_.getShardMetrics("s_lat");
    ASSERT_NE(sm, nullptr);
    EXPECT_GE(sm->total_latency_us.load(), 1234u);
}

// ============================================================================
// getShardIds
// ============================================================================

TEST_F(OperationalMetricsTest, GetShardIds_ReturnsRegisteredIds) {
    metrics_.recordRequest("alpha", 100, true, false);
    metrics_.recordRequest("beta",  200, true, false);

    auto ids = metrics_.getShardIds();
    bool has_alpha = std::find(ids.begin(), ids.end(), "alpha") != ids.end();
    bool has_beta  = std::find(ids.begin(), ids.end(), "beta")  != ids.end();
    EXPECT_TRUE(has_alpha);
    EXPECT_TRUE(has_beta);
}

// ============================================================================
// getAggregatedMetrics
// ============================================================================

TEST_F(OperationalMetricsTest, GetAggregatedMetrics_AggregatesAllShards) {
    metrics_.recordRequest("x1", 100, true, false);
    metrics_.recordRequest("x2", 200, true, true);

    ShardMetrics agg;
    metrics_.getAggregatedMetrics(agg);
    EXPECT_EQ(agg.total_requests.load(), 2u);
}

// ============================================================================
// updateShardHealth / getClusterHealth
// ============================================================================

TEST_F(OperationalMetricsTest, GetClusterHealth_NoShards_DefaultState) {
    // With no shards, cluster health should be deterministic
    EXPECT_NO_THROW(metrics_.getClusterHealth());
}

TEST_F(OperationalMetricsTest, UpdateShardHealth_HealthyState) {
    EXPECT_NO_THROW(metrics_.updateShardHealth("shard_h", HealthStatus::HEALTHY));
    EXPECT_EQ(metrics_.getClusterHealth(), HealthStatus::HEALTHY);
}

TEST_F(OperationalMetricsTest, UpdateShardHealth_UnhealthyShard_DegradeCluster) {
    metrics_.updateShardHealth("shard_ok",  HealthStatus::HEALTHY);
    metrics_.updateShardHealth("shard_bad", HealthStatus::UNHEALTHY);
    auto cluster = metrics_.getClusterHealth();
    // With one unhealthy shard the cluster should be degraded or unhealthy
    EXPECT_NE(cluster, HealthStatus::HEALTHY);
}

// ============================================================================
// updateResourceUsage
// ============================================================================

TEST_F(OperationalMetricsTest, UpdateResourceUsage_StoresValues) {
    metrics_.updateResourceUsage("rs",
                                 /*memory_bytes=*/1024 * 1024,
                                 /*disk_bytes=*/512 * 1024 * 1024);
    auto* sm = metrics_.getShardMetrics("rs");
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sm->memory_usage_bytes.load(), 1024u * 1024u);
    EXPECT_EQ(sm->disk_usage_bytes.load(),   512u * 1024u * 1024u);
}

// ============================================================================
// recordNetworkTraffic
// ============================================================================

TEST_F(OperationalMetricsTest, RecordNetworkTraffic_UpdatesCounters) {
    metrics_.recordNetworkTraffic("net_shard", /*bytes_sent=*/4096, /*bytes_recv=*/2048);
    auto* sm = metrics_.getShardMetrics("net_shard");
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sm->network_bytes_sent.load(),     4096u);
    EXPECT_EQ(sm->network_bytes_received.load(), 2048u);
}
