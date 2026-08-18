// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
///
/// @file bench_tensor_partial_refit.cc
/// @brief Phase B performance benchmark for tensor partial refit operations
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-08-17
///
/// Performance targets:
/// - Patch path: p99 <= 5ms for small deltas (<10%)
/// - Partial refit: p99 <= 30ms for medium deltas (10-50%)
/// - Rebuild fallback: p99 <= 100ms
///
/// This benchmark validates bounded-window update latencies under realistic workloads.

#include <benchmark/benchmark.h>
#include <cmath>
#include <random>
#include <vector>

#include "src/distributed_tensor/include/snapshot_update_worker.h"
#include "src/distributed_tensor/include/tensor_delta_log.h"
#include "src/distributed_tensor/include/artifact_manifest.h"

namespace {

using namespace themis::distributed_tensor;

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr uint64_t kArtifactSizeBytes = 1024 * 1024;  // 1 MB
constexpr uint32_t kDeltaBytesPerEntry = 128;

/// Creates a realistic artifact manifest
ArtifactManifest makeManifest() {
  ArtifactManifest manifest;
  manifest.artifact_id = "artifact-bench";
  manifest.tensor_name = "bench/embedding";
  manifest.kind = ArtifactKind::ADVISORY_SUMMARY;
  manifest.version = 1;
  manifest.integrity.crc32 = 0xACED1234u ^ static_cast<uint32_t>(kCanonicalRngSeed);
  manifest.integrity.payload_bytes = kArtifactSizeBytes;
  manifest.rank_cap = 2048;
  manifest.rank_status = 64;
  manifest.residual = 0.05;
  manifest.lifecycle_state = LifecycleState::READY;
  manifest.truth_semantic = TruthSemantic::ADVISORY_ONLY;
  manifest.created_at = std::chrono::system_clock::now();
  return manifest;
}

/// Creates a delta window with specified entry count and mutation distribution
DeltaWindow makeWindow(int entry_count, double insert_ratio = 0.4, double update_ratio = 0.6) {
  DeltaWindow window;
  window.artifact_id = "artifact-bench";
  window.sequence_start = 1;
  window.sequence_end = static_cast<uint64_t>(entry_count);
  window.extracted_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  window.total_payload_size_bytes = 0;

  int inserts = static_cast<int>(entry_count * insert_ratio);
  int updates = entry_count - inserts;

  for (int i = 0; i < entry_count; ++i) {
    DeltaLogEntry entry;
    entry.sequence_number = static_cast<uint64_t>(i + 1);
    entry.mutation_type = (i < inserts) ? DeltaMutationType::INSERT
                                        : DeltaMutationType::UPDATE;
    entry.affected_entity_id = "node-" + std::to_string(i);
    entry.recorded_at_ms = window.extracted_at_ms - 1000;
    entry.source_transaction_id = "tx-" + std::to_string(i);
    entry.shard_hint = "bench";
    entry.payload_size_bytes = kDeltaBytesPerEntry;
    window.total_payload_size_bytes += entry.payload_size_bytes;
    window.entries.push_back(std::move(entry));
  }

  return window;
}

}  // namespace

// ===========================================================================
// Benchmark: Patch Path (Small Delta < 10%)
// ===========================================================================

