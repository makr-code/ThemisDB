# Wave 3-B Closure Evidence — Query Module

**Date**: 2026-08-25  
**Author**: Wave 3-B implementation agent  
**Scope**: `src/query/` — CRITICAL and HIGH gaps identified by gap-verifier subagent

---

## 1. Fixes Delivered

| # | Severity | File | Location | Gap Type | Fix Summary |
|---|----------|------|----------|----------|-------------|
| 1 | CRITICAL | `parallel_executor.cpp` | `waitWithTimeout()` | `blocking_no_timeout` | Replaced stub `(void)timeout; tg.wait()` with watchdog-thread `cancel_group_execution()` pattern |
| 2 | CRITICAL | `continuous_query_engine.cpp` | `stopLoop()` ~L143 | `blocking_no_timeout` | Replaced bare `loop_thread_.join()` with timed-join via watcher thread + `condition_variable::wait_for(5s)` |
| 3 | CRITICAL | `query_engine.cpp` | `tbbWaitWithTimeout()` + inline `tg.wait()` ~L4872 | `blocking_no_timeout` / `no_timeout` | Rewrote `tbbWaitWithTimeout` to use watchdog; replaced inline `tg.wait()` with `tbbWaitWithTimeout()` call |
| 4 | HIGH | `parallel_executor.cpp` | `parallelScan()` sequential branch ~L224 | `null_dereference` (asymmetry) | Added `if (input.empty()) return Ok(Table{});` before `sequentialScan()` to match TBB path's guard |
| 5 | HIGH | `query_compiler.cpp` | `trySpecialise()` `catch(...)` ~L422 | `catch_all_swallow` | Added `jit_state_corrupted_` sentinel; `catch(...)` now sets it and logs at `THEMIS_ERROR`; added `isJitStateCorrupted()` accessor |

---

## 2. False-Positive Triage Table

Gaps that were in the original CRITICAL list but are confirmed false positives or were already handled:

| Gap ID | File | Type | Disposition | Rationale |
|--------|------|------|-------------|-----------|
| — | `query_canceller.cpp:49` | `blocking_no_timeout` | **WAVE1-FIXED** | `waitUntilCancelledFor()` added in Wave 1 |
| — | `cq_watermark.cpp:60` | `db_connection_leak` | **WAVE1-FIXED** | Lock-free-only implementation; no resource acquired at early-return |
| — | `query_rewrite_rule.cpp:105` | `iterator_invalidation` | **WAVE1-FIXED** | `push_back` is on a different container; `reserve()` in place |
| — | `tensor_aware_query_optimizer.cpp:113,118,123` | `multiplication_overflow` | **WAVE1-FIXED** | All arithmetic via `safeMul()` on `double` values |
| — | `continuous_query_planner.cpp:24` | `scope_mismatch` | **WAVE1-FIXED** | Aliases renamed to `plan_spec/plan_synopsis/plan_wm` |
| — | Various (brace_imbalance×14) | `braces_imbalance` | **PRE-WAVE1-FIXED** | 12 files verified balanced; 2 genuine gaps resolved |
| — | `parallel_executor.cpp` L238 | `null_dereference` | **FP (TBB path only)** | Guard already in TBB lambda; sequential path now also guarded (Fix 4) |

---

## 3. Design Decisions

### Watchdog-thread pattern for TBB timeout (Fixes 1, 2, 3)

TBB `task_group::wait()` has no `wait_until()` overload in C++17/oneTBB-2021.  The correct approach is:
1. A lightweight watchdog `std::thread` polls a shared `std::atomic<bool>` every 50 ms.
2. When the deadline elapses it calls `tg.cancel_group_execution()`, which signals all pending morsel lambdas to check `task::is_cancelled()` and return early.
3. `tg.wait()` on the main thread unblocks as soon as the group is drained or cancelled.
4. The watchdog is joined after `tg.wait()` returns, so no thread leaks.

The 50 ms polling interval gives a worst-case cancellation latency of 50 ms beyond the timeout — acceptable for the 5 s / 30 s deadlines in this codebase.

### Timed-join for `ContinuousQueryEngineImpl::stopLoop()` (Fix 2)

`std::thread::join()` has no timeout overload in C++17/20.  The standard pattern is a watcher thread that joins the target thread and signals a `condition_variable`.  The destructor waits on `cv.wait_for(5s)` and detaches if the deadline fires.  The watcher thread itself is detached in the timeout path to avoid a secondary deadlock.

### `jit_state_corrupted_` sentinel lifetime (Fix 5)

The flag is `false` initially and may only transition to `true` — never back.  This is intentional: once an unknown exception has escaped the compilation subsystem, the internal state of any partially-built closures is unknowable, so the compiler conservatively disables all future specialisation for the lifetime of that `QueryCompiler` instance.  Callers that need to recover must construct a new `QueryCompiler`.

---

## 4. Tests

File: `tests/query/test_wave3b_query_timeout_fixes.cpp`

| Test | Assertion |
|------|-----------|
| `W3B-01` `ParallelExecutorTimeoutTest` | `parallelScan` returns within 10 s on 20-row / 4-thread input |
| `W3B-02` `ContinuousQueryEngineDestructorTest` | Engine destructor returns within 10 s (no deadlock) |
| `W3B-03` `ParallelExecutorNullInputTest` | Empty input → `Ok(empty Table)`, no crash on sequential path |
| `W3B-04` `QueryCompilerCorruptionSentinelTest` | `std::exception` does NOT set `jit_state_corrupted_`; `execute()` never propagates exceptions |

---

## 5. Files Modified

| File | Change |
|------|--------|
| `src/query/parallel_executor.cpp` | Fix 1 (waitWithTimeout watchdog), Fix 4 (sequential empty-guard) |
| `src/query/continuous_query_engine.cpp` | Fix 2 (stopLoop timed join) |
| `src/query/query_engine.cpp` | Fix 3 (tbbWaitWithTimeout watchdog + inline tg.wait() replacement) |
| `src/query/query_compiler.cpp` | Fix 5 (jit_state_corrupted_ sentinel, catch(...)→THEMIS_ERROR, isJitStateCorrupted()) |
| `include/query/query_compiler.h` | Fix 5 (isJitStateCorrupted() public accessor declaration) |
| `src/query/MODULE_GAPS.md` | Updated CRITICAL count (52→49), HIGH count (430→428), Wave 3-B section added |
| `tests/query/test_wave3b_query_timeout_fixes.cpp` | New — 4 regression tests (auto-registered by CMakeLists glob) |
| `src/query/WAVE_3B_CLOSURE_EVIDENCE.md` | This file |
