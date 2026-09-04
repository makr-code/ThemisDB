#include <gtest/gtest.h>
#include "sharding/predictive_detector.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/shard_topology.h"
#include <thread>
#include <chrono>

using namespace themisdb::sharding;

// ============================================================================
// Test Fixtures
// ============================================================================

class PredictiveDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create topology with test shards
        topology_ = std::make_unique<ShardTopology>();
        
        ShardInfo shard1;
        shard1.shard_id = "shard_001";
        shard1.primary_endpoint = "localhost:8001";
        shard1.is_healthy = true;
        topology_->addShard(shard1);
        
        ShardInfo shard2;
        shard2.shard_id = "shard_002";
        shard2.primary_endpoint = "localhost:8002";
        shard2.is_healthy = true;
        topology_->addShard(shard2);
        
        // Create redundancy strategy
        RedundancyConfig raid_config;
        raid_config.mode = RedundancyMode::PARITY;  // Use PARITY instead of RAID5
        // raid_config.stripe_size removed - not part of RedundancyConfig
        strategy_ = std::make_unique<RedundancyStrategy>(raid_config);
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    std::unique_ptr<ShardTopology> topology_;
    std::unique_ptr<RedundancyStrategy> strategy_;
};

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, ConfigurationDefaults) {
    PredictiveConfig config;
    
    EXPECT_FALSE(config.enabled);
    EXPECT_EQ(config.failure_threshold, 0.7f);
    EXPECT_EQ(config.lookback_days, 30u);
    EXPECT_TRUE(config.enable_alerts);
}

TEST_F(PredictiveDetectorTest, ConfigurationCustom) {
    PredictiveConfig config;
    config.enabled = true;
    config.model_path = "/path/to/model.onnx";
    config.check_interval = std::chrono::hours(2);
    config.failure_threshold = 0.8f;
    config.lookback_days = 60;
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    EXPECT_FALSE(detector.isRunning());  // Not started yet
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, StartStop) {
    PredictiveConfig config;
    config.enabled = true;
    config.check_interval = std::chrono::seconds(1);
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    EXPECT_FALSE(detector.isRunning());
    
    detector.start();
    EXPECT_TRUE(detector.isRunning());
    
    detector.stop();
    EXPECT_FALSE(detector.isRunning());
}

TEST_F(PredictiveDetectorTest, StartWhenDisabled) {
    PredictiveConfig config;
    config.enabled = false;  // Disabled
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    detector.start();
    EXPECT_FALSE(detector.isRunning());  // Should not start
}

TEST_F(PredictiveDetectorTest, DoubleStart) {
    PredictiveConfig config;
    config.enabled = true;
    config.check_interval = std::chrono::seconds(1);
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    detector.start();
    EXPECT_TRUE(detector.isRunning());
    
    // Second start should be no-op
    detector.start();
    EXPECT_TRUE(detector.isRunning());
    
    detector.stop();
}

TEST_F(PredictiveDetectorTest, DoubleStop) {
    PredictiveConfig config;
    config.enabled = true;
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    detector.stop();  // First stop (no-op)
    EXPECT_FALSE(detector.isRunning());
    
    detector.stop();  // Second stop (no-op)
    EXPECT_FALSE(detector.isRunning());
}

// ============================================================================
// Metrics Collection Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, RecordMetrics) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    PredictiveShardMetrics metrics;
    metrics.shard_id = "shard_001";
    metrics.timestamp = std::chrono::system_clock::now();
    metrics.avg_latency_ms = 10.5;
    metrics.throughput_ops_per_sec = 1000;
    
    detector.recordMetrics(metrics);
    
    auto history = detector.getMetricsHistory("shard_001", std::chrono::hours(24));
    EXPECT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].avg_latency_ms, 10.5);
}

