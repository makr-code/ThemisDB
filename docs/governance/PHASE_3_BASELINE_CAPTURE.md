# Phase 3: GPU Baseline Capture & Evidence Collection

**Author:** ThemisDB Contributors  
**Created:** 2026-09-23  
**Last Updated:** 2026-09-23  
**Status:** draft  

**Issue:** makr-code/ThemisDB#6575  
**Phase:** 3 (Baseline Capture)  
**Timeline:** 2026-11-03 to 2026-11-17 (1–2 weeks, after Phase 1 infrastructure + Phase 2 code changes)  
**Owner:** GPU Module Team + Platform Release  
**Status:** 🔴 BLOCKED — Awaiting Phase 1 infrastructure completion  

---

## Overview

Phase 3 executes baseline measurements and evidence collection on representative hardware. All measurements feed into `GPU_BASELINES_2026_Q4.json` for formal sign-off in Phase 4.

**Dependencies:**
- Phase 1 complete: gpu-cuda self-hosted runner online ✅ (or 🔴 if deferred)
- Phase 2 complete: CUDA reduction code changes merged to develop ✅ (or 🔴 if in progress)

**Prerequisites:**
Before Phase 3 starts:
- [ ] Phase 1 complete: gpu-cuda runner online and healthy
- [ ] Phase 2 complete or on track: Code changes committed to develop
- [ ] Benchmark binaries built and tested: `/tmp/phase1-bench-build/bin/bench_gpu_a8_baselines`
- [ ] Test suites ready: GPU-TIMEOUT-01..12, GPU-EXHAUST-01..12, GPU-FALLBACK-01..12

---

## Deliverables

### 1. Baseline Capture Workflow

**Location:** `.github/workflows/phase3-gpu-baseline-capture.yml` (create if not exists)

```yaml
name: Phase 3 GPU Baseline Capture

on:
  workflow_dispatch:
    inputs:
      run_representative_hardware:
        description: "Execute on representative hardware (A8-class GPU)"
        required: true
        default: "true"

jobs:
  representative-hardware-baseline:
    runs-on: [self-hosted, gpu-cuda, linux]
    if: github.event.inputs.run_representative_hardware == 'true'
    
    steps:
      - uses: actions/checkout@v5
        with:
          ref: develop
      
      - name: "System Information"
        run: |
          echo "=== GPU Information ==="
          nvidia-smi --query-gpu=index,name,memory.total --format=csv
          echo "=== CUDA & Libraries ==="
          nvcc --version
          echo "=== Build Tools ==="
          cmake --version && ninja --version && gcc --version
      
      - name: "Build Benchmarks (with CUDA enabled)"
        run: |
          cmake --preset community-release \
            -DTHEMIS_ENABLE_CUDA=ON \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -B /tmp/phase3-build
          cmake --build /tmp/phase3-build --target bench_gpu_a8_baselines -j4
          cmake --build /tmp/phase3-build --target gpu_fallback_01 -j4  # Test suites
      
      - name: "Warmup Run (1 minute)"
        run: |
          echo "Running 1-minute warmup to stabilize GPU..."
          /tmp/phase3-build/bin/bench_gpu_a8_baselines \
            --benchmark_min_time=60s \
            --benchmark_out=/tmp/warmup.json \
            --benchmark_out_format=json
      
      - name: "Latency Baselines (p50/p95/p99)"
        run: |
          echo "Executing latency benchmarks..."
          /tmp/phase3-build/bin/bench_gpu_a8_baselines \
            --benchmark_min_time=300s \
            --benchmark_repetitions=10 \
            --benchmark_out=/tmp/phase3_latency.json \
            --benchmark_out_format=json
          # Expected: p50, p95, p99 latency metrics in JSON output
      
      - name: "Throughput Baselines (ops/sec)"
        run: |
          echo "Executing throughput benchmarks..."
          /tmp/phase3-build/bin/bench_gpu_a8_baselines \
            --benchmark_min_time=300s \
            --benchmark_repetitions=5 \
            --benchmark_out=/tmp/phase3_throughput.json \
            --benchmark_out_format=json
          # Expected: ops/sec, vectors/sec metrics
      
      - name: "Wave A Test Suite Execution"
        run: |
          echo "=== GPU-TIMEOUT-01..12 (Kernel SLA enforcement) ==="
          /tmp/phase3-build/bin/test_gpu_timeout_guard_01 && echo "✅ TIMEOUT-01"
          # ... (repeat for TIMEOUT-02..12)
          
          echo "=== GPU-EXHAUST-01..12 (Resource exhaustion) ==="
          /tmp/phase3-build/bin/test_gpu_exhaustion_01 && echo "✅ EXHAUST-01"
          # ... (repeat for EXHAUST-02..12)
          
          echo "=== GPU-FALLBACK-01..12 (CPU fallback all errors) ==="
          /tmp/phase3-build/bin/test_gpu_fallback_01 && echo "✅ FALLBACK-01"
          # ... (repeat for FALLBACK-02..12)
      
      - name: "CUDA Call Reduction Measurement"
        run: |
          echo "Measuring CUDA call reduction percentage..."
          # Use audit script to count unchecked CUDA calls in Phase 3 codebase
          python3 scripts/audit_cuda_calls.py --phase=3 --output=/tmp/phase3_reduction.json
          # Expected: ≥40% reduction vs Wave 7 baseline
      
      - name: "Populate Baseline Evidence JSON"
        run: |
          python3 scripts/populate_baseline_json.py \
            --latency=/tmp/phase3_latency.json \
            --throughput=/tmp/phase3_throughput.json \
            --reduction=/tmp/phase3_reduction.json \
            --test_results=/tmp/test_results.json \
            --output=benchmarks/wave8/GPU_BASELINES_2026_Q4.json
      
      - name: "Upload Evidence Artifacts"
        uses: actions/upload-artifact@v5
        with:
          name: gpu-baseline-evidence-phase3
          path: |
            benchmarks/wave8/GPU_BASELINES_2026_Q4.json
            /tmp/phase3_latency.json
            /tmp/phase3_throughput.json
            /tmp/phase3_reduction.json
      
      - name: "Summary Report"
        run: |
          echo "=== Phase 3 Baseline Capture Complete ==="
          echo "Baseline JSON: benchmarks/wave8/GPU_BASELINES_2026_Q4.json"
          cat benchmarks/wave8/GPU_BASELINES_2026_Q4.json | jq '.metadata + .cuda_reduction_metrics + .acceptance_checklist'
```

