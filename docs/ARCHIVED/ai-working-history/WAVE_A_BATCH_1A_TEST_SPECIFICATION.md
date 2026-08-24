# Wave A Batch 1A — Timeout Safety Test Specification

**Date:** 2026-08-17  
**Module:** src/query (query_compiler, query_executor, query_canceller)  
**Wave:** A (Runtime Reliability First)  
**Gates:** ROADMAP.md §12-13  

## Test Objectives

Verify that all timeout safety implementations enforce deterministic behavior:
1. No indefinite blocking on any path
2. Timeout-expired operations fail gracefully with fallback
3. All timeout events are observable via logging
4. Federated timeout edge cases handled correctly
5. Cancellation and timeout signals coexist without race conditions

---

## Unit Test Cases

### Test Suite 1: query_compiler Timeout Enforcement

**Test File:** tests/query/test_query_compiler_timeout.cpp

#### TC1.1: Compilation Within Deadline
**Precondition:** config.compilation_timeout_ms = 100ms, normal executor
**Action:** Call compile() then execute() at hot_threshold
**Expected:**
- Entry.is_compiled = true
- Stats.compilations incremented
- Stats.compilation_timeouts = 0
- Log: INFO message with actual compilation time

#### TC1.2: Compilation Timeout — Specialisation Aborted
**Precondition:** config.compilation_timeout_ms = 100ms, mock executor that sleeps
**Action:** Mock executor sleeps 200ms; trigger specialisation at hot_threshold
**Expected:**
- Entry.compile_failed = true
- Entry.is_compiled = false
- Stats.compilation_timeouts incremented
- Log: WARN message with "exceeded deadline"
- Future execute() calls use cold path indefinitely

#### TC1.3: Compilation Exception Handling
**Precondition:** config.compilation_timeout_ms = 100ms, executor throws
**Action:** Executor throws std::runtime_error; trigger specialisation
**Expected:**
- Entry.compile_failed = true
- Stats.compilation_failures incremented
- Execution continues with cold path
- No propagation of executor exception

#### TC1.4: Compilation Timeout Edge Case — Exactly at Deadline
**Precondition:** config.compilation_timeout_ms = 100ms
**Action:** Specialisation takes exactly 100ms
**Expected:**
- Should NOT timeout (deadline check uses `>`, not `>=`)
- Entry.is_compiled = true
- Stats.compilation_timeouts = 0

#### TC1.5: JIT Disabled — No Specialisation Timeout
**Precondition:** config.enable_jit = false
**Action:** Call compile() and execute()
**Expected:**
- Entry.is_compiled stays false
- No timeout checks performed
- Stats.compilations = 0

### Test Suite 2: query_executor Timeout Enforcement

**Test File:** tests/query/test_query_executor_timeout.cpp

#### TC2.1: Execute Within Timeout
**Precondition:** context.timeout_ms = 1000ms, 100 rows to iterate
**Action:** Call execute()
**Expected:**
- All 100 rows returned
- No timeout exception
- No WARN log about timeout

#### TC2.2: Execute Timeout — Exception Thrown
**Precondition:** context.timeout_ms = 1ms, 10k rows to iterate
**Action:** Call execute()
**Expected:**
- Throws std::runtime_error with "timeout" in message
- ResultSet.rows.size() < 10k (partial before exception)
- Log: WARN message with elapsed time and row count
- Exception includes ExecutionContext.timeout_ms value

#### TC2.3: Execute Streaming Timeout — Partial Results
**Precondition:** context.timeout_ms = 1ms, 10k rows to iterate
**Action:** Call execute_streaming() with counter callback
**Expected:**
- Callback invoked for first N rows (N << 10k)
- Returns N < 10k
- No exception thrown (graceful degradation)
- Log: WARN message with elapsed time and delivered row count

#### TC2.4: Execute No Timeout (Default)
**Precondition:** context.timeout_ms = 0 (default), 10k rows
**Action:** Call execute()
**Expected:**
- All 10k rows returned
- No timeout checks performed (timeout_ms == 0 skips overhead)
- Execution time dominated by row processing, not timeout checks

#### TC2.5: Timeout Coexists with Row Limit
**Precondition:** context.timeout_ms = 100ms, context.row_limit = 50 rows
**Action:** Call execute() with 10k rows available
**Expected:**
- Returns exactly 50 rows (row_limit takes precedence, earlier in check order)
- No timeout exception
- Log: No timeout warning (limit reached before timeout)

#### TC2.6: Cancellation + Timeout Coexistence
**Precondition:** context.timeout_ms = 100ms, 10k rows
**Action:** Start execute() in thread A; call abort() from thread B after 50 rows
**Expected:**
- abort() takes precedence (checked first in loop)
- Returns ~50 rows (or slightly more due to race window)
- No timeout exception
- Log: No timeout warning (cancellation wins)

