/**
 * @file test_shard_rpc_integration.cpp
 * @brief Tests for the Shard RPC Integration acceptance criteria (Issue #202):
 *
 *   AC3: ShardRpcRetryPolicy struct in shard_rpc_client.h.
 *   AC4: MTLSConnectionPoolManager wired into ShardRPCClient;
 *        max_pool_connections configurable via GossipConfigManagerConfig.
 *   AC5: OperationalMetrics::recordRpcCall + PrometheusMetrics::recordRpcCall
 *        called per RPC attempt with shard_id / method / outcome labels.
 *   AC6: MTLSConnectionPoolManager::onCertificateRotated() gracefully drains
 *        idle connections without dropping in-flight requests.
 */

#include <gtest/gtest.h>
#include "sharding/shard_rpc_client.h"
#include "sharding/mtls_connection_pool.h"
#include "sharding/operational_metrics.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/gossip_config_manager.h"
#include <string>
#include <memory>
#include <atomic>

using namespace themis::sharding;
using namespace themisdb::sharding;

// ============================================================================
// AC3 – ShardRpcRetryPolicy struct
// ============================================================================

TEST(ShardRpcRetryPolicyTest, DefaultValues) {
    ShardRpcRetryPolicy policy;
    EXPECT_EQ(policy.max_attempts, 3);
    EXPECT_EQ(policy.initial_backoff_ms, 100);
    EXPECT_EQ(policy.max_backoff_ms, 5000);
    EXPECT_TRUE(policy.use_circuit_breaker);
}

TEST(ShardRpcRetryPolicyTest, CustomValues) {
    ShardRpcRetryPolicy policy;
    policy.max_attempts        = 5;
    policy.initial_backoff_ms  = 50;
    policy.max_backoff_ms      = 2000;
    policy.use_circuit_breaker = false;

    EXPECT_EQ(policy.max_attempts, 5);
    EXPECT_EQ(policy.initial_backoff_ms, 50);
    EXPECT_EQ(policy.max_backoff_ms, 2000);
    EXPECT_FALSE(policy.use_circuit_breaker);
}

// ============================================================================
// AC4 – Connection pool config fields
// ============================================================================

TEST(ShardRpcClientConfigPoolTest, DefaultConnectionPoolConfig) {
    ShardRPCClient::Config cfg;
    EXPECT_EQ(cfg.connection_pool, nullptr);
    EXPECT_EQ(cfg.max_pool_connections, 50);
}

TEST(ShardRpcClientConfigPoolTest, PoolPointerCanBeSet) {
    MTLSConnectionPoolManager pool;

    ShardRPCClient::Config cfg;
    cfg.endpoint        = "localhost:50051";
    cfg.connection_pool = &pool;
    cfg.max_pool_connections = 100;

    EXPECT_EQ(cfg.connection_pool, &pool);
    EXPECT_EQ(cfg.max_pool_connections, 100);
}

TEST(GossipConfigRpcPoolTest, DefaultRpcMaxPoolConnections) {
    GossipConfigManagerConfig gcfg;
    EXPECT_EQ(gcfg.rpc_max_pool_connections, 50u);
}

TEST(GossipConfigRpcPoolTest, CanSetRpcMaxPoolConnections) {
    GossipConfigManagerConfig gcfg;
    gcfg.rpc_max_pool_connections = 200;
    EXPECT_EQ(gcfg.rpc_max_pool_connections, 200u);
}

// Verify that max_pool_connections from gossip config can be propagated
// into ShardRPCClient::Config (integration of AC4 components).
TEST(ShardRpcClientConfigPoolTest, PropagateGossipPoolSizeToClientConfig) {
    GossipConfigManagerConfig gcfg;
    gcfg.rpc_max_pool_connections = 75;

    ShardRPCClient::Config client_cfg;
    client_cfg.endpoint = "localhost:50051";
    client_cfg.max_pool_connections = static_cast<int>(gcfg.rpc_max_pool_connections);

    EXPECT_EQ(client_cfg.max_pool_connections, 75);
}

// ============================================================================
// AC5 – Metrics config fields and recording
// ============================================================================

TEST(ShardRpcClientConfigMetricsTest, DefaultMetricsPointersAreNull) {
    ShardRPCClient::Config cfg;
    EXPECT_EQ(cfg.operational_metrics, nullptr);
    EXPECT_EQ(cfg.prometheus_metrics,  nullptr);
}

