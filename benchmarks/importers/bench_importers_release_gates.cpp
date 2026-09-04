// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_importers_release_gates.cpp
 * @brief Phase 5 importers hot-path release-gate benchmarks (IMRG-01..IMRG-06).
 *
 * Provides reproducible latency and throughput measurements for the
 * importers hot paths identified in the importers module roadmap
 * (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### IMRG-01 — CSV row parse (10 columns)
 *   ≥ 5M rows/s (in-memory)
 *
 * ### IMRG-02 — Schema validation (per-row check)
 *   p99 ≤ 50 µs
 *
 * ### IMRG-03 — Duplicate key check (hash map, 10k keys)
 *   p99 ≤ 100 µs
 *
 * ### IMRG-04 — Row buffer commit (100 rows, mock persistence)
 *   p99 ≤ 5 ms
 *
 * ### IMRG-05 — Import quota check
 *   p99 ≤ 50 µs
 *
 * ### IMRG-06 — Schema evolution compatibility check
 *   p99 ≤ 200 µs
 *
 * ## Hard release gates
 *
 * | Gate ID       | Benchmark | Threshold       |
 * |---------------|-----------|-----------------|
 * | GATE-IMRG-01  | IMRG-01   | ≥ 5M rows/s     |
 * | GATE-IMRG-02  | IMRG-02   | p99 ≤ 50 µs     |
 * | GATE-IMRG-03  | IMRG-03   | p99 ≤ 100 µs    |
 * | GATE-IMRG-04  | IMRG-04   | p99 ≤ 5 ms (RT) |
 * | GATE-IMRG-05  | IMRG-05   | p99 ≤ 50 µs     |
 * | GATE-IMRG-06  | IMRG-06   | p99 ≤ 200 µs    |
 *
 * @see include/importers/importers_api_contract.h
 * @see src/importers/ROADMAP.md — Phase 5 item
 */

#include <benchmark/benchmark.h>

#include "importers/importers_api_contract.h"

#include <atomic>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace themis::importers;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace imrg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kImportersCanonicalSeed = 42;
static constexpr int      kRepetitions            = 5;
static constexpr int      kWarmupIterations       = 200;

// ---------------------------------------------------------------------------
// Mock: CSV row parse (split on delimiter, in-memory)
// ---------------------------------------------------------------------------

struct ParsedRow {
    std::string cells[10];
    int         cell_count = 0;
};

static ParsedRow parseCsvRow(const char* src, int len) noexcept {
    ParsedRow row{};
    const char* p = src;
    const char* end = src + len;
    while (p < end && row.cell_count < 10) {
        const char* q = p;
        while (q < end && *q != ',') {
          ++q;
        }
        row.cells[row.cell_count++] = std::string(p, static_cast<std::size_t>(q - p));
        p = (q < end) ? q + 1 : end;
    }
    return row;
}

static std::string makeCsvLine(int idx) {
    std::ostringstream ss;
    for (int c = 0; c < 10; ++c) {
        if (c > 0) {
          ss << ',';
        }
        ss << idx + c;
    }
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock: per-row schema validation (10 required integer columns)
// ---------------------------------------------------------------------------

struct RawRow {
    std::string cells[10];
    int         count;
    bool isValid() const noexcept { return count == 10; }
};

static bool validateRow(const RawRow& row) noexcept {
    return row.isValid();
}

// ---------------------------------------------------------------------------
// Mock: duplicate key check (hash set, pre-loaded with 10k keys)
// ---------------------------------------------------------------------------

using KeySet = std::unordered_set<std::string>;

static KeySet makeKeySet(int n) {
    KeySet ks;
    ks.reserve(n * 2);
    for (int i = 0; i < n; ++i) {
      ks.insert("key-" + std::to_string(i));
    }
    return ks;
}

static bool isDuplicateKey(const KeySet& ks, const std::string& key) {
    return ks.count(key) > 0;
}

// ---------------------------------------------------------------------------
// Mock: row buffer commit (100 rows, mock persistence via string builder)
// ---------------------------------------------------------------------------

struct RowBuffer {
    std::vector<RawRow> rows;

    void add(RawRow r) { rows.push_back(std::move(r)); }

    std::string commit() {
        std::ostringstream ss;
        for (auto& r : rows)
            for (int i = 0; i < r.count; ++i)
                ss << r.cells[i] << ',';
        rows.clear();
        return ss.str();
    }
};

// ---------------------------------------------------------------------------
// Mock: import quota check
// ---------------------------------------------------------------------------

struct ImportQuota {
    std::uint64_t max_rows;
    std::atomic<std::uint64_t> used{0};

    ImporterErrorCode check(std::uint64_t n) noexcept {
        auto prev = used.load(std::memory_order_relaxed);
        if (max_rows > 0 && prev + n > max_rows)
            return ImporterErrorCode::IMPORT_QUOTA_EXCEEDED;
        used.store(prev + n, std::memory_order_relaxed);
        return ImporterErrorCode::OK;
    }
};

// ---------------------------------------------------------------------------
// Mock: schema evolution check (existing vs incoming column lists)
// ---------------------------------------------------------------------------

struct ColDef { const char* name; int type; bool required; };

static const ColDef kExistingSchema[] = {
    {"id",    0, true}, {"name", 1, true}, {"ts",   0, true},
    {"val_a", 2, false}, {"val_b", 2, false}
};
static constexpr int kExistingCols = 5;

static SchemaChangeKind checkSchemaEvolution(
        int incoming_cols,
        const ColDef* incoming,
        int existing_cols,
        const ColDef* existing) noexcept {
    // Check for breaking: removed required or type changed
    for (int e = 0; e < existing_cols; ++e) {
        bool found = false;
        for (int i = 0; i < incoming_cols; ++i) {
            if (std::string(existing[e].name) == std::string(incoming[i].name)) {
                if (existing[e].type != incoming[i].type) {
                  return SchemaChangeKind::Breaking;
                }
                found = true;
                break;
            }
        }
        if (!found && existing[e].required) {
          return SchemaChangeKind::Breaking;
        }
    }
    // Check additive
    for (int i = 0; i < incoming_cols; ++i) {
        bool found = false;
        for (int e = 0; e < existing_cols; ++e) {
            if (std::string(incoming[i].name) == std::string(existing[e].name)) {
                found = true; break;
            }
        }
        if (!found && !incoming[i].required) {
          return SchemaChangeKind::Additive;
        }
    }
    return SchemaChangeKind::NoChange;
}

// ===========================================================================
// IMRG-01 — CSV row parse (10 columns)  (≥ 5M rows/s)
// ===========================================================================

/**
 * @brief IMRG-01: parseCsvRow() for a 10-column CSV line.
 * GATE-IMRG-01: ≥ 5M rows/s.
 */
static void BM_IMRG01_CsvRowParse(benchmark::State& state) {
    auto line = makeCsvLine(0);
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(parseCsvRow(line.data(), static_cast<int>(line.size())));

    std::int64_t ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(parseCsvRow(line.data(), static_cast<int>(line.size())));
        ++ops;
    }
    state.SetItemsProcessed(ops);
    state.SetLabel("GATE-IMRG-01: >= 5M rows/s");
}
BENCHMARK(BM_IMRG01_CsvRowParse)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// IMRG-02 — Schema validation (per-row check)  (p99 ≤ 50 µs)
// ===========================================================================

/**
 * @brief IMRG-02: validateRow() per-row schema check.
 * GATE-IMRG-02: p99 ≤ 50 µs.
 */
static void BM_IMRG02_PerRowSchemaValidation(benchmark::State& state) {
    RawRow row;
    row.count = 10;
    for (int c = 0; c < 10; ++c) {
      row.cells[c] = std::to_string(c);
    }

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(validateRow(row));

    for (auto _ : state) {
        benchmark::DoNotOptimize(validateRow(row));
    }
    state.SetLabel("GATE-IMRG-02: p99 <= 50 us");
}
BENCHMARK(BM_IMRG02_PerRowSchemaValidation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// IMRG-03 — Duplicate key check (hash map, 10k keys)  (p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief IMRG-03: isDuplicateKey() against a pre-loaded 10k-key hash set.
 * GATE-IMRG-03: p99 ≤ 100 µs.
 */
static void BM_IMRG03_DuplicateKeyCheck(benchmark::State& state) {
    auto ks = makeKeySet(10'000);
    std::mt19937_64 rng(kImportersCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, 9'999);

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(isDuplicateKey(ks, "key-" + std::to_string(dist(rng))));

    for (auto _ : state) {
        benchmark::DoNotOptimize(isDuplicateKey(ks, "key-" + std::to_string(dist(rng))));
    }
    state.SetLabel("GATE-IMRG-03: p99 <= 100 us");
}
BENCHMARK(BM_IMRG03_DuplicateKeyCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// IMRG-04 — Row buffer commit (100 rows, mock persistence)  (p99 ≤ 5 ms)
// ===========================================================================

/**
 * @brief IMRG-04: RowBuffer::commit() for 100 rows.
 * UseRealTime() because this involves heap allocation.
 * GATE-IMRG-04: p99 ≤ 5 ms.
 */
static void BM_IMRG04_RowBufferCommit(benchmark::State& state) {
    auto fillBuffer = []() {
        RowBuffer buf;
        for (int r = 0; r < 100; ++r) {
            RawRow row; row.count = 10;
            for (int c = 0; c < 10; ++c) {
              row.cells[c] = std::to_string(r * 10 + c);
            }
            buf.add(row);
        }
        return buf;
    };

    for (int i = 0; i < kWarmupIterations; ++i) {
        auto buf = fillBuffer();
        benchmark::DoNotOptimize(buf.commit());
    }

    for (auto _ : state) {
        auto buf = fillBuffer();
        benchmark::DoNotOptimize(buf.commit());
    }
    state.SetLabel("GATE-IMRG-04: p99 <= 5 ms");
}
BENCHMARK(BM_IMRG04_RowBufferCommit)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// IMRG-05 — Import quota check  (p99 ≤ 50 µs)
// ===========================================================================

/**
 * @brief IMRG-05: ImportQuota::check() — atomic load + compare.
 * GATE-IMRG-05: p99 ≤ 50 µs.
 */
static void BM_IMRG05_ImportQuotaCheck(benchmark::State& state) {
    ImportQuota quota{/*max_rows=*/10'000'000u};

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(quota.check(1u));

    for (auto _ : state) {
        benchmark::DoNotOptimize(quota.check(1u));
    }
    state.SetLabel("GATE-IMRG-05: p99 <= 50 us");
}
BENCHMARK(BM_IMRG05_ImportQuotaCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// IMRG-06 — Schema evolution compatibility check  (p99 ≤ 200 µs)
// ===========================================================================

/**
 * @brief IMRG-06: checkSchemaEvolution() comparing 5-column existing vs 6-column incoming.
 * GATE-IMRG-06: p99 ≤ 200 µs.
 */
static void BM_IMRG06_SchemaEvolutionCheck(benchmark::State& state) {
    static const ColDef kIncoming[] = {
        {"id",    0, true}, {"name", 1, true}, {"ts",    0, true},
        {"val_a", 2, false}, {"val_b", 2, false}, {"extra", 1, false}
    };
    static constexpr int kIncomingCols = 6;

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(
            checkSchemaEvolution(kIncomingCols, kIncoming,
                                 kExistingCols, kExistingSchema));

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            checkSchemaEvolution(kIncomingCols, kIncoming,
                                 kExistingCols, kExistingSchema));
    }
    state.SetLabel("GATE-IMRG-06: p99 <= 200 us");
}
BENCHMARK(BM_IMRG06_SchemaEvolutionCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace imrg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
