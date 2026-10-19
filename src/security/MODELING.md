---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-19
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Security Module Modeling Notes

AI provenance note: Exact model backend not verifiable from final document text alone.


This document models the security module as a collection of enforcement domains, trust boundaries, and failure-driven control loops. It follows the source contract in `include/security/security_api_contract.h`, the public interfaces in `include/security`, and the runtime implementations in `src/security`.

## 1. Core model

The module can be modeled as four coordinated control planes:

1. Identity and access control
   - `AccessControl` and `RBAC` define user/session and permission evaluation.
   - `ZeroTrustPolicyEnforcer` adds network and risk-based verification.

2. Key and data protection
   - `KeyProvider` and `VaultKeyProvider`/`PKIKeyProvider` establish key lifecycle semantics.
   - `field_encryption.cpp` binds ciphertext to `key_id`, `key_version`, and metadata so decryption is version-aware.

3. Threat and query monitoring
   - `AQLInjectionDetector` and related detection logic reject suspicious or unsafe query patterns before execution.
   - `BehavioralAnomalyDetector` and `MalwareScanner` provide anomaly and threat-signal surfaces.

4. Evidence and operational assurance
   - `SecurityEvidenceCollector` produces logs, metrics, and retention-aware evidence bundles for audits and operator review.

## 2. Trust boundaries

The module defines clear trust boundaries around:

- external clients and network identity
- permission-bearing principals and roles
- key providers and HSM/Vault/PKI backends
- query strings and AI tool actions that can mutate state
- audit data generated from security decisions

The most important invariant is fail-closed handling. A missing policy, untrusted token, invalid certificate, unavailable key, or inconsistent audit channel does not result in silent allowance.

## 3. State model and lifecycle

### Key lifecycle

The design in `security_api_contract.h` requires the following ordering:

- generate a key
- publish it as active or rotating
- allow a bounded overlap period during rotation
- revoke the old key after the overlap window
- deny use of revoked or missing keys

This is observable in `include/security/key_provider.h` and in the provider implementations. The contract explicitly constrains rotation behavior and the associated safety window.

### Policy lifecycle

Authorization is modeled as policy lookup plus evaluation. Conflicting or missing policy state must deny. This is expressed in the contract and enforced in runtime checks.

### Threat lifecycle

Signals travel through detection and response stages:

- source event or payload
- suspicious pattern detection or score computation
- decision output (allow, block, require approval, deny)
- audit/evidence emission

## 4. Error and safety model

The implementation distinguishes between:

- invalid input and malformed requests
- dependency outages or unavailable providers
- policy conflicts and timeouts
- security violations and operational degradation

The module treats dependency failure as a security event, not a harmless fallback. This is especially visible in the HSM provider fallback logic and in the policy evaluation timeout contract.

## 5. Operational model

A secure runtime is not only about cryptography; it is about how the system fails under stress:

- `ZeroTrustPolicyEnforcer::verify` returns `Deny` on identity or network failure.
- `AQLInjectionDetector` rejects suspicious patterns before queries execute.
- `SecurityEvidenceCollector` is designed to preserve a verifiable audit trail for review.
- `AiOperationGuard` constrains AI-initiated destructive actions by class and environment.

## 6. Limits of the model

The model is intentionally bounded to what the code actually implements. It does not assume universal production finalization for every control. The roadmap continues to track remaining gaps such as full production HSM certification and residual audit-gap closure. This is why the security module documents itself as production-evidenced but not universally closed.

## 7. Consequences for design decisions

The modeling choices follow a conservative pattern:

- prefer explicit denial over implicit trust
- bind cryptographic material to key metadata and versioning
- require evidence for security decisions and changes
- classify harmful operations before they run
- keep policy decisions deterministic and auditable

This is a practical security architecture rather than a purely abstract policy layer.
