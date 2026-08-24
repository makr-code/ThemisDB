# Ingestion Module Phase 2 — CRITICAL Fixes Agent Dispatch Specs

**Target Agent:** `themisdb-implementer`  
**Scope:** 41 CRITICAL findings  
**Timeline:** Aug 22 – Sep 5, 2026  
**Deliverable:** INGESTION_PHASE_2_COMPLETION_SUMMARY.md + merged changes to develop  

---

## Agent Configuration

```
Agent Type: themisdb-implementer
Focus Area: ingestion module
Scope: CRITICAL severity fixes only
Test Coverage: 100% (1 test per fix minimum)
Output Pattern: Per-file code fix + focused test + GitHub issue/rationale
Validation: clang-format + clang-tidy + unit tests
Merge Target: develop
Report: engine-tools-report_progress after each file batch
```

---

## Fix Taxonomy by Category

### Category 1: Timeout Findings (5 instances)
**Root Cause:** File I/O or network operations without timeout parameter  
**Impact:** Can block indefinitely under load or network degradation  
**Files:** ingestion_manager.cpp (3), ingestion_coordinator.cpp (2)  

**Fix Pattern:**
```cpp
// BEFORE (INGESTION-TIMEOUT-001):
bool CheckpointStore::write(const IngestionCheckpoint& cp) {
    return checkpoint_file_.write(cp);  // No timeout
}

// AFTER:
bool CheckpointStore::write(const IngestionCheckpoint& cp, 
                            std::chrono::milliseconds timeout = std::chrono::seconds(30)) {
    return checkpoint_file_.write(cp, timeout);  // Explicit timeout
}
```

**Files Affected:**
- `ingestion_manager.cpp`: Lines 352, 368, 776 (file_io without timeout)
- `ingestion_coordinator.cpp`: Lines 206, 489 (coordinator io without timeout)

**Tests to Add:** `INGESTION-TIMEOUT-001` through `INGESTION-TIMEOUT-005`
- Test: timeout parameter is correctly passed
- Test: operation completes within timeout
- Test: timeout error is properly propagated

---

### Category 2: Data Race Findings (4 instances)
**Root Cause:** Shared data access without synchronization  
**Impact:** Memory corruption, crash under concurrent load  
**Files:** ingestion_manager.cpp (1), database_connector.cpp (1), others (2)  

**Fix Pattern:**
```cpp
// BEFORE (INGESTION-DATA-RACE-001):
auto plug = plugin_registry_.create(pit->second);  // Unsynchronized access

// AFTER:
std::lock_guard<std::mutex> lock(registry_mutex_);
auto plug = plugin_registry_.create(pit->second);  // Protected access
```

**Files Affected:**
- `ingestion_manager.cpp`: Line 706 (plugin_registry_ access)
- `database_connector.cpp`: Line 234 (connection pool access)
- `ingestion_sinks.cpp`: Line 145 (sink registry access)
- `workflow_engine.cpp`: Line 289 (workflow state access)

**Tests to Add:** `INGESTION-DATA-RACE-001` through `INGESTION-DATA-RACE-004`
- Test: concurrent access produces consistent results
- Test: mutex is properly acquired/released
- Test: ThreadSanitizer validation (if available)

**ThreadSanitizer Requirement:**
```bash
# Validation before merge:
cmake --preset community-release -DENABLE_TSAN=ON
ctest --preset community-release -L ingestion -V
# Must complete without TSAN warnings
```

---

### Category 3: Resource Leak in Exception (8 instances)
**Root Cause:** Resources allocated without RAII wrapping; leak on exception throw  
**Impact:** Memory leak, file descriptor leak under error paths  
**Files:** filesystem_ingester.cpp (2), database_connector.cpp (2), cdc_connector.cpp (1), others (3)  

**Fix Pattern:**
```cpp
// BEFORE (INGESTION-RESOURCE-LEAK-001):
std::string* result = new std::string();  // Manual new
try {
    *result = processData();
} catch (...) {
    // result not freed — LEAK
}
return result;

// AFTER:
auto result = std::make_unique<std::string>();
try {
    *result = processData();
} catch (...) {
    // Automatically freed on exception
}
return result;  // Ownership transfer
```

**Files Affected:**
- `filesystem_ingester.cpp`: Lines 112, 289 (file handle leak)
- `database_connector.cpp`: Lines 156, 445 (connection handle leak)
- `cdc_connector.cpp`: Line 178 (CDC stream handle leak)
- `ingestion_quality_judge.cpp`: Line 234 (model resource leak)
- `entity_assembler.cpp`: Line 167 (temporary buffer leak)
- `kafka_connector.cpp`: Line 201 (topic handle leak)
- `object_storage_connector.cpp`: Line 145 (storage connection leak)

