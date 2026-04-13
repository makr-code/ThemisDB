# Disabled Benchmark Policy

**Status:** Active  
**Effective:** v1.9.0  
**Owners:** Benchmarking Governance (@makr-code/benchmarks)

---

## Problem

Multiple benchmark files register stubs with the naming pattern `BM_*_Disabled` to
placeholder-register a benchmark binary while the underlying feature is unavailable
(missing GPU runner, unimplemented API, external service not present in CI, etc.).
Without lifecycle governance these stubs accumulate indefinitely, causing dead code
and obscuring actual benchmark coverage.

---

## Policy

Every `BENCHMARK(BM_*_Disabled)` or `BENCHMARK_REGISTER_F(…, *_Disabled)` macro
invocation **must** be accompanied — in the same file, within 300 characters of the
registration line — by **both**:

1. **An issue reference** — a GitHub issue number in the form `#NNN` (three or more
   digits), or a URL fragment `.../issues/NNN`.
2. **A sunset deadline** — a version or calendar date in the form
   `Deadline: vX.Y.Z` or `Deadline: YYYY-QN` or `Deadline: YYYY-MM-DD`.

### Canonical annotation format

```cpp
// Disabled: <brief reason> | Deadline: vX.Y.Z | Issue: #NNN
BENCHMARK(BM_Foo_Disabled);
```

Both fields must appear on the same comment line **immediately above** (or as a
trailing comment on) the `BENCHMARK(…)` line.

---

## Lifecycle Rules

| Stage | Action |
|---|---|
| **New disabled stub** | Annotate at creation time; open a tracking issue if none exists. |
| **Deadline reached** | Remove the stub OR replace it with a real benchmark and close the issue. |
| **One release past deadline** | CI check **FAIL** (blocks merge). |

---

## Enforcement

Two automated tools enforce this policy:

### 1. `tools/check_disabled_bench_policy.py` (standalone lint)

```bash
python3 tools/check_disabled_bench_policy.py
# exits 0 if all stubs are compliant, 1 on any violation
```

Run this locally before opening a PR that touches benchmark files.

### 2. `tools/perf_expectations_audit.py` (Maßnahme #9)

The existing audit script (`check_measure_9`) is kept aligned and also validates
disabled-stub compliance.  It emits `STATUS_WARN` for violations.

### 3. CI workflow `05-quality_bench-disabled-policy-ci.yml`

The CI workflow runs `tools/check_disabled_bench_policy.py` on every pull request
that touches `benchmarks/**/*.cpp`.  It **fails** the check on any violation,
blocking merge until the annotation is added or the stub is removed.

---

## Existing Disabled Benchmarks

All existing disabled stubs have been annotated at the time this policy was
introduced.  The table below lists them for reference:

| File | Benchmark | Issue | Deadline |
|---|---|---|---|
| `benchmarks/bench_async_io_multiscan.cpp` | `BM_AsyncIO_Multiscan_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_fused_kernels.cpp` | `BM_FusedKernels_GPUDisabled` | #5 | v1.9.0 |
| `benchmarks/bench_knowledge_gap_detector_phase2.cpp` | `BM_FLARE_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_legal_lora_pipeline.cpp` | `BM_LegalLoRAPipeline_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_lora_gpu.cpp` | `BM_LoRAGPU_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_olap_analytics.cpp` | `BM_OLAP_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_rag_ethics.cpp` | `BM_EthicalCompliance_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_stream_protocol.cpp` | `BM_StreamProtocol_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_tpcc.cpp` | `BM_TPCC_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_video_processor.cpp` | `BM_VideoProcessor_Disabled` | #5 | v1.9.0 |
| `benchmarks/bench_vulkan_lora.cpp` | `BM_VulkanLoRA_GPUDisabled` | #5 | v1.9.0 |
| `benchmarks/bench_ycsb.cpp` | `BM_YCSB_Disabled` | #5 | v1.9.0 |
| `benchmarks/performance_optimizations/phase2/benchmark_phase2.cpp` | `BM_Phase2_Disabled` | #258 | v2.0.0 |

---

## FAQ

**Q: Can I add a disabled benchmark without an issue?**  
A: No.  Open a tracking issue first, then annotate the stub.

**Q: What if the feature will never be implemented?**  
A: Remove the stub entirely.  A disabled benchmark that will never be re-enabled has
no value.

**Q: Can I extend the deadline?**  
A: Yes — update the comment and the tracking issue.  The policy checker uses the
annotation in the source file, so an updated `Deadline:` comment is sufficient.
