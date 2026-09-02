# Wave A: GPU CUDA Audit Execution Plan (Sept 7-15, 2026)
## Status: ✅ READY FOR EXECUTION

---

## Executive Summary

**Audit Gate:** GPU AC-L3 — CUDA Wrapper Adoption Reduction (340 → ≤170)
**Timeline:** Sept 7-15, 2026 (9 days)
**Critical Path:** Audit completion unblocks GPU determinism validation + Q3 module integration (Sept 20+)
**Outcome Required:** GPU_CUDA_WRAPPER_ADOPTION_PLAN due Sept 15

---

## Baseline Assessment (Sept 2-5 Snapshot)

### Current State
- **Baseline CUDA Calls:** 340 unchecked direct `cuda*()` calls in `src/gpu/` and `include/gpu/`
- **Wrapper Coverage:** ~40 calls wrapped in RAII guards (CudaStreamGuard, CudaEventGuard, CudaDeviceMemoryGuard)
- **Audit Status:** Initial scan via `tools/ci/gpu_cuda_call_audit.py` (ready)
- **Test Files Existing:**
  - `tests/gpu/test_cuda_wrapper_integration.cpp` (11 tests, stub phase)
  - `tests/gpu/test_cuda_error_hardening.cpp` (24 tests, existing, Wave 5 archive)

### Phase C Target
- **Goal:** Reduce unchecked calls from 340 → ≤170 (50% reduction)
- **Timeline:** Sept 7-15 (audit planning phase) + Sept 16-30 (implementation starts)
- **Success Metric:** All 170 remaining calls have explicit "UNCHECKED: reason" audit comments

### Phase D Target (Post-Wave A)
- **Goal:** Reduce 340 → ≤51 (85% reduction)
- **Timeline:** Q4 2026 (after GPU module hardening complete)

---

## Audit Execution Roadmap (Sept 7-15)

### Phase 1: Baseline Audit Execution (Sept 7-8)

**Objective:** Execute comprehensive CUDA call scan + categorize by wrapper adoption priority

**Tasks:**
1. **Run Audit Tool** (Sept 7, 09:00 UTC)
   - Execute: `python3 tools/ci/gpu_cuda_call_audit.py --repo /home/runner/work/ThemisDB/ThemisDB --output gpu_audit_baseline_2026_09_07.json`
   - Capture output: Full call inventory with file/line/function context
   - Validate: Confirm 340 ± 10% baseline (tolerance for code changes)

2. **Categorize Calls by Type** (Sept 7, 14:00 UTC)
   - Memory allocation: `cudaMalloc`, `cudaMallocHost`, `cudaMemcpy` (estimated: ~80 calls)
   - Stream/Event management: `cudaStreamCreate`, `cudaEventRecord`, `cudaEventSynchronize` (estimated: ~60 calls)
   - Kernel launches: `<<<>>>` (estimated: ~100 calls)
   - Device queries: `cudaGetDeviceProperties`, `cudaGetDevice`, `cudaSetDevice` (estimated: ~50 calls)
   - Synchronization: `cudaDeviceSynchronize`, `cudaStreamSynchronize` (estimated: ~30 calls)
   - Error checking: `cudaGetLastError` (estimated: ~20 calls)

3. **Identify Wrapper Targets** (Sept 8, 09:00 UTC)
   - Tiers (by adoption priority):
     - **TIER 1 (CRITICAL):** Memory allocations + kernel launches (~180 calls)
       - Rationale: Highest resource leak + crash risk
       - Wrapper candidates: `CudaUniquePtr<T>`, `CudaMemoryGuard`, `CudaKernelGuard`
       - Target: ≤100 unchecked after wrapping (55% reduction)
     
     - **TIER 2 (HIGH):** Stream/Event management (~60 calls)
       - Rationale: Resource leaks if not cleaned up
       - Wrapper candidates: `CudaStreamGuard`, `CudaEventGuard`
       - Target: ≤30 unchecked after wrapping (50% reduction)
     
     - **TIER 3 (MEDIUM):** Synchronization + device queries (~80 calls)
       - Rationale: Error handling critical, but lower resource risk
       - Wrapper candidates: `CudaSyncGuard`, annotate with inline audit comments
       - Target: ≤40 unchecked after wrapping (50% reduction)

