> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Security Module - Architecture Guide

**Version:** 1.2
**Last Updated:** 2026-09-09
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

## 8. Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` (crypto helpers, string sanitization) | Low-level cryptographic primitives and input-sanitization utilities |
| metadata | `include/metadata/schema_audit_log.h` | Schema-level audit log integration for DDL-event security evidence |
| observability | `include/observability/audit_logger.h`, `include/observability/metrics_collector.h` | Structured security-event audit emission and policy-decision metric recording |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/security/rbac.h`, `include/security/zero_trust_policy_enforcer.h` | Request-time RBAC decisions and zero-trust policy evaluation at HTTP ingress |
| query | `include/security/query_masking_policy.h`, `include/security/pii_redaction_policy.h`, `include/security/aql_injection_detector.h` | Result-set masking, PII redaction, and injection detection during query execution |
| storage | `include/security/security_signature.h`, `include/security/field_encryption.h` | Storage-layer write signing and field-level encryption/decryption |
| index | `include/security/field_encryption.h` | Encrypted index field reads/writes |
| llm | `include/security/pii_redaction_policy.h` | PII stripping before LLM prompt submission |
| rag | `include/security/pii_redaction_policy.h` | PII stripping in retrieved context before generation |

## 9. Integration Points

### Critical Integration: RbacManager ↔ Server Request Pipeline
**Files:** `include/security/rbac.h` ↔ `src/server/http_server.cpp`
**Contract:** `RbacManager::authorize(principal, resource, action)` returns `ALLOW` or `DENY`; the server must not route the request further on `DENY`. Decision is synchronous and must complete within the configured policy timeout.
**Thread Safety:** `RbacManager` is read-concurrent after policy load; policy reloads use an atomic swap under `std::shared_mutex`.
**Failure Mode:** Deny-by-default — any exception or timeout from `authorize()` is treated as `DENY`.

### Critical Integration: FieldEncryption ↔ Storage Write Path
**Files:** `include/security/field_encryption.h` ↔ `src/storage/` write path
**Contract:** Designated fields are encrypted before serialisation using the active `KeyProvider` DEK. The storage layer must not persist plaintext for encrypted-field columns.
**Thread Safety:** `FieldEncryption` instances are stateless post-init; `KeyProvider` access is internally synchronized.
**Failure Mode:** `KeyProvider` unavailability propagates as `EncryptionKeyUnavailable`; the write is rejected, not silently stored in plaintext.

### Critical Integration: AqlInjectionDetector ↔ Query Module
**Files:** `include/security/aql_injection_detector.h` ↔ `src/query/` parse pipeline
**Contract:** `AqlInjectionDetector::scan(query_ast)` returns a risk score and optional violation list; queries exceeding the configured threshold are rejected before execution planning.
**Thread Safety:** Detector is stateless; safe for concurrent calls.
**Failure Mode:** Scanner error returns `SCAN_ERROR`; query execution is blocked, not passed through.



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
