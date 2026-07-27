# AdaLoRA↔TT Bridge — Reproducible Benchmark Protocol

**Document type:** Benchmark Protocol (publication-grade)  
**Status:** Design-complete; awaiting runtime integration for GGML/FLARE tracks  
**Version:** 1.0  
**Last updated:** 2026-07-27  
**Scope:** `AdaLoraTTBridge`, `TensorNetworkStorageEngine`, `TensorFingerprintGraph`, `GgmlTensorBridge`  
**Artifact home:** `research/experiments/adalora_tt_bridge/`  
**Related research:** `research/ADALORA_TT_BRIDGE_RESEARCH.md`, `research/ADALORA_TT_BRIDGE_ARXIV_DRAFT.md`

> **Integrity clause:** This document defines *how* to measure; it does not report or imply
> measurements that have not yet been taken. All quantitative claims in future papers must
> be traceable to run artifacts under `research/experiments/adalora_tt_bridge/results/`.

---

## 1. Purpose and Scope

This protocol establishes the reproducibility contract for publication-level claims on the
AdaLoRA↔Tensor-Train (TT) bridge in ThemisDB. It covers four measurement tracks:

| Track ID | Name | Scope |
|----------|------|-------|
| BT-1 | Adapter load-path latency | Baseline vs. bridge-path load, cold vs. warm |
| BT-2 | Storage deduplication efficiency | Homogeneous and heterogeneous adapter corpora |
| BT-3 | Rank pruning / reconstruction quality | AdaLoRA greedy vs. TT-rounding under equal budget |
| BT-4 | Runtime adapter switching (FLARE-style) | Token latency and task quality, retrieval modes |

Each track specifies: experiment matrix, hardware/software prerequisites, statistical reporting
minimum, artifact naming, command/config template, failure modes, and validity threats.

---

## 2. Implementation Readiness

Before running any track, verify the integration status of the required code path:

| Code path | Current status | Track dependency |
|-----------|---------------|-----------------|
| `AdaLoraTTBridge::exportToTT()` | Implemented | BT-1, BT-2, BT-3 |
| `AdaLoraTTBridge::store()` / `loadAdapter()` | Implemented | BT-1, BT-2 |
| `TensorFingerprintGraph` dedup | Implemented | BT-2 |
| `AdaLoraTTBridge::roundAndReallocate()` | Implemented (built-in + callback) | BT-3 |
| `AdaLoraTTBridge::mapAdapter()` + `MapAdapterFn` | Callback bridge ready; GGML backend gated | BT-4 |
| `GgmlTensorBridge::mapAdapter()` (zero-copy serving) | Partial stub — requires callback wiring | BT-4 (blocked) |
| FLARE retrieval integration | Planned (Phase 3/4) | BT-4 (blocked) |

Tracks BT-1 through BT-3 are runnable today against the in-process bridge path.
Track BT-4 (FLARE switching) is **blocked** pending GGML bridge callback wiring and
FLARE retrieval integration. Section 6.4 specifies the exact unblocking gate.

---

## 3. Hardware and Software Environment Schema

Every published result **must** include all fields below. Missing fields invalidate the result.

### 3.1 Environment descriptor (JSON)

```json
{
  "env_schema_version": "1.0",
  "hardware": {
    "cpu_model": "<e.g. AMD EPYC 7763 64-Core>",
    "cpu_sockets": 1,
    "cpu_cores_per_socket": "<N>",
    "ram_gb": "<N>",
    "gpu_model": "<model or null>",
    "gpu_vram_gb": "<N or null>",
    "storage_class": "<NVMe SSD / SATA SSD / RAM>",
    "numa_topology": "<single / dual-socket / ...>"
  },
  "software": {
    "os": "<Ubuntu 22.04 LTS>",
    "compiler": "<clang-17 or gcc-12>",
    "compiler_flags": "-O3 -march=native -DNDEBUG",
    "cmake_preset": "<linux-release or windows-release>",
    "google_benchmark_version": ">=1.8",
    "python_version": ">=3.10",
    "themisdb_git_sha": "<40-char SHA>",
    "themisdb_branch": "develop"
  },
  "isolation": {
    "background_processes_killed": true,
    "cpu_governor": "performance",
    "turbo_boost": "disabled",
    "hyperthreading": "<enabled / disabled>",
    "aslr": "disabled"
  }
}
```

