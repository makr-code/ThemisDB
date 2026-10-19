---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-19
Status: active
AI_Used: true
AI_Models: MAI-Code-1.1-Flash
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Security Module Research Notes

AI provenance note: Exact model backend not verifiable from final document text alone.


The security module is grounded in the code paths under `src/security`, the public API contract in `include/security/security_api_contract.h`, and the module-level controls documented in `ARCHITECTURE.md`, `ROADMAP.md`, and `SECURITY.md`.

## Scope and evidence base

This document describes the research and engineering assumptions behind the module rather than a marketing summary. Its primary evidence is the concrete source in the following files:

- `include/security/security_api_contract.h` — transport, key lifecycle, policy evaluation, audit, and threat-detection contracts
- `include/security/key_provider.h` — the primary key lifecycle API and failure semantics
- `include/security/access_control.h` — authentication and authorization orchestration
- `include/security/ai_operation_guard.h` — destructive AI operation classification and approval guardrails
- `src/security/aql_injection_detector.cpp` — query sanitization and AST-level attack detection
- `src/security/zero_trust_policy_enforcer.cpp` — fail-closed request verification and trust scoring
- `src/security/hsm_provider_pkcs11.cpp` — HSM integration and the deliberate stub/fallback pattern with runtime guarding

## Why this module exists

ThemisDB treats identity, authorization, data protection, and audit evidence as first-class runtime properties. The module does not assume a trusted, single-process internal environment; it enforces boundaries that are explicit in code and in the contract header. The `security_api_contract.h` file sets the minimum production requirements for TLS 1.3, fail-closed authorization, key rotation overlap behavior, audit retention, and threat-detection latency.

These rules are not advisory. They are encoded as invariants and are used by tests and production validation paths, including the security-focused suites under `tests/security` and the release-gate benchmarks in `benchmarks/security`.

## Research questions the implementation addresses

1. Can data be protected before it leaves application boundaries?
   - `field_encryption.cpp` and `encryption.h` implement field-level AES-256-GCM encryption and key metadata serialization.
   - The encrypted blob format keeps key identifiers and version information with the payload so decryption can be bound to the correct active key version.

2. Can policy decisions fail closed instead of silently allowing unsafe access?
   - `security_api_contract.h` explicitly requires missing or conflicting policy state to deny.
   - `zero_trust_policy_enforcer.cpp` performs identity verification, network policy checks, and trust scoring before allowing access.

3. Can destructive AI actions be classified before they execute?
   - `ai_operation_guard.h` models operation classes from read-only to critical mutation. The runtime path rejects or escalates according to environment and caller capabilities.

4. Can classification and validation keep up with query-language attack patterns?
   - `aql_injection_detector.cpp` rejects empty input, dangerous mutation statements, suspicious AST nodes, and suspicious literals before execution enters the query path.

5. Can the operator understand what happened after a security decision?
   - `security_evidence_collector.h` is dedicated to tamper-evident audit bundles, retention windows, and key-rotation evidence. This is a design response to the policy that security events must be explained and auditable.

## Research constraints and assumptions

- The module assumes external dependencies such as Vault, HSM, PKCS#11, and TLS certificate providers can fail or be unavailable. This is why fail-closed conditions and explicit error paths are a first-class requirement rather than an afterthought.
- The API contract is intentionally strict about the minimum standards: TLS 1.3, key rotation overlap windows, deny-by-default policy evaluation, and audit retention. This is a security assumption, not a convenience feature.
- The implementation treats a runtime fallback from HSM-backed cryptography as a controlled, observable, and opt-in exception path rather than a default production behavior. In `hsm_provider_pkcs11.cpp`, the fallback path is accompanied by explicit safeguarding and runtime gating.

## Trade-offs and failure modes

- Security latency is real. Policy evaluation has a hard timeout in the API contract, and many enforcement paths are intentionally designed to block work rather than permit degraded access.
- Key lifecycle correctness is more important than a permissive system. The contract prioritizes safe rotation and revocation semantics over convenience or broad compatibility.
- Audit evidence is retained and exported in structured form; the cost is additional memory and I/O, but the benefit is operational traceability and incident handling.

## Evaluation and validation path

The design is validated by tests in `tests/security`, especially the production-focused validation and contract-hardening test groups, and by benchmark gates in `benchmarks/security`. The docs intentionally align with the roadmap status: security capabilities are strong, but full production sign-off remains pending until remaining security gaps are closed as tracked in `MODULE_GAPS.md` and `ROADMAP.md`.

## Conclusion

The security module is a control plane for trust, access, confidentiality, and evidence. It is deliberately skeptical of ambient trust: default deny, explicit rotation, authenticated enforcement, and observable audit trails are the core research assumptions that shape the implementation.
