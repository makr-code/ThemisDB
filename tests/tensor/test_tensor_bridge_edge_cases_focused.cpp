// tests/tensor/test_tensor_bridge_edge_cases_focused.cpp
// Stream B Block 1 - Deterministic Edge Scenario Handling
// Comprehensive test suite for tensor edge case handler (TEDGE-01..30)

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>

#include "tensor/tensor_edge_case_handler.h"
#include "utils/error_registry.h"
namespace themis::tensor::test {

using ::testing::Test;

// ============================================================================
// FIXTURE: TensorEdgeCaseHandlerFixture
// ============================================================================
class TensorEdgeCaseHandlerFixture : public Test {
 protected:
  TensorEdgeCaseHandlerFixture() : handler_() {}

  void SetUp() override {
    handler_.resetStats();
  }

  void TearDown() override {
    // Verify handler was exercised
    const auto stats = handler_.getStats();
    const std::size_t total =
        stats.invalid_adapter_refs +
        stats.out_of_bounds_accesses +
        stats.stale_fingerprints +
        stats.self_similarity_failures +
        stats.null_train_comparisons +
        stats.concurrent_modifications +
        stats.bridge_routing_failures +
        stats.adapter_comm_failures +
        stats.invalid_decompositions +
        stats.kappa_gate_violations +
        stats.export_serialization_failures +
        stats.replay_deserialization_failures +
        stats.partial_graph_losses +
        stats.oom_during_computation +
        stats.concurrent_memory_exhaustion;
    EXPECT_GT(total, 0u)
        << "Handler should have processed at least one scenario";
  }

  themis::tensor::TensorEdgeCaseHandler handler_;
};

// ============================================================================
// TEDGE-01: Invalid Adapter Reference
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_01_InvalidAdapterReference) {
  // Arrange: Invalid adapter ID
  std::string invalid_adapter_key = "nonexistent_adapter_xyz";
  std::string context = "findSimilar";
  
  // Act
  auto result = handler_.handleInvalidAdapterReference(
      invalid_adapter_key, context);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_ADAPTER_NOT_FOUND));
  EXPECT_FALSE(result.is_recoverable);
}

// ============================================================================
// TEDGE-02: Out-of-Bounds Index Access
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_02_OutOfBoundsIndexAccess) {
  // Arrange: Request k > graph size
  size_t requested_k = 99999;
  size_t actual_size = 10;
  
  // Act
  auto result = handler_.handleOutOfBoundsIndex(requested_k, actual_size);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_INDEX_LOOKUP_FAILED));
  EXPECT_TRUE(result.is_recoverable);
  EXPECT_EQ(result.recovery_action, "degrade");
}

// ============================================================================
// TEDGE-03: Stale/Invalid Fingerprint (NaN/Inf)
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_03_StaleFingerprintNaN) {
  // Arrange: Fingerprint with NaN norm
  std::string adapter_key = "adapter_with_nan";
  float norm_value = std::numeric_limits<float>::quiet_NaN();
  
  // Act
  auto result = handler_.handleStaleFingerprint(adapter_key, norm_value);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_GRAPH_INVALID_SELF_IP));
  EXPECT_FALSE(result.is_recoverable);
}

// ============================================================================
// TEDGE-04: Self-Similarity Computation Failure
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_04_SelfSimilarityComputationFailure) {
  // Arrange: Self-similarity should be 1.0, but got 0.5
  std::string adapter_key = "adapter_bad_self_sim";
  float computed_score = 0.5f;  // Should be 1.0 for self-similarity
  
  // Act
  auto result = handler_.handleSelfSimilarityFailure(adapter_key, computed_score);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_GRAPH_INVALID_SELF_IP));
}

// ============================================================================
// TEDGE-05: Null Train in Comparison
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_05_NullTrainInComparison) {
  // Arrange: Null/empty train in comparison
  std::string key_a = "adapter_a";
  std::string key_b = "adapter_b_with_null_train";
  std::string null_train_key = key_b;
  
  // Act
  auto result = handler_.handleNullTrainComparison(key_a, key_b, null_train_key);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_GRAPH_OTHER_TRAIN_NOT_FOUND));
  EXPECT_FALSE(result.is_recoverable);
}

