> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-08-17 -->
# Security - Security Module

> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|---|---|
| Unauthorized data access via policy bypass | RBAC/ABAC/RLS and masking policy enforcement paths |
| Key/provider degradation causing unsafe crypto behavior | explicit key-provider error handling and secure-default behavior |
| Injection abuse in query input paths | AQL injection detection and validation surfaces |
| Audit/evidence tampering risk | tamper-evident audit/evidence pipeline integration |
| Elevated auth abuse patterns | anomaly/rate-limiting detection surfaces |

## Security Controls

- Access-control and policy enforcement surfaces (`rbac`, `access_control_manager`, `row_level_security`).
- Encryption and key-provider paths (`field_encryption`, vault/HSM/PKI providers).
- Detection controls (`aql_injection_detector`, anomaly/detection helpers, malware scanning).
- Security evidence collection (`security_evidence_collector`) plus runtime policy/detection controls (`zero_trust_policy_enforcer`, `behavioral_anomaly_detector`).

## Known Limitations

- Some high-assurance dependency-failure combinations still require broader validation evidence.
- Certain performance envelopes for hot security paths need continuous benchmark-backed monitoring.
- A fresh full security-gap rescan is still open; final module-level production-ready sign-off remains pending until the residual findings in `MODULE_GAPS.md` are closed.

## Current PR Scope and Deferred Work

The current PR is scoped to the work that is feasible without claiming full end-to-end security readiness:

- ✅ Align the public security contract and policy baseline to the repo’s enforced minimum of TLS 1.3 for production transport.
- ✅ Remove stale TLS 1.2-era wording from the published security docs and acceptance checklist.
- ✅ Keep the repo’s security status honest by documenting that production-ready sign-off remains pending until the remaining gap scan and residual findings are closed.

The following items remain intentionally deferred and must be tracked as follow-up work rather than claimed as complete in this PR:

- ⚠️ Real PKCS#11 HSM signing integration for production deployments.
- ⚠️ Full RFC 3161 / eIDAS timestamp-authority hardening and production validation.
- ⚠️ Fresh full security-gap rescan replacing legacy Batch-4 aggregate summaries.
- ⚠️ Closure of residual non-TSA CRITICAL/HIGH findings tracked in `MODULE_GAPS.md`.
- ⚠️ Final module-level production-ready sign-off after the remaining security gap backlog is closed.

## Sourcecode Verification (Module: security/security)

- Verified files:
  - `src/security/rbac.cpp`
  - `src/security/access_control_manager.cpp`
  - `src/security/row_level_security.cpp`
  - `src/security/field_encryption.cpp`
  - `src/security/vault_key_provider.cpp`
  - `src/security/hsm_provider_pkcs11.cpp`
  - `src/security/pki_key_provider.cpp`
  - `src/security/aql_injection_detector.cpp`
  - `src/security/security_evidence_collector.cpp`
  - `src/security/zero_trust_policy_enforcer.cpp`
  - `src/security/behavioral_anomaly_detector.cpp`
- Verified controls:
  - policy/access decisions
  - encryption/key-provider behavior
  - detection and evidence/audit-related security paths
