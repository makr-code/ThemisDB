---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Security - Governance Module

Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md).
This page describes module-level security-relevant interfaces; it does not
assert product-wide certification or guarantee that every caller applies them.

## Trust and enforcement limits

- `PolicyEngine` returns decision data. Every consuming caller must enforce the
  relevant allow/export/cache/ANN/redaction restrictions at its operation
  boundary. Native handling of missing or unknown classification is strict, but
  this does not establish fail-closed behavior across every evaluator and caller.
- OPA REST failure can fall back to native evaluation. This is not equivalent to
  approval by the unavailable OPA policy.
- Without an injected WASM evaluator, the built-in WASM stub may return a
  permissive `observe` decision when a bundle exists and the real runtime is
  unavailable. Do not use this path for production authorization.
- `DataMasker` returns input unchanged when masking is disabled or no rules are
  present. It applies configured rules to matching JSON string fields; callers
  must invoke it on every relevant output path.
- Tokenization uses HMAC-SHA-256 when a nonempty collection secret is provided;
  the current fallback can use unkeyed SHA-256. That fallback does not provide
  keyed pseudonym separation.
- Subject-rights erasure operates on registered targets independently and can
  partially fail. Inspect `fully_erased` and each store result; there is no
  cross-store rollback.
- Lineage and operational audit data may be bounded/in-memory. Their presence
  does not establish durable audit retention or key custody.

## Security follow-ups

- Validate integration and caller enforcement at each protected operation.
- Verify audit storage, key management, and retention in the deployment.
- Continue focused regression coverage for evaluator failures, masking,
  lineage, and multi-store erasure.

## References

- [Trust boundaries](TRUST.md) · [Runbook](RUNBOOK.md)
- [`policy_engine.h`](../../include/governance/policy_engine.h)
- [`opa_adapter.h`](../../include/governance/opa_adapter.h)
- [`data_masker.h`](../../include/governance/data_masker.h)
- [`gdpr_subject_rights.h`](../../include/governance/gdpr_subject_rights.h)
