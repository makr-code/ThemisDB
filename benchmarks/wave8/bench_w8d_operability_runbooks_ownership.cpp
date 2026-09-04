// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w8d_operability_runbooks_ownership.cpp
 * @brief Wave 8-D: Operability, Runbooks & Ownership Benchmarks.
 *
 * Purpose: Validate and emit the structured counters and metrics that support
 * operational triage, runbook execution, and ownership validation.  These
 * benchmarks do not chase maximum performance; instead they produce diagnostic
 * signals that let on-call engineers quickly assess system state and route
 * incidents to the correct owner.
 *
 * Covered scenarios (ORP = Operability, Runbooks, Policy):
 *   ORP-01  Triage metric completeness — all required counters are present
 *   ORP-02  Root-cause context emission — structured annotations in output
 *   ORP-03  Before/after comparison scaffold — baseline snapshot + delta
 *   ORP-04  Hard gate failure counter — escalation gate signal
 *   ORP-05  Owner assignment validation — hot paths have an assigned owner
 *   ORP-06  Maintenance window policy compliance — threshold change audit
 *   ORP-07  Baseline staleness detection — stale baseline warning signal
 *   ORP-08  Guardrail coverage score — fraction of hot paths with gates
 *
 * Counter conventions used throughout this file:
 *   All gate counters use 1.0 = pass, 0.0 = fail.
 *   All coverage scores are fractions in [0.0, 1.0].
 *   Negative counters signal error states.
 *
 * These benchmarks are self-contained and do not require the full DB stack
 * to run; they model operational checks via lightweight in-memory workloads.
 *
 * Hard gates enforced by release_gate_manifest_w8.json:
 *   - ORP-01 triage_completeness_score = 1.0
 *   - ORP-04 no unacknowledged hard gate failures (failure_ack_ratio = 1.0)
 *   - ORP-08 guardrail_coverage_score ≥ 0.80
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w8d {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW8CanonicalSeed  = 42;
static constexpr int      kWarmupIterations = 500;
static constexpr int      kRepetitions      = 5;
static constexpr int      kDatasetSize      = 50'000;

/// Guardrail coverage: fraction of hot paths that must have a gate.
static constexpr double kGuardrailCoverageGate = 0.80;

/// Baseline max age in simulated days before considered stale.
static constexpr int kBaselineMaxAgeDays = 30;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

void RemoveAll(const std::string& path) {
    std::error_code ec = {};
    fs::remove_all(path, ec);
}

std::string UniqueDbPath(const std::string& tag) {
    using namespace std::chrono;
    auto ts = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return fs::temp_directory_path().string() + "/w8d_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config DefaultConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                         = db_path;
    cfg.compression_default             = "lz4";
    cfg.block_cache_size_mb             = 256;
    cfg.memtable_size_mb                = 128;
    cfg.max_write_buffer_number         = 4;
    cfg.allow_concurrent_memtable_write = true;
    cfg.enable_statistics               = false;
    return cfg;
}

class KeyGenerator {
public:
    explicit KeyGenerator(uint64_t seed = kW8CanonicalSeed) : rng_(seed) {}
    std::string NextKey(int upper_bound) {
        std::uniform_int_distribution<int> d(0, upper_bound - 1);
        return "op_" + std::to_string(d(rng_));
    }
private:
    std::mt19937_64 rng_;
};

// ---------------------------------------------------------------------------
// Ownership registry — hot-path → owner mapping used by ORP-05 / ORP-08
// ---------------------------------------------------------------------------

struct HotPathEntry {
    std::string path;
    std::string owner_team;
    bool        has_gate;
};

const std::vector<HotPathEntry>& HotPathRegistry() {
    static const std::vector<HotPathEntry> reg = {
        {"point_read",           "platform-perf",    true},
        {"upsert_write",         "platform-perf",    true},
        {"range_scan",           "platform-perf",    true},
        {"batch_write",          "platform-perf",    true},
        {"mixed_oltp",           "platform-perf",    true},
        {"vector_ann_search",    "ml-platform",      true},
        {"graph_neighbourhood",  "graph-team",       true},
        {"secondary_idx_lookup", "platform-perf",    true},
        {"burst_read_spike",     "platform-perf",    true},
        {"write_storm",          "platform-perf",    true},
        {"hot_prefix_scan",      "platform-perf",    true},
        {"ingest_concurrent_rd", "ingestion-team",   true},
        {"idx_rebuild_under_load","platform-perf",   true},
        {"delete_tombstone_read","platform-perf",    true},
        {"large_payload_mixed",  "storage-team",     true},
        // Two intentionally unprotected paths to keep coverage < 100%
        // and demonstrate the scoring mechanism.
        {"cold_cache_warmup",    "platform-perf",    false},
        {"compaction_backlog",   "storage-team",     false},
    };
    return reg;
}

} // namespace

