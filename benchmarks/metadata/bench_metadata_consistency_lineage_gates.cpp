// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_metadata_consistency_lineage_gates.cpp
 * @brief Release-gate benchmarks for metadata consistency and lineage hot paths.
 *
 * Covers the Phase D roadmap item:
 *   "broaden benchmark depth beyond cache-centric metadata hot paths"
 *
 * Gates:
 *   GATE-MCL-01  ConsistencyIssue::toJSON() ≥ 10M ops/s
 *   GATE-MCL-02  ColumnRef::toString()      ≥ 50M ops/s
 *   GATE-MCL-03  ColumnRef JSON round-trip  ≥ 5M ops/s
 *   GATE-MCL-04  TransformationType string  ≥ 20M ops/s
 *
 * Benchmark hygiene (benchmarks/MEASUREMENT_HYGIENE.md):
 *   - kCanonicalRngSeed = 42
 *   - No I/O; CPU-only benchmarks use default clock
 *   - Repetitions(5) + ReportAggregatesOnly(true) for stable statistics
 */

#include <benchmark/benchmark.h>

#include "metadata/schema_consistency_checker.h"
#include "metadata/column_lineage.h"

#include <string>
#include <array>

using namespace themis;
using namespace themis::metadata;

static constexpr uint64_t kCanonicalRngSeed = 42;

// ============================================================================
// GATE-MCL-01: ConsistencyIssue::toJSON() throughput
// Target: ≥ 10M ops/s
// ============================================================================
static void BM_ConsistencyIssue_ToJSON(benchmark::State& state) {
    ConsistencyIssue ci;
    ci.issue_type  = "stale_stats";
    ci.table_name  = "orders";
    ci.column_name = "updated_at";
    ci.detail      = "Stats older than 86400 seconds";

    for (auto _ : state) {
        auto j = ci.toJSON();
        benchmark::DoNotOptimize(j);
    }
}
BENCHMARK(BM_ConsistencyIssue_ToJSON)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MCL-02: ColumnRef::toString() throughput
// Target: ≥ 50M ops/s
// ============================================================================
static void BM_ColumnRef_ToString(benchmark::State& state) {
    static const std::array<ColumnRef, 4> kRefs = {{
        {"orders",   "customer_id"},
        {"users",    "email"},
        {"products", "sku"},
        {"catalog",  "version"},
    }};
    uint64_t idx = kCanonicalRngSeed;

    for (auto _ : state) {
        const auto& ref = kRefs[idx++ % 4];
        auto s = ref.toString();
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_ColumnRef_ToString)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MCL-03: ColumnRef toJSON / fromJSON round-trip throughput
// Target: ≥ 5M ops/s
// ============================================================================
static void BM_ColumnRef_JsonRoundTrip(benchmark::State& state) {
    ColumnRef original{"shipments", "tracking_id"};

    for (auto _ : state) {
        auto j        = original.toJSON();
        auto restored = ColumnRef::fromJSON(j);
        benchmark::DoNotOptimize(restored.table_name.data());
    }
}
BENCHMARK(BM_ColumnRef_JsonRoundTrip)->Repetitions(5)->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MCL-04: TransformationType string conversion throughput
// Target: ≥ 20M ops/s
// ============================================================================
static void BM_TransformationType_StringConversion(benchmark::State& state) {
    static const std::array<TransformationType, 8> kTypes = {{
        TransformationType::DIRECT_COPY,
        TransformationType::RENAME,
        TransformationType::CAST,
        TransformationType::COMPUTED,
        TransformationType::AGGREGATION,
        TransformationType::ANONYMIZATION,
        TransformationType::ENRICHMENT,
        TransformationType::CUSTOM,
    }};
    uint64_t idx = kCanonicalRngSeed;

    for (auto _ : state) {
        const auto t = kTypes[idx++ % 8];
        auto s = transformationTypeToString(t);
        benchmark::DoNotOptimize(s.data());
    }
}
BENCHMARK(BM_TransformationType_StringConversion)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
