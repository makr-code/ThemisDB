# BATCH 5 - QUICK REFERENCE: GAP INVENTORY & FIX CHECKLIST

**Generated:** 2026-08-17  
**Total Gaps:** 67 across 6 files  
**Status:** Analysis Complete - Ready for Implementation

---

## CRITICAL GAPS (BLOCKING RELEASE) - 2 TOTAL

### 1. stub #289: GGUF Model Fallback Missing
- **File:** lora_training_service.cpp:1851
- **Function:** `LoRATrainingService::loadModelFromGGUF()`
- **Issue:** Production inference fails without fallback when GGUF missing/corrupted
- **Impact:** BLOCKING - GA release cannot proceed
- **Fix Effort:** 2-3 days
- **Status:** [ ] NOT STARTED
- **Notes:** Implement fallback factory with caching + synthetic model generation

### 2. REL-73: GPU Memory Not Cleared on Device Error
- **File:** gpu_memory_manager.cpp:124
- **Function:** `MemoryHolder::freeGPUMemory()`
- **Issue:** Sensitive data left in VRAM when device switch fails
- **Impact:** BLOCKING - Security vulnerability
- **Fix Effort:** 1 day
- **Status:** [ ] NOT STARTED
- **Notes:** Add fallback CPU clear when device unavailable

---

## HIGH-SEVERITY GAPS (18 TOTAL) - PRIORITY FIXES

### GPU Memory Management (2 gaps)

| Ref | File | Line | Issue | Fix Effort | Status |
|-----|------|------|-------|-----------|--------|
| REL-66 | gpu_memory_manager.cpp | 1147 | cudaFree error not handled | 1 day | [ ] |
| REL-73 | gpu_memory_manager.cpp | 124 | cudaSetDevice error in cleanup | 1 day | [ ] |

### Device Initialization (2 gaps)

| Ref | File | Line | Issue | Fix Effort | Status |
|-----|------|------|-------|-----------|--------|
| REL-40 | multi_gpu_memory_coordinator.cpp | 71 | CUDA device set error | 1 day | [ ] |
| REL-41 | multi_gpu_memory_coordinator.cpp | 131 | HIP device set error | 1 day | [ ] |

### P2P Access Setup (4 gaps) - TRANSACTIONAL

| Ref | File | Line | Issue | Fix Effort | Status |
|-----|------|------|-------|-----------|--------|
| REL-42 | multi_gpu_memory_coordinator.cpp | 350 | CUDA P2P forward error | 2 days | [ ] |
| REL-43 | multi_gpu_memory_coordinator.cpp | 373 | CUDA P2P backward error | (with REL-42) | [ ] |
| REL-44 | multi_gpu_memory_coordinator.cpp | 427 | HIP P2P forward error | 2 days | [ ] |
| REL-45 | multi_gpu_memory_coordinator.cpp | 447 | HIP P2P backward error | (with REL-44) | [ ] |

**Note:** These 4 gaps form a transaction - fix all or none. Recommended: Implement unified retry framework.

### Synchronization Verification (2 gaps)

| Ref | File | Line | Issue | Fix Effort | Status |
|-----|------|------|-------|-----------|--------|
| REL-46 | multi_gpu_memory_coordinator.cpp | 598 | CUDA sync without device check | 1 day | [ ] |
| REL-47 | multi_gpu_memory_coordinator.cpp | 615 | HIP sync without device check | 1 day | [ ] |

### NCCL Collective Communication (4 gaps)

| Ref | File | Line | Issue | Fix Effort | Status |
|-----|------|------|-------|-----------|--------|
| REL-69 | nccl_backend.cpp | 151 | ncclGroupEnd error on exit | 1 day | [ ] |
| REL-70 | nccl_backend.cpp | 161 | ncclGroupEnd error on success | 1 day | [ ] |
| REL-11 | nccl_backend.cpp | 236 | Missing stream sync in broadcast | 1 day | [ ] |
| REL-12 | nccl_backend.cpp | 281 | Missing stream sync in barrier | 1 day | [ ] |

### RCCL Collective Communication (4 gaps) - HIP/ROCM Variants

