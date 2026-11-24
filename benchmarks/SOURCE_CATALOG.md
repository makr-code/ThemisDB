---
Author: ThemisDB Contributors
Created: 2026-11-23
Last Updated: 2026-11-23
Source Level: 1
SOT Domain: module-behavior
Canonical Sources: benchmarks/BENCHMARK_STANDARDS.md; benchmarks/MEASUREMENT_HYGIENE.md; tools/ci/audit_benchmark_coverage.py
Related Milestone: DOC-WEEKLY-2026-48
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Benchmark source catalog and triage

This is the benchmark-source inventory and triage index, not a new measurement
standard. [`BENCHMARK_STANDARDS.md`](BENCHMARK_STANDARDS.md) and
[`MEASUREMENT_HYGIENE.md`](MEASUREMENT_HYGIENE.md) remain canonical.

## Snapshot and registration

Static audit of the current checkout on 2026-11-23. The worktree was dirty;
these counts describe the audited checkout snapshot, not a clean committed
revision or release evidence. Re-run the audit on a pinned revision before
publishing or using these counts for a release decision.

| Audit result | Count | Interpretation |
|---|---:|---|
| Discovered benchmark-named C++ sources | 389 | Recursive `benchmarks/**/*.cpp` filename-based discovery by `tools/ci/audit_benchmark_coverage.py` |
| Intentionally excluded from benchmark-tier coverage | 3 | One existing legacy exclusion and two diagnostic-only stubs (listed below) |
| Eligible benchmark sources | 386 | Discovered minus the three exclusions |
| Eligible sources with CMake target mappings | 386 | No unregistered eligible source in this audit |
| Eligible targets with benchmark CTest registration | 386 | 100% of this scoped denominator |
| Separate non-benchmark diagnostics | 2 | Both use ordinary CTest registration, not benchmark JSON smokes |
| Registered CTest entries | 405 | Entries can be multiple per target; this is not a source count |

The three exclusions are:

| Source | Treatment |
|---|---|
| `benchmarks/performance_optimizations/phase2/benchmark_phase2.cpp` | Existing intentional exclusion from default benchmark registration |
| `benchmarks/replication/bench_replication_dedicated_gates.cpp` | Stub-only diagnostic; separately tracked as an ordinary CTest target |
| `benchmarks/sharding/bench_sharding_dedicated_gates.cpp` | Stub-only diagnostic; separately tracked as an ordinary CTest target |

The registration result is not a semantic or performance-quality result.
CTest smoke registration proves only that the target is registered for the
smoke path; it does not prove a production code path, workload correctness,
threshold enforcement, release readiness, or reference-suite conformance.

## Module assignment gaps

The static matrix assigns a source only when its first path component matches
an immediate directory under `src/`. Of the 386 eligible sources, 335 meet
that naming rule and 51 do not. **The 51 paths below are not CMake-unregistered:**
the same audit found their targets and benchmark CTest registrations. This is
a source-to-module attribution gap, not a build-registration gap.

The `src/` directories with no assigned benchmark sources are:

- `src/ai_working/`
- `src/llm_l_wiki/`

Unassigned source paths:

