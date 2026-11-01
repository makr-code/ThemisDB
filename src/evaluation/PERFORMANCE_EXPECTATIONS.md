# Evaluation Module Performance Expectations

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

> **Evidence status (2026-10-31):** Numeric values and gate language in this
> document are proposed expectations, not measured baselines, verified SLAs, or
> proof of a release gate. The source under `benchmarks/evaluation/` uses
> in-process stubs and does not measure evaluation implementations. The separate
> `benchmarks/epic2_evaluation/` suite is conditionally registered and has
> distinct harnesses: the planner programs report percentiles for in-process
> `selectPath()` calls, the matrix program includes synthetic workloads explicitly
> marked non-release, and storage placement calls
> `DefaultShardPlacementStrategy` with synthetic node inputs. The artifact-
> staleness source exercises `ArtifactLifecycleManager` with Google Benchmark
> but is not registered. No benchmark was run for issue #7344; none of these
> source capabilities constitutes measured evidence.

<!-- Historical status: proposed Phase 5 guardrails; validation dated 2026-08-18 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_EVIDENCE.md · benchmarks/epic2_evaluation/README.md -->

## Current Evidence

The EPIC 2 CMake file returns unless benchmarks are enabled. Planner decision
and error-path executables additionally require `epic2_evaluation_lib`; the
matrix target requires Google Benchmark and `themis_evaluation`; storage source
is passed to the standard benchmark helper and its wrapper is registered if the
target exists. The artifact-staleness source is not registered. The
`BASELINES.md` tables are blank. The 2026-08 vcpkg/RocksDB blocker recorded in
`MODULE_EVIDENCE.md` is historical and does not establish the current build
environment.

The EV-BM-01..04 benchmark under `benchmarks/evaluation/` measures stub
functions only. It cannot validate the planner, matrix, lifecycle or metric
implementation targets described below. This is separate from the EPIC 2
planner harnesses, which call `selectPath()` with fixed in-process inputs and
report p50/p95/p99, and the matrix/storage harnesses, which have their own
synthetic workload semantics. EPIC 2's unregistered
`artifact_staleness_bench.cc` exercises `ArtifactLifecycleManager` through
Google Benchmark; the storage benchmark calls the actual
`DefaultShardPlacementStrategy`, but supplies synthetic node inputs rather than
a live cluster.

## Phase-Gated Performance Expectations

### Phase 3-4 (behavior + tests)
- establish deterministic correctness and stability baselines for EPIC 2 contracts
- validate planner and policy semantics with contract-focused regressions

### Phase 5 (performance hardening) — historical proposed targets
- define latency/throughput budgets for planner and evaluation paths (proposed)
- define storage and hardware-profile evaluation budget expectations (proposed)
- lock benchmark matrices for representative workload classes (proposed)
- capture measured baselines and fallback-rate guardrails (the 2026-08 snapshot
  records this as blocked; current environment status is unknown)

### Phase 6-7 (acceptance + integration)
- publish only benchmark-backed performance claims
- enforce regression gates before default integration
- keep evidence blockers explicit when benchmark execution is not possible in the current environment

---

## Phase 5 Performance Targets (Proposed; Not Measured)

### 1. Planner Decision Latency — proposed target, not measured

**What:** Wall-clock time for `QueryPlanner::selectPath()` in the fixed path-selection scenarios implemented by `planner_decision_bench.cc`.

**Targets (latency budget for ADR E2-003 SLA):**
- **p50:** ≤ 100 µs (fast-path typical case)
- **p95:** ≤ 500 µs (elevated policy evaluation)
- **p99:** ≤ 1000 µs (pathological edge case with all safety checks active)

**Measurement strategy:**
- The source runs six fixed eligibility/freshness cases (ANN-only,
  ANN-plus-tensor, stale-tensor fallback, force-exact, module-gap fallback, and
  distributed selection), repeats `QueryPlanner::selectPath()` calls, and
  reports avg/p50/p95/p99/min/max for each case. It has no direct Path 3 latency
  case and does not execute downstream ANN, tensor, graph, or distributed
  retrieval.
- Its final nonzero regression check compares maximum p95 with 800 µs. It prints
  p99 and a 1 ms limit but does not fail on p99. These source-level checks are
  not measured results or a verified release gate.
- Capture actual results on representative hardware (proposed class: 2+ GHz
  Intel/ARM CPU, 16GB+ RAM) before comparing them to the proposed targets.

**Proposed criteria, not established by this harness:** p95 ≤ 500 µs and p99
≤ 1000 µs. The source's implemented failure threshold is p95 > 800 µs; its p99
limit is printed but not enforced.

**Environment dependency:** Linux x86_64 with GCC 11+ / Clang 14+ (no GPU required).

---

### 2. Planner Fallback Rate Benchmark — proposed target, not measured

**What:** Fraction of planner decisions that fall back from preferred path due to module gap thresholds or eligibility mismatches.

