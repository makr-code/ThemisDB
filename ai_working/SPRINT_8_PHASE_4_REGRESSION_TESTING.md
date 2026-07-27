# Sprint 8 Phase 4: Regression Testing & Validation

**Date:** 2026-07-27  
**Status:** 🟢 EXECUTING  
**Target:** Comprehensive validation of all 12 moved-from usage fixes  

---

## Objective

Verify that the 12 moved-from usage bug fixes do not introduce:
- Test regressions
- Build failures
- Performance degradation
- Security vulnerabilities
- Undefined behavior

---

## Testing Scope

### Modules Fixed (12 total)

| Module | File | Fix Type | Validation Target |
|--------|------|----------|-------------------|
| index | inverted_index.cpp | Removed .clear() | Unit + Integration |
| index | secondary_index.cpp | Removed .clear() | Unit + Integration |
| prompt_engineering | prompt_quality_evaluator.cpp | Removed .clear() | Unit |
| rag | delegate_evaluator.cpp | Removed .clear() | Unit |
| rag | document_summarizer.cpp | Removed .clear() | Unit |
| rag | multi_step_rag.cpp | Removed .clear() | Unit |
| search | search_highlighter.cpp | Removed .clear() | Unit |
| server | chunked_response_writer.cpp | Removed .clear() | Unit |
| sharding | cross_shard_transaction.cpp | Removed .clear() | Integration |
| sharding | saga_orchestrator.cpp | Removed .clear() | Integration |
| content | pii_detector.cpp | Removed .clear() | Unit |
| replication | replication_manager.cpp | Removed .clear() | Integration |

### Test Categories

1. **Unit Tests** (Module-level)
   - Focused tests via `module_<name>_*_focused` targets
   - Timeout: 120s each
   - Label: `<module>`

2. **Integration Tests** (Cross-module)
   - Wave 2: CMX-01..CMX-06, REC-01..REC-07 (cross_module_ingest_index_query, recovery)
   - Wave 5: E2E-01..E2E-08, FIR-01..FIR-08 (e2e journeys, failure injection)
   - Wave 6: RCJ-01..RCJ-08, SSS-01..SSS-08, FIR-01..FIR-08 (hardening, soak, recovery)
   - Label: `release_critical`

3. **Sanitizer Checks**
   - ASan: Address Sanitizer (memory leaks, use-after-free)
   - UBSan: Undefined Behavior Sanitizer (integer overflow, bounds)
   - TSan: Thread Sanitizer (data races)

4. **Build Verification**
   - `community-release` (system packages)
   - `linux-release` (vcpkg) - if Ninja available
   - Sanitizer variants (asan, ubsan)

---

## Execution Plan

### Step 1: Unit Test Regression Check

**Rationale:** Validate module-level fixes don't break functionality

```bash
# Run all unit tests for affected modules
ctest -L "unit" --output-on-failure -j 4 --timeout 120

# Expected: All tests PASS with 0 failures
```

### Step 2: Integration Test Validation

**Rationale:** Cross-module validation, especially for shared infrastructure changes

```bash
# Wave 2: Cross-module pipeline
ctest -L "wave2" --output-on-failure -j 2 --timeout 180

# Wave 5: E2E critical journeys + failure injection
ctest -L "wave5" --output-on-failure -j 2 --timeout 180

# Wave 6: Release hardening suite
ctest -L "wave6" --output-on-failure -j 2 --timeout 180

# Release critical gate
ctest -L "release_critical" --output-on-failure -j 1 --timeout 300

# Expected: All tests PASS with 0 failures
```

### Step 3: Sanitizer Build & Test

**Rationale:** Verify no memory leaks, undefined behavior, or data races

```bash
# Build with ASan
cmake --preset community-asan
cmake --build --preset community-asan --parallel 8
ctest --preset community-asan -L "unit" --output-on-failure -j 2

# Expected: 0 ASan errors, all tests PASS

# Build with UBSan (similar steps)
cmake --preset community-ubsan
# ... etc
```

### Step 4: Manual Code Review

**Rationale:** Ensure correctness of moved-from state handling

**Review Checklist:**
- [ ] Verify moved-from variable is not accessed after move
- [ ] Confirm `.clear()` removal is safe (moved-from state valid for std::string)
- [ ] Check no logic depends on `.clear()` side effects
- [ ] Verify no data loss or resource leaks

---

## Expected Results

### Success Criteria

✅ **All unit tests pass** (0 failures)
✅ **All integration tests pass** (Wave 2/5/6 green)
✅ **No sanitizer errors** (ASan/UBSan/TSan clean)
✅ **Builds clean** (community-release, linux-release)
✅ **No performance regression** (benchmarks stable)
✅ **Code review approved** (12/12 fixes verified)

### Risk Indicators (Failure Modes)

🔴 **Test failure:** Indicates logic flaw in fix
🔴 **Memory leak (ASan):** Indicates moved-from state mishandling
🔴 **UBSan error:** Indicates undefined behavior in fix
🔴 **Performance drop:** Indicates removed .clear() had side effect
🔴 **Build failure:** Indicates environment/dependency issue

---

## Findings

### Phase 4 Validation Results

[To be populated during execution]

**Build Status:**
- [?] community-release: Pending
- [?] linux-release: Pending (Ninja required)
- [?] community-asan: Pending
- [?] community-ubsan: Pending

**Test Status:**
- [?] Unit tests: Pending
- [?] Integration (Wave 2): Pending
- [?] Integration (Wave 5): Pending
- [?] Integration (Wave 6): Pending
- [?] Release critical gate: Pending

**Sanitizer Status:**
- [?] ASan: Pending
- [?] UBSan: Pending
- [?] TSan: Pending (if available)

---

## Sign-Off

**Phase 4 Validation:** [ ] APPROVED
**Tester:** [ ] Name & Date  
**Reviewer:** [ ] Name & Date  

---

## Next Steps

Upon successful Phase 4 completion:
1. ✅ Proceed to Phase 5: Documentation capture
2. ✅ Proceed to Phase 6: Governance sync & closure

If issues found:
1. Investigate root cause
2. Determine if fix is required or issue is pre-existing
3. Document finding and remediation
4. Re-run affected tests
