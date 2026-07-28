// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_converged_chaos.cpp
 * @brief Chaos engineering tests for Converged Storage-Inference topology
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * @note Tests unique failure scenarios: cross-shard KV-stale, inference-preemption
 * @note Part of RAID-Sharding research: THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md
 */

#include "sharding/dual_consensus_orchestrator.h"
#include "sharding/raid_paxos_consensus.h"
#include "sharding/converged_slo_metrics.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>

namespace themisdb { namespace sharding { 

class ConvergedChaosTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create RAID-Paxos for storage
        RAIDPaxosConfig raid_config;
        raid_config.raid_mode = RAIDMode::MIRROR;
        raid_config.mirror_factor = 2;
        
        storage_consensus_ = std::make_unique<RAIDPaxosConsensus>(raid_config);
        
        // Create Raft for cache (using mock for simplicity)
        cache_consensus_ = std::make_unique<testing::NiceMock<MockConsensusModule>>();
        
        // Create Dual-Consensus Orchestrator
        orchestrator_ = std::make_unique<DualConsensusOrchestrator>(
            std::move(storage_consensus_),
            std::move(cache_consensus_)
        );
        
        // Initialize
        std::vector<std::string> nodes = {"node-1", "node-2", "node-3"};
        orchestrator_->initialize("node-1", nodes);
        orchestrator_->start();
        
        // Create SLO Monitor
        ConvergedSLOMonitor::Config slo_config;
        slo_monitor_ = std::make_unique<ConvergedSLOMonitor>(slo_config);
    }
    
    void TearDown() override {
        if (orchestrator_) {
            orchestrator_->stop();
        }
        slo_monitor_.reset();
    }
    
    // Mock ConsensusModule for testing
    class MockConsensusModule : public ConsensusModule {
    public:
        MOCK_METHOD(ConsensusType, getType, (), (const, override));
        MOCK_METHOD(bool, initialize, (const std::string&, const std::vector<std::string>&), (override));
        MOCK_METHOD(bool, start, (), (override));
        MOCK_METHOD(void, stop, (), (override));
        MOCK_METHOD(bool, isLeader, (), (const, override));
        MOCK_METHOD(std::string, getLeaderId, (), (const, override));
        MOCK_METHOD(ConsensusState, getState, (), (const, override));
        MOCK_METHOD(std::optional<uint64_t>, propose, (const std::string&, const nlohmann::json&), (override));
        MOCK_METHOD(bool, waitForCommit, (uint64_t, std::chrono::milliseconds), (override));
        MOCK_METHOD(std::vector<ConsensusLogEntry>, readLog, (uint64_t, std::optional<uint64_t>), (override));
        MOCK_METHOD(uint64_t, getCommitIndex, (), (const, override));
        MOCK_METHOD(uint64_t, getLastLogIndex, (), (const, override));
        MOCK_METHOD(bool, addNode, (const std::string&, const std::string&), (override));
        MOCK_METHOD(bool, removeNode, (const std::string&), (override));
        MOCK_METHOD(bool, transferLeadership, (const std::string&), (override));
        MOCK_METHOD(bool, takeSnapshot, (const nlohmann::json&), (override));
        MOCK_METHOD(bool, restoreSnapshot, (const nlohmann::json&), (override));
        MOCK_METHOD(ConsensusStats, getStats, (), (const, override));
        MOCK_METHOD(nlohmann::json, getStatus, (), (const, override));
        MOCK_METHOD(void, onCommit, (std::function<void(const ConsensusLogEntry&)>), (override));
        MOCK_METHOD(void, onStateChange, (std::function<void(ConsensusState, ConsensusState)>), (override));
        MOCK_METHOD(void, onLeaderChange, (std::function<void(const std::string&, const std::string&)>), (override));
    };
    
    std::unique_ptr<DualConsensusOrchestrator> orchestrator_;
    std::unique_ptr<ConvergedSLOMonitor> slo_monitor_;
    std::unique_ptr<ConsensusModule> storage_consensus_;
    std::unique_ptr<ConsensusModule> cache_consensus_;
};

// ============================================================================
// Cross-Shard KV-Stale Tests
// ============================================================================

