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

# LLM Wiki Trust Model

## Purpose

This page identifies what the LLM Wiki implementation trusts, what controls
are present in source, and what must not be assumed from design prose alone.
It is for integrators, maintainers, and incident responders. The module
security overview is [`SECURITY.md`](SECURITY.md); this document narrows that
overview to source-verifiable boundaries and residual risks.

## Trust boundaries

| Boundary | Data or decision | Source-grounded control | Residual risk |
|---|---|---|---|
| Client/query to plugin | Query text and options | Plugin implementation checks lower-cased unsafe-pattern substrings; `guardrail_patterns.h` provides a separate whitespace-normalizing pattern checker | The two matchers are distinct; the plugin-local matcher is a small list, not a complete prompt-injection defense |
| Plugin to edition configuration | Feature availability | Compile-time `THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED` selects Enterprise vs Community in the current gate | No runtime license verification is performed by this gate; edition enum values do not prove editions are detected |
| Process policy file to runtime | Stage and tuning configuration | YAML parsing, schema validation, and semantic checks reject invalid configured policy; optional reload preserves the prior in-memory policy on candidate failure | Policy file integrity and access permissions are operator responsibilities; the schema is not an authentication mechanism |
| Workspace files to module | Page/state/log data | JSON parsing and structural checks; state manager attempts log recovery | The checksum implementation is FNV-1a with a `sha256:` prefix, not cryptographic SHA-256; local filesystem writers are trusted |
| Embedding provider to retrieval | Text/vector output and possible credentials | Provider options are declared in SDK; provider behavior depends on concrete implementation/build | Do not assume provider names imply implemented backends or secure secret handling |

## Guardrails are a filter, not an authorization system

The guardrail implementation uses case-insensitive substring matching after
normalization in `guardrail_patterns.h`. The plugin implementation contains a
separate smaller matcher and does not thereby inherit every pattern in the
header. Such filters can flag known phrases but cannot establish intent,
content safety, tenant authorization, or resistance to novel/encoded attacks.
Do not use a clean guardrail result as permission to disclose protected data.

Workspace isolation depends on callers supplying and securing the correct
workspace root. The source does not show a complete user identity or
document-level authorization service in the plugin implementation. Enforce
authentication, authorization, tenant routing, and filesystem ownership at
the integrating service boundary.

## Credentials and sensitive data

Do not store provider keys, passwords, or other credentials in workspace
pages, state, policy files, or diagnostic logs. Supply secrets through the
deployment's secret manager and limit access to the process identity.
Workspace backups and incident artifacts may contain sensitive ingested
content; restrict access, retention, and transfer accordingly.

No claim of cryptographic provenance follows from the JSON entity schema or
workspace checksum. Provenance fields describe origin and transformations;
their authenticity depends on the trusted writer and storage path. The
workspace checksum is not tamper evidence.

## Security review and incident handling

- Treat wiki pages, ingested files, queries, policy files, and provider
  outputs as untrusted until validated by the owning boundary.
- Keep edition checks and process-policy invariants enabled. Do not bypass a
  denial by silently falling back to reduced or unguarded behavior.
- On suspected cross-workspace exposure, malicious policy change, or
  persisted-state tampering, preserve logs and workspace data, stop unsafe
  writes where needed, and escalate to security/on-call owners.
- Sanitize diagnostic artifacts; do not attach raw secrets or unrelated
  tenant content.
- See [`RUNBOOK.md`](RUNBOOK.md) for concrete triage and recovery steps.

## Related documents

- [`SECURITY.md`](SECURITY.md) — existing module security overview
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — implementation behavior and gaps
- [`MODELING.md`](MODELING.md) — entity/provenance data shape
- [`OPERATIONS.md`](OPERATIONS.md) — routine deployment controls

> Exact model backend not verifiable from final document text alone.
