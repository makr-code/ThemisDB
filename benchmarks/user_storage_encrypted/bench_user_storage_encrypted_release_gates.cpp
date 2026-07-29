// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_user_storage_encrypted_release_gates.cpp
 * @brief Phase 5 user_storage_encrypted module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the user_storage_encrypted
 * module hot paths identified in the module roadmap (Phase 5 — Performance
 * and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-USE-01 — Error enum cast throughput
 *   Measures the cost of casting UserStorageEncryptedError values from int32_t.
 *
 * ### GATE-USE-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all UserStorageEncryptedError codes.
 *
 * ### GATE-USE-03 — EncryptedMountDescriptor struct allocation
 *   Measures in-process heap allocation for EncryptedMountDescriptor; release
 *   gate for mount/unmount lifecycle hot paths.
 *
 * ### GATE-USE-04 — Batch error cast (1 000 iterations)
 *   Amortised error-cast cost across 1 000 mixed codes; simulates a
 *   high-frequency mount-lifecycle error classification loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-USE-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-USE-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-USE-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-USE-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/user_storage_encrypted/ROADMAP.md — Phase 5 items
 * @see include/user_storage_encrypted/user_storage_encrypted_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "user_storage_encrypted/user_storage_encrypted_api_contract.h"

#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace use {

/// Canonical PRNG seed for all USE benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-USE-01 — Error enum cast throughput
// ============================================================================

static void BM_USE01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        user_storage_encrypted::UserStorageEncryptedError::kBackendUnavailable);
    for (auto _ : state) {
        auto e = static_cast<user_storage_encrypted::UserStorageEncryptedError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-USE-01: p99 <= 5 ns");
}
BENCHMARK(BM_USE01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-USE-02 — Switch dispatch throughput
// ============================================================================

static void BM_USE02_SwitchDispatch(benchmark::State& state) {
    using E = user_storage_encrypted::UserStorageEncryptedError;
    const E codes[] = {
        E::kSuccess, E::kMountFailed, E::kUnmountFailed,
        E::kKeyDerivationFailed, E::kRotationFailed,
        E::kBackendUnavailable, E::kInvalidPath,
        E::kPermissionDenied, E::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 9;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 9]) {
            case E::kSuccess:             label = "ok"; break;
            case E::kMountFailed:         label = "mount"; break;
            case E::kUnmountFailed:       label = "unmount"; break;
            case E::kKeyDerivationFailed: label = "kdf"; break;
            case E::kRotationFailed:      label = "rot"; break;
            case E::kBackendUnavailable:  label = "bknd"; break;
            case E::kInvalidPath:         label = "path"; break;
            case E::kPermissionDenied:    label = "perm"; break;
            case E::kInternalError:       label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-USE-02: p99 <= 10 ns");
}
BENCHMARK(BM_USE02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-USE-03 — EncryptedMountDescriptor struct allocation
// ============================================================================

static void BM_USE03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        user_storage_encrypted::EncryptedMountDescriptor desc;
        desc.container_path = "/data/enc/bench-user-42";
        desc.mount_point    = "/mnt/bench-user-42";
        desc.read_only      = false;
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-USE-03: p99 <= 500 ns");
}
BENCHMARK(BM_USE03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-USE-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_USE04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8600, 8601, 8602, 8603, 8604, 8605, 8606, 8607
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<user_storage_encrypted::UserStorageEncryptedError>(
                kRawCodes[seed % 8]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-USE-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_USE04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace use
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
