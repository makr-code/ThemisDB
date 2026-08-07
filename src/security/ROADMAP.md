> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Security Module Roadmap

## Current Status
Production-grade security stack with transport/auth/access-control, encryption/key-management, auditing, and threat-detection components in active use.

## In Progress
- [~] Security hardening wave Phase 2+3: cryptographic assurance, policy enforcement consistency, and operational resilience (Target: Q4 2026)
  - [~] Phase 2 Cryptography & Key Management Hardening (Target: Q4 2026)
    - [x] Key-lifecycle validation tests: K-LIFE-01..K-LIFE-04 (tests/security/test_security_phase2_crypto_hardening_focused.cpp) (2026-08-07)
    - [x] Crypto error-path tests: K-ERR-01..K-ERR-04 (fail-closed enforcement) (2026-08-07)
    - [x] Key-provider failover tests: K-PROV-01..K-PROV-04 (Vault, HSM, PKI) (2026-08-07)
    - [x] Phase 2 benchmarks: K-ROT-01..K-ROT-04 in bench_security_phase2_crypto_gates.cpp (2026-08-07)
    - [ ] Production validation: Vault/HSM integration, failover testing
  - [~] Phase 3 Policy & Data-Protection Hardening (Target: Q4 2026)
    - [x] RLS regression tests: P-RLS-01..P-RLS-04 (tests/security/test_security_phase3_policy_hardening_focused.cpp) (2026-08-07)
    - [x] Policy-merge tests: P-MRG-01..P-MRG-04 (deny precedence, precedence rules) (2026-08-07)
    - [x] Deny-by-default tests: P-DENY-01..P-DENY-04 (timeout, concurrent updates) (2026-08-07)
    - [x] Query masking tests: P-MASK-01..P-MASK-02 (PII redaction, audit trail) (2026-08-07)
    - [x] Phase 3 benchmarks: P-MRG-01..P-MRG-05 in bench_security_phase3_policy_gates.cpp (2026-08-07)
    - [ ] Production validation: RLS under real query workloads, policy-merge edge cases

## Planned Features

