# PERFORMANCE_EXPECTATIONS - src/tensor

## Scope

- Module: src/tensor
- This file defines measurable tensor module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/tensor/bench_tensor_fingerprint_graph.cpp
  - benchmarks/tensor/bench_tensor_fingerprint.cpp
  - benchmarks/tensor/bench_tensor_deduplication_manager.cpp
  - benchmarks/tensor/bench_tensor_release_gates.cpp
  - benchmarks/tensor/bench_tensor_dedicated_gates.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TENP-1 | tensor fingerprint graph insert/query/neighbour operations remain bounded | BM_TFG_Insert_Throughput, BM_TFG_Insert_SingleNode, BM_TFG_FindSimilar, BM_TFG_Neighbours |
| TENP-2 | tensor fingerprint graph concurrent-read and metadata/export paths remain bounded | BM_TFG_ConcurrentReads, BM_TFG_NodeCount, BM_TFG_ExportPersistedGraph |
| TENP-3 | tensor fingerprint fixture insert/similarity/storage-ratio paths remain bounded | FingerprintInsertFixture/BM_FingerprintInsert, FindSimilarFixture/BM_FindSimilar_100K, StorageReductionFixture/BM_StorageReductionRatio |
| TENP-4 | tensor dedup snapshot/replay throughput paths remain bounded | BM_TDM_SnapshotRestoreRoundTrip, BM_TDM_JournalReplayThroughput |
| TENP-5 | tensor release hot-path gates remain bounded | BM_TRNRG01..BM_TRNRG06 (TRNRG gates) |
| TENP-6 | tensor dedicated wave gates remain bounded | BM_TN_BM_01..BM_TN_BM_04 (TN-BM gates) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TENG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| TENG-2 | tensor hot-path p99 <= release threshold | p99 from mapped tensor benchmark cases |
| TENG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |
| TENG-4 | TRNRG and TN-BM gate suites both present in release artifacts | benchmark JSON + gate summary + command log |

## Tensor RoPE Performance Gates (Q4 2026 / Q1 2027)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TEN-ROPE-G1 | Recall@10 improves by >= 2.0% absolute on >= 1 production-like dataset | compare `none` vs profile (`fixed`/`relational`/`learned`) |
| TEN-ROPE-G2 | nDCG@10 regression is not worse than -0.5% on required evaluation set | paired-run delta |
| TEN-ROPE-G3 | TT rank remains non-inferior (<= +5%) at equal quality target | avg active rank / max rank |
| TEN-ROPE-G4 | p95 latency overhead remains bounded | <= +8% CPU-only, <= +5% accelerated |
| TEN-ROPE-G5 | profile mismatch is fail-closed with explicit diagnostics | CTest focused mismatch suite |

## Neurocognitive Bridge Gates (Q1 2027 / Q2 2027)

| Gate ID | Expectation | Measurement |
|---|---|---|
| BRAIN-REPLAY-DET-01 | replay path is deterministic under fixed seed/manifest | bitwise/metric-equivalence check across replay reruns |
| BRAIN-HOMEO-BOUND-01 | homeostasis policy keeps growth bounded without quality collapse | growth envelope + Recall/nDCG non-regression checks |
| BRAIN-ARBITER-DET-01 | route arbitration is deterministic for equal-confidence conflicts | repeated equivalent-input route selection consistency |
| BRAIN-SAFETY-CORE-01 | core safety envelope remains fail-closed and rollback-capable | fail-closed mismatch checks + rollback suite + hidden-fallback absence |

## Machine Dreaming Research Gates (Q2 2027)

| Gate ID | Expectation | Measurement |
|---|---|---|
| BRAIN-DREAM-ISOLATION | synthetic dream-mode execution remains isolated from production truth paths | sandbox/profile separation checks + forbidden path assertions |
| BRAIN-DREAM-LABEL-INTEGRITY | synthetic provenance labels persist end-to-end (`synthetic=true`, `dream_mode=true`) | label persistence checks across storage/retrieval/evaluation stages |
| BRAIN-DREAM-UTILITY | validated dream candidates provide measurable utility in downstream tasks | post-validation delta on Recall/nDCG/task-quality metrics |
| BRAIN-DREAM-NONREGRESSION | no increase in online hallucination/error rates in production profiles | production-profile hallucination/error nonregression suite |
| BRAIN-DREAM-ETHICS-BOUNDARY | dream-mode content remains policy-constrained by ethics checks | ethics-policy conformance and veto-path verification |

## Statistical Evidence Protocol (Neurocognitive Gates)

- latency/throughput metrics: >= 20 independent runs per condition.
- quality metrics (Recall/nDCG): >= 10 full-eval runs per condition.
- required reporting: mean, median, p95/p99, standard deviation, 95% confidence interval, effect size vs baseline.
- promotion rule: no statistically meaningful regression in mandatory operational gates.

## Benchmarking Protocol (Execution)

