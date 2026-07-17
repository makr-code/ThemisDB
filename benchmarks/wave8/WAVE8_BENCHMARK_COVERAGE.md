# Wave 8 Benchmark Coverage

**Wave:** 8 — Post-Release Performance Reliability  
**Date:** 2026-07-17  
**Branch:** develop

---

## Overview

Wave 8 delivers the post-release performance reliability layer for ThemisDB, covering four
dimensions required for durable operational confidence after a production release:

| PR   | Benchmark File                                      | Purpose                                              |
|------|-----------------------------------------------------|------------------------------------------------------|
| W8-A | `bench_w8a_incident_regression_shielding.cpp`       | Incident-driven regression shielding (known patterns)|
| W8-B | `bench_w8b_threshold_hardening_drift_detection.cpp` | Hardened thresholds + drift detection                |
| W8-C | `bench_w8c_deterministic_ci_harness.cpp`            | Deterministic, low-noise CI harness                  |
| W8-D | `bench_w8d_operability_runbooks_ownership.cpp`      | Operability counters, ownership, policy compliance   |

---

## W8-A: Incident Regression Shielding

Scenarios derived from post-release incidents and near-misses:

| ID     | Benchmark Name                             | Incident Category             | Hard Gate                          |
|--------|--------------------------------------------|-------------------------------|------------------------------------|
| IRS-01 | `W8A/IRS01_BurstReadSpike_mean_250us_gate` | Read amplification under burst| Soft: mean ≤ 250 µs                |
| IRS-02 | `W8A/IRS02_WriteStorm_throughput_60k_gate` | Memtable stall (write storm)  | Hard: ≥ 60k ops/s (GATE-W8-04)    |
| IRS-03 | `W8A/IRS03_ReadAfterWrite_consistency_latency` | Near-stale read after flush | Diagnostic (no hard gate)          |
| IRS-04 | `W8A/IRS04_HotPrefixRangeScan_p99_400us_gate` | Hot-prefix scan regression | Soft: p99 ≤ 400 µs                 |
| IRS-05 | `W8A/IRS05_IngestWithConcurrentRead_p99_400us_gate` | Live ingest + read collision | Soft: p99 ≤ 400 µs          |
| IRS-06 | `W8A/IRS06_IndexRebuild_per_1k_latency`    | Index rebuild under load      | Diagnostic (no hard gate)          |
| IRS-07 | `W8A/IRS07_DeleteTombstone_read_overhead`  | Compaction debt / delete storm| Diagnostic (no hard gate)          |
| IRS-08 | `W8A/IRS08_LargePayload_mixed_bandwidth`   | Bandwidth saturation (16 KB)  | Diagnostic (no hard gate)          |

---

## W8-B: Threshold Hardening & Drift Detection

| ID     | Benchmark Name                                   | Gate Type      | Threshold (W8)   | Prior (W7)  |
|--------|--------------------------------------------------|----------------|------------------|-------------|
| THD-01 | `W8B/THD01_TightenedRead_p99_175us_gate`         | Hard (GATE-W8-01) | p99 ≤ 175 µs  | 200 µs      |
| THD-02 | `W8B/THD02_TightenedWrite_90k_ops_s_gate`        | Hard (GATE-W8-02) | ≥ 90k ops/s   | 80k ops/s   |
| THD-03 | `W8B/THD03_ReadThroughput_drift_detection`       | Soft (SGATE-W8-01) | CV ≤ 8%     | —           |
| THD-04 | `W8B/THD04_WriteThroughput_drift_detection`      | Soft (SGATE-W8-02) | CV ≤ 10%    | —           |
| THD-05 | `W8B/THD05_Latency_trend_slope`                  | Soft (SGATE-W8-03) | slope ≤ 0    | —           |
| THD-06 | `W8B/THD06_DeltaBaseline_comparison`             | Diagnostic       | delta ≤ 10%     | —           |
| THD-07 | `W8B/THD07_TightenedBatch_p99_4ms_gate`          | Hard (GATE-W8-03) | p99 ≤ 4 ms   | 5 ms        |
| THD-08 | `W8B/THD08_CompositeDrift_score`                 | Soft             | score = 1.0     | —           |

---

## W8-C: Deterministic CI Harness

| ID     | Benchmark Name                                    | Harness Invariant                        | Gate                        |
|--------|---------------------------------------------------|------------------------------------------|-----------------------------|
| DCH-01 | `W8C/DCH01_SeedReproducibility_zero_mismatch`     | Same seed → same key sequence            | Soft: mismatches = 0        |
| DCH-02 | `W8C/DCH02_TempDirIsolation_zero_collision`       | Unique temp path per fixture             | Soft: collisions = 0        |
| DCH-03 | `W8C/DCH03_IterStability_delta_pct`               | Normalised latency stable vs iter count  | Soft: delta ≤ 25%           |
| DCH-04 | `W8C/DCH04_WarmupEfficacy_improvement_pct`        | Post-warmup ≤ 120% of pre-warmup mean    | Soft: warmup_gate_passed=1  |
| DCH-05 | `W8C/DCH05_TimerResolution_sub_us_gate`           | steady_clock resolution ≤ 1 µs          | Soft: timer_sub_us = 1.0    |
| DCH-06 | `W8C/DCH06_ParallelIsolation_no_cross_state`      | Parallel fixtures share no state         | Diagnostic                  |
| DCH-07 | `W8C/DCH07_TeardownCompleteness_no_leaks`         | DB directory removed after TearDown      | Diagnostic                  |
| DCH-08 | `W8C/DCH08_FlakeDetection_cv_3pct_gate`           | CV across repetitions < 3%              | Soft (SGATE-W8-04): CV ≤ 3% |

