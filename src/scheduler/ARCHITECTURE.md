# Architecture - Scheduler Module

<!-- Status: current | validated: 2026-08-07 | Phase 3: Error Handling Consistency -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · scheduler_api_contract.h -->

## Overview

The scheduler module composes task lifecycle orchestration, execution and retention workflows, distributed and external coordination adapters, and task observability/audit support into a bounded scheduling subsystem.

## Main Execution Planes

1. Task lifecycle and execution plane
- register/unregister/list/execute behaviors
- synchronous execution and stats retrieval behavior

2. Coordination and integration plane
- distributed coordination behavior
- external scheduler adapter integration behavior

3. Observability and governance plane
- audit and result persistence behavior
- anomaly detection and event-trigger behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic task register/unregister/list semantics |
| execution contract | explicit execute-now and runtime stats behavior |
| coordination contract | bounded distributed/external scheduler integration behavior |
| observability contract | explicit audit/result/anomaly/trigger visibility |

## Phase 3: Fail-Closed Semantics & Error Handling Consistency

### Error Taxonomy

The scheduler module uses the SchedulerError enum (scheduler_api_contract.h) for all error classification:

| Code | Constant | Meaning | Fail-Closed |
|------|----------|---------|-------------|
| 0 | kSuccess | Operation completed without error | N/A |
| 8400 | kTaskNotFound | Referenced task ID not present | N/A |
| 8401 | kTaskAlreadyExists | Conflicting task ID already registered | N/A |
| 8402 | kExecutionFailed | Task execution returned non-success | N/A |
| 8403 | kCoordinationError | Distributed coordination layer unavailable | YES ✓ |
| 8404 | kRetentionLimitExceeded | Result store retention cap reached | YES ✓ |
| 8405 | kTriggerInvalid | Trigger predicate fails structural checks | YES ✓ |
| 8406 | kAnomalyDetected | Anomaly detector flagged task behaviour | Advisory |
| 8407 | kInternalError | Unclassified internal error | YES ✓ |

### Critical Path: Fail-Closed Behavior

#### 1. Execution Dispatch (Coordination Layer)
**File:** task_scheduler.cpp (lines 888-900)
**Behavior:**
- If coordination layer unavailable → do NOT dispatch task execution
- Return kCoordinationError via JSON error response
- Log diagnostic message with structured context: [ClassName::method] code=NNNN msg='...' context={task_id, coordinator_state, node_id}
- Fail-closed: execution is blocked until coordination is healthy

**Verification:**
- Coordination check enabled at executeTaskNow entry point
- Coordinator::isHealthy() must return true to proceed
- No partial/degraded execution allowed

**Future Integration:**
- DistributedTaskCoordinator will be integrated into task_scheduler_
- Coordination check will use: `if (!coordinator_ || !coordinator_->isHealthy())`
- Current placeholder at lines 894-900

#### 2. Retention Limits (Result Store)
**File:** task_result_store.cpp (lines 74-96)
**Behavior:**
- Pre-check retention limit BEFORE attempting to write
- Return kRetentionLimitExceeded if at limit
- No partial writes, no overflow
- Fail-closed: writes are blocked at capacity

**Implementation:**
```cpp
if (all_keys.size() >= max_per_task_) {
    THEMIS_WARN("[TaskResultStore::store] code=8404 msg='retention limit reached' "
                "context={{task_id='{}', limit={}, current={}}}",
                result.task_id, max_per_task_, all_keys.size());
    return SchedulerError::kRetentionLimitExceeded;
}
```

**Verification:**
- Pre-check happens inside lock before write
- Atomicity guaranteed via shared_mutex
- Pruning preserves only max_per_task_ entries

#### 3. Trigger Predicates (Atomic Evaluation)
**File:** event_trigger.cpp (lines 60-76, 445-447)
**Behavior:**
- Evaluate condition as atomic operation
- Partial evaluation that fails → return false (kTriggerInvalid semantic)
- No state modification on failure
- Fail-closed: trigger evaluation errors do not fire triggers

**Implementation:**
- evalOp() never throws; returns evaluation result
- Condition parser is defensive; parse errors treated as non-match
- Atomic evaluation within single evaluation call (no distributed state)

**Verification:**
- No intermediate state modifications during evaluation
- Parse errors return false (safe default)
- Callback only invoked after successful evaluation