**Tests to Add:** `INGESTION-RESOURCE-LEAK-001` through `INGESTION-RESOURCE-LEAK-008`
- Test: resource is freed on exception
- Test: resource is freed on success
- Test: valgrind or AddressSanitizer validation (zero leak reports)

**Valgrind/ASAN Requirement:**
```bash
# Validation before merge:
cmake --preset community-release -DENABLE_ASAN=ON
ctest --preset community-release -L ingestion -V
# Must complete without ASAN leak reports
```

---

### Category 4: Uninitialized Access (3 instances)
**Root Cause:** Member variables not initialized in constructor  
**Impact:** Undefined behavior, crash on use  
**Files:** ingestion_coordinator.cpp (2), parse_text_step.cpp (1)  

**Fix Pattern:**
```cpp
// BEFORE (INGESTION-UNINITIALIZED-001):
class IngestionCoordinator {
public:
    IngestionCoordinator() {
        // retry_count_ NOT initialized
    }
private:
    int retry_count_;  // Garbage value
};

// AFTER:
class IngestionCoordinator {
public:
    IngestionCoordinator() : retry_count_(0), state_(State::IDLE) {}
private:
    int retry_count_ = 0;  // Explicit initialization
    State state_ = State::IDLE;
};
```

**Files Affected:**
- `ingestion_coordinator.cpp`: Lines 145, 289 (member init in ctor)
- `parse_text_step.cpp`: Line 67 (buffer init)

**Tests to Add:** `INGESTION-UNINITIALIZED-001` through `INGESTION-UNINITIALIZED-003`
- Test: object constructed without crash
- Test: member variables have correct initial values
- Test: debug build with initialization checks passes

---

### Category 5: Pointer Arithmetic Unbounded (2 instances)
**Root Cause:** Array access without bounds checking  
**Impact:** Buffer overflow, crash  
**Files:** filesystem_ingester.cpp (1), tensor_core_bridge_step.cpp (1)  

**Fix Pattern:**
```cpp
// BEFORE (INGESTION-PTR-ARITHMETIC-001):
const char* buffer = data.c_str();
int index = computed_offset;  // Unchecked
return buffer[index];  // Potential out-of-bounds

// AFTER:
const char* buffer = data.c_str();
int index = computed_offset;
if (index < 0 || index >= static_cast<int>(data.size())) {
    throw std::out_of_range("buffer index out of bounds");
}
return buffer[index];  // Safe access
```

**Files Affected:**
- `filesystem_ingester.cpp`: Line 423 (chunked read buffer)
- `tensor_core_bridge_step.cpp`: Line 156 (tensor element access)

**Tests to Add:** `INGESTION-PTR-ARITHMETIC-001` through `INGESTION-PTR-ARITHMETIC-002`
- Test: valid index access succeeds
- Test: out-of-bounds access throws exception
- Test: negative index rejected

---

### Category 6: Exception in Destructor (2 instances)
**Root Cause:** Destructor calls code that may throw  
**Impact:** Crash during exception unwinding, program termination  
**Files:** ingestion_sinks.cpp (1), cdc_connector.cpp (1)  

**Fix Pattern:**
```cpp
// BEFORE (INGESTION-EXCEPTION-DTOR-001):
class IngestionSink {
public:
    ~IngestionSink() {
        cleanup_resources();  // May throw
    }
private:
    void cleanup_resources() {
        // Destructor exception — crashes
    }
};

// AFTER:
class IngestionSink {
public:
    ~IngestionSink() noexcept {
        try {
            cleanup_resources();
        } catch (const std::exception& e) {
            log_error("Cleanup failed: " + std::string(e.what()));
            // Swallow exception, do not re-throw
        }
    }
private:
    void cleanup_resources() {
        // Safe cleanup
    }
};
```

**Files Affected:**
- `ingestion_sinks.cpp`: Line 78 (destructor cleanup)
- `cdc_connector.cpp`: Line 334 (connection tear-down)

**Tests to Add:** `INGESTION-EXCEPTION-DTOR-001` through `INGESTION-EXCEPTION-DTOR-002`
- Test: destructor marked `noexcept`
- Test: destructor completes without throwing
- Test: exception during cleanup is logged, not propagated

---

## Implementation Workflow

### Step 1: Receive Phase 1 Triage Output
```json
{
  "phase": 1,
  "date": "2026-08-22",
  "findings_tier_1_critical_blocking": [...],
  "findings_tier_2_critical_high": [...],
  "false_positives_isolated": [...],
  "high_risk_flags": {
    "timeout_priority": true,
    "data_race_priority": true,
    "resource_leak_priority": true
  }
}
```

