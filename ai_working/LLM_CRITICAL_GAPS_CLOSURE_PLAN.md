# LLM Module Critical Gaps Closure Plan (2026-08-17)

## Executive Summary

The LLM module has **12,474 total gaps** with **1,400 IMPL gaps** (11% of total).
This plan focuses on closing the **CRITICAL** implementation gaps with high impact:

- **37 files** with documented gaps (TODO=1, Stub=1-3, Mock=1, Sim=0-2 per file)
- **128 circular_lock_ordering** issues → Thread-safety with std::mutex + proper ordering
- **11 data_race** instances → Atomic ops + proper synchronization
- **289+ RAII gaps** (manual_cleanup, delete_no_nullptr, memory leaks)
- **13+ exception_in_destructor** → noexcept guarantees
- **59+ null_dereference** → Defensive null checks

## Critical Gap Categories (Priority Order)

### Category 1: Thread-Safety (CRITICAL) — 128 circular_lock_ordering + 11 data_race
**Estimated Impact:** 50+ files need review
**Risk Level:** HIGH (Deadlocks, data corruption in production)
**Effort:** 2-3 days

Files to Review:
- multi_lora_manager.cpp (20 Critical, 25 High gaps)
- ml_model_manager.cpp (11 Critical, 48 High gaps)
- llama_wrapper.cpp (35 Critical, 76 High gaps)
- production_validator.cpp (2 Critical, 25 High gaps)
- Any file with >10 High/Critical gaps

**Fix Pattern:**
```cpp
// Before (potential race):
void unsafeUpdate() {
    shared_data_ = value;
}

// After (thread-safe):
void safeUpdate(int value) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    shared_data_ = value;
}
```

### Category 2: RAII & Resource Management (HIGH) — 289+ gaps
**Estimated Impact:** All files with resource allocation
**Risk Level:** HIGH (Memory leaks, file handle exhaustion)
**Effort:** 2 days

Subcategories:
- **44 manual_cleanup** → Replace with std::unique_ptr
- **12 delete_no_nullptr** → Add nullptr assignment after delete
- **192 db_connection_leak** → Scoped connection management
- **10 gpu_memory_leak** → GPU RAII wrappers

**Fix Pattern:**
```cpp
// Before (manual cleanup):
MyResource* res = new MyResource();
try {
    // ... use res ...
} catch (...) {
    delete res;  // Easy to forget!
    throw;
}
delete res;

// After (RAII):
{
    auto res = std::make_unique<MyResource>();
    // ... use res ...
}  // Automatically freed on scope exit
```

### Category 3: Exception Safety (HIGH) — 13+ destructors + 61 uncaught exceptions
**Estimated Impact:** All resource-holding classes
**Risk Level:** MEDIUM (Resource leaks on exception)
**Effort:** 1-2 days

**Fix Pattern:**
```cpp
// Before:
~Resource() {
    // WRONG: Can throw on exception path!
    helper_->cleanup();
}

// After:
~Resource() noexcept {
    try {
        helper_->cleanup();
    } catch (...) {
        // Log and suppress
        SPDLOG_ERROR("Cleanup failed: {}", std::current_exception());
    }
}
```

### Category 4: Null Safety (MEDIUM) — 59+ null_dereference
**Estimated Impact:** Function entry validation points
**Risk Level:** MEDIUM (Crashes on invalid input)
**Effort:** 1 day

**Fix Pattern:**
```cpp
// Before:
void process(Obj* obj) {
    obj->method();  // Can crash if obj == nullptr
}

// After:
void process(Obj* obj) {
    if (!obj) {
        throw std::invalid_argument("obj cannot be null");
    }
    obj->method();
}
```

## Implementation Roadmap

### Phase 1: Quick Wins (Batch 1-2) — Day 1
Target: Fix top 20% of gaps that affect test stability

**Batch 1.1: Critical null checks in model loading**
- Files: llama_wrapper.cpp, model_loader.cpp, gguf_loader.cpp
- Changes: Add nullptr guards before dereferencing
- Tests: test_llm_phase1_hardening.cpp (LLM-EXC-01..04)

**Batch 1.2: RAII wrapper for GPU/DB resources**
- Files: gpu_memory_manager.cpp, llm_model_storage.cpp
- Changes: Replace raw pointers with unique_ptr
- Tests: test_active_vram_allocator.cpp, test_llm_caching.cpp

### Phase 2: Thread-Safety (Batch 3-4) — Days 2-3
Target: Eliminate data races and deadlock risks

**Batch 2.1: Mutex-protect shared state**
- Files: multi_lora_manager.cpp, ml_model_manager.cpp
- Changes: Add std::mutex, lock_guard to all shared data
- Tests: Run ThreadSanitizer (TSan)

**Batch 2.2: Atomic ops for simple types**
- Files: Any file with atomic counters
- Changes: Use std::atomic<T> with proper memory ordering
- Tests: Run concurrent workload tests

### Phase 3: Exception Safety (Batch 5-6) — Days 4-5
Target: Ensure no resource leaks on exceptions

**Batch 3.1: noexcept on destructors**
- Files: All resource-holding classes
- Changes: Mark destructors noexcept, add try-catch
- Tests: Exception path tests

**Batch 3.2: Proper exception wrapping**
- Files: Any file with custom exceptions
- Changes: Ensure exceptions propagate cleanly
- Tests: test_llm_phase1_hardening.cpp (LLM-EXC-05..08)

### Phase 4: Testing & Validation (Days 5-6)
Target: 120+ tests passing, 0 memory leaks/races

**Validation Steps:**
1. Build: `cmake --preset community-release && make -j$(nproc)`
2. Unit Tests: `ctest -L llm -V`
3. ASan (leak detection): `cmake --preset community-asan && ctest`
4. TSan (race detection): `cmake --preset develop-tsan && ctest`
5. Performance Gates: `./bench_runner GATE-LLM-01..08`

## Acceptance Criteria

✓ Build: 0 compilation errors, ≤5 warnings
✓ Tests: 120+ LLM tests pass, 0 failures
✓ Memory: 0 leaks (AddressSanitizer clean)
✓ Races: 0 data races (ThreadSanitizer clean)
✓ Performance: All GATE-LLM-01..08 benchmarks pass
✓ Code: Modern C++20, RAII, const-correct, noexcept guarantees

## Risk Mitigation

| Risk | Mitigation | Owner |
|------|-----------|-------|
| Large diff hard to review | Split into focused PRs by subsystem | Agent |
| Regression in performance | Run benchmark gates before/after | CI Pipeline |
| Test suite incomplete | Add new tests for fixed gaps | Test Team |
| Dependency issues | Document all new dependencies | DevOps |

## Dependencies
- C++20 compiler (GCC 11+, Clang 14+, MSVC 2022)
- RocksDB (if using storage backends)
- CUDA toolkit (if GPU support enabled)
- GoogleTest for test compilation

## Next Steps

1. Start with Batch 1.1 (null checks in model loading)
2. Run tests after each batch
3. Escalate failures to human review
4. Tag PR with labels: `llm`, `critical-gaps`, `thread-safety`, `raii`

---

**Generated:** 2026-08-17
**Status:** READY FOR IMPLEMENTATION
**Target Completion:** 2026-08-20
