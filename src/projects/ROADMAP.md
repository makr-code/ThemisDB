> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Projects Module Roadmap

## Current Status

v1.0.0 — Core project management features implemented: lifecycle state machine, snapshot
versioning, structural diff/merge, template instantiation, and collaboration session management.

## Completed ✅

- [x] `ProjectLifecycle` — state-machine transitions with guard validation (`project_lifecycle.cpp`)
- [x] `ProjectVersioning` — snapshot creation, listing, and restoration (`project_versioning.cpp`)
- [x] `ProjectDiff` / `ProjectMerge` — structural delta computation and three-way merge (`project_diff.cpp`)
- [x] `ProjectTemplate` — built-in templates (`BLANK`, `ANALYTICS`, `ML_PIPELINE`, `REPORT`) and custom YAML/JSON instantiation (`project_template.cpp`)
- [x] `CollaborationManager` — concurrent editing sessions with `READ/WRITE/ADMIN/OWNER` permission enforcement (`collaboration_manager.cpp`)
- [x] `IProjectAuditLog` interface — append-only audit trail for project operations (`include/projects/project_audit_log.h`)
- [x] `IProjectBundleManager` interface — project bundle import/export (`include/projects/project_bundle.h`)

## In Progress 🚧

*(none currently in progress)*

## Planned Features 📋

### Short-term (2026-Q3)
- [ ] Unit and integration tests for all lifecycle transition paths (Target: 2026-Q3)
- [ ] Unit tests for `ProjectDiff` conflict detection and `ProjectMerge` three-way merge (Target: 2026-Q3)
- [ ] REST API endpoints for project bundle export/import via `IProjectBundleManager` (Target: 2026-Q3)

### Medium-term (2026-Q4)
- [ ] Operational hardening: metrics and audit hooks for all state-changing operations (Target: 2026-Q4)
- [ ] Security review: permission enforcement edge cases in `CollaborationManager` (Target: 2026-Q4)

## Implementation Phases

### Phase 1: Core Implementation (Status: Completed ✅)
- [x] `ProjectLifecycle` state machine with guard validation
- [x] `ProjectVersioning` snapshot model with `SnapshotMeta`
- [x] `ProjectDiff` delta types (`ADDED`, `REMOVED`, `MODIFIED`, `MOVED`) and `DeltaSet`
- [x] `ProjectTemplate` with built-in templates and custom schema support
- [x] `CollaborationManager` with `Permission`, `User`, `Change` model

### Phase 2: Interfaces & Integration (Status: Completed ✅)
- [x] `IProjectAuditLog` interface with `ProjectAuditAction` enum
- [x] `IProjectBundleManager` interface with `ProjectBundleManifest`

### Phase 3: Tests (Status: Planned)
- [ ] Unit tests for lifecycle transitions and invalid-transition rejection (Target: 2026-Q3)
- [ ] Unit tests for versioning round-trips and snapshot restore (Target: 2026-Q3)
- [ ] Integration tests for concurrent collaboration session scenarios (Target: 2026-Q3)

### Phase 4: Observability & Security Hardening (Status: Planned)
- [ ] Prometheus metrics for collaboration session counts and diff computation latency (Target: 2026-Q4)
- [ ] Audit log integration for all state-changing operations (Target: 2026-Q4)

## Production Readiness Checklist

- [x] Core implementation complete (5 source files, 7 header interfaces)
- [ ] Test coverage for critical lifecycle and merge paths
- [ ] Observability: metrics and audit logging hooked up
- [ ] Security review complete

## Known Issues & Limitations

- Test coverage for the projects module has not yet been confirmed; integration tests are planned for 2026-Q3.
- `ProjectMerge` returns conflicts for the caller to resolve; no automatic conflict resolution is applied.
- Real-time transport for collaboration (WebSocket/SSE) is handled by the server module, not here.

## Breaking Changes

None.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `DocumentManager` – Verwaltet Dokument-Lifecycle in Projekten; Interface in project_lifecycle.h
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `uploadDocument` – Lädt Dokument in den DocumentManager hoch
- `getDocumentBlob` – Gibt rohen Blob eines Dokuments zurück
- `getDocumentChunks` – Gibt Text-Chunks eines Dokuments zurück (für RAG-Pipeline)
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.

