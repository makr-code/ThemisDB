### Context

This issue implements the roadmap item 'Multi-Tenant Schedule Isolation' for the maintenance domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.0.0.

Primary detail section: Multi-Tenant Schedule Isolation

### Goal

Deliver the scoped changes for Multi-Tenant Schedule Isolation in src/maintenance/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### Multi-Tenant Schedule Isolation
**Priority:** Low
**Target Version:** v2.0.0

All schedules currently share a single global namespace and window. In a SaaS deployment, different tenants need independent maintenance windows and quotas.

**Implementation Notes:**
- `[ ]` Add `MaintenanceScheduleEntry::tenant_id` (optional; empty = global/system schedule).
- `[ ]` Per-tenant window enforcement: tenant's schedule fires only when the current hour is within that tenant's configured maintenance window, loaded from the tenant config.
- `[ ]` Per-tenant quota: max N concurrent running maintenance jobs per tenant; enforced in `executeSchedule()`.
- `[ ]` Admin API: `GET /api/v1/maintenance/schedules?tenant_id={id}` filters by tenant.

---

### Acceptance Criteria

- [ ] Add `MaintenanceScheduleEntry::tenant_id` (optional; empty = global/system schedule).
- [ ] Per-tenant window enforcement: tenant's schedule fires only when the current hour is within that tenant's configured maintenance window, loaded from the tenant config.
- [ ] Per-tenant quota: max N concurrent running maintenance jobs per tenant; enforced in `executeSchedule()`.
- [ ] Admin API: `GET /api/v1/maintenance/schedules?tenant_id={id}` filters by tenant.

### Relationships

- Roadmap row: #253 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#multi-tenant-schedule-isolation
- Source key: roadmap:253:maintenance:v2.0.0:multi-tenant-schedule-isolation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:253:maintenance:v2.0.0:multi-tenant-schedule-isolation -->
<!-- roadmap-ref: row=253;module=maintenance;target=v2.0.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#multi-tenant-schedule-isolation -->
