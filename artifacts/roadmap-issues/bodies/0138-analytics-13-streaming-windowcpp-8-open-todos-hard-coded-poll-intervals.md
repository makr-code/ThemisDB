### Context

This issue implements the roadmap item '`streaming_window.cpp` — 8 Open TODOs + Hard-coded Poll Intervals' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 13 · `streaming_window.cpp` — 8 Open TODOs + Hard-coded Poll Intervals

### Goal

Deliver the scoped changes for `streaming_window.cpp` — 8 Open TODOs + Hard-coded Poll Intervals in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 13 · `streaming_window.cpp` — 8 Open TODOs + Hard-coded Poll Intervals
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/streaming_window.cpp` (header reports `TODOs: 8`)

The file header (line 14) self-reports 8 open TODOs and scores 85/100 for quality.  Two
concrete structural issues are observable:

**13a – Hard-coded expiry poll intervals:**
- `SessionWindow::expiryLoop()` line 792: `expiry_cv_.wait_for(lk, std::chrono::milliseconds(200), …)` — 200 ms is hard-coded
- `WindowManager::timerLoop()` line 1073: `timer_cv_.wait_for(lk, std::chrono::milliseconds(500), …)` — 500 ms is hard-coded

These intervals control session-gap detection resolution and GLOBAL-window emission
latency, respectively.  Operators with sub-second SLAs cannot tune them without
recompiling.

**13b – `timerLoop()` holds `windows_mutex_` while calling user `callback_`:**
Lines 1079–1085 lock `windows_mutex_`, iterate windows, and invoke `callback_(w.events, …)`
inside the lock — the same pattern described in section 2 for CEP, but in the streaming
window layer.

**Implementation Notes:**
- `[ ]` Add `session_expiry_check_interval_ms` and `global_window_emit_interval_ms` fields to `WindowConfig` (default 200 ms and 500 ms respectively) — pass them to `wait_for` in `expiryLoop()` and `timerLoop()` instead of literals
- `[ ]` In `timerLoop()`, collect `(events_copy, start, now)` snapshots into a local vector under `windows_mutex_`, release the lock, then call all callbacks on the snapshot
- `[ ]` Identify and document all 8 open TODOs in a `KNOWN_ISSUES.md` or inline comments so they are trackable in code review; the file-header counter is not sufficient
- `[ ]` Add a test asserting that `SessionWindow` emits a result within `gap + expiry_check_interval_ms + 50 ms` of the last event — validates the configurable interval end-to-end

**Performance Targets:**
- Session expiry detection latency: `gap + config.session_expiry_check_interval_ms ± 20 ms`

---

### Acceptance Criteria

- [ ] `SessionWindow::expiryLoop()` line 792: `expiry_cv_.wait_for(lk, std::chrono::milliseconds(200), …)` — 200 ms is hard-coded
- [ ] `WindowManager::timerLoop()` line 1073: `timer_cv_.wait_for(lk, std::chrono::milliseconds(500), …)` — 500 ms is hard-coded
- [ ] Add `session_expiry_check_interval_ms` and `global_window_emit_interval_ms` fields to `WindowConfig` (default 200 ms and 500 ms respectively) — pass them to `wait_for` in `expiryLoop()` and `timerLoop()` instead of literals
- [ ] In `timerLoop()`, collect `(events_copy, start, now)` snapshots into a local vector under `windows_mutex_`, release the lock, then call all callbacks on the snapshot
- [ ] Identify and document all 8 open TODOs in a `KNOWN_ISSUES.md` or inline comments so they are trackable in code review; the file-header counter is not sufficient
- [ ] Add a test asserting that `SessionWindow` emits a result within `gap + expiry_check_interval_ms + 50 ms` of the last event — validates the configurable interval end-to-end
- [ ] Session expiry detection latency: `gap + config.session_expiry_check_interval_ms ± 20 ms`

### Relationships

- Roadmap row: #138 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#13--streaming_windowcpp--8-open-todos--hard-coded-poll-intervals
- Source key: roadmap:138:analytics:v1.8.0:13-streaming-windowcpp-8-open-todos-hard-coded-poll-intervals

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:138:analytics:v1.8.0:13-streaming-windowcpp-8-open-todos-hard-coded-poll-intervals -->
<!-- roadmap-ref: row=138;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#13--streaming_windowcpp--8-open-todos--hard-coded-poll-intervals -->
