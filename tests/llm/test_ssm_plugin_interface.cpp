/**
 * @file test_ssm_plugin_interface.cpp
 * @brief Phase 1 unit tests for SSM plugin interface and state store (P1-D07).
 * @version 0.1.0-alpha
 * @note Test Suite: module_llm_test_ssm_plugin_interface_focused
 * @note Timeout: 120 seconds (TIMEOUT flag in CMakeLists.txt)
 */

#include <gtest/gtest.h>

#include "llm/context_quality_metrics.h"
#include "llm/i_ssm_plugin.h"
#include "llm/infini_attention_cpu.h"
#include "llm/ssm_drift_metrics.h"
#include "llm/ssm_state_store.h"
#include "llm/ssm_stub_plugin.h"

#include <Eigen/Dense>
#include <thread>

using namespace themis::llm;

// ============================================================================
// P1-D01: ISSMPlugin Interface Tests
// ============================================================================

class ISSMPluginInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<SyntheticSSMStub>();
        ASSERT_TRUE(plugin_->initialize());
    }

    std::unique_ptr<SyntheticSSMStub> plugin_;
};

TEST_F(ISSMPluginInterfaceTest, PluginMetadata) {
    EXPECT_EQ(plugin_->getName(), "synthetic-ssm-stub");
    EXPECT_EQ(plugin_->getVersion(), "0.1.0-alpha");
    EXPECT_TRUE(plugin_->isAvailable());
}

TEST_F(ISSMPluginInterfaceTest, UpdateStateValidation) {
    std::vector<int32_t> tokens = {100, 101, 102, 103};
    EXPECT_TRUE(plugin_->updateState(tokens));

    // Invalid tokens should fail
    std::vector<int32_t> invalid_tokens = {-1, 100000};
    EXPECT_FALSE(plugin_->updateState(invalid_tokens));
}

TEST_F(ISSMPluginInterfaceTest, StateSnapshotRoundTrip) {
    // Update state
    std::vector<int32_t> tokens = {50, 51, 52, 53};
    EXPECT_TRUE(plugin_->updateState(tokens));

    // Get snapshot with dummy HLC timestamp
    core::HLCTimestamp ts{.wall_clock_ms = 1000, .logical_clock = 1};
    SSMStateSnapshot snap = plugin_->getStateSnapshot(ts);

    EXPECT_EQ(snap.snapshot_ts.wall_clock_ms, 1000);
    EXPECT_EQ(snap.sequence_counter, 4);  // 4 tokens processed
    EXPECT_FALSE(snap.state_fingerprint.empty());

    // Restore state
    EXPECT_TRUE(plugin_->restoreState(snap));

    // Verify retention score is valid
    double retention = plugin_->getStateRetentionScore();
    EXPECT_GE(retention, 0.0);
    EXPECT_LE(retention, 1.0);
}

TEST_F(ISSMPluginInterfaceTest, FingerPrintValidation) {
    std::vector<int32_t> tokens = {100, 101};
    EXPECT_TRUE(plugin_->updateState(tokens));

    core::HLCTimestamp ts{.wall_clock_ms = 2000, .logical_clock = 2};
    SSMStateSnapshot snap = plugin_->getStateSnapshot(ts);

    // Corrupt fingerprint
    std::string original_fp = snap.state_fingerprint;
    snap.state_fingerprint = "corrupted-fp";

    EXPECT_FALSE(plugin_->restoreState(snap));  // Should reject

    // Restore with correct fingerprint
    snap.state_fingerprint = original_fp;
    EXPECT_TRUE(plugin_->restoreState(snap));
}

TEST_F(ISSMPluginInterfaceTest, ResetState) {
    std::vector<int32_t> tokens = {100, 101, 102};
    plugin_->updateState(tokens);

    plugin_->resetState();
    double retention_after_reset = plugin_->getStateRetentionScore();
    EXPECT_LE(retention_after_reset, 0.5);  // Retention drops after reset
}

// ============================================================================
// P1-D02: SSMStateStore Interface Tests
// ============================================================================

class SSMStateStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<InMemorySSMStateStore>(10);
    }

    std::unique_ptr<InMemorySSMStateStore> store_;

    SSMStateSnapshot makeSnapshot(const std::string& fp, uint64_t seq,
                                   core::HLCTimestamp ts) {
        SSMStateSnapshot snap;
        snap.state_fingerprint = fp;
        snap.sequence_counter = seq;
        snap.snapshot_ts = ts;
        snap.state_data.resize(128, 42);  // Dummy data
        return snap;
    }
};

TEST_F(SSMStateStoreTest, CheckpointResumeRoundTrip) {
    std::string session_id = "test-session-1";
    core::HLCTimestamp ts{.wall_clock_ms = 1000, .logical_clock = 1};
    SSMStateSnapshot snap = makeSnapshot("fp-v0", 100, ts);

    // Checkpoint
    EXPECT_TRUE(store_->checkpoint(session_id, snap));

    // Resume
    auto resumed = store_->resume(session_id, ts);
    ASSERT_TRUE(resumed.has_value());
    EXPECT_EQ(resumed->sequence_counter, 100);
    EXPECT_EQ(resumed->state_fingerprint, "fp-v0");
}

TEST_F(SSMStateStoreTest, DuplicateCheckpointRejected) {
    std::string session_id = "test-session-2";
    core::HLCTimestamp ts{.wall_clock_ms = 2000, .logical_clock = 2};
    SSMStateSnapshot snap1 = makeSnapshot("fp-v0", 50, ts);
    SSMStateSnapshot snap2 = makeSnapshot("fp-v0", 60, ts);  // Same timestamp

    EXPECT_TRUE(store_->checkpoint(session_id, snap1));
    EXPECT_FALSE(store_->checkpoint(session_id, snap2));  // Duplicate rejected
}

TEST_F(SSMStateStoreTest, ResumeLatestSnapshot) {
    std::string session_id = "test-session-3";

    core::HLCTimestamp ts1{.wall_clock_ms = 1000, .logical_clock = 1};
    core::HLCTimestamp ts2{.wall_clock_ms = 2000, .logical_clock = 2};

    SSMStateSnapshot snap1 = makeSnapshot("fp-v0", 50, ts1);
    SSMStateSnapshot snap2 = makeSnapshot("fp-v0", 100, ts2);

    store_->checkpoint(session_id, snap1);
    store_->checkpoint(session_id, snap2);

    // Resume without timestamp should return latest
    auto latest = store_->resume(session_id, std::nullopt);
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->sequence_counter, 100);  // Should be snap2
}

TEST_F(SSMStateStoreTest, InvalidateSession) {
    std::string session_id = "test-session-4";
    core::HLCTimestamp ts{.wall_clock_ms = 3000, .logical_clock = 3};
    SSMStateSnapshot snap = makeSnapshot("fp-v0", 200, ts);

    store_->checkpoint(session_id, snap);
    EXPECT_TRUE(store_->invalidate(session_id));

    // Should not find after invalidate
    auto result = store_->resume(session_id, std::nullopt);
    EXPECT_FALSE(result.has_value());
}

TEST_F(SSMStateStoreTest, StoreStats) {
    std::string session_id = "test-session-5";
    core::HLCTimestamp ts{.wall_clock_ms = 4000, .logical_clock = 4};
    SSMStateSnapshot snap = makeSnapshot("fp-v0", 300, ts);

    store_->checkpoint(session_id, snap);
    std::string stats_json = store_->getStats();

    EXPECT_TRUE(stats_json.find("session_count") != std::string::npos);
    EXPECT_TRUE(stats_json.find("total_snapshots") != std::string::npos);
}

// ============================================================================
// P1-D03: SyntheticSSMStub Tests
// ============================================================================

class SyntheticSSMStubTest : public ::testing::Test {
protected:
    void SetUp() override {
        stub_ = std::make_unique<SyntheticSSMStub>();
        ASSERT_TRUE(stub_->initialize());
    }

    std::unique_ptr<SyntheticSSMStub> stub_;
};

TEST_F(SyntheticSSMStubTest, DeterministicBehavior) {
    // Seed=42 should produce deterministic results
    std::vector<int32_t> tokens = {1, 2, 3, 4, 5};
    stub_->updateState(tokens);

    double retention1 = stub_->getStateRetentionScore();

    // Reset and replay
    stub_->resetState();
    stub_->updateState(tokens);
    double retention2 = stub_->getStateRetentionScore();

    // Retention should be deterministic (same seed)
    EXPECT_DOUBLE_EQ(retention1, retention2);
}

