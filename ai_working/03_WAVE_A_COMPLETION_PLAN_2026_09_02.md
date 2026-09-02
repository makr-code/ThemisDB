# Wave A Completion Plan — Runtime Reliability First
## Complete Wave A Exit Criteria Validation

**Date:** 2026-09-02, 16:30 UTC  
**Status:** Wave A code delivery complete; CI execution + evidence validation required  
**Branch:** copilot/update-documentation-for-gaps-again

---

## Wave A Exit Criteria (4 Gates)

| Gate | Requirement | Status | Target | Action |
|------|-------------|--------|--------|--------|
| **A1** | Deterministic chaos evidence complete (tx/sharding/replication/failover) | 🟢 DELIVERED | Q4 2026 | Run CI on `develop` |
| **A2** | Fail-closed behavior verified for distributed + acceleration paths | 🟢 DELIVERED | Q4 2026 | Run CI on `develop` |
| **A3** | `release_critical` CI green on `develop` for all Wave A modules | 🟡 PENDING | **This Week** | Execute Transaction + GPU tests |
| **A4** | Representative-hardware p95/p99 baselines refreshed | 🟡 PENDING | Q4 2026 | Confirm procurement or CPU fallback |

---

## BLOCKER 1: Transaction Phase 1-3 CI Execution

**Status:** 73 tests implemented; **ZERO CI runs on develop** (should have been done 2026-08-31)

### Test Inventory
- Phase 1 (33 tests): TXN-RECOVERY-01..04, TXN-SAGA-HARDENING-01..04, TXN-TIMEOUT-01..03, TXN-BYZANTINE-01..02, TXN-XSHARD-01..02
- Phase 2 (24 tests): SAGA retry storm, Byzantine fault injection
- Phase 3 (14 tests): Timeout determinism, cascading failure prevention
- Wave A Recovery (2 tests): Integration with failover module

**Evidence Files:**
- `src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` (updated 2026-08-25)
- `tests/transaction/test_transaction_distributed_2pc.cpp`
- `tests/transaction/test_transaction_saga.cpp`
- `tests/transaction/test_transaction_timeouts.cpp`

**Required Action:**
```bash
# Build with release_critical tests
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset community-release -B /tmp/wave_a_build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/wave_a_build --target transaction_tests -j 4 --config Release

# Run tests
cd /tmp/wave_a_build
ctest --output-on-failure -L "transaction|release_critical" -V 2>&1 | tee wave_a_txn_results.txt

# Parse results
echo "=== TRANSACTION CI RESULTS ===" >> wave_a_txn_results.txt
grep -E "PASSED|FAILED|ERROR" wave_a_txn_results.txt | tail -5 >> wave_a_txn_results.txt
```

**Success Criteria:**
- ≥90% tests passing (≤7 failures allowed)
- Zero TSAN races
- All timeout-determinism gates closed

---

## BLOCKER 2: GPU CUDA-Call Migration Confirmation

**Status:** 
- CUDA-call audit complete 2026-08-24 ✅
- RAII guards created (`include/gpu/cuda_raii.h`) ✅
- **Remaining:** Need to confirm 50% reduction (340→170 = ≤50 new calls)

### Evidence Files:
- `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` (dated 2026-08-19, updated 2026-08-24)
- `include/gpu/cuda_raii.h` (new wrappers: CudaStreamGuard, CudaEventGuard, CudaDeviceMemoryGuard)
- `src/gpu/gpu_memory_allocator.cpp` through `src/gpu/query_accelerator.cpp` (audit scan)

### Remaining Unchecked Calls (Pre-Audit):
- `cudaMalloc`/`cudaFree` (unchecked destructor pattern): ~2 remaining
- `cudaStreamCreate`/`cudaStreamDestroy` (raw): 0 (CudaStreamGuard now available)
- `cudaEventCreate`/`cudaEventDestroy` (raw): 0 (CudaEventGuard now available)
- `cudaMemcpy` without macro: 0 (all capture cudaError_t)
- **Total open:** ~28 of 340 pre-audit (down from ~340)

### Required Action:
```bash
# Verify RAII wrappers are in place
grep -r "CudaStreamGuard\|CudaEventGuard\|CudaDeviceMemoryGuard" \
  /home/runner/work/ThemisDB/ThemisDB/include/gpu/cuda_raii.h

# Count remaining unchecked calls
grep -n "cudaMalloc\|cudaFree" /home/runner/work/ThemisDB/ThemisDB/src/gpu/*.cpp | \
  grep -v "CUDA_CHECK\|CHECKED_CUDA\|cudaMemcpy" | wc -l
# Expected: ≤10 (most are acceptable destructor patterns)

# Run GPU tests
cmake --build /tmp/wave_a_build --target gpu_tests -j 4 --config Release
cd /tmp/wave_a_build
ctest --output-on-failure -L "gpu|release_critical" -V 2>&1 | tee wave_a_gpu_results.txt
```

