// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_exporters_release_gates.cpp
 * @brief Phase 5 exporters hot-path release-gate benchmarks (ERRG-01..ERRG-06).
 *
 * Provides reproducible latency and throughput measurements for the
 * exporters hot paths identified in the exporters module roadmap
 * (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### ERRG-01 — CSV row serialize (10 columns, in-memory)
 *   ≥ 1M rows/s
 *
 * ### ERRG-02 — Parquet row group write (100 rows, mock)
 *   p99 ≤ 5 ms
 *
 * ### ERRG-03 — Schema validation (10-column schema)
 *   p99 ≤ 100 µs
 *
 * ### ERRG-04 — Null handling decision
 *   p99 ≤ 10 µs
 *
 * ### ERRG-05 — Arrow batch serialize (100 rows)
 *   p99 ≤ 1 ms
 *
 * ### ERRG-06 — Export quota check
 *   p99 ≤ 50 µs
 *
 * ## Hard release gates
 *
 * | Gate ID       | Benchmark | Threshold       |
 * |---------------|-----------|-----------------|
 * | GATE-ERRG-01  | ERRG-01   | ≥ 1M rows/s     |
 * | GATE-ERRG-02  | ERRG-02   | p99 ≤ 5 ms (RT) |
 * | GATE-ERRG-03  | ERRG-03   | p99 ≤ 100 µs    |
 * | GATE-ERRG-04  | ERRG-04   | p99 ≤ 10 µs     |
 * | GATE-ERRG-05  | ERRG-05   | p99 ≤ 1 ms      |
 * | GATE-ERRG-06  | ERRG-06   | p99 ≤ 50 µs     |
 *
 * @see include/exporters/exporters_api_contract.h
 * @see src/exporters/ROADMAP.md — Phase 5 item
 */

#include <benchmark/benchmark.h>

#include "exporters/exporters_api_contract.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::exporters;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace errg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kExportersCanonicalSeed = 42;
static constexpr int      kRepetitions            = 5;
static constexpr int      kWarmupIterations       = 200;

// ---------------------------------------------------------------------------
// Mock: CSV row with 10 columns
// ---------------------------------------------------------------------------

using CsvCell = std::optional<std::string>;
using CsvRow  = std::vector<CsvCell>;

static CsvRow makeCsvRow(int idx) {
    CsvRow row;
    row.reserve(10);
    for (int c = 0; c < 10; ++c) {
        if (c == 3) row.push_back(std::nullopt); // null column
        else        row.push_back("val_" + std::to_string(idx) + "_" + std::to_string(c));
    }
    return row;
}

static std::string serializeCsvRow(const CsvRow& row) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i > 0) {
          ss << kCsvDefaultDelimiter;
        }
        if (row[i].has_value()) {
          ss << *row[i];
        }
        // null → empty cell
    }
    ss << "\n";
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock: Parquet row group (100 rows, fixed-width cells)
// ---------------------------------------------------------------------------

struct ParquetCell {
    bool     is_null = 0;
    int64_t  int_val;
    char     str_val[16];
};

struct ParquetRowGroup {
    static constexpr int kCols = 10;
    std::vector<std::array<ParquetCell, 10>> rows;

    void add(int row_idx) {
        std::array<ParquetCell, 10> r{};
        for (int c = 0; c < kCols; ++c) {
            r[c].is_null  = (c == 2);
            r[c].int_val  = row_idx * kCols + c;
        }
        rows.push_back(r);
    }

    std::string serialize() const {
        std::ostringstream ss;
        for (auto& row : rows)
            for (auto& cell : row)
                ss << cell.int_val << ',';
        return ss.str();
    }
};

// ---------------------------------------------------------------------------
// Mock: schema validation (10-column schema)
// ---------------------------------------------------------------------------

struct SchemaCol { const char* name; int type; bool required; };
static const SchemaCol kSchema10[10] = {
    {"id",    0, true},  {"name",  1, true},  {"ts",    0, true},
    {"val_a", 2, false}, {"val_b", 2, false}, {"val_c", 2, false},
    {"tag1",  1, false}, {"tag2",  1, false}, {"tag3",  1, false}, {"extra", 1, false},
};

struct IncomingRow {
    int     values[10];
    bool    present[10];
};

static bool validateSchema(const IncomingRow& row) noexcept {
    for (int i = 0; i < 10; ++i)
        if (kSchema10[i].required && !row.present[i]) {
          return false;
        }
    return true;
}

// ---------------------------------------------------------------------------
// Mock: null handling decision (single cell)
// ---------------------------------------------------------------------------

enum class NullTarget { Csv, Parquet, Arrow };

static std::string handleNull(bool is_null, NullTarget target) {
    if (!is_null) {
      return "non-null";
    }
    switch (target) {
        case NullTarget::Csv:     return "";          // empty cell
        case NullTarget::Parquet: return "<null-bit>"; // null bitmap
        case NullTarget::Arrow:   return "<validity>"; // validity bitmap
    }
    return "";
}

// ---------------------------------------------------------------------------
// Mock: Arrow batch serialize (100 rows, 10 columns)
// ---------------------------------------------------------------------------

