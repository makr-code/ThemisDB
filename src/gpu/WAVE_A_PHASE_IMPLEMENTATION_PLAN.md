# Wave A GPU CUDA Reduction — Implementation Phases

**Document Type:** Phase Implementation Plan  
**Issue:** makr-code/ThemisDB#6575 — Wave-A GPU CUDA Reduction + Representative-Hardware Baseline Evidence  
**Created:** 2026-09-23  
**Target Completion:** Q4 2026  
**Status:** 🟡 Phase 1 Complete, Phase 2 Underway, Phase 3-4 Pending

---

## Overview

This document details the four implementation phases required to close the Wave A GPU CUDA reduction requirement: **≥40% reduction in unchecked CUDA kernel calls vs Wave 7 baseline**, plus infrastructure preparation, baseline capture, and formal sign-off.

---

## Phase 1: Prepare GPU Infrastructure

**Target:** By end of Q3 2026  
**Status:** 🟡 DOCUMENTATION COMPLETE — Hardware deployment pending

### Deliverables

- [x] Audit CUDA call patterns across `src/gpu/` and `include/gpu/` (2026-08-24)
- [x] Document infrastructure requirements (`GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md`)
- [x] Create baseline evidence template (`GPU_BASELINES_2026_Q4.json`)
- [ ] Deploy `gpu-cuda` self-hosted runner with CUDA 12.x
  - Hardware: NVIDIA A100 or H100 (≥40 GB VRAM)
  - CUDA: 12.x with cuDNN, NCCL
  - Build tools: CMake 3.22+, Ninja 1.11+, GCC 11+
- [ ] Validate runner connectivity to GitHub Actions
- [ ] Run health check benchmark on deployed infrastructure

### Success Criteria

- ✅ Infrastructure requirements fully documented in `GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md`
- ⏳ `gpu-cuda` runner registered and online
- ⏳ Health check benchmarks green on representative hardware

### Risks

| Risk | Severity | Mitigation |
|------|----------|-----------|
| No GPU hardware available | HIGH | Already documented; defer to Phase 3 with CPU fallback validation complete |
| CUDA driver compatibility | MEDIUM | Test with multiple driver versions (470+, 510+, 530+) |
| NCCL library conflicts | MEDIUM | Use version-pinned installation per requirements doc |

---

## Phase 2: CUDA Reduction Optimization

**Target:** Q3–Q4 2026  
**Status:** 🟡 IN PROGRESS — ~30% reduction achieved, target ≥40%

### Deliverables

- [x] CUDA call audit complete with wrapper recommendations (2026-08-24)
- [x] RAII wrapper library deployed:
  - `include/gpu/cuda_raii.h` — `CudaStreamGuard`, `CudaEventGuard`, `CudaDeviceMemoryGuard`
  - `include/gpu/gpu_safe_raii.h` — `DeviceMemoryGuard<T>`, `CUDA_CHECK` macro
  - `include/gpu/gpu_raii_wrappers.hpp` — `GPUStreamHandle`, `GPUMemoryHandle<T>`, `GPUEventHandle`
- [x] Kernel SLA timeout enforcement confirmed (`KernelSLAGuard` at 11 deployment sites)
- [ ] Migrate all high-risk unchecked CUDA calls to wrapped versions
  - Target files: `query_accelerator.cpp` (10 sites), `memory_pool.cpp` (4 sites)
- [ ] Validate all new allocations use wrapper types
- [ ] Run focused regression tests on hardened paths
- [ ] Measure CUDA call reduction percentage

### Success Criteria

- ✅ All new CUDA calls in `src/gpu/` module use RAII wrappers
- ✅ Kernel SLA timeout enforced in production paths
- ✅ CPU fallback verified for all error classes (GPU-FALLBACK-01..12)
- ✅ Resource exhaustion safe (GPU-EXHAUST-01..12)
- ⏳ ≥40% reduction in unchecked CUDA calls measured on representative hardware
- ⏳ All Wave A test suites (GPU-TIMEOUT, GPU-EXHAUST, GPU-FALLBACK) green on CI

### Implementation Tasks

1. **Stream/Event Management** (2–3 days)
   - Migrate `stream_manager.cpp` to use `CudaStreamGuard`
   - Migrate `cuda_operations.cpp` event handling to `CudaEventGuard`
   - Update 8 raw `cudaStreamCreate`/`cudaStreamDestroy` sites

2. **Memory Management** (3–5 days)
   - Review `gpu_memory_allocator.cpp` for wrapper opportunities
   - Wrap allocation failure paths with `CudaDeviceMemoryGuard<T>`
   - Validate null pointer handling on allocation failure