### Step 2: Prioritize by Category & File
- **Day 1–2:** Timeout findings (ingestion_manager.cpp, ingestion_coordinator.cpp)
- **Day 3–4:** Data race findings (multi-file lock additions)
- **Day 5–7:** Resource leak findings (RAII refactoring)
- **Day 8–10:** Uninitialized access + pointer arithmetic fixes
- **Day 11–12:** Exception-in-destructor fixes + validation

### Step 3: Per-Fix Execution
For each CRITICAL finding:
1. **Understand the Issue:** Read scanner output, verify in source
2. **Implement Fix:** Apply appropriate pattern from above
3. **Write Test:** 1 focused test per fix (INGESTION-xxx pattern)
4. **Validate:**
   ```bash
   clang-format -i <file>
   clang-tidy <file> -- [include paths]
   ctest --preset community-release -L ingestion
   # ThreadSanitizer (for data race fixes)
   # AddressSanitizer (for resource leak fixes)
   ```
5. **Commit:** 
   ```bash
   git add <file> tests/ingestion/test_<category>_focused.cpp
   git commit -m "INGESTION Phase 2: Fix <category>-<number> — <short description>"
   ```
6. **Report:** Update engine-tools-report_progress with this fix + batch progress

### Step 4: Batch Handoff
After each category or daily batch:
- [ ] Commit all changes
- [ ] Run full test suite: `ctest --preset community-release`
- [ ] Check `release_critical` CI gate status
- [ ] Call engine-tools-report_progress with updated checklist

### Step 5: Generate Phase 2 Summary
At end of phase:
- [ ] Create `INGESTION_PHASE_2_COMPLETION_SUMMARY.md` with:
  - Per-fix status (resolved / deferred / escalated)
  - Test coverage report (41 tests for 41 fixes)
  - Performance impact (none expected for this phase)
  - Issues found during implementation
- [ ] Merge all commits to develop
- [ ] Final engine-tools-report_progress call

---

## Test Case Naming Convention

Each test follows: `INGESTION-<CATEGORY>-<NUMBER>-<VARIANT>`

Examples:
- `INGESTION-TIMEOUT-001-valid` → timeout parameter correctly passed
- `INGESTION-TIMEOUT-001-expired` → timeout is respected
- `INGESTION-DATA-RACE-001-concurrent` → concurrent access consistency
- `INGESTION-RESOURCE-LEAK-001-exception` → resource freed on throw
- `INGESTION-RESOURCE-LEAK-001-success` → resource freed on normal exit

Test files:
- `tests/ingestion/test_critical_timeout_focused.cpp`
- `tests/ingestion/test_critical_data_race_focused.cpp`
- `tests/ingestion/test_critical_resource_leak_focused.cpp`
- `tests/ingestion/test_critical_uninitialized_focused.cpp`
- `tests/ingestion/test_critical_pointer_safety_focused.cpp`
- `tests/ingestion/test_critical_exception_safety_focused.cpp`

---

## Quality Gates (Must Pass Before Merge)

- [ ] All 41 CRITICAL findings have fixes
- [ ] All 41 fixes have 1+ test case
- [ ] All test cases pass on community-release preset
- [ ] clang-format passes (no style warnings)
- [ ] clang-tidy passes (no remaining tidy warnings for fixed lines)
- [ ] ThreadSanitizer passes (data race fixes)
- [ ] AddressSanitizer passes (resource leak fixes)
- [ ] Benchmarks neutral (no performance regression)
- [ ] `release_critical` CI gate green on develop

---

## Escalation Criteria

If any CRITICAL finding is complex or requires architectural review:
- [ ] Document in issue/PR comment
- [ ] Link to architecture discussion
- [ ] Flag for Phase 2 review checkpoint
- [ ] Mark as "deferred pending architecture review" (not a failure)

Example: "INGESTION-TIMEOUT-005 requires coordinator state machine refactoring; flagged for Week 2 architecture review."

---

## Success Criteria

Phase 2 is complete when:
1. All 41 CRITICAL findings resolved (or justified as deferred)
2. 100% test coverage (41+ tests, one per fix minimum)
3. All validation gates pass
4. Merged to develop
5. `release_critical` CI gate green
6. INGESTION_PHASE_2_COMPLETION_SUMMARY.md generated and committed

**Target Date:** Sep 5, 2026  
**Status:** Ready for Agent Dispatch (as of 2026-08-15)

