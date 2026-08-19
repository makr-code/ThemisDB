# acceleration — MODULE_GAPS.md (Batch 4 Wave A Analysis)

**Batch:** Tier 3 Batch 4  
**Wave:** A (Runtime Reliability First)  
**Module:** `src/acceleration` (746 gaps identified)  
**Last Updated:** 2026-08-14  
**Status:** Gap categorization in progress (IMPL vs DOC phase)

## Gap Summary

| Metric | Value |
|---|---|
| **Total Gaps** | ~746 |
| **Implementation Gaps (IMPL)** | ~448 (60%) |
| **Documentation Gaps (DOC)** | ~298 (40%) |
| **Critical Severity** | ~75 |
| **High Severity** | ~224 |
| **Medium Severity** | ~447 |

## Gap Categorization: IMPL vs DOC

### Implementation Gaps (IMPL) — Code/Logic Gaps: ~448

**Categories:**
1. **GPU Memory Management:** ~110 gaps
   - VRAM allocation without size checking
   - GPU memory fragmentation not managed
   - Device memory paging logic incomplete
   - OOM handling on GPU memory exhaustion incomplete
   - Severity: CRITICAL (affects stability)

2. **CUDA Kernel Safety & Execution:** ~120 gaps
   - Unchecked CUDA kernel calls (missing cudaGetLastError)
   - Kernel timeout enforcement missing
   - Exception handling in kernels incomplete
   - CUDA stream management has race conditions
   - Severity: CRITICAL (Batch A4 blocking item)

3. **CPU Fallback Paths & Graceful Degradation:** ~100 gaps
   - GPU failure detection incomplete
   - Fallback trigger logic not tested
   - CPU path seamless takeover incomplete
   - Performance parity between GPU and CPU paths not verified
   - Severity: HIGH (affects availability)

4. **Hardware Detection & Capability Probing:** ~80 gaps
   - GPU availability detection unreliable
   - Compute capability querying incomplete
   - Device selection logic not robust
   - Runtime capability negotiation missing
   - Severity: MEDIUM (affects initialization)

5. **Performance Profiling & Telemetry:** ~38 gaps
   - GPU kernel profiling incomplete
   - Memory usage tracking unreliable
   - Performance regression detection missing
   - Telemetry event emission incomplete
   - Severity: MEDIUM (affects observability)

### Documentation Gaps (DOC) — Documentation/Evidence: ~298

**Categories:**
1. **GPU Memory Architecture Documentation:** ~75 gaps
   - VRAM allocation strategy not documented
   - Memory fragmentation management not specified
   - Device memory paging behavior incomplete
   - OOM error handling semantics not documented
   - Severity: HIGH (critical for deployments)

2. **CUDA Kernel Safety & Timeout Guarantees:** ~70 gaps
   - Kernel execution timeout strategy not specified
   - Exception handling in CUDA kernels not documented
   - Stream management semantics incomplete
   - Unchecked CUDA call elimination strategy incomplete
   - Severity: HIGH (critical for safety)

3. **CPU Fallback & Graceful Degradation:** ~65 gaps
   - GPU failure detection strategy not documented
   - Fallback behavior and performance expectations incomplete
   - Transparent failover semantics not specified
   - Performance parity expectations not documented
   - Severity: MEDIUM (affects SLA design)

4. **Hardware Compatibility & Capability Matrix:** ~50 gaps
   - Supported GPU models not documented
   - Compute capability requirements not specified
   - Device detection algorithm not documented
   - Runtime negotiation behavior incomplete
   - Severity: MEDIUM (affects deployment)

5. **Performance Baseline & Observability:** ~38 gaps
   - GPU kernel performance baselines missing
   - Memory usage baseline expectations missing
   - Telemetry metric definitions incomplete
   - Regression detection thresholds not specified
   - Severity: LOW (affects performance tracking)

## Wave A (Runtime Reliability) Focus Areas