**Targets (fallback rate boundaries):**
- **Category A (Safety/GA-critical modules):** < 5% fallback rate
  - Blocked by module gap thresholds in gpu_error_handling, gpu_parity_validation, query_exception_handling.
- **Category B (Performance enhancement):** < 2% fallback rate
  - Blocked by missing distributed manifests, shard availability.
- **Category C (Error handling enforcement):** < 1% fallback rate
  - Forced exact downgrade, force_exact flag, resource exhaustion.

**Measurement strategy:**
- `planner_decision_bench.cc` runs a fixed set of eligibility/freshness cases;
  its observer counts selected paths and `ModuleGapThreshold` decisions. It does
  not sample workflow distributions or report per-scenario fallback rates.
- Do not infer production fallback rates from these fixed in-process cases.
  Define representative workloads and capture their results separately before
  evaluating the proposed rate boundaries.

**Success criteria:**
- Scenario 1 (full): ≤ 2% fallback rate.
- Scenario 2 (GPU unavailable): ≤ 8% fallback rate (higher expected).
- Scenario 3 (distributed): ≤ 5% fallback rate.
- Scenario 4 (stale/rebuild): ≤ 10% fallback rate (intentional degradation).
- Scenario 5 (module gaps): ≤ 15% fallback rate (gap-remediation in progress).
- No fallback category should dominate unexpectedly (no single reason > 50% of total fallbacks).

**Environment dependency:** Linux x86_64, 16GB+ RAM; GPU optional (test both with and without).

---

### 3. Benchmark Matrix Throughput — proposed target, not measured

**What:** Scenario × dimension matrix fill and query throughput under synthetic representative workloads.

**Targets (throughput for matrix operations):**
- **Matrix record() operation:** ≥ 1M records/second
  - Single matrix[scenario][dimension] insertion.
- **Matrix lookup() operation:** ≥ 500K lookups/second
  - Single matrix[scenario][dimension] retrieval with cache-friendly access.
- **Full matrix scan (all scenarios + dimensions):** ≥ 50K full-fills/second
  - Populate all ~16×14 = ~224 entries per fill.
- **Scenario slice retrieval:** ≥ 100K slice-queries/second
  - Retrieve all dimensions for one scenario.

**Measurement strategy:**
- Run `benchmark_matrix_bench.cc` through Google Benchmark for its
  `BenchmarkMatrix` operations. The source also contains synthetic HNSW,
  DiskANN, tensor, graph, commit, and routing simulations and labels them
  non-release signals. Google Benchmark output is not p50/p95/p99 release
  evidence for those underlying systems.
- Capture time/operation and ops/sec for each actual matrix operation; evaluate
  synthetic simulation cases only as smoke/overhead measurements, not as
  production retrieval performance.

**Success criteria:**
- record() avg ≤ 1000 ns/op (1M ops/s equivalent).
- lookup() avg ≤ 2000 ns/op (500K ops/s equivalent).
- Full fill avg ≤ 5000 ns/op (200K ops/s for batch).
- Regression threshold: if any operation avg > 2x baseline, investigate.

**Environment dependency:** Linux x86_64 with C++17 support; no GPU required.

---

### 4. Artifact Staleness Detection Overhead — proposed target; source not registered in EPIC 2 CMake

**What:** Incremental latency overhead introduced by artifact staleness state computation and batch detection.

**Targets (latency budget for lifecycle management):**
- **Single artifact state computation:** ≤ 50 µs overhead
  - `ArtifactLifecycleManager::computeState()` with full policy.
- **Batch staleness detection (10 artifacts):** ≤ 500 µs total
  - Amortized: ≤ 50 µs per artifact in batch.
- **Batch staleness detection (100 artifacts):** ≤ 4000 µs total
  - Amortized: ≤ 40 µs per artifact at scale.
- **Rebuild identification overhead:** ≤ 10 µs per artifact
  - Filter-and-rebuild-select operation.

**Measurement strategy (proposed; no current target):**
- The source `benchmarks/epic2_evaluation/artifact_staleness_bench.cc` exists,
  but `benchmarks/epic2_evaluation/CMakeLists.txt` does not register it. The
  source uses Google Benchmark to exercise `ArtifactLifecycleManager` methods,
  including state computation and batch filtering, but do not describe it as a
  CMake-registered runnable target until wired.
- The source currently includes single-artifact cases and batches of 100 and
  1000 records with selected policies/states. Add other sizes or workload
  distributions only as an explicit benchmark change; do not imply they are
  already measured.

**Success criteria:**
- Proposed target: single artifact with full policy ≤ 50 µs at p95.
- Batch of 100 artifacts amortized ≤ 50 µs per artifact.
- No quadratic behavior (if 10 artifacts = 500 µs, 100 artifacts must be ≤ 5000 µs, not 50,000 µs).
- Regression threshold: if any amortized time > 1.5x baseline.

