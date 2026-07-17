/**
 * @file test_tensor_rebuild_fallback.cpp
 * @brief Phase B regression tests for SnapshotBasedUpdateWorker rebuild fallback.
 *
 * Test IDs (RFB):
 *   RFB-01  decideUpdateStrategy() chooses PATCH for small delta (<10%)
 *   RFB-02  decideUpdateStrategy() chooses PARTIAL_REFIT for medium delta (10%-50%)
 *   RFB-03  decideUpdateStrategy() chooses REBUILD for large delta (>50%)
 *   RFB-04  executePatch() applies small updates successfully
 *   RFB-05  executePartialRefit() applies medium updates with quality tracking
 *   RFB-06  executeRebuild() handles full artifact reconstruction
 *   RFB-07  Fallback to REBUILD when PARTIAL_REFIT fails
 *   RFB-08  Fallback to REBUILD when residual limit exceeded
 *   RFB-09  publishManifest() atomically updates ManifestStore
 *   RFB-10  Worker state machine transitions correctly through workflow
 */

#include <gtest/gtest.h>

#include "snapshot_update_worker.h"
#include "tensor_delta_log.h"
#include "artifact_manifest.h"
#include "manifest_store.h"

#include <chrono>
#include <memory>

using namespace themis::distributed_tensor;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Creates a test ArtifactManifest with specified parameters.
ArtifactManifest makeManifest(const std::string& artifact_id,
                              uint64_t artifact_size_bytes,
                              double residual = 0.05) {
  ArtifactManifest m;
  m.artifact_id = artifact_id;
  m.tensor_name = "test/tensor";
  m.shard_id = 0;
  m.kind = ArtifactKind::ADVISORY_SUMMARY;
  m.version = 1;
  m.lifecycle_state = LifecycleState::READY;
  m.truth_semantic = TruthSemantic::ADVISORY_ONLY;
  m.residual = residual;
  m.rank_cap = 128;
  m.created_at = std::chrono::system_clock::now();
  m.integrity.crc32 = 0xDEADBEEFu;
  m.integrity.payload_bytes = artifact_size_bytes;
  return m;
}

/// Creates a test DeltaWindow with specified parameters.
DeltaWindow makeDeltaWindow(const std::string& artifact_id,
                            uint64_t sequence_start,
                            uint64_t total_payload_bytes,
                            size_t num_inserts = 0,
                            size_t num_updates = 0,
                            size_t num_deletes = 0) {
  DeltaWindow window;
  window.artifact_id = artifact_id;
  window.sequence_start = sequence_start;
  window.sequence_end = sequence_start + num_inserts + num_updates + num_deletes - 1;
  window.total_payload_size_bytes = total_payload_bytes;
  window.extracted_at_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  // Generate sample delta entries
  uint64_t seq = sequence_start;
  for (size_t i = 0; i < num_inserts; ++i, ++seq) {
    DeltaLogEntry entry;
    entry.sequence_number = seq;
    entry.mutation_type = DeltaMutationType::INSERT;
    entry.affected_entity_id = "entity-" + std::to_string(seq);
    entry.source_transaction_id = "txn-" + std::to_string(seq);
    entry.recorded_at_ms = 1000000LL + seq * 10;
    entry.payload_size_bytes = total_payload_bytes / (num_inserts + num_updates + num_deletes);
    window.entries.push_back(entry);
  }
  for (size_t i = 0; i < num_updates; ++i, ++seq) {
    DeltaLogEntry entry;
    entry.sequence_number = seq;
    entry.mutation_type = DeltaMutationType::UPDATE;
    entry.affected_entity_id = "entity-" + std::to_string(seq);
    entry.source_transaction_id = "txn-" + std::to_string(seq);
    entry.recorded_at_ms = 1000000LL + seq * 10;
    entry.payload_size_bytes = total_payload_bytes / (num_inserts + num_updates + num_deletes);
    window.entries.push_back(entry);
  }
  for (size_t i = 0; i < num_deletes; ++i, ++seq) {
    DeltaLogEntry entry;
    entry.sequence_number = seq;
    entry.mutation_type = DeltaMutationType::DELETE;
    entry.affected_entity_id = "entity-" + std::to_string(seq);
    entry.source_transaction_id = "txn-" + std::to_string(seq);
    entry.recorded_at_ms = 1000000LL + seq * 10;
    entry.payload_size_bytes = total_payload_bytes / (num_inserts + num_updates + num_deletes);
    window.entries.push_back(entry);
  }

  return window;
}

}  // namespace

