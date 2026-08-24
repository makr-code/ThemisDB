# Immediate Action Items for LLM Critical Gaps (2026-08-17)

## Summary
The LLM module analysis revealed **1,400 IMPL gaps** concentrated in 37 files.
Focus on **highest-impact** fixes that will enable tests to pass and sanitizers to run clean.

## Critical Issues Identified

### Issue 1: RocksDB TransactionDB Not Initialized (llm_plugin_manager.cpp:880)
**Status:** TODO comment exists, needs implementation
**Impact:** MEDIUM - SSM state store won't work correctly
**Files:**
- src/llm/llm_plugin_manager.cpp (line 880)

**Current Code:**
```cpp
// TODO: P2-D05: Initialize RocksDB TransactionDB instance
// For now, this is a placeholder that logs the intent
spdlog::info("LLMPluginManager::initializeStateStore: ...");
```

**Required Fix:**
- Actually initialize RocksDB TransactionDB when state_db_ is available
- Ensure column families are created properly
- Add error handling for RocksDB initialization failures

---

### Issue 2: HLC Timestamp Comparison Not Implemented (ssm_state_rocksdb_store.cpp:187)
**Status:** TODO comment, simple heuristic used
**Impact:** LOW-MEDIUM - Retention window logic may not work correctly
**Files:**
- src/llm/ssm_state_rocksdb_store.cpp (line 187)

**Current Code:**
```cpp
// TODO: Proper HLC comparison
// For now, simple heuristic: if physical_time < cutoff, delete
if (ts->physical() < static_cast<uint64_t>(cutoff_ms)) {
    keys_to_delete.push_back(it->key().ToString());
}
```

**Required Fix:**
- Implement proper Hybrid Logical Clock (HLC) comparison
- Use both physical and logical components
- Ensure monotonicity of comparisons

---

### Issue 3: Binary Serialization Not Optimized (ssm_state_rocksdb_store.cpp:252)
**Status:** TODO comment, using JSON inefficiently
**Impact:** LOW - Performance optimization, not correctness
**Files:**
- src/llm/ssm_state_rocksdb_store.cpp (line 252)

**Current Code:**
```cpp
// TODO: Use protobuf or binary serialization for efficiency
nlohmann::json j;
// ... serialize snapshot to JSON ...
```

**Required Fix:**
- Optimize serialization for performance
- Can defer to Phase 2 if it doesn't affect tests

---

## Thread-Safety & RAII Assessment

### Files with Known Threading Issues (Partial List)
From gap analysis: 128 circular_lock_ordering + 11 data_race instances

**High-Risk Files** (>30 High+Critical gaps):
- llama_wrapper.cpp (35C, 76H)
- ml_model_manager.cpp (11C, 48H)
- production_validator.cpp (2C, 25H)
- grafana_metrics.cpp (4C, 32H)
- gpu_memory_manager.cpp (7C, 28H)

**Current Status:**
- Most use std::mutex + std::lock_guard correctly
- Some may have deadlock potential with nested locks
- Document lock hierarchies to prevent circular locks

---

## Acceptance Criteria for This Phase

### Build & Compilation
- [ ] 0 compilation errors
- [ ] ≤ 5 warnings (non-blocking)
- [ ] All includes resolve correctly

### Tests
- [ ] 120+ LLM tests pass
- [ ] 0 test failures
- [ ] All exception-safety tests (LLM-EXC-01..08) pass
- [ ] All RAII tests (LLM-RAII-01..08) pass

### Sanitizers
- [ ] AddressSanitizer: 0 memory leaks, 0 out-of-bounds
- [ ] ThreadSanitizer: 0 data races (if TSan tests run)
- [ ] UndefinedBehaviorSanitizer: 0 UB issues

### Code Quality
- [ ] All destructors marked noexcept
- [ ] RAII used for all resource management
- [ ] Thread-safe access to shared data
- [ ] Proper const-correctness

---

## Implementation Sequence

### Phase 1: Quick Wins (Day 1)
**Priority:** HIGH
**Effort:** 2-4 hours

1. ✓ Fix the RocksDB initialization TODO (HIGH impact)
2. ✓ Add proper HLC timestamp comparison (MEDIUM impact)
3. ✓ Review and document lock hierarchies (PREVENTIVE)

### Phase 2: Thread-Safety Review (Day 2)
**Priority:** MEDIUM
**Effort:** 4-6 hours

1. Audit files with >30 H+C gaps for deadlock risks
2. Verify mutex ordering is consistent
3. Add comments documenting lock order

### Phase 3: Exception Safety Review (Day 3)
**Priority:** MEDIUM
**Effort:** 3-4 hours

1. Verify all destructors are noexcept
2. Add try-catch where needed in cleanup paths
3. Test with exception-safety tests

### Phase 4: Validation (Day 3-4)
**Priority:** HIGH
**Effort:** 2-4 hours

1. Run full build
2. Run LLM test suite
3. Run with ASan/TSan if possible
4. Verify no regressions

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|-----------|
| RocksDB init breaks state store | MEDIUM | HIGH | Add unit test for RocksDB init |
| HLC comparison causes retention bugs | LOW | MEDIUM | Thorough testing with time simulation |
| New code introduces memory leaks | MEDIUM | HIGH | Run with ASan |
| New code has data races | LOW | HIGH | Run with TSan |
| Performance regression | LOW | MEDIUM | Run benchmark gates |

---

## Success Metrics

- [ ] All 12 TODO/Stub items in critical files addressed
- [ ] 120+ LLM tests pass without new failures
- [ ] 0 memory leaks detected (ASan)
- [ ] 0 data races detected (TSan)
- [ ] All performance gates pass
- [ ] Code review approved
- [ ] Documentation updated

---

**Next Step:** Begin Phase 1 implementation of RocksDB initialization fix.
