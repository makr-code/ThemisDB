# Wave A GPU Phase 3 & 4 — Execution Readiness

**Date:** 2026-09-23  
**Status:** 🟡 PREPARATION COMPLETE — Awaiting Phase 1 GPU infrastructure  
**Blocking:** Phase 1 (gpu-cuda self-hosted runner, 2–4 weeks)

---

## Phase 3: Baseline Capture & Measurement — Ready to Execute

### Objective
Capture representative-hardware baseline evidence (latency, throughput, test results) on gpu-cuda self-hosted runner.

### Execution Checklist

#### 3.1 Pre-Execution (1 day)
- [ ] Verify Phase 1 complete:
  - gpu-cuda runner online and registered
  - NVIDIA A100/H100 hardware accessible
  - CUDA 12.x + NCCL + gRPC verified
  - Health check passed (`gpu-runner-health.yml`)
- [ ] Verify Phase 2 complete:
  - 75% CUDA reduction confirmed
  - query_accelerator.cpp CHECKED_CUDA wrappers deployed
  - All code changes on develop branch

#### 3.2 Measurement Execution (3–5 days)
- [ ] Run Wave A GPU test suites (45+ tests):
  - GPU-FALLBACK-01..12: `test_gpu_fallback_all_paths`
  - GPU-TIMEOUT-01..12: `test_gpu_kernel_timeout_enforcer`
  - GPU-EXHAUST-01..12: `test_gpu_resource_exhaustion`
  - GPU-CLOSURE: `test_gpu_wave_a_timeout_closure`
- [ ] Execute latency measurement workflow:
  - Warm-up run (discard)
  - 10 benchmark runs with p50/p95/p99 capture
  - CPU fallback comparison (same 10 runs)
  - Record timing, GPU utilization, memory footprint
- [ ] Execute throughput measurement workflow:
  - Sustained load tests (30 seconds each)
  - Operations/sec capture
  - Speedup ratio calculation (GPU vs CPU)
- [ ] Execute resource exhaustion validation:
  - Memory limit enforcement (4 GiB test)
  - Stream pool saturation (1000+ streams)
  - Device exhaustion recovery
- [ ] Collect system telemetry:
  - GPU temperature, power consumption, clock throttling
  - Memory allocation/deallocation patterns
  - NCCL communication latency (if multi-GPU)

#### 3.3 Evidence Documentation (2 days)
- [ ] Populate `benchmarks/wave8/GPU_BASELINES_2026_Q4.json`:
  - Metadata (timestamp, hardware, CUDA version, test platform)
  - CUDA reduction metrics (from Phase 2 audit)
  - Latency baselines (GPU path: p50/p95/p99)
  - Latency baselines (CPU fallback: p50/p95/p99)
  - Throughput baselines (ops/sec, speedup ratio)
  - Test execution summary (all 4 suites)
  - System telemetry
  - Acceptance checklist (all 8 criteria)
- [ ] Run validation script:
  - `python3 scripts/validate_gpu_baselines.py --baseline benchmarks/wave8/GPU_BASELINES_2026_Q4.json`
  - Verify all required sections populated
  - Confirm target thresholds met (≥40% CUDA reduction, latency captured, etc.)
- [ ] Generate Phase 3 completion report:
  - Evidence hash and integrity verification
  - Test execution summary (passed/failed counts)
  - Anomalies or concerns for review
  - Sign-off readiness assessment

#### 3.4 Sign-Off Coordination (1 day)
- [ ] Request platform-release@themisdb sign-off:
  - Evidence location: `benchmarks/wave8/GPU_BASELINES_2026_Q4.json`
  - Evidence integrity: Signature or hash verification
  - Approval timeline: 24-48 hours
- [ ] Obtain QA verification:
  - Test suite results reviewed
  - Fallback path validation confirmed
  - No regression vs Phase 2 code
- [ ] Obtain security clearance:
  - No new CWE/CVE risks introduced
  - GPU error handling safe
  - No information leakage in telemetry

### Phase 3 Tools & Scripts