// ============================================================================
// TEDGE-06: Concurrent Modification Conflict
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_06_ConcurrentModificationConflict) {
  // Arrange: Graph modified during traversal
  std::string operation = "findSimilar";
  std::string affected_key = "adapter_removed";
  
  // Act
  auto result = handler_.handleConcurrentModification(operation, affected_key);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_CONCURRENT_MODIFICATION));
  EXPECT_TRUE(result.is_recoverable);
  EXPECT_EQ(result.recovery_action, "retry");
}

// ============================================================================
// TEDGE-07: Bridge Routing Failure
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_07_BridgeRoutingFailure) {
  // Arrange: Routing queue full
  double load_level = 1.2;  // Over capacity
  size_t queue_depth = 1000;
  
  // Act
  auto result = handler_.handleBridgeRoutingFailure(load_level, queue_depth);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_INDEX_ROUTING_FAILED));
  EXPECT_TRUE(result.is_recoverable);
}

// ============================================================================
// TEDGE-08: Adapter Communication Failure
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_08_AdapterCommunicationFailure) {
  // Arrange: Adapter communication timeout
  std::string adapter_key = "adapter_unreachable";
  uint32_t timeout_ms = 5000;
  
  // Act
  auto result = handler_.handleAdapterCommunicationFailure(adapter_key, timeout_ms);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_ADAPTER_COMMUNICATION_ERROR));
  EXPECT_TRUE(result.is_recoverable);
}

// ============================================================================
// TEDGE-09: Invalid Decomposition Parameters
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_09_InvalidDecompositionParameters) {
  // Arrange: Decomposition result invalid (e.g., NaN in cores)
  std::string chunk_id = "chunk_001";
  std::string error_detail = "rank_overflow";
  
  // Act
  auto result = handler_.handleInvalidDecompositionResult(chunk_id, error_detail);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED));
}

// ============================================================================
// TEDGE-10: Kappa-Gate Violation (Threshold Breach)
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_10_KappaGateViolation) {
  // Arrange: Estimated kappa below minimum threshold
  size_t embedding_dim = 1024;
  double estimated_kappa = 2.5;  // Below minimum
  double min_kappa = 5.0;
  
  // Act
  auto result = handler_.handleKappaGateViolation(embedding_dim, estimated_kappa, min_kappa);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED));
}

// ============================================================================
// TEDGE-11: Export Serialization Failure
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_11_ExportSerializationFailure) {
  // Arrange: Export path write failed
  std::string export_path = "/invalid/path/graph.pb";
  std::string error_detail = "permission_denied";
  
  // Act
  auto result = handler_.handleExportSerializationFailure(export_path, error_detail);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_PERSISTENCE_FAILED));
}

// ============================================================================
// TEDGE-12: Replay Deserialization Failure
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_12_ReplayDeserializationFailure) {
  // Arrange: Corrupted fingerprint during replay
  std::string adapter_key = "corrupt_entry_001";
  std::string corruption_type = "invalid_checksum";
  
  // Act
  auto result = handler_.handleReplayDeserialization(adapter_key, corruption_type);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_PERSISTENCE_FAILED));
}

// ============================================================================
// TEDGE-13: Partial Graph Loss (Incomplete Edges)
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_13_PartialGraphLoss) {
  // Arrange: Some entries lost in replay
  size_t total_entries = 1000;
  size_t recovered_entries = 850;  // 15% loss
  
  // Act
  auto result = handler_.handlePartialGraphLossRecovery(total_entries, recovered_entries);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_PERSISTENCE_FAILED));
  EXPECT_TRUE(result.is_recoverable);
}

// ============================================================================
// TEDGE-14: Out-of-Memory During Computation
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_14_OutOfMemoryDuringComputation) {
  // Arrange: Allocation exceeds available memory
  size_t requested_bytes = 32ULL * 1024 * 1024 * 1024;  // 32 GB
  std::string operation = "fingerprint_compute";
  
  // Act
  auto result = handler_.handleOutOfMemoryDuringComputation(requested_bytes, operation);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED));
  EXPECT_FALSE(result.is_recoverable);
}

// ============================================================================
// TEDGE-15: Concurrent Memory Exhaustion
// ============================================================================
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_15_ConcurrentMemoryExhaustion) {
  // Arrange: Multiple threads exhausting memory simultaneously
  size_t current_memory_bytes = 7ULL * 1024 * 1024 * 1024;  // 7 GB
  size_t max_memory_bytes = 8ULL * 1024 * 1024 * 1024;      // 8 GB limit
  size_t concurrent_threads = 8;
  
  // Act
  auto result = handler_.handleConcurrentMemoryExhaustion(
      current_memory_bytes, max_memory_bytes, concurrent_threads);
  
  // Assert
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_LOCK_ACQUISITION_FAILED));
  EXPECT_FALSE(result.is_recoverable);
}

