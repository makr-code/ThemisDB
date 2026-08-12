#include <benchmark/benchmark.h>

#include "src/distributed_tensor/include/tensor_delta_log.h"
#include "src/distributed_tensor/include/snapshot_update_worker.h"
#include "src/distributed_tensor/include/artifact_manifest.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace themis::distributed_tensor;

namespace {

static constexpr uint32_t kDynamicUpdateSeed = 42;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build @p n DeltaLogEntries with payload @p bytes_each and add them to @p log.
void populateDeltaLog(TensorDeltaLog& log, size_t n,
                      uint32_t bytes_each = 100) {
    for (size_t i = 0; i < n; ++i) {
        log.appendDelta(DeltaMutationType::INSERT,
                        "entity-" + std::to_string(i),
                        "txn-" + std::to_string(i),
                        "shard-0",
                        bytes_each);
    }
}

/// Build a DeltaWindow that represents @p pct percent of @p artifact_size bytes.
DeltaWindow makeWindowWithFraction(const std::string& artifact_id,
                                    double pct,
                                    uint64_t artifact_size) {
    DeltaWindow win;
    win.artifact_id    = artifact_id;
    win.sequence_start = 1;

    const uint64_t target_bytes = static_cast<uint64_t>(artifact_size * pct / 100.0);
    const size_t n_entries = std::max<size_t>(1, target_bytes / 100);
    win.sequence_end   = static_cast<uint64_t>(n_entries);
    win.total_payload_size_bytes = target_bytes;
    win.extracted_at_ms = 1718000000000LL;

    for (size_t i = 0; i < n_entries; ++i) {
        DeltaLogEntry e;
        e.sequence_number       = static_cast<uint64_t>(i + 1);
        e.mutation_type         = DeltaMutationType::INSERT;
        e.affected_entity_id    = "entity-" + std::to_string(i);
        e.recorded_at_ms        = 1718000000000LL;
        e.source_transaction_id = "txn-" + std::to_string(i);
        e.payload_size_bytes    = 100;
        win.entries.push_back(e);
    }
    return win;
}

/// Minimal ArtifactManifest for update worker tasks.
ArtifactManifest makeManifest(const std::string& id = "art-bench",
                               double residual = 0.01) {
    ArtifactManifest m;
    m.artifact_id = id;
    m.tensor_name = "bench/embedding";
    m.kind        = ArtifactKind::ADVISORY_SUMMARY;
    m.shard_id    = 0;
    m.version     = 1;
    m.created_at  = std::chrono::system_clock::now();
    m.residual    = residual;
    m.rank_status = 10;
    m.rank_cap    = 256;
    return m;
}

}  // namespace