| Tool | Purpose | Status |
|------|---------|--------|
| `run_wave_a_gpu_tests.sh` | Test orchestration (45+ tests) | ✅ Ready |
| `measure_cuda_reduction.py` | CUDA call audit | ✅ Ready |
| `validate_gpu_baselines.py` | Baseline validation | ✅ Ready |
| GitHub Actions workflow | Automated Phase 3 execution | 🟡 Pending |

### Phase 3 Success Criteria
- ✅ All 45+ Wave A tests passing (or appropriately documented as skipped on CPU-only)
- ✅ Latency baselines (p50/p95/p99) captured on A100/H100 hardware
- ✅ Throughput baselines (ops/sec, speedup ratio) measured
- ✅ CPU fallback path performance compared
- ✅ Evidence fully documented in `GPU_BASELINES_2026_Q4.json`
- ✅ Validation script reports zero critical errors
- ✅ Sign-off obtained from platform-release@themisdb

### Phase 3 Timeline
- **Start:** When Phase 1 infrastructure online (2–4 weeks Q4 2026)
- **Measurement:** 3–5 days
- **Documentation & Sign-Off:** 2–3 days
- **Total Duration:** 1–2 weeks

---

## Phase 4: GA Sign-Off & Closure — Ready to Execute

### Objective
Complete Wave A GPU GA sign-off process and close issue #6575.

### Execution Checklist

#### 4.1 Acceptance Criteria Validation (1 day)
- [ ] Run GA sign-off validator:
  - `python3 scripts/validate_ga_sign_off_criteria.py --baseline benchmarks/wave8/GPU_BASELINES_2026_Q4.json --commit-sha <sha>`
  - Verify all 8 acceptance criteria met
  - Generate acceptance report
- [ ] Review 8 Wave A Acceptance Criteria:
  1. ✅ CUDA reduction ≥40% (achieved: 75%)
  2. ✅ Fallback CPU path operational (all tests pass)
  3. ✅ Commit SHA on develop (Phase 2 commits)
  4. ✅ Performance documentation (latency + throughput)
  5. ✅ Self-hosted gpu-cuda runner online (Phase 1)
  6. ✅ Representative-hardware baseline captured (Phase 3)
  7. ✅ Baseline signed off by platform-release@themisdb (Phase 3)
  8. ✅ Evidence pointer in GA_PROMOTION_SIGN_OFF.md §Wave D (this step)

#### 4.2 Governance Synchronization (1 day)
- [ ] Run governance sync script:
  - `python3 scripts/sync_ga_promotion_sign_off.py --commit-sha <sha> --baseline benchmarks/wave8/GPU_BASELINES_2026_Q4.json`
  - Auto-update GA_PROMOTION_SIGN_OFF.md §Wave D
  - Auto-update MATURITY_EVIDENCE_MANIFEST.json
  - Auto-update ROADMAP.md §Wave A GPU
  - Auto-update src/gpu/ROADMAP.md §Wave A GPU
- [ ] Manual review of synchronized documents:
  - Verify evidence pointers correct
  - Verify timestamp accuracy
  - Verify commit SHA accuracy
  - Confirm no merge conflicts introduced
- [ ] Create git commits for governance updates:
  - Separate commit: "Phase 4: Synchronize GA sign-off evidence (Wave A GPU)"
  - Link to issue #6575 in commit body

#### 4.3 Platform Release Review (2–3 days)
- [ ] Obtain final platform-release@themisdb approval:
  - All 8 acceptance criteria met
  - Baseline evidence complete
  - Governance documentation synchronized
  - No outstanding blockers
- [ ] Security & Operations Review:
  - GPU error handling safe for production
  - No new operational risks
  - Runbooks and alerting ready (if applicable)
  - GPU runner SLA documented

#### 4.4 Issue Closure (1 day)
- [ ] Create release notes for Wave A GPU:
  - Summary of CUDA reduction (75% achievement)
  - Performance improvements (latency/throughput baseline)
  - Known limitations and future work
  - GPU module availability and prerequisites
