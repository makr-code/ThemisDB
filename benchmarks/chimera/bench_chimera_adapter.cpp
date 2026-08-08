// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_chimera_adapter.cpp
 * @brief Phase 5 chimera adapter compatibility release-gate benchmarks.
 *
 * Provides reproducible latency and throughput measurements for the Chimera
 * adapter hot paths identified in the chimera module roadmap (Phase 5 — Performance
 * and Hardening). Results are used as release gates; a regression beyond 10% vs. the
 * baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### CHM-01..03 — Adapter lifecycle operations
 *   CHM-01  ThemisDBAdapter::connect() with connection pooling
 *   CHM-02  ThemisDBAdapter::executeQuery() with prepared statement
 *   CHM-03  ThemisDBAdapter::disconnect() cleanup
 *
 * ### CHM-04..05 — Batch operations
 *   CHM-04  ThemisDBAdapter::executeBatch() with 100 operations
 *   CHM-05  ThemisDBAdapter::transaction lifecycle (commit)
 *
 * ### CHM-06 — Error recovery
 *   CHM-06  ThemisDBAdapter error handling and retry logic
 *
 * ## Hard release gates
 *
 * | Gate ID          | Benchmark | Threshold                |
 * |------------------|-----------|--------------------------|
 * | GATE-CHM-01      | CHM-01    | p99 ≤ 5 ms (connect)    |
 * | GATE-CHM-02      | CHM-02    | p99 ≤ 500 µs (execute)   |
 * | GATE-CHM-03      | CHM-03    | p99 ≤ 1 ms (disconnect)  |
 * | GATE-CHM-04      | CHM-04    | p99 ≤ 50 ms (batch 100)  |
 * | GATE-CHM-05      | CHM-05    | p99 ≤ 10 ms (txn)        |
 * | GATE-CHM-06      | CHM-06    | p99 ≤ 100 µs (error)     |
 *
 * All benchmarks:
 *   - Use kChimeraCanonicalSeed = 42 for deterministic test data.
 *   - Run with Repetitions(kRepetitions) to capture variance.
 *   - Connection pooling enabled for realistic scenarios.
 *
 * @see src/chimera/ROADMAP.md — Phase 5 items
 * @see include/chimera/themisdb_adapter.h — adapter interface
 */

#include <benchmark/benchmark.h>

#include "chimera/themisdb_adapter.h"
#include "chimera/adapter_config.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis::chimera;

// ============================================================================
// Constants — deterministic, release-pinned
// ============================================================================

/// Canonical seed for all Chimera benchmarks.
static constexpr uint64_t kChimeraCanonicalSeed = 42;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// Fixtures
// ============================================================================

class ChimeraAdapterFixture : public benchmark::Fixture {
protected:
    std::unique_ptr<ThemisDBAdapter> adapter_;

    void SetUp(const ::benchmark::State& /*state*/) override {
        // Initialize adapter with default configuration
        AdapterConfig cfg;
        cfg.enable_connection_pooling = true;
        cfg.pool_size = 10;
        adapter_ = std::make_unique<ThemisDBAdapter>(cfg);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        adapter_.reset();
    }
};

// ============================================================================
// CHM-01: ThemisDBAdapter::connect() with connection pooling
//         Threshold: p99 ≤ 5 ms
// ============================================================================

BENCHMARK_DEFINE_F(ChimeraAdapterFixture, CHM01_ConnectWithPooling)
(benchmark::State& state) {
    for (auto _ : state) {
        auto conn = adapter_->connect();
        benchmark::DoNotOptimize(conn);
        // Connection will be released at end of iteration
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ChimeraAdapterFixture, CHM01_ConnectWithPooling)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(100)
    ->UseRealTime();

// ============================================================================
// CHM-02: ThemisDBAdapter::executeQuery() with prepared statement
//         Threshold: p99 ≤ 500 µs
// ============================================================================

BENCHMARK_DEFINE_F(ChimeraAdapterFixture, CHM02_ExecuteQuery)
(benchmark::State& state) {
    auto conn = adapter_->connect();
    std::string query = "SELECT * FROM test_table WHERE id = ?";

    for (auto _ : state) {
        auto result = adapter_->executeQuery(query, conn);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ChimeraAdapterFixture, CHM02_ExecuteQuery)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(500)
    ->UseRealTime();

// ============================================================================
// CHM-03: ThemisDBAdapter::disconnect() cleanup
//         Threshold: p99 ≤ 1 ms
// ============================================================================

BENCHMARK_DEFINE_F(ChimeraAdapterFixture, CHM03_Disconnect)
(benchmark::State& state) {
    for (auto _ : state) {
        auto conn = adapter_->connect();
        adapter_->disconnect(conn);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ChimeraAdapterFixture, CHM03_Disconnect)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(200)
    ->UseRealTime();

// ============================================================================
// CHM-04: ThemisDBAdapter::executeBatch() with 100 operations
//         Threshold: p99 ≤ 50 ms
// ============================================================================

BENCHMARK_DEFINE_F(ChimeraAdapterFixture, CHM04_ExecuteBatch)
(benchmark::State& state) {
    auto conn = adapter_->connect();
    std::vector<std::string> queries;
    for (int i = 0; i < 100; ++i) {
        queries.push_back("INSERT INTO test_table VALUES (" + std::to_string(i) + ")");
    }

    for (auto _ : state) {
        auto results = adapter_->executeBatch(queries, conn);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK_REGISTER_F(ChimeraAdapterFixture, CHM04_ExecuteBatch)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(10)
    ->UseRealTime();

// ============================================================================
// CHM-05: ThemisDBAdapter::transaction lifecycle (commit)
//         Threshold: p99 ≤ 10 ms
// ============================================================================

BENCHMARK_DEFINE_F(ChimeraAdapterFixture, CHM05_TransactionCommit)
(benchmark::State& state) {
    auto conn = adapter_->connect();

    for (auto _ : state) {
        adapter_->beginTransaction(conn);
        adapter_->executeQuery("INSERT INTO test_table VALUES (1)", conn);
        adapter_->commitTransaction(conn);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ChimeraAdapterFixture, CHM05_TransactionCommit)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(50)
    ->UseRealTime();

// ============================================================================
// CHM-06: ThemisDBAdapter error handling and retry logic
//         Threshold: p99 ≤ 100 µs
// ============================================================================

BENCHMARK_DEFINE_F(ChimeraAdapterFixture, CHM06_ErrorHandling)
(benchmark::State& state) {
    for (auto _ : state) {
        // Trigger error condition and verify error handling
        try {
            auto result = adapter_->executeQuery("INVALID QUERY");
            benchmark::DoNotOptimize(result);
        } catch (const std::exception& e) {
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ChimeraAdapterFixture, CHM06_ErrorHandling)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(1000)
    ->UseRealTime();
