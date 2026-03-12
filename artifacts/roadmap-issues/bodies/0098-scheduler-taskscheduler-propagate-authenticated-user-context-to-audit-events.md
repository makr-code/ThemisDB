### Context

This issue implements the roadmap item '`TaskScheduler`: Propagate Authenticated User Context to Audit Events' for the scheduler domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `TaskScheduler`: Propagate Authenticated User Context to Audit Events

### Goal

Deliver the scoped changes for `TaskScheduler`: Propagate Authenticated User Context to Audit Events in src/scheduler/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `TaskScheduler`: Propagate Authenticated User Context to Audit Events
**Priority:** High
**Target Version:** v1.8.0

`task_scheduler.cpp` has 5 TODOs for user context propagation — all audit events hardcode `"system"` as the actor instead of the actual requesting user:
- Line 452: "Integrate with `AuthenticationContext` to retrieve actual `user_id`"
- Line 453: "Integrate with `RequestContext` to retrieve actual client IP address"
- Lines 494, 524, 552, 632: "Get actual user from auth context"

This means audit trails for scheduled task creation/modification/deletion are not attributable to the operator who made the change.

**Implementation Notes:**
- `[ ]` Add an `AuthContext` parameter (optional, defaulting to a "system" context) to `TaskScheduler::scheduleTask()`, `cancelTask()`, `updateTask()`, and `runNow()`.
- `[ ]` Thread-local `RequestContext` injection: define `TaskScheduler::setRequestContext(ctx)` (thread-local) so HTTP handler code can set the context before calling into the scheduler without changing all method signatures.
- `[ ]` Propagate `AuthContext::user_id` and `AuthContext::client_ip` into `TaskAuditEvent` at lines 494, 524, 552, 632.
- `[ ]` Line 1146: "Consider sandboxing function execution" — add `TaskScheduler::Config::sandbox_execution` boolean flag; when `true`, wrap user-provided task functions in the `ModuleSandbox` from `src/base/module_sandbox.cpp`.

---

### Acceptance Criteria

- [ ] Line 452: "Integrate with `AuthenticationContext` to retrieve actual `user_id`"
- [ ] Line 453: "Integrate with `RequestContext` to retrieve actual client IP address"
- [ ] Lines 494, 524, 552, 632: "Get actual user from auth context"
- [ ] Add an `AuthContext` parameter (optional, defaulting to a "system" context) to `TaskScheduler::scheduleTask()`, `cancelTask()`, `updateTask()`, and `runNow()`.
- [ ] Thread-local `RequestContext` injection: define `TaskScheduler::setRequestContext(ctx)` (thread-local) so HTTP handler code can set the context before calling into the scheduler without changing all method signatures.
- [ ] Propagate `AuthContext::user_id` and `AuthContext::client_ip` into `TaskAuditEvent` at lines 494, 524, 552, 632.
- [ ] Line 1146: "Consider sandboxing function execution" — add `TaskScheduler::Config::sandbox_execution` boolean flag; when `true`, wrap user-provided task functions in the `ModuleSandbox` from `src/base/module_sandbox.cpp`.

### Relationships

- Roadmap row: #98 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/scheduler/FUTURE_ENHANCEMENTS.md#taskscheduler-propagate-authenticated-user-context-to-audit-events
- Source key: roadmap:98:scheduler:v1.8.0:taskscheduler-propagate-authenticated-user-context-to-audit-events

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:98:scheduler:v1.8.0:taskscheduler-propagate-authenticated-user-context-to-audit-events -->
<!-- roadmap-ref: row=98;module=scheduler;target=v1.8.0 -->
<!-- roadmap-detail: src/scheduler/FUTURE_ENHANCEMENTS.md#taskscheduler-propagate-authenticated-user-context-to-audit-events -->
