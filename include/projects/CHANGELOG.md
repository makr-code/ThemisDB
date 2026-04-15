<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Projects Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/projects/CHANGELOG.md`.

## [Unreleased]

### Planned
- Project import/export REST API (ZIP bundle).
- Cross-project object sharing with permission delegation.
- Project templates from OCI registry.

## [2.0.0] — 2026-04-15

### Added
- `project_versioning.h` — `ProjectVersioning` class: immutable, content-addressed snapshots with SHA-256 integrity; `createSnapshot`, `getSnapshot`, `listSnapshots`, `deleteSnapshot`, `restoreSnapshot`, `verifySnapshot`.
- `project_diff.h` — `ProjectDiff` (field-level diff between snapshots or arbitrary JSON), `ProjectMerge` (three-way merge with conflict reporting), `DeltaSet`, `DeltaEntry` (with `toJson`/`fromJson` round-trip).
- `project_lifecycle.h` — `ProjectLifecycle`: atomic state machine (CREATED→ACTIVE→ARCHIVED/DELETED), `initProject`, `activate`, `archive`, `deleteProject`, `getState`, `getAuditTrail`; append-only `ProjectStateTransition` audit log entries.
- `project_template.h` — `ProjectTemplate` factory: `instantiate` (7 `BuiltinTemplate` values), `instantiateFromDefinition`, `validateTemplateDefinition`, `listBuiltinTemplates`.
- `collaboration_manager.h` — `CollaborationManager`: `shareProject`/`revokeAccess`/`getUserPermission`, `subscribe`/`unsubscribeAll`, `lockObject`/`unlockObject`/`isLocked`, `notifyChange`, `getChanges`; `Change`, `Permission`, `User` types.
- `src/projects/` — matching `.cpp` implementations for all five new headers.
- `tests/test_projects.cpp` — 32 unit tests (PV-01..PV-06, PD-01..PD-07, PL-01..PL-07, PT-01..PT-05, CM-01..CM-08).

## [1.0.0] — 2025-06

### Added
- `DocumentManager/document_manager.h` — `DocumentManager` for project CRUD, object organization (tables, indexes, queries, models), project-level permissions, versioning, snapshots, and template management.