4. **Generate Audit Report** (Sept 8, 15:00 UTC)
   - Output: `ai_working/GPU_CUDA_BASELINE_AUDIT_2026_09_07.md`
   - Sections:
     - Summary stats (340 total, 40 wrapped, breakdown by tier)
     - Call inventory (JSON matrix: file, line, function, type, priority)
     - Wrapper adoption roadmap (Tier 1/2/3 sequence + targets)
     - Risk assessment (high-risk calls still unchecked)

**Deliverables:**
- ✅ Baseline audit JSON: `gpu_audit_baseline_2026_09_07.json`
- ✅ Audit report markdown: `GPU_CUDA_BASELINE_AUDIT_2026_09_07.md`
- ✅ Wrapper adoption targets: Identified by tier + call count

---

### Phase 2: Wrapper Adoption Strategy Planning (Sept 8-9)

**Objective:** Plan implementation sequence + estimate effort for Sept 16+ execution

**Tasks:**
1. **Design Wrapper Hierarchy** (Sept 8, 16:00 UTC)
   - Define RAII wrapper classes (or extend existing):
     - `CudaUniquePtr<T>`: Auto-free memory (uses cudaFree destructor)
     - `CudaMemoryGuard`: Scoped memory allocation + tracking
     - `CudaStreamGuard`: Scoped stream lifecycle (create → destroy)
     - `CudaEventGuard`: Scoped event lifecycle
     - `CudaKernelGuard`: Kernel launch + error check (returns error code)
     - `CudaSyncGuard`: Synchronization point with timeout + error handling
   - Decision points:
     - Existing wrappers in `include/gpu/cuda_utils.h`? (Wave 2-B created CudaUniquePtr)
     - Compatibility: Can new wrappers reuse existing patterns?
     - Include locations: All in `cuda_utils.h` or split by subsystem?

2. **Map Adoption Sequence** (Sept 9, 09:00 UTC)
   - Sequence by dependency order (low-level first):
     - **Step 1 (Sept 16-18):** Memory allocations (Tier 1a)
       - Wrap all `cudaMalloc` → `CudaUniquePtr<T>`
       - Estimated calls: 50
       - Effort: 3 days (1 FTE)
       - Blockers: None (wrappers exist)
     
     - **Step 2 (Sept 19-20):** Kernel launches (Tier 1b)
       - Add `THEMIS_CUDA_CHECK` after each kernel launch
       - Estimated calls: 100
       - Effort: 2 days (1 FTE)
       - Blockers: None (macro already exists)
     
     - **Step 3 (Sept 21-22):** Stream/Event management (Tier 2)
       - Wrap `cudaStreamCreate` → `CudaStreamGuard`
       - Wrap event pairs → `CudaEventGuard`
       - Estimated calls: 60
       - Effort: 2 days (1 FTE)
       - Blockers: May need new wrapper classes
     
     - **Step 4 (Sept 23-24):** Sync + Device queries (Tier 3)
       - Add audit comments to remaining unchecked calls
       - Estimated calls: 80
       - Effort: 2 days (1 FTE)
       - Blockers: Document rationale for intentional unchecked calls

3. **Identify High-Risk Calls** (Sept 9, 11:00 UTC)
   - Calls with no error checking + resource implications:
     - `cudaMalloc` without corresponding `cudaFree` in same scope
     - Kernel launches without error check before D2H copy
     - Stream creation in loops without explicit cleanup
   - Document remediation priority (must-wrap vs. nice-to-wrap)