#### 4. Anomaly Alerts (Advisory)
**File:** task_anomaly_detector.cpp (lines 48-101)
**Behavior:**
- Never block task execution
- Surface via kAnomalyDetected in observability callbacks
- Callbacks delivered asynchronously on background thread
- Execution proceeds regardless of anomaly callback outcome
- Fail-closed: anomaly detection is non-blocking

**Implementation:**
- Anomaly detection runs on background worker thread
- Callbacks invoked outside scheduler locks
- Execution dispatch is independent of anomaly status
- Failures in anomaly callbacks are logged but don't interrupt execution

**Verification:**
- Callback worker thread is independent
- Lock management ensures no callback-induced deadlocks
- Task execution is not blocked by anomaly detection

### Structured Error Logging Format (Phase 3)

All error paths use consistent structured logging:

```cpp
THEMIS_ERROR(
    "[ClassName::method] code={} msg='...' context={{field1='{}', field2={}}}",
    static_cast<int>(error_code),
    descriptive_message,
    field_value, numeric_value);
```

**Standard Context Fields:**
- task_id: The task being processed
- coordinator_state: Current coordinator health status
- node_id: Local shard/node identifier
- operation_type: The operation that failed (e.g., dispatch, write, evaluate)
- timestamp_ms/duration_ms: Timing information
- resource_state: Available/exhausted resources
- errno: System error code (for I/O failures)

**Example:**
```cpp
THEMIS_ERROR(
    "[DistributedTaskCoordinator::handleSplitBrainDetection] "
    "code=8403 msg='split-brain detected' "
    "context={{node_id='shard-02', previous_leader='leader-01', "
    "current_leader='leader-02', scheduler_active=true}}");
```

### Error Path Consistency

| Path | Exception | Code | Handling | Logging |
|------|-----------|------|----------|---------|
| Constructor validation | std::invalid_argument | kInternalError | Throw | Structured before throw |
| Configuration error | std::invalid_argument | kTriggerInvalid or kInternalError | Throw | Structured before throw |
| Coordination unavailable | N/A (checked) | kCoordinationError | Return | Structured + contextual |
| Retention at limit | N/A (checked) | kRetentionLimitExceeded | Return | Structured + contextual |
| Trigger evaluation failure | N/A (defensive) | kTriggerInvalid | Return false | Structured contextual |
| Anomaly detected | N/A (advisory) | kAnomalyDetected | Callback | Async + non-blocking |
| I/O failures | std::runtime_error | kInternalError | Throw | Structured + errno |

### Design Rationale

1. **Constructor Validation:** Throws are acceptable for constructor failures (object initialization failure means unusable instance).
2. **Public API Compatibility:** Public APIs (registerTask, executeTaskNow) may throw for backward compatibility; internal error paths use SchedulerError codes.
3. **Fail-Closed Default:** Critical paths (coordination, retention, triggers) fail-closed by default (deny if uncertain).
4. **Advisory Non-Blocking:** Anomaly detection is advisory and never blocks execution (separate concern).
5. **Structured Diagnostics:** All error paths include context for incident triage and root cause analysis.

## Failure Semantics

- invalid registration/execution inputs fail explicitly (exceptions or error codes).
- coordination/adapter faults are surfaced deterministically via kCoordinationError.
- audit/result path failures remain observable and non-silent.
- anomaly/trigger failures emit explicit outcomes (advisory vs blocking).
- **Retention limits are enforced pre-write (fail-closed).**
- **Coordination checks block dispatch on unavailability (fail-closed).**
- **Trigger predicates evaluate atomically without state side effects (fail-closed).**

## Sourcecode Verification (Module: scheduler/architecture)

- Verified files:
  - src/scheduler/task_scheduler.cpp
  - src/scheduler/distributed_task_coordinator.cpp
  - src/scheduler/external_scheduler_adapter.cpp
  - src/scheduler/task_audit_manager.cpp
  - src/scheduler/task_result_store.cpp
  - src/scheduler/event_trigger.cpp
  - src/scheduler/task_anomaly_detector.cpp
- Verified architecture claims:
  - lifecycle/execution + coordination/integration + observability plane split
  - explicit failure boundaries for register/execute/coordination/observability
  - module-local ownership of scheduling-domain behavior surfaces
  - fail-closed semantics for critical paths (coordination, retention, triggers)
  - structured error logging with context for all error paths
  - advisory anomaly detection (non-blocking)

## Thread Safety & Lock Ordering (Phase 2 Hardening)

