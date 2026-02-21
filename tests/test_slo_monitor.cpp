/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_slo_monitor.cpp                               ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2026 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/slo_monitor.h"
#include <thread>
#include <chrono>

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