3. **Query Acceleration Paths** (5–7 days)
   - Harden 10 kernel dispatch sites in `query_accelerator.cpp`
   - Ensure all kernel launches have post-launch error checking
   - Validate `CHECKED_CUDA` macro coverage

4. **Fallback Path Validation** (2–3 days)
   - Run GPU-FALLBACK-01..12 test suite
   - Verify all error classes → CPU path
   - Document any unrecoverable error cases

5. **Reduction Measurement** (1–2 days)
   - Run profiling on Wave 7 baseline
   - Run profiling on current code
   - Calculate reduction percentage
   - Document in `GPU_BASELINES_2026_Q4.json`

### Risks

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Wrapper overhead increases latency | MEDIUM | Benchmark before/after; optimize critical paths if needed |
| Existing code incompatible with RAII | MEDIUM | Gradual migration with compatibility layer (fallback to raw pointers in legacy paths) |
| Reduction target missed (<40%) | HIGH | Identify additional unchecked sites; extend timeline if needed |

---

## Phase 3: Baseline Capture

**Target:** Q4 2026  
**Status:** 🔴 BLOCKED — Awaiting Phase 1 infrastructure completion

### Deliverables

- [ ] Execute `bench_gpu_a8_baselines.cpp` on representative hardware
  - Capture p50/p95/p99 latency for GPU vs CPU paths
  - Capture throughput (ops/sec) for vector operations
  - Document memory utilization patterns
- [ ] Execute CUDA reduction quantification
  - Measure unchecked CUDA calls on Phase 3 codebase
  - Calculate % reduction vs Wave 7 baseline
  - Verify target ≥40% met
- [ ] Execute CPU/GPU break-even analysis
  - Identify minimum computation size for GPU benefit
  - Document categories: Category A (eager), Category B (selective)
- [ ] Generate baseline evidence JSON
  - Populate `GPU_BASELINES_2026_Q4.json` with measured data
  - Attach profiler traces and summary statistics
- [ ] Execute all Wave A test suites on representative hardware
  - GPU-TIMEOUT-01..12 (kernel SLA enforcement)
  - GPU-EXHAUST-01..12 (resource exhaustion)
  - GPU-FALLBACK-01..12 (CPU fallback all paths)
- [ ] Upload evidence artifacts to `benchmarks/wave8/`

### Success Criteria

- ✅ p50/p95/p99 latency baselines captured and documented
- ✅ Throughput baselines captured and documented
- ✅ CUDA reduction ≥40% quantified and documented
- ✅ All Wave A tests pass on representative hardware
- ✅ Evidence JSON fully populated with measured data
- ✅ Baseline evidence signed by platform-release team

### Workflow Integration

```yaml
# In .github/workflows/build-wave-a-gpu.yml
jobs:
  representative-hardware-baseline:
    runs-on: [self-hosted, gpu-cuda, linux]
    if: github.event.inputs.run_representative_hardware == 'true'
    steps:
      - uses: actions/checkout@v5
      - name: "Build benchmarks"
        run: cmake --preset community-release -DTHEMIS_BUILD_BENCHMARKS=ON
      - name: "Capture baselines"
        run: |
          ./build/bin/bench_gpu_a8_baselines --benchmark_out=baselines.json
      - name: "Upload evidence"
        uses: actions/upload-artifact@v5
        with:
          name: gpu-baselines-a8
          path: benchmarks/wave8/GPU_BASELINES_2026_Q4.json
```

### Risks

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Runner unavailable during Phase 3 | HIGH | Pre-test infrastructure health 1 week before baseline capture |
| Thermal throttling during sustained benchmarks | MEDIUM | Add thermal monitoring; reduce concurrency if needed |
| Environmental variance in latency measurements | MEDIUM | Run multiple iterations (n=10+); report p95/p99 instead of mean |

---

## Phase 4: Sign-Off

**Target:** Q4 2026  
**Status:** 🔴 PENDING — Awaiting Phase 3 completion

### Deliverables

- [ ] Platform release team review of baseline evidence
  - Verify reduction ≥40%
  - Verify p95/p99 latencies meet SLA
  - Verify all test suites pass
- [ ] Update `GA_PROMOTION_SIGN_OFF.md` §Wave A GPU with evidence pointers
- [ ] Update `docs/governance/MATURITY_EVIDENCE_MANIFEST.json` with GPU closure status
- [ ] Archive baseline evidence in `benchmarks/wave8/GPU_BASELINES_2026_Q4.json`
- [ ] Record sign-off timestamp and approver identity
- [ ] Merge sign-off to `develop` branch

### Sign-Off Checklist

