<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Projects Module Roadmap

## Current Status

v1.0.0 — initial. `DocumentManager` provides project CRUD, versioning, snapshots, and templates. Additional project management APIs are planned.

## Completed

- [x] `DocumentManager` — project CRUD with versioning, snapshots, and templates
- [x] Project-level RBAC permission enforcement
- [x] Object organization (tables, indexes, queries, models)

## Implementation Phases

### Phase 1 — Core API ✅
- [x] `DocumentManager` project lifecycle (create/read/update/delete)
- [x] Snapshot and versioning support

### Phase 2 — Object Organization ✅
- [x] Object registry (tables, indexes, queries, process definitions)
- [x] Project-scoped plugin activation

### Phase 3 — Advanced Project Features (Planned)
- [ ] Project import/export (ZIP bundle with all objects) (Target: Q3 2026)
- [ ] Cross-project object sharing with permission delegation (Target: Q4 2026)
- [ ] Project templates from OCI registry (Target: Q3 2026)

### Phase 4 — Collaboration (Planned)
- [ ] Multi-user project locking and concurrent edit notifications (Target: Q4 2026)
- [ ] Project activity audit log (Target: Q3 2026)

### Phase 5 — Performance / Hardening (Planned)
- [ ] Lazy snapshot loading for large projects (Target: Q4 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] ARCHITECTURE.md complete
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] `DocumentManager` RBAC tested with all role combinations
- [x] Snapshot restore tested across multiple ThemisDB versions
- [ ] Cross-project sharing (Target: Q4 2026)