4. **Estimate Implementation Effort** (Sept 9, 14:00 UTC)
   - **Phase C (Sept 16-24, 9 days):** Wrap 170+ calls
     - Step 1-2 (memory + kernels): 140 calls, 5 days → 50 unchecked remaining
     - Step 3-4 (streams + audit): 170 calls, 4 days → 170 unchecked target ✅
     - Effort: 1 FTE (sequential steps)
     - Parallel possibility: Steps 1-2 can run in parallel (2 FTE, compress to 3 days)
   
   - **Phase D (Oct 2026, estimated):** Wrap remaining 170 → ≤51
     - Requires GPU module hardening (vectorization, memory pool refactor)
     - Not in Wave A scope; deferred to Q4

---

### Phase 3: Test Strategy Alignment (Sept 9-10)

**Objective:** Validate test coverage for wrapper adoption + define success criteria

**Tasks:**
1. **Review Existing Test Files** (Sept 9, 15:00 UTC)
   - `tests/gpu/test_cuda_wrapper_integration.cpp` (11 tests, stub)
     - Tests: Wrapper RAII lifecycle, error propagation, cleanup on exception
     - Extend with per-tier coverage:
       - T1: Memory allocation edge cases (null, OOM, double-free)
       - T2: Stream/Event lifecycle under concurrent access
       - T3: Sync timeout + fallback behavior
   
   - `tests/gpu/test_cuda_error_hardening.cpp` (24 tests, Wave 5 archive)
     - Review for compatibility with Phase C wrappers
     - May need updates for new error handling patterns

2. **Extend Test Coverage** (Sept 10, 09:00 UTC)
   - New tests needed for Phase C wrappers:
     - `CudaUniquePtr<T>` correctness (null, move semantics, exception safety)
     - `CudaMemoryGuard` tracking + overflow protection
     - `CudaKernelGuard` error propagation + timeout
     - Error code taxonomy (reuse 1100-1199 from INDEX module? or new range?)
   
   - Test matrix (by tier + scenario):
     | Wrapper | Happy Path | Error Case | Concurrency | Exception | Timeout |
     |---------|-----------|-----------|------------|-----------|---------|
     | CudaUniquePtr | ✅ | ✅ | ✅ | ✅ | N/A |
     | CudaStreamGuard | ✅ | ✅ | ✅ | ✅ | ✅ |
     | CudaEventGuard | ✅ | ✅ | ✅ | ✅ | ✅ |
     | CudaKernelGuard | ✅ | ✅ | ✅ | ✅ | ✅ |
   
   - Effort: 2-3 additional tests per wrapper (8 total tests, ~2 days after wrapping)

3. **Define Success Criteria** (Sept 10, 14:00 UTC)
   - All tests pass ✅
   - No memory leaks (valgrind / CUDA compute-sanitizer)
   - Wrapper overhead ≤ 5% on representative workload (measure during Phase 4)
   - 100% of wrapped calls have corresponding test coverage

---

### Phase 4: Risk & Dependency Review (Sept 10-11)

**Objective:** Identify blockers + mitigation strategies before implementation starts

**Tasks:**
1. **Hardware Availability** (Sept 10, 15:00 UTC)
   - **Determinism Testing (AC-L4)** depends on A100 or H100 GPU
   - Status: Not available in current environment (Ubuntu, no GPU hardware)
   - Workaround: 
     - Run determinism tests on CPU mock (emulate CUDA behavior)
     - Defer hardware validation to Sept 20+ after provisioning
     - Document workaround in test code + ROADMAP
   - Impact: **Phase D determinism validation deferred; Phase C (wrapper adoption) unaffected**

2. **Dependency Verification** (Sept 11, 09:00 UTC)
   - Confirm existing RAII wrappers: `include/gpu/cuda_utils.h`
   - Dependency check: Do wrappers already exist or need creation?
     - If exist: Reuse + extend
     - If missing: Add in Phase C, estimate +2-3 days effort
   - Verify wrapper headers included in all GPU source files
   - Check for conflicting wrapper patterns (use consistent naming)