TEST(ShardRpcClientConfigMetricsTest, CanSetMetricsPointers) {
    OperationalMetrics ops;
    PrometheusMetrics::Config pcfg;
    PrometheusMetrics prom(pcfg);

    ShardRPCClient::Config cfg;
    cfg.endpoint            = "localhost:50051";
    cfg.operational_metrics = &ops;
    cfg.prometheus_metrics  = &prom;

    EXPECT_EQ(cfg.operational_metrics, &ops);
    EXPECT_EQ(cfg.prometheus_metrics,  &prom);
}

// OperationalMetrics::recordRpcCall must not throw and must register
// as a request for the given shard.
TEST(OperationalMetricsRpcCallTest, RecordRpcCallRegistersRequest) {
    OperationalMetrics metrics;
    metrics.registerShard("shard-1");

    EXPECT_NO_THROW(
        metrics.recordRpcCall("shard-1", "prepare", "success", 1500u));
    EXPECT_NO_THROW(
        metrics.recordRpcCall("shard-1", "commit", "retryable_error", 0u));
    EXPECT_NO_THROW(
        metrics.recordRpcCall("shard-1", "abort", "non_retryable_error", 0u));

    // The ShardMetrics for shard-1 should reflect 3 total requests.
    const auto* m = metrics.getShardMetrics("shard-1");
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->total_requests.load(), 3u);
}

// PrometheusMetrics::recordRpcCall must not throw.
TEST(PrometheusMetricsRpcCallTest, RecordRpcCallNoThrow) {
    PrometheusMetrics::Config cfg;
    PrometheusMetrics prom(cfg);

    EXPECT_NO_THROW(prom.recordRpcCall("shard-A", "prepare",  "success",            1.2));
    EXPECT_NO_THROW(prom.recordRpcCall("shard-A", "commit",   "retryable_error",    0.0));
    EXPECT_NO_THROW(prom.recordRpcCall("shard-A", "abort",    "non_retryable_error",0.0));
}

// ShardRPCClient wired with metrics must record a call for the in-process path.
TEST(ShardRpcClientMetricsIntegrationTest, InProcessPathRecordsMetric) {
    OperationalMetrics ops;
    ops.registerShard("shard-x");

    ShardRPCClient::Config cfg;
    cfg.endpoint            = "localhost:50051"; // loopback → in-process path
    cfg.shard_id            = "shard-x";
    cfg.max_retries         = 1;
    cfg.operational_metrics = &ops;

    ShardRPCClient client(cfg);

    // ping() exercises the in-process sendRequest path.
    EXPECT_NO_THROW(client.ping());

    const auto* m = ops.getShardMetrics("shard-x");
    ASSERT_NE(m, nullptr);
    // At least one request should have been recorded.
    EXPECT_GE(m->total_requests.load(), 1u);
}

// ============================================================================
// AC6 – Certificate rotation drain
// ============================================================================

TEST(MtlsConnectionPoolCertRotationTest, OnCertificateRotatedDoesNotThrow) {
    MTLSConnectionPoolManager pool;
    EXPECT_NO_THROW(pool.onCertificateRotated());
}

TEST(MtlsConnectionPoolCertRotationTest, OnCertificateRotatedWithActivePools) {
    MTLSConnectionPoolManager pool;

    // Pre-warm the pool manager with a pool entry (no real SSL connections).
    auto ep_pool = pool.getPool("localhost:50051");
    ASSERT_NE(ep_pool, nullptr);

    // Rotate: idle connections (none here) are drained; active connections
    // (none here either) are left open.  Must not crash.
    EXPECT_NO_THROW(pool.onCertificateRotated());

    // After rotation, pool is still operational.
    auto ep_pool_after = pool.getPool("localhost:50051");
    EXPECT_NE(ep_pool_after, nullptr);
}

TEST(MtlsConnectionPoolCertRotationTest, StatisticsRemainsConsistentAfterRotation) {
    MTLSConnectionPoolManager pool;
    pool.getPool("shard1:50051");
    pool.getPool("shard2:50051");

    pool.onCertificateRotated();

    // Both endpoint pools should still be tracked.
    const auto stats = pool.getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 2u);
}

// ============================================================================
// Circuit-breaker recovery default (acceptance criterion: 5 s)
// ============================================================================

TEST(ShardRpcClientCbDefaultTest, CircuitBreakerRecoveryDefault5s) {
    ShardRPCClient::Config cfg;
    EXPECT_EQ(cfg.circuit_breaker_recovery_ms, 5000);
}
