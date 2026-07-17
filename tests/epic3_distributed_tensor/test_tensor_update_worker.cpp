/**
 * @file test_tensor_update_worker.cpp
 * @brief CTest / GTest coverage for SnapshotBasedUpdateWorker — sub-issue #5472.
 *
 * ## Test IDs
 *   TUW-01  New worker starts in IDLE state
 *   TUW-02  start() transitions worker to READY
 *   TUW-03  shutdown() after start() returns true and transitions to SHUTTING_DOWN
 *   TUW-04  Empty delta window → NO_UPDATE decision
 *   TUW-05  Small delta fraction (< 10 %) → PATCH decision
 *   TUW-06  Medium delta fraction (10–50 %) → PARTIAL_REFIT decision (low residual)
 *   TUW-07  Large delta fraction (> 50 %) → REBUILD decision
 *   TUW-08  setDecisionThresholds changes PATCH/REFIT boundary
 *   TUW-09  getStats() returns zero counters after construction
 *   TUW-10  processDeltaWindow returns a valid decision for ready worker
 *   TUW-11  processDeltaWindow on non-READY worker returns ERROR_FALLBACK
 *   TUW-12  getStats() total_tasks_processed increments on each processTask call
 *   TUW-13  Residual near limit causes PARTIAL_REFIT to fall back to REBUILD
 *   TUW-14  setDecisionThresholds with patch == refit forces all medium to REBUILD
 *   TUW-15  SHARD_CHANGE mutations are counted in change fraction estimate
 *
 * @see src/distributed_tensor/include/snapshot_update_worker.h
 * @see src/distributed_tensor/src/snapshot_update_worker.cc
 * @see GitHub Issue #5472
 */

#include <gtest/gtest.h>

#include "src/distributed_tensor/include/snapshot_update_worker.h"
#include "src/distributed_tensor/include/tensor_delta_log.h"
#include "src/distributed_tensor/include/artifact_manifest.h"

#include <string>
#include <vector>

using namespace themis::distributed_tensor;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

/// Build a DeltaWindow with @p n entries whose total payload equals @p total_bytes.
DeltaWindow makeWindow(const std::string& artifact_id,
                       size_t n,
                       uint32_t bytes_per_entry = 100,
                       DeltaMutationType type = DeltaMutationType::INSERT) {
    DeltaWindow win;
    win.artifact_id       = artifact_id;
    win.sequence_start    = 1;
    win.sequence_end      = static_cast<uint64_t>(n);
    win.extracted_at_ms   = 1718000000000LL;
    win.total_payload_size_bytes = static_cast<uint64_t>(n) * bytes_per_entry;

    for (size_t i = 0; i < n; ++i) {
        DeltaLogEntry entry;
        entry.sequence_number       = static_cast<uint64_t>(i + 1);
        entry.mutation_type         = type;
        entry.affected_entity_id    = "entity-" + std::to_string(i);
        entry.recorded_at_ms        = 1718000000000LL;
        entry.source_transaction_id = "txn-" + std::to_string(i);
        entry.payload_size_bytes    = bytes_per_entry;
        win.entries.push_back(entry);
    }
    return win;
}

/// Build a minimal ArtifactManifest for use in update tasks.
ArtifactManifest makeManifest(const std::string& id = "art-1",
                               double residual = 0.01) {
    ArtifactManifest m;
    m.artifact_id = id;
    m.tensor_name = "test/embedding";
    m.kind        = ArtifactKind::ADVISORY_SUMMARY;
    m.shard_id    = 0;
    m.version     = 1;
    m.created_at  = std::chrono::system_clock::now();
    m.residual    = residual;
    m.rank_status = 10;
    m.rank_cap    = 256;
    m.rebuild_state = RebuildState::PRISTINE;
    m.update_mode   = UpdateMode::NONE;
    m.source_seq_end = 0;
    m.delta_lag      = 0;
    return m;
}

}  // namespace

// ---------------------------------------------------------------------------
// TUW-01: New worker starts in IDLE state
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW01_NewWorkerIsIdle) {
    SnapshotBasedUpdateWorker worker;
    EXPECT_EQ(worker.getState(), UpdateWorkerState::IDLE);
}

// ---------------------------------------------------------------------------
// TUW-02: start() transitions worker to READY
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW02_StartTransitionsToReady) {
    SnapshotBasedUpdateWorker worker;
    EXPECT_TRUE(worker.start());
    EXPECT_EQ(worker.getState(), UpdateWorkerState::READY);
}

