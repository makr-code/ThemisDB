> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-31 -->
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
- Security orchestration and evidence collection (`security_manager`, `security_evidence_collector`).

## Known Limitations

- Some high-assurance dependency-failure combinations still require broader validation evidence.
- Certain performance envelopes for hot security paths need continuous benchmark-backed monitoring.

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
  - `src/security/security_manager.cpp`
  - `src/security/security_evidence_collector.cpp`
- Verified controls:
  - policy/access decisions
  - encryption/key-provider behavior
  - detection and evidence/audit-related security paths