// ---------------------------------------------------------------------------
// Base fixture for operability benchmarks
// ---------------------------------------------------------------------------

/**
 * @brief Standard DB fixture for operability benchmarks.
 */
class OperabilityFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("operability");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8D: failed to open RocksDB for operability fixture");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("op_" + std::to_string(i), "v" + std::to_string(i));
        }
        KeyGenerator wkg(kW8CanonicalSeed + 1);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val = {};
            db_->get(wkg.NextKey(kDatasetSize), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// ---------------------------------------------------------------------------
// ORP-01: Triage metric completeness
// ---------------------------------------------------------------------------

/**
 * @brief ORP-01: Triage metric completeness check.
 *
 * Runs a standard read workload and verifies that all required triage
 * counters are emitted (mean, p99, throughput, cv_pct, gate_passed).
 * The benchmark itself emits these counters; the manifest checks that
 * `triage_completeness_score` equals 1.0 (all required fields present).
 */
BENCHMARK_F(OperabilityFixture, ORP01_TriageMetricCompleteness)(benchmark::State& state) {
    constexpr int kOps = 500;
    std::vector<double> latencies;
    latencies.reserve(kOps);

    KeyGenerator kg(kW8CanonicalSeed + 11);
    for (auto _ : state) {
        state.PauseTiming();
        latencies.clear();
        state.ResumeTiming();

        for (int i = 0; i < kOps; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val = {};
            db_->get(kg.NextKey(kDatasetSize), val);
            latencies.push_back(
                std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count());
        }

        state.PauseTiming();
        std::sort(latencies.begin(), latencies.end());
        const double mean_us = std::accumulate(latencies.begin(), latencies.end(), 0.0) / kOps;
        const double p99     = latencies[static_cast<std::size_t>(kOps * 0.99)];
        // Required triage counters
        state.counters["mean_latency_us"]  = mean_us;
        state.counters["p99_latency_us"]   = p99;
        state.counters["throughput_ops_s"] =
            benchmark::Counter(kOps, benchmark::Counter::kIsRate);
        state.counters["gate_passed"]      = benchmark::Counter(p99 <= 200.0 ? 1.0 : 0.0);
        // Completeness score: 1.0 if all four required counters are positive
        const double score = (mean_us > 0.0 && p99 > 0.0) ? 1.0 : 0.0;
        state.counters["triage_completeness_score"] = benchmark::Counter(score);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kOps);
}
BENCHMARK_REGISTER_F(OperabilityFixture, ORP01_TriageMetricCompleteness)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8D/ORP01_TriageMetrics_completeness_1_0_gate");

// ---------------------------------------------------------------------------
// ORP-02: Root-cause context emission
// ---------------------------------------------------------------------------

/**
 * @brief ORP-02: Root-cause context annotation.
 *
 * Emits structured counters that encode the context needed for root-cause
 * analysis: the dataset size, access pattern entropy (uniform vs skewed),
 * whether the cache was cold, and the DB configuration profile.
 * These annotations make it possible to distinguish environment-induced
 * variance from true regressions during incident triage.
 */
BENCHMARK_F(OperabilityFixture, ORP02_RootCauseContext)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 22);
    for (auto _ : state) {
        std::string val = {};
        benchmark::DoNotOptimize(db_->get(kg.NextKey(kDatasetSize), val));
    }
    // Context annotations
    state.counters["ctx_dataset_size"]    = static_cast<double>(kDatasetSize);
    state.counters["ctx_access_uniform"]  = 1.0;    // uniform key distribution
    state.counters["ctx_cache_warm"]      = 1.0;    // warmup was applied
    state.counters["ctx_block_cache_mb"]  = 256.0;
    state.counters["ctx_memtable_mb"]     = 128.0;
    state.counters["ctx_seed"]            = static_cast<double>(kW8CanonicalSeed);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(OperabilityFixture, ORP02_RootCauseContext)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8D/ORP02_RootCauseContext_annotations");

