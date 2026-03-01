<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Updates Module Roadmap

## Current Status
v1.x – Production-ready zero-downtime update and migration system. HotReloadEngine, release manifest management, schema migration framework, digital signature verification, automatic backup, rollback, and binary delta updates are all implemented.

## Completed ✅
- [x] HotReloadEngine – atomic file replacement with fsync and all-or-nothing semantics
- [x] Resume-capable downloads from GitHub releases
- [x] Automatic backup (rollback points) before applying changes
- [x] CMS/PKCS#7 signature validation with X.509 certificate verification
- [x] Progress tracking via callback API
- [x] Dry-run mode (test upgrade without applying)
- [x] Platform support: Windows, Linux, macOS
- [x] Schema migration framework with versioned migrations
- [x] Version compatibility checking and upgrade path resolution
- [x] Rollback capability with multiple restore points (`listRollbackPoints`)
- [x] Incremental and full migration strategies
- [x] Dependency tracking for complex migrations
- [x] Update scheduling and notification system
- [x] Delta (binary diff) updates to reduce download size (PR: #2488)
- [x] Canary rollout mode (update a fraction of nodes first) (PR: #2587)
- [x] In-place schema migration without data copy for additive changes (Issue: #2480)
- [x] Schema migration testing framework (apply to staging before production) (Issue: #2487)

## In Progress 🚧
- [!] Update pre-flight health checks (disk space, memory, dependency versions) (Target: Q3 2026) (Issue: #2490)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [!] Migration dry-run with detailed change preview (Issue: #2481)
- [P] Notification webhooks (Slack, PagerDuty) on update success/failure (Issue: #2482)
  - Affected files: `include/updates/notification_webhook.h`, `src/updates/notification_webhook.cpp`
  - Runtime: HTTP POST to Slack/PagerDuty within 10 s timeout (5 s connect), non-blocking
  - Error cases: network failure → log + return false; invalid config (empty URL/key) → channel not activated
  - Tests: 30+ unit tests via injectable `HttpSendFunc` (no real network)
  - Performance: <100 ms overhead on notify(); requires `THEMIS_ENABLE_CURL` or custom sender
  - Compatibility: additive; no existing API changed

### Long-term (6-12 months)
- [!] Kubernetes operator integration (rolling update coordination) (Issue: #2483)

## Implementation Phases

### Phase 1: Hot Reload & Schema Migration Framework (Status: Completed ✅)
- [x] `HotReloadEngine` – atomic file replacement with fsync and all-or-nothing semantics
- [x] Resume-capable downloads from GitHub releases
- [x] Automatic backup (rollback points) before applying changes
- [x] CMS/PKCS#7 signature validation with X.509 certificate verification
- [x] Progress tracking via callback API and dry-run mode
- [x] Platform support: Windows, Linux, macOS
- [x] Schema migration framework with versioned migrations
- [x] Version compatibility checking and upgrade path resolution
- [x] Rollback capability with multiple restore points (`listRollbackPoints`)
- [x] Incremental and full migration strategies with dependency tracking
- [x] Update scheduling and notification system

### Phase 2: Delta Updates & Canary Rollout (Status: In Progress 🚧)
- [x] Delta (binary diff) updates to reduce download size
- [x] Canary rollout mode (update a fraction of nodes first)
- [~] Update pre-flight health checks (disk space, memory, dependency versions)

### Phase 3: In-Place Migration & Notification Webhooks (Status: In Progress 🚧)
- [x] In-place schema migration without data copy for additive changes
- [ ] Migration dry-run with detailed change preview
- [P] Notification webhooks (Slack, PagerDuty) on update success/failure
  - API: `UpdateEvent` enum, `UpdateEventPayload`, `SlackConfig`, `PagerDutyConfig`, `NotificationWebhook`
  - Slack: color-coded attachments (good/warning/danger), structured fields, injectable sender
  - PagerDuty: Events API v2, trigger/resolve actions, dedup_key per version, severity mapping
  - Error handling: per-channel failures logged, both channels always attempted, returns false on any failure
  - Tests: injectable `HttpSendFunc` stub, no network required; 30+ unit tests
  - Performance: 10 s HTTP timeout, 5 s connect timeout, <100 ms overhead
- [x] Automatic rollback on post-update health check failure
- [x] Update history log (who, when, from/to version)

### Phase 4: Kubernetes & Blue/Green Deployment (Status: In Progress 🚧)
- [ ] Kubernetes operator integration (rolling update coordination)
- [x] Blue/green deployment support (run two versions simultaneously)
- [x] Multi-node coordinated update with replication-safe sequencing
- [x] Update bundle signing with hardware-backed keys (HSM)
- [x] Schema migration testing framework (apply to staging before production)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (DeltaUpdateEngine: 29 tests; InPlaceSchemaMigrator: 23 tests; module total: 97 tests)
- [x] Integration tests (applyDelta end-to-end: generate → apply → hash verify → atomic install; InPlaceSchemaMigrator: apply → version verify → history check)
- [?] Performance benchmarks (migration duration, downtime measurement)
- [x] Security audit (path traversal in update bundles fixed; `isSafePath` guard in `applyDelta`; InPlaceSchemaMigrator: metadata-only, no data access, no path operations)
- [x] Documentation complete (full API documentation in `delta_update_engine.h` and `in_place_schema_migrator.h`)
- [x] API stability guaranteed (`DeltaUpdateEngine` and `InPlaceSchemaMigrator` are additive; no existing API changed)

## Known Issues & Limitations
- HotReloadEngine is single-threaded; concurrent updates are not allowed.
- Download network protocols (HTTP/S) are handled by the utils module.
- Concurrent update prevention uses filesystem locks; cross-node coordination is provided by `CoordinatedUpdateManager` (transport-agnostic via injected callbacks).

## Breaking Changes
- `HotReloadEngine::Config` is stable from v1.x; new optional fields only.
- Migration version numbering scheme is fixed; no re-sequencing allowed after initial deployment.