- [ ] CUDA reduction ≥40% documented
- [ ] Representative-hardware p95/p99 baselines captured
- [ ] All Wave A tests (GPU-TIMEOUT, GPU-EXHAUST, GPU-FALLBACK) pass
- [ ] Kernel SLA timeout enforcement verified
- [ ] CPU fallback works for all error classes
- [ ] No new crashes or regressions observed
- [ ] Evidence artifacts uploaded to GitHub
- [ ] Infrastructure health validated

### Success Criteria

- ✅ Formal sign-off recorded in `GA_PROMOTION_SIGN_OFF.md`
- ✅ All acceptance criteria in `GPU_CUDA_REDUCTION_TRACKING.md` marked complete
- ✅ Wave A GPU module promoted to release-ready status
- ✅ v2.4.0 GA can proceed with CPU-path modules (non-blocking for GPU modules)

---

## Timeline and Milestones

| Phase | Duration | Start | End | Status |
|-------|----------|-------|-----|--------|
| Phase 1: Infrastructure Prep | 2–4 weeks | 2026-09-23 | 2026-10-21 | 🟡 In Progress |
| Phase 2: CUDA Reduction | 3–6 weeks | 2026-09-23 | 2026-11-03 | 🟡 In Progress |
| Phase 3: Baseline Capture | 1–2 weeks | 2026-11-03 | 2026-11-17 | 🔴 Pending |
| Phase 4: Sign-Off | 1 week | 2026-11-17 | 2026-11-24 | 🔴 Pending |
| **Total** | **8–13 weeks** | **2026-09-23** | **2026-11-24** | 🟡 On track for Q4 2026 |

---

## Dependency Map

```
Phase 1 (Infrastructure)
  ├── GPU Hardware acquisition
  ├── CUDA Toolkit 12.x installation
  ├── CI/CD runner registration
  └── Health check validation
       ↓
Phase 2 (CUDA Reduction) ────→ Phase 3 (Baseline Capture)
  ├── Audit complete (✅ 2026-08-24)     ├── Execute benchmarks
  ├── Wrapper library deployed (✅)      ├── Measure reduction
  ├── Migration of call sites (in progress) ├── Capture p95/p99
  ├── Regression testing              ├── Execute test suites
  └── Reduction measurement            └── Generate evidence JSON
       ↓
Phase 4 (Sign-Off)
  ├── Platform release review
  ├── Accept/defer GPU GA
  └── Update governance docs
```

---

## Communication and Escalation

### Weekly Status Updates

**Frequency:** Every Monday 09:00 UTC  
**Forum:** GitHub Issue #6575 comment thread  
**Content:**
- Phase progress (% complete)
- Blockers and resolutions
- Infrastructure health status
- Baseline measurement schedule

### Escalation Path

| Issue | Owner | Escalation |
|-------|-------|-----------|
| GPU hardware unavailable | Platform Team | Notify platform-release@themisdb; consider CPU-only deferral |
| CUDA wrapper incompatibility | GPU Module Lead | Rollback wrapper, file compatibility issue |
| Reduction target not met | GPU Module Lead | Extend Phase 2; identify additional call sites |
| CI/CD runner offline | Infrastructure Team | Activate backup runner; trigger manual baseline |

---

## Roll-Back Plan

If any phase fails irreparably:

1. **Phase 1 Failure (Infrastructure):** 
   - Fallback to CPU-only testing (already validated in Phase 2)
   - Defer GPU evidence to Wave B with human approval
   - Update `GA_PROMOTION_SIGN_OFF.md` with deferral reason

2. **Phase 2 Failure (Reduction target missed):**
   - Continue wrapper deployment for production safety
   - Document reduced-CUDA baseline (e.g., 30% instead of 40%)
   - Escalate to platform-release for acceptance decision

3. **Phase 3 Failure (Baseline capture fails):**
   - Use Phase 2 CPU-only test results as evidence
   - Schedule retry on next available hardware slot
   - Update timeline in roadmap

4. **Phase 4 Failure (Sign-off withheld):**
   - Address reviewer feedback
   - Re-execute any missing evidence
   - Resubmit to platform-release team

---

## Related Documentation

- **Root Roadmap:** `ROADMAP.md` §Wave A GPU, §Critical Release Blockers
- **GPU Module Roadmap:** `src/gpu/ROADMAP.md`
- **Tracking Document:** `src/gpu/GPU_CUDA_REDUCTION_TRACKING.md`
- **Infrastructure Spec:** `docs/governance/GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md`
- **Baseline Template:** `benchmarks/wave8/GPU_BASELINES_2026_Q4.json`
- **Closure Evidence:** `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`
- **GA Sign-Off:** `docs/governance/GA_PROMOTION_SIGN_OFF.md` §Wave A GPU

---

*Document Status: Active Implementation Plan  
*Owner: GPU Module Team + Platform Release  
*Review Cadence: Weekly during execution phases  
*Last Updated: 2026-09-23*
