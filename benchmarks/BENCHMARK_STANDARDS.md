---
Author: ThemisDB Contributors
Created: 2026-08-21
Last Updated: 2026-10-29
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

# ThemisDB Benchmark Standards

Exact model backend not verifiable from final document text alone.

Status: Canonical
Scope: All C++ benchmark files under benchmarks/ and related CMake/CTest wiring.

## 1. Purpose

This document defines binding standards for:
- Google Benchmark source structure and execution behavior
- CTest integration for benchmark smoke coverage where applicable
- Reproducibility and measurement hygiene baseline requirements

If another benchmark document conflicts with this file, this file wins for
benchmark structure and registration rules.

## 2. Google Benchmark Standard (Binding)

### 2.1 File Structure

- Benchmark files follow a consistent naming and fixture structure.
- Setup and teardown logic must be separated from measured loops.
- Shared helpers must be locally scoped to avoid global collisions.

### 2.2 Naming

- Benchmark names must express workload and scenario clearly.
- Keep naming consistent across modules for comparable workloads.
- Avoid aliases that obscure implementation path or dataset class.

### 2.3 Measurement Contract

- I/O-bound benchmarks must use wall-clock semantics (UseRealTime).
- benchmark_min_time must include an explicit unit (for example, 0.05s).
- Counters and processed-item metrics must be consistent and comparable.

### 2.4 Multi-Thread Rules

- No thread-unsafe finalization inside measured multi-thread loops.
- Shared mutable state must be synchronized or partitioned.
- Final aggregation belongs in fixture teardown or dedicated post-phase logic.

## 3. Reproducibility Standard (Binding)

- Use deterministic random seeds unless explicitly documented otherwise.
- Use unique temp paths for per-run artifacts.
- Keep warmup policy explicit and outside measured loops.
- JSON output mode should be available for baseline comparisons.

## 4. CMake and CTest Integration

- Benchmark targets must be built deterministically by preset/target mapping.
- CTest registration for benchmark smoke tests must use the canonical
  `themis_register_benchmark_ctest()` helper from
  `benchmarks/cmake/BenchmarkPolicy.cmake`.
- Direct `add_test()` + `set_tests_properties()` calls are **forbidden** in
  benchmark CMakeLists; they bypass `$<TARGET_FILE:...>` resolution and omit
  the mandatory label schema.
- Script-only benchmarks may remain outside CTest but must document runner flow.
- No orphaned CTest entries pointing to missing benchmark binaries; the helper
  already guards against this automatically.
- The `vscode-windows-benchmark-smokes` test preset runs only `tier:benchmark`
  tests serially. It is independent of the normal unit-test presets. Use the
  configure preset `vscode-windows-bench-release` and build preset
  `windows-bench-release`; local CTest runs use
  `ctest --preset vscode-windows-benchmark-smokes --output-on-failure` (no
  dashboard `-T test` mode).
- A CTest fixture archives prior output before the benchmark smokes and schedules
  `compare_expectations.py` as fixture cleanup. CTest runs that cleanup last,
  including when one or more benchmark smokes fail. The postprocessor's report
  failure is surfaced as a CTest failure as well.
- Each benchmark smoke writes `<CTest-name>.json` under
  `build/windows-bench-release/bench_results/current/` to avoid output
  collisions when a target has multiple smoke registrations. Previous result
  directories are preserved under `bench_results/archive/`; the final report is
  `benchmark_comparison_report.md` in the current run directory.
- This comparison step evaluates cases documented in
  `src/*/PERFORMANCE_EXPECTATIONS.md`. It does not execute or analyze ordinary
  unit tests, and an expectation missing from the JSON is reported as `NOT_RUN`.

### 4.1 Canonical Registration Pattern

```cmake
# Example: module-level smoke entry
themis_register_benchmark_ctest(
    NAME         bench_<module>_<workload>
    TARGET       bench_<module>_<workload>
    MODULE       "<module>"
    TIMEOUT      120           # seconds; adjust for long-running suites
    EXTRA_LABELS release_critical  # only when gate-critical
)
```

The helper enforces:
- `$<TARGET_FILE:TARGET>` resolution — CTest finds the binary regardless of
  output directory structure.
- Mandatory label set: `module:<module>;tier:benchmark;kind:smoke`.
- JSON output written to `${CMAKE_BINARY_DIR}/bench_results/current/<CTest-name>.json`.
- Standard `--benchmark_min_time=0.1s` unless overridden via `MIN_TIME`.
- Guard: the entry is silently skipped when the target is not available (avoids
  orphaned CTest entries when a benchmark is disabled by a feature flag).

## 5. Migration Rules for Existing Benchmarks

- Any touched benchmark file must be aligned to this standard in the same PR.
- Existing measurement docs should be retained as domain-specific addenda.
- Domain-specific docs may extend rules, but must not weaken this baseline.

## 6. Relationship to Measurement Hygiene

MEASUREMENT_HYGIENE.md remains a mandatory detailed companion for protocol,
warmup, and variance control. This file defines baseline standards; measurement
hygiene provides deeper execution detail.

## 7. Verification Checklist

- [ ] Naming and fixture structure are consistent
- [ ] Setup and measurement loops are cleanly separated
- [ ] benchmark_min_time uses explicit units
- [ ] I/O paths use real-time measurements where required
- [ ] CTest smoke registration is valid (if applicable)
- [ ] Reproducibility controls are documented and active