| Ref | File | Line | Issue | Fix Effort | Status |
|-----|------|------|-------|-----------|--------|
| REL-72 | rccl_backend.cpp | 160 | ncclGroupEnd error on success (HIP) | 1 day | [ ] |
| REL-15 | rccl_backend.cpp | 235 | Missing stream sync in broadcast (HIP) | 1 day | [ ] |
| REL-16 | rccl_backend.cpp | 280 | Missing stream sync in barrier (HIP) | 1 day | [ ] |
| REL-71 | rccl_backend.cpp | 121 | Missing ncclGroupStart check | 1 day | [ ] |

---

## MEDIUM-SEVERITY GAPS (47 TOTAL) - BATCH FIXES

### Group A: Simulation Fallback Paths (43 gaps)

**Distribution:**
- gpu_memory_manager.cpp: 24 gaps (lines: 470, 618, 701, 1545, 1782, 1820, 1856, 2356, ...)
- multi_gpu_memory_coordinator.cpp: 7 gaps (various fallback conditions)
- lora_training_service.cpp: 2 gaps (CPU training path)
- gpu_utilization_monitor.cpp: 2 gaps (device unavailable fallbacks)
- rccl_backend.cpp: 4 gaps (HIP unavailable fallbacks)
- nccl_backend.cpp: 4 gaps (CUDA unavailable fallbacks)

**Unified Approach:**
1. Add SIMULATION/PRODUCTION mode markers
2. Document fallback behavior and production delta
3. Test both GPU and CPU paths
4. Update Doxygen documentation

**Fix Effort:** 5 days (bulk fix with consistent pattern)  
**Status:** [ ] NOT STARTED  

**Example Pattern:**
```cpp
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // PRODUCTION: Real GPU path
    } else {
        // SIMULATION: CPU fallback (no GPU detected)
    }
#else
    // SIMULATION: CUDA not compiled
#endif
```

### Group B: Stub Methods (10 gaps)

| Stub ID | File | Line | Method | Issue | Fix Effort |
|---------|------|------|--------|-------|------------|
| #309 | gpu_memory_manager.cpp | 331 | setNvmlTemperatureFn() | NVML temperature injection | 1 day |
| #289 | lora_training_service.cpp | 53 | setModelPathProviderFn() | Model path fallback | 3 days* |
| #310-313 | gpu_utilization_monitor.cpp | 120-400 | queryNVML/queryROCm/queryVulkan/queryDirectX | Query implementation | 2 days |
| #320 | nccl_backend.cpp | 100-350 | initialize_nccl() | NCCL initialization | 2 days |
| #321 | rccl_backend.cpp | 100-350 | initialize_rccl() | RCCL initialization | 2 days |

*Note: #289 is CRITICAL, see above. Others are MEDIUM priority.

---

## IMPLEMENTATION PHASES & SCHEDULE

### Phase 1: CRITICAL & HIGH (Week 1, ~20 days)

**Must Complete Before GA:**
- [ ] stub #289 - Model fallback chain
- [ ] REL-73 - GPU cleanup fallback
- [ ] REL-66 - cudaFree error handling

**Must Complete for Stability (v1.0.1):**
- [ ] REL-40, 41 - Device init retry
- [ ] REL-42-45 - P2P error handling (transactional set)
- [ ] REL-46, 47 - Sync verification
- [ ] REL-69, 70, 72 - NCCL/RCCL group end checks
- [ ] REL-11, 12, 15, 16 - Stream sync verification

**Estimated Timeline:**
- Days 1-4: stub #289, REL-73, REL-66 (CRITICAL)
- Days 5-6: REL-40, 41 (Device init)
- Days 7-9: REL-42-45 (P2P setup - transactional)
- Day 10: REL-46, 47 (Sync)
- Days 11-12: REL-69, 70, 72 (Group end)
- Day 13: REL-11, 12, 15, 16 (Stream sync)
- Days 14-20: Testing + integration validation

### Phase 2: MEDIUM Stubs (Week 2-3, ~16 days)

- [ ] #309 - NVML temperature function
- [ ] #310-313 - GPU query implementations
- [ ] #320 - NCCL initialization
- [ ] #321 - RCCL initialization

### Phase 3: MEDIUM Simulation Paths (Week 3-4, ~10 days)

- [ ] Mark all 43 simulation paths with consistent SIMULATION/PRODUCTION pattern
- [ ] Test both GPU and CPU paths
- [ ] Update documentation

### Phase 4: Validation & Documentation (Week 4-5, ~10 days)

- [ ] Comprehensive testing (unit + integration)
- [ ] Performance benchmarking
- [ ] Update API documentation
- [ ] Create migration guide for users

