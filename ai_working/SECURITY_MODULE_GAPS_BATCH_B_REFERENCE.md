# Security Module — Gap Closure Batch B (HIGH) Quick Reference

**Agent:** 2  
**Scope:** HIGH gaps (234 items) excluding Agent 1 allocation  
**Target Files:** 15+ files  
**Execution Time Estimate:** 60-90 minutes

---

## Quick Start

**Prerequisites:**
- Agent 1 has completed and merged CRITICAL fixes to develop
- You will rebase on develop after Agent 1 merges

**Your task:** Fix all HIGH gaps (excluding those already assigned to Agent 1's HIGH-A batch)

---

## Gap Distribution

**Total HIGH gaps to fix:** ~234

**By Type:**
- scope_mismatch: ~150 gaps (RAII lifetime issues)
- todo_as_productionlogic: ~20 gaps (TODO in production code)
- uncaught_exception: ~20 gaps (missing exception handling)
- manual_cleanup: ~15 gaps (RAII violations)
- no_retry_logic: ~10 gaps (missing failover)
- Other HIGH: ~19 gaps

---

## Target Files (by density)

Priority order (work high-density files first for maximum impact):

1. **vault_key_provider.cpp** (4 HIGH)
2. **vcc_pki_client.cpp** (8 HIGH)
3. **pki_key_provider.cpp** (8 HIGH)
4. **encrypted_field.cpp** (2 HIGH)
5. **confidential_computing.cpp** (3 HIGH)
6. **timestamp_authority_openssl.cpp** (4 HIGH)
7. **hsm_signing.cpp** (HIGH count TBD)
8. **input_validation.cpp** (HIGH count TBD)
9. **query_masking_policy.cpp** (HIGH count TBD)
10. **row_level_security.cpp** (HIGH count TBD)
11. **rbac.cpp** (HIGH count TBD)
12. **key_cache.cpp** (HIGH count TBD)
13. **post_quantum_crypto.cpp** (HIGH count TBD)
14. +2 additional files as needed

---

## Gap Types & Fix Patterns

### Type 1: Scope Mismatch (150 gaps)

**Pattern:** Variable lifetime/RAII issues
**Common Issues:**
- Raw pointers instead of smart pointers
- Variables declared at wrong scope
- Resources not cleaned up on exception

**Fix Template:**
```cpp
// Before (WRONG):
void process() {
    SomeResource* res = new SomeResource();
    try {
        doWork(res);
    } catch (...) {
        // res NOT cleaned up here
        throw;
    }
    delete res;  // May not execute
}

// After (CORRECT):
void process() {
    auto res = std::make_unique<SomeResource>();
    doWork(res.get());  // Exception safety: auto-cleanup via unique_ptr dtor
}
```

### Type 2: TODO as Production Logic (20 gaps)

**Pattern:** `TODO`, `FIXME`, `XXX` comments in active code paths
**Fix:** Either implement the logic or document why it's deferred

**Fix Template:**
```cpp
// Before:
if (needsProcessing) {
    // TODO: implement real processing
    return false;  // Stub behavior
}

// After - Option A: Implement:
if (needsProcessing) {
    return executeRealProcessing();
}

// After - Option B: Document deferral:
if (needsProcessing) {
    // Deferred to Wave D: see ROADMAP.md § Future Enhancements
    LOG(WARNING) << "Feature not yet implemented; using fallback";
    return fallbackBehavior();
}
```

### Type 3: Uncaught Exception (20 gaps)

**Pattern:** Missing exception handling on potentially-throwing calls
**Common Issues:**
- Ignoring exceptions from external service calls
- Not handling allocation failures
- Crypto/key operations without try-catch

**Fix Template:**
```cpp
// Before (WRONG):
auto key = keyProvider->getKey(keyId);  // Can throw
encryptData(key);  // Proceeds without catching exception

// After (CORRECT):
std::optional<Key> key;
try {
    key = keyProvider->getKey(keyId);
} catch (const SecurityException& e) {
    LOG(ERROR) << "Key retrieval failed: " << e.what();
    return SecurityErrorCode::KEY_NOT_FOUND;
}
if (!key) return SecurityErrorCode::KEY_LOOKUP_FAILED;
encryptData(*key);
```

### Type 4: Manual Cleanup (15 gaps)

**Pattern:** Manual resource management instead of RAII
**Common Issues:**
- `delete` calls that can be missed
- Lock guards that should be std::lock_guard/std::unique_lock

**Fix Template:**
```cpp
// Before (WRONG):
void criticalSection() {
    lock_->lock();
    try {
        doWork();
    } catch (...) {
        lock_->unlock();  // Manual unlock, easy to miss
        throw;
    }
    lock_->unlock();
}

// After (CORRECT):
void criticalSection() {
    std::lock_guard<std::mutex> guard(*lock_);
    doWork();  // Guard automatically unlocks on scope exit
}
```

### Type 5: No Retry Logic (10 gaps)

**Pattern:** External service calls with no failover/retry
**Common Issues:**
- Network timeouts with no retry
- Key provider failover not implemented
- No fallback on external dependency failure

**Fix:** Implement retry with exponential backoff or failover to alternate provider

---

## Implementation Guidelines

### Code Style & Practices

**Use modern C++ idioms:**
- `std::unique_ptr` / `std::shared_ptr` over raw pointers
- `std::optional` for nullable returns
- `std::lock_guard` / `std::unique_lock` for synchronization
- Exception handling with specific catch blocks
- `noexcept` annotations where appropriate

**Follow existing patterns in:**
- `include/security/security_api_contract.h` (API stability)
- `src/failover/auto_failover_manager.cpp` (retry/failover patterns)
- `src/updates/ROADMAP.md` (error code taxonomy)

### Testing After Each Fix

- Compile with no warnings: `cmake --build --preset windows-release`
- Run security tests: `ctest -L "security" --output-on-failure`
- Verify no performance regression: `cmake --build --target bench_security_*`

---

## Success Criteria for Agent 2

✅ All 234 HIGH gaps resolved  
✅ All target files compile without errors/warnings  
✅ Security module unit tests pass (100% pass rate)  
✅ No performance regression in benchmarks  
✅ Exception handling patterns verified  
✅ RAII compliance across all changes  

---

## Handoff Deliverable

When complete, create: `SECURITY_GAPS_BATCH_B_COMPLETION.md`

Contents:
- Summary of all 234 HIGH gaps fixed
- File-by-file change log
- Test evidence
- Any issues or edge cases encountered
- Ready for merge to develop

---

**Reference:** ai_working/SECURITY_MODULE_GAPS_BATCH4_MASTER_PLAN.md  
**Estimated Duration:** 60-90 minutes  
**Wait For:** Agent 1 merge to develop (Agent 1 estimated 8-12 min)
