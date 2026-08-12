/**
 * Integration test for Prometheus metrics in sharding components
 * 
 * This test validates that metrics are properly recorded and exported
 * in Prometheus format for the sharding subsystem.
 */

#include <gtest/gtest.h>
#include "sharding/prometheus_metrics.h"
#include "sharding/shard_router.h"
#include "sharding/data_migrator.h"
#include "sharding/metrics_registry.h"
#include <memory>
#include <string>
#include <regex>

using namespace themis::sharding;

class PrometheusMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create metrics instance
        PrometheusMetrics::Config config;
        config.enable_histograms = true;
        metrics_ = std::make_shared<PrometheusMetrics>(config);
    }

    void TearDown() override {
        metrics_.reset();
    }

    std::shared_ptr<PrometheusMetrics> metrics_;
};

// Test basic metric recording
TEST_F(PrometheusMetricsTest, RecordBasicMetrics) {
    // Record some metrics
    metrics_->recordRoutingRequest("local");
    metrics_->recordRoutingRequest("remote");
    metrics_->recordRoutingRequest("scatter_gather");
    
    // Get metrics output
    std::string output = metrics_->getMetrics();
    
    // Verify metrics are present
    EXPECT_TRUE(output.find("themis_routing_requests_total") != std::string::npos);
}

// Test metrics with annotations
TEST_F(PrometheusMetricsTest, MetricsWithAnnotations) {
    // Record metrics
    metrics_->recordShardHealth("shard-1", "healthy");
    metrics_->recordClusterSize(5);
    
    // Get annotated metrics
    std::string output = metrics_->getMetricsWithAnnotations();
    
    // Verify HELP and TYPE annotations
    EXPECT_TRUE(output.find("# HELP themis") != std::string::npos);
    EXPECT_TRUE(output.find("# TYPE themis") != std::string::npos);
    
    // Verify metric values
    EXPECT_TRUE(output.find("themis_cluster_size 5") != std::string::npos);
}

// Test cross-shard join metrics
TEST_F(PrometheusMetricsTest, CrossShardJoinMetrics) {
    // Record join operation
    metrics_->recordCrossShardJoin("broadcast_hash");
    metrics_->recordCrossShardJoinDuration("broadcast_hash", 543.2);
    metrics_->recordCrossShardJoinRows("broadcast_hash", 1000, 2000, 1500);
    metrics_->recordHashTableBuildTime(123.4);
    
    std::string output = metrics_->getMetrics();
    
    // Verify metrics
    EXPECT_TRUE(output.find("themis_cross_shard_joins_total") != std::string::npos);
    EXPECT_TRUE(output.find("broadcast_hash") != std::string::npos);
}

// Test migration metrics
TEST_F(PrometheusMetricsTest, MigrationMetrics) {
    std::string operation_id = "shard1_to_shard2";
    
    // Record migration progress
    metrics_->recordMigrationProgress(operation_id, 5000, 1048576, 50.0);
    metrics_->recordMigrationDuration(operation_id, 123.45);
    
    std::string output = metrics_->getMetrics();
    
    // Verify metrics
    EXPECT_TRUE(output.find("themis_migration_records_total") != std::string::npos);
    EXPECT_TRUE(output.find("themis_migration_progress_percent") != std::string::npos);
    EXPECT_TRUE(output.find(operation_id) != std::string::npos);
}

// Test gossip protocol metrics
TEST_F(PrometheusMetricsTest, GossipMetrics) {
    metrics_->recordGossipMessage("heartbeat");
    metrics_->recordGossipMessage("peer_list");
    metrics_->recordGossipPeerCount(10);
    metrics_->recordGossipRoundTrip(15.5);
    
    std::string output = metrics_->getMetrics();
    
    EXPECT_TRUE(output.find("themis_gossip_messages_total") != std::string::npos);
    EXPECT_TRUE(output.find("themis_gossip_peer_count") != std::string::npos);
}

// Test metrics registry
TEST_F(PrometheusMetricsTest, MetricsRegistry) {
    // Register metrics
    ShardingMetricsRegistry::instance().registerMetrics(metrics_);
    
    // Retrieve from registry
    auto retrieved = ShardingMetricsRegistry::instance().getMetrics();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved.get(), metrics_.get());
    
    // Get metrics string from registry
    metrics_->recordClusterSize(3);
    std::string output = ShardingMetricsRegistry::instance().getMetricsString();
    EXPECT_TRUE(output.find("themis_cluster_size") != std::string::npos);
}

// Test histogram quantiles
TEST_F(PrometheusMetricsTest, HistogramQuantiles) {
    // Record multiple latency samples
    for (int i = 0; i < 100; i++) {
        metrics_->recordRoutingLatency("get", 10.0 + i * 0.5);
    }
    
    std::string output = metrics_->getMetrics();
    
    // Verify quantiles are present
    EXPECT_TRUE(output.find("quantile=\"0.5\"") != std::string::npos);
    EXPECT_TRUE(output.find("quantile=\"0.95\"") != std::string::npos);
    EXPECT_TRUE(output.find("quantile=\"0.99\"") != std::string::npos);
}

// Test Prometheus format compliance
TEST_F(PrometheusMetricsTest, PrometheusFormatCompliance) {
    metrics_->recordRoutingRequest("local");
    metrics_->recordShardHealth("shard-1", "healthy");
    
    std::string output = metrics_->getMetricsWithAnnotations();
    
    // Check format compliance
    // Each metric should have HELP and TYPE
    std::regex help_regex(R"(# HELP themis_\w+ .+)");
    std::regex type_regex(R"(# TYPE themis_\w+ (counter|gauge|histogram))");
    std::regex metric_regex(R"(themis_\w+(\{[^}]+\})? \d+(\.\d+)?)");
    
    EXPECT_TRUE(std::regex_search(output, help_regex));
    EXPECT_TRUE(std::regex_search(output, type_regex));
    EXPECT_TRUE(std::regex_search(output, metric_regex));
}