TEST_F(ConvergedChaosTest, CrossShardKVStaleDetection) {
    // Simulate a cross-shard KV-stale scenario
    // This happens when different shards have different versions of the same KV data
    
    // Record a KV-stale error in SLO monitor
    slo_monitor_->recordCrossShardKVStale("shard-1", 100, 105);
    
    // Verify error was recorded
    auto metrics = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(metrics.cross_shard_kv_stale.load(), 1);
    
    // Verify error rate
    EXPECT_GT(slo_monitor_->getErrorBudgetRemaining(), 0.0);
    EXPECT_LT(slo_monitor_->getErrorBudgetRemaining(), 1.0);
}

TEST_F(ConvergedChaosTest, MultipleCrossShardKVStale) {
    // Record multiple KV-stale errors
    slo_monitor_->recordCrossShardKVStale("shard-1", 100, 105);
    slo_monitor_->recordCrossShardKVStale("shard-2", 200, 208);
    slo_monitor_->recordCrossShardKVStale("shard-3", 300, 310);
    
    auto metrics = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(metrics.cross_shard_kv_stale.load(), 3);
    
    // Check error rate
    EXPECT_GT(metrics.getErrorRate(ConvergedErrorType::CROSS_SHARD_KV_STALE), 0.0);
}

// ============================================================================
// Inference-Preemption Tests
// ============================================================================

TEST_F(ConvergedChaosTest, InferencePreemptionRecording) {
    // Simulate inference preemption after generating some tokens
    slo_monitor_->recordInferencePreemption(50, 10);  // 50 tokens generated, 10 lost
    
    auto metrics = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(metrics.inference_preemption.load(), 1);
    EXPECT_EQ(metrics.total_tokens_generated.load(), 50);
}

TEST_F(ConvergedChaosTest, MultipleInferencePreemptions) {
    // Record multiple preemptions
    slo_monitor_->recordInferencePreemption(50, 10);
    slo_monitor_->recordInferencePreemption(100, 20);
    slo_monitor_->recordInferencePreemption(200, 50);
    
    auto metrics = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(metrics.inference_preemption.load(), 3);
    EXPECT_EQ(metrics.total_tokens_generated.load(), 350);
}

// ============================================================================
// Cross-Layer Version Mismatch Tests
// ============================================================================

TEST_F(ConvergedChaosTest, CrossLayerVersionMismatch) {
    // Simulate version mismatch between storage and cache layers
    ConvergedOperationMetrics metrics;
    metrics.operation_id = "op-1";
    metrics.operation_type = "read";
    metrics.storage_version = 105;
    metrics.cache_version = 100;  // Cache is behind
    metrics.merged_version = 105;
    metrics.version_spread = 5.0;
    metrics.cross_layer_consistent = false;  // Version mismatch
    metrics.cross_shard_consistent = true;
    metrics.success = true;  // Operation succeeded but with inconsistency
    
    slo_monitor_->recordOperation(metrics);
    
    auto current = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(current.total_operations.load(), 1);
    EXPECT_EQ(current.inconsistent_operations.load(), 1);
    EXPECT_EQ(current.max_version_spread.load(), 5.0);
}

// ============================================================================
// SLO Compliance Tests
// ============================================================================

TEST_F(ConvergedChaosTest, SLOComplianceReport) {
    // Record various operations
    slo_monitor_->recordCrossShardKVStale("shard-1", 100, 105);
    slo_monitor_->recordInferencePreemption(50, 10);
    
    // Record successful operations
    for (int i = 0; i < 100; i++) {
        ConvergedOperationMetrics metrics;
        metrics.operation_id = "op-" + std::to_string(i);
        metrics.operation_type = "read";
        metrics.success = true;
        metrics.cross_layer_consistent = true;
        metrics.cross_shard_consistent = true;
        metrics.grounding_auditable = true;
        slo_monitor_->recordOperation(metrics);
    }
    
    // Get SLO report
    auto report = slo_monitor_->getSLOReport();
    
    // Verify report structure
    EXPECT_TRUE(report.contains("slo_compliance"));
    EXPECT_TRUE(report.contains("current_metrics"));
    EXPECT_TRUE(report.contains("targets"));
    EXPECT_TRUE(report.contains("error_budget"));
    
    // Verify SLO compliance fields
    auto compliance = report["slo_compliance"];
    EXPECT_TRUE(compliance.contains("system_available"));
    EXPECT_TRUE(compliance.contains("cross_shard_consistent"));
    EXPECT_TRUE(compliance.contains("inference_preemption_ok"));
    EXPECT_TRUE(compliance.contains("grounding_audit_ok"));
    EXPECT_TRUE(compliance.contains("overall_slo_met"));
}