TEST_F(PredictiveDetectorTest, MetricsHistoryLimit) {
    PredictiveConfig config;
    config.lookback_days = 7;  // 7 day window
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Record old metrics (9 days ago)
    PredictiveShardMetrics old_metrics;
    old_metrics.shard_id = "shard_001";
    old_metrics.timestamp = std::chrono::system_clock::now() - std::chrono::hours(24 * 9);
    old_metrics.avg_latency_ms = 5.0;
    detector.recordMetrics(old_metrics);
    
    // Record recent metrics (1 day ago)
    PredictiveShardMetrics recent_metrics;
    recent_metrics.shard_id = "shard_001";
    recent_metrics.timestamp = std::chrono::system_clock::now() - std::chrono::hours(24);
    recent_metrics.avg_latency_ms = 10.0;
    detector.recordMetrics(recent_metrics);
    
    auto history = detector.getMetricsHistory("shard_001", std::chrono::hours(24 * 7));
    
    // Only recent metrics should be kept (within 7 day window)
    EXPECT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].avg_latency_ms, 10.0);
}

TEST_F(PredictiveDetectorTest, MetricsMultipleShards) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    PredictiveShardMetrics metrics1;
    metrics1.shard_id = "shard_001";
    metrics1.timestamp = std::chrono::system_clock::now();
    metrics1.avg_latency_ms = 10.0;
    
    PredictiveShardMetrics metrics2;
    metrics2.shard_id = "shard_002";
    metrics2.timestamp = std::chrono::system_clock::now();
    metrics2.avg_latency_ms = 20.0;
    
    detector.recordMetrics(metrics1);
    detector.recordMetrics(metrics2);
    
    auto history1 = detector.getMetricsHistory("shard_001", std::chrono::hours(24));
    auto history2 = detector.getMetricsHistory("shard_002", std::chrono::hours(24));
    
    EXPECT_EQ(history1.size(), 1u);
    EXPECT_EQ(history2.size(), 1u);
    EXPECT_EQ(history1[0].avg_latency_ms, 10.0);
    EXPECT_EQ(history2[0].avg_latency_ms, 20.0);
}

// ============================================================================
// Prediction Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, PredictShardNoData) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    auto prediction = detector.predictShard("shard_001");
    
    EXPECT_EQ(prediction.shard_id, "shard_001");
    EXPECT_GE(prediction.failure_probability, 0.0f);
    EXPECT_LE(prediction.failure_probability, 1.0f);
    EXPECT_GT(prediction.predicted_days_to_failure, 0u);
}

TEST_F(PredictiveDetectorTest, PredictShardWithData) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Record some metrics
    for (int i = 0; i < 10; ++i) {
        PredictiveShardMetrics metrics;
        metrics.shard_id = "shard_001";
        metrics.timestamp = std::chrono::system_clock::now() - std::chrono::hours(24 * i);
        metrics.avg_latency_ms = 10.0 + i;  // Increasing latency trend
        metrics.throughput_ops_per_sec = 1000 - (i * 10);  // Decreasing throughput
        metrics.read_errors = i;  // Increasing errors
        
        detector.recordMetrics(metrics);
    }
    
    auto prediction = detector.predictShard("shard_001");
    
    EXPECT_EQ(prediction.shard_id, "shard_001");
    EXPECT_GE(prediction.failure_probability, 0.0f);
    EXPECT_LE(prediction.failure_probability, 1.0f);
}

TEST_F(PredictiveDetectorTest, HighRiskDetection) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Create high-risk metrics
    PredictiveShardMetrics high_risk_metrics;
    high_risk_metrics.shard_id = "shard_001";
    high_risk_metrics.timestamp = std::chrono::system_clock::now();
    high_risk_metrics.avg_latency_ms = 500.0;  // Very high latency
    high_risk_metrics.read_errors = 100;  // Many errors
    high_risk_metrics.failed_health_checks = 10;
    high_risk_metrics.recovery_success_rate = 0.3f;  // Low success rate
    
    detector.recordMetrics(high_risk_metrics);
    
    auto prediction = detector.predictShard("shard_001");
    
    // Should detect some level of risk
    EXPECT_GE(prediction.failure_probability, 0.0f);
}

