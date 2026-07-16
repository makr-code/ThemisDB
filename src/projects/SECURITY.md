# Security - Projects Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the projects module focuses on lifecycle transition integrity, snapshot restore integrity checks, permission/lock enforcement for collaboration, and explicit non-silent conflict/failure signaling.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthorized lifecycle mutation | transition validation and actor-aware state paths |
| snapshot tampering or corrupted restore inputs | integrity checks before restore apply |
| unauthorized collaboration mutation | permission checks and lock ownership enforcement |
| hidden merge/data consistency loss | explicit conflict surfaces and deterministic error returns |

## Implemented Security Controls

- lifecycle and collaboration mutations are validation-gated.
- snapshot restore applies integrity checks before persistence.
- lock ownership mismatch is surfaced explicitly.
- audit and metrics surfaces keep project operations observable.

## Security Follow-ups

- expand fuzz/stress paths for malformed snapshot metadata/payloads.
- tighten permission diagnostics for multi-actor collaboration incidents.
- deepen lock-contention/retry deterministic behavior coverage.

## Sourcecode Verification (Module: projects/security)

- Verified files:
  - src/projects/project_lifecycle.cpp
  - src/projects/project_versioning.cpp
  - src/projects/collaboration_manager.cpp
  - src/projects/project_diff.cpp
  - src/projects/in_memory_project_audit_log.cpp
- Verified controls:
  - validation-gated lifecycle/collaboration mutations
  - deterministic snapshot integrity/restore behavior
  - explicit conflict and lock-failure signaling