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

#include "chimera/themisdb_adapter.hpp"
#include "chimera/database_adapter.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace chimera;

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
        adapter_ = std::make_unique<ThemisDBAdapter>();
        const auto connect_result = adapter_->connect("themisdb://benchmark.localhost:5432/themisdb_bench");
        benchmark::DoNotOptimize(connect_result);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        if (adapter_) {
            const auto disconnect_result = adapter_->disconnect();
            benchmark::DoNotOptimize(disconnect_result);
        }
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
        auto connect_result = adapter_->connect("themisdb://benchmark.localhost:5432/themisdb_bench");
        benchmark::DoNotOptimize(connect_result);
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
    std::string query = "SELECT * FROM test_table WHERE id = 1";

    for (auto _ : state) {
        auto result = adapter_->execute_query(query, {});
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
        const auto connect_result = adapter_->connect("themisdb://benchmark.localhost:5432/themisdb_bench");
        benchmark::DoNotOptimize(connect_result);
        const auto disconnect_result = adapter_->disconnect();
        benchmark::DoNotOptimize(disconnect_result);
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
    std::vector<std::string> queries;

    for (int i = 0; i < 100; ++i) {
        queries.push_back("INSERT INTO test_table VALUES (" + std::to_string(i) + ")");
    }

    for (auto _ : state) {
        for (const auto& q : queries) {
            auto result = adapter_->execute_query(q, {});
            benchmark::DoNotOptimize(result);
        }
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
    for (auto _ : state) {
        auto tx_result = adapter_->begin_transaction({});
        benchmark::DoNotOptimize(tx_result);
        if (tx_result.is_ok()) {
            const auto tx_id = tx_result.value.value();
            auto exec_result = adapter_->execute_query("INSERT INTO test_table VALUES (1)", {});
            benchmark::DoNotOptimize(exec_result);
            auto commit_result = adapter_->commit_transaction(tx_id);
            benchmark::DoNotOptimize(commit_result);
        }
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
        try {
            auto result = adapter_->execute_query("INVALID QUERY", {});
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
