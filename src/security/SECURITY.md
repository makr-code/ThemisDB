---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# Security - Security Module

> Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md).
> This module page documents controls and risks; it is not a product security certification.

## Threat Model

| Threat | Mitigation |
|---|---|
| Unauthorized data access via policy bypass | RBAC/ABAC/RLS and masking policy enforcement paths |
| Key/provider degradation causing unsafe crypto behavior | explicit key-provider error handling and secure-default behavior |
| Injection abuse in query input paths | AQL injection detection and validation surfaces |
| Audit/evidence tampering risk | tamper-evident audit/evidence pipeline integration |
| Elevated auth abuse patterns | anomaly/rate-limiting detection surfaces |

## Security Controls

- Production safeguards use the canonical `themis::core::ProductionMode` parser; unsupported
  `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT` values fail instead of silently selecting development mode.
- Access-control and policy enforcement surfaces (`rbac`, `access_control_manager`, `row_level_security`).
- Encryption and key-provider paths (`field_encryption`, vault/HSM/PKI providers).
- Detection controls (`aql_injection_detector`, anomaly/detection helpers, malware scanning).
- Security evidence collection (`security_evidence_collector`) plus runtime policy/detection controls (`zero_trust_policy_enforcer`, `behavioral_anomaly_detector`).

## Known Limitations

- Security posture depends on configured providers, runtime flags, and deployment controls. This page is not a module-readiness or product-certification decision; consult the active roadmap, gap tracker, and release evidence.
- `SecretManager` keeps secret versions in process memory; a process dump or debugger with sufficient access can expose them.
- `FieldEncryption::createDefault()` is a test/demo mock provider, not durable production key management. In non-test builds it requires explicit `THEMIS_ALLOW_MOCK_KEY_PROVIDER` opt-in.
- The HSM software stub is not equivalent to hardware protection. Keep `THEMIS_ALLOW_HSM_STUB` and `--allow-stub-hsm` disabled in production.

## Permission and Secret Handling

- RBAC matches resource/action permissions, including explicit `*` wildcards; review wildcard grants as broad privileges. Invalid role-hierarchy state denies permission checks.
- Supply a real key provider (for example Vault or HSM-backed) for persistent data. Vault configuration requires a token and HTTPS except for loopback development endpoints; keep the token out of source, logs, command lines, and unprotected configuration files.
- Treat key material and secrets returned to this process as exposed to process-level compromise. Restrict service-account, debugger, core-dump, and host access; rotate affected credentials after suspected exposure.
- Field-encryption debug output omits raw key bytes. Preserve that invariant when changing diagnostic or evidence paths.

## Incident Handling

For suspected key, policy, or secret exposure, stop affected privileged operations, preserve security/audit evidence, identify whether the boundary was identity, authorization, key provider, or process memory, and rotate compromised credentials through the configured provider before restoring service. Follow [SOP-05](../../SOP.md) for vulnerability disclosure and [SOP-08](../../SOP.md) for operational incidents.

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

## Source References

- `src/security/secret_manager.cpp`
- `src/security/hsm_provider.cpp`
- `src/security/field_encryption.cpp`
- `src/security/vault_key_provider.cpp`
- [Trust and risk model](TRUST.md)
