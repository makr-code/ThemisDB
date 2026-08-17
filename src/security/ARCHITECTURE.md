> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Security Module - Architecture Guide

**Version:** 1.2
**Last Updated:** 2026-08-17
**Module Path:** `src/security/`

## 1. Overview

The security module provides encryption, key/provider integration, access-control/policy enforcement, detection, and audit-evidence surfaces for ThemisDB.

## 2. Design Principles

- Fail-closed behavior on invalid or unsafe security states.
- Deny-by-default policy behavior for access control paths.
- Separation of cryptographic key/provider responsibilities from data-plane logic.
- Source-verifiable security controls over speculative claims.

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `field_encryption.cpp` / `encrypted_field.cpp` | field-level encryption/decryption paths |
| `vault_key_provider.cpp` / `pki_key_provider.cpp` / `hsm_provider_pkcs11.cpp` | key/provider integration |
| `rbac.cpp` / `access_control_manager.cpp` / `row_level_security.cpp` | authorization and row-level enforcement |
| `query_masking_policy.cpp` / `pii_redaction_policy.cpp` | masking and PII policy behavior |
| `aql_injection_detector.cpp` | query-injection detection surface |
| `zero_trust_policy_enforcer.cpp` | request-level policy enforcement |
| `security_evidence_collector.cpp` | security evidence aggregation and export surfaces |
| `behavioral_anomaly_detector.cpp` / `malware_scanner.cpp` | detection and threat-signal paths |

### 3.2 Data Flow (Simplified)

```
Request/data path
  -> access-control/policy decision
  -> optional masking/redaction
  -> encryption/decryption/key-provider paths (where required)
  -> evidence/audit signal emission

Detection path
  -> query/content/security event input
  -> detector/scanner scoring or validation
  -> security status/event output
```

## 4. Integration Points

| Direction | Module | Interface |
|---|---|---|
| Used by | `src/server/` | request-time policy and security checks |
| Used by | `src/query/` | query-level security checks and masking |
| Used by | `src/storage/` | encryption/key-provider integration points |
| Uses | utility/audit components | evidence and chained audit output |

## 5. Threading and Concurrency Model

- Security control paths are designed for concurrent runtime usage.
- Shared registries/caches/providers are protected by explicit synchronization in their implementations.
- Security-sensitive state transitions must remain deterministic under concurrency.

## 6. Security and Reliability Considerations

- Invalid critical security state should lead to deny/reject behavior.
- Key/provider dependency failures are handled through explicit error paths.
- Detection and audit/evidence paths must remain operationally bounded.

## 7. Known Limitations and Future Work

- Some high-assurance runtime envelopes still require broader benchmark evidence.
- Some dependency-failure combinations remain under ongoing hardening verification.
- A fresh full security-gap rescan and remaining non-TSA Batch-4 closure work are still open in `MODULE_GAPS.md`.
- Operator-facing policy and diagnostics clarity is still being improved.

## 8. Sourcecode Verification (Module: security/architecture)

- Verified files:
  - `src/security/field_encryption.cpp`
  - `src/security/vault_key_provider.cpp`
  - `src/security/hsm_provider_pkcs11.cpp`
  - `src/security/pki_key_provider.cpp`
  - `src/security/rbac.cpp`
  - `src/security/access_control_manager.cpp`
  - `src/security/row_level_security.cpp`
  - `src/security/query_masking_policy.cpp`
  - `src/security/aql_injection_detector.cpp`
  - `src/security/zero_trust_policy_enforcer.cpp`
  - `src/security/security_evidence_collector.cpp`
- Verified interfaces/behaviors:
  - encryption and key-provider surfaces
  - policy enforcement and access-control behavior
  - detection and security evidence collection paths
