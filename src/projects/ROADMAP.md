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

### Short-term (2026-Q3) — Completed ✅
- [x] Unit and integration tests for all lifecycle transition paths (Target: 2026-Q3) — `tests/test_projects.cpp` PL-01..PL-07; `tests/test_project_collaboration_concurrent.cpp` CC-08 lifecycle race
- [x] Unit tests for `ProjectDiff` conflict detection and `ProjectMerge` three-way merge (Target: 2026-Q3) — `tests/test_projects.cpp` PD-01..PD-07
- [x] Unit tests for `ProjectVersioning` round-trips and snapshot restore (Target: 2026-Q3) — `tests/test_projects.cpp` PV-01..PV-06
- [x] Integration tests for concurrent collaboration session scenarios (Target: 2026-Q3) — `tests/test_project_collaboration_concurrent.cpp` CC-01..CC-08
- [x] REST API endpoints for project bundle export/import via `IProjectBundleManager` (Target: 2026-Q3) — interface in `include/projects/project_bundle.h`

### Medium-term (2026-Q4)
- [x] Operational hardening: Prometheus metrics for collaboration session counts and diff computation latency (Target: 2026-Q4)
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

### Phase 3: Tests (Status: Completed ✅ 2026-04-21)
- [x] Unit tests for lifecycle transitions and invalid-transition rejection — `ProjectsModuleTests` (PL-01..PL-07) in `tests/test_projects.cpp`
- [x] Unit tests for versioning round-trips and snapshot restore — `ProjectsModuleTests` (PV-01..PV-06) in `tests/test_projects.cpp`
- [x] Unit tests for `ProjectDiff` conflict detection and `ProjectMerge` three-way merge — `ProjectsModuleTests` (PD-01..PD-07) in `tests/test_projects.cpp`
- [x] Integration tests for concurrent collaboration session scenarios — `ProjectCollaborationConcurrentTests` (CC-01..CC-08) in `tests/test_project_collaboration_concurrent.cpp`

### Phase 4: Observability & Security Hardening (Status: In Progress 🚧)
- [x] `InMemoryProjectAuditLog` concrete implementation — `include/projects/in_memory_project_audit_log.h` + `src/projects/in_memory_project_audit_log.cpp` (PAL-01..PAL-06 tests in `tests/test_project_collaboration_concurrent.cpp`)
- [x] Audit log integration for all state-changing operations (Target: 2026-Q4) — `CollaborationManager::setAuditLog()` DI setter wires `notifyChange()` → `IProjectAuditLog::record()`
- [x] Prometheus metrics for collaboration session counts and diff computation latency (Target: 2026-Q4)

## Production Readiness Checklist

- [x] Core implementation complete (5 source files, 7 header interfaces)
- [x] Test coverage for critical lifecycle and merge paths — `ProjectsModuleTests` + `ProjectCollaborationConcurrentTests` (CC-01..CC-08)
- [x] Observability: audit logging hooked up via `setAuditLog()` DI on `CollaborationManager`; PAL-01..PAL-06 tests
- [x] Prometheus metrics: collaboration session counts and diff latency (2026-Q4)
- [ ] Security review complete

## Known Issues & Limitations

- `ProjectMerge` returns conflicts for the caller to resolve; no automatic conflict resolution is applied.
- Real-time transport for collaboration (WebSocket/SSE) is handled by the server module, not here.
- REST API for `IProjectBundleManager` (export/import) is planned for 2026-Q3.

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

