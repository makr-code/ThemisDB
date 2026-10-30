**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `security`
**Canonical source references:** [module security](SECURITY.md), [security implementation](content_security.cpp), [security API](../../include/content/content_security.h), [manager API](../../include/content/content_manager.h), [production requirements](PRODUCTION_REQUIREMENTS.md)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Content module trust boundaries and risks

## Purpose

This document describes where content-module trust begins and ends for security reviewers,
integrators, and operators. It records control surfaces and gaps visible in the module
contract; deployment authentication and authorization policy remain owned by the API and
security layers.

## Boundaries and data flow

1. **Untrusted input boundary:** caller-provided bytes, filename, MIME type, stream, and
   JSON configuration enter through manager/import APIs. MIME detection, filename
   validation, size checks, format validation, and policy are relevant controls; these
   must be verified on each production intake path.
2. **Processor/dependency boundary:** format processors and optional OCR, LLM/embedding,
   malware, abuse, and PII implementations may be conditionally built, registered, or
   enabled. Their availability and behavior are not implied by the public class list.
3. **Storage/index boundary:** the content module hands data to storage and index
   managers; it does not own their access-control, durability, or key-management
   implementations.
4. **Caller/deployment boundary:** the module API accepts `user_context`, including for
   contextual encryption integration. That string is not an authentication or
   authorization check. The cited content API does not define the caller's identity,
   tenant isolation, or access-control decision.
5. **Observability boundary:** errors and metrics may expose identifiers and processing
   context. Sanitize output and restrict access; never put raw content, secrets, or
   encryption material in logs or metric labels.

## Controls and configuration risks

`ContentSecurityConfig` has configurable malware, PII, abuse, error-sanitization, and
archive-check settings. The header defaults enable malware/archive checks, but PII and
abuse detection default off; scanner/detector pointers are separately attached. Tests
also demonstrate that enabled check paths without an attached detector can report a
non-finding result. Therefore a "checked" flag or successful status alone does not prove
that a production scanner was operational. Verify dependency registration and policy
with deployment evidence.

Archive protection includes a ratio and file-count bound in the security configuration.
Validation and policy settings also include size/format controls. Do not lower or disable
these checks to resolve ingestion errors without security-owner review and a documented
rollback plan.

## Key handling and access

The manager can receive a `FieldEncryption` dependency, and metadata/blob retrieval
accepts an optional `user_context`. This documentation does not establish encryption
defaults, key custody, key rotation, authorization, or isolation guarantees. Those
controls must be verified in the owning security/storage services and deployment.
Treat `user_context` as potentially sensitive and do not log it.

## Incident handling and review

On a suspected unsafe-content event, preserve sanitized error codes, correlation/content
IDs, active configuration, and dependency health. Restrict or suspend the affected intake
path if a required control is absent; do not convert the incident into permissive
processing. Follow [`RUNBOOK.md`](RUNBOOK.md) and the project
[security reporting policy](../../SECURITY.md). Review optional-control settings,
scanner connectivity, size/archive bounds, logs, and replay decisions after recovery.

## Sources and evidence limits

- [Module security overview](SECURITY.md) and
  [production requirements](PRODUCTION_REQUIREMENTS.md).
- [Security API](../../include/content/content_security.h),
  [policy API](../../include/content/content_policy.h),
  [manager API](../../include/content/content_manager.h), and
  [security implementation](content_security.cpp).
- [Security tests](../../tests/content/test_content_security.cpp),
  [policy tests](../../tests/content/test_content_policy.cpp), and
  [validator tests](../../tests/content/test_content_streaming_ingestion.cpp).
- [Architecture](ARCHITECTURE.md) and [incident runbook](RUNBOOK.md).

The tests cover selected API/configuration branches; they do not certify detector
effectiveness, access-control enforcement, encryption configuration, or deployment
readiness.