TEST_F(SyntheticSSMStubTest, StateRetentionDecay) {
    // Retention should decay with token count
    std::vector<int32_t> batch1(100, 1);
    stub_->updateState(batch1);
    double retention_100 = stub_->getStateRetentionScore();

    std::vector<int32_t> batch2(8000, 1);
    stub_->updateState(batch2);
    double retention_8100 = stub_->getStateRetentionScore();

    // More tokens → lower retention
    EXPECT_GE(retention_100, retention_8100);
}

// ============================================================================
// P1-D04: Infini-Attention CPU Tests
// ============================================================================

class InfiniAttentionCPUTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.hidden_dim = 64;
        config_.seq_len = 32;
        config_.memory_size = 128;
        config_.epsilon = 1e-6f;
        config_.use_fp64 = false;

        engine_ = std::make_unique<attention::InfiniAttentionCPU>(config_);
        ASSERT_TRUE(engine_->initialize());
    }

    attention::InfiniAttentionCPU::Config config_;
    std::unique_ptr<attention::InfiniAttentionCPU> engine_;
};

TEST_F(InfiniAttentionCPUTest, ForwardPassBasic) {
    Eigen::MatrixXf Q = Eigen::MatrixXf::Random(config_.seq_len, config_.hidden_dim);
    Eigen::MatrixXf K = Eigen::MatrixXf::Random(config_.seq_len, config_.hidden_dim);
    Eigen::MatrixXf V = Eigen::MatrixXf::Random(config_.seq_len, config_.hidden_dim);
    Eigen::MatrixXf output = Eigen::MatrixXf::Zero(config_.seq_len, config_.hidden_dim);

    EXPECT_TRUE(engine_->forward(Q, K, V, output));
    EXPECT_FALSE(output.isZero());  // Output should be non-zero after forward pass
}

TEST_F(InfiniAttentionCPUTest, MemorySnapshot) {
    Eigen::MatrixXf Q = Eigen::MatrixXf::Random(config_.seq_len, config_.hidden_dim);
    Eigen::MatrixXf K = Eigen::MatrixXf::Random(config_.seq_len, config_.hidden_dim);
    Eigen::MatrixXf V = Eigen::MatrixXf::Random(config_.seq_len, config_.hidden_dim);
    Eigen::MatrixXf output = Eigen::MatrixXf::Zero(config_.seq_len, config_.hidden_dim);

    engine_->forward(Q, K, V, output);

    // Get snapshot
    auto snapshot = engine_->getMemorySnapshot();
    EXPECT_EQ(snapshot.size(), config_.hidden_dim * config_.memory_size);

    // Reset and restore
    engine_->resetMemory();
    EXPECT_TRUE(engine_->restoreMemory(snapshot));
}

TEST_F(InfiniAttentionCPUTest, MemoryStats) {
    auto stats = engine_->getMemoryStats();
    EXPECT_GT(stats.total_bytes, 0);
    EXPECT_EQ(stats.total_bytes, stats.memory_matrix_bytes + stats.temp_buffer_bytes);
}

// ============================================================================
// P1-D05: Drift Metrics Tests
// ============================================================================

TEST(DriftMetricsTest, RecordAndRetrieve) {
    auto& metrics = metrics::SSMDriftMetrics::instance();

    // Record drift
    metrics.recordFactualDriftScore("session-1", 0.5);
    double drift = metrics.getFactualDriftScore("session-1");
    EXPECT_GT(drift, 0.0);
    EXPECT_LT(drift, 1.0);
}

TEST(DriftMetricsTest, CheckpointCounter) {
    auto& metrics = metrics::SSMDriftMetrics::instance();

    uint64_t before = metrics.getTotalCheckpoints();
    metrics.recordSSMStateCheckpoint("session-2", 1024);
    uint64_t after = metrics.getTotalCheckpoints();

    EXPECT_EQ(after, before + 1);
}