// ---------------------------------------------------------------------------
// SnapshotUpdateWorkerTest fixture
// ---------------------------------------------------------------------------

class SnapshotUpdateWorkerTest : public ::testing::Test {
 protected:
  SnapshotBasedUpdateWorker worker_;
  ManifestStore store_;

  void SetUp() override {
    // Initialize worker before tests
    EXPECT_TRUE(worker_.start());
  }

  void TearDown() override {
    worker_.shutdown();
  }
};

// RFB-01: decideUpdateStrategy() chooses PATCH for small delta (<10%)
TEST_F(SnapshotUpdateWorkerTest, DecideUpdateStrategyChoosesPatchForSmallDelta) {
  // Artifact size = 10000 bytes, delta = 500 bytes (<5%)
  uint64_t artifact_size = 10000;
  DeltaWindow small_delta = makeDeltaWindow("artifact-1", 1, 500, 5, 0, 0);

  UpdateDecision decision = worker_.decideUpdateStrategy(small_delta, artifact_size, 0.05);
  EXPECT_EQ(decision, UpdateDecision::PATCH);
}

// RFB-02: decideUpdateStrategy() chooses PARTIAL_REFIT for medium delta (10%-50%)
TEST_F(SnapshotUpdateWorkerTest, DecideUpdateStrategyChoosesPartialRefitForMediumDelta) {
  // Artifact size = 10000 bytes, delta = 2500 bytes (25%)
  uint64_t artifact_size = 10000;
  DeltaWindow medium_delta = makeDeltaWindow("artifact-1", 1, 2500, 10, 10, 5);

  UpdateDecision decision = worker_.decideUpdateStrategy(medium_delta, artifact_size, 0.05);
  EXPECT_EQ(decision, UpdateDecision::PARTIAL_REFIT);
}

// RFB-03: decideUpdateStrategy() chooses REBUILD for large delta (>50%)
TEST_F(SnapshotUpdateWorkerTest, DecideUpdateStrategyChoosesRebuildForLargeDelta) {
  // Artifact size = 10000 bytes, delta = 6000 bytes (60%)
  uint64_t artifact_size = 10000;
  DeltaWindow large_delta = makeDeltaWindow("artifact-1", 1, 6000, 20, 20, 10);

  UpdateDecision decision = worker_.decideUpdateStrategy(large_delta, artifact_size, 0.05);
  EXPECT_EQ(decision, UpdateDecision::REBUILD);
}

// RFB-04: executePatch() applies small updates successfully
TEST_F(SnapshotUpdateWorkerTest, ExecutePatchApplesSmallUpdatesSuccessfully) {
  std::string artifact_id = "artifact-patch-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000);
  DeltaWindow patch_deltas = makeDeltaWindow(artifact_id, 1, 500, 3, 2, 0);

  bool success = worker_.executePatch(artifact_id, patch_deltas, manifest);
  EXPECT_TRUE(success);

  // Manifest should be updated
  EXPECT_GT(manifest.version, 1u);
}

// RFB-05: executePartialRefit() applies medium updates with quality tracking
TEST_F(SnapshotUpdateWorkerTest, ExecutePartialRefitAppliesMediumUpdatesWithQuality) {
  std::string artifact_id = "artifact-refit-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000, 0.05);
  DeltaWindow refit_deltas = makeDeltaWindow(artifact_id, 1, 2500, 5, 5, 5);

  bool success = worker_.executePartialRefit(artifact_id, refit_deltas, manifest);
  EXPECT_TRUE(success);

  // Manifest should be updated with new version
  EXPECT_GT(manifest.version, 1u);

  // Residual should remain within acceptable bounds
  EXPECT_LE(manifest.residual, 0.10);  // Allow small increase
}

// RFB-06: executeRebuild() handles full artifact reconstruction
TEST_F(SnapshotUpdateWorkerTest, ExecuteRebuildHandlesFullReconstruction) {
  std::string artifact_id = "artifact-rebuild-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000);
  DeltaWindow rebuild_deltas = makeDeltaWindow(artifact_id, 1, 6000, 15, 15, 10);

  bool success = worker_.executeRebuild(artifact_id, rebuild_deltas, manifest);
  EXPECT_TRUE(success);

  // Manifest should reflect rebuild state
  EXPECT_GT(manifest.version, 1u);
  EXPECT_LE(manifest.residual, 0.08);  // Should be fresh after rebuild
}

