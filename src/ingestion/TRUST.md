---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Ingestion Trust Model

## Purpose and scope

This page identifies trust boundaries and controls for ingestion data and
execution. It is for security reviewers and maintainers integrating connectors,
workflow steps, or admin APIs. The module's
[`SECURITY.md`](./SECURITY.md) and
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) remain the
security-requirement sources; this page does not certify a deployment.

## Trust boundaries

1. **External source to connector:** input may be malformed, oversized, stale,
   unauthenticated, or hostile. Connector parsing and configured schema/content
   validation are the first processing boundary.
2. **Connector to workflow step:** extracted text and metadata are passed to
   ordered built-in or dynamically loaded steps. A step can transform data or
   invoke injected inference; plugin registration is therefore a code-execution
   boundary.
3. **Workflow to persistence sink:** generated entities, relations, vectors, or
   documents are passed to injected sink adapters. The ingestion module does not
   own the storage backend's authorization, encryption, or durability policy.
4. **Caller to admin control:** `IngestionAdminApi` exposes source lifecycle,
   reconfiguration, retry, and destructive quarantine operations in-process.
   The wrapper does not authenticate or authorize callers; any network/API
   exposure must enforce those controls before dispatch.

## Controls and residual risks

- HTTP connectors configure TLS peer verification in the inspected API/crawler
  implementations. Verify the TLS behavior of each connector and deployment
  separately; do not generalize one implementation's setting to all sources.
- API keys, OAuth tokens/secrets, passwords, and related option values are
  masked by `sanitizeOptions()` for a fixed set of exact option keys. This is a
  log-sanitization helper, not a secret store, and unknown aliases or secrets
  embedded in URLs are not automatically removed.
- `QuarantineEntry` can retain a serialized raw payload. Restrict access to
  quarantine inspection/export and apply retention rules to source content.
- Preview results and error details can contain source data. Avoid copying
  payloads or credentials to ordinary application logs and incident tickets.
- Dynamic workflow step manifests are permissive when empty: no path or MIME
  allowlist is applied. For production plugin loading, provide explicit
  `allowed_paths` and, where relevant, `allowed_mime_types`; allowlists do not
  replace code review, artifact integrity, or process isolation.
- Several limits default to unlimited or disabled. Configure finite page/message
  limits, bounded retries, timeouts, rate limits, and queue/resource bounds for
  untrusted or externally controlled sources.
- The default shared checkpoint/leader-election implementations are in-memory.
  They are unsuitable as cross-host coordination or durable recovery controls.

## Incident handling

For suspected credential exposure, stop further logging/export of the affected
configuration, rotate the credential at its issuer, and review access to
reports, quarantine, and logs. For hostile source content, isolate the source,
preserve the structured error/correlation evidence without copying raw content,
and validate a corrected or filtered sample before resuming.

For plugin or authorization incidents, disable the affected source/profile or
plugin through the owning service's controlled path and escalate to the
repository security process. Report vulnerabilities through the project-level
[`SECURITY.md`](../../SECURITY.md); do not include secret values in reports.

## Verification sources

- [`include/ingestion/ingestion_manager.h`](../../include/ingestion/ingestion_manager.h) — option redaction, source configuration, quarantine and admin methods.
- [`include/ingestion/workflow_engine.h`](../../include/ingestion/workflow_engine.h) and [`workflow_engine.cpp`](./workflow_engine.cpp) — optional plugin path/MIME allowlists and loading checks.
- [`include/ingestion/inference_backend.h`](../../include/ingestion/inference_backend.h) — backend injection boundaries and null backends.
- [`SECURITY.md`](./SECURITY.md) and [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) — module security/production expectations.

> Exact model backend not verifiable from final document text alone.
