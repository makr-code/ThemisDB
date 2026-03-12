### Context

This issue implements the roadmap item 'Lock Held Across User Callbacks in `CEPEngine::timerLoop()`' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 2 · Lock Held Across User Callbacks in `CEPEngine::timerLoop()`

### Goal

Deliver the scoped changes for Lock Held Across User Callbacks in `CEPEngine::timerLoop()` in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 2 · Lock Held Across User Callbacks in `CEPEngine::timerLoop()`
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/cep_engine.cpp` lines 1071–1095

`WindowManager::timerLoop()` acquires `windows_mutex_` (line 1079) and then immediately
calls `callback_(w.events, w.start, …)` for every open GLOBAL window (line 1082–1084).
User-supplied callbacks are arbitrary code and can perform I/O, database writes, or other
blocking work.  While `windows_mutex_` is held, no other thread can add events, close
windows, or read window state.

**Implementation Notes:**
- `[ ]` In `timerLoop()`, snapshot the callbacks and their arguments under the lock (copy event vectors and timestamps), release `windows_mutex_`, then invoke callbacks on the snapshot — identical to the copy-and-dispatch idiom
- `[ ]` Introduce a `WindowCallbackBatch` value type that carries `(events_copy, start, now)` to make snapshots cheap via move semantics
- `[ ]` Apply the same pattern to `closeWindow()` callers that invoke user callbacks while holding partition locks in `cep_engine.cpp` lines 428–440
- `[ ]` `metricsLoop()` (line 2403) uses bare `std::this_thread::sleep_for(config_.metrics_interval)` — replace with a `condition_variable::wait_for` so the thread wakes immediately on `running_ = false`; current implementation can delay engine shutdown by one full `metrics_interval`
- `[ ]` Add a regression test that calls `CEPEngine::stop()` and asserts it returns within 100 ms regardless of `metrics_interval` value

**Performance Targets:**
- `CEPEngine::stop()` must return within ≤ 100 ms on all background threads

---

### Acceptance Criteria

- [ ] In `timerLoop()`, snapshot the callbacks and their arguments under the lock (copy event vectors and timestamps), release `windows_mutex_`, then invoke callbacks on the snapshot — identical to the copy-and-dispatch idiom
- [ ] Introduce a `WindowCallbackBatch` value type that carries `(events_copy, start, now)` to make snapshots cheap via move semantics
- [ ] Apply the same pattern to `closeWindow()` callers that invoke user callbacks while holding partition locks in `cep_engine.cpp` lines 428–440
- [ ] `metricsLoop()` (line 2403) uses bare `std::this_thread::sleep_for(config_.metrics_interval)` — replace with a `condition_variable::wait_for` so the thread wakes immediately on `running_ = false`; current implementation can delay engine shutdown by one full `metrics_interval`
- [ ] Add a regression test that calls `CEPEngine::stop()` and asserts it returns within 100 ms regardless of `metrics_interval` value
- [ ] `CEPEngine::stop()` must return within ≤ 100 ms on all background threads

### Relationships

- Roadmap row: #41 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#2--lock-held-across-user-callbacks-in-cepenginetimerloop
- Source key: roadmap:41:analytics:v1.8.0:2-lock-held-across-user-callbacks-in-cepenginetimerloop

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:41:analytics:v1.8.0:2-lock-held-across-user-callbacks-in-cepenginetimerloop -->
<!-- roadmap-ref: row=41;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#2--lock-held-across-user-callbacks-in-cepenginetimerloop -->