### 3.2 Canonical CI baseline

For CI reference runs that will be cited, the baseline is:

| Field | Value |
|-------|-------|
| OS image | `ubuntu-22.04` |
| Compiler | `clang-17` |
| Flags | `-O3 -march=native -DNDEBUG` |
| CMake preset | `linux-release` |
| Google Benchmark | ≥ 1.8 |
| RNG seed | `42` (matches `kCanonicalRngSeed`) |

---

## 4. Statistical Reporting Minimums

All tracks share these reporting requirements. Track-specific additions are in Section 6.

| Requirement | Minimum |
|-------------|---------|
| Independent runs per configuration | ≥ 30 |
| Warmup iterations (Google Benchmark repetitions) | 500 (cold: 50, warm: 100, hot: 200 per MEASUREMENT_HYGIENE.md §3) |
| Google Benchmark `--benchmark_repetitions` | 10 |
| Summary statistics required | mean, stddev, CV(%), p50, p95, p99 |
| Effect size | Cohen's d vs. baseline (for comparison tracks) |
| Confidence interval | 95% bootstrap CI for p50 / p99 |
| Variance ceiling | CV ≤ 15% (hard); CV > 25% invalidates the run |
| Outlier treatment | Report separately; do not silently discard |

Use `research/experiments/adalora_tt_bridge/collect_results.py` to compute these statistics
from Google Benchmark JSON output.

---

## 5. Artifact Naming and Storage Convention

```
research/experiments/adalora_tt_bridge/
├── RESULT_SCHEMA.md          # Canonical result schema documentation
├── result_schema.json        # JSON Schema for machine-validation of result files
├── run_config_template.json  # Canonical run configuration template
├── collect_results.py        # Result collection and statistical reporting script
└── results/
    └── <YYYY-MM-DD>_<track-id>_<variant>_<env-hash>/
        ├── env.json           # Environment descriptor (§3.1)
        ├── raw/               # Raw Google Benchmark JSON output files
        │   └── <benchmark_name>_rep<N>.json
        ├── summary.json       # Computed statistics (schema: result_schema.json)
        └── README.md          # Run notes (deviations, anomalies, context)
```

**Naming rules:**

- `<YYYY-MM-DD>`: ISO date of the run.
- `<track-id>`: `bt1`, `bt2`, `bt3`, or `bt4`.
- `<variant>`: descriptor of the experimental variant (e.g., `cold_load_rank8`, `dedup_homogeneous`).
- `<env-hash>`: first 8 chars of `sha256(env.json)` for uniqueness.

Example: `results/2026-09-15_bt1_cold_load_rank8_a3f2c1b0/`

---

## 6. Experiment Tracks

### 6.1 BT-1: Adapter Load-Path Latency

**Research question:** How does the bridge-path load latency (TT-format, via
`AdaLoraTTBridge::loadAdapter()`) compare to a flat-format baseline, across cold and warm runs?

#### 6.1.1 Experiment matrix

| Experiment ID | Variant | Rank | Layers | Run type |
|---------------|---------|------|--------|----------|
| BT1-A-cold-r4  | Bridge path | 4  | 8 | Cold (no cache) |
| BT1-A-cold-r16 | Bridge path | 16 | 8 | Cold (no cache) |
| BT1-A-cold-r64 | Bridge path | 64 | 8 | Cold (no cache) |
| BT1-B-warm-r4  | Bridge path | 4  | 8 | Warm (re-load after initial load) |
| BT1-B-warm-r16 | Bridge path | 16 | 8 | Warm (re-load after initial load) |
| BT1-B-warm-r64 | Bridge path | 64 | 8 | Warm (re-load after initial load) |
| BT1-C-baseline | Flat B+A load | 4..64 | 8 | Cold |

**Required metrics per cell:** p50 (µs), p95 (µs), p99 (µs), CV(%)

#### 6.1.2 Prerequisites

- `AdaLoraTTBridge` implementation reachable from benchmark binary.
- `TensorNetworkStorageEngine` in-process store populated before warm runs.
- OS temp directory used for any on-disk artifacts (no hardcoded paths).

