/**
 * @file test_tracing_integration_sharding.cpp
 * @brief Integration tests for distributed tracing in Sharding module.
 * @version 2.4.0
 * @date 2026-09-02
 *
 * Wave D Phase 2A: Distributed Tracing SDK - Sharding Integration
 *
 * Tests:
 * - SHARD_TRACE_001: routeWrite span creation with shard context baggage
 * - SHARD_TRACE_002: Exact-path gate transition tracing
 * - SHARD_TRACE_003: Topology change event recording
 * - SHARD_TRACE_004: Rebalance operation span hierarchy
 * - SHARD_TRACE_005: Multi-shard write distribution tracing
 * - SHARD_TRACE_006: Shard discovery latency in spans
 * - SHARD_TRACE_007: Stall detection event pattern
 * - SHARD_TRACE_008: Topology refresh trigger tracing
 * - SHARD_TRACE_009: Parent span reference from coordinator
 * - SHARD_TRACE_010: Baggage propagation across shards
 *
 * Gate: W4A-TRACE-01 (overhead ≤ 2% vs Wave 7 baseline)
 */

#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <vector>

#include "observability/distributed_trace_span.h"
#include "observability/distributed_tracing_sdk.h"

namespace themis {
namespace observability {
namespace test {

class ShardingTracingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        DistributedTracingConfig config;
        config.service_name = "shard-router";
        config.enable_trace_correlation_logging = false;
        sdk_ = std::make_unique<DistributedTracingSDK>(config);
    }

    std::unique_ptr<DistributedTracingSDK> sdk_;
};

/**
 * SHARD_TRACE_001: routeWrite span creation with shard context baggage
 */
TEST_F(ShardingTracingIntegrationTest, RouteWriteSpanCreation) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto route_span = std::make_shared<DistributedTraceSpan>("ShardRouter::routeWrite", root_ctx);
    
    // Add routing context as baggage
    route_span->addBaggage("transaction_id", "txn_route_001");
    route_span->addBaggage("write_key", "user:123:profile");
    route_span->addBaggage("shard_count", "8");
    route_span->addBaggage("consistency_level", "strong");
    
    route_span->addEvent("routing_started");
    
    EXPECT_FALSE(route_span->spanId().empty());
    EXPECT_FALSE(route_span->traceId().empty());
    EXPECT_EQ(route_span->operationName(), "ShardRouter::routeWrite");
    
    auto baggage = route_span->baggage();
    EXPECT_GE(baggage.size(), 4);
}

/**
 * SHARD_TRACE_002: Exact-path gate transition tracing
 */
TEST_F(ShardingTracingIntegrationTest, ExactPathGateTransition) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto gate_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::validateExactPath", root_ctx);
    gate_span->addBaggage("transaction_id", "txn_exact_001");
    gate_span->addBaggage("shard_set", "2,4,6");
    
    // Trace state transitions through exact-path gate
    gate_span->addEvent("gate_validation_start", {
        {"current_topology", "epoch_42"},
        {"shard_count", "8"}
    });
    
    gate_span->addEvent("gate_validation_passed", {
        {"latency_us", "245"},
        {"exactness_verified", "true"}
    });
    
    gate_span->setStatus(SpanStatus::Ok);
    
    auto events = gate_span->events();
    EXPECT_GE(events.size(), 2);
}

/**
 * SHARD_TRACE_003: Topology change event recording
 */
TEST_F(ShardingTracingIntegrationTest, TopologyChangeEventRecording) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto topo_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::onTopologyChange", root_ctx);
    topo_span->addBaggage("old_epoch", "41");
    topo_span->addBaggage("new_epoch", "42");
    
    topo_span->addEvent("topology_change_detected", {
        {"old_shard_count", "8"},
        {"new_shard_count", "10"}
    });
    
    topo_span->addEvent("topology_update_in_progress", {
        {"shards_being_added", "2"},
        {"shards_being_removed", "0"}
    });
    
    topo_span->addEvent("topology_update_complete", {
        {"update_latency_ms", "156"}
    });
    
    topo_span->setStatus(SpanStatus::Ok);
    
    auto events = topo_span->events();
    EXPECT_EQ(events.size(), 3);
}

/**
 * SHARD_TRACE_004: Rebalance operation span hierarchy
 */
TEST_F(ShardingTracingIntegrationTest, RebalanceSpanHierarchy) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto rebalance_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::rebalanceTopology", root_ctx);
    rebalance_span->addBaggage("transaction_id", "txn_rebalance_001");
    
    rebalance_span->addEvent("rebalance_started", {
        {"shard_count", "8"},
        {"target_distribution", "even"}
    });
    
    // Create child spans for each shard movement
    for (int shard_id = 0; shard_id < 3; ++shard_id) {
        auto shard_ctx = rebalance_span->childContext("MoveShard_" + std::to_string(shard_id));
        auto shard_span = std::make_shared<DistributedTraceSpan>(
            "MoveShard_" + std::to_string(shard_id), shard_ctx);
        
        shard_span->addBaggage("shard_id", std::to_string(shard_id));
        shard_span->addEvent("shard_move_start");
        shard_span->addEvent("shard_move_complete", {{"duration_ms", "234"}});
        shard_span->setStatus(SpanStatus::Ok);
    }
    
    rebalance_span->addEvent("rebalance_complete", {{"total_duration_ms", "1050"}});
    rebalance_span->setStatus(SpanStatus::Ok);
}

/**
 * SHARD_TRACE_005: Multi-shard write distribution tracing
 */
