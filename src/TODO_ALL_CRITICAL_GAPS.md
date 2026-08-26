# TODO: All Critical Gaps Across ThemisDB Modules

> ⚠️ **Status (2026-08-25): Historical snapshot**  
> Diese Datei ist **nicht mehr die kanonische Priorisierungsquelle** fuer reale Sourcecode-Gaps.  
> Aktuelle Core-first Priorisierung und naechste Implementierungsschritte stehen in:  
> `src/MODULE_GAP_ANALYSIS_WAVE2.md` (Wave-4 Plan, verifizierte Real-Gaps).

> Generated: 2026-06-13
> Source: MODULE_GAPS.md files from src/*/ modules
> **Total Critical+High Issues: 1524 (llm) + 997 (sharding) + 22 (whisper) + ... = ~2500+**

---

## 📊 MODULE OVERVIEW

| Module | Total Findings | Critical | High | Medium | Low | Status |
|--------|----------------|----------|------|--------|-----|--------|
| [llm](./llm/TODO_CRITICAL_GAPS.md) | 2146 | 567 | 957 | 548 | 74 | ⚠️ Needs Work |
| [sharding](./sharding/TODO_CRITICAL_GAPS.md) | 1371 | 257 | 740 | 354 | 20 | ⚠️ Needs Work |
| [whisper](./whisper/MODULE_GAPS.md) | 43 | 6 | 16 | 21 | 0 | ✅ Partially Fixed |
| [ai_working](./ai_working/MODULE_GAPS.md) | 0 | 0 | 0 | 0 | 0 | ✅ Clean |

**Total: ~3560 findings, ~1525 Critical+High**

---

## 🔴 GLOBAL CRITICAL PRIORITIES (Across All Modules)

### 1. Thread Join Without Timeout
- **llm:** 11 occurrences
- **sharding:** 38 occurrences (12 already fixed in W4-Sharding)
- **whisper:** 5 occurrences (✅ ALL FIXED in test_whisper_plugin.cpp)
- **Total:** ~54 occurrences
- **Action:** Replace all `th.join()` with `try_join_for(std::chrono::seconds(30))` + error handling
- **Status:** ✅ whisper done, ⚠️ sharding partial, ❌ llm pending

### 2. Resource Leaked in Exception
- **llm:** 108 occurrences
- **sharding:** 79 occurrences
- **Total:** 187 occurrences
- **Action:** Use RAII wrappers, ensure all resources released in exception paths
- **Status:** ❌ Not started

### 3. DB Connection Leak
- **llm:** 104 occurrences
- **sharding:** 35 occurrences
- **Total:** 139 occurrences
- **Action:** Use RAII for DB connections, ensure proper cleanup
- **Status:** ❌ Not started

### 4. Manual Cleanup (vs RAII)
- **llm:** 44 occurrences
- **sharding:** 29 occurrences
- **whisper:** 1 occurrence (✅ FIXED in whisper_transcriber.cpp)
- **Total:** 74 occurrences
- **Action:** Replace manual cleanup with smart pointers
- **Status:** ✅ whisper done, ❌ others pending

### 5. Missing Consensus
- **llm:** 2 occurrences
- **sharding:** 84 occurrences
- **Total:** 86 occurrences
- **Action:** Add consensus/replication acknowledgment to all write operations
- **Status:** ❌ Not started

### 6. Missing Version Tracking
- **sharding:** 86 occurrences
- **Total:** 86 occurrences
- **Action:** Add version tokens/vectors to all concurrent data modifications
- **Status:** ❌ Not started

### 7. Data Race
- **llm:** 330 occurrences
- **sharding:** 23 occurrences (some fixed in W5-Sharding)
- **Total:** 353 occurrences
- **Action:** Use mutexes, copy-under-lock, atomic operations
- **Status:** ⚠️ sharding partial, ❌ llm pending

### 8. Uninitialized Access
- **llm:** 84 occurrences
- **sharding:** 71 occurrences
- **whisper:** 1 occurrence
- **Total:** 156 occurrences
- **Action:** Ensure all variables initialized before use, add bounds checking
- **Status:** ❌ Not started

### 9. Generic Catch Blocks
- **llm:** 22 occurrences
- **sharding:** 4 occurrences
- **whisper:** 2 occurrences (✅ FIXED in whisper_plugin.cpp)
- **Total:** 30 occurrences
- **Action:** Replace `catch (...)` with specific exception type catches
- **Status:** ✅ whisper done, ❌ others pending

### 10. Smart Pointer Misuse
- **llm:** 5 occurrences
- **whisper:** 1 occurrence (✅ FIXED in whisper_plugin.cpp)
- **Total:** 6 occurrences
- **Action:** Use `make_unique`/`make_shared` instead of raw `new`
- **Status:** ✅ whisper done, ❌ llm pending

---

## 📋 MODULE-SPECIFIC TODOS

### ✅ WHISPER Module (22 findings, 6 Critical, 16 High)
**Status:** Mostly Fixed
- [x] Thread join without timeout (5 occurrences) - **FIXED**
- [x] Smart pointer misuse (1 occurrence) - **FIXED**
- [x] Manual cleanup in destructor (1 occurrence) - **FIXED** via RAII
- [x] Generic catch blocks (2 occurrences) - **FIXED**
- [ ] Resource leaks (various) - Partial
- [ ] DB connection leaks - To investigate
- [ ] Missing retry logic - To investigate

**Files Modified:**
- `src/whisper/tests/test_whisper_plugin.cpp` - Thread timeout fixes
- `src/whisper/whisper_plugin.cpp` - Smart pointer + catch blocks
- `src/whisper/whisper_transcriber.cpp` - RAII for whisper context

---

### 🟡 SHARDING Module (1371 findings, 257 Critical, 740 High)
**Status:** Partially Fixed (W4/W5-Sharding)
**TODO File:** [sharding/TODO_CRITICAL_GAPS.md](./sharding/TODO_CRITICAL_GAPS.md)

**Already Fixed:**
- [x] thread_join_no_timeout in 12 files (W4-Sharding, 2026-06-10)
- [x] data race fixes in stream_protocol.cpp (W5-Sharding)
- [x] version token metadata in shard_router.cpp (W5-Sharding)
- [x] lock snapshots in redundancy_strategy.cpp (W5-Sharding)

**Top Remaining Issues:**
1. missing_consensus (84 occurrences)
2. missing_version_tracking (86 occurrences)
3. unspecified_consistency (171 occurrences)
4. resource_leaked_in_exception (79 occurrences)
5. db_connection_leak (35 occurrences)

---

### 🔴 LLM Module (2146 findings, 567 Critical, 957 High)
**Status:** Not Started
**TODO File:** [llm/TODO_CRITICAL_GAPS.md](./llm/TODO_CRITICAL_GAPS.md)

**Top Issues:**
1. data_race (330 occurrences)
2. model_integrity_gap (123 occurrences)
3. resource_leaked_in_exception (108 occurrences)
4. db_connection_leak (104 occurrences)
5. pointer_arithmetic_unbounded (109 occurrences)
6. copy_overhead (109 occurrences)

---

## 📊 IMPLEMENTATION ROADMAP

### Phase 1: Critical Thread Safety (Week 1-2)
- [ ] Fix all thread_join_no_timeout across all modules
- [ ] Fix data_race issues (start with highest impact files)
- [ ] Fix shared_state_no_sync issues

### Phase 2: Resource Management (Week 2-3)
- [ ] Fix all resource_leaked_in_exception issues
- [ ] Fix all db_connection_leak issues
- [ ] Fix manual cleanup issues (replace with RAII)

### Phase 3: Consistency & Versioning (Week 3-4)
- [ ] Fix missing_consensus issues
- [ ] Fix missing_version_tracking issues
- [ ] Fix unspecified_consistency issues

### Phase 4: Error Handling (Week 4-5)
- [ ] Fix uncaught_exception issues
- [ ] Fix no_retry_logic issues
- [ ] Fix generic_catch issues

### Phase 5: Security & Quality (Week 5+)
- [ ] Fix prompt_injection, sql_injection, command_injection
- [ ] Fix unvalidated_llm_output
- [ ] Fix hardcoded_path, hardcoded_output
- [ ] Fix pointer_arithmetic_unbounded

---

## 🎯 QUICK WINS (Can be done immediately)

1. **Thread Join Timeout** (whisper: ✅ Done, others: ❌)
   - Pattern: `th.join()` → `th.try_join_for(std::chrono::seconds(30))`
   - Files: All files with thread.join()
   - Effort: 2-4 hours

2. **Generic Catch Blocks** (whisper: ✅ Done, others: ❌)
   - Pattern: `catch (...)` → `catch (const std::exception& ex)`
   - Files: All files with catch blocks
   - Effort: 2-4 hours

3. **Smart Pointer Misuse** (whisper: ✅ Done, llm: ❌)
   - Pattern: `new Type()` → `std::make_unique<Type>()`
   - Files: Factory functions, constructors
   - Effort: 4-8 hours

4. **Header Consistency** (Partial)
   - Fix version numbers in all header files
   - Remove duplicate header blocks
   - Effort: 2-4 hours

---

## 📈 PROGRESS TRACKING

### ✅ Completed
- [x] whisper module: Thread join timeout (5/5)
- [x] whisper module: Smart pointer misuse (1/1)
- [x] whisper module: Manual cleanup in destructor (1/1)
- [x] whisper module: Generic catch blocks (2/2)
- [x] whisper module: Header consistency (6/6 header files)
- [x] sharding module: Thread join timeout (12/38 files)
- [x] PRODUCTION_REQUIREMENTS.md created for whisper

### 🟡 In Progress
- [ ] sharding module: Thread join timeout (remaining 26)
- [ ] sharding module: Other critical issues

### ❌ Not Started
- [ ] llm module: All critical issues (2146 findings)
- [ ] llm module: TODO list created but no fixes applied
- [ ] Other modules: Need to scan for MODULE_GAPS.md

---

## 🔗 REFERENCE DOCUMENTS

### Module-Specific TODOs
- [llm/TODO_CRITICAL_GAPS.md](./llm/TODO_CRITICAL_GAPS.md) - 2146 findings
- [sharding/TODO_CRITICAL_GAPS.md](./sharding/TODO_CRITICAL_GAPS.md) - 1371 findings
- [whisper/MODULE_GAPS.md](./whisper/MODULE_GAPS.md) - 43 findings

### Source Documents
- [llm/MODULE_GAPS.md](./llm/MODULE_GAPS.md) - Scanner findings
- [sharding/MODULE_GAPS.md](./sharding/MODULE_GAPS.md) - Scanner findings
- [whisper/MODULE_GAPS.md](./whisper/MODULE_GAPS.md) - Scanner findings

---

## 📝 BEST PRACTICES

### 1. Thread Safety
```cpp
// BAD: Blocking indefinitely
thread.join();

// GOOD: With timeout
if (!thread.try_join_for(std::chrono::seconds(30))) {
    // Handle timeout
}
```

### 2. Resource Management
```cpp
// BAD: Manual cleanup
MyResource* res = new MyResource();
// ...
delete res;

// GOOD: RAII
auto res = std::make_unique<MyResource>();
// Automatic cleanup
```

### 3. Exception Handling
```cpp
// BAD: Generic catch
try {
    // ...
} catch (...) {
    // Loses exception information
}

// GOOD: Specific catches
try {
    // ...
} catch (const std::exception& ex) {
    // Handle with full information
}
```

### 4. Consensus & Versioning
```cpp
// BAD: Write without consensus
data.insert(data.end(), new_chunk.begin(), new_chunk.end());

// GOOD: With consensus
if (consensus->proposeWrite(new_chunk)) {
    data.insert(data.end(), new_chunk.begin(), new_chunk.end());
}
```

---

**Total Estimated Effort:** ~300-500 hours (for all modules)
**Recommended Team Size:** 3-5 developers
**Estimated Timeline:** 4-8 weeks (depending on team size)
**Priority:** Critical issues must be fixed before production deployment