// ============================================================================
// BDU-01: DeltaLog append throughput
// ============================================================================
static void BDU01_DeltaLogAppendThroughput(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state) {
        TensorDeltaLog log("bench-append");
        for (int i = 0; i < n; ++i) {
            benchmark::DoNotOptimize(
                log.appendDelta(DeltaMutationType::INSERT,
                                "entity-" + std::to_string(i),
                                "txn-" + std::to_string(i),
                                "shard-0", 100));
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BDU01_DeltaLogAppendThroughput)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BDU-02: extractWindow — small window (10 entries)
// ============================================================================
static void BDU02_ExtractWindowSmall(benchmark::State& state) {
    TensorDeltaLog log("bench-win-small");
    populateDeltaLog(log, 100);  // pre-fill 100 entries

    for (auto _ : state) {
        auto win = log.extractWindow(1, 10);
        benchmark::DoNotOptimize(win);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU02_ExtractWindowSmall)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BDU-03: extractWindow — large window (1 000 entries)
// ============================================================================
static void BDU03_ExtractWindowLarge(benchmark::State& state) {
    TensorDeltaLog log("bench-win-large");
    populateDeltaLog(log, 1000);
    const uint64_t end_seq = log.getCurrentSequence();

    for (auto _ : state) {
        auto win = log.extractWindow(1, end_seq);
        benchmark::DoNotOptimize(win);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU03_ExtractWindowLarge)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BDU-04: DeltaWindow::estimateChangeFraction
// ============================================================================
static void BDU04_EstimateChangeFraction(benchmark::State& state) {
    const auto win = makeWindowWithFraction("art-frac", 20.0, 10000);

    for (auto _ : state) {
        benchmark::DoNotOptimize(win.estimateChangeFraction(10000));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU04_EstimateChangeFraction)->Unit(benchmark::kNanosecond);

// ============================================================================
// BDU-05: DeltaLogEntry serialize/deserialize round-trip
// ============================================================================
static void BDU05_SerializeDeserializeRoundTrip(benchmark::State& state) {
    DeltaLogEntry entry;
    entry.sequence_number       = 42;
    entry.mutation_type         = DeltaMutationType::UPDATE;
    entry.affected_entity_id    = "entity-benchmark-xyz";
    entry.recorded_at_ms        = 1718000000000LL;
    entry.source_transaction_id = "txn-benchmark-001";
    entry.shard_hint            = "shard-3";
    entry.payload_size_bytes    = 512;
    entry.payload_checksum      = "crc32:aabbccdd";

    const std::string serialized = entry.serialize();
    for (auto _ : state) {
        auto parsed = DeltaLogEntry::deserialize(serialized);
        benchmark::DoNotOptimize(parsed);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU05_SerializeDeserializeRoundTrip)->Unit(benchmark::kNanosecond);

// ============================================================================
// BDU-06: decideUpdateStrategy — PATCH path (< 10 %)
// ============================================================================
static void BDU06_DecideStrategyPatch(benchmark::State& state) {
    SnapshotBasedUpdateWorker worker;
    worker.start();

    // 5 % fraction → PATCH
    const auto win = makeWindowWithFraction("art-patch-bench", 5.0, 10000);

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            worker.decideUpdateStrategy(win, 10000, 0.01));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU06_DecideStrategyPatch)->Unit(benchmark::kNanosecond);

// ============================================================================
// BDU-07: decideUpdateStrategy — PARTIAL_REFIT path (20 %)
// ============================================================================
static void BDU07_DecideStrategyPartialRefit(benchmark::State& state) {
    SnapshotBasedUpdateWorker worker;
    worker.start();

    // 20 % fraction → PARTIAL_REFIT (low residual)
    const auto win = makeWindowWithFraction("art-refit-bench", 20.0, 10000);

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            worker.decideUpdateStrategy(win, 10000, 0.01));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU07_DecideStrategyPartialRefit)->Unit(benchmark::kNanosecond);

// ============================================================================
// BDU-08: decideUpdateStrategy — REBUILD path (> 50 %)
// ============================================================================
static void BDU08_DecideStrategyRebuild(benchmark::State& state) {
    SnapshotBasedUpdateWorker worker;
    worker.start();

    // 60 % fraction → REBUILD
    const auto win = makeWindowWithFraction("art-rebuild-bench", 60.0, 10000);

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            worker.decideUpdateStrategy(win, 10000, 0.01));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU08_DecideStrategyRebuild)->Unit(benchmark::kNanosecond);

// ============================================================================
// BDU-09: DeltaLog garbage_collect cost (10 000 entries)
// ============================================================================
static void BDU09_GarbageCollectCost(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        TensorDeltaLog log("bench-gc");
        populateDeltaLog(log, 10000);
        const uint64_t cutoff = log.getCurrentSequence() / 2;
        state.ResumeTiming();

        benchmark::DoNotOptimize(log.garbage_collect(cutoff));
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU09_GarbageCollectCost)->Unit(benchmark::kMillisecond);

// ============================================================================
// BDU-10: DeltaLog getStats() under high entry count
// ============================================================================
static void BDU10_GetStatsHighEntryCount(benchmark::State& state) {
    TensorDeltaLog log("bench-stats");
    populateDeltaLog(log, 10000);

    for (auto _ : state) {
        benchmark::DoNotOptimize(log.getStats());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BDU10_GetStatsHighEntryCount)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Smoke targets — fast CI validation (each < 1 s)
// ============================================================================

static void BDU_Smoke_AppendAndExtract(benchmark::State& state) {
    for (auto _ : state) {
        TensorDeltaLog log("smoke");
        log.appendDelta(DeltaMutationType::INSERT, "e1", "t1", "", 100);
        log.appendDelta(DeltaMutationType::UPDATE, "e2", "t1", "", 100);
        benchmark::DoNotOptimize(log.extractWindow(1, 2));
    }
}
BENCHMARK(BDU_Smoke_AppendAndExtract)
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

static void BDU_Smoke_DecideStrategy(benchmark::State& state) {
    SnapshotBasedUpdateWorker worker;
    worker.start();
    const auto win = makeWindowWithFraction("smoke-decide", 5.0, 10000);
    for (auto _ : state) {
        benchmark::DoNotOptimize(worker.decideUpdateStrategy(win, 10000, 0.01));
    }
}
BENCHMARK(BDU_Smoke_DecideStrategy)
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