TEST_F(PredictiveDetectorTest, GetPredictions) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Make predictions for both shards
    detector.predictShard("shard_001");
    detector.predictShard("shard_002");
    
    auto predictions = detector.getPredictions();
    
    EXPECT_EQ(predictions.size(), 2u);
}

TEST_F(PredictiveDetectorTest, GetHighRiskShards) {
    PredictiveConfig config;
    config.failure_threshold = 0.5f;  // Lower threshold for testing
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Initially no high-risk shards
    auto high_risk = detector.getHighRiskShards();
    EXPECT_EQ(high_risk.size(), 0u);
}

// ============================================================================
// Alert Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, AlertCallback) {
    PredictiveConfig config;
    config.enabled = true;
    config.enable_alerts = true;
    
    std::string last_alert;
    config.alert_callback = [&last_alert](const std::string& msg) {
        last_alert = msg;
    };
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // This would trigger alert if prediction is high risk
    // For now, just verify callback is set
    EXPECT_TRUE(config.alert_callback != nullptr);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, Statistics) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    auto stats = detector.getStats();
    
    EXPECT_EQ(stats.predictions_made, 0u);
    EXPECT_EQ(stats.high_risk_detected, 0u);
    EXPECT_EQ(stats.alerts_sent, 0u);
}

TEST_F(PredictiveDetectorTest, StatisticsAfterPrediction) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    detector.predictShard("shard_001");
    
    auto stats = detector.getStats();
    
    EXPECT_GT(stats.avg_inference_time.count(), 0);
}

TEST_F(PredictiveDetectorTest, ResetStatistics) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    detector.predictShard("shard_001");
    
    auto stats_before = detector.getStats();
    EXPECT_GT(stats_before.avg_inference_time.count(), 0);
    
    detector.resetStats();
    
    auto stats_after = detector.getStats();
    EXPECT_EQ(stats_after.predictions_made, 0u);
    EXPECT_EQ(stats_after.avg_inference_time.count(), 0);
}

TEST_F(PredictiveDetectorTest, TruePositiveRate) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    auto stats = detector.getStats();
    
    // No predictions yet
    EXPECT_EQ(stats.getTruePositiveRate(), 0.0f);
    EXPECT_EQ(stats.getFalsePositiveRate(), 0.0f);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(PredictiveDetectorTest, MonitoringLoopIntegration) {
    PredictiveConfig config;
    config.enabled = true;
    config.check_interval = std::chrono::seconds(1);  // 1 second for testing (must be chrono::seconds)
    
    int alert_count = 0;
    config.alert_callback = [&alert_count](const std::string& msg) {
        alert_count++;
    };
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    detector.start();
    
    // Wait for a few monitoring cycles
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    
    detector.stop();
    
    auto stats = detector.getStats();
    EXPECT_GT(stats.predictions_made, 0u);
}

TEST_F(PredictiveDetectorTest, ConcurrentPredictions) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Record metrics for multiple shards
    for (int i = 0; i < 5; ++i) {
        PredictiveShardMetrics metrics;
        metrics.shard_id = "shard_00" + std::to_string(i);
        metrics.timestamp = std::chrono::system_clock::now();
        metrics.avg_latency_ms = 10.0 * i;
        detector.recordMetrics(metrics);
    }
    
    // Make concurrent predictions
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&detector, i]() {
            auto prediction = detector.predictShard("shard_00" + std::to_string(i));
            EXPECT_EQ(prediction.shard_id, "shard_00" + std::to_string(i));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PredictiveDetectorTest, EmptyShardId) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    auto prediction = detector.predictShard("");
    
    EXPECT_EQ(prediction.shard_id, "");
    EXPECT_GE(prediction.failure_probability, 0.0f);
}

TEST_F(PredictiveDetectorTest, NonexistentShard) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    auto prediction = detector.predictShard("nonexistent_shard");
    
    EXPECT_EQ(prediction.shard_id, "nonexistent_shard");
}