#### 6.1.3 Build and run template

```bash
# Build (linux-release preset, Ninja required)
cmake --preset linux-release
cmake --build --preset linux-release --parallel 8 \
  --target bench_adalora_tt_bridge_latency

# Run — cold (flush in-process cache between repetitions)
./build-linux-release/benchmarks/bench_adalora_tt_bridge_latency \
  --benchmark_filter="BT1.*cold" \
  --benchmark_repetitions=10 \
  --benchmark_report_aggregates_only=false \
  --benchmark_out=results/bt1_cold.json \
  --benchmark_out_format=json

# Run — warm
./build-linux-release/benchmarks/bench_adalora_tt_bridge_latency \
  --benchmark_filter="BT1.*warm" \
  --benchmark_repetitions=10 \
  --benchmark_out=results/bt1_warm.json \
  --benchmark_out_format=json

# Compute statistics
python3 research/experiments/adalora_tt_bridge/collect_results.py \
  --input results/bt1_cold.json results/bt1_warm.json \
  --track bt1 \
  --output research/experiments/adalora_tt_bridge/results/$(date +%F)_bt1_<variant>_<env-hash>/summary.json
```

#### 6.1.4 Failure modes and validity threats

| Threat | Mitigation |
|--------|-----------|
| Cache warming from OS page cache across cold runs | Use `posix_fadvise(DONTNEED)` or restart process per cold repetition |
| JIT/branch-predictor warm-up on first run | Discard first 3 repetitions for cold track, verify with CV |
| Different rank sizes produce non-comparable storage paths | Report rank separately; do not aggregate across ranks |
| Benchmark binary not linked against production bridge (stub path) | Verify `mapAdapter` and `MapAdapterFn` wiring in build configuration before run |

---

### 6.2 BT-2: Storage Deduplication Efficiency

**Research question:** What raw byte reduction and deduplication hit rate does
`TensorFingerprintGraph` achieve for domain-homogeneous vs. domain-heterogeneous adapter corpora?

#### 6.2.1 Experiment matrix

| Experiment ID | Corpus type | Corpus size | Similarity threshold | Expected dedup rate |
|---------------|-------------|-------------|----------------------|---------------------|
| BT2-A-homo-10 | Homogeneous (same domain) | 10 adapters | 0.999 | High (hypothesis) |
| BT2-A-homo-50 | Homogeneous | 50 adapters | 0.999 | High (hypothesis) |
| BT2-B-hetero-10 | Heterogeneous (mixed domains) | 10 adapters | 0.999 | Low (hypothesis) |
| BT2-B-hetero-50 | Heterogeneous | 50 adapters | 0.999 | Low (hypothesis) |
| BT2-C-fp-check | Collision / false-positive test | 1000 random pairs | 0.999 | < 1% FP rate |

> **Note:** "Homogeneous" = adapters fine-tuned on the same domain/task class with
> different seeds or hyperparameter variants. "Heterogeneous" = adapters from
> at least 3 distinct domains (e.g., legal, medical, code).

**Required metrics per cell:**

- Total bytes before dedup (sum of raw B+A matrices across all layers and adapters)
- Total bytes after dedup (unique TT-cores stored in `TensorNetworkStorageEngine`)
- Dedup ratio: `bytes_after / bytes_before` (lower = better; report as percentage)
- Dedup hit count: number of layers whose TT-core fingerprint matched an existing entry
- Collision / false-positive count (BT2-C only): pairs classified as similar where
  the true cosine similarity of original B+A matrices is below the threshold

#### 6.2.2 Adapter corpus generation

For reproducibility, synthetic adapter corpora must be generated with:

```python
# Corpus generation seed — must match kCanonicalRngSeed
CORPUS_RNG_SEED = 42

# Homogeneous corpus: N adapters, same (d, k, r) shape, different weight seeds
# Heterogeneous corpus: N adapters, at least 3 distinct (d, k, r) shapes
```

Corpora generated from this seed and shape specification must be archived alongside
results in `results/<run-dir>/corpora/` as `.npy` files or JSON weight dumps.

#### 6.2.3 Build and run template

