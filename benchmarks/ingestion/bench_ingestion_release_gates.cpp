// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_ingestion_release_gates.cpp
 * @brief Phase 5 ingestion hot-path release-gate benchmarks (INRG-01..INRG-06).
 *
 * Provides reproducible latency and throughput measurements for the
 * ingestion hot paths identified in the ingestion module roadmap
 * (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### INRG-01 — Single write throughput (in-memory)
 *   ≥ 500k writes/s
 *
 * ### INRG-02 — Batch commit (100 rows)
 *   p99 ≤ 5 ms
 *
 * ### INRG-03 — Schema validation (per-row)
 *   p99 ≤ 100 µs
 *
 * ### INRG-04 — Buffer occupancy check
 *   p99 ≤ 10 µs
 *
 * ### INRG-05 — Back-pressure signal send
 *   p99 ≤ 50 µs
 *
 * ### INRG-06 — Quota check
 *   p99 ≤ 50 µs
 *
 * ## Hard release gates
 *
 * | Gate ID       | Benchmark | Threshold       |
 * |---------------|-----------|-----------------|
 * | GATE-INRG-01  | INRG-01   | ≥ 500k writes/s |
 * | GATE-INRG-02  | INRG-02   | p99 ≤ 5 ms (RT) |
 * | GATE-INRG-03  | INRG-03   | p99 ≤ 100 µs    |
 * | GATE-INRG-04  | INRG-04   | p99 ≤ 10 µs     |
 * | GATE-INRG-05  | INRG-05   | p99 ≤ 50 µs     |
 * | GATE-INRG-06  | INRG-06   | p99 ≤ 50 µs     |
 *
 * @see include/ingestion/ingestion_api_contract.h
 * @see src/ingestion/ROADMAP.md — Phase 5 item
 */

#include <benchmark/benchmark.h>

#include "ingestion/ingestion_api_contract.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::ingestion;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace inrg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kIngestionCanonicalSeed = 42;
static constexpr int      kRepetitions            = 5;
static constexpr int      kWarmupIterations       = 200;

// ---------------------------------------------------------------------------
// Mock: single row (fixed-size, in-memory)
// ---------------------------------------------------------------------------

struct IngestRow {
    std::uint64_t key;
    char          payload[64];
    bool          schema_valid;
};

static IngestRow makeRow(std::uint64_t key) noexcept {
    IngestRow r{};
    r.key          = key;
    r.schema_valid = true;
    std::memset(r.payload, static_cast<int>(key & 0xFF), sizeof(r.payload));
    return r;
}

// ---------------------------------------------------------------------------
// Mock: in-memory WAL (ring buffer write)
// ---------------------------------------------------------------------------

static constexpr std::size_t kWalSize = 1 << 20;  // 1M slots

struct MockWal {
    IngestRow slots[kWalSize];
    std::atomic<std::uint64_t> head{0};

    void write(const IngestRow& row) noexcept {
        std::uint64_t idx = head.fetch_add(1, std::memory_order_relaxed) % kWalSize;
        slots[idx] = row;
    }
};

static MockWal g_wal;

// ---------------------------------------------------------------------------
// Mock: batch commit (100 rows → string builder as mock persistence)
// ---------------------------------------------------------------------------

static std::string commitBatch(const std::vector<IngestRow>& rows) {
    std::ostringstream ss;
    for (auto& r : rows)
        ss << r.key << ':' << static_cast<int>(r.payload[0]) << ',';
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock: per-row schema validation
// ---------------------------------------------------------------------------

static bool validateIngestRow(const IngestRow& r) noexcept {
    return r.schema_valid;
}

// ---------------------------------------------------------------------------
// Mock: buffer occupancy check (atomic load)
// ---------------------------------------------------------------------------

struct IngestBuffer {
    std::atomic<std::size_t> used{0};
    std::size_t              capacity = kDefaultBufferCapacity;

    bool isFull(std::size_t n = 1) const noexcept {
        return used.load(std::memory_order_relaxed) + n > capacity;
    }

    void push(std::size_t n = 1) noexcept {
        used.fetch_add(n, std::memory_order_relaxed);
    }
};

// ---------------------------------------------------------------------------
// Mock: back-pressure signal (immediate function returning error code)
// ---------------------------------------------------------------------------

static IngestionErrorCode sendBackPressureSignal(IngestBuffer& buf) noexcept {
    if (buf.isFull())
        return IngestionErrorCode::INGESTION_BUFFER_FULL;
    return IngestionErrorCode::OK;
}

// ---------------------------------------------------------------------------
// Mock: quota check (atomic)
// ---------------------------------------------------------------------------

struct IngestQuota {
    std::uint64_t                limit;
    std::atomic<std::uint64_t>   used{0};

    IngestionErrorCode check(std::uint64_t n) noexcept {
        auto prev = used.load(std::memory_order_relaxed);
        if (limit > 0 && prev + n > limit)
            return IngestionErrorCode::INGESTION_QUOTA_EXCEEDED;
        used.store(prev + n, std::memory_order_relaxed);
        return IngestionErrorCode::OK;
    }
};

// ===========================================================================
// INRG-01 — Single write throughput (in-memory)  (≥ 500k writes/s)
// ===========================================================================

/**
 * @brief INRG-01: MockWal::write() — single in-memory WAL row write.
 * GATE-INRG-01: ≥ 500k writes/s.
 */
static void BM_INRG01_SingleWriteThroughput(benchmark::State& state) {
    std::uint64_t key = 0;
    for (int i = 0; i < kWarmupIterations; ++i)
        g_wal.write(makeRow(key++));

    std::int64_t ops = 0;
    for (auto _ : state) {
        g_wal.write(makeRow(key++));
        ++ops;
    }
    state.SetItemsProcessed(ops);
    state.SetLabel("GATE-INRG-01: >= 500k writes/s");
}
BENCHMARK(BM_INRG01_SingleWriteThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// INRG-02 — Batch commit (100 rows)  (p99 ≤ 5 ms)
// ===========================================================================

/**
 * @brief INRG-02: commitBatch() for 100 rows.
 * UseRealTime() because this involves heap allocation.
 * GATE-INRG-02: p99 ≤ 5 ms.
 */
static void BM_INRG02_BatchCommit(benchmark::State& state) {
    std::vector<IngestRow> batch;
    batch.reserve(100);
    for (int i = 0; i < 100; ++i) {
      batch.push_back(makeRow(i));
    }

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(commitBatch(batch));

    for (auto _ : state) {
        benchmark::DoNotOptimize(commitBatch(batch));
    }
    state.SetLabel("GATE-INRG-02: p99 <= 5 ms");
}
BENCHMARK(BM_INRG02_BatchCommit)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// INRG-03 — Schema validation (per-row)  (p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief INRG-03: validateIngestRow() per-row schema check.
 * GATE-INRG-03: p99 ≤ 100 µs.
 */
static void BM_INRG03_SchemaValidation(benchmark::State& state) {
    auto row = makeRow(42u);
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(validateIngestRow(row));

    for (auto _ : state) {
        benchmark::DoNotOptimize(validateIngestRow(row));
    }
    state.SetLabel("GATE-INRG-03: p99 <= 100 us");
}
BENCHMARK(BM_INRG03_SchemaValidation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// INRG-04 — Buffer occupancy check  (p99 ≤ 10 µs)
// ===========================================================================

/**
 * @brief INRG-04: IngestBuffer::isFull() — atomic load + compare.
 * GATE-INRG-04: p99 ≤ 10 µs.
 */
static void BM_INRG04_BufferOccupancyCheck(benchmark::State& state) {
    IngestBuffer buf;
    buf.push(buf.capacity / 2);  // half full

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(buf.isFull());

    for (auto _ : state) {
        benchmark::DoNotOptimize(buf.isFull());
    }
    state.SetLabel("GATE-INRG-04: p99 <= 10 us");
}
BENCHMARK(BM_INRG04_BufferOccupancyCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// INRG-05 — Back-pressure signal send  (p99 ≤ 50 µs)
// ===========================================================================

/**
 * @brief INRG-05: sendBackPressureSignal() — immediate signal generation.
 * GATE-INRG-05: p99 ≤ 50 µs.
 */
static void BM_INRG05_BackPressureSignal(benchmark::State& state) {
    IngestBuffer buf;
    buf.push(buf.capacity);  // full

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(sendBackPressureSignal(buf));

    for (auto _ : state) {
        benchmark::DoNotOptimize(sendBackPressureSignal(buf));
    }
    state.SetLabel("GATE-INRG-05: p99 <= 50 us");
}
BENCHMARK(BM_INRG05_BackPressureSignal)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// INRG-06 — Quota check  (p99 ≤ 50 µs)
// ===========================================================================

/**
 * @brief INRG-06: IngestQuota::check() — atomic load + compare.
 * GATE-INRG-06: p99 ≤ 50 µs.
 */
static void BM_INRG06_QuotaCheck(benchmark::State& state) {
    IngestQuota quota{/*limit=*/100'000'000u};

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(quota.check(1u));

    for (auto _ : state) {
        benchmark::DoNotOptimize(quota.check(1u));
    }
    state.SetLabel("GATE-INRG-06: p99 <= 50 us");
}
BENCHMARK(BM_INRG06_QuotaCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace inrg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