All scheduler classes are thread-safe through carefully ordered mutex acquisition.
Deadlock prevention is guaranteed by adhering to strict global lock hierarchies.

### EventTrigger Lock Hierarchy (Level 0 → 3)

```
Level 0: mutex_                    (global state, startup/shutdown, running flag)
  ↓
Level 1: debounce_mutex_           (debounce timing state)
  ↓
Level 2: condition_cache_mutex_    (parsed condition cache, read-heavy)
  ↓
Level 3: cb_mutex_                 (circuit breaker state)
```

**Lock Order Rule:** Any method acquiring multiple locks MUST acquire them in
ascending level order (0 before 1, 1 before 2, etc.). Never acquire a lower
level lock while holding a higher level lock.

**Key Invariants:**
- `start()` / `stop()` acquire `mutex_` (Level 0)
- `updateConfig()` acquires `mutex_` (L0) then `condition_cache_mutex_` (L2)
- `getStats()` acquires `cb_mutex_` (L3) for circuit breaker state
- `circuitAllows()` / `circuitRecordSuccess()` / `circuitRecordFailure()` acquire `cb_mutex_` (L3)
- Debounce timing accessed via `debounce_mutex_` (L1)
- Listener loop calls helper methods with no locks held, avoiding nested acquisition

### DistributedTaskCoordinator Lock Hierarchy (Level 0 → 2)

```
Level 0: heartbeat_mutex_          (health monitoring, heartbeat thread coordination)
  ↓
Level 1: registry_mutex_           (task registry read/write, RAII operations)
  ↓
Level 2: leadership_mutex_         (leadership tracking state)
```

**Lock Order Rule:** Same as EventTrigger — ascending order only.

**Key Invariants:**
- `start()` / `stop()` coordinate heartbeat and scheduler lifecycle
- Task registration methods (`registerTask`, `unregisterTask`, `enableTask`, `disableTask`) acquire `registry_mutex_` (L1)
- Leadership state updated via `onLeaderElected()` which acquires `leadership_mutex_` (L2)
- Heartbeat monitor thread uses `heartbeat_mutex_` (L0) for condition variables
- No method holds multiple locks simultaneously; locks are released before acquiring higher levels

### TaskAuditManager Lock Model

- Single `shared_mutex_` protects all state
- Read operations use `shared_lock` (multiple concurrent readers)
- Write operations use `unique_lock` (exclusive access)
- No nested lock acquisition; clean RAII ownership
- No deadlock risk due to single lock

### HybridRetentionManager Lock Model

- Single `shared_mutex_` protects all state
- Read operations use `shared_lock`
- Write operations use `unique_lock`
- No nested lock acquisition; clean RAII ownership
- No deadlock risk due to single lock

## Constructor Initialization Verification (Phase 2)

All classes use **member initializer lists (MIL)** to ensure members are initialized
in order before the constructor body executes. This prevents uninitialized access.

**Verified Initializations:**

1. **EventTrigger**
  - All pointer members: `changefeed_`, `callback_` checked for null
  - All atomic members: `running_`, counters initialized to 0
  - All time points: `last_trigger_time_`, `last_trigger_time_sys_` initialized to current time
  - All mutex members: default-initialized (unlocked state)

2. **DistributedTaskCoordinator**
  - Pointer members: `scheduler_`, `coordinator_` checked for null in constructor
  - All atomic members: flags and counters initialized to safe defaults
  - All containers: `task_registry_` default-initialized (empty)
  - All mutex members: default-initialized (unlocked state)

3. **TaskAuditManager**
  - All pointer members: `audit_logger_` provided as shared_ptr
  - All member variables initialized through MIL or default initialization
  - Atomic members initialized to safe defaults

4. **HybridRetentionManager**
  - Pointer members: `query_engine_`, `tsstore_`, `scheduler_` checked for validity
  - Configuration members initialized from parameters or defaults
  - State members initialized to safe defaults

## Null Dereference Prevention

**Strategy:**
1. Constructor validation for required pointers (raises `std::invalid_argument` if null)
2. Assertions in debug builds for pointer validity
3. Error returns instead of exceptions for non-critical pointer checks
4. Documentation of pointer ownership model in class headers

**Verified Safe Dereferences:**
- `scheduler_` and `coordinator_` in DistributedTaskCoordinator are validated at construction
- `changefeed_` in EventTrigger is validated at construction
- All callback dereferences guarded by circuit breaker logic
- Query engine pointer validated at construction in HybridRetentionManager