### Short-term (3-6 months)
- [ ] Harden policy evaluation consistency across RBAC/ABAC/RLS enforcement paths (Target: Q4 2026)
- [ ] Expand key-rotation and key-provider failover validation under degraded external dependencies (Target: Q4 2026)
- [ ] Strengthen audit-evidence integrity and export reliability under high event volume (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Advance crypto-provider hardening and migration readiness across classical and PQ modes (Target: Q1 2027)
- [ ] Expand detection and response coverage for auth abuse and injection-style attack patterns (Target: Q1 2027)
- [ ] Improve zero-trust policy diagnostics and deny-by-default explainability for operators (Target: Q1 2027)

## Implementation Phases

### Phase 1: Access and Identity Hardening
- [x] Freeze security module API contract — transport/TLS, key lifecycle, policy evaluation, audit, threat detection, error taxonomy (include/security/security_api_contract.h) (Target: Q3 2026)
- [x] Define explicit SecurityErrorCode taxonomy (12+ codes: CERT_VALIDATION_FAILED, KEY_NOT_FOUND, KEY_ROTATION_IN_PROGRESS, POLICY_DENY, AUDIT_WRITE_FAILED, THREAT_DETECTED, ACCESS_DENIED, ENCRYPTION_FAILED, …) (Target: Q3 2026)
- [x] Re-validate authentication/session/control paths for fail-closed behavior under edge cases (Target: Q3 2026)
- [x] Strengthen token/session invalidation and revocation guarantees (Target: Q3 2026)

### Phase 2: Cryptography and Key Management Hardening
- [~] Expand key lifecycle validation (create/rotate/revoke/recover) across providers (Target: Q4 2026)
  - [x] Key-lifecycle tests K-LIFE-01..K-LIFE-04 (2026-08-07)
  - [x] Key-provider benchmarks K-ROT-01..K-ROT-04 (2026-08-07)
  - [ ] Vault provider integration testing
  - [ ] HSM provider integration testing
  - [ ] PKI provider integration testing
- [~] Tighten crypto error-path handling and secure-default enforcement (Target: Q4 2026)
  - [x] Crypto error-path tests K-ERR-01..K-ERR-04 (2026-08-07)
  - [x] Key-provider failover tests K-PROV-01..K-PROV-04 (2026-08-07)
  - [ ] Production failure-injection matrix validation

### Phase 3: Policy and Data-Protection Hardening
- [~] Expand RLS/masking/policy-enforcement regression coverage under mixed query workloads (Target: Q4 2026)
  - [x] RLS tests P-RLS-01..P-RLS-04 (2026-08-07)
  - [x] Query masking tests P-MASK-01..P-MASK-02 (2026-08-07)
  - [x] Policy benchmarks P-MRG-01..P-MRG-05 (2026-08-07)
  - [ ] Real query workload testing
  - [ ] Mixed RLS+ABAC scenarios
- [~] Validate deny-by-default and policy-merge semantics under conflicting rule sets (Target: Q4 2026)
  - [x] Policy-merge tests P-MRG-01..P-MRG-04 (2026-08-07)
  - [x] Deny-by-default tests P-DENY-01..P-DENY-04 (2026-08-07)
  - [ ] Conflict resolution edge case validation
  - [ ] Concurrent policy update atomicity

### Phase 4: Threat Detection and Audit Hardening
- [x] Contract-hardening focused tests SEC-01..SEC-16 covering TLS/cert, key management, policy evaluation, and audit invariants (tests/security/test_security_contract_hardening_focused.cpp) (Target: Q1 2027)
- [x] Re-baseline detection latency and false-positive controls for security signal paths (Target: Q1 2027)
- [x] Ensure tamper-evidence and audit export behavior remains bounded and reliable at scale (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [x] Lock benchmark-backed release gates for security hot paths: SRG-01..SRG-06 in benchmarks/security/bench_security_release_gates.cpp (policy eval p99≤1ms, JWT p99≤500µs, key lookup p99≤100µs, audit write p99≤500µs, RBAC p99≤200µs, cert validation p99≤2ms) (Target: Q3 2026)
- [x] Keep security docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [x] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist
- [x] Tracking in progress
- [x] Contract header frozen: include/security/security_api_contract.h (Phase 1)
- [x] Contract-hardening tests: tests/security/test_security_contract_hardening_focused.cpp (Phase 4, SEC-01..SEC-16)
- [x] Release-gate benchmarks: benchmarks/security/bench_security_release_gates.cpp (Phase 5, SRG-01..SRG-06)
- [~] Phase 2 crypto hardening: key-lifecycle + error-path + failover tests + benchmarks (K-LIFE, K-ERR, K-PROV, K-ROT gates) (Target: Q4 2026)
  - [x] Tests: test_security_phase2_crypto_hardening_focused.cpp (2026-08-07)
  - [x] Benchmarks: bench_security_phase2_crypto_gates.cpp (2026-08-07)
  - [ ] Production integration validation
- [~] Phase 3 policy hardening: RLS + policy-merge + deny-by-default + masking tests + benchmarks (P-RLS, P-MRG, P-DENY, P-MASK gates) (Target: Q4 2026)
  - [x] Tests: test_security_phase3_policy_hardening_focused.cpp (2026-08-07)
  - [x] Benchmarks: bench_security_phase3_policy_gates.cpp (2026-08-07)
  - [ ] Production integration validation
- [x] Benchmark CMakeLists registered: benchmarks/security/CMakeLists.txt
- Nachweise: security focused tests, auth/policy regressions, crypto/key-provider tests, security benchmarks (Phase 1-5)
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some high-assurance runtime envelopes still require broader benchmark and regression evidence.
- Certain external dependency failure combinations need additional hardening validation.
- Policy explainability and operator-facing diagnostics continue to be refined.

## Breaking Changes
- Security public APIs in active major lines remain additive-first.
- Any behavior change requiring migration must be versioned and documented in changelog/migration notes.
