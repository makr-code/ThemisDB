> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Security Module Roadmap

## Current Status
Production-grade security stack with transport/auth/access-control, encryption/key-management, auditing, and threat-detection components in active use.

## In Progress
- [~] Security hardening wave for cryptographic assurance, policy enforcement consistency, and operational resilience (Target: Q3 2026)
  - [x] Compiler/linker security hardening flags: stack-protector, FORTIFY_SOURCE=3, PIE/ASLR, RELRO, CFG (SEC-CC-4) (Target: Q3 2026)
  - [x] CMake presets for ASAN/UBSAN sanitizer builds: community-asan, community-ubsan, linux-asan, linux-ubsan (SEC-CC-4) (Target: Q3 2026)
  - [x] CI sanitizer jobs (ASan + UBSan) in cmake-multi-platform.yml (Target: Q3 2026)
  - [ ] Complete remaining verification for high-assurance crypto/runtime configurations (Target: Q3 2026)
  - [ ] Tighten failure-path behavior and observability for access-control and key-management surfaces (Target: Q3 2026)

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
- [ ] Expand key lifecycle validation (create/rotate/revoke/recover) across providers (Target: Q4 2026)
- [ ] Tighten crypto error-path handling and secure-default enforcement (Target: Q4 2026)

### Phase 3: Policy and Data-Protection Hardening
- [ ] Expand RLS/masking/policy-enforcement regression coverage under mixed query workloads (Target: Q4 2026)
- [ ] Validate deny-by-default and policy-merge semantics under conflicting rule sets (Target: Q4 2026)

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
- [x] Benchmark CMakeLists registered: benchmarks/security/CMakeLists.txt
- Nachweise: security focused tests, auth/policy regressions, crypto/key-provider tests, security benchmarks
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some high-assurance runtime envelopes still require broader benchmark and regression evidence.
- Certain external dependency failure combinations need additional hardening validation.
- Policy explainability and operator-facing diagnostics continue to be refined.

## Breaking Changes
- Security public APIs in active major lines remain additive-first.
- Any behavior change requiring migration must be versioned and documented in changelog/migration notes.