```text
benchmarks/ann/bench_ann_cpu_gpu_dispatch.cpp
benchmarks/ann/bench_binary_quantization.cpp
benchmarks/ann/bench_hnsw_prefilter_minimal.cpp
benchmarks/ann/bench_hybrid_vector_geo.cpp
benchmarks/ann/bench_learned_quantization.cpp
benchmarks/ann/bench_lossy_vs_lossless.cpp
benchmarks/ann/bench_product_quantization.cpp
benchmarks/ann/bench_residual_quantization.cpp
benchmarks/ann/bench_vector_compression_lossless.cpp
benchmarks/ann/bench_vector_prefilter.cpp
benchmarks/ann/bench_vector_search.cpp
benchmarks/epic1_retrieval/bench_ann_distance_cpu_vs_flat.cpp
benchmarks/epic1_retrieval/bench_multishard_exact.cpp
benchmarks/mmdb/bench_mmdb.cpp
benchmarks/monitoring/bench_cycle_metrics.cpp
benchmarks/performance_optimizations/bench_phase3_optimization.cpp
benchmarks/performance_optimizations/bench_v1_3_0_features.cpp
benchmarks/performance_optimizations/bench_v1_3_4_optimizations.cpp
benchmarks/performance_optimizations/benchmark_huge_pages.cpp
benchmarks/performance_optimizations/benchmark_lirs_cache.cpp
benchmarks/performance_optimizations/benchmark_mimalloc.cpp
benchmarks/performance_optimizations/benchmark_rcu_index.cpp
benchmarks/performance_optimizations/benchmark_safe_fail.cpp
benchmarks/phase0/bench_p0_crud_baseline.cpp
benchmarks/tpc/bench_tpcc.cpp
benchmarks/tpc/bench_tpch.cpp
benchmarks/wave2/bench_w2a_mixed_path.cpp
benchmarks/wave2/bench_w2b_concurrency.cpp
benchmarks/wave2/bench_w2c_recovery.cpp
benchmarks/wave4/bench_w4a_release_gates.cpp
benchmarks/wave4/bench_w4b_resilience.cpp
benchmarks/wave4/bench_w4c_determinism.cpp
benchmarks/wave4/bench_w4d_diagnostics.cpp
benchmarks/wave5/bench_w5a_production_workloads.cpp
benchmarks/wave5/bench_w5b_degradation.cpp
benchmarks/wave5/bench_w5c_determinism.cpp
benchmarks/wave5/bench_w5d_governance.cpp
benchmarks/wave7/bench_p2_d05_compression_state_store.cpp
benchmarks/wave7/bench_w7a_release_critical_signoff.cpp
benchmarks/wave7/bench_w7b_endurance_soak.cpp
benchmarks/wave7/bench_w7c_degradation_fault_recovery.cpp
benchmarks/wave7/bench_w7d_guardrails_variance_operability.cpp
benchmarks/wave8/bench_w8a_incident_regression_shielding.cpp
benchmarks/wave8/bench_w8b_threshold_hardening_drift_detection.cpp
benchmarks/wave8/bench_w8c_deterministic_ci_harness.cpp
benchmarks/wave8/bench_w8d_operability_runbooks_ownership.cpp
benchmarks/wave9/bench_w9a_security_overhead_audit.cpp
benchmarks/wave9/bench_w9b_sla_measurement_compliance.cpp
benchmarks/wave9/bench_w9c_chaos_fault_recovery.cpp
benchmarks/wave9/bench_w9d_multi_tenant_isolation.cpp
benchmarks/ycsb/bench_ycsb.cpp
```

Regenerate the per-source target, CTest, module, and evidence matrix from the
current checkout; do not treat a historical snapshot as current:

```powershell
python tools/ci/audit_benchmark_coverage.py --repo-root .
python tools/ci/audit_benchmark_coverage.py --repo-root . `
  --module-matrix "$env:TEMP\themisdb-benchmark-module-matrix.json"
