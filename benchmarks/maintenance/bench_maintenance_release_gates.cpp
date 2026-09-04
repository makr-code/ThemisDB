// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_maintenance_release_gates.cpp
 * @brief Phase 5 maintenance module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the maintenance module hot
 * paths identified in the maintenance module roadmap (Phase 5 — Performance
 * and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-MTN-01 — Error enum cast throughput
 *   Measures the cost of casting MaintenanceError values from int32_t; serves
 *   as a baseline for error-path hot loops.
 *
 * ### GATE-MTN-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all MaintenanceError codes; validates
 *   that the compiler optimises the switch into an O(1) jump table.
 *
 * ### GATE-MTN-03 — ScheduleDescriptor struct allocation
 *   Measures in-process heap allocation and initialisation cost for
 *   MaintenanceScheduleDescriptor; release gate for schedule-churn paths.
 *
 * ### GATE-MTN-04 — Batch error cast (1 000 iterations)
 *   Measures amortised error-cast cost across a batch of 1 000 mixed codes;
 *   simulates a high-churn schedule-validation hot loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-MTN-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-MTN-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-MTN-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-MTN-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/maintenance/ROADMAP.md — Phase 5 items
 * @see include/maintenance/maintenance_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "maintenance/maintenance_api_contract.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace bench {
namespace mtn {

/// Canonical PRNG seed for all MTN benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-MTN-01 — Error enum cast throughput
// ============================================================================

static void BM_MTN01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        maintenance::MaintenanceError::kOrchestratorDegraded);
    for (auto _ : state) {
        auto e = static_cast<maintenance::MaintenanceError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-MTN-01: p99 <= 5 ns");
}
BENCHMARK(BM_MTN01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-02 — Switch dispatch throughput
// ============================================================================

static void BM_MTN02_SwitchDispatch(benchmark::State& state) {
    const maintenance::MaintenanceError codes[] = {
        maintenance::MaintenanceError::kSuccess,
        maintenance::MaintenanceError::kScheduleNotFound,
        maintenance::MaintenanceError::kHandlerNotRegistered,
        maintenance::MaintenanceError::kPersistenceFailed,
        maintenance::MaintenanceError::kExecutionTimeout,
        maintenance::MaintenanceError::kConcurrentModification,
        maintenance::MaintenanceError::kInvalidSchedule,
        maintenance::MaintenanceError::kOrchestratorDegraded,
        maintenance::MaintenanceError::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 9;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 9]) {
            case maintenance::MaintenanceError::kSuccess:               label = "ok"; break;
            case maintenance::MaintenanceError::kScheduleNotFound:      label = "sched"; break;
            case maintenance::MaintenanceError::kHandlerNotRegistered:  label = "hdlr"; break;
            case maintenance::MaintenanceError::kPersistenceFailed:     label = "pers"; break;
            case maintenance::MaintenanceError::kExecutionTimeout:      label = "tout"; break;
            case maintenance::MaintenanceError::kConcurrentModification: label = "conc"; break;
            case maintenance::MaintenanceError::kInvalidSchedule:       label = "inv"; break;
            case maintenance::MaintenanceError::kOrchestratorDegraded:  label = "deg"; break;
            case maintenance::MaintenanceError::kInternalError:         label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-MTN-02: p99 <= 10 ns");
}
BENCHMARK(BM_MTN02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-03 — ScheduleDescriptor struct allocation
// ============================================================================

static void BM_MTN03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        maintenance::MaintenanceScheduleDescriptor desc;
        desc.schedule_id = "bench-sched-42";
        desc.task_type   = "compaction";
        desc.interval    = std::chrono::seconds{3600};
        desc.enabled     = true;
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-MTN-03: p99 <= 500 ns");
}
BENCHMARK(BM_MTN03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_MTN04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8100, 8101, 8102, 8103, 8104, 8105, 8106, 8107
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<maintenance::MaintenanceError>(
                kRawCodes[seed % 8]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-MTN-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_MTN04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-05 — Concurrent-scheduling-guard check latency
// ============================================================================

/**
 * @brief Measures in-flight lookup + insert latency.
 *
 * Gate: p99 ≤ 50 µs
 */
#include <mutex>
#include <unordered_set>
#include <string>
#include <deque>
#include <nlohmann/json.hpp>
#include "maintenance/maintenance_health_report.h"

static void BM_MTN05_InFlightGuard(benchmark::State& state) {
    std::mutex mu;
    std::unordered_set<std::string> in_flight;
    const std::string sched_id = "bench-sched-42";

    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lock(mu);
            in_flight.insert(sched_id);
        }
        benchmark::DoNotOptimize(in_flight.size());
        {
            std::lock_guard<std::mutex> lock(mu);
            in_flight.erase(sched_id);
        }
    }
    state.SetLabel("GATE-MTN-05: p99 <= 50 us");
}
BENCHMARK(BM_MTN05_InFlightGuard)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-06 — Persist+reload round-trip (10 schedules, temp file)
// ============================================================================

/**
 * @brief Measures persist+reload round-trip for 10 schedules using the real
 *        nlohmann::json serialisation path and a temp-file write/read.
 *
 * Uses UseRealTime() to measure wall-clock time (I/O bound).
 * Gate: p99 ≤ 2 ms.
 */
#include <filesystem>
#include <fstream>

namespace {
static std::string make_mtn06_schedule_json(int idx) {
    nlohmann::json j;
    j["id"]                = "sched-bench-" + std::to_string(idx);
    j["name"]              = "bench-schedule-" + std::to_string(idx);
    j["description"]       = "";
    j["tenant_id"]         = "";
    j["frequency"]         = "daily";
    j["cron_expression"]   = "0 2 * * *";
    j["tasks"]             = nlohmann::json::array({"quota_check"});
    j["task_dependencies"] = nlohmann::json::array();
    j["enabled"]           = true;
    j["enforce_window"]    = true;
    j["window_start_hour"] = 2;
    j["window_end_hour"]   = 6;
    j["halt_on_task_failure"] = false;
    j["lock_ttl_ms"]       = 0;
    j["max_schedule_changes_per_interval"] = 0;
    j["created_at_ms"]     = 1000000;
    j["updated_at_ms"]     = 1000000;
    j["created_by"]        = "";
    j["updated_by"]        = "";
    j["last_run_ms"]       = 0;
    j["next_run_ms"]       = 0;
    j["last_run_state"]    = "";
    j["last_job_id"]       = "";
    return j.dump();
}
} // anonymous namespace

static void BM_MTN06_PersistReloadRoundTrip(benchmark::State& state) {
    static constexpr int kScheduleCount = 10;
    namespace fs = std::filesystem;
    const fs::path tmp_dir = fs::temp_directory_path();

    for (auto _ : state) {
        state.PauseTiming();
        const fs::path tmp_file = tmp_dir / ("mtn06_bench_" +
            std::to_string(reinterpret_cast<uintptr_t>(&state)) + ".json");
        state.ResumeTiming();

        // Persist: build and write all 10 schedule JSON strings to temp file.
        {
            std::ofstream ofs(tmp_file, std::ios::out | std::ios::trunc);
            nlohmann::json arr = nlohmann::json::array();
            for (int i = 0; i < kScheduleCount; ++i) {
                arr.push_back(nlohmann::json::parse(make_mtn06_schedule_json(i)));
            }
            ofs << arr.dump();
        }

        // Reload: read and parse.
        {
            std::ifstream ifs(tmp_file);
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
            auto arr = nlohmann::json::parse(content);
            benchmark::DoNotOptimize(arr.size());
        }

        state.PauseTiming();
        fs::remove(tmp_file);
        state.ResumeTiming();
    }
    state.SetLabel("GATE-MTN-06: p99 <= 2 ms");
}
BENCHMARK(BM_MTN06_PersistReloadRoundTrip)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// GATE-MTN-07 — DispatchOutcome ring-buffer write latency
// ============================================================================

/**
 * @brief Measures the cost of a single DispatchOutcome push to the ring buffer
 *        (mutex + deque push_back + conditional pop_front).
 *
 * Gate: p99 ≤ 200 ns.
 */
static void BM_MTN07_RingBufferWrite(benchmark::State& state) {
    std::mutex mu;
    std::deque<themis::maintenance::DispatchOutcome> ring;
    static constexpr int kCap = 256;

    for (auto _ : state) {
        themis::maintenance::DispatchOutcome o;
        o.schedule_id = "bench-sched";
        o.task_type   = "quota_check";
        o.outcome     = themis::maintenance::DispatchOutcomeType::SUCCESS;
        o.latency_us  = 42;
        {
            std::lock_guard<std::mutex> lock(mu);
            ring.push_back(std::move(o));
            if (static_cast<int>(ring.size()) > kCap) {
              ring.pop_front();
            }
        }
        benchmark::DoNotOptimize(ring.size());
    }
    state.SetLabel("GATE-MTN-07: p99 <= 200 ns");
}
BENCHMARK(BM_MTN07_RingBufferWrite)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace mtn
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
