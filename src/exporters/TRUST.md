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

# Exporters Trust Model

Exact model backend not verifiable from final document text alone.

## Trust boundaries

1. **Caller to exporter:** callers provide entities, format/configuration,
   collection/user identity, optional tenant context, policy engine, audit
   logger, and output destination. Those values must be validated and
   authorized by the deployment.
2. **Exporter to local artifact:** serialization, field projection,
   compression, and optional encryption determine what data is written.
   Filesystem permissions, retention, backup, and access control remain
   deployment responsibilities.
3. **Exporter to remote Hub:** `HuggingFaceHubClient` sends dataset content to
   a configured external endpoint using a bearer token. This is a separate
   egress boundary and must be approved for the data classification.

## Risks and controls

| Risk | Control and remaining limitation |
|---|---|
| Unauthorized export | `enforceExportPolicy()` checks a configured `PolicyEngine` before output/cursor activity. A deployment that omits the policy engine does not gain authorization from this library alone. |
| Excess field disclosure | Use explicit include/exclude fields and predicate filters. Review the final artifact; format/template-specific field mapping can omit or reshape fields. |
| PII leakage | `PIIDetector` supports pattern-based detection and mask/hash/remove/partial redaction. Detection is configurable and disabled by default in several exporter configs; pattern matching is not a complete privacy guarantee. |
| Plaintext artifact exposure | Encryption is optional. When configured, the encryption path uses AES-256-GCM and a key-provider/KEK reference; protect the destination, temporary files, backups, and key provider separately. |
| Credential leakage | Hub auth accepts token/key-provider configuration. Prefer the approved key-provider path; never place tokens in logs, issue text, or diagnostic bundles. |
| Unapproved external transfer | Confirm repo identity, visibility, policy approval, endpoint, and included fields before upload. A public Hub repository can expose data beyond the ThemisDB trust boundary. |
| Resource exhaustion | Configure file/throughput limits, streaming buffer/page size, join memory budget, and Hub timeout/retries. Zero-valued shared limits may mean unlimited. |

## Key and identity handling

The shared `ExportOptions` has an optional `PolicyEngine*`, `AuditLogger*`,
requesting user, collection name, and optional tenant context. The caller must
populate them consistently with the authenticated request; a caller-supplied
identity string is not itself authentication. Policy failures must remain
fail-closed.

`ExportEncryptionConfig` references a KEK identifier and `KeyProvider`;
documented code paths derive job data keys and do not write raw key material
to disk or logs. Keep key-provider access least-privileged, rotate according
to deployment policy, and restrict access to both plaintext staging files and
encrypted outputs. Do not assume that every exporter or every output path
automatically encrypts unless that path is configured and verified.

## Incident handling

For suspected unauthorized export or PII disclosure, stop new exports and
uploads, contain access to local and remote artifacts, preserve audit evidence,
and notify the security and data-owner incident contacts. Revoke or rotate
affected credentials where appropriate. Determine the affected collection,
request identity, artifact destinations, upload status, and time window using
sanitized logs and audit records. Do not copy sensitive payloads into
diagnostic tickets.

Operational triage and recovery steps are in [RUNBOOK.md](./RUNBOOK.md).
Production requirements are defined by
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md); this trust note
does not replace deployment authorization or security review.
