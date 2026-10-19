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

# Security Module Implementation Notes

AI provenance note: Exact model backend not verifiable from final document text alone.


This document maps the implementation structure of the security module to the code that currently exists in `src/security` and the public interfaces in `include/security`.

## Relevant implementation files

- `src/security/field_encryption.cpp` — field-level AES-256-GCM encryption and serialization.
- `src/security/hsm_provider_pkcs11.cpp` — PKCS#11 integration and hardware-backed fallback gate.
- `src/security/zero_trust_policy_enforcer.cpp` — request verification with trust and risk scoring.
- `src/security/aql_injection_detector.cpp` — query validation and suspicious-token detection.
- `src/security/security_evidence_collector.cpp` — audit bundles and evidence export.
- `src/security/behavioral_anomaly_detector.cpp` — anomaly detection logic.
- `src/security/access_control_manager.cpp` and `src/security/rbac.cpp` — role and permission management.
- `src/security/ai_operation_guard.cpp` — approval and classification logic for destructive AI tools.

## Implementation principles

The current code follows several clear principles:

1. Explicit risk handling
   - any error in key provider access, policy evaluation, or identity verification produces a deny or exception path rather than an implicit allow.

2. Separation of policy from storage
   - key management and policy enforcement are implemented as dedicated subsystems with their own contracts and failure modes.

3. Deterministic handling for security events
   - evidence collection, anomaly detection, and policy denial produce structured outputs rather than unbounded logs.

4. Versioned key and policy material
   - versions are tracked in the key metadata and rotation workflows to avoid blind use of an outdated or revoked key.

## Key implementation details

### Field encryption

`field_encryption.cpp` performs the core cryptographic work: it serializes encrypted fields with metadata, validates input, and uses OpenSSL-backed AES-256-GCM operations. The code is intentionally defensive about invalid blobs and explicitly avoids raw-key exposure in debug output.

### Key provider interface

`include/security/key_provider.h` defines the canonical lifecycle contract for key retrieval, rotation, deletion, and metadata lookup. The design supports versioned access and rotation semantics and keeps key status explicit (`ACTIVE`, `ROTATING`, `DEPRECATED`, `DELETED`).

### Policy enforcement

The fail-closed pattern is central to runtime enforcement. In `zero_trust_policy_enforcer.cpp`, identity, network policy, and risk thresholds are evaluated before a request is allowed. The request is denied when the token is invalid, the IP is disallowed, or the risk score exceeds the threshold.

### AI operation governance

`ai_operation_guard.h` defines operation classes for read-only, safe writes, destructive actions, and critical actions. The classification path is used to require approvals or block critical operations in production environments, which keeps AI-driven mutation under human review.

### Audit evidence

The material stored in `security_evidence_collector.*` is not an optional feature. It is a structured evidence flow used for operator diagnosis, compliance review, and incident response. The bundle model includes retention windows, key-rotation records, access-control reports, and network control snapshots.

## Operational constraints

The implementation is honest about the gaps it still carries. The security module roadmap documents production-grade evidence while also recording that a full fresh security-gap rescan and residual closure tasks remain open. This is a boundary condition of the current implementation state and should be respected during operations and documentation updates.

## Validation and code quality expectations

The module's core behavior is verified by tests under `tests/security`, including contract hardening and production validation suites. The implementation should remain conservative, deterministic, and deny-by-default under edge cases rather than trying to recover silently from security failures.
