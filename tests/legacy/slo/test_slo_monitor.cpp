// Copyright 2026 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/slo_monitor.h"
#include <thread>
#include <chrono>
#include <future>

using namespace themis::sharding;

class SLOMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        SLOMonitor::Config config;
        config.window_duration = std::chrono::seconds(60);  // Short window for testing
        config.enable_alerting = true;
        config.alert_threshold = 0.9;
        
        monitor_ = std::make_unique<SLOMonitor>(config);
    }
    
    void TearDown() override {
        monitor_.reset();
    }
    
    std::unique_ptr<SLOMonitor> monitor_;
};

TEST_F(SLOMonitorTest, AvailabilityTracking) {
    const std::string shard_id = "shard-001";
    
    // Record availability
    for (int i = 0; i < 9; ++i) {
        monitor_->recordShardAvailability(shard_id, true);
    }
    monitor_->recordShardAvailability(shard_id, false);
    
    double error_budget = monitor_->getErrorBudget(shard_id);
    EXPECT_GE(error_budget, 0.0);
    EXPECT_LE(error_budget, 1.0);
}

TEST_F(SLOMonitorTest, LatencyTracking) {
    const std::string shard_id = "shard-001";
    const std::string query_type = "single_shard_query";
    
    for (int i = 0; i < 100; ++i) {
        monitor_->recordQueryLatency(shard_id, query_type, 5.0 + (i % 10));
    }
    
    EXPECT_TRUE(monitor_->isLatencySLOMet(query_type));
}

TEST_F(SLOMonitorTest, SLOReportGeneration) {
    const std::string shard_id = "shard-001";
    
    monitor_->recordShardAvailability(shard_id, true);
    monitor_->recordQueryLatency(shard_id, "single_shard_query", 10.0);
    monitor_->recordReplicationLag(shard_id, 50.0);
    
    std::string report = monitor_->generateSLOReport();
    
    EXPECT_NE(report.find(shard_id), std::string::npos);
    EXPECT_NE(report.find("AVAILABILITY SLO"), std::string::npos);
}

TEST_F(SLOMonitorTest, ComplianceSnapshotIncludesErrorBudget) {
    const std::string shard_id = "shard-001";
    monitor_->recordShardAvailability(shard_id, true);

    const auto compliance = monitor_->getSLOCompliance();
    EXPECT_TRUE(compliance.contains("error_budget"));
    EXPECT_GE(compliance.at("error_budget"), 0.0);
    EXPECT_LE(compliance.at("error_budget"), 1.0);
}

TEST_F(SLOMonitorTest, ReportGenerationStaysResponsiveDuringConcurrentUpdates) {
    const std::string shard_id = "shard-001";
    monitor_->recordShardAvailability(shard_id, true);
    monitor_->recordReplicationLag(shard_id, 10.0);

    SLOTarget updated_targets = monitor_->getTargets();
    updated_targets.max_replication_lag_ms = 5.0;

    auto update_task = std::async(std::launch::async, [this, &updated_targets]() {
        for (int i = 0; i < 100; ++i) {
            monitor_->updateTargets(updated_targets);
            updated_targets.max_replication_lag_ms = 5.0 + (i % 20);
        }
    });

    auto report_task = std::async(std::launch::async, [this]() {
        for (int i = 0; i < 100; ++i) {
            auto report = monitor_->generateSLOReportJSON();
            EXPECT_FALSE(report.empty());
        }
    });

    EXPECT_EQ(update_task.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    EXPECT_EQ(report_task.wait_for(std::chrono::seconds(2)), std::future_status::ready);

    update_task.get();
    report_task.get();
}