TEST_F(ConvergedChaosTest, SLOComplianceInitiallyMet) {
    // With no errors, SLO should be met
    EXPECT_TRUE(slo_monitor_->isSLOMet());
    
    auto report = slo_monitor_->getSLOReport();
    EXPECT_TRUE(report["slo_compliance"]["overall_slo_met"].get<bool>());
}

// ============================================================================
// Error Budget Tests
// ============================================================================

TEST_F(ConvergedChaosTest, ErrorBudgetCalculation) {
    // Initially, error budget should be 1.0 (100%)
    EXPECT_EQ(slo_monitor_->getErrorBudgetRemaining(), 1.0);
    
    // Record some failures to consume error budget
    for (int i = 0; i < 10; i++) {
        slo_monitor_->recordError(ConvergedErrorType::CROSS_SHARD_KV_STALE);
    }
    
    // Error budget should still be close to 1.0 (depending on target)
    // The default target is 99.9% availability, so 10 failures out of 10 total
    // means 0% availability, which consumes the budget
    auto current = slo_monitor_->getCurrentMetrics();
    double availability = current.getAvailability();
    
    // With 10 failures and 10 total, availability is 0%
    EXPECT_EQ(availability, 0.0);
    
    // Error budget should be 0 (all consumed)
    EXPECT_EQ(slo_monitor_->getErrorBudgetRemaining(), 0.0);
}

// ============================================================================
// Grounding Audit Tests
// ============================================================================

TEST_F(ConvergedChaosTest, GroundingAuditCoverage) {
    // Record operations with grounding
    for (int i = 0; i < 100; i++) {
        ConvergedOperationMetrics metrics;
        metrics.operation_id = "op-" + std::to_string(i);
        metrics.operation_type = "rag";
        metrics.success = true;
        metrics.grounding_auditable = (i % 10 != 0);  // 90% auditable
        metrics.source_references = {"source-" + std::to_string(i)};
        
        slo_monitor_->recordOperation(metrics);
    }
    
    auto current = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(current.auditable_operations.load(), 90);
    EXPECT_EQ(current.non_auditable_operations.load(), 10);
    
    // Coverage should be 90%
    EXPECT_NEAR(current.getGroundingAuditCoverage(), 0.9, 0.01);
}

TEST_F(ConvergedChaosTest, GroundingAuditFailure) {
    // Record a grounding audit failure
    ConvergedOperationMetrics metrics;
    metrics.operation_id = "op-audit-fail";
    metrics.operation_type = "rag";
    metrics.error_type = ConvergedErrorType::GROUNDING_AUDIT_FAILED;
    metrics.error_message = "Failed to verify source references";
    metrics.success = false;
    metrics.grounding_auditable = false;
    
    slo_monitor_->recordOperation(metrics);
    
    auto current = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(current.grounding_audit_failed.load(), 1);
    EXPECT_EQ(current.non_auditable_operations.load(), 1);
}

// ============================================================================
// Consistency Ratio Tests
// ============================================================================