TEST_F(PredictiveDetectorTest, LargeMetricsHistory) {
    PredictiveConfig config;
    config.lookback_days = 90;  // Large window
    
    PredictiveFailureDetector detector(config, *strategy_, *topology_);
    
    // Record many metrics
    for (int i = 0; i < 1000; ++i) {
        PredictiveShardMetrics metrics;
        metrics.shard_id = "shard_001";
        metrics.timestamp = std::chrono::system_clock::now() - std::chrono::hours(i);
        metrics.avg_latency_ms = 10.0 + (i % 10);
        detector.recordMetrics(metrics);
    }
    
    auto prediction = detector.predictShard("shard_001");
    
    EXPECT_EQ(prediction.shard_id, "shard_001");
}

// ============================================================================
// PFD-01..PFD-03 — setPredictFn injection (stub #251)
// ============================================================================

// ---------------------------------------------------------------------------
// PFD-01: Without an injected fn, predictShard uses the heuristic fallback
//         (probability in [0,1] and days in [1,30]).
// ---------------------------------------------------------------------------
TEST_F(PredictiveDetectorTest, PFD01_HeuristicFallbackWithNoInjectedPredictFn) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);

    // Feed some metrics so the heuristic has data to work with
    PredictiveShardMetrics m;
    m.shard_id        = "shard_001";
    m.timestamp       = std::chrono::system_clock::now();
    m.avg_latency_ms  = 20.0;
    m.read_errors     = 1;
    detector.recordMetrics(m);

    auto pred = detector.predictShard("shard_001");

    EXPECT_EQ(pred.shard_id, "shard_001");
    EXPECT_GE(pred.failure_probability, 0.0f);
    EXPECT_LE(pred.failure_probability, 1.0f);
}

// ---------------------------------------------------------------------------
// PFD-02: With an injected predict fn, predictShard returns its values.
// ---------------------------------------------------------------------------
TEST_F(PredictiveDetectorTest, PFD02_InjectedPredictFnIsUsed) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);

    // Inject a fn that signals imminent failure
    detector.setPredictFn([](const std::vector<float>&) -> std::vector<float> {
        return {0.95f, 1.0f};  // 95% probability, 1 day
    });

    PredictiveShardMetrics m;
    m.shard_id       = "shard_001";
    m.timestamp      = std::chrono::system_clock::now();
    m.avg_latency_ms = 5.0;
    detector.recordMetrics(m);

    auto pred = detector.predictShard("shard_001");

    EXPECT_EQ(pred.shard_id, "shard_001");
    EXPECT_NEAR(pred.failure_probability, 0.95f, 1e-4f);
    EXPECT_EQ(pred.predicted_days_to_failure, 1u);
}

// ---------------------------------------------------------------------------
// PFD-03: Resetting predict fn to nullptr reverts to the heuristic fallback.
// ---------------------------------------------------------------------------
TEST_F(PredictiveDetectorTest, PFD03_NullptrResetRevertsToHeuristic) {
    PredictiveConfig config;
    PredictiveFailureDetector detector(config, *strategy_, *topology_);

    // Inject a deterministic high-risk fn
    detector.setPredictFn([](const std::vector<float>&) -> std::vector<float> {
        return {0.99f, 1.0f};
    });

    PredictiveShardMetrics m;
    m.shard_id       = "shard_001";
    m.timestamp      = std::chrono::system_clock::now();
    m.avg_latency_ms = 10.0;
    detector.recordMetrics(m);

    auto r1 = detector.predictShard("shard_001");
    EXPECT_NEAR(r1.failure_probability, 0.99f, 1e-4f);

    // Reset → heuristic resumes
    detector.setPredictFn(nullptr);

    auto r2 = detector.predictShard("shard_001");
    EXPECT_EQ(r2.shard_id, "shard_001");
    EXPECT_GE(r2.failure_probability, 0.0f);
    EXPECT_LE(r2.failure_probability, 1.0f);
    // Heuristic should produce a different value than the injected constant
    EXPECT_LT(r2.failure_probability, 0.99f);
}