```bash
cmake --build --preset linux-release --parallel 8 \
  --target bench_adalora_tt_bridge_dedup

./build-linux-release/benchmarks/bench_adalora_tt_bridge_dedup \
  --benchmark_filter="BT2.*" \
  --benchmark_repetitions=10 \
  --benchmark_out=results/bt2.json \
  --benchmark_out_format=json

python3 research/experiments/adalora_tt_bridge/collect_results.py \
  --input results/bt2.json \
  --track bt2 \
  --output research/experiments/adalora_tt_bridge/results/$(date +%F)_bt2_<variant>_<env-hash>/summary.json
```

#### 6.2.4 Failure modes and validity threats

| Threat | Mitigation |
|--------|-----------|
| Fingerprint graph state not reset between repetitions | Clear graph state before each BT2 repetition group |
| Synthetic adapters too similar by construction (inflate dedup rate) | Validate pairwise ground-truth similarities with L2 distance check |
| False-positive rate inflated by low-dimension test adapters | Use minimum rank 4 and minimum dimension 64 for all test adapters |
| Byte comparison does not account for metadata overhead | Report raw weight bytes only; metadata reported separately |

---

### 6.3 BT-3: Rank Pruning / Reconstruction Quality

**Research question:** Under equal parameter budget, how does TT-rounding
(`AdaLoraTTBridge::roundAndReallocate()`) compare to AdaLoRA greedy pruning
on reconstruction error and downstream task metrics?

#### 6.3.1 Experiment matrix

| Experiment ID | Pruning method | Budget | Layers | Metric |
|---------------|---------------|--------|--------|--------|
| BT3-A-adalora-greedy-10 | AdaLoRA greedy | 10% rank reduction | 8 | Recon. error (‖ΔW − ΔWapprox‖_F) |
| BT3-A-adalora-greedy-30 | AdaLoRA greedy | 30% rank reduction | 8 | Recon. error |
| BT3-A-adalora-greedy-50 | AdaLoRA greedy | 50% rank reduction | 8 | Recon. error |
| BT3-B-tt-round-10 | TT-rounding (ε = budget-equivalent) | 10% rank reduction | 8 | Recon. error |
| BT3-B-tt-round-30 | TT-rounding | 30% rank reduction | 8 | Recon. error |
| BT3-B-tt-round-50 | TT-rounding | 50% rank reduction | 8 | Recon. error |
| BT3-C-downstream | Both methods | 30% reduction | 8 | Task accuracy delta |

**Budget equivalence rule:** For a fair comparison, the TT-rounding epsilon `ε` must be set
such that the resulting total active rank after rounding matches the total active rank after
AdaLoRA greedy pruning to within ±1 rank unit across all layers. Document the actual ε chosen
and the resulting rank counts per layer in `summary.json`.

**Required metrics:**

- Per-layer Frobenius reconstruction error: `‖ΔW_original − ΔW_pruned‖_F / ‖ΔW_original‖_F`
- Aggregate reconstruction error: mean and max across layers
- Total active rank before and after (both methods)
- Downstream task accuracy delta (BT3-C): if a downstream eval task is available,
  report accuracy delta vs. unpruned baseline (must specify task, dataset, and eval harness)

#### 6.3.2 Downstream task specification (BT3-C)

Downstream evaluation (BT3-C) requires an external task. The task must be:

- Publicly available (e.g., LegalBench, MedMCQA, or SuperGLUE subset)
- Reproducible from a fixed seed (document dataset split seed)
- Evaluated with the same inference configuration for both methods
- At least 500 evaluation examples

If no downstream task is available at run time, BT3-C must be marked `SKIPPED` in the
summary with justification. Reconstruction error (BT3-A/B) can proceed independently.

#### 6.3.3 Build and run template

```bash
cmake --build --preset linux-release --parallel 8 \
  --target bench_adalora_tt_bridge_rank_quality

./build-linux-release/benchmarks/bench_adalora_tt_bridge_rank_quality \
  --benchmark_filter="BT3.*" \
  --benchmark_repetitions=10 \
  --benchmark_out=results/bt3.json \
  --benchmark_out_format=json

# For BT3-C (downstream eval — external harness required):
# python3 scripts/run_downstream_eval.py \
#   --method adalora_greedy --budget 0.30 --task legalbench \
#   --adapter-dir results/bt3_adapters/adalora_greedy_30/
# python3 scripts/run_downstream_eval.py \
#   --method tt_rounding --eps <computed_eps> --task legalbench \
#   --adapter-dir results/bt3_adapters/tt_rounding_30/
```

