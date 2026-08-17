# Security - Utils Module

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the utils module focuses on safe shared-helper behavior for audit logging, privacy processing, key derivation, compression boundaries, and defensive failure handling in reusable runtime support code.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| audit or log integrity regressions | dedicated audit and structured logging surfaces |
| privacy leakage through shared helpers | PII scan and pseudonymization code paths |
| unsafe key-handling regressions | HKDF and LEK management helper paths |
| hidden degradation in shared runtime helpers | explicit utility-layer error handling and follow-up hardening |

## Implemented Security Controls

- audit and logging behavior is kept in dedicated utility surfaces.
- privacy scan and pseudonymization paths are isolated from unrelated runtime helpers.
- key derivation and local key lifecycle logic remain explicit and separable.
- compression and runtime helper boundaries are documented and benchmarked where hot-path relevant.
- PKI REST paths enforce fail-closed TLS pinning when pinning is enabled (`CURLOPT_PINNEDPUBLICKEY`).
- Error registry read/write paths are synchronized (`std::shared_mutex`) for concurrent callers.

## Security Follow-ups

- deepen validation around privacy false-negative and pseudonymization edge behavior.
- tighten key-material handling and lifecycle diagnostics across helper boundaries.
- broaden stress and failure-path coverage for shared runtime helper misuse or overload.
- refresh scanner aggregates in MODULE_GAPS.md after 2026-08-17 pki/error-registry hardening.

## Sourcecode Verification (Module: utils/security)

- Verified files:
  - src/utils/audit_logger.cpp
  - src/utils/pii_detection_engine.cpp
  - src/utils/pii_pseudonymizer.cpp
  - src/utils/hkdf_helper.cpp
  - src/utils/lek_manager.cpp
  - src/utils/input_validator.cpp
  - src/utils/pki_client.cpp
  - src/utils/error_registry.cpp
- Verified controls:
  - dedicated audit and privacy helper surfaces
  - explicit key-derivation and local key lifecycle paths
  - bounded shared-helper failure behavior
  - fail-closed TLS pinning semantics for PKI REST integration
  - synchronized registry access for concurrent high-fan-out error lookups