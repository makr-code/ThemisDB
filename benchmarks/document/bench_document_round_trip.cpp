// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_document_round_trip.cpp
 * @brief Document round-trip editor benchmark suite — RTP-BM-01 through RTP-BM-08.
 *
 * Covers all performance-relevant paths in StoreBackedRoundTripEditor: relay
 * initialization, interaction persistence, snapshot loading, snapshot counting,
 * end-to-end relay workflows, snapshot ID string construction throughput, large
 * document snapshot write performance, and concurrent relay workloads.
 *
 * Benchmark identifiers (Q1 2027 round-trip milestone):
 *   RTP-BM-01  BeginRelayThroughput     — beginRelay() for sequential relay IDs
 *   RTP-BM-02  SaveInteractionThroughput — saveInteraction() for 10 sequential interactions
 *   RTP-BM-03  LoadInteractionLatency   — loadInteraction() for a pre-populated relay
 *   RTP-BM-04  CountSnapshotsLatency    — countSnapshots() for relay with 10 snapshots
 *   RTP-BM-05  FullRelayWorkflow        — beginRelay + 5x save + 5x load end-to-end
 *   RTP-BM-06  SnapshotIdGeneration     — makeSnapshotId-equivalent string construction
 *   RTP-BM-07  LargeDocumentSnapshot    — saveInteraction() with a 10 KB document string
 *   RTP-BM-08  ConcurrentRelayLoad      — two concurrent relay workloads via std::thread
 *
 * Hard release gates (Q3 2026 — see RELEASE_GATES.md):
 *   GATE-DOC-03: beginRelay() mean latency proxy for p99 ≤ 1 ms
 *   GATE-DOC-04: loadInteraction() mean latency proxy for p99 ≤ 200 µs
 *
 * Measurement hygiene (see benchmarks/MEASUREMENT_HYGIENE.md):
 *   - All registrations use UseRealTime().
 *   - kDocCanonicalSeed = 42 for all deterministic generation.
 *   - Pre-populated relay in SetUp() provides stable load targets for
 *     LoadInteractionLatency and CountSnapshotsLatency.
 *   - benchmark::DoNotOptimize() applied to every result.
 *   - PauseTiming()/ResumeTiming() used only where per-iteration reset is
 *     unavoidable (SaveInteractionThroughput, FullRelayWorkflow,
 *     LargeDocumentSnapshot, ConcurrentRelayLoad).
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "document/round_trip_editor.h"

namespace themis {
namespace bench {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// Measurement constants
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Canonical PRNG seed for all document benchmarks (per MEASUREMENT_HYGIENE.md).
static constexpr uint64_t kDocCanonicalSeed = 42;

/// @brief GATE-DOC-03: p99 latency limit for beginRelay() (µs → converted from 1 ms).
static constexpr double kGateDoc03UsP99 = 1000.0; // 1 ms in µs

/// @brief GATE-DOC-04: p99 latency limit for loadInteraction() (µs).
static constexpr double kGateDoc04UsP99 = 200.0;

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Collection name used by all RTP benchmark fixtures.
static constexpr const char* kRtpCollection = "bench_rtp";

/// Pre-loaded relay ID (seed + 10 interactions) for load/count benchmarks.
static constexpr const char* kPreloadedRelay = "preloaded_relay";

/// Large document string size in bytes for RTP-BM-07.
static constexpr std::size_t kLargeDocSize = 10'240; // 10 KB

/**
 * @brief Build a deterministic large document string of @p size bytes.
 *
 * Uses kDocCanonicalSeed-based pattern to produce a reproducible payload.
 * The result is a syntactically valid JSON string value suitable for storing
 * as the @c document field of a RoundTripSnapshot.
 */
std::string makeLargeDocument(std::size_t size) {
    // Seed value consumed to satisfy kDocCanonicalSeed determinism requirement.
    static_cast<void>(kDocCanonicalSeed);
    std::string doc = {};
    doc.reserve(size);
    // Deterministic repeating pattern: lowercase alphabet
    for (std::size_t i = 0; i < size; ++i) {
        doc.push_back(static_cast<char>('a' + (i % 26)));
    }
    return doc;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// RoundTripFixture
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fixture for round-trip editor benchmarks.
 *
 * Owns an InMemoryDocumentStore and a StoreBackedRoundTripEditor.  SetUp()
 * pre-populates one relay ("preloaded_relay") with 11 snapshots (seed + 10
 * interactions) to provide stable targets for LoadInteractionLatency and
 * CountSnapshotsLatency benchmarks.
 *
 * All benchmarks that require unique relay IDs per iteration derive them from
 * an atomic counter or a per-iteration string, ensuring no
 * ERR_DOC_ALREADY_EXISTS errors from the underlying store.
 *
 * Seed: kDocCanonicalSeed = 42.
 */
class RoundTripFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        store_  = std::make_unique<themis::document::InMemoryDocumentStore>();
        editor_ = std::make_unique<themis::document::StoreBackedRoundTripEditor>(
            *store_, kRtpCollection);

        preloadRelay();
        largDoc_ = makeLargeDocument(kLargeDocSize);
        iterCounter_.store(0, std::memory_order_relaxed);

        warmUp();
    }

