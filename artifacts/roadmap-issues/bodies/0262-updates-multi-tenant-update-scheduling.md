### Context

This issue implements the roadmap item 'Multi-Tenant Update Scheduling' for the updates domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: Multi-Tenant Update Scheduling

### Goal

Deliver the scoped changes for Multi-Tenant Update Scheduling in src/updates/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Multi-Tenant Update Scheduling
**Priority:** Low  
**Target Version:** v1.8.0

Per-tenant update schedules and maintenance windows.

**Features:**
- Tenant-specific maintenance windows
- Update blackout periods
- Priority tiers (critical, normal, low)
- Tenant consent for updates
- Rollback per tenant

**Configuration:**
```cpp
TenantUpdateScheduler scheduler;

// Configure tenant maintenance windows
scheduler.setMaintenanceWindow("tenant-123", {
    .days = {"Saturday", "Sunday"},
    .time_range = {"02:00", "06:00"},
    .timezone = "America/New_York"
});

scheduler.setMaintenanceWindow("tenant-456", {
    .days = {"Daily"},
    .time_range = {"23:00", "05:00"},
    .timezone = "Europe/London"
});

// Set update policy
scheduler.setUpdatePolicy("tenant-123", {
    .auto_update = false,           // Require manual approval
    .critical_auto_update = true,   // Auto-apply critical updates
    .notification_lead_time = std::chrono::hours(24)
});

// Check if update can be applied now
if (scheduler.canUpdateNow("tenant-123")) {
    engine->applyHotReload("1.5.0");
} else {
    auto next_window = scheduler.getNextMaintenanceWindow("tenant-123");
    LOG_INFO("Next maintenance window: {}", next_window);
}
```

---

### Acceptance Criteria

- [ ] Tenant-specific maintenance windows
- [ ] Update blackout periods
- [ ] Priority tiers (critical, normal, low)
- [ ] Tenant consent for updates
- [ ] Rollback per tenant

### Relationships

- Roadmap row: #262 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/updates/FUTURE_ENHANCEMENTS.md#multi-tenant-update-scheduling
- Source key: roadmap:262:updates:v1.8.0:multi-tenant-update-scheduling

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:262:updates:v1.8.0:multi-tenant-update-scheduling -->
<!-- roadmap-ref: row=262;module=updates;target=v1.8.0 -->
<!-- roadmap-detail: src/updates/FUTURE_ENHANCEMENTS.md#multi-tenant-update-scheduling -->