3. **Integration Points** (Sept 11, 11:00 UTC)
   - GPU module subsystems affected by wrapper adoption:
     - `gpu_memory_oversubscription.cpp`: Memory allocation heavy
     - `cuda_hnsw_graph_traversal.cpp`: Kernel launches + stream management
     - `gpu_vector_index.cpp`: Batch operations + error handling
     - `multi_gpu_vector_index.cpp`: Distributed GPU operations
   - Estimate refactoring scope: 6-8 core files × 2-3 hours each = 18 hours (1 FTE day)

4. **Build System Impact** (Sept 11, 13:00 UTC)
   - Check CMakeLists.txt for CUDA compilation flags
   - Verify error checking enabled: `set(CUDA_RUNTIME_LIBRARY shared)`
   - Confirm CUDA library paths in link stage
   - Test compile with wrapper changes early (Sept 14, before full implementation)

---

### Phase 5: Execution Plan + Checkpoints (Sept 11-15)

**Objective:** Finalize weekly checkpoints + schedule synchronization

**Weekly Schedule:**

**Week 1 (Sept 7-13): Audit + Planning**
- Sept 7: Baseline audit execution (target: confirm 340 calls)
- Sept 8: Categorize calls by type + tier
- Sept 9: Wrapper strategy + adoption sequence finalized
- Sept 10: Test strategy defined + success criteria documented
- Sept 11: Risk review + blockers identified
- Sept 12: GPU team kickoff meeting (review plan + confirm capacity)
- Sept 13: Build verification test (compile with sample wrapper changes)

**Week 2 (Sept 14-15): Final Planning + Go-Live Prep**
- Sept 14: Resolve any blockers from Sept 13 testing
- Sept 15: Generate final ADOPTION_PLAN document
- Sept 15: Sign-off from GPU module owner

**Execution Checkpoints (Daily Sept 16+):**
- Morning (09:00 UTC): Status update (calls wrapped, tests passing, blockers)
- Afternoon (15:00 UTC): Code review + merge (daily checkpoint)
- Weekly (Friday 14:00 UTC): Rollup (cumulative progress, risk assessment)

---

## GPU_CUDA_WRAPPER_ADOPTION_PLAN Deliverable (Sept 15)

**Document:** `ai_working/GPU_CUDA_WRAPPER_ADOPTION_PLAN_2026_09_15.md`

**Sections:**
1. Executive Summary (baseline 340 → target 170, success criteria)
2. Call Inventory (categorized by type/tier, estimated effort per tier)
3. Wrapper Hierarchy (classes, interfaces, error codes)
4. Adoption Sequence (Step 1-4, timeline, effort, FTE allocation)
5. Test Strategy (coverage matrix, new tests needed, success metrics)
6. Risk Assessment (hardware, dependencies, integration points)
7. Implementation Checklist (daily tracking template for Sept 16+)
8. Rollback Strategy (how to undo wrapper changes if needed)
9. Success Metrics (50% reduction verified, all tests green, overhead ≤5%)

**Appendices:**
- A: Full call inventory JSON (340 calls, categorized)
- B: Wrapper API documentation (class signatures, error codes)
- C: Test file template (pytest-compatible for GPU tests)
- D: Integration guide (per-file wrapper adoption steps)

---

## Critical Decisions (Ready for Sept 12 Kickoff)

| Decision | Options | Recommendation | Rationale |
|----------|---------|---|---|
| **Wrapper Reuse** | Extend existing / Create new | Extend existing (`CudaUniquePtr` exists) | Minimize rework + maintain consistency |
| **Error Codes** | New range / Reuse INDEX (1100-1199) | New range (1500-1599 for GPU) | Avoid collisions + clear module attribution |
| **Priority Sequence** | Memory-first / Kernel-first | Memory-first (Tier 1a/1b parallel) | Memory leaks most critical risk |
| **Hardware Plan** | Block Phase C / Use CPU mock | Use CPU mock + defer hardware to Sept 20 | Phase C (wrapper adoption) independent of hardware |
| **Parallel vs. Sequential** | Sequential (1 FTE) / Parallel (2 FTE) | Parallel (compress 9 days → 5-6 days) | Meets Sept 15 deadline + reduces schedule risk |

---