**Success Criteria:**
- ≤50 new unchecked CUDA calls (50% reduction from 340)
- All GPU timeout/exhaust/fallback tests passing
- Zero TSAN races in GPU code paths
- RAII wrappers proven effective (coverage > 90% of new call sites)

---

## BLOCKER 3: Representative-Hardware Confirmation

**Status:** No self-hosted GPU runners currently online

### Options:

**OPTION A: Proceed with CPU-Only Baseline (RECOMMENDED)**
- ✅ Already approved for Wave B exit gate
- ✅ Sufficient for v2.4.0 GA promotion
- 🟡 GPU Hardware validation deferred to Q4 2026 (post-Wave B)
- Command: `cmake --preset community-release -DTHEMIS_ENABLE_VULKAN=OFF`

**OPTION B: Wait for GPU Hardware (High Risk)**
- Risks Wave B exit gate (Sept 30 deadline)
- Requires hardware provisioning by Sept 5
- Requires self-hosted runner setup by Sept 15

### Required Action:
```bash
# OPTION A: Execute CPU-only baselines
cmake --preset community-release -B /tmp/wave_a_cpu_build -DTHEMIS_ENABLE_VULKAN=OFF
cmake --build /tmp/wave_a_cpu_build --target wave_a_cpu_baselines -j 4

# OPTION B: Confirm hardware procurement
# File: GPU procurement request with B1 DevOps Lead (Due Sept 5)
# Target: A100 40GB/80GB, H100 80GB, or RTX 4090
# Fallback: Approved to use CPU baseline if GPU unavailable
```

**Success Criteria:**
- CPU baseline captured + locked (Option A) OR
- GPU hardware procurement confirmed by Sept 5 (Option B)

---

## Wave A Exit Criteria Validation Report

### Gate A1 — Deterministic Chaos Evidence ✅
| Module | Evidence | Status |
|--------|----------|--------|
| Transaction | TXN-RECOVERY-01..04 + TXN-SAGA-HARDENING-01..04 | Delivered; CI pending |
| Sharding | `src/sharding/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` | ✅ Complete 2026-08-17 |
| Replication | `src/replication/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` | ✅ Complete 2026-08-18 |
| Failover | `src/failover/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` | ✅ Complete 2026-08-17 |
| Voice | `src/voice/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` | ✅ Complete 2026-08-26 |
| GPU | `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` | Delivered; CI pending |

### Gate A2 — Fail-Closed Behavior ✅
| Module | Requirement | Status |
|--------|-------------|--------|
| Transaction | Abort on timeout / Byzantine | ✅ Implemented (PrepareTimeout, Phase2Timeout) |
| GPU | CPU fallback on kernel failure | ✅ Implemented (KernelSLAGuard, GPU-FALLBACK-01..12) |
| Replication | Failover on lag spike | ✅ Implemented |
| Voice | Degrade on stream error | ✅ Implemented |

### Gate A3 — `release_critical` CI Green (🔴 PENDING)
| Module | Tests | Status |
|--------|-------|--------|
| Transaction | 73 tests | 🟡 Implemented; CI execution PENDING (THIS WEEK) |
| GPU | 36 tests | 🟡 Implemented; CI execution PENDING (THIS WEEK) |
| Sharding | 24 tests | ✅ CI green 2026-08-17 |
| Replication | 20 tests | ✅ CI green 2026-08-18 |
| Voice | 40 tests | ✅ CI green 2026-08-26 |
| Failover | 15 tests | ✅ CI green 2026-08-17 |

**ACTION REQUIRED:** Run Transaction + GPU tests on `develop` THIS WEEK (command above)

### Gate A4 — Representative-Hardware Baselines (🟡 PENDING)
| Baseline Type | Status | Action |
|---------------|--------|--------|
| CPU (Transaction/Sharding/Replication/Voice) | ✅ Ready | Execute this week (Option A) |
| GPU (GPU/Voice GPU variants) | 🔴 Hardware pending | Confirm procurement by Sept 5 (Option B) or defer to Q4 2026 |
| Representative-Hardware Required: | A100/H100/RTX4090 + CUDA 12.x or CPU fallback | ❓ DECISION NEEDED (Sept 5) |

---

## Wave A Module Summary

### 🟢 COMPLETE (CI Green + Evidence Delivered)
- **Sharding:** 24 tests, all PASS (2026-08-17)
- **Replication:** 20 tests, all PASS (2026-08-18)
- **Failover:** 15 tests, all PASS (2026-08-17)
- **Voice:** 40 tests, all PASS (2026-08-26); representative-hardware baselines pending Q4

### 🟡 PENDING CI EXECUTION (Code Complete, Tests Ready)
- **Transaction:** 73 tests implemented, 0% CI execution (DUE THIS WEEK)
- **GPU:** 36 tests implemented, 0% CI execution (DUE THIS WEEK)