```

The generated matrix lists each discovered source under its module row or the
unassigned list and includes its target and CTest registrations. Its workload
evidence fields are not populated by registration discovery.

## Workload classification and triage

Assign one primary class per source only after inspecting the measured path,
not from its directory or filename:

| Class | Required evidence |
|---|---|
| `micro` | Isolated operation/kernel with no production service or multi-component path; label simulations separately |
| `component` | Calls a production component API and verifies the operation/result at that boundary |
| `integration` | Verifies a real path across multiple production components |
| `e2e` | Exercises a real application/server boundary and validates client-visible results |
| `reference_workload` | Pinned upstream driver/protocol and dataset/query/ground-truth provenance with verified semantics; a familiar suite name is insufficient |

Current source-backed examples and triage:

| Source / artifact | Triage class or issue | Action / limitation |
|---|---|---|
| `benchmarks/phase0/bench_p0_crud_baseline.cpp` | Simulated in-process micro workload | Uses a local `unordered_map` mock, ignores several operation results, and has no readback/state assertions. Its profile is explicitly `simulation-only`; do not use for production performance claims. |
| `benchmarks/mmdb/bench_mmdb.cpp` | Component | Uses RocksDB and `SecondaryIndexManager`, checks expected query/edge results, and labels cases `component`. Source evidence is not a benchmark run or a release baseline. |
| `benchmarks/standard_benchmarks_integration.py` and `benchmarks/ycsb/README.md` | E2E, YCSB-inspired only; outside the 389 C++ source denominator | Real local HTTP Entity operations and readback are documented, but concurrency is serial, workload semantics differ from upstream YCSB, and `external_comparison_ready` is false. |
| `benchmarks/core_http_mix.py` | E2E: `themisdb:e2e/core-http-crud-mix@1`; outside the C++ denominator | Equal disjoint-key CRUD, bounded closed-loop concurrency, final-state checks and owned cleanup. No transaction or durability claim. |
| `benchmarks/core_http_aql_index.py` | E2E: `themisdb:e2e/core-http-aql-index-equality@1`; outside the C++ denominator | Serial indexed equality lookups and validated results; the reported plan is not a physical I/O trace. |
| `benchmarks/chimera/bench_chimera_dedicated_gates.cpp` | Stub/simulation; diagnostic quality concern | `RouterStub` and `stubMerge` measure in-process stand-ins, not production router/adapter paths. Thresholds in comments are not evidence of production behavior. |
| `benchmarks/acceleration/bench_acceleration_dedicated_gates.cpp` | Stub/simulation; diagnostic quality concern | Source explicitly says in-process stubs and no GPU/CUDA/HIP runtime; labels state gate values but do not validate production GPU paths. |
| `benchmarks/analytics/bench_analytics_release_gates.cpp` | Mixed component/synthetic cases; weak gate | ARG-06 is explicitly a forecast-inference stub with no model I/O. `SetLabel` text alone does not enforce the displayed latency limits. |
| `benchmarks/config/README.md` and its referenced benchmark sources | Placeholder / weak gate | The document identifies a metrics-placeholder case and says scan labels do not assert thresholds. |
| `benchmarks/prompt_engineering/README.md` | Synthetic implementation | `bench_rewrite_engine.cpp` uses local synthetic rewrite helpers rather than `RewriteEngine` / `RewriteRuleLoader`. |
| Excluded replication/sharding diagnostic sources | Stub-only, weak pass/fail behavior | Their local helpers print `WARN` or `PASS`; the shown `main()` returns zero. They are not benchmark-tier CTest smokes and must not be presented as enforced release gates. |
| `benchmarks/ycsb/`, `benchmarks/tpc/`, `benchmarks/ann/`, `benchmarks/ldbc/` | Reference-workload candidates, not conformance classifications | Pin/verify the actual reference runner, workload semantics and dataset evidence before classifying a result as `reference_workload`. |

These examples are triage findings, not a blanket classification of every
source in the generated matrix. The static coverage audit does not inspect
measured production calls or infer semantic class. Per-source classification
of the remaining sources is still open; do not fill it from naming heuristics.

## Workload identity and profile artifacts

[`workload_profiles.json`](workload_profiles.json) is the machine-readable
profile catalog; [`workload_profiles.schema.json`](workload_profiles.schema.json)
validates its required fields. IDs use the immutable
`themisdb:<class>/<domain-workload>@<major>` form. Keep an ID when configuration
changes do not alter workload semantics; create a new major ID for a semantic change. Numeric parameters
not established by source evidence are `null` or caller-configured, never
guessed. The current profiles are source descriptions only and contain no
performance targets.

[`benchmark_suite_map.json`](benchmark_suite_map.json) points to this catalog
and `workload_profile_ids` references profiles intersecting its binary filters.
The separate `related_http_workload_profile_ids` field describes thematic
relationships only: these Python workloads are not selected by those binary
filters or thereby scheduled in CI. Neither field changes suite selection.

Each current CRUD or AQL HTTP runner produces one distinct workload family and
adds its catalog `workload_id` to successful JSON reports. The ID identifies
semantics, not a particular configuration, build, server or run; retain the
report and full configuration separately. Older reports have no such field.
CTest case observations and MSVC normalized records and repeatability entries
use `observed_case_workload_links` in the suite map as their single exact,
case-sensitive mapping, validated against the profile catalog. Missing, invalid
or ambiguous mappings yield `null` without blocking the benchmark. Executable
entries have no case identity and retain `null`. Old reports remain readable.
No suite regex or approximate name matching supplies an identity.

On 2026-11-23, `--benchmark_list_tests=true` on the existing
`build-msvc-windows-bench/bin/bench_p0_crud_baseline.exe` listed the four
`BM_P0_{Insert,Read,Update,Delete}Heavy/min_time:1.000/iterations:10000/real_time`
names. The existing `bench_mmdb.exe` listed four
`MMDBFixture/<case>/1000/real_time` names, with cases IndexedProductLookup,
CategoryDocumentJoin, GraphOneHop and MultiModelContextAssembly. Registrations
in the corresponding source include `UseRealTime`, Phase0's MinTime and
Iterations, and MMDB's Arg(1000). These cached binaries were not proven built
from the current worktree. Only these exact observed forms are mapped; other
library versions, aggregate/repetition suffixes, changed registrations and
unverified aliases remain unknown. The link does not establish binary freshness
or prove unchanged workload semantics under a rebuilt binary with the same name.

Validate the catalog, references, documented HTTP coverage and mocked report
contracts without a live server or performance run (requires `jsonschema`
and the runners' Python dependencies):

```powershell
python -m unittest discover -s benchmarks/tests -p test_workload_profiles.py -v
python -m unittest discover -s benchmarks/tests -p test_core_http_mix.py -v
python -m unittest discover -s benchmarks/tests -p test_core_http_aql_index.py -v
```

