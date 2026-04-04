### Context

This issue implements the roadmap item '`ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration' for the security domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration

### Goal

Deliver the scoped changes for `ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration in src/security/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration
**Priority:** High
**Target Version:** v1.8.0

`arrow_user_registration_plugin.cpp` has 4 TODO stubs:
- Line 82: "TODO: Implement Apache Arrow integration"
- Line 118: "TODO: Implement Arrow-based authentication"
- Line 136: "TODO: Implement bulk user sync from Arrow source"
- Line 156: "TODO: Implement user update from Arrow source"

All Arrow-based user management operations silently no-op.

**Implementation Notes:**
- `[ ]` Wire `arrow::RecordBatch` deserialization for user records using the Apache Arrow C++ library (already a dependency via `src/exporters/arrow_ipc_exporter.cpp`).
- `[ ]` Implement `bulkSyncFromArrow(arrow::RecordBatch)`: upsert users from a record batch with columns `user_id`, `password_hash`, `roles`, `email`.
- `[ ]` Implement `authenticateFromArrow(user_id, credentials)`: look up user record from the Arrow-backed store.
- `[ ]` Add unit tests: bulk sync of 1000 users via Arrow record batch; verify authentication works for synced users.

---

### Acceptance Criteria

- [ ] Line 82: "TODO: Implement Apache Arrow integration"
- [ ] Line 118: "TODO: Implement Arrow-based authentication"
- [ ] Line 136: "TODO: Implement bulk user sync from Arrow source"
- [ ] Line 156: "TODO: Implement user update from Arrow source"
- [ ] Wire `arrow::RecordBatch` deserialization for user records using the Apache Arrow C++ library (already a dependency via `src/exporters/arrow_ipc_exporter.cpp`).
- [ ] Implement `bulkSyncFromArrow(arrow::RecordBatch)`: upsert users from a record batch with columns `user_id`, `password_hash`, `roles`, `email`.
- [ ] Implement `authenticateFromArrow(user_id, credentials)`: look up user record from the Arrow-backed store.
- [ ] Add unit tests: bulk sync of 1000 users via Arrow record batch; verify authentication works for synced users.

### Relationships

- Roadmap row: #99 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/security/FUTURE_ENHANCEMENTS.md#arrowuserregistrationplugin-implement-apache-arrow-integration
- Source key: roadmap:99:security:v1.8.0:arrowuserregistrationplugin-implement-apache-arrow-integration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:99:security:v1.8.0:arrowuserregistrationplugin-implement-apache-arrow-integration -->
<!-- roadmap-ref: row=99;module=security;target=v1.8.0 -->
<!-- roadmap-detail: src/security/FUTURE_ENHANCEMENTS.md#arrowuserregistrationplugin-implement-apache-arrow-integration -->