---

## COMMIT STRATEGY

**Batch Commits (50+ gaps per commit as requested):**

### Commit 1: CRITICAL + HIGH Device Management (20 gaps)
```
git commit -m "Fix Batch 5 CRITICAL/HIGH GPU device management

- stub #289: Implement GGUF model fallback chain with caching
- REL-73: Add GPU cleanup fallback when device unavailable
- REL-66: Add error handling for cudaFree in defrag path
- REL-40/41: Add retry logic for device initialization
- REL-42-45: Add error handling for P2P access setup (CUDA+HIP)
- REL-46/47: Add device verification before synchronization
- REL-69/70: Add ncclGroupEnd verification for NCCL allreduce
- REL-72: Add ncclGroupEnd verification for RCCL
- REL-11/12/15/16: Add stream sync verification

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
"
```

### Commit 2: MEDIUM Stub Implementations (10 gaps)
```
git commit -m "Fix Batch 5 MEDIUM stub implementations

- #309: Implement NVML temperature injection function
- #310-313: Implement GPU utilization query backends (NVML/ROCm/Vulkan/DirectX)
- #320: Implement NCCL backend initialization
- #321: Implement RCCL backend initialization

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
"
```

### Commit 3: MEDIUM Simulation Path Cleanup (43 gaps)
```
git commit -m "Fix Batch 5 MEDIUM simulation path marking

- Add consistent SIMULATION/PRODUCTION mode markers (43 paths)
- Document fallback behavior and production deltas
- Add simulation mode testing coverage
- Update Doxygen documentation for all paths

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
"
```

**Total: 3-4 commits covering 67 gaps**

---

## VALIDATION CHECKLIST

Before Each Commit:

- [ ] Code compiles without errors
- [ ] Doxygen comments correct and updated
- [ ] Unit tests pass (CPU mode)
- [ ] Unit tests pass (GPU mode if available)
- [ ] ASan clean (no address sanitizer warnings)
- [ ] UBSan clean (no undefined behavior)
- [ ] TSan clean (no thread sanitizer warnings)
- [ ] No performance regression vs baseline
- [ ] Code review approved

---

## DEPENDENCY NOTES

### Internal Dependencies

1. **stub #289** depends on caching infrastructure - may need LRU cache implementation
2. **REL-40/41** should use unified retry framework
3. **REL-42-45** form transaction - fix together
4. **REL-46-47** use results from device init
5. **Simulation paths** should follow consistent pattern

### External Dependencies

- CUDA Runtime (cudaSetDevice, cudaMalloc, cudaFree, etc.)
- NCCL (ncclGroupStart, ncclGroupEnd, nccl communication)
- HIP Runtime (hipSetDevice, hipMalloc, etc.)
- RCCL (rocm collective communication)
- NVML (NVIDIA GPU monitoring)
- ROCm SMI (AMD GPU monitoring)

---

## METRICS & SUCCESS MEASURES

**Batch 5 Success:**
- ✅ 67/67 gaps identified and categorized
- ✅ 2/2 CRITICAL gaps fixed and tested
- ✅ 18/18 HIGH severity gaps fixed and tested
- ✅ 47/47 MEDIUM gaps marked or fixed
- ✅ Zero new compiler warnings
- ✅ Zero new ASan/UBSan/TSan errors
- ✅ 100% test pass rate
- ✅ No performance regression
- ✅ Full documentation updated

---

## RISK MATRIX

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| P2P setup deadlock (REL-42-45) | MEDIUM | CRITICAL | Comprehensive multi-GPU tests with stress scenarios |
| Memory corruption in collective ops | LOW | CRITICAL | Extensive validation + data integrity checks |
| Device driver incompatibility | LOW | HIGH | Test with multiple driver versions |
| Performance regression | LOW | MEDIUM | Benchmark before/after each phase |
| Schedule slip | MEDIUM | MEDIUM | Parallel implementation of independent fixes |

---

## REFERENCE LINKS

- **Full Analysis:** BATCH_5_GAP_ANALYSIS_AND_FIX_PLAN.md
- **Gap Tracking:** Track in sql todos table (per user preference)
- **CI/CD:** GitHub Actions workflows in .github/workflows/
- **Build:** CMake presets in CMakePresets.json

---

**Last Updated:** 2026-08-17  
**Next Review:** After Phase 1 completion