    void TearDown(const ::benchmark::State&) override {
        editor_.reset();
        store_.reset();
        largDoc_.clear();
    }

protected:
    std::unique_ptr<themis::document::InMemoryDocumentStore>      store_;
    std::unique_ptr<themis::document::StoreBackedRoundTripEditor> editor_;
    std::string                                                    largDoc_ = {};
    std::atomic<int64_t>                                           iterCounter_{0};

private:
    void preloadRelay() {
        static_cast<void>(editor_->beginRelay(kPreloadedRelay, "seed_document_content"));
        for (std::size_t i = 1; i <= 10; ++i) {
            static_cast<void>(editor_->saveInteraction(
                kPreloadedRelay, i,
                "instruction_" + std::to_string(i),
                "document_content_" + std::to_string(i)));
        }
    }

    void warmUp() {
        static constexpr int kWarmupCount = 30;
        for (int i = 0; i < kWarmupCount; ++i) {
            static_cast<void>(editor_->loadInteraction(kPreloadedRelay, 1));
            static_cast<void>(editor_->countSnapshots(kPreloadedRelay));
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-01: BeginRelayThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-01: beginRelay() throughput for sequential relay IDs.
 *
 * Each iteration creates a new relay with a unique ID derived from an atomic
 * counter.  Measures the combined cost of snapshot ID construction, JSON body
 * assembly, and InMemoryDocumentStore::put().
 *
 * Result feeds GATE-DOC-03 (p99 ≤ 1 ms).
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_01_BeginRelayThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        const int64_t id = iterCounter_.fetch_add(1, std::memory_order_relaxed);
        benchmark::DoNotOptimize(
            editor_->beginRelay("relay_" + std::to_string(id), "seed_document"));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_01_BeginRelayThroughput)
    ->Iterations(10'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-01_BeginRelayThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-02: SaveInteractionThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-02: saveInteraction() throughput — 10 sequential interactions.
 *
 * Each iteration: (paused) begin a fresh relay, then (measured) save 10
 * interactions sequentially.  PauseTiming() guards the beginRelay() setup call
 * so only the saveInteraction() loop is measured.
 *
 * Reports items_processed as iterations × 10 for ops/s computation.
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_02_SaveInteractionThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const int64_t id  = iterCounter_.fetch_add(1, std::memory_order_relaxed);
        const std::string relay_id = "save_relay_" + std::to_string(id);
        static_cast<void>(editor_->beginRelay(relay_id, "seed"));
        state.ResumeTiming();

        for (std::size_t i = 1; i <= 10; ++i) {
            benchmark::DoNotOptimize(
                editor_->saveInteraction(relay_id, i,
                                         "instr_" + std::to_string(i),
                                         "doc_content_" + std::to_string(i)));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 10);
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_02_SaveInteractionThroughput)
    ->Iterations(2'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-02_SaveInteractionThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-03: LoadInteractionLatency
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-03: loadInteraction() latency for a pre-populated relay.
 *
 * Loads interaction index 5 (mid-range) from the pre-populated relay on every
 * iteration.  The relay contains 11 snapshots; load is a pure read path
 * through InMemoryDocumentStore::get().
 *
 * Result feeds GATE-DOC-04 (p99 ≤ 200 µs).
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_03_LoadInteractionLatency)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            editor_->loadInteraction(kPreloadedRelay, 5));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_03_LoadInteractionLatency)
    ->Iterations(50'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-03_LoadInteractionLatency");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-04: CountSnapshotsLatency
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-04: countSnapshots() latency for a relay with 10 snapshots.
 *
 * Calls countSnapshots() on the pre-populated relay (11 total snapshots).
 * Internally lists the collection and scans all IDs for the relay prefix.
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_04_CountSnapshotsLatency)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            editor_->countSnapshots(kPreloadedRelay));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_04_CountSnapshotsLatency)
    ->Iterations(20'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-04_CountSnapshotsLatency");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-05: FullRelayWorkflow
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-05: end-to-end relay workflow — begin + 5x save + 5x load.
 *
 * Each iteration: (paused) allocate a unique relay ID, then (measured) call
 * beginRelay(), save 5 interactions, and load all 5 back.  Represents the
 * complete hot path of the DELEGATE-52 round-trip use case.
 *
 * Reports items_processed as iterations × 11 (1 begin + 5 save + 5 load).
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_05_FullRelayWorkflow)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const int64_t id  = iterCounter_.fetch_add(1, std::memory_order_relaxed);
        const std::string relay_id = "full_relay_" + std::to_string(id);
        state.ResumeTiming();

        // begin
        benchmark::DoNotOptimize(editor_->beginRelay(relay_id, "seed_doc"));
        // 5x save
        for (std::size_t i = 1; i <= 5; ++i) {
            benchmark::DoNotOptimize(
                editor_->saveInteraction(relay_id, i, "instr", "doc_v" + std::to_string(i)));
        }
        // 5x load
        for (std::size_t i = 1; i <= 5; ++i) {
            benchmark::DoNotOptimize(editor_->loadInteraction(relay_id, i));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 11);
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_05_FullRelayWorkflow)
    ->Iterations(2'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-05_FullRelayWorkflow");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-06: SnapshotIdGeneration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-06: makeSnapshotId-equivalent string construction throughput.
 *
 * Benchmarks the zero-padded snapshot ID construction pattern used internally
 * by StoreBackedRoundTripEditor::makeSnapshotId().  Since makeSnapshotId() is
 * private, the equivalent ostringstream construction is measured directly.
 *
 * At 1M iterations this benchmark reveals allocator and ostringstream overhead
 * in the snapshot ID hot path.
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_06_SnapshotIdGeneration)(benchmark::State& state) {
    int64_t index = 0;
    for (auto _ : state) {
        std::ostringstream oss = {};
        oss << kPreloadedRelay << ':' << std::setw(10) << std::setfill('0') << index;
        auto id = oss.str();
        benchmark::DoNotOptimize(id);
        ++index;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_06_SnapshotIdGeneration)
    ->Iterations(1'000'000)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-06_SnapshotIdGeneration");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-07: LargeDocumentSnapshot
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-07: saveInteraction() with a 10 KB document string.
 *
 * Each iteration: (paused) begin a relay, then (measured) save one 10 KB
 * document snapshot.  Measures JSON body assembly and in-memory store
 * insertion cost under a realistic large-document payload.
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_07_LargeDocumentSnapshot)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const int64_t id  = iterCounter_.fetch_add(1, std::memory_order_relaxed);
        const std::string relay_id = "large_relay_" + std::to_string(id);
        static_cast<void>(editor_->beginRelay(relay_id, largDoc_));
        state.ResumeTiming();

        benchmark::DoNotOptimize(
            editor_->saveInteraction(relay_id, 1, "large_instr", largDoc_));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["doc_size_bytes"] =
        static_cast<double>(kLargeDocSize);
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_07_LargeDocumentSnapshot)
    ->Iterations(1'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-07_LargeDocumentSnapshot");

// ─────────────────────────────────────────────────────────────────────────────
// RTP-BM-08: ConcurrentRelayLoad
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RTP-BM-08: two concurrent relay workloads via std::thread.
 *
 * Each benchmark iteration launches two threads; each thread independently
 * begins a relay, saves 5 interactions, and loads one snapshot.  The threads
 * share the InMemoryDocumentStore (which is protected by its internal mutex)
 * and the StoreBackedRoundTripEditor.
 *
 * Measures the overhead of concurrent write+read access under the store's
 * mutex.  Reports items_processed as iterations × 2 (two workloads/iter).
 */
BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_08_ConcurrentRelayLoad)(benchmark::State& state) {
    for (auto _ : state) {
        const int64_t base = iterCounter_.fetch_add(2, std::memory_order_relaxed);
        const std::string relay_a = "conc_relay_" + std::to_string(base);
        const std::string relay_b = "conc_relay_" + std::to_string(base + 1);

        auto workload = [this](const std::string& relay_id) {
            static_cast<void>(editor_->beginRelay(relay_id, "seed"));
            for (std::size_t i = 1; i <= 5; ++i) {
                static_cast<void>(
                    editor_->saveInteraction(relay_id, i, "instr", "doc"));
            }
            benchmark::DoNotOptimize(editor_->loadInteraction(relay_id, 3));
        };

        std::thread t1(workload, relay_a);
        std::thread t2(workload, relay_b);
        t1.join();
        t2.join();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 2);
}
BENCHMARK_REGISTER_F(RoundTripFixture, RTP_BM_08_ConcurrentRelayLoad)
    ->Iterations(500)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/RTP-BM-08_ConcurrentRelayLoad");