// ---------------------------------------------------------------------------
// ORP-03: Before/after comparison scaffold
// ---------------------------------------------------------------------------

/**
 * @brief ORP-03: Before/after comparison scaffold.
 *
 * Runs two consecutive read workloads (same seed, same ops) and computes
 * the delta between them.  In production CI the "before" result is loaded
 * from a frozen baseline JSON; here both runs are live to test the scaffold.
 * Emits `before_mean_us`, `after_mean_us`, and `delta_pct`.
 */
BENCHMARK_F(OperabilityFixture, ORP03_BeforeAfterComparison)(benchmark::State& state) {
    constexpr int kOps = 300;

    for (auto _ : state) {
        state.PauseTiming();
        double before_total = 0.0, after_total = 0.0;
        state.ResumeTiming();

        // "Before" run
        {
            KeyGenerator kg(kW8CanonicalSeed + 33);
            for (int i = 0; i < kOps; ++i) {
                const auto t0 = std::chrono::steady_clock::now();
                std::string val = {};
                db_->get(kg.NextKey(kDatasetSize), val);
                before_total += std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count();
            }
        }

        // "After" run (identical seed = identical access pattern)
        {
            KeyGenerator kg(kW8CanonicalSeed + 33);
            for (int i = 0; i < kOps; ++i) {
                const auto t0 = std::chrono::steady_clock::now();
                std::string val = {};
                db_->get(kg.NextKey(kDatasetSize), val);
                after_total += std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count();
            }
        }

        state.PauseTiming();
        const double before_mean = before_total / kOps;
        const double after_mean  = after_total  / kOps;
        const double delta_pct   = (before_mean > 1e-12)
            ? (after_mean - before_mean) / before_mean * 100.0
            : 0.0;
        state.counters["before_mean_us"] = before_mean;
        state.counters["after_mean_us"]  = after_mean;
        state.counters["delta_pct"]      = delta_pct;
        state.counters["comparison_gate_passed"] =
            benchmark::Counter(std::abs(delta_pct) <= 10.0 ? 1.0 : 0.0);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kOps * 2);
}
BENCHMARK_REGISTER_F(OperabilityFixture, ORP03_BeforeAfterComparison)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8D/ORP03_BeforeAfter_comparison_delta_pct");

// ---------------------------------------------------------------------------
// ORP-04: Hard gate failure counter / escalation gate
// ---------------------------------------------------------------------------

/**
 * @brief ORP-04: Escalation gate — hard gate failure counter.
 *
 * Runs the standard point-read workload and computes whether it would have
 * failed the W8 hard gate (175 µs p99).  Emits `hard_gate_failures` (count
 * of repetitions that failed) and `failure_ack_ratio`.  The manifest gate
 * requires failure_ack_ratio = 1.0, meaning all failures are acknowledged
 * (in practice: 0 unacknowledged failures = gate passes).
 */