TEST(DriftMetricsTest, RouterDecisions) {
    auto& metrics = metrics::SSMDriftMetrics::instance();

    metrics.recordHybridRouterDecision("transformer");
    metrics.recordHybridRouterDecision("infini");
    metrics.recordHybridRouterDecision("ssm");

    std::string stats = metrics.getRouterDecisionStats();
    EXPECT_TRUE(stats.find("router_transformer") != std::string::npos);
    EXPECT_TRUE(stats.find("router_infini") != std::string::npos);
    EXPECT_TRUE(stats.find("router_ssm") != std::string::npos);
}

TEST(DriftMetricsTest, PrometheusExport) {
    auto& metrics = metrics::SSMDriftMetrics::instance();

    metrics.recordFactualDriftScore("session-3", 0.25);
    metrics.recordSSMStateCheckpoint("session-3", 2048);

    std::string prometheus = metrics.exportPrometheus();
    EXPECT_TRUE(prometheus.find("themis_factual_drift_score") !=
                std::string::npos);
    EXPECT_TRUE(prometheus.find("themis_ssm_state_checkpoints_total") !=
                std::string::npos);
    EXPECT_TRUE(prometheus.find("themis_hybrid_router_decision_total") !=
                std::string::npos);
}

// ============================================================================
// P1-D06: ContextQualityMetrics Tests
// ============================================================================

TEST(ContextQualityMetricsTest, QualityDecisions) {
    ContextQualityMetrics metrics;

    // High quality: should prefer Transformer
    metrics.state_retention_score = 0.9;
    metrics.factual_drift_estimate = 0.95;  // 5% drift
    EXPECT_TRUE(metrics.isTransformerQuality());
    EXPECT_FALSE(metrics.shouldRefreshRAG());

    // Medium quality: Infini acceptable, but RAG refresh needed
    metrics.state_retention_score = 0.6;
    metrics.factual_drift_estimate = 0.85;  // 15% drift
    EXPECT_TRUE(metrics.isInfiniQuality());
    EXPECT_TRUE(metrics.shouldRefreshRAG());

    // Low quality: SSM only
    metrics.state_retention_score = 0.4;
    metrics.factual_drift_estimate = 0.5;  // 50% drift
    EXPECT_TRUE(metrics.isSSMQuality());
    EXPECT_TRUE(metrics.shouldRefreshRAG());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(SSMStateStoreTest, ThreadSafety) {
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    auto worker = [this, &errors](const std::string& session_id) {
        for (int i = 0; i < 10; ++i) {
            core::HLCTimestamp ts{.wall_clock_ms = static_cast<uint64_t>(i * 1000),
                                  .logical_clock = static_cast<uint32_t>(i)};
            SSMStateSnapshot snap = makeSnapshot("fp", i, ts);

            if (!store_->checkpoint(session_id, snap)) {
                errors++;
            }
        }
    };

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back(worker, "session-" + std::to_string(t));
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(errors, 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class Phase1IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<InMemorySSMStateStore>(5);
        plugin_ = std::make_unique<SyntheticSSMStub>();
        plugin_->initialize();
    }

    std::unique_ptr<InMemorySSMStateStore> store_;
    std::unique_ptr<SyntheticSSMStub> plugin_;
};

TEST_F(Phase1IntegrationTest, EndToEndStateLifecycle) {
    // 1. Process tokens with plugin
    std::vector<int32_t> tokens = {10, 11, 12, 13, 14};
    EXPECT_TRUE(plugin_->updateState(tokens));

    // 2. Capture state
    core::HLCTimestamp ts{.wall_clock_ms = 5000, .logical_clock = 5};
    SSMStateSnapshot snap = plugin_->getStateSnapshot(ts);

    // 3. Store checkpoint
    std::string session_id = "integration-test";
    EXPECT_TRUE(store_->checkpoint(session_id, snap));

    // 4. Simulate session restart: resume from checkpoint
    auto recovered = store_->resume(session_id, ts);
    ASSERT_TRUE(recovered.has_value());

    // 5. Restore plugin state
    plugin_->resetState();
    EXPECT_TRUE(plugin_->restoreState(recovered.value()));

    // 6. Verify state restored
    double retention = plugin_->getStateRetentionScore();
    EXPECT_GE(retention, 0.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