**Execution:**
- [ ] Trigger workflow manually from GitHub Actions (Workflows → Phase 3 GPU Baseline Capture → "Run workflow")
- [ ] Select: `run_representative_hardware: true`
- [ ] Monitor job progress in GitHub Actions logs

**Estimated Effort:** 2-3 hours (including runs and uploads)

---

### 2. Baseline Evidence Collection

**Output File:** `benchmarks/wave8/GPU_BASELINES_2026_Q4.json`

The workflow populates this JSON with:

1. **CUDA Reduction Metrics**
   ```json
   {
     "cuda_reduction_metrics": {
       "wave7_baseline_calls": 340,
       "current_unchecked_calls": 204,
       "reduction_percentage": 40.0,
       "target_percentage": 40.0,
       "status": "PASS"
     }
   }
   ```

2. **Latency Baselines (GPU vs CPU)**
   ```json
   {
     "latency_baselines": {
       "gpu_path": {
         "p50_ms": 2.5, "p95_ms": 4.2, "p99_ms": 6.1,
         "measurement_count": 150,
         "methodology": "10 repetitions, 300s warmup per run"
       },
       "cpu_fallback": {
         "p50_ms": 15.0, "p95_ms": 18.5, "p99_ms": 22.0
       }
     }
   }
   ```

3. **Throughput Baselines**
   ```json
   {
     "throughput_baselines": {
       "vector_operations": 2.8e6,  // ops/sec on A8-class GPU
       "reduction_operations": 1.2e6,
       "sort_operations": 890000
     }
   }
   ```

4. **Test Suite Results**
   ```json
   {
     "test_execution_summary": {
       "gpu_timeout_01_12": {"passed": 12, "failed": 0, "status": "PASS"},
       "gpu_exhaust_01_12": {"passed": 12, "failed": 0, "status": "PASS"},
       "gpu_fallback_01_12": {"passed": 12, "failed": 0, "status": "PASS"}
     }
   }
   ```

**Checklist:**
- [ ] Latency p50/p95/p99 captured for GPU and CPU paths
- [ ] Throughput (ops/sec) measured for vector, reduction, sort operations
- [ ] CUDA reduction ≥40% verified
- [ ] All 36 Wave A tests (TIMEOUT, EXHAUST, FALLBACK) pass
- [ ] Memory utilization patterns documented
- [ ] Break-even analysis complete (min computation size for GPU benefit)
- [ ] JSON schema validation passes

**Estimated Effort:** 1 day

---

### 3. Multi-GPU Scaling Analysis (Optional)

If multi-GPU hardware available:

```bash
# Measure peer-to-peer (P2P) latency and NCCL scaling
/tmp/phase3-build/bin/bench_gpu_multi_gpu_p2p --num_devices=2 --output=/tmp/p2p_latency.json
/tmp/phase3-build/bin/bench_gpu_nccl_scaling --num_devices=2 --output=/tmp/nccl_scaling.json
```

**Deliverable:** Append to `GPU_BASELINES_2026_Q4.json` under `multi_gpu_scaling` section

**Estimated Effort:** 1 day (if hardware available)

---

### 4. Break-Even Analysis

