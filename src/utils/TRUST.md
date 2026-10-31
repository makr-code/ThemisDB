---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Utils Trust Boundaries

## Purpose

This page identifies trust assumptions for developers and operators using
shared utility APIs. Utils provides security-sensitive mechanisms, but it is
not itself an authorization boundary or a substitute for application policy.
Report vulnerabilities through the project
[`SECURITY.md`](../../SECURITY.md) process.

## Boundaries and controls

| Boundary | Risk | Control and residual responsibility |
|---|---|---|
| Caller input → regex/NER/scanner | Malformed, oversized, adversarial, or chunk-split input can consume resources or produce incomplete findings | Regex input/pattern/time bounds, NER gazetteer caps, and scanner lookahead are implemented in the respective APIs. Callers must handle errors/sentinels and must not treat a failed or incomplete scan as a clean scan. |
| PII engine configuration → factory | Tampered configuration or untrusted engine metadata | `createSigned` verifies a configuration hash and PKI signature before initialization. `createUnsigned` bypasses this verification and is suitable only for trusted built-ins/test contexts; do not expose it as an untrusted plugin-loading path. |
| Runtime → PKI/network | Untrusted peer, invalid certificate, pinning misconfiguration, or missing trust material | Configure a trust store and certificate pins for the intended production path. When pinning is enabled, invalid or missing usable pin material must fail closed. Confirm `verifyHash` behavior for the deployed configuration; the PKI header documents that absent trust-store configuration can skip chain validation. |
| Key manager → storage/key provider | Key loss, unauthorized key access, stale/revoked key, or persistence failure | Keep key providers and storage protected and alive for the manager lifetime. Respect rotation/revocation and error results; key unavailability must not cause plaintext fallback. |
| PII mapping → caller | Unauthorized reveal or accidental disclosure through diagnostics | Gate reveal operations with application authorization. `user_id` is an audit argument, not proof of authorization by itself. Avoid placing raw PII or key material in logs/traces. |
| Compressed/serialized data → decoder | Decompression bombs, malformed frames, nesting/size exhaustion | Prefer safe `Result` APIs, preserve encoded/decompressed size limits, and reject malformed input. Do not replace explicit errors with empty/default payloads. |

## Security assumptions and residual risks

- Detection rules can miss values or flag benign text. Detection is not
  authorization, complete DLP, or proof of non-sensitivity.
- `PIIDetector::detectInText` logs and continues after an individual engine
  throws; callers that require all-engine success need a separate explicit
  completeness signal or a fail-closed calling path.
- Pseudonymization depends on correct key and storage configuration. A
  returned identifier does not grant reveal access; the caller must authorize
  reveal and audit its use.
- Utility thread safety is API-specific. Do not infer safety for shared
  mutable contexts from the existence of a thread-safe helper elsewhere.
- Optional codecs and external dependencies can be unavailable. Callers must
  inspect the declared error/result rather than assuming a transparent,
  security-equivalent fallback.

## Incident handling

For suspected key compromise, signature failure, audit-integrity loss,
privacy exposure, or unsafe trust configuration, stop the affected sensitive
operation, preserve sanitized error context, and escalate through the root
security process. Rotate/revoke keys only through the approved key lifecycle;
do not delete evidence or disable verification to restore availability.
Operational symptom handling is in [`RUNBOOK.md`](RUNBOOK.md), and controls
must remain aligned with [`SECURITY.md`](SECURITY.md).