#### 6.3.4 Failure modes and validity threats

| Threat | Mitigation |
|--------|-----------|
| Budget inequivalence inflates TT advantage | Verify rank counts per-layer in summary; flag if mismatch > 1 |
| Reconstruction error dominated by outlier layers | Report per-layer distribution, not only aggregate |
| Downstream task OOD from adapter training distribution | Document adapter training task and downstream task; flag OOD if domain differs |
| TT-rounding used with non-canonical ε produces cherry-picked results | Archive the exact ε value; report sensitivity curve across 3 ε values |

---

### 6.4 BT-4: Runtime Adapter Switching (FLARE-style)

**Research question:** In a retrieval-augmented generation (FLARE-style) scenario,
what token latency and task quality impact does live adapter switching via the TT bridge
produce vs. no switching?

> **Gate:** BT-4 is **blocked** until the following conditions are met:
>
> 1. `AdaLoraTTBridge::setMapAdapterFn()` is wired to a production `GgmlTensorBridge::mapAdapter()` implementation (not a returning-`false` stub).
> 2. `GgmlTensorBridge` has resolved Stub #271 (GGML type registration and real alloc callbacks).
> 3. A FLARE retrieval scenario is implemented in the llama.cpp integration layer.
>
> Until this gate is cleared, BT-4 results **must not** be published. Document this
> gate state in every results `README.md` for BT-4 runs.

#### 6.4.1 Experiment matrix (for when gate is cleared)

| Experiment ID | Mode | Retrieval | Adapter switching |
|---------------|------|-----------|-------------------|
| BT4-A-retrieve-only | Retrieval only | Yes (FLARE) | No |
| BT4-B-retrieve-switch | Retrieval + switching | Yes (FLARE) | Yes (TT bridge) |
| BT4-C-no-retrieve | Baseline | No | No |
| BT4-D-cold-switch | Switching overhead | No retrieval | Yes (TT bridge), cold |

**Required metrics:**

- Token latency (ms/token): p50, p95, p99 for each mode
- Time-to-first-token (TTFT): p50, p95 for modes involving adapter load
- Adapter switch overhead: `latency(BT4-B) − latency(BT4-A)` per token
- Task accuracy delta vs. BT4-C (specify task and eval harness)

#### 6.4.2 Build and run template (post-gate)

```bash
cmake --build --preset linux-release --parallel 8 \
  --target bench_adalora_tt_bridge_flare_switching

./build-linux-release/benchmarks/bench_adalora_tt_bridge_flare_switching \
  --benchmark_filter="BT4.*" \
  --benchmark_repetitions=10 \
  --benchmark_out=results/bt4.json \
  --benchmark_out_format=json
```

#### 6.4.3 Failure modes and validity threats

| Threat | Mitigation |
|--------|-----------|
| `mapAdapter()` silently returns `false` (stub path) | Assert bridge is wired before run; fail fast if `stats().stores_total == 0` |
| GGML context not fully initialised | Run pre-flight health check binary before benchmark |
| Task quality measured with different decode params | Lock `temperature=0`, `top_p=1.0`, `seed=42` for all modes |
| Network retrieval latency contaminates adapter switch overhead | Use local retrieval corpus only; no external endpoints |

---

## 7. Reporting Contract

For every quantitative claim in any publication, report all of the following. Incomplete
reporting is grounds for rejection in peer review and for retraction of internal claims.

| Field | Requirement |
|-------|-------------|
| Hardware spec | Full hardware descriptor per §3.1 |
| Software stack | Full software descriptor per §3.1 |
| Dataset / task | Exact dataset name, version, and split seed |
| Command / config | Exact command line or config file (archived in `results/<run-dir>/`) |
| Number of runs | ≥ 30 independent runs per configuration |
| Summary statistics | mean, stddev, CV(%), p50, p95, p99 |
| Effect size | Cohen's d vs. baseline (where applicable) |
| Confidence interval | 95% bootstrap CI for p50 and p99 |
| Variance assessment | CV ≤ 15% (valid); CV > 25% (invalid — repeat run) |
| Artifact location | Path in repo under `research/experiments/adalora_tt_bridge/results/` |
| Gate status | For BT-4: explicit gate-cleared confirmation with commit SHA |