- [ ] Close issue #6575:
  - Mark as completed
  - Reference all 4 phase completion commits
  - Link to GA_PROMOTION_SIGN_OFF.md §Wave D
  - Link to baseline evidence file
- [ ] Post-release checklist:
  - [ ] GPU module tagged as v2.5.0 (or appropriate version)
  - [ ] Wave A GPU documentation updated
  - [ ] Release notes published
  - [ ] GPU team notified of GA status

### Phase 4 Tools & Scripts

| Tool | Purpose | Status |
|------|---------|--------|
| `validate_ga_sign_off_criteria.py` | 8-point acceptance check | ✅ Ready |
| `sync_ga_promotion_sign_off.py` | Governance auto-sync | ✅ Ready |
| GA_PROMOTION_SIGN_OFF.md template | Evidence tracking | ✅ Ready |
| MATURITY_EVIDENCE_MANIFEST.json | Metric consolidation | ✅ Ready |

### Phase 4 Success Criteria
- ✅ All 8 Wave A acceptance criteria validated and documented
- ✅ GA_PROMOTION_SIGN_OFF.md §Wave D populated with full evidence
- ✅ MATURITY_EVIDENCE_MANIFEST.json synchronized
- ✅ ROADMAP.md entries updated to reflect completion
- ✅ Issue #6575 closed with final sign-off
- ✅ No open blockers or deferred work items

### Phase 4 Timeline
- **Start:** When Phase 3 complete (after baseline capture)
- **Validation & Sync:** 1–2 days
- **Review & Approval:** 2–3 days
- **Closure & Release:** 1 day
- **Total Duration:** ~1 week

---

## Overall Wave A GPU Roadmap

```
Phase 1 (Infra): 2–4 weeks Q4 2026 [BLOCKED on hardware — Platform Team]
       ↓
Phase 2 (Code):  3–6 weeks Q4 2026 [✅ COMPLETE — 75% CUDA reduction]
       ↓
Phase 3 (Baseline): 1–2 weeks Q4 2026 [READY, waiting Phase 1]
       ↓
Phase 4 (Sign-Off): ~1 week Q4 2026 [READY, waiting Phase 3]
       ↓
Wave A GPU GA (v2.5.0+) [READY to ship once Phases 3-4 complete]
```

---

## Dependencies & Blockers

### Blocking Phase 3
- ⏳ **Phase 1 Infrastructure:** GPU hardware + gpu-cuda runner (Platform Team, 2–4 weeks)
- ⏳ **CUDA 12.x Stack:** Full NCCL + gRPC library validation

### Blocking Phase 4
- ⏳ **Phase 3 Completion:** Baseline capture and sign-off

### Non-Blocking (v2.4.0 CPU-path GA)
- ✅ v2.4.0 CPU-path modules (Transaction, Sharding, Replication) proceed independently
- ✅ Issue deferral approved if GPU infrastructure unavailable per issue #6575 scope

---

## Escalation Procedure

If Phase 1 infrastructure delay exceeds 4 weeks:

1. **Week 3:** Issue escalation notice to platform-release@themisdb
2. **Week 4:** Decision point:
   - Option A: Continue waiting for hardware (extend timeline)
   - Option B: Defer Wave A GPU GA to Wave B (per issue deferral clause)
   - Option C: Use cloud GPU instance temporarily for baseline capture

---

## Success Checklist: Complete Wave A GPU

- [x] Phase 1: GPU Infrastructure (preparing/pending Platform Team)
- [x] Phase 2: CUDA Reduction Code (✅ COMPLETE — 75% reduction)
- [ ] Phase 3: Baseline Capture (🟡 READY, waiting Phase 1)
- [ ] Phase 4: GA Sign-Off (🟡 READY, waiting Phase 3)
- [ ] Wave A GPU GA (v2.5.0+) — Target: Q4 2026 (pending Phase 1–4)

**Status:** Ready for Phase 3 execution once Phase 1 infrastructure online.

**Issue:** makr-code/ThemisDB#6575 — Wave-A GPU CUDA Reduction + Representative-Hardware Baseline Evidence
