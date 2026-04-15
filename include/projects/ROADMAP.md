<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Projects Module Roadmap

## Current Status

v2.0.0 — `DocumentManager` (v1.0.0) plus five new production-ready headers: `ProjectVersioning`, `ProjectDiff`/`ProjectMerge`, `ProjectLifecycle`, `ProjectTemplate`, and `CollaborationManager`.

## Completed

- [x] `DocumentManager` — project CRUD with versioning, snapshots, and templates
- [x] Project-level RBAC permission enforcement
- [x] Object organization (tables, indexes, queries, models)
- [x] `ProjectVersioning` — immutable snapshots with SHA-256 content integrity
- [x] `ProjectDiff` / `ProjectMerge` — structured DeltaSet diff and three-way merge
- [x] `ProjectLifecycle` — atomic state machine with append-only audit trail
- [x] `ProjectTemplate` — 7 built-in templates + custom JSON template instantiation
- [x] `CollaborationManager` — RBAC sharing, optimistic locking, event subscriptions, change feed

## Implementation Phases

### Phase 1 — Core API ✅
- [x] `DocumentManager` project lifecycle (create/read/update/delete)
- [x] Snapshot and versioning support

### Phase 2 — Object Organization ✅
- [x] Object registry (tables, indexes, queries, process definitions)
- [x] Project-scoped plugin activation

### Phase 3 — Advanced Project Features ✅
- [x] `ProjectVersioning`: immutable snapshot creation, SHA-256 verification, restore (Target: Q2 2026)
- [x] `ProjectDiff` / `ProjectMerge`: field-level DeltaSet and three-way merge (Target: Q2 2026)
- [x] `ProjectTemplate`: 7 built-in templates + custom JSON definitions (Target: Q2 2026)
- [ ] Project import/export ZIP bundle (Target: Q3 2026)
- [ ] Cross-project object sharing with permission delegation (Target: Q4 2026)
- [ ] Project templates from OCI registry (Target: Q3 2026)

### Phase 4 — Collaboration ✅ (partial)
- [x] `CollaborationManager`: RBAC sharing, optimistic locking, change callbacks (Target: Q2 2026)
- [ ] Multi-user project locking with WebSocket notifications (Target: Q4 2026)
- [ ] Project activity audit log REST API (Target: Q3 2026)
- [x] `ProjectLifecycle`: atomic state transitions with append-only audit trail (Target: Q2 2026)

### Phase 5 — Performance / Hardening (Planned)
- [ ] Lazy snapshot loading for large projects (Target: Q4 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] ARCHITECTURE.md complete
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] `DocumentManager` RBAC tested with all role combinations
- [x] Snapshot restore tested across multiple ThemisDB versions
- [x] `ProjectVersioning` SHA-256 checksum verified in PV-01..PV-06
- [x] `ProjectLifecycle` state machine validated in PL-01..PL-07
- [x] `CollaborationManager` thread-safe locking validated in CM-04..CM-06
- [ ] Cross-project sharing (Target: Q4 2026)