---

## 8. What Is Currently Measurable

The following is the current (2026-07-27) measurement-ready state:

| Track | Measurable now | Blocked on |
|-------|----------------|------------|
| BT-1: Load latency | ✅ Yes (in-process bridge) | — |
| BT-2: Dedup efficiency | ✅ Yes (TensorFingerprintGraph) | — |
| BT-3: Rank quality | ✅ Yes (roundAndReallocate + recon. error) | BT-3-C downstream: external task |
| BT-4: FLARE switching | ❌ No | GGML bridge wiring (Stub #271), FLARE integration |

Running BT-1, BT-2, and BT-3-A/B generates publication-grade evidence for load-path and
storage claims. BT-3-C and BT-4 require integration milestones outside the bridge itself.

---

## 9. Known Limitations and Validity Threats (Protocol Level)

1. **Norm-product approximation for singular values:** `AdaLoraTTBridge` uses
   `‖B[:,i]‖₂ · ‖A[i,:]‖₂` as a proxy for singular values. This is not exact SVD.
   Any reconstruction error measurement (BT-3) reflects this approximation. Papers must
   disclose this explicitly.

2. **In-process store only (current):** BT-1 load latency measures the in-process cache
   path, not a full persistent RocksDB round-trip. Papers must label results as
   "in-process bridge latency" until the persistent storage path is fully integrated.

3. **Synthetic adapter corpora (BT-2, BT-3):** Until real fine-tuned adapter weights from
   production training runs are available, dedup and quality experiments use synthetic
   weight matrices. These may not represent real fine-tuning distributions. Mark all
   synthetic-corpus results as `corpus_type: synthetic`.

4. **No GPU baseline for BT-1/BT-3:** The current bridge operates on CPU for
   export/import. GPU-accelerated paths (Phase 4/5) will require a separate measurement
   track.

5. **BT-4 is not runnable (Stub #271 outstanding):** Any BT-4 result published before
   the gate in §6.4 is cleared is a protocol violation.

---

## 10. References

### Internal artifacts

- `include/training/adalora_tt_bridge.h` — bridge API contract
- `src/training/adalora_tt_bridge.cpp` — implementation
- `tests/test_adalora_tt_bridge.cpp` — injection-hook test coverage
- `include/storage/ggml_tensor_bridge.h` — GGML bridge (partially gated)
- `src/training/ROADMAP.md` — bridge roadmap and phase plan
- `src/training/PERFORMANCE_EXPECTATIONS.md` — module hard gates
- `benchmarks/MEASUREMENT_HYGIENE.md` — canonical warmup protocol and seed convention
- `research/ADALORA_TT_BRIDGE_RESEARCH.md` — codebase-aligned research review
- `research/ADALORA_TT_BRIDGE_ARXIV_DRAFT.md` — arXiv preparation draft
- `research/experiments/adalora_tt_bridge/result_schema.json` — result schema
- `research/experiments/adalora_tt_bridge/collect_results.py` — result collection tool

### External references

1. Zhang, Q. et al. (2023). *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient
   Fine-Tuning*. ICLR 2023. https://arxiv.org/abs/2303.10512
2. Oseledets, I. V. (2011). *Tensor-Train Decomposition*. SIAM J. Sci. Comput., 33(5).
   https://doi.org/10.1137/090752142
3. Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models*.
   ICLR 2022. https://arxiv.org/abs/2106.09685
4. Jiang, Z. et al. (2023). *Active Retrieval Augmented Generation (FLARE)*. EMNLP 2023.
   https://arxiv.org/abs/2305.06983
5. Yadav, P. et al. (2023). *TIES-Merging: Resolving Interference When Merging Models*.
   NeurIPS 2023. https://arxiv.org/abs/2306.01708

---

<!-- Status: design-complete | runtime-integration pending for BT-4 | validated: 2026-07-27 -->
<!-- Links: ADALORA_TT_BRIDGE_RESEARCH.md · ADALORA_TT_BRIDGE_ARXIV_DRAFT.md · experiments/adalora_tt_bridge/ -->