## Resource Allocation (Sept 7-15)

**Team:** 1-2 FTE
- **Lead Auditor/Planner** (1 FTE): Run audit, categorize calls, design wrappers, write plan
- **Test Engineer** (0.5 FTE): Review test files, extend coverage, build verification
- **Reviewer** (0.5 FTE, ad-hoc): Risk validation, blockers resolution

**Time Breakdown:**
- Audit execution + categorization: 8 hours (Sept 7-8)
- Wrapper design + strategy: 6 hours (Sept 8-9)
- Test review + planning: 4 hours (Sept 9-10)
- Risk + dependency review: 5 hours (Sept 10-11)
- Team sync + kickoff: 3 hours (Sept 12)
- Build verification + final plan: 4 hours (Sept 13-15)

**Total:** 30 hours (3-4 days FTE)

---

## Success Criteria (Sept 15, 18:00 UTC)

✅ **All must be true:**
- [ ] Audit baseline captured: 340 ± 10% calls confirmed
- [ ] Call inventory categorized: By type (memory, kernel, stream, sync, etc.)
- [ ] Wrapper strategy designed: Classes defined + error codes allocated
- [ ] Adoption sequence finalized: Step 1-4 with effort/FTE/timeline estimates
- [ ] Test strategy aligned: Coverage matrix + new test count defined
- [ ] Risk assessment complete: Hardware, dependencies, integration points documented
- [ ] GPU_CUDA_WRAPPER_ADOPTION_PLAN generated: All sections complete + signed off
- [ ] GPU module owner confirmed: 1-2 FTE available Sept 16-24 for implementation

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| Actual baseline ≠ 340 | LOW | LOW | Audit tool ±10% tolerance; recount if outlier |
| Wrapper classes don't exist | MEDIUM | MEDIUM | Check `include/gpu/cuda_utils.h` Sept 7; if missing, allocate +3 days to create |
| Integration complexity higher than estimated | MEDIUM | MEDIUM | Build verification Sept 13 will surface issues; adjust Sept 14 if needed |
| GPU hardware unavailable (determinism) | HIGH | LOW | Use CPU mock for Phase C tests; defer hardware validation to Sept 20+ |
| Implementation team capacity unavailable Sept 16 | LOW | HIGH | Confirm Sept 12 kickoff; if unavailable, request deferral to Oct 1 |
| Wrapper overhead > 5% | LOW | MEDIUM | Profile Sept 21; if exceeded, optimize wrapper implementation Sept 23-24 |

---

## Next Steps

### Pre-Audit (Sept 2-6)
- [ ] Team notified: GPU audit planning starts Sept 7
- [ ] Audit tool ready: `tools/ci/gpu_cuda_call_audit.py` tested locally
- [ ] Test files available: `tests/gpu/test_cuda_wrapper_integration.cpp` + `test_cuda_error_hardening.cpp` reviewed
- [ ] Access confirmed: Can run audit on full repo + generate outputs

### During Audit (Sept 7-15)
- [ ] Daily logs captured: Audit findings + categorization progress
- [ ] Risk identified: Blockers logged daily, mitigations documented
- [ ] Decisions made: Wrapper strategy finalized by Sept 9
- [ ] Plan signed off: GPU module owner approves by Sept 15, 18:00 UTC

### Post-Audit (Sept 16+)
- [ ] Kickoff executed: Sept 16, 09:00 UTC (15 min sync on implementation team assignment)
- [ ] Implementation starts: Step 1 (memory allocations) begins immediately
- [ ] Daily standups: 15 min each, track calls wrapped + tests passing
- [ ] Weekly rollups: Friday, cumulative progress + risk management

---

## Prepared By
- **Facilitator:** Copilot Coding Agent
- **Date:** Sept 2, 2026, 14:18 UTC
- **Session:** Wave A GPU CUDA Audit Execution Planning
- **Gate:** Critical path for GPU Phase C (wrapper adoption 340→170)

**Status:** ✅ READY FOR KICKOFF (Sept 7, 2026, 09:00 UTC)