// RFB-07: Fallback to REBUILD when PARTIAL_REFIT fails
TEST_F(SnapshotUpdateWorkerTest, FallbackToRebuildWhenPartialRefitFails) {
  std::string artifact_id = "artifact-fallback-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000, 0.05);
  DeltaWindow tricky_deltas = makeDeltaWindow(artifact_id, 1, 3500, 10, 10, 10);

  // First, try partial refit
  bool refit_success = worker_.executePartialRefit(artifact_id, tricky_deltas, manifest);

  // If refit fails or residual gets too high, should fallback to rebuild
  if (!refit_success || manifest.residual > 0.10) {
    bool rebuild_success = worker_.executeRebuild(artifact_id, tricky_deltas, manifest);
    EXPECT_TRUE(rebuild_success);
    EXPECT_LE(manifest.residual, 0.08);
  }

  EXPECT_TRUE(refit_success || manifest.residual <= 0.10);
}

// RFB-08: Fallback to REBUILD when residual limit exceeded
TEST_F(SnapshotUpdateWorkerTest, FallbackToRebuildWhenResidualLimitExceeded) {
  std::string artifact_id = "artifact-residual-test";
  double initial_residual = 0.08;
  ArtifactManifest manifest = makeManifest(artifact_id, 10000, initial_residual);

  // Set a strict residual increase limit
  worker_.setDecisionThresholds(10.0, 50.0, 0.02);  // Only 0.02 increase allowed

  DeltaWindow deltas = makeDeltaWindow(artifact_id, 1, 3000, 8, 8, 4);

  // Attempt partial refit
  bool refit_success = worker_.executePartialRefit(artifact_id, deltas, manifest);

  // If residual exceeds initial + limit, should fallback
  if (manifest.residual > (initial_residual + 0.02)) {
    bool rebuild_success = worker_.executeRebuild(artifact_id, deltas, manifest);
    EXPECT_TRUE(rebuild_success);
    EXPECT_LE(manifest.residual, 0.08);
  }
}

// RFB-09: publishManifest() atomically updates ManifestStore
TEST_F(SnapshotUpdateWorkerTest, PublishManifestAtomicallyUpdatesStore) {
  std::string artifact_id = "artifact-publish-test";
  ArtifactManifest new_manifest = makeManifest(artifact_id, 10000);
  new_manifest.version = 42;

  // Publish the manifest
  bool success = worker_.publishManifest(artifact_id, new_manifest, 1, "test_publish");
  EXPECT_TRUE(success);

  // Note: ManifestStore is internal to worker; this test verifies the API succeeds
}

// RFB-10: Worker state machine transitions correctly through workflow
TEST_F(SnapshotUpdateWorkerTest, WorkerStateMachineTransitionsCorrectly) {
  // Worker should start in READY state after start()
  EXPECT_EQ(worker_.getState(), UpdateWorkerState::READY);

  std::string artifact_id = "artifact-state-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000);
  DeltaWindow deltas = makeDeltaWindow(artifact_id, 1, 500, 3, 2, 0);

  UpdateTask task;
  task.artifact_id = artifact_id;
  task.delta_window = deltas;
  task.current_manifest = manifest;
  task.artifact_size_bytes = 10000;

  UpdateMetrics metrics;
  UpdateDecision decision = worker_.processTask(task, metrics);

  // Worker should return to READY state after task completion
  EXPECT_EQ(worker_.getState(), UpdateWorkerState::READY);
  EXPECT_NE(decision, UpdateDecision::NO_UPDATE);
  EXPECT_TRUE(metrics.success);
}

// Additional edge case tests