static void BM_TensorPatchPath_SmallDelta(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();
  
  benchmark::DoNotOptimize(kCanonicalRngSeed);

  for (auto _ : state) {
    auto manifest = makeManifest();
    const auto window = makeWindow(static_cast<int>(state.range(0)), 0.3, 0.7);
    
    // Decision should be PATCH for small delta
    const auto decision = worker.decideUpdateStrategy(window, kArtifactSizeBytes, manifest.residual);
    if (decision == UpdateDecision::PATCH) {
      benchmark::DoNotOptimize(worker.executePatch("artifact-bench", window, manifest));
    } else {
      state.SkipWithError("Expected PATCH decision for small delta");
    }
    benchmark::DoNotOptimize(manifest);
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorPatchPath_SmallDelta)
    ->Arg(5)     // 640 bytes (0.06% of 1MB)
    ->Arg(10)    // 1.3 KB (0.13% of 1MB)
    ->Arg(20)    // 2.5 KB (0.25% of 1MB)
    ->Arg(50)    // 6.4 KB (0.62% of 1MB)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

// ===========================================================================
// Benchmark: Patch Path - Decision Logic Only
// ===========================================================================

static void BM_TensorPatchDecision_Fast(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();
  
  for (auto _ : state) {
    auto window = makeWindow(static_cast<int>(state.range(0)), 0.3, 0.7);
    
    benchmark::DoNotOptimize(worker.decideUpdateStrategy(
        window, kArtifactSizeBytes, 0.05));
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorPatchDecision_Fast)
    ->Arg(10)
    ->Arg(50)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

// ===========================================================================
// Benchmark: Partial Refit Path (Medium Delta 10-50%)
// ===========================================================================

static void BM_TensorPartialRefit_MediumDelta(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto manifest = makeManifest();
    manifest.rank_status = 50;
    manifest.rank_cap = 256;
    
    const auto window = makeWindow(static_cast<int>(state.range(0)), 0.4, 0.6);
    
    const auto decision = worker.decideUpdateStrategy(window, kArtifactSizeBytes, manifest.residual);
    if (decision == UpdateDecision::PARTIAL_REFIT) {
      benchmark::DoNotOptimize(worker.executePartialRefit(
          "artifact-bench", window, manifest));
    } else if (decision == UpdateDecision::REBUILD) {
      benchmark::DoNotOptimize(worker.executeRebuild(
          "artifact-bench", window, manifest));
    } else {
      state.SkipWithError("Expected PARTIAL_REFIT or REBUILD");
    }
    benchmark::DoNotOptimize(manifest);
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorPartialRefit_MediumDelta)
    ->Arg(32)    // ~4 KB (0.4% of 1MB) - should be PATCH
    ->Arg(100)   // ~12.8 KB (1.25% of 1MB) - should be PATCH
    ->Arg(320)   // ~41 KB (4% of 1MB) - should be PATCH
    ->Arg(800)   // ~102 KB (10% of 1MB) - boundary, could be PATCH or REFIT
    ->Arg(1600)  // ~205 KB (20% of 1MB) - should be PARTIAL_REFIT
    ->Arg(3200)  // ~410 KB (40% of 1MB) - should be PARTIAL_REFIT
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(500);

// ===========================================================================
// Benchmark: Rebuild Path (Large Delta > 50%)
// ===========================================================================

static void BM_TensorRebuild_LargeDelta(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto manifest = makeManifest();
    
    const auto window = makeWindow(static_cast<int>(state.range(0)), 0.45, 0.55);
    
    const auto decision = worker.decideUpdateStrategy(window, kArtifactSizeBytes, manifest.residual);
    if (decision == UpdateDecision::REBUILD) {
      benchmark::DoNotOptimize(worker.executeRebuild(
          "artifact-bench", window, manifest));
    } else if (decision == UpdateDecision::PARTIAL_REFIT) {
      benchmark::DoNotOptimize(worker.executePartialRefit(
          "artifact-bench", window, manifest));
    } else {
      state.SkipWithError("Expected REBUILD or PARTIAL_REFIT");
    }
    benchmark::DoNotOptimize(manifest);
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorRebuild_LargeDelta)
    ->Arg(2000)  // ~256 KB (25% of 1MB) - might be PARTIAL_REFIT
    ->Arg(4000)  // ~512 KB (50% of 1MB) - boundary
    ->Arg(6000)  // ~768 KB (75% of 1MB) - should be REBUILD
    ->Arg(8000)  // ~1 MB (100% of 1MB) - should be REBUILD
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(100);

// ===========================================================================
// Benchmark: Validation Overhead - Patch Applicability Check
// ===========================================================================

static void BM_TensorValidatePatch_Overhead(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto window = makeWindow(static_cast<int>(state.range(0)), 0.3, 0.7);
    
    benchmark::DoNotOptimize(worker.isValidForPatchingPublic(window, 3600000));
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorValidatePatch_Overhead)
    ->Arg(10)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(5000);

// ===========================================================================
// Benchmark: Instability Detection - Overhead
// ===========================================================================

static void BM_TensorDetectInstability_Overhead(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto window = makeWindow(static_cast<int>(state.range(0)), 0.5, 0.5);
    
    benchmark::DoNotOptimize(worker.detectInstabilityPublic(window, 0.05));
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorDetectInstability_Overhead)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(5000);

// ===========================================================================
// Benchmark: End-to-End Update Workflow
// ===========================================================================

static void BM_TensorUpdateE2E_SmallToLarge(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto manifest = makeManifest();
    manifest.rank_status = 30;
    manifest.rank_cap = 256;
    
    const auto window = makeWindow(static_cast<int>(state.range(0)), 0.4, 0.6);
    
    UpdateMetrics metrics;
    UpdateTask task;
    task.artifact_id = "artifact-bench";
    task.delta_window = window;
    task.current_manifest = manifest;
    task.artifact_size_bytes = kArtifactSizeBytes;
    
    benchmark::DoNotOptimize(worker.processTask(task, metrics));
    benchmark::DoNotOptimize(manifest);
    benchmark::DoNotOptimize(metrics);
  }

  state.SetItemsProcessed(state.iterations() * state.range(0));
  worker.shutdown();
}

BENCHMARK(BM_TensorUpdateE2E_SmallToLarge)
    ->Arg(20)    // Small delta -> PATCH
    ->Arg(400)   // Medium delta -> PARTIAL_REFIT
    ->Arg(4000)  // Large delta -> REBUILD
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(200);

// ===========================================================================
// Benchmark: State Machine Transitions
// ===========================================================================

static void BM_TensorStateTransitions_PatchPath(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto manifest = makeManifest();
    const auto window = makeWindow(5, 0.3, 0.7);
    
    // Measure just the state transition in executePatch
    benchmark::DoNotOptimize(worker.executePatch("artifact-bench", window, manifest));
    benchmark::DoNotOptimize(manifest);
  }

  state.SetItemsProcessed(state.iterations());
  worker.shutdown();
}

BENCHMARK(BM_TensorStateTransitions_PatchPath)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

static void BM_TensorStateTransitions_RefitPath(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto manifest = makeManifest();
    manifest.rank_status = 50;
    manifest.rank_cap = 256;
    const auto window = makeWindow(200, 0.4, 0.6);
    
    // Measure just the state transition in executePartialRefit
    benchmark::DoNotOptimize(worker.executePartialRefit("artifact-bench", window, manifest));
    benchmark::DoNotOptimize(manifest);
  }

  state.SetItemsProcessed(state.iterations());
  worker.shutdown();
}

BENCHMARK(BM_TensorStateTransitions_RefitPath)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

static void BM_TensorStateTransitions_RebuildPath(benchmark::State& state) {
  SnapshotBasedUpdateWorker worker;
  worker.start();

  for (auto _ : state) {
    auto manifest = makeManifest();
    const auto window = makeWindow(5000, 0.5, 0.5);
    
    // Measure just the state transition in executeRebuild
    benchmark::DoNotOptimize(worker.executeRebuild("artifact-bench", window, manifest));
    benchmark::DoNotOptimize(manifest);
  }

  state.SetItemsProcessed(state.iterations());
  worker.shutdown();
}

BENCHMARK(BM_TensorStateTransitions_RebuildPath)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