### Critical Path 1: GPU Memory Safety & Allocation (IMPL + DOC)
- [ ] **IMPL Gap:** Implement VRAM allocation with explicit size checking
- [ ] **IMPL Gap:** Implement memory fragmentation management
- [ ] **IMPL Gap:** Implement GPU memory OOM detection and recovery
- [ ] **DOC Gap:** Document VRAM allocation strategy and limits
- [ ] **DOC Gap:** Document memory fragmentation mitigation
- [ ] **Test Gate:** GPU-Mem-01 to GPU-Mem-06 focused tests (allocation, fragmentation, OOM)
- [ ] **Benchmark Gate:** Allocation latency ≤100µs, fragmentation <20% waste, OOM detection ≤1ms
- **Target:** Q3 2026 | **Severity:** CRITICAL

### Critical Path 2: CUDA Kernel Timeout & Exception Safety (IMPL + DOC)
- [ ] **IMPL Gap:** Eliminate all unchecked CUDA calls (add cudaGetLastError after every call)
- [ ] **IMPL Gap:** Implement kernel timeout enforcement (watchdog timer)
- [ ] **IMPL Gap:** Implement exception handling in kernel launch paths
- [ ] **IMPL Gap:** Implement CUDA stream lifecycle management (no race conditions)
- [ ] **DOC Gap:** Document kernel timeout strategy and policies
- [ ] **DOC Gap:** Document CUDA safety guarantees and exception handling
- [ ] **Test Gate:** CUDA-01 to CUDA-08 focused tests (timeout, exception, stream safety, unchecked calls)
- [ ] **Benchmark Gate:** Timeout detection ≤100ms, exception handling overhead <1%
- **Target:** Q3 2026 | **Severity:** CRITICAL

### Critical Path 3: CPU Fallback & Graceful Degradation (IMPL + DOC)
- [ ] **IMPL Gap:** Implement GPU failure detection (automatic or explicit)
- [ ] **IMPL Gap:** Implement seamless CPU fallback (transparent to caller)
- [ ] **IMPL Gap:** Verify CPU path performance parity with GPU (worst-case fallback)
- [ ] **DOC Gap:** Document GPU failure detection strategy
- [ ] **DOC Gap:** Document CPU fallback behavior and latency expectations
- [ ] **Test Gate:** Fallback-01 to Fallback-06 focused tests (failure detection, seamless fallback, parity)
- [ ] **Benchmark Gate:** Fallback latency p99≤10% slower than GPU, detection overhead <5%
- **Target:** Q3 2026 | **Severity:** HIGH

### Critical Path 4: Hardware Detection & Capability Probing (IMPL + DOC)
- [ ] **IMPL Gap:** Implement robust GPU availability detection (CUDA runtime checks)
- [ ] **IMPL Gap:** Implement compute capability querying and validation
- [ ] **IMPL Gap:** Implement device selection logic with fallback
- [ ] **DOC Gap:** Document GPU detection algorithm
- [ ] **DOC Gap:** Document compute capability requirements matrix
- [ ] **Test Gate:** Device-01 to Device-06 focused tests (detection, capability, selection)
- [ ] **Benchmark Gate:** Detection latency ≤100ms, capability query accuracy >99%
- **Target:** Q4 2026 | **Severity:** MEDIUM
- **2026-08-19 update:** `DeviceManager` now supports deterministic injected capability snapshots for focused validation, synthesizes a CPU fallback on empty probe results, and has explicit coverage for cache reuse, refresh re-probe, best-device selection, and observability logging. Remaining work: runtime capability thresholds and benchmark evidence on real GPU hardware.

### Critical Path 5: Performance Profiling & Telemetry (IMPL + DOC)
- [ ] **IMPL Gap:** Implement GPU kernel performance profiling
- [ ] **IMPL Gap:** Implement memory usage tracking and reporting
- [ ] **IMPL Gap:** Implement performance regression detection
- [ ] **DOC Gap:** Document performance baseline expectations
- [ ] **DOC Gap:** Document telemetry metric definitions
- [ ] **Test Gate:** Telemetry-01 to Telemetry-06 focused tests (profiling, tracking, regression detection)
- [ ] **Benchmark Gate:** Profiling overhead <2%, metric accuracy >95%
- **Target:** Q4 2026 | **Severity:** MEDIUM