### 1) Configure + build benchmark targets

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target bench_tensor_release_gates bench_tensor_dedicated_gates
```

### 2) Run release gate benchmarks (TRNRG)

```powershell
.\build\windows-release\benchmarks\tensor\bench_tensor_release_gates.exe --benchmark_out=artifacts\benchmarks\tensor\trnrg.json --benchmark_out_format=json
```

### 3) Run dedicated tensor gates (TN-BM)

```powershell
.\build\windows-release\benchmarks\tensor\bench_tensor_dedicated_gates.exe --benchmark_out=artifacts\benchmarks\tensor\tnbm.json --benchmark_out_format=json
```

### 4) RoPE ablation benchmark set (when available)

- Required matrix: `none`, `fixed`, `relational`, `learned`
- Required report fields per profile:
  - p50/p95/p99 latency
  - Recall@10, nDCG@10
  - average TT rank, compression ratio (kappa)

### 5) Neurocognitive bridge benchmark set (when available)

- Required matrix:
  - replay off/on
  - homeostasis off/on
  - arbitration off/on
- Required report fields:
  - p50/p95/p99 latency
  - Recall@10, nDCG@10
  - inter-run variance
  - growth envelope metrics
  - rollback success/failure counts

### 6) Machine dreaming benchmark set (when available)

- Required matrix:
  - dream mode off/on (offline sandbox only)
  - validator strictness baseline/strict
  - ethics veto off/on (test profile only, never production default)
- Required report fields:
  - validated-candidate utility delta
  - online nonregression delta (hallucination/error rate)
  - label-integrity pass rate
  - veto-path activation counts

## CTest Validation Protocol (Tensor + RoPE)

### 1) Build focused tensor tests

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target module_tensor_test_tensor_ingestion_bridge_focused module_tensor_test_tensor_index_manager_focused module_tensor_test_tensor_contract_hardening_focused module_tensor_test_tensor_bridge_edge_cases_focused
```

### 2) Run tensor focused suites

```powershell
ctest --preset windows-release -R "test_tensor_ingestion_bridge_tensor_FocusedTests|test_tensor_index_manager_tensor_FocusedTests|test_tensor_contract_hardening_focused_tensor_FocusedTests|test_tensor_bridge_edge_cases_focused_tensor_FocusedTests" --output-on-failure
```

### 3) Run full tensor-labeled slice

```powershell
ctest --preset windows-release -L tensor --output-on-failure
```

### 4) RoPE-specific acceptance additions (to be introduced with TN-ROPE tests)

- `test_tensor_rope_profile_determinism_tensor_FocusedTests`
- `test_tensor_rope_profile_mismatch_failclosed_tensor_FocusedTests`
- `test_tensor_rope_hybrid_parity_tensor_FocusedTests`
- `test_tensor_rope_llama_cpp_rag_profile_tensor_FocusedTests`
- `test_tensor_rope_adalora_bridge_profile_tensor_FocusedTests`

All five RoPE acceptance tests are required for TEN-ROPE-G5 pass.

### 5) Neurocognitive acceptance additions (to be introduced with WP-BRAIN tests)

- `test_tensor_brain_replay_determinism_tensor_FocusedTests`
- `test_tensor_brain_homeostasis_bounded_growth_tensor_FocusedTests`
- `test_tensor_brain_arbiter_determinism_tensor_FocusedTests`
- `test_tensor_brain_core_safety_envelope_tensor_FocusedTests`

All four neurocognitive acceptance tests are required for BRAIN-SAFETY-CORE-01 pass.

### 6) Machine dreaming acceptance additions (to be introduced with WP-BRAIN-04 tests)

- `test_tensor_brain_dream_isolation_tensor_FocusedTests`
- `test_tensor_brain_dream_label_integrity_tensor_FocusedTests`
- `test_tensor_brain_dream_utility_tensor_FocusedTests`
- `test_tensor_brain_dream_nonregression_tensor_FocusedTests`
- `test_tensor_brain_dream_ethics_boundary_tensor_FocusedTests`

All five machine-dreaming acceptance tests are required for BRAIN-DREAM-* gate closure.

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional tensor benchmark scenarios are introduced.
- For TN-ROPE, neurocognitive, and machine-dreaming gate decisions, benchmark + CTest evidence must be attached in one packet:
  - benchmark command log
  - benchmark JSON artifacts
  - CTest command log with pass/fail summary
  - go/no-go decision note

## Sourcecode Verification (Module: tensor/performance)

- Verified benchmark sources:
  - benchmarks/tensor/bench_tensor_fingerprint_graph.cpp
  - benchmarks/tensor/bench_tensor_fingerprint.cpp
  - benchmarks/tensor/bench_tensor_deduplication_manager.cpp
  - benchmarks/tensor/bench_tensor_release_gates.cpp
  - benchmarks/tensor/bench_tensor_dedicated_gates.cpp
- Verified mapping surfaces:
  - fingerprint graph, fingerprint fixtures, dedup snapshot/replay behavior, and gate benchmark suites
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.