### Test Suite 3: query_canceller Timeout Safety (Verification)

**Test File:** tests/query/test_query_canceller_timeout.cpp

#### TC3.1: Register + Cancel Within Timeout
**Precondition:** Single-threaded, kLockTimeout = 200ms
**Action:** registerQuery(), cancel(), unregisterQuery()
**Expected:**
- All operations complete successfully
- No WARN logs about lock timeout

#### TC3.2: Registry Lock Contention — Timeout + Fallback
**Precondition:** Thread A holds registry lock; Thread B tries cancel()
**Action:** Thread A sleeps > 200ms; Thread B calls cancel() during hold
**Expected:**
- Thread B: cancel() returns false immediately (timeout)
- Log: WARN "lock timeout for ..."
- Token still valid in Thread A (not corrupted)
- Thread A completes sleep and releases lock; subsequent ops succeed

#### TC3.3: Weak Ptr Cleanup — Token Expired
**Precondition:** Query completes; token strong_ptr dropped; registry still has weak_ptr
**Action:** Main thread calls cancel() on stale weak_ptr
**Expected:**
- cancel() returns false
- Dead weak_ptr cleaned up (tokens_.erase())
- Log: No error, clean degradation

#### TC3.4: ScopedRegistration RAII
**Precondition:** Create ScopedRegistration("req_123")
**Action:** Let ScopedRegistration go out of scope
**Expected:**
- Destructor calls unregisterQuery()
- Entry removed from registry
- No resource leak

---

## Integration Test Cases

### Test Suite 4: End-to-End Query Execution with Timeouts

**Test File:** tests/query/test_query_execution_e2e_timeout.cpp

#### TC4.1: Large Result Set with Execution Timeout
**Setup:** 100k-row result set, Executor configured with timeout_ms = 500ms
**Action:** Call execute()
**Expected:**
- Returns after 500ms
- Partial result set (size varies based on row processing speed)
- Exception indicates timeout with context

#### TC4.2: Compilation + Execution Timeouts
**Setup:** 
- Slow specialising compiler (compile_timeout = 50ms, actual = 150ms)
- Fast query execution (< 10ms per execution)
**Action:** 
- Trigger compilation at hot_threshold
- Verify cold path used
- Execute multiple times to verify no retry of compilation
**Expected:**
- First hit at hot_threshold triggers failed compilation
- Statistics: compilation_timeouts = 1, compilations = 0
- All subsequent executions use cold path

#### TC4.3: Federated Query with Shard Timeouts
**Setup:** query_federation_timeout context + per-shard timeout
**Action:** Execute federated query where one shard times out after 200ms
**Expected:**
- Query-level timeout respected
- Shard timeout doesn't cause cascade
- Partial results returned with timeout metadata

---

## Chaos/Stress Test Cases

### Test Suite 5: High-Contention Scenario

**Test File:** tests/query/test_query_chaos_timeout_contention.cpp

#### TC5.1: Concurrent Registry Operations Under Contention
**Setup:** 
- 10 threads concurrently calling registerQuery(), cancel(), unregisterQuery()
- Artificial mutex hold-time using std::this_thread::sleep
**Action:** Run for 10 seconds
**Expected:**
- All operations complete without deadlock
- WARN logs for lock timeouts are reasonable (< 5% of ops)
- No data corruption in registry
- All tokens cleaned up correctly

#### TC5.2: Compilation Timeout Under Load
**Setup:** 
- 10 threads, each executing different queries
- 3 queries trigger compilation (hit hot_threshold)
- Slow mock executor delays specialisation
**Action:** Run for 10 seconds
**Expected:**
- Compilation timeouts logged appropriately
- No interference between threads
- Statistics aggregated correctly (race-free)

#### TC5.3: Mixed Cancellation + Timeout Signals
**Setup:** 
- Query executing with 5-second timeout
- Cancellation triggered after 2 seconds by separate thread
**Action:** Run multiple times
**Expected:**
- Cancellation always wins (checked first)
- No race condition where both signals corrupt execution
- Logs show cancellation event, not timeout

---

## Performance Regression Tests

### Test Suite 6: Timeout Overhead Measurement

**Test File:** tests/query/test_query_timeout_overhead.cpp

#### TC6.1: Overhead of Timeout Checks (No Timeout Set)
**Baseline:** Execute 10k rows with timeout_ms = 0
**Test:** Execute 10k rows with timeout_ms = 10000
**Expected:** 
- Overhead < 2% (timeout check is just an atomic compare)
- Measured via std::chrono::steady_clock

#### TC6.2: Overhead of Compilation Timeout Checks
**Baseline:** Compile 100 queries with compilation_timeout_ms = 0 (disabled)
**Test:** Compile 100 queries with compilation_timeout_ms = 100
**Expected:**
- Overhead < 1ms per compilation (one chrono call)

