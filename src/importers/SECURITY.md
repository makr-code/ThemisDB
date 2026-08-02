# Security - Importers Module

<!-- Status: current | validated: 2026-08-02 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · BUILD_STATUS.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the importers module focuses on trusted ingestion boundaries, schema/validation safety, deterministic conflict handling, and auditable import behavior under connector and data-quality constraints.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe source payload ingestion | source validation and schema/type checks before import commit |
| schema drift or malformed data propagation | schema validator/inference gates and structured error handling |
| duplicate/conflicting record corruption | explicit conflict strategy enforcement and deterministic behavior |
| hidden import tampering or weak traceability | audit trail and integrity-related evidence surfaces |
| connector capability mismatch and silent failures | explicit connector support checks and bounded fallback/error paths |

## Implemented Security Controls

- importer flows use explicit source validation and typed schema checks.
- conflict-resolution pathways enforce configured strategy behavior.
- data-quality and audit components provide observable evidence of import decisions.
- advanced paths (CDC/stream/object connectors) expose deterministic unsupported/degraded outcomes.

## Security Follow-ups

- continue hardening connector-specific edge behavior under malformed input.
- tighten diagnostics around permission/auth and unsupported connector incidents.
- expand abuse-case coverage for high-volume mixed-format ingestion.

## Sourcecode Verification (Module: importers/security)

- Verified files:
  - src/importers/postgres_importer.cpp
  - src/importers/mysql_importer.cpp
  - src/importers/mongo_importer.cpp
  - src/importers/flatfile_importer.cpp
  - src/importers/schema_validator.cpp
  - src/importers/conflict_resolver.cpp
  - src/importers/data_quality.cpp
  - src/importers/audit_trail.cpp
  - src/importers/blockchain_integrity.cpp
- Verified controls:
  - source/schema/conflict validation gates
  - auditable and deterministic import error/fallback surfaces
  - connector capability safety boundaries