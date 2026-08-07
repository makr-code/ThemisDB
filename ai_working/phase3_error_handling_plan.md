# Phase 3: API Error Handling Consistency Implementation Plan

## Scope
Standardize error handling, improve diagnostics, and verify fail-closed behavior across scheduler module paths.

## Current State Analysis

### File-by-File Throw Point Mapping

#### task_scheduler.cpp (~40 throw statements)
- Line 447: `query_engine_ cannot be null` → **kInternalError** (constructor validation)
- Line 588: `Missing permission 'task:register'` → Keep as exception (auth layer)
- Line 606: `CDC event triggers require Changefeed` → **kInternalError** (config error)
- Line 663: `Task already exists with different descriptor` → **kTaskAlreadyExists** (collision)
- Line 1116: `Dependency cycle` → **kExecutionFailed** (DAG error)
- Line 1130: `Missing permission 'task:execute'` → Keep as exception (auth layer)
- Line 1147: `Unknown task in DAG` → **kTaskNotFound** (lookup error)
- Line 1467: Multiple validation throws → Map to appropriate codes
- Lines 2130-2555: Mostly validation throws → Keep internal or map strategically

#### distributed_task_coordinator.cpp (~4 throw statements)
- Lines 38-61: Constructor validation → Map to **kInternalError**
- Lines 485, 509, 524: Already return SchedulerError codes ✓

#### external_scheduler_adapter.cpp (~4 throw statements)
- Lines 210-216: Task/config validation → Map to **kInternalError**
- Lines 323-379: Manifest validation → Map to **kInternalError**

#### event_trigger.cpp (~6 throw statements)
- Lines 106-115: Constructor validation → Map to **kInternalError**
- Line 164: Config validation → Map to **kTriggerInvalid**
- Line 546: Manager constructor → Map to **kInternalError**

#### task_audit_manager.cpp (~1 throw statement)
- Line 559: File I/O error → Map to **kInternalError**

#### task_result_store.cpp
- Already implements fail-closed retention check (lines 78-96) ✓
- Returns SchedulerError codes properly ✓

### Error Code Mapping Strategy

| Exception | Code | Rationale |
|-----------|------|-----------|
| query_engine null | kInternalError | Unclassified internal config error |
| changefeed null | kInternalError | Missing required dependency |
| task already exists (conflict) | kTaskAlreadyExists | Registration conflict |
| task not found | kTaskNotFound | Lookup failure |
| dependency cycle | kExecutionFailed | DAG execution error |
| coordination unavailable | kCoordinationError | Distributed layer down |
| retention limit | kRetentionLimitExceeded | Already implemented ✓ |
| trigger validation | kTriggerInvalid | Trigger config error |
| anomaly detected | kAnomalyDetected | Anomaly flagged (advisory) |
| I/O errors | kInternalError | File system errors |

## Implementation Phases

### Phase 1: Add Structured Logging (Priority 1)
- Replace generic error logs with structured format:
  ```cpp
  THEMIS_ERROR("[ClassName::method] code={} msg='{}' context={{task_id='{}', ...}}",
               static_cast<int>(error_code), message, task_id, ...);
  ```
- Include: task_id, coordinator state, node_id, operation type, duration, resource state

### Phase 2: Verify Fail-Closed Paths (Priority 2)
- [ ] Execution dispatch: Verify coordination check is enabled
- [ ] Retention limits: Verify pre-check before write
- [ ] Trigger predicates: Verify atomic evaluation
- [ ] Anomaly alerts: Verify advisory-only (non-blocking)

### Phase 3: Map Exceptions (Priority 3)
- Focus on internal error handling
- Keep public API exceptions for backward compatibility
- Add context to error messages

### Phase 4: Documentation Update
- Update ARCHITECTURE.md with:
  - Fail-closed semantics section
  - Error propagation patterns
  - Diagnostic message examples
  - Critical path invariants

## Files to Modify
1. distributed_task_coordinator.cpp - Add structured logging to error paths
2. external_scheduler_adapter.cpp - Map validation errors + logging
3. event_trigger.cpp - Map validation errors + logging
4. task_audit_manager.cpp - Map file I/O errors + logging
5. task_scheduler.cpp - Selective structured logging (keep backward compat)
6. ARCHITECTURE.md - Add fail-closed semantics documentation

## Success Criteria
1. All error paths logged with structured context
2. Fail-closed behavior verified for critical paths
3. Consistency in error classification
4. No API breaking changes
5. All tests pass