#### TC6.3: Registry Lock Timeout — No Contention Case
**Baseline:** 100k registerQuery() calls, no threads
**Test:** 100k registerQuery() calls, no lock timeout
**Expected:**
- No measurable difference (mutex acquired immediately)
- No WARN logs (no contention)

---

## Observability Tests

### Test Suite 7: Logging and Metrics

**Test File:** tests/query/test_query_timeout_observability.cpp

#### TC7.1: Compilation Timeout Logging
**Setup:** Trigger compilation timeout (slow executor)
**Expected Log:**
```
WARN QueryCompiler: compilation timeout (exceeded deadline) 
     key=abc123def456 150000us > 100000us (100ms)
```

#### TC7.2: Execution Timeout Logging (Materialised)
**Setup:** Execute with timeout_ms = 50ms, large result set
**Expected Log:**
```
WARN QueryExecutor::execute: timeout exceeded after 50ms, 
     processed 523 rows
```

#### TC7.3: Execution Timeout Logging (Streaming)
**Setup:** Execute streaming with timeout_ms = 50ms, 10k rows
**Expected Log:**
```
WARN QueryExecutor::execute_streaming: timeout exceeded after 50ms, 
     delivered 312 rows
```

#### TC7.4: Lock Timeout Logging (Registry)
**Setup:** Concurrent access causing timeout
**Expected Log:**
```
WARN QueryCanceller::cancel: lock timeout for 'req_xyz'
```

#### TC7.5: Statistics Tracking
**Setup:** Trigger various timeout scenarios
**Expected:**
- stats_.compilation_timeouts incremented correctly
- stats_.compilation_failures distinguished from timeouts
- No overlap or double-counting

---

## Test Execution Plan

### Phase 1: Syntax Verification (Pre-Merge)
- [ ] Verify brace balance in all modified files
- [ ] Check includes are correct (chrono, fmt, logger)
- [ ] Ensure no undefined symbol references

### Phase 2: Unit Tests (CI Pipeline)
- [ ] Run tests/query/test_query_compiler_timeout.cpp (5 tests)
- [ ] Run tests/query/test_query_executor_timeout.cpp (6 tests)
- [ ] Run tests/query/test_query_canceller_timeout.cpp (4 tests)
- **Gate:** All 15 unit tests pass with 100% coverage

### Phase 3: Integration Tests (CI Pipeline)
- [ ] Run tests/query/test_query_execution_e2e_timeout.cpp (3 tests)
- **Gate:** All integration tests pass

### Phase 4: Chaos/Stress Tests (Nightly Pipeline)
- [ ] Run tests/query/test_query_chaos_timeout_contention.cpp (3 tests, 10s each)
- [ ] Duration: ~5 minutes
- **Gate:** No deadlocks, no data corruption, <5% timeout WARN rate

### Phase 5: Performance Tests (Release Pipeline)
- [ ] Run tests/query/test_query_timeout_overhead.cpp (3 tests)
- [ ] Collect baseline metrics
- **Gate:** Overhead < 2% in all scenarios

### Phase 6: Observability Verification (Manual Review)
- [ ] Verify all log messages are accurate and complete
- [ ] Check statistics tracking (no double-counting)

---

## Success Criteria

| Criterion | Pass Threshold | Measurement |
|-----------|-----------------|-------------|
| **Unit Tests** | 100% pass rate | 15 tests execute without failure |
| **Integration Tests** | 100% pass rate | 3 tests execute without failure |
| **Chaos Tests** | 0 deadlocks | 30+ minutes without hang |
| **Performance Overhead** | < 2% | Benchmarked with chrono |
| **Logging Coverage** | All paths logged | Every timeout path has corresponding WARN/INFO |
| **Statistics Accuracy** | 100% tracking | No double-count, correct aggregation |

---

## Risk Mitigation

### Known Risks

1. **Race condition in timeout check + row iteration**
   - **Mitigation:** Check happens at loop start (before row access), atomic isExecutionTimeoutExceeded()
   - **Fallback:** If timeout missed, next iteration will catch it

2. **Timeout clock resolution (Windows vs Linux)**
   - **Mitigation:** Using std::chrono::steady_clock (consistent across platforms)
   - **Test:** TC4.1 verifies actual timeout behavior

3. **Cascading timeouts in federated queries**
   - **Mitigation:** Per-shard timeout independent of execution timeout
   - **Test:** TC4.3 verifies federated case

---

## Documentation References

- ROADMAP.md: Wave A §12-13 timeout safety requirements
- WAVE_A_BATCH_1A_IMPLEMENTATION_SUMMARY.md: Implementation details
- query_compiler.h: SLA documentation for compilation_timeout_ms
- query_executor.h: SLA documentation for ExecutionContext.timeout_ms

---

**Test Plan Version:** 1.0  
**Last Updated:** 2026-08-17  
**Next Review:** Post-CI green (scheduled after merge)
