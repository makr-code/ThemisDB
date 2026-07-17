// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "snapshot_update_worker.h"

namespace {

constexpr uint64_t kCanonicalRngSeed = 42;

themis::distributed_tensor::ArtifactManifest makeManifest() {
    using namespace themis::distributed_tensor;

    ArtifactManifest manifest;
    manifest.artifact_id = "artifact-bench";
    manifest.tensor_name = "bench/embedding";
    manifest.kind = ArtifactKind::ADVISORY_SUMMARY;
    manifest.version = 1;
    manifest.integrity.crc32 = 0xACED1234u ^ static_cast<uint32_t>(kCanonicalRngSeed);
    manifest.integrity.payload_bytes = 16 * 1024;
    manifest.rank_cap = 2048;
    manifest.rank_status = 64;
    manifest.residual = 0.05;
    return manifest;
}

themis::distributed_tensor::DeltaWindow makeWindow(int entry_count) {
    using namespace themis::distributed_tensor;

    DeltaWindow window;
    window.artifact_id = "artifact-bench";
    window.sequence_start = 1;
    window.sequence_end = static_cast<uint64_t>(entry_count);
    window.extracted_at_ms = 1;

    for (int i = 0; i < entry_count; ++i) {
        DeltaLogEntry entry;
        entry.sequence_number = static_cast<uint64_t>(i + 1);
        entry.mutation_type = (i % 3 == 0) ? DeltaMutationType::INSERT
                                           : DeltaMutationType::UPDATE;
        entry.affected_entity_id = "node-" + std::to_string(i);
        entry.recorded_at_ms = 1 + i;
        entry.source_transaction_id = "tx-" + std::to_string(i);
        entry.shard_hint = "bench";
        entry.payload_size_bytes = 128;
        window.total_payload_size_bytes += entry.payload_size_bytes;
        window.entries.push_back(std::move(entry));
    }

    return window;
}

void BM_TensorPartialRefit(benchmark::State& state) {
    themis::distributed_tensor::SnapshotBasedUpdateWorker worker;
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        auto manifest = makeManifest();
        const auto window = makeWindow(static_cast<int>(state.range(0)));
        const auto decision = worker.decideUpdateStrategy(window, 16 * 1024, manifest.residual);
        if (decision == themis::distributed_tensor::UpdateDecision::PARTIAL_REFIT) {
            benchmark::DoNotOptimize(worker.executePartialRefit(
                "artifact-bench", window, manifest));
        } else {
            benchmark::DoNotOptimize(worker.executeRebuild(
                "artifact-bench", window, manifest));
        }
        benchmark::DoNotOptimize(manifest);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

BENCHMARK(BM_TensorPartialRefit)
    ->Arg(8)
    ->Arg(32)
    ->Arg(96)
    ->Iterations(5000)
    ->Unit(benchmark::kMicrosecond);

}  // namespace