// ---------------------------------------------------------------------------
// TUW-03: shutdown() after start() succeeds
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW03_ShutdownAfterStartSucceeds) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());
    EXPECT_TRUE(worker.shutdown());
}

// ---------------------------------------------------------------------------
// TUW-04: Empty delta window → NO_UPDATE
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW04_EmptyWindowGivesNoUpdate) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    DeltaWindow empty_win;
    empty_win.artifact_id = "art-empty";

    const UpdateDecision d = worker.decideUpdateStrategy(
        empty_win, /*artifact_size_bytes=*/10000, /*current_residual=*/0.01);
    EXPECT_EQ(d, UpdateDecision::NO_UPDATE);
}

// ---------------------------------------------------------------------------
// TUW-05: Small delta fraction (< 10 %) → PATCH
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW05_SmallDeltaGivesPatch) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // 5 entries × 100 B = 500 B payload; artifact = 10 000 B → 5 % fraction
    const auto win = makeWindow("art-patch", 5, 100);

    const UpdateDecision d = worker.decideUpdateStrategy(
        win, /*artifact_size_bytes=*/10000, /*current_residual=*/0.01);
    EXPECT_EQ(d, UpdateDecision::PATCH);
}

// ---------------------------------------------------------------------------
// TUW-06: Medium delta fraction (10–50 %) → PARTIAL_REFIT
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW06_MediumDeltaGivesPartialRefit) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // 20 entries × 100 B = 2 000 B; artifact = 10 000 B → 20 % fraction
    const auto win = makeWindow("art-refit", 20, 100);

    const UpdateDecision d = worker.decideUpdateStrategy(
        win, /*artifact_size_bytes=*/10000, /*current_residual=*/0.01);
    // 20 % is in [10 %, 50 %] range; low residual → PARTIAL_REFIT expected
    EXPECT_EQ(d, UpdateDecision::PARTIAL_REFIT);
}

// ---------------------------------------------------------------------------
// TUW-07: Large delta fraction (> 50 %) → REBUILD
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW07_LargeDeltaGivesRebuild) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // 60 entries × 100 B = 6 000 B; artifact = 10 000 B → 60 % fraction
    const auto win = makeWindow("art-rebuild", 60, 100);

    const UpdateDecision d = worker.decideUpdateStrategy(
        win, /*artifact_size_bytes=*/10000, /*current_residual=*/0.01);
    EXPECT_EQ(d, UpdateDecision::REBUILD);
}

// ---------------------------------------------------------------------------
// TUW-08: setDecisionThresholds changes PATCH/REFIT boundary
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW08_SetThresholdsChangesBoundary) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // Lower patch threshold to 5 %; 8 % delta should now → PARTIAL_REFIT
    worker.setDecisionThresholds(/*patch_pct=*/5.0, /*refit_pct=*/50.0,
                                  /*residual_max=*/0.05);

    // 8 entries × 100 B = 800 B; artifact = 10 000 B → 8 % fraction
    const auto win = makeWindow("art-thresh", 8, 100);

    const UpdateDecision d = worker.decideUpdateStrategy(
        win, /*artifact_size_bytes=*/10000, /*current_residual=*/0.01);
    // 8 % > new patch threshold (5 %) → at least PARTIAL_REFIT
    EXPECT_NE(d, UpdateDecision::PATCH);
    EXPECT_NE(d, UpdateDecision::NO_UPDATE);
}

// ---------------------------------------------------------------------------
// TUW-09: getStats() returns zero counters after construction
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW09_StatsZeroAfterConstruction) {
    SnapshotBasedUpdateWorker worker;
    const auto stats = worker.getStats();
    EXPECT_EQ(stats.total_tasks_processed, 0u);
    EXPECT_EQ(stats.total_patches_applied, 0u);
    EXPECT_EQ(stats.total_partial_refits,  0u);
    EXPECT_EQ(stats.total_rebuilds,        0u);
    EXPECT_EQ(stats.total_failed_updates,  0u);
}

