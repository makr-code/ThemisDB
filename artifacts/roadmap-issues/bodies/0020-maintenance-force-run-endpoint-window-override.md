### Context

This issue implements the roadmap item 'Force-Run Endpoint: Window Override' for the maintenance domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: Force-Run Endpoint: Window Override

### Goal

Deliver the scoped changes for Force-Run Endpoint: Window Override in src/maintenance/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### Force-Run Endpoint: Window Override
**Priority:** High
**Target Version:** v1.1.0

There is no way to trigger a schedule outside its maintenance window without editing the window configuration. Operators need an emergency override for urgent maintenance.

**Implementation Notes:**
- `[ ]` Add `POST /api/v1/maintenance/schedules/{id}/run` with optional body `{"force": true}`.
- `[ ]` When `force: true`, bypass the UTC window check in `executeSchedule()`; set `forced: true` in the audit log entry.
- `[ ]` Require `maintenance:admin` scope for the force flag; `maintenance:write` allows manual trigger within the window only.
- `[ ]` Unit test: schedule with a window that excludes the current hour; force-run triggers execution; regular run is skipped.

---

### Acceptance Criteria

- [ ] Add `POST /api/v1/maintenance/schedules/{id}/run` with optional body `{"force": true}`.
- [ ] When `force: true`, bypass the UTC window check in `executeSchedule()`; set `forced: true` in the audit log entry.
- [ ] Require `maintenance:admin` scope for the force flag; `maintenance:write` allows manual trigger within the window only.
- [ ] Unit test: schedule with a window that excludes the current hour; force-run triggers execution; regular run is skipped.

### Relationships

- Roadmap row: #20 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#force-run-endpoint-window-override
- Source key: roadmap:20:maintenance:v1.1.0:force-run-endpoint-window-override

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:20:maintenance:v1.1.0:force-run-endpoint-window-override -->
<!-- roadmap-ref: row=20;module=maintenance;target=v1.1.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#force-run-endpoint-window-override -->