// ============================================================================
// PROPERTY-BASED TESTS: Combinations & Stress Scenarios (TEDGE-16..30)
// ============================================================================

// TEDGE-16: Multiple Consecutive Out-of-Bounds Accesses
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_16_MultipleConsecutiveOutOfBoundsAccesses) {
  std::vector<size_t> requests = {100, 500, 1000, 5000};
  size_t actual_size = 50;
  
  for (size_t req : requests) {
    auto result = handler_.handleOutOfBoundsIndex(req, actual_size);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, static_cast<int>(themis::errors::ErrorCode::ERR_TENSOR_INDEX_LOOKUP_FAILED));
  }
}

// TEDGE-17: Recovery Path Retry Loop
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_17_RecoveryPathRetryLoop) {
  std::string operation = "findSimilar";
  std::string affected_key = "adapter_001";
  
  for (int attempt = 0; attempt < 3; ++attempt) {
    auto result = handler_.handleConcurrentModification(operation, affected_key);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.is_recoverable);
    EXPECT_EQ(result.recovery_action, "retry");
  }
}

// TEDGE-18: Fallback Path After Primary Failure
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_18_FallbackPathAfterPrimaryFailure) {
  std::string path1 = "/primary/location/graph.pb";
  std::string path2 = "/fallback/location/graph.json";
  
  auto result1 = handler_.handleExportSerializationFailure(path1, "io_error");
  auto result2 = handler_.handleExportSerializationFailure(path2, "fallback_attempt");
  
  EXPECT_FALSE(result1.success);
  EXPECT_FALSE(result2.success);
}

// TEDGE-19: Degradation Path - Partial Results
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_19_DegradationPathPartialResults) {
  size_t total_entries = 10000;
  size_t recovered_entries = 8000;  // 20% loss
  
  auto result = handler_.handlePartialGraphLossRecovery(total_entries, recovered_entries);
  
  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.is_recoverable);
}

// TEDGE-20: Fail-Closed Path - Unrecoverable Error
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_20_FailClosedPathUnrecoverableError) {
  size_t requested_bytes = 100ULL * 1024 * 1024 * 1024;  // 100 GB
  std::string operation = "fingerprint_compute";
  
  auto result = handler_.handleOutOfMemoryDuringComputation(requested_bytes, operation);
  
  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.is_recoverable);
  EXPECT_EQ(result.recovery_action, "fail-closed");
}

// TEDGE-21: Stale Fingerprint + Concurrent Modification
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_21_StaleFingerprintAndConcurrentModification) {
  // Compound failure scenario
  auto stale_result = handler_.handleStaleFingerprint("adapter_001", std::numeric_limits<float>::infinity());
  auto concurrent_result = handler_.handleConcurrentModification("findSimilar", "adapter_001");
  
  EXPECT_FALSE(stale_result.success);
  EXPECT_FALSE(concurrent_result.success);
}

// TEDGE-22: Self-Similarity + Kappa-Gate Combined
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_22_SelfSimilarityAndKappaGateCombined) {
  auto sim_result = handler_.handleSelfSimilarityFailure("adapter_002", 0.3f);
  auto kappa_result = handler_.handleKappaGateViolation(512, 1.5, 5.0);
  
  EXPECT_FALSE(sim_result.success);
  EXPECT_FALSE(kappa_result.success);
}

// TEDGE-23: Adapter Communication + Bridge Routing Failure
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_23_AdapterCommunicationAndBridgeRoutingFailure) {
  auto comm_result = handler_.handleAdapterCommunicationFailure("adapter_003", 3000);
  auto routing_result = handler_.handleBridgeRoutingFailure(1.5, 500);
  
  EXPECT_FALSE(comm_result.success);
  EXPECT_FALSE(routing_result.success);
}

// TEDGE-24: Large Serialization + Partial Loss
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_24_LargeSerializationAndPartialLoss) {
  auto export_result = handler_.handleExportSerializationFailure("/large/graph", "write_timeout");
  auto loss_result = handler_.handlePartialGraphLossRecovery(50000, 40000);
  
  EXPECT_FALSE(export_result.success);
  EXPECT_FALSE(loss_result.success);
}