## 8. Resource And Runner Contract

`run_benchmarks_msvc.ps1` records executable-lifetime CPU time, wall time,
peak working set, peak commit, page faults and process I/O in
`resource_summary.csv`. These include setup, calibration and teardown, not
just the measured loop. They are not per-operation memory measurements.
Exit-time private bytes can be small after allocator cleanup and must not
be substituted for peak memory. Child processes and remote servers are excluded;
GPU memory, energy, physical disk I/O and persistent disk footprint are not measured.

- `run_manifest.json` records source revision, dirty-worktree flag, CMake-cache
  hash, selected executable/runner hashes, OS, CPU, RAM and requested execution
  parameters. A dirty-worktree flag is not a source snapshot: archive the patch
  and runtime dependency versions before publication. An empty binary selection
  returns exit code 2 rather than reporting a successful run.
- `-Filter` selects binaries; `-BenchmarkFilter` selects Google Benchmark cases.
  `-Repetitions` repeats cases. Repetition aggregates are not request percentiles.
- `-TimeoutSeconds` bounds each binary (default 300 seconds). A timeout is a
  failed measurement, not a slow successful sample. Increase the bound explicitly
  for deliberately long workloads.
- Exit code zero, a nonempty measurement array and no `error_occurred` entries
  are required for acceptance. Partial suites with failed/skipped cases are not
  accepted as complete suites. Missing hardware must be reported separately.
- Status/resource CSVs are checkpointed before the next binary starts. Raw JSON
  remains untouched; normalized `real_time_ns`/`cpu_time_ns` are in nanoseconds.
- Any failed suite makes the runner return a nonzero exit code, even when other
  suites succeed. The generated run README records accepted versus discovered suites.

Example resource/reproducibility check, not an external database comparison:

```powershell
.\benchmarks\run_benchmarks_msvc.ps1 -BuildRoot .\build-msvc-windows-release `
  -Filter '^bench_importer_throughput\.exe$' `
  -BenchmarkFilter '^BM_Importer_Postgres/10000/0/1$' `
  -MinTime 0.1s -Repetitions 3 -TimeoutSeconds 60
```

## 9. Cross-Database Comparability

Google Benchmark standardizes the measurement harness, not database semantics.
The MSVC runner therefore labels results `internal_component_benchmark` and
`external_comparison_ready=false`. Neither `bench_ycsb` (YCSB Lite) nor SIMD,
tensor or distance-kernel timings establish end-to-end database rankings.

| Domain | Reference workload | Evidence required before a comparable run |
|---|---|---|
| KV / serving | YCSB A-F | Pinned upstream client/workload revision; identical operation mixes, key distributions, record/field sizes and scan semantics; validated read/write success and durability |
| Vector index | ANN-Benchmarks | Identical dataset/query/ground-truth hashes, distance metric and k; recall@k versus throughput/latency; index build time and size; distinguish index-only from database access |
| Vector database | VectorDBBench | Pinned runner/dataset and matching client concurrency/filter semantics; recall@k, latency, throughput, ingestion/index build time and server resources |
| Analytics | ClickBench | Identical dataset/queries, result validation, engine configuration, cold/warm policy, per-query times, load time and storage footprint |
| Graph | LDBC SNB | Pinned workload variant/version, generator scale factor, updates/queries, driver and validation evidence; report deviations, not an unsupported compliance claim |

Every published comparison must archive:

- Harness, adapter, database version/build flags, source patch, binary hashes,
  dataset/query checksums, seeds and all database/index settings.
- CPU/GPU model and memory, RAM limit, storage/filesystem, OS, network topology,
  client/server placement, resource limits and comparable hardware allocations.
- Durability/consistency/isolation settings, compression, caches and cache state;
  separate load/index build, warmup, steady measurement and recovery phases.
- Actual concurrency, arrival model and duration; operation counts/errors,
  throughput and request-level p50/p95/p99 with explicit units. Never derive
  request quantiles from Google Benchmark means or repetition aggregates.
- Correctness/quality gates (including recall for ANN), repeated independent
  trials, dispersion/confidence intervals and complete failed/timeout evidence.
- Server process/container resource scope separate from the load generator;
  peak memory, CPU time, disk footprint and I/O definitions, plus GPU metrics
  where applicable. Unavailable metrics remain explicitly unavailable, not zero.
- Upstream version/license/provenance and any publication restrictions reviewed
  for the exact artifacts used; raw result and configuration artifacts retained.

Match each reference workload's prescribed measurement protocol. The 0.1-second
runner default is a smoke/baseline setting, not a sufficient publication protocol.
Matching names or adding metadata does not make an adapter standard-conformant.
Reference-driver adapters and validated external datasets remain open work in
`ROADMAP.md`; current local runs must not be presented as certified results.

The Python YCSB-inspired canonical HTTP path is documented in
[`ycsb/README.md`](ycsb/README.md). It requires real operation adapters and has no
sleep fallback or unverified industry-grade comparison. Its serial A/B/C/F
adapter validates reads and cleans up its own dataset, but uses uniform
keys, whole-record updates and non-atomic RMW. These are explicit deviations,
not an upstream-driver integration. Optional `--server-pid` captures local
server endpoint-memory snapshots and phase CPU/process-I/O deltas; Windows
high-water memory counters cover the whole process lifetime, not a workload.
Without explicit observation, server resources remain unavailable. Four short
Community Debug live smokes verified this path, not comparative performance.