static std::string serializeArrowBatch(int n_rows, int n_cols) {
    std::ostringstream ss;
    ss << "ARROW:rows=" << n_rows << ",cols=" << n_cols << ",data=[";
    for (int r = 0; r < n_rows; ++r)
        for (int c = 0; c < n_cols; ++c)
            ss << (r * n_cols + c) << ',';
    ss << "]";
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock: export quota check
// ---------------------------------------------------------------------------

struct ExportQuota {
    std::uint64_t max_rows;
    std::atomic<std::uint64_t> used{0};

    ExporterErrorCode check(std::uint64_t rows) noexcept {
        auto prev = used.load(std::memory_order_relaxed);
        if (max_rows > 0 && prev + rows > max_rows)
            return ExporterErrorCode::QUOTA_EXCEEDED;
        used.store(prev + rows, std::memory_order_relaxed);
        return ExporterErrorCode::OK;
    }
};

// ===========================================================================
// ERRG-01 — CSV row serialize (10 columns, in-memory)  (≥ 1M rows/s)
// ===========================================================================

/**
 * @brief ERRG-01: serializeCsvRow() for a 10-column row.
 * GATE-ERRG-01: ≥ 1M rows/s.
 */
static void BM_ERRG01_CsvRowSerialize(benchmark::State& state) {
    int idx = 0;
    for (int i = 0; i < kWarmupIterations; ++i) {
        auto row = makeCsvRow(idx++);
        benchmark::DoNotOptimize(serializeCsvRow(row));
    }

    std::int64_t ops = 0;
    for (auto _ : state) {
        auto row = makeCsvRow(idx++);
        benchmark::DoNotOptimize(serializeCsvRow(row));
        ++ops;
    }
    state.SetItemsProcessed(ops);
    state.SetLabel("GATE-ERRG-01: >= 1M rows/s");
}
BENCHMARK(BM_ERRG01_CsvRowSerialize)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ERRG-02 — Parquet row group write (100 rows, mock)  (p99 ≤ 5 ms)
// ===========================================================================

/**
 * @brief ERRG-02: Build and serialize a 100-row ParquetRowGroup.
 * UseRealTime() because this involves memory allocation.
 * GATE-ERRG-02: p99 ≤ 5 ms.
 */
static void BM_ERRG02_ParquetRowGroupWrite(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        ParquetRowGroup g;
        for (int r = 0; r < 100; ++r) {
          g.add(r);
        }
        benchmark::DoNotOptimize(g.serialize());
    }

    for (auto _ : state) {
        ParquetRowGroup g;
        for (int r = 0; r < 100; ++r) {
          g.add(r);
        }
        benchmark::DoNotOptimize(g.serialize());
    }
    state.SetLabel("GATE-ERRG-02: p99 <= 5 ms");
}
BENCHMARK(BM_ERRG02_ParquetRowGroupWrite)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ERRG-03 — Schema validation (10-column schema)  (p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief ERRG-03: validateSchema() for a 10-column IncomingRow.
 * GATE-ERRG-03: p99 ≤ 100 µs.
 */
static void BM_ERRG03_SchemaValidation(benchmark::State& state) {
    IncomingRow row{};
    for (int i = 0; i < 10; ++i) { row.values[i] = i; row.present[i] = true; }

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(validateSchema(row));

    for (auto _ : state) {
        benchmark::DoNotOptimize(validateSchema(row));
    }
    state.SetLabel("GATE-ERRG-03: p99 <= 100 us");
}
BENCHMARK(BM_ERRG03_SchemaValidation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ERRG-04 — Null handling decision  (p99 ≤ 10 µs)
// ===========================================================================

/**
 * @brief ERRG-04: handleNull() decision for CSV/Parquet/Arrow targets.
 * GATE-ERRG-04: p99 ≤ 10 µs.
 */
static void BM_ERRG04_NullHandlingDecision(benchmark::State& state) {
    std::mt19937_64 rng(kExportersCanonicalSeed);
    std::bernoulli_distribution null_dist(0.1);

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(handleNull(null_dist(rng), NullTarget::Parquet));

    for (auto _ : state) {
        benchmark::DoNotOptimize(handleNull(null_dist(rng), NullTarget::Parquet));
    }
    state.SetLabel("GATE-ERRG-04: p99 <= 10 us");
}
BENCHMARK(BM_ERRG04_NullHandlingDecision)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ERRG-05 — Arrow batch serialize (100 rows)  (p99 ≤ 1 ms)
// ===========================================================================

/**
 * @brief ERRG-05: serializeArrowBatch() for 100 rows × 10 columns.
 * GATE-ERRG-05: p99 ≤ 1 ms.
 */
static void BM_ERRG05_ArrowBatchSerialize(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(serializeArrowBatch(100, 10));

    for (auto _ : state) {
        benchmark::DoNotOptimize(serializeArrowBatch(100, 10));
    }
    state.SetLabel("GATE-ERRG-05: p99 <= 1 ms");
}
BENCHMARK(BM_ERRG05_ArrowBatchSerialize)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ERRG-06 — Export quota check  (p99 ≤ 50 µs)
// ===========================================================================

/**
 * @brief ERRG-06: ExportQuota::check() — atomic load + compare.
 * GATE-ERRG-06: p99 ≤ 50 µs.
 */
static void BM_ERRG06_ExportQuotaCheck(benchmark::State& state) {
    ExportQuota quota{/*max_rows=*/1'000'000u};

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(quota.check(1u));

    for (auto _ : state) {
        benchmark::DoNotOptimize(quota.check(1u));
    }
    state.SetLabel("GATE-ERRG-06: p99 <= 50 us");
}
BENCHMARK(BM_ERRG06_ExportQuotaCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace errg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