---

## W8-D: Operability, Runbooks & Ownership

| ID     | Benchmark Name                                       | Operability Signal                        | Gate                            |
|--------|------------------------------------------------------|-------------------------------------------|---------------------------------|
| ORP-01 | `W8D/ORP01_TriageMetrics_completeness_1_0_gate`      | All required counters emitted             | Hard (GATE-W8-05): score = 1.0  |
| ORP-02 | `W8D/ORP02_RootCauseContext_annotations`             | Structured diagnostic annotations         | Diagnostic                      |
| ORP-03 | `W8D/ORP03_BeforeAfter_comparison_delta_pct`         | Before/after delta scaffold               | Soft: delta ≤ 10%               |
| ORP-04 | `W8D/ORP04_EscalationGate_failure_ack_ratio`         | Unacknowledged hard gate failures = 0     | Diagnostic                      |
| ORP-05 | `W8D/ORP05_OwnerAssignment_zero_unowned`             | All hot paths have an assigned owner      | Soft: unowned = 0               |
| ORP-06 | `W8D/ORP06_MaintenancePolicy_compliance_score`       | Threshold changes comply with policy      | Soft (SGATE-W8-08): score = 1.0 |
| ORP-07 | `W8D/ORP07_BaselineStaleness_fresh_gate`             | Baseline age ≤ 30 days                    | Soft (SGATE-W8-05): fresh = 1.0 |
| ORP-08 | `W8D/ORP08_GuardrailCoverage_score_80pct_gate`       | ≥ 80% of hot paths have gates             | Hard (GATE-W8-06): score ≥ 0.80 |

---

## Hard Gate Summary

| Gate ID       | Benchmark        | Metric              | Threshold  | Direction         |
|--------------|------------------|---------------------|------------|-------------------|
| GATE-W8-01   | THD-01           | latency_p99_us      | ≤ 175      | lower_is_better   |
| GATE-W8-02   | THD-02           | items_per_second    | ≥ 90 000   | higher_is_better  |
| GATE-W8-03   | THD-07           | latency_p99_ms      | ≤ 4        | lower_is_better   |
| GATE-W8-04   | IRS-02           | items_per_second    | ≥ 60 000   | higher_is_better  |
| GATE-W8-05   | ORP-01           | completeness_score  | = 1.0      | higher_is_better  |
| GATE-W8-06   | ORP-08           | coverage_score      | ≥ 0.80     | higher_is_better  |

---

## Coverage Gaps and Follow-Ups

The following areas are **not yet covered** by W8 benchmarks and are candidates for Wave 9:

| Area                           | Reason Not in W8                          | Priority |
|-------------------------------|-------------------------------------------|----------|
| Vector index compaction        | No incident history yet; to add in W9     | Medium   |
| AQL query regression           | Covered by separate AQL benchmark suite   | Low      |
| Distributed replication path   | Requires multi-node CI setup               | High     |
| Cold-cache warm-up time        | Simulated in DCH-04; full cold boot TBD   | Medium   |
| Compaction backlog measurement | Background I/O isolation needed           | Medium   |

---

## Tooling

| Tool                     | Path                                   | Purpose                                  |
|--------------------------|----------------------------------------|------------------------------------------|
| Gate reporter            | `benchmarks/wave8/report_variance_w8.py` | Variance, drift, and gate evaluation    |
| Gate manifest            | `benchmarks/wave8/release_gate_manifest_w8.json` | Hard/soft gate definitions     |
| Runbook                  | `benchmarks/wave8/RUNBOOK_W8.md`       | Execution SOP and escalation matrix      |
| Repro & triage guide     | `benchmarks/wave8/REPRO_TRIAGE_W8.md` | Minimal repro and owner routing          |
| CI experiments config    | `benchmarks/ci_wave8_experiments.json` | CI job configuration (to be added)       |

---

## Relationship to Prior Waves

| Wave | Status     | Relationship to W8                                    |
|------|-----------|-------------------------------------------------------|
| W5   | Complete  | W8 tightens W5 production-workload gates              |
| W6   | Complete  | W8 adds IRS coverage for W6 failure scenarios         |
| W7   | Complete  | W8 hardens all W7 hard gates by 10–20%                |
| W8   | Active    | Post-release reliability, drift detection, operability |
