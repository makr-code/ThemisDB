<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Updates Module Roadmap

## Current Status
v1.x – Production-ready zero-downtime update and migration system. HotReloadEngine, release manifest management, schema migration framework, digital signature verification, automatic backup, and rollback are all implemented.

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

## In Progress 🚧
- [!] Delta (binary diff) updates to reduce download size (Target: Q2 2026) (Issue: #2488)
- [!] Canary rollout mode (update a fraction of nodes first) (Target: Q2 2026) (Issue: #2489)
- [!] Update pre-flight health checks (disk space, memory, dependency versions) (Target: Q3 2026) (Issue: #2490)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [!] In-place schema migration without data copy for additive changes (Issue: #2480)
- [!] Migration dry-run with detailed change preview (Issue: #2481)
- [!] Notification webhooks (Slack, PagerDuty) on update success/failure (Issue: #2482)
- [I] Automatic rollback on post-update health check failure (Issue: #2335)
- [I] Update history log (who, when, from/to version) (Issue: #2336)

### Long-term (6-12 months)
- [!] Kubernetes operator integration (rolling update coordination) (Issue: #2483)
- [I] Blue/green deployment support (run two versions simultaneously) (Issue: #2484)
- [!] Multi-node coordinated update with replication-safe sequencing (Issue: #2485)
- [I] Update bundle signing with hardware-backed keys (HSM) (Issue: #2486)
- [!] Schema migration testing framework (apply to staging before production) (Issue: #2487)

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
- [~] Delta (binary diff) updates to reduce download size
- [~] Canary rollout mode (update a fraction of nodes first)
- [~] Update pre-flight health checks (disk space, memory, dependency versions)

### Phase 3: In-Place Migration & Notification Webhooks (Status: Planned 📋)
- [ ] In-place schema migration without data copy for additive changes
- [ ] Migration dry-run with detailed change preview
- [ ] Notification webhooks (Slack, PagerDuty) on update success/failure
- [ ] Automatic rollback on post-update health check failure
- [ ] Update history log (who, when, from/to version)

### Phase 4: Kubernetes & Blue/Green Deployment (Status: Planned 📋)
- [ ] Kubernetes operator integration (rolling update coordination)
- [ ] Blue/green deployment support (run two versions simultaneously)
- [ ] Multi-node coordinated update with replication-safe sequencing
- [ ] Update bundle signing with hardware-backed keys (HSM)
- [ ] Schema migration testing framework (apply to staging before production)

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (download, verify, apply, rollback lifecycle)
- [?] Performance benchmarks (migration duration, downtime measurement)
- [?] Security audit (signature verification, path traversal in update bundles)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- HotReloadEngine is single-threaded; concurrent updates are not allowed.
- Download network protocols (HTTP/S) are handled by the utils module.
- Concurrent update prevention uses filesystem locks; cross-node coordination is not implemented.

## Breaking Changes
- `HotReloadEngine::Config` is stable from v1.x; new optional fields only.
- Migration version numbering scheme is fixed; no re-sequencing allowed after initial deployment.