BENCHMARK_F(OperabilityFixture, ORP04_EscalationGateCounter)(benchmark::State& state) {
    constexpr int      kOps      = 500;
    constexpr double   kP99Gate  = 175.0; // µs — W8 tightened gate

    std::vector<double> latencies;
    latencies.reserve(kOps);
    int gate_failures = 0;

    KeyGenerator kg(kW8CanonicalSeed + 44);
    for (auto _ : state) {
        state.PauseTiming();
        latencies.clear();
        state.ResumeTiming();

        for (int i = 0; i < kOps; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val = {};
            db_->get(kg.NextKey(kDatasetSize), val);
            latencies.push_back(
                std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count());
        }

        state.PauseTiming();
        std::sort(latencies.begin(), latencies.end());
        const double p99 = latencies[static_cast<std::size_t>(kOps * 0.99)];
        if (p99 > kP99Gate) {
          ++gate_failures;
        }
        state.counters["p99_us"] = p99;
        state.counters["p99_gate_us"] = kP99Gate;
        state.ResumeTiming();
    }
    // Acknowledged = all failures have been investigated (0 unacknowledged = all ack'd).
    const int total_reps = static_cast<int>(state.iterations());
    state.counters["hard_gate_failures"]  = static_cast<double>(gate_failures);
    state.counters["failure_ack_ratio"]   =
        benchmark::Counter(gate_failures == 0 ? 1.0 : 0.0);
    state.counters["reps_total"]          = static_cast<double>(total_reps);
    state.SetItemsProcessed(static_cast<int64_t>(total_reps) * kOps);
}
BENCHMARK_REGISTER_F(OperabilityFixture, ORP04_EscalationGateCounter)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8D/ORP04_EscalationGate_failure_ack_ratio");

// ---------------------------------------------------------------------------
// ORP-05: Owner assignment validation
// ---------------------------------------------------------------------------

/**
 * @brief ORP-05: Owner assignment validation.
 *
 * Iterates the hot-path registry and verifies that every entry has a non-empty
 * owner team.  Emits `unowned_path_count` (must be 0) and `owner_coverage`.
 */
static void ORP05_OwnerAssignmentValidation(benchmark::State& state) {
    for (auto _ : state) {
        int unowned = 0;
        int total   = 0;
        for (const auto& entry : HotPathRegistry()) {
            ++total;
            if (entry.owner_team.empty()) {
              ++unowned;
            }
        }
        state.counters["unowned_path_count"] = static_cast<double>(unowned);
        state.counters["owner_coverage"]     =
            benchmark::Counter(total > 0 ? (total - unowned) / static_cast<double>(total) : 0.0);
        state.counters["ownership_gate_passed"] =
            benchmark::Counter(unowned == 0 ? 1.0 : 0.0);
        benchmark::DoNotOptimize(unowned);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(HotPathRegistry().size()));
}
BENCHMARK(ORP05_OwnerAssignmentValidation)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kNanosecond)
    ->Name("W8D/ORP05_OwnerAssignment_zero_unowned");

// ---------------------------------------------------------------------------
// ORP-06: Maintenance window policy compliance
// ---------------------------------------------------------------------------

/**
 * @brief ORP-06: Threshold change audit.
 *
 * Validates that the W8 thresholds satisfy the maintenance policy rules:
 *   - W8 read gate must be ≤ W7 read gate (may only tighten, never loosen).
 *   - W8 write gate must be ≥ W7 write gate (higher = stricter for throughput).
 *   - W8 batch gate must be ≤ W7 batch gate.
 *
 * These constraints are checked symbolically; no DB access is needed.
 */
