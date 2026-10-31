# Security - Ingestion Module

<!-- Status: module security boundaries; deployment controls remain caller-owned | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security review focuses on external-source input, connector-specific credentials,
workflow/plugin loading, quarantine data, and the in-process admin surface. This
file is not a security certification. Authentication, authorization, logging,
retention, and resource limits must be verified in the service/deployment that
exposes ingestion.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe or malformed source payloads | configured schema/content validation where used; connector/parser behavior is source-specific |
| connector capability mismatch | connector-specific availability/error handling; do not assume common fallback semantics |
| repeated failing payload loops | configured retry/quarantine limits; zero-valued source limits may mean unlimited |
| hidden quality regressions | optional quality judge; unavailable backend can leave scores unevaluated |
| workflow/plugin misuse | ordered step interfaces and optional path/MIME allowlists; empty plugin manifest is permissive |

## Implemented Security Controls

- The manager exposes structured errors and configurable retry/quarantine controls; behavior is not uniform across all connector code paths.
- Schema field validation applies only to parseable JSON content.
- The filesystem workflow path may report successful processing without persisting the assembled result to a sink.
- `IngestionAdminApi` is in-process and does not authenticate or authorize callers.
- Coordinator checkpoint/leader-election defaults are in-memory; they are not durable cross-host security or recovery controls.

## Security Follow-ups

- continue hardening mixed connector edge paths under malformed or hostile input.
- tighten diagnostics for connector auth/capability failure classes.
- expand stress/abuse coverage for high-volume ingestion scenarios.

## Source evidence and limits

- [`include/ingestion/ingestion_manager.h`](../../include/ingestion/ingestion_manager.h)
  defines source/admin and retry configuration; the admin wrapper is not an
  authentication boundary.
- [`include/ingestion/workflow_engine.h`](../../include/ingestion/workflow_engine.h)
  defines optional plugin path/MIME allowlists; the default empty manifest is
  permissive.
- [`src/ingestion/ingestion_manager.cpp`](./ingestion_manager.cpp) shows the
  filesystem workflow dispatch and processed-count behavior; the workflow
  result is not handed to a sink in that branch.
- Schema behavior is implemented through the manager's configured validation
  path and covered by
  [`test_ingestion_schema_validation.cpp`](../../tests/ingestion/test_ingestion_schema_validation.cpp).
  Do not generalize one connector's TLS, credential, fallback, or limit behavior
  to the whole module.

The source review above is not a comprehensive penetration test, deployment
audit, or proof that every connector enforces the same controls. Verify the
exact connector, caller authorization, secret source, resource bounds, and
downstream sink before production use.