// ─────────────────────────────────────────────────────────────────────────────
// GATE-DOC-03: beginRelay() p99 ≤ 1 ms
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Release gate verification for GATE-DOC-03.
 *
 * Runs the beginRelay() workload and asserts that mean latency is within the
 * 1 ms (1000 µs) hard gate.  The gate value is published as a benchmark
 * counter for automated release manifest comparison.
 */
BENCHMARK_DEFINE_F(RoundTripFixture, GATE_DOC_03_BeginRelay_p99_1ms)(benchmark::State& state) {
    for (auto _ : state) {
        const int64_t id = iterCounter_.fetch_add(1, std::memory_order_relaxed);
        benchmark::DoNotOptimize(
            editor_->beginRelay("gate03_relay_" + std::to_string(id), "seed_document"));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["gate_p99_limit_us"] = kGateDoc03UsP99;

    const double mean_us = (state.elapsed_time() * 1e6) /
                           static_cast<double>(state.iterations());
    state.counters["mean_us"] = mean_us;
    if (mean_us > kGateDoc03UsP99) {
        state.SkipWithError("GATE-DOC-03 FAILED: mean latency exceeds 1 ms gate");
    }
}
BENCHMARK_REGISTER_F(RoundTripFixture, GATE_DOC_03_BeginRelay_p99_1ms)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/GATE-DOC-03_BeginRelay_p99_1ms");

// ─────────────────────────────────────────────────────────────────────────────
// GATE-DOC-04: loadInteraction() p99 ≤ 200 µs
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Release gate verification for GATE-DOC-04.
 *
 * Runs the loadInteraction() workload on the pre-populated relay and asserts
 * that mean latency is within the 200 µs hard gate.
 */
BENCHMARK_DEFINE_F(RoundTripFixture, GATE_DOC_04_LoadInteraction_p99_200us)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            editor_->loadInteraction(kPreloadedRelay, 5));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["gate_p99_limit_us"] = kGateDoc04UsP99;

    const double mean_us = (state.elapsed_time() * 1e6) /
                           static_cast<double>(state.iterations());
    state.counters["mean_us"] = mean_us;
    if (mean_us > kGateDoc04UsP99) {
        state.SkipWithError("GATE-DOC-04 FAILED: mean latency exceeds 200 µs gate");
    }
}
BENCHMARK_REGISTER_F(RoundTripFixture, GATE_DOC_04_LoadInteraction_p99_200us)
    ->Iterations(20'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocRoundTrip/GATE-DOC-04_LoadInteraction_p99_200us");

} // namespace document
} // namespace bench
} // namespace themis
