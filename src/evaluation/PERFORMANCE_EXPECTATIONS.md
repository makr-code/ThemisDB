# Evaluation Module Performance Expectations

<!-- Status: Phase 5 guardrails DEFINED | validated: 2026-08-18 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_EVIDENCE.md · benchmarks/epic2_evaluation/README.md -->

## Current Baseline

Evaluation module behavior is no longer documentation-first only: the repository now
contains benchmark entry points for planner decision overhead, benchmark-matrix
scenarios, artifact staleness, and storage-strategy follow-up paths. Phase 5 defines
explicit guardrails for these measurements. Measured baselines are blocked by build
environment (vcpkg/RocksDB gap, documented in `src/evaluation/MODULE_EVIDENCE.md`).

## Phase-Gated Performance Expectations

### Phase 3-4 (behavior + tests)
- establish deterministic correctness and stability baselines for EPIC 2 contracts
- validate planner and policy semantics with contract-focused regressions

### Phase 5 (performance hardening) — ✅ GUARDRAILS DEFINED
- define latency/throughput budgets for planner and evaluation paths ✅
- define storage and hardware-profile evaluation budget expectations ✅
- lock benchmark matrices for representative workload classes ✅
- capture measured baselines and fallback-rate guardrails (blocked by vcpkg)

### Phase 6-7 (acceptance + integration)
- publish only benchmark-backed performance claims
- enforce regression gates before default integration
- keep evidence blockers explicit when benchmark execution is not possible in the current environment

---

## Phase 5 Performance Guardrails (Defined)

### 1. Planner Decision Latency — GUARDRAIL

**What:** Wall-clock time for `QueryPlanner::selectPath()` operation across all five execution paths.

**Targets (latency budget for ADR E2-003 SLA):**
- **p50:** ≤ 100 µs (fast-path typical case)
- **p95:** ≤ 500 µs (elevated policy evaluation)
- **p99:** ≤ 1000 µs (pathological edge case with all safety checks active)

**Measurement strategy:**
- Run `planner_decision_bench.cc` with N=100,000 iterations per path.
- Compute percentiles over the per-call wall-clock times (captured in nanoseconds).
- Test on **representative hardware:** 2+ GHz Intel/ARM CPU, 16GB+ RAM.
- Report latency for each of the 5 paths separately:
  - Path 1 (ANN Only): expected ~50–150 µs
  - Path 2 (ANN + Tensor Summary): expected ~100–300 µs
  - Path 4 (Stale Tensor): expected ~80–250 µs
  - Path 4 (force_exact): expected ~75–200 µs
  - Path 5 (Distributed): expected ~200–500 µs

**Success criteria:** All paths achieve p95 ≤ 500 µs and p99 ≤ 1000 µs. Regression threshold: if any path p95 > 800 µs (>60% regression from baseline).

**Environment dependency:** Linux x86_64 with GCC 11+ / Clang 14+ (no GPU required).

---

### 2. Planner Fallback Rate Benchmark — GUARDRAIL

**What:** Fraction of planner decisions that fall back from preferred path due to module gap thresholds or eligibility mismatches.

**Targets (fallback rate boundaries):**
- **Category A (Safety/GA-critical modules):** < 5% fallback rate
  - Blocked by module gap thresholds in gpu_error_handling, gpu_parity_validation, query_exception_handling.
- **Category B (Performance enhancement):** < 2% fallback rate
  - Blocked by missing distributed manifests, shard availability.
- **Category C (Error handling enforcement):** < 1% fallback rate
  - Forced exact downgrade, force_exact flag, resource exhaustion.

**Measurement strategy:**
- Run `planner_decision_bench.cc` CountingObserver over 100,000 sampled decision contexts.
- Vary eligibility mixes to simulate real workflow distributions:
  - Scenario 1: Full eligibility (all modules available, CUDA enabled).
  - Scenario 2: GPU unavailable (cuda_available=false, requires fallback to CPU).
  - Scenario 3: Distributed multi-shard (manifests available).
  - Scenario 4: Stale tensor + artifact rebuild (forces fallback to fresh-exact).
  - Scenario 5: Mixed module gaps (index_buffer_safety_ok=false, etc.).
- Compute fallback rate as `(fallback_count / total_decisions) * 100%` per scenario.
- Record which `FallbackReason` category each fallback belongs to.