// TEDGE-25: Invalid Adapter + Out-of-Bounds (Cascading Failures)
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_25_InvalidAdapterAndOutOfBoundsCascading) {
  auto adapter_result = handler_.handleInvalidAdapterReference("bad_key", "findSimilar");
  auto oob_result = handler_.handleOutOfBoundsIndex(9999, 100);
  
  EXPECT_FALSE(adapter_result.success);
  EXPECT_FALSE(oob_result.success);
  EXPECT_FALSE(adapter_result.is_recoverable);
  EXPECT_TRUE(oob_result.is_recoverable);
}

// TEDGE-26: NaN/Inf Detection in Fingerprint
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_26_NaNInfDetectionInFingerprint) {
  auto nan_result = handler_.handleStaleFingerprint("nan_adapter", std::numeric_limits<float>::quiet_NaN());
  auto inf_result = handler_.handleStaleFingerprint("inf_adapter", std::numeric_limits<float>::infinity());
  
  EXPECT_FALSE(nan_result.success);
  EXPECT_FALSE(inf_result.success);
}

// TEDGE-27: Zero Recovery Entry Edge Case
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_27_ZeroRecoveryEntryEdgeCase) {
  // Edge case: no entries recovered at all
  auto result = handler_.handlePartialGraphLossRecovery(1000, 0);
  
  EXPECT_FALSE(result.success);
}

// TEDGE-28: Multiple Memory Allocation Failures
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_28_MultipleMemoryAllocationFailures) {
  std::vector<size_t> allocations = {
      50ULL * 1024 * 1024 * 1024,  // 50 GB
      32ULL * 1024 * 1024 * 1024,  // 32 GB
      16ULL * 1024 * 1024 * 1024,  // 16 GB
  };
  
  for (size_t req : allocations) {
    auto result = handler_.handleOutOfMemoryDuringComputation(req, "compute");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.is_recoverable);
  }
}

// TEDGE-29: Concurrent Modification Stress Test
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_29_ConcurrentModificationStressTest) {
  std::vector<std::string> affected_keys = {
      "adapter_001", "adapter_002", "adapter_003",
      "adapter_004", "adapter_005"
  };
  
  for (const auto& key : affected_keys) {
    auto result = handler_.handleConcurrentModification("findSimilar", key);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.is_recoverable);
  }
}

// TEDGE-30: Full Recovery Path Simulation
TEST_F(TensorEdgeCaseHandlerFixture, TEDGE_30_FullRecoveryPathSimulation) {
  // Simulate: failure → identify recovery action → retry
  auto failure_result = handler_.handleAdapterCommunicationFailure("flaky_adapter", 5000);
  
  EXPECT_FALSE(failure_result.success);
  EXPECT_TRUE(failure_result.is_recoverable);
  EXPECT_EQ(failure_result.recovery_action, "retry");
  
  // Caller would retry the operation here
  // For test purposes, we just verify the recovery action was correct
}

// ============================================================================
// STATISTICS & DIAGNOSTICS VALIDATION
// ============================================================================

TEST_F(TensorEdgeCaseHandlerFixture, ValidateStatisticsTracking) {
  handler_.handleInvalidAdapterReference("test_key", "test_op");
  handler_.handleOutOfBoundsIndex(100, 10);
  handler_.handleStaleFingerprint("test_adapter", 1.5f);
  
  auto stats = handler_.getStats();
  EXPECT_GT(stats.invalid_adapter_refs + stats.out_of_bounds_accesses + stats.stale_fingerprints, 0u);
}

TEST_F(TensorEdgeCaseHandlerFixture, ValidateDiagnosticMessageGeneration) {
  auto result = handler_.handleOutOfBoundsIndex(50, 10);
  
  EXPECT_FALSE(result.error_message.empty());
}

TEST_F(TensorEdgeCaseHandlerFixture, ValidateErrorCodeMapping) {
  // Verify error codes are in tensor module range (9510-9589)
  auto result1 = handler_.handleInvalidAdapterReference("key", "op");
  auto result2 = handler_.handleOutOfBoundsIndex(100, 10);
  auto result3 = handler_.handleConcurrentMemoryExhaustion(1000000, 2000000, 4);
  
  EXPECT_GE(result1.error_code, 9510);
  EXPECT_LE(result1.error_code, 9589);
  EXPECT_GE(result2.error_code, 9510);
  EXPECT_LE(result2.error_code, 9589);
  EXPECT_GE(result3.error_code, 9510);
  EXPECT_LE(result3.error_code, 9589);
}

}  // namespace themis::tensor::test