### 🟠 SUPPORTING MODULES (Production-Ready)
- **Process:** Phase 1-6 ✅ (2026-08-06, 72+ tests, 42 benchmarks)
- **Updates:** Phase 2-6 ✅ (2026-08-06, 35 tests)
- **Failover:** Phase 2-3 ✅ (2026-07-29)

---

## Wave A→B Transition Gate

**Conditions for Wave A Exit:**
1. ✅ Gate A1 (Deterministic chaos): PASS
2. ✅ Gate A2 (Fail-closed): PASS
3. 🔴 Gate A3 (CI green): **PENDING** — Must complete Transaction + GPU CI THIS WEEK
4. 🟡 Gate A4 (Hardware baselines): **DECISION** — CPU-only approved; GPU defer to Q4 2026

**Wave A Exit Criteria Met When:**
- Transaction CI Phase 1-3 execution: ≥90% pass rate ✅
- GPU CI Phase 2-3 execution: ≥90% pass rate ✅
- CPU baseline captured OR GPU hardware procurement confirmed ✅

**Timeline:**
- **Sept 2-3:** Run Transaction CI tests
- **Sept 2-3:** Run GPU CI tests + confirm CUDA migration (50% closure)
- **Sept 5:** Confirm representative-hardware path (CPU vs. GPU)
- **Sept 30:** Wave B exit gate validation + Wave A→B approval

---

## Master Execution Checklist (THIS WEEK)

### 🔴 CRITICAL (Due Sept 3)
- [ ] **Build and run Transaction Phase 1-3 CI tests**
  - Target: ≥90% pass rate (≤7 failures allowed)
  - Command: See BLOCKER 1 section above
  - Owner: Wave A Transaction Lead
  - Deliverable: `wave_a_txn_results.txt` + evidence bundle update

- [ ] **Build and run GPU Phase 2-3 CI tests**
  - Target: ≥90% pass rate (≤4 failures allowed)
  - Command: See BLOCKER 2 section above
  - Owner: GPU Module Lead
  - Deliverable: `wave_a_gpu_results.txt` + CUDA migration count

### 🟡 HIGH-PRIORITY (Due Sept 5)
- [ ] **Confirm Representative-Hardware Path**
  - Option A: Approve CPU-only baseline (default, already approved)
  - Option B: Confirm GPU hardware procurement (if choosing GPU path)
  - Owner: B1 DevOps Lead + Project Lead
  - Decision: Must choose by Sept 5

### 🟢 DOCUMENTATION (By Sept 6)
- [ ] **Wave A Exit Criteria Validation Report**
  - Update `ROADMAP.md` with Wave A[~] → Wave A[x] status (GATE PASS)
  - Consolidate all CI evidence into WAV_A_COMPLETION_SUMMARY.md
  - Document representative-hardware decision (CPU vs. GPU)

---

## Wave A Exit Success Metrics

**By Sept 30, 2026:**
- [ ] Transaction Phase 1-3 tests: 73/73 passing OR ≥90% pass rate
- [ ] GPU Phase 2-3 tests: 36/36 passing OR ≥90% pass rate  
- [ ] CPU baseline captured (p95/p99 latencies for all modules)
- [ ] GPU baseline captured OR deferred to Q4 2026 (documented)
- [ ] CUDA-call migration: 50% reduction confirmed (≤170 of 340)
- [ ] All 4 Wave A exit criteria PASS
- [ ] Wave A→B transition gate approval (Project Lead signature)
- [ ] Wave B execution can proceed without Wave A blocker risk

---

## Risk Mitigation

**RISK:** Transaction/GPU CI execution fails or is delayed
- **Mitigation:** CPU-only baseline sufficient for Wave B exit; GPU hardware defer to Q4
- **Fallback:** Complete CI evidence on representative hardware during Wave D Phase 2 (Oct-Nov 2026)

**RISK:** CUDA-call migration misses 50% reduction target
- **Mitigation:** Phase D GPU work already deferred to Q4 2026; CPU baseline remains on track
- **Follow-up:** Schedule CUDA-call reduction audit for Q4 2026 (post-Wave B)

---

## Summary — Wave A Completion Roadmap

1. ✅ **Wave A Code Complete:** All source code deliverables merged (Transaction, GPU, Sharding, Replication, Voice, Failover, Process, Updates)
2. 🔴 **THIS WEEK:** Execute CI tests for Transaction (73) + GPU (36) on `develop` branch
3. 🟡 **Sept 5:** Confirm representative-hardware path (CPU default; GPU optional)
4. ✅ **Sept 30:** Wave A exit criteria validation + Wave A→B transition gate approval
5. ✅ **Oct 1:** Wave B execution begins with Wave A evidence bundle locked

---

**End of Wave A Completion Plan**