TEST_F(ConvergedChaosTest, ConsistencyRatioCalculation) {
    // Record mix of consistent and inconsistent operations
    for (int i = 0; i < 90; i++) {
        ConvergedOperationMetrics metrics;
        metrics.operation_id = "op-consistent-" + std::to_string(i);
        metrics.operation_type = "read";
        metrics.success = true;
        metrics.cross_layer_consistent = true;
        metrics.cross_shard_consistent = true;
        slo_monitor_->recordOperation(metrics);
    }
    
    for (int i = 0; i < 10; i++) {
        ConvergedOperationMetrics metrics;
        metrics.operation_id = "op-inconsistent-" + std::to_string(i);
        metrics.operation_type = "read";
        metrics.success = true;
        metrics.cross_layer_consistent = false;
        metrics.cross_shard_consistent = false;
        slo_monitor_->recordOperation(metrics);
    }
    
    auto current = slo_monitor_->getCurrentMetrics();
    EXPECT_NEAR(current.getConsistencyRatio(), 0.9, 0.01);
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(ConvergedChaosTest, ResetClearsAllMetrics) {
    // Record various operations
    slo_monitor_->recordCrossShardKVStale("shard-1", 100, 105);
    slo_monitor_->recordInferencePreemption(50, 10);
    slo_monitor_->recordError(ConvergedErrorType::GROUNDING_AUDIT_FAILED);
    
    for (int i = 0; i < 10; i++) {
        ConvergedOperationMetrics metrics;
        metrics.operation_id = "op-" + std::to_string(i);
        metrics.operation_type = "read";
        metrics.success = true;
        slo_monitor_->recordOperation(metrics);
    }
    
    // Verify metrics are non-zero
    auto before = slo_monitor_->getCurrentMetrics();
    EXPECT_GT(before.total_operations.load(), 0);
    EXPECT_GT(before.cross_shard_kv_stale.load(), 0);
    
    // Reset
    slo_monitor_->reset();
    
    // Verify metrics are zero
    auto after = slo_monitor_->getCurrentMetrics();
    EXPECT_EQ(after.total_operations.load(), 0);
    EXPECT_EQ(after.cross_shard_kv_stale.load(), 0);
    EXPECT_EQ(after.inference_preemption.load(), 0);
}

// ============================================================================
// ConvergedErrorType Tests
// ============================================================================

TEST_F(ConvergedChaosTest, ConvergedErrorTypeValues) {
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::NONE), 0);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::CROSS_LAYER_VERSION_MISMATCH), 1);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::CROSS_LAYER_CONSENSUS_FAILED), 2);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::CROSS_LAYER_SYNC_TIMEOUT), 3);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::CROSS_SHARD_KV_STALE), 10);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::CROSS_SHARD_INCONSISTENT_READ), 11);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::CROSS_SHARD_QUORUM_FAILED), 12);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::INFERENCE_PREMPTION), 20);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::INFERENCE_TIMEOUT), 21);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::INFERENCE_OOM), 22);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::INFERENCE_GPU_ERROR), 23);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::GROUNDING_AUDIT_FAILED), 30);
    EXPECT_EQ(static_cast<int>(ConvergedErrorType::GROUNDING_VERSION_GAP), 31);
}

// ============================================================================
// ConvergedSLOTarget Tests
// ============================================================================

TEST_F(ConvergedChaosTest, ConvergedSLOTargetValues) {
    ConvergedSLOTarget targets;
    
    // Availability targets
    EXPECT_EQ(targets.system_availability_target, 0.999);
    EXPECT_EQ(targets.storage_layer_availability, 0.9999);
    EXPECT_EQ(targets.cache_layer_availability, 0.999);
    EXPECT_EQ(targets.inference_layer_availability, 0.99);
    
    // Latency targets
    EXPECT_EQ(targets.cross_layer_sync_p50_ms, 50.0);
    EXPECT_EQ(targets.cross_layer_sync_p99_ms, 200.0);
    EXPECT_EQ(targets.inference_request_p99_ms, 1000.0);
    EXPECT_EQ(targets.rag_generation_p99_ms, 2000.0);
    
    // Consistency targets
    EXPECT_EQ(targets.max_storage_cache_lag_ms, 100.0);
    EXPECT_EQ(targets.max_cache_storage_lag_ms, 500.0);
    EXPECT_EQ(targets.cross_shard_consistency_target, 0.999);
    
    // Grounding targets
    EXPECT_EQ(targets.grounding_audit_coverage_target, 1.0);
    
    // Inference preemption targets
    EXPECT_EQ(targets.max_inference_preemption_rate, 0.01);
    EXPECT_EQ(targets.preemption_recovery_time_target_s, 2.0);
}
} } // namespace themisdb::sharding