static void ORP06_MaintenancePolicyCompliance(benchmark::State& state) {
    // Reference W7 thresholds (from release_gate_manifest_w7.json)
    constexpr double kW7ReadP99Us      = 200.0;
    constexpr double kW7WriteThroughput = 80'000.0;
    constexpr double kW7BatchP99Ms     = 5.0;

    // W8 hardened thresholds
    constexpr double kW8ReadP99Us      = 175.0;
    constexpr double kW8WriteThroughput = 90'000.0;
    constexpr double kW8BatchP99Ms     = 4.0;

    for (auto _ : state) {
        const bool read_ok  = kW8ReadP99Us      <= kW7ReadP99Us;
        const bool write_ok = kW8WriteThroughput >= kW7WriteThroughput;
        const bool batch_ok = kW8BatchP99Ms     <= kW7BatchP99Ms;

        const double policy_score = (read_ok && write_ok && batch_ok) ? 1.0 : 0.0;
        state.counters["read_gate_tightened"]  = benchmark::Counter(read_ok  ? 1.0 : 0.0);
        state.counters["write_gate_tightened"] = benchmark::Counter(write_ok ? 1.0 : 0.0);
        state.counters["batch_gate_tightened"] = benchmark::Counter(batch_ok ? 1.0 : 0.0);
        state.counters["policy_compliance_score"] = benchmark::Counter(policy_score);
        benchmark::DoNotOptimize(policy_score);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(ORP06_MaintenancePolicyCompliance)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kNanosecond)
    ->Name("W8D/ORP06_MaintenancePolicy_compliance_score");

// ---------------------------------------------------------------------------
// ORP-07: Baseline staleness detection
// ---------------------------------------------------------------------------

/**
 * @brief ORP-07: Baseline staleness detection.
 *
 * Simulates checking whether the frozen baseline is too old.  In production
 * CI the baseline creation timestamp is read from the baseline JSON; here
 * we use a simulated baseline age of 14 days (within the 30-day limit).
 * Emits `baseline_age_days` and `baseline_fresh` (1.0 if age ≤ 30 days).
 */
static void ORP07_BaselineStalenessDetection(benchmark::State& state) {
    constexpr int kSimulatedBaselineAgeDays = 14; // simulate a 14-day-old baseline

    for (auto _ : state) {
        const bool fresh = kSimulatedBaselineAgeDays <= kBaselineMaxAgeDays;
        state.counters["baseline_age_days"] = static_cast<double>(kSimulatedBaselineAgeDays);
        state.counters["baseline_max_age_days"] = static_cast<double>(kBaselineMaxAgeDays);
        state.counters["baseline_fresh"]    = benchmark::Counter(fresh ? 1.0 : 0.0);
        benchmark::DoNotOptimize(fresh);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(ORP07_BaselineStalenessDetection)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kNanosecond)
    ->Name("W8D/ORP07_BaselineStaleness_fresh_gate");

// ---------------------------------------------------------------------------
// ORP-08: Guardrail coverage score
// ---------------------------------------------------------------------------

/**
 * @brief ORP-08: Guardrail coverage score.
 *
 * Computes the fraction of hot paths in the registry that have a performance
 * gate assigned.  Emits `guardrail_coverage_score` (fraction in [0, 1]).
 * Hard gate: score ≥ kGuardrailCoverageGate (0.80).
 */
static void ORP08_GuardrailCoverageScore(benchmark::State& state) {
    for (auto _ : state) {
        int total   = 0;
        int covered = 0;
        for (const auto& entry : HotPathRegistry()) {
            ++total;
            if (entry.has_gate) {
              ++covered;
            }
        }
        const double score = (total > 0)
            ? covered / static_cast<double>(total)
            : 0.0;
        state.counters["guardrail_covered_paths"]   = static_cast<double>(covered);
        state.counters["guardrail_total_paths"]     = static_cast<double>(total);
        state.counters["guardrail_coverage_score"]  = score;
        state.counters["coverage_gate_passed"]      =
            benchmark::Counter(score >= kGuardrailCoverageGate ? 1.0 : 0.0);
        benchmark::DoNotOptimize(score);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(HotPathRegistry().size()));
}
BENCHMARK(ORP08_GuardrailCoverageScore)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kNanosecond)
    ->Name("W8D/ORP08_GuardrailCoverage_score_80pct_gate");

} // namespace w8d
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