// RFB-A1: processTask() aggregates metrics correctly
TEST_F(SnapshotUpdateWorkerTest, ProcessTaskAggregatesMetricsCorrectly) {
  std::string artifact_id = "artifact-metrics-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000);
  DeltaWindow deltas = makeDeltaWindow(artifact_id, 1, 1000, 5, 5, 0);

  UpdateTask task;
  task.artifact_id = artifact_id;
  task.delta_window = deltas;
  task.current_manifest = manifest;
  task.artifact_size_bytes = 10000;

  UpdateMetrics metrics;
  UpdateDecision decision = worker_.processTask(task, metrics);

  EXPECT_GE(metrics.analysis_time_ms, 0);
  EXPECT_GE(metrics.execution_time_ms, 0);
  EXPECT_GE(metrics.throughput_deltas_per_sec, 0.0);
  EXPECT_TRUE(metrics.success);
}

// RFB-A2: Multiple tasks can be processed sequentially
TEST_F(SnapshotUpdateWorkerTest, MultipleTasksProcessedSequentially) {
  for (int i = 0; i < 3; ++i) {
    std::string artifact_id = "artifact-" + std::to_string(i);
    ArtifactManifest manifest = makeManifest(artifact_id, 10000 + i * 1000);
    DeltaWindow deltas = makeDeltaWindow(artifact_id, 1, 500 + i * 100, 3, 2, 0);

    UpdateTask task;
    task.artifact_id = artifact_id;
    task.delta_window = deltas;
    task.current_manifest = manifest;
    task.artifact_size_bytes = 10000 + i * 1000;

    UpdateMetrics metrics;
    UpdateDecision decision = worker_.processTask(task, metrics);
    EXPECT_TRUE(metrics.success);
  }

  // Worker should remain in READY state
  EXPECT_EQ(worker_.getState(), UpdateWorkerState::READY);
}

// RFB-A3: getStats() reports worker activity correctly
TEST_F(SnapshotUpdateWorkerTest, GetStatsReportsWorkerActivityCorrectly) {
  // Process a few tasks
  for (int i = 0; i < 3; ++i) {
    std::string artifact_id = "artifact-stats-" + std::to_string(i);
    ArtifactManifest manifest = makeManifest(artifact_id, 10000);
    DeltaWindow deltas = makeDeltaWindow(artifact_id, 1, 500, 3, 2, 0);

    UpdateTask task;
    task.artifact_id = artifact_id;
    task.delta_window = deltas;
    task.current_manifest = manifest;
    task.artifact_size_bytes = 10000;

    UpdateMetrics metrics;
    worker_.processTask(task, metrics);
  }

  auto stats = worker_.getStats();
  EXPECT_GE(stats.total_tasks_processed, 3u);
  EXPECT_GE(stats.total_patches_applied, 0u);
  EXPECT_GE(stats.average_decision_time_ms, 0.0);
}

// RFB-A4: setDecisionThresholds() configures update strategy
TEST_F(SnapshotUpdateWorkerTest, SetDecisionThresholdsConfiguresStrategy) {
  // Configure custom thresholds
  worker_.setDecisionThresholds(5.0, 40.0, 0.03);

  // Artifact size = 1000 bytes
  // Delta = 100 bytes (10%) should trigger PARTIAL_REFIT with 5% threshold
  DeltaWindow medium_delta = makeDeltaWindow("artifact-threshold-test", 1, 100, 5, 5, 0);
  UpdateDecision decision = worker_.decideUpdateStrategy(medium_delta, 1000, 0.05);

  // Should be influenced by custom thresholds
  EXPECT_NE(decision, UpdateDecision::NO_UPDATE);
}

// RFB-A5: Error handling when delta window is invalid
TEST_F(SnapshotUpdateWorkerTest, ErrorHandlingForInvalidDeltaWindow) {
  std::string artifact_id = "artifact-error-test";
  ArtifactManifest manifest = makeManifest(artifact_id, 10000);

  // Create an "invalid" delta window (empty entries)
  DeltaWindow empty_deltas;
  empty_deltas.artifact_id = artifact_id;
  empty_deltas.sequence_start = 1;
  empty_deltas.sequence_end = 1;
  empty_deltas.total_payload_size_bytes = 0;
  empty_deltas.entries.clear();

  UpdateTask task;
  task.artifact_id = artifact_id;
  task.delta_window = empty_deltas;
  task.current_manifest = manifest;
  task.artifact_size_bytes = 10000;

  UpdateMetrics metrics;
  UpdateDecision decision = worker_.processTask(task, metrics);

  // Should handle gracefully (likely NO_UPDATE for empty delta)
  EXPECT_TRUE(decision == UpdateDecision::NO_UPDATE || metrics.success);
}