TEST_F(ShardingTracingIntegrationTest, MultiShardDistribution) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto dist_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::distributeWrite", root_ctx);
    dist_span->addBaggage("transaction_id", "txn_distribute_001");
    dist_span->addBaggage("target_shards", "2,4,6");
    
    dist_span->addEvent("distribution_started", {{"shard_count", "3"}});
    
    // Trace writes to each shard
    int shard_ids[] = {2, 4, 6};
    for (int shard_id : shard_ids) {
        auto shard_ctx = dist_span->childContext("WriteShard_" + std::to_string(shard_id));
        auto shard_span = std::make_shared<DistributedTraceSpan>(
            "WriteShard_" + std::to_string(shard_id), shard_ctx);
        
        shard_span->addBaggage("shard_id", std::to_string(shard_id));
        shard_span->addEvent("write_queued");
        shard_span->addEvent("write_acknowledged", {{"latency_us", "342"}});
        shard_span->setStatus(SpanStatus::Ok);
    }
    
    dist_span->addEvent("distribution_complete");
    dist_span->setStatus(SpanStatus::Ok);
}

/**
 * SHARD_TRACE_006: Shard discovery latency in spans
 */
TEST_F(ShardingTracingIntegrationTest, ShardDiscoveryLatency) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto discovery_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::discoverShards", root_ctx);
    
    auto start = std::chrono::high_resolution_clock::now();
    discovery_span->addEvent("discovery_start");
    
    // Simulate discovery delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    discovery_span->addEvent("discovery_complete", {
        {"discovered_shards", "8"},
        {"latency_us", std::to_string(latency_us)}
    });
    
    discovery_span->setStatus(SpanStatus::Ok);
    
    // Verify duration calculation
    auto duration = discovery_span->durationMicros();
    EXPECT_GT(duration, 9000);  // Should be at least 9ms
}

/**
 * SHARD_TRACE_007: Stall detection event pattern
 */
TEST_F(ShardingTracingIntegrationTest, StallDetectionPattern) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto stall_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::detectStall", root_ctx);
    stall_span->addBaggage("transaction_id", "txn_stall_001");
    
    stall_span->addEvent("stall_monitoring_started");
    
    // Simulate stall detection
    stall_span->addEvent("stall_detected", {
        {"shard_id", "4"},
        {"pending_operations", "125"},
        {"time_since_last_progress_ms", "5034"}
    });
    
    stall_span->addEvent("stall_recovery_attempted", {
        {"recovery_strategy", "FORCE_COMPLETE"}
    });
    
    stall_span->addEvent("stall_recovered", {
        {"recovery_latency_ms", "245"}
    });
    
    stall_span->setStatus(SpanStatus::Ok);
}

/**
 * SHARD_TRACE_008: Topology refresh trigger tracing
 */
TEST_F(ShardingTracingIntegrationTest, TopologyRefreshTrigger) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto refresh_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::refreshTopology", root_ctx);
    refresh_span->addBaggage("trigger_reason", "STALE_EPOCH");
    
    refresh_span->addEvent("topology_refresh_started", {
        {"current_epoch", "41"},
        {"latest_epoch", "42"}
    });
    
    refresh_span->addEvent("topology_fetched", {
        {"shard_count", "8"},
        {"fetch_latency_ms", "34"}
    });
    
    refresh_span->addEvent("topology_refresh_complete", {
        {"new_epoch", "42"}
    });
    
    refresh_span->setStatus(SpanStatus::Ok);
}

/**
 * SHARD_TRACE_009: Parent span reference from coordinator
 */
TEST_F(ShardingTracingIntegrationTest, ParentCoordinatorReference) {
    // Simulate coordinator creating parent span
    auto coordinator_ctx = DistributedTraceContext::createRoot();
    auto coordinator_span = std::make_shared<DistributedTraceSpan>(
        "Coordinator::execute", coordinator_ctx);
    coordinator_span->addBaggage("transaction_id", "txn_parent_001");
    
    // Router receives child context
    auto router_ctx = coordinator_span->childContext("ShardRouter::route");
    auto router_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::route", router_ctx);
    
    // Verify parent-child linkage
    EXPECT_EQ(router_span->traceId(), coordinator_span->traceId());
    EXPECT_EQ(router_ctx->parentSpanId(), coordinator_span->spanId());
    
    // Verify baggage inheritance
    auto router_baggage = router_span->baggage();
    auto has_txn_id = std::find_if(router_baggage.begin(), router_baggage.end(),
        [](const auto& pair) { return pair.first == "transaction_id"; });
    EXPECT_NE(has_txn_id, router_baggage.end());
}

/**
 * SHARD_TRACE_010: Baggage propagation across shards
 */
TEST_F(ShardingTracingIntegrationTest, BaggagePropagationAcrossShards) {
    auto root_ctx = DistributedTraceContext::createRoot();
    auto route_span = std::make_shared<DistributedTraceSpan>(
        "ShardRouter::route", root_ctx);
    
    // Add routing baggage
    route_span->addBaggage("transaction_id", "txn_prop_001");
    route_span->addBaggage("client_id", "client_abc");
    route_span->addBaggage("consistency_level", "strong");
    
    // Verify propagation to shards
    for (int shard_id = 0; shard_id < 4; ++shard_id) {
        auto shard_ctx = route_span->childContext("Shard_" + std::to_string(shard_id));
        auto shard_span = std::make_shared<DistributedTraceSpan>(
            "Shard_" + std::to_string(shard_id), shard_ctx);
        
        // Verify all baggage items propagated
        auto shard_baggage = shard_span->baggage();
        EXPECT_GE(shard_baggage.size(), 3);
        
        auto has_txn = std::find_if(shard_baggage.begin(), shard_baggage.end(),
            [](const auto& pair) { return pair.first == "transaction_id"; });
        EXPECT_NE(has_txn, shard_baggage.end());
        EXPECT_EQ(has_txn->second, "txn_prop_001");
    }
}

}  // namespace test
}  // namespace observability
}  // namespace themis