**Success criteria:**
- Scenario 1 (full): ≤ 2% fallback rate.
- Scenario 2 (GPU unavailable): ≤ 8% fallback rate (higher expected).
- Scenario 3 (distributed): ≤ 5% fallback rate.
- Scenario 4 (stale/rebuild): ≤ 10% fallback rate (intentional degradation).
- Scenario 5 (module gaps): ≤ 15% fallback rate (gap-remediation in progress).
- No fallback category should dominate unexpectedly (no single reason > 50% of total fallbacks).

**Environment dependency:** Linux x86_64, 16GB+ RAM; GPU optional (test both with and without).

---

### 3. Benchmark Matrix Throughput — GUARDRAIL

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
- Run `benchmark_matrix_bench.cc` via Google Benchmark with --benchmark_min_time=0.1.
- Profile each operation separately:
  - BM_Matrix_Record_* : measure single-entry insertion latency.
  - BM_Matrix_Lookup_* : measure single-entry retrieval latency.
  - BM_Matrix_FillAll_* : measure full-matrix population latency.
  - BM_Matrix_ScenarioSlice : measure per-scenario slicing.
- Report time/operation (ns) and ops/sec for each benchmark.
- Memory footprint should remain < 10 MB for the full matrix.

**Success criteria:**
- record() avg ≤ 1000 ns/op (1M ops/s equivalent).
- lookup() avg ≤ 2000 ns/op (500K ops/s equivalent).
- Full fill avg ≤ 5000 ns/op (200K ops/s for batch).
- Regression threshold: if any operation avg > 2x baseline, investigate.

**Environment dependency:** Linux x86_64 with C++17 support; no GPU required.

---

### 4. Artifact Staleness Detection Overhead — GUARDRAIL

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

**Measurement strategy:**
- Run `artifact_staleness_bench.cc` via Google Benchmark.
- Create 10, 50, 100 artifacts with varied freshness states:
  - Fresh (age < max_staleness_ms).
  - Stale (age > max_staleness_ms).
  - Rebuilding (rebuild_in_progress=true).
  - Delta-lag exceeded (delta_lag > threshold).
- Measure per-artifact and amortized times separately.
- Policy configurations:
  - EmptyPolicy: no thresholds (baseline).
  - WithAgeThreshold: single age check.
  - WithMultipleThresholds: full age/delta/residual/rank policy.

**Success criteria:**
- Single artifact with full policy ≤ 50 µs (achieved at p95).
- Batch of 100 artifacts amortized ≤ 50 µs per artifact.
- No quadratic behavior (if 10 artifacts = 500 µs, 100 artifacts must be ≤ 5000 µs, not 50,000 µs).
- Regression threshold: if any amortized time > 1.5x baseline.

**Environment dependency:** Linux x86_64; no GPU required.

---

### 5. Storage Strategy Efficiency — GUARDRAIL

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

**Measurement strategy:**
- Run `storage_strategy_bench.cc` via Google Benchmark.
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

### 6. Planner Error Path Overhead — GUARDRAIL (New)

**What:** Latency impact of Category C enforcement (forced exact downgrade, resource exhaustion handling).

**Targets (error handling budget):**
- **Fallback initiation overhead:** ≤ 50 µs
  - Time to detect error condition and trigger fallback.
- **Fallback completion overhead:** ≤ 100 µs
  - Time to set state and return to caller.
- **Total error path latency:** ≤ 150 µs
  - End-to-end error handling in worst case.

**Measurement strategy:**
- Run `planner_error_path_bench.cc` (new benchmark).
- Test error scenarios:
  - Query exception handling disabled (query_exception_handling_ok=false).
  - Index buffer safety check failed (index_buffer_safety_ok=false).
  - Thread safety check failed (query_thread_safety_ok=false).
  - Resource pool exhausted (simulated via mock).
- Measure decision latency under each error scenario.
- Compare against nominal path latency to isolate error overhead.

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

- **Baseline capture:** Run full benchmark suite on reference hardware (2+ GHz x86_64, 16GB+ RAM, Linux) before release.
- **Regression detection:** If measured value exceeds guardrail p95 by > 20%, trigger regression alert.
- **Gate enforcement:** Benchmark gates are `release_critical` and must remain green before GA.
- **Blocked evidence:** Current build environment lacks vcpkg/RocksDB initialization; measured baselines deferred (see `MODULE_EVIDENCE.md` justified gap).

## Non-Goals (Current Stage)

- no production performance numbers are asserted yet without measured baseline evidence
- no optimization claims are made without benchmark artifacts
- no GPU-specific performance guarantees until CUDA environment is available