**Environment dependency:** Linux x86_64; no GPU required.

---

### 5. Storage Strategy Placement Efficiency — proposed target; run evidence unavailable

**What:** Time and memory overhead for distributed placement strategy computation and optimization.

**Targets (placement computation budget):**
- **Single placement computation:** ≤ 500 µs
  - `DefaultShardPlacementStrategy::compute_placement()` for 32-shard artifact.
- **Placement validation:** ≤ 100 µs
  - `validate_placement()` check against constraint.
- **Placement optimization:** ≤ 200 µs
  - `optimize_placement()` rebalancing pass.
- **Total end-to-end placement cycle (compute + validate + optimize):** ≤ 800 µs
  - Amortized per 32-shard artifact.

**Measurement strategy (proposed):**
- The EPIC 2 CMake file passes `storage_strategy_bench.cc` to the standard
  benchmark helper under target name
  `bench_epic2_evaluation_storage_strategy_bench`, when benchmark configuration
  and the helper's prerequisites allow target creation. This is separate from
  the quantization/mmap/zero-copy assessment tracked by EPIC #5436, issue #5443,
  whose `StorageStrategyAssessor` implementation is under `distributed_tensor`.
- The source calls the actual `DefaultShardPlacementStrategy` for compute,
  validation and optimization, but constructs synthetic node inputs; this is
  not a live-cluster performance measurement or a measurement of
  `StorageStrategyAssessor`. No completed benchmark run/result is recorded.
- Test with representative shard topologies:
  - Node count: 4, 12, 24 (small, medium, large cluster).
  - Constraint modes: NO_CONSTRAINT, ACCELERATOR_PREFERRED, ACCELERATOR_REQUIRED.
  - Artifact size: 16 MB (typical mid-layer tensor).
- Measure each operation and full cycle separately.
- Report time/op and memory overhead (estimate via sample profiling).

**Success criteria:**
- Single operation ≤ 500 µs (p95 target).
- Full cycle ≤ 800 µs (p95 target).
- No regression: if any operation avg > 2x baseline, investigate.
- Memory overhead < 5 MB per placement computation (temp state).

**Environment dependency:** Linux x86_64 with distributed_tensor available; GPU optional (ACCELERATOR tests only if CUDA available).

---

### 6. Planner Error Path Overhead — proposed target, not measured

**What:** Latency impact of Category C enforcement (forced exact downgrade, resource exhaustion handling).

**Targets (error handling budget):**
- **Fallback initiation overhead:** ≤ 50 µs
  - Time to detect error condition and trigger fallback.
- **Fallback completion overhead:** ≤ 100 µs
  - Time to set state and return to caller.
- **Total error path latency:** ≤ 150 µs
  - End-to-end error handling in worst case.

**Measurement strategy:**
- `planner_error_path_bench.cc` is a conditional EPIC 2 target when
  `epic2_evaluation_lib` exists. It times in-process `selectPath()` calls for a
  nominal case and fixed exception-handling, index-buffer, thread-safety, and
  compound-error inputs; it reports avg/p50/p95/p99 and compares an error p95
  with nominal p95.
- Its implemented check returns failure when the representative error p95
  exceeds 150 µs or its p95 overhead exceeds 50%. It does not exercise a
  resource-pool mock or downstream execution. These are harness checks, not
  captured results.

**Success criteria:**
- Error overhead ≤ 150 µs p95.
- Error paths must not be slower than nominal paths by > 50%.
- No cascading timeouts or deadlocks under error injection.

**Environment dependency:** Linux x86_64; no GPU required.

---

## Benchmark Work Items

- keep `benchmarks/epic2_evaluation/` aligned with issue #5428 workload classes
- map benchmark scenarios to hardware profile classes and policy modes
- maintain release-baseline tracking for phase-gate promotion
- preserve source-to-evidence traceability through `src/evaluation/MODULE_EVIDENCE.md`
- **[Phase 5]** document expected results and measurement methodology in `benchmarks/epic2_evaluation/README.md`
- **[Phase 5]** record reproducibility requirements (hardware class, data volume, shard configuration)
- **[Phase 5]** baseline measurements when environment allows vcpkg initialization

## Baseline Tracking & Regression Gates

- **Baseline capture proposal:** Run the relevant registered benchmarks on
  recorded reference hardware before making a release comparison.
- **Regression detection proposal:** Compare measured values against an
  accepted baseline; no alerting integration is established by this document.
- **Gate enforcement:** Any release-critical status must be confirmed from current root governance/CI configuration; this document alone does not establish gate enforcement.
- **Historical blocker:** The vcpkg/RocksDB validation issue recorded in `MODULE_EVIDENCE.md` is dated 2026-08 and must not be presented as the current environment state.

## Non-Goals (Current Stage)

- no production performance numbers are asserted yet without measured baseline evidence
- no optimization claims are made without benchmark artifacts
- no GPU-specific performance guarantees until CUDA environment is available