Quantify GPU vs CPU crossover point (minimum vector size for GPU advantage).

**Calculation:**
```
break_even_size = (GPU_setup_overhead) / (GPU_speedup - 1.0)

Example: If GPU setup = 2ms, CPU baseline = 10ms, GPU result = 3ms
  break_even = 2ms / ((10ms/3ms) - 1.0) ≈ 600 elements
```

**Categories:**
- **Category A (Eager):** Size > 3× break_even → Always use GPU
- **Category B (Selective):** Size in [break_even, 3× break_even] → Consider GPU if batch multiple
- **Category C (CPU-Only):** Size < break_even → Fallback to CPU

**Deliverable:** Document in `GPU_BASELINES_2026_Q4.json` under `break_even_analysis`

**Estimated Effort:** 1 day

---

## Timeline & Checkpoints

| Day | Task | Owner | Status |
|-----|------|-------|--------|
| Day 1 (11-03) | Warmup run + system info collection | GPU Module Team | ⏳ |
| Day 1-2 (11-04) | Latency baseline execution (10 reps, 300s each) | GPU Module Team | ⏳ |
| Day 2-3 (11-05) | Throughput baseline execution (5 reps) | GPU Module Team | ⏳ |
| Day 3 (11-06) | Wave A test suite execution (36 tests) | GPU Module Team | ⏳ |
| Day 4 (11-07) | CUDA reduction measurement + JSON population | GPU Module Team | ⏳ |
| Day 5 (11-10) | Break-even analysis + optional multi-GPU analysis | GPU Module Team | ⏳ |
| Day 6 (11-12) | Evidence review + validation | Platform Release | ⏳ |
| Day 7 (11-15) | Sign-off preparation | Platform Release | ⏳ |

---

## Success Criteria

**Phase 3 is complete when ALL of the following are met:**

- ✅ Latency baselines captured: p50, p95, p99 for GPU and CPU paths
- ✅ Throughput baselines captured: ops/sec for vector, reduction, sort operations
- ✅ CUDA reduction measured: ≥40% verified (204 unchecked ÷ 340 baseline)
- ✅ All 36 Wave A tests pass: GPU-TIMEOUT-01..12, GPU-EXHAUST-01..12, GPU-FALLBACK-01..12
- ✅ Break-even analysis complete and documented
- ✅ Memory utilization patterns documented (peak VRAM, fragmentation analysis)
- ✅ `GPU_BASELINES_2026_Q4.json` fully populated with measured data
- ✅ Evidence JSON schema validation passes
- ✅ All artifact uploads to GitHub Actions completed

---

## Risk Mitigation

| Risk | Severity | Mitigation |
|------|----------|-----------|
| **Runner unavailable during Phase 3** | HIGH | Pre-test infrastructure health 1 week before; maintain backup runner |
| **Thermal throttling during sustained benchmarks** | MEDIUM | Add thermal monitoring in workflow; reduce benchmark duration if core >85°C |
| **Environmental variance in latency** | MEDIUM | Run ≥10 iterations; report p95/p99 instead of mean; document ambient conditions |
| **Benchmark binary crashes mid-run** | MEDIUM | Implement checkpoint/resume; capture partial results if failure occurs |
| **NCCL library version mismatch** | MEDIUM | Pin NCCL version in Phase 1; verify in workflow warmup run |

---

## Escalation Path

| Issue | Owner | Resolution |
|-------|-------|-----------|
| Runner offline during Phase 3 | Infrastructure | Activate backup runner; reschedule baseline capture |
| Baseline measurements fail | GPU Module Team | Investigate benchmark binary; check GPU memory; retry with reduced workload |
| CUDA reduction <40% | GPU Module Team | Extend Phase 2; identify additional unchecked call sites |
| Test suite fails | GPU Module Team | Debug failing test; check Phase 2 code changes; fix root cause before Phase 4 |

---

## Approval & Sign-Off

**Phase 3 complete when:**

```markdown
- [ ] Latency p50/p95/p99 baselines captured and documented
- [ ] Throughput baselines captured and documented
- [ ] CUDA reduction ≥40% verified
- [ ] All 36 Wave A tests pass on representative hardware
- [ ] GPU_BASELINES_2026_Q4.json fully populated and validated
- [ ] Evidence artifacts uploaded to GitHub Actions
- [ ] GPU Module Team sign-off: <name> on <date> at <time> UTC
```

Upon completion, notify Platform Release team to proceed with Phase 4 sign-off.

---

## Next Phase

Upon Phase 3 completion:
- **Phase 4 Kickoff:** Platform release review and formal sign-off (1 week)
- **Outcome:** Wave A GPU module promoted to release-ready status

---

**Document Type:** Operational Procedure  
**Owner:** GPU Module Team  
**Review Cadence:** Daily during Phase 3 execution  
**Escalation:** platform-release@themisdb  