## Wave A Closure Status

### Test Evidence Gates (Batch 4, Wave A)
- [ ] **ACC-GPU-Mem-01 to ACC-GPU-Mem-06:** GPU memory validation (allocation, fragmentation, OOM)
- [ ] **ACC-CUDA-01 to ACC-CUDA-08:** CUDA kernel validation (timeout, exception, stream, unchecked calls)
- [ ] **ACC-Fallback-01 to ACC-Fallback-06:** CPU fallback validation (detection, seamless failover, parity)
- [ ] **ACC-Device-01 to ACC-Device-06:** Hardware detection validation (detection, capability, selection)
- [ ] **ACC-Telemetry-01 to ACC-Telemetry-06:** Performance telemetry validation (profiling, tracking, regression)
- **Target:** Q3 2026 | **Status:** In Progress

### Benchmark Gates (Batch 4, Wave A)
- [ ] **ACC-GRG-01:** VRAM allocation latency ≤100µs, memory fragmentation <20%
- [ ] **ACC-GRG-02:** Kernel timeout detection ≤100ms, exception handling <1%
- [ ] **ACC-GRG-03:** CPU fallback latency p99 ≤10% slower than GPU
- [ ] **ACC-GRG-04:** GPU detection latency ≤100ms
- [ ] **ACC-GRG-05:** Compute capability query accuracy >99%
- [ ] **ACC-GRG-06:** Performance profiling overhead <2%
- **Target:** Q3 2026 | **Status:** In Progress

## Priority Assessment and Action Plan

### P0 — Wave A Gate Blockers (resolve by Q3 2026 end)
1. **Eliminate unchecked CUDA calls** → Add cudaGetLastError after every CUDA call
2. **GPU memory OOM handling** → Explicit size checking + recovery
3. **Kernel timeout enforcement** → Watchdog timer + graceful shutdown
4. **CPU fallback implementation** → Seamless failover + performance parity validation
5. **GPU failure detection** → Automatic detection + explicit error reporting

### P1 — Post-Wave-A Hardening (Q4 2026)
1. CUDA stream race condition elimination
2. Memory fragmentation optimization
3. Performance profiling integration
4. Hardware capability matrix expansion

## Known Issues & Limitations

1. **CUDA support:** NVIDIA CUDA only; AMD ROCm support pending
2. **Kernel timeout:** Watchdog requires OS-level timer support; implementation platform-specific
3. **Memory fragmentation:** No compaction strategy; relies on allocator heuristics
4. **CPU fallback:** Can introduce 10-100x latency penalty; not suitable for latency-critical workloads
5. **Device selection:** Simple first-available strategy; no advanced topology-aware selection

## Cross-Module Dependencies

| Dependency | Module | Nature | Wave |
|---|---|---|---|
| CUDA runtime (NVIDIA) | external | Dependency for GPU kernel execution | Wave A |
| Host memory management | core | Dependency for pinned memory allocation | Wave A |
| Performance monitoring | observability | Optional for telemetry collection | Wave B |

## Batch 4 Contribution to Program Success

This module contributes to **Wave A (Runtime Reliability)** by:
1. ✅ Eliminating unchecked CUDA calls and ensuring kernel safety
2. ✅ Implementing GPU failure detection and CPU fallback
3. ✅ Proving graceful degradation under GPU failures
4. ✅ Delivering GPU memory safety and timeout guarantees

**Gate Status for Wave A Exit:** 🟡 In Progress (P0 items resolve by Q3 2026 end)

---

**Next Steps:**
1. Execute P0 gap resolution (CUDA safety, memory, timeout, fallback, detection) by EOQ3 2026
2. Deliver focused test gates (ACC-GPU-Mem, ACC-CUDA, ACC-Fallback, ACC-Device, ACC-Telemetry) by EOQ3 2026
3. Benchmark gates must pass at ≥95th percentile by EOQ3 2026
