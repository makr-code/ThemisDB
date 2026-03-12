### Context

This issue implements the roadmap item 'Config Audit Trail' for the config domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Config Audit Trail

### Goal

Deliver the scoped changes for Config Audit Trail in src/config/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Config Audit Trail
**Priority:** High
**Target Version:** v1.8.0

Every successful call to `ConfigPathResolver::resolve()` / `tryResolve()` is recorded in a bounded, thread-safe in-memory audit log (`ConfigAuditLog`) with the requested path, resolved path, UTC timestamp, and flags indicating whether a legacy fallback or LRU cache hit occurred.

**Implementation Notes:**
- `[x]` New files `config_audit_log.h` / `config_audit_log.cpp`; `ConfigAuditLog` class is a standalone bounded ring-buffer (mutex + `std::deque<AuditEntry>`).
- `[x]` `AuditEntry` struct: `requested_path`, `resolved_path`, `timestamp` (`std::chrono::system_clock::time_point`), `is_legacy` (true when the legacy fallback branch was used), `is_cache_hit` (true when served from LRU cache).
- `[x]` Audit logging is disabled by default; opt-in via `ConfigPathResolver::setAuditLogEnabled(true)`.
- `[x]` Maximum entries bounded to 10,000 by default (oldest-first eviction); configurable at runtime via `ConfigPathResolver::setAuditLogMaxEntries(n)`.
- `[x]` `is_legacy` detection uses an explicit `was_legacy_fallback` boolean set at the point the legacy fallback branch is taken — no post-hoc path comparison that could give false positives.
- `[x]` Cache-hit entries also recorded: `is_legacy` is determined by checking `isLegacyPath(normalized) && (*cached == normalized)`.
- `[x]` Audit entry emits a `spdlog::trace` structured message for log-aggregation integration.
- `[x]` Public API: `setAuditLogEnabled(bool)`, `auditLog()` → `std::vector<AuditEntry>`, `clearAuditLog()`, `setAuditLogMaxEntries(std::size_t)`.

**Performance Targets:**
- Hot path overhead (when disabled): one `std::atomic`-equivalent load (`isEnabled()` acquires a mutex; consider relaxing to `std::atomic<bool>` if profiling shows contention at > 100 k RPS).
- Entry insertion (when enabled): single mutex lock + `deque::push_back` < 200 ns.
- `auditLog()` snapshot for 10,000 entries: < 1 ms (single mutex lock + vector copy).

---

### Acceptance Criteria

- [ ] New files `config_audit_log.h` / `config_audit_log.cpp`; `ConfigAuditLog` class is a standalone bounded ring-buffer (mutex + `std::deque<AuditEntry>`).
- [ ] `AuditEntry` struct: `requested_path`, `resolved_path`, `timestamp` (`std::chrono::system_clock::time_point`), `is_legacy` (true when the legacy fallback branch was used), `is_cache_hit` (true when served from LRU cache).
- [ ] Audit logging is disabled by default; opt-in via `ConfigPathResolver::setAuditLogEnabled(true)`.
- [ ] Maximum entries bounded to 10,000 by default (oldest-first eviction); configurable at runtime via `ConfigPathResolver::setAuditLogMaxEntries(n)`.
- [ ] `is_legacy` detection uses an explicit `was_legacy_fallback` boolean set at the point the legacy fallback branch is taken — no post-hoc path comparison that could give false positives.
- [ ] Cache-hit entries also recorded: `is_legacy` is determined by checking `isLegacyPath(normalized) && (*cached == normalized)`.
- [ ] Audit entry emits a `spdlog::trace` structured message for log-aggregation integration.
- [ ] Public API: `setAuditLogEnabled(bool)`, `auditLog()` → `std::vector<AuditEntry>`, `clearAuditLog()`, `setAuditLogMaxEntries(std::size_t)`.
- [ ] Hot path overhead (when disabled): one `std::atomic`-equivalent load (`isEnabled()` acquires a mutex; consider relaxing to `std::atomic<bool>` if profiling shows contention at > 100 k RPS).
- [ ] Entry insertion (when enabled): single mutex lock + `deque::push_back` < 200 ns.
- [ ] `auditLog()` snapshot for 10,000 entries: < 1 ms (single mutex lock + vector copy).

### Relationships

- Roadmap row: #59 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#config-audit-trail
- Source key: roadmap:59:config:v1.8.0:config-audit-trail

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:59:config:v1.8.0:config-audit-trail -->
<!-- roadmap-ref: row=59;module=config;target=v1.8.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#config-audit-trail -->
