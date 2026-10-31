---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Trust Boundaries — Themis Core

## Purpose

This page describes the trust boundaries in `src/themis` for maintainers and
operators. It is scoped to the core build, license/edition, module lifecycle,
dependency, and wire surfaces; it does not replace the repository security
policy or deployment-specific key-management requirements.

## Boundaries and controls

| Input or boundary | Trust assumption and control |
|---|---|
| Build configuration and binary metadata | Build identity is reported by the running binary. Protect build manifests: `ReproducibilityInfo` can include branch, host, user, dependency, and binary-hash data. |
| License server and embedded license data | License responses and embedded data are inputs to license validation and runtime gate decisions. Protect license API keys and license files; do not log their secret values. |
| Edition and feature names | Compile-time feature flags and runtime license checks jointly restrict named premium features. Unknown feature behavior differs by API; never expose these checks as a general authorization system for arbitrary user-provided names. |
| Module file and expected hash manifest | Hash comparison detects divergence from an expected digest, but a manifest must itself come from a trusted source. Hash matching does not authenticate the publisher. |
| Module signature and platform trust store | Authenticode/GPG verification depends on certificate or keyring trust configuration. A valid signature authenticates according to that trust configuration; it does not prove the module is safe or authorized for every capability. |
| Dependency metadata | Dependency names, versions, and graph constraints affect activation order. Reject unresolved or cyclic dependencies; do not override failures to force activation. |
| Wire peer and payload | Treat remote bytes, frame lengths, and peer lifecycle as untrusted. Preserve input validation, bounded resource handling, and explicit error behavior. |

The verifier API has an important limitation: `verifyLicenseSignature` is
documented to return true if a signature is absent. Callers must separately
require a signature when policy requires one. The signature and hash verifiers
also do not replace edition authorization or dependency checks.

## Secrets and incident handling

- Keep license-server API keys, signing private keys, and deployment trust
  material in the approved secret/key-management system; never commit them or
  include them in diagnostic output.
- Treat build host/user fields and module paths as potentially sensitive
  operational metadata when exporting or sharing build manifests.
- On a hash/signature mismatch or unexplained trust decision, preserve the
  artifact and logs, stop activation, and escalate through the security
  incident process. Redeploy only an artifact verified against the trusted
  release source.
- Do not disable a gate, weaken a trust store, or recompute expected hashes
  from an unverified artifact as an incident workaround.

See [`SECURITY.md`](./SECURITY.md) for the module threat summary and
[`RUNBOOK.md`](./RUNBOOK.md) for response steps.
