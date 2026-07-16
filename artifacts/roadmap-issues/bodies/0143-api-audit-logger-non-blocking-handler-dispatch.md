### Context

This issue implements the roadmap item 'Audit Logger — Non-Blocking Handler Dispatch' for the api domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v2.0.0.

Primary detail section: Audit Logger — Non-Blocking Handler Dispatch

### Goal

Deliver the scoped changes for Audit Logger — Non-Blocking Handler Dispatch in src/api/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### Audit Logger — Non-Blocking Handler Dispatch
**Priority:** Medium
**Target Version:** v2.0.0

`include/api/audit_logger.h::AuditLogger::log()` holds `mutex_` for the entire duration of calling all registered handlers. Handlers may write to disk, push to a network audit sink, or run regex matching — all while the mutex is held.

**Implementation Notes:**
- `[ ]` **`AuditLogger::log()` holds `mutex_` during handler callbacks** (`audit_logger.h::log()`): a `std::lock_guard<std::mutex> lock(mutex_)` is held for the entire body of `log()`, including the inner `for (const auto& handler : handlers_) { handler(entry); }` loop. File-writing or network-sending handlers will stall every concurrent API thread that tries to emit an audit entry. Decouple: copy the handlers vector under the lock (O(n) pointer copies), release the lock, then invoke the handlers outside the critical section. The buffer append (also inside the lock) is already fast and should remain protected.
- `[ ]` **In-memory audit buffer is not persistent** (`audit_logger.h`): `buffer_` (a circular in-memory vector) is lost on process restart. Add an optional file-backed `AuditLogHandler` that appends newline-delimited JSON audit entries to a configurable path, and register it by default when `config/audit.yaml` specifies `persistence: file`.

---

### Acceptance Criteria

- [ ] **`AuditLogger::log()` holds `mutex_` during handler callbacks** (`audit_logger.h::log()`): a `std::lock_guard<std::mutex> lock(mutex_)` is held for the entire body of `log()`, including the inner `for (const auto& handler : handlers_) { handler(entry); }` loop. File-writing or network-sending handlers will stall every concurrent API thread that tries to emit an audit entry. Decouple: copy the handlers vector under the lock (O(n) pointer copies), release the lock, then invoke the handlers outside the critical section. The buffer append (also inside the lock) is already fast and should remain protected.
- [ ] **In-memory audit buffer is not persistent** (`audit_logger.h`): `buffer_` (a circular in-memory vector) is lost on process restart. Add an optional file-backed `AuditLogHandler` that appends newline-delimited JSON audit entries to a configurable path, and register it by default when `config/audit.yaml` specifies `persistence: file`.

### Relationships

- Roadmap row: #143 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#audit-logger--non-blocking-handler-dispatch
- Source key: roadmap:143:api:v2.0.0:audit-logger-non-blocking-handler-dispatch

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:143:api:v2.0.0:audit-logger-non-blocking-handler-dispatch -->
<!-- roadmap-ref: row=143;module=api;target=v2.0.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#audit-logger--non-blocking-handler-dispatch -->