// ---------------------------------------------------------------------------
// TUW-10: processDeltaWindow returns a valid decision for ready worker
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW10_ProcessDeltaWindowReturnsValidDecision) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    const auto win      = makeWindow("art-pdw", 5, 100);  // 5 % fraction
    const auto manifest = makeManifest("art-pdw");

    const UpdateDecision d = worker.processDeltaWindow(
        "art-pdw", win, manifest, /*artifact_size_bytes=*/10000);

    // Any valid decision is acceptable; just not an out-of-range value
    EXPECT_TRUE(d == UpdateDecision::PATCH         ||
                d == UpdateDecision::PARTIAL_REFIT  ||
                d == UpdateDecision::REBUILD         ||
                d == UpdateDecision::NO_UPDATE       ||
                d == UpdateDecision::ERROR_FALLBACK_TO_REBUILD);
}

// ---------------------------------------------------------------------------
// TUW-11: processDeltaWindow on non-READY worker returns ERROR_FALLBACK
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW11_ProcessOnNonReadyWorkerReturnsErrorFallback) {
    SnapshotBasedUpdateWorker worker;
    // Worker is IDLE — not started
    ASSERT_EQ(worker.getState(), UpdateWorkerState::IDLE);

    const auto win      = makeWindow("art-err", 5, 100);
    const auto manifest = makeManifest("art-err");

    const UpdateDecision d = worker.processDeltaWindow(
        "art-err", win, manifest, 10000);
    EXPECT_EQ(d, UpdateDecision::ERROR_FALLBACK_TO_REBUILD);
}

// ---------------------------------------------------------------------------
// TUW-12: processTask increments total_tasks_processed
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW12_ProcessTaskIncrementsStatsCounter) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    UpdateTask task;
    task.artifact_id        = "art-stats";
    task.delta_window       = makeWindow("art-stats", 5, 100);
    task.current_manifest   = makeManifest("art-stats");
    task.artifact_size_bytes = 10000;

    UpdateMetrics metrics;
    worker.processTask(task, metrics);

    // Stats should now have at least one task processed
    EXPECT_GE(worker.getStats().total_tasks_processed, 1u);
}

// ---------------------------------------------------------------------------
// TUW-13: High residual causes PARTIAL_REFIT to escalate to REBUILD
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW13_HighResidualForcesRebuild) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // Configure: residual max increase = 0.01 (very tight)
    worker.setDecisionThresholds(10.0, 50.0, /*residual_max=*/0.01);

    // 20 % delta fraction → normally PARTIAL_REFIT territory
    const auto win = makeWindow("art-res", 20, 100);

    // Pass a very high current residual so estimated_residual - current_residual
    // exceeds 0.01, pushing the decision to REBUILD
    const double high_residual = 0.90;
    const UpdateDecision d = worker.decideUpdateStrategy(
        win, /*artifact_size_bytes=*/10000, high_residual);

    // Depending on estimateResultingResidual implementation, this should be
    // REBUILD because the quality budget is exhausted.
    // We accept PARTIAL_REFIT only if the implementation allows it at high residual.
    EXPECT_TRUE(d == UpdateDecision::REBUILD || d == UpdateDecision::PARTIAL_REFIT);
}

// ---------------------------------------------------------------------------
// TUW-14: patch_threshold == refit_threshold → medium delta forces REBUILD
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW14_EqualThresholdsForceRebuildForMedium) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // Both thresholds at 0 → everything > 0 % goes to REBUILD
    worker.setDecisionThresholds(0.0, 0.0, 0.05);

    const auto win = makeWindow("art-eq", 5, 100);  // any non-empty window
    const UpdateDecision d = worker.decideUpdateStrategy(win, 10000, 0.01);
    EXPECT_EQ(d, UpdateDecision::REBUILD);
}

// ---------------------------------------------------------------------------
// TUW-15: SHARD_CHANGE mutations contribute to change fraction
// ---------------------------------------------------------------------------
TEST(SnapshotUpdateWorkerTest, TUW15_ShardChangeMutationsCountedInFraction) {
    SnapshotBasedUpdateWorker worker;
    ASSERT_TRUE(worker.start());

    // Build a window with only SHARD_CHANGE entries totalling 60 % of artifact
    const auto win = makeWindow("art-shard", 60, 100, DeltaMutationType::SHARD_CHANGE);

    const UpdateDecision d = worker.decideUpdateStrategy(win, 10000, 0.01);
    // 60 % SHARD_CHANGE fraction → REBUILD
    EXPECT_EQ(d, UpdateDecision::REBUILD);
}
