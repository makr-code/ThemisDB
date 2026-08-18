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
    - [x] Production validation: Vault/HSM/PKI hardening + failover/fail-closed matrix tests (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
  - [~] Phase 3 Policy & Data-Protection Hardening (Target: Q4 2026)
    - [x] RLS regression tests: P-RLS-01..P-RLS-04 (tests/security/test_security_phase3_policy_hardening_focused.cpp) (2026-08-07)
    - [x] Policy-merge tests: P-MRG-01..P-MRG-04 (deny precedence, precedence rules) (2026-08-07)
    - [x] Deny-by-default tests: P-DENY-01..P-DENY-04 (timeout, concurrent updates) (2026-08-07)
    - [x] Query masking tests: P-MASK-01..P-MASK-02 (PII redaction, audit trail) (2026-08-07)
    - [x] Phase 3 benchmarks: P-MRG-01..P-MRG-05 in bench_security_phase3_policy_gates.cpp (2026-08-07)
    - [x] Production validation: real-query-workload simulation + policy-merge/atomicity validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
  - [~] Phase 4a Audit Export Hardening — Wave C Batch 1 (Target: Q4 2026)
    - [~] High-volume stress tests for audit export reliability (Target: Q4 2026)
      - [x] EXPORT-STRESS-01..EXPORT-STRESS-10: 10 focused stress test cases in `src/security/test_audit_export_stress_focused.cpp` (2026-08-18)
      - [x] Sustained load validation: 1000+ events/sec for 5 seconds (2026-08-18)
      - [x] Atomicity under concurrent exports: 4 threads × 10 exports each (2026-08-18)
      - [x] Idempotency with duplicate detection via bundle IDs (2026-08-18)
      - [x] Crash-recovery checkpoint validation at 10% intervals (2026-08-18)
      - [x] Client disconnect and recovery scenarios (2026-08-18)
      - [x] Memory pressure graceful degradation (2026-08-18)
      - [x] Event loss detection and reliability gates (2026-08-18)
      - [x] Export latency p95/p99 under sustained load (2026-08-18)
      - [x] Recovery time after disconnect: ≤2s gate (2026-08-18)
    - [~] Audit export reliability gates and metrics (Target: Q4 2026)
      - [x] ExportMetrics struct: export_start_ms, export_end_ms, events_sent, events_confirmed, resend_count (include/security/security_evidence_collector.h) (2026-08-18)
      - [x] export_atomicity_guarantee() method: all-or-nothing semantics (2026-08-18)
      - [x] export_idempotency_check() method: deduplication on retry (2026-08-18)
      - [x] lastExportMetrics() method: retrieve metrics from last export (2026-08-18)
      - [x] Crash-recovery mechanism: checkpoint at 10% export intervals (2026-08-18)
    - [~] Export performance benchmark gates (Target: Q4 2026)
      - [x] bench_audit_export_gates.cpp with 5 benchmarks: latency, rate, file throughput, recovery time, atomicity/idempotency checks (2026-08-18)
      - [x] Export rate gate: ≥10,000 events/sec (p99) (2026-08-18)
      - [x] Export latency gate: ≤500ms per 1000-event batch (p99) (2026-08-18)
      - [x] Recovery time gate: ≤2s after disconnect (p99) (2026-08-18)
      - [x] Gate manifest baseline: benchmarks/wave9/audit_export_gate_manifest.json (2026-08-18)

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
  - [x] Vault provider production-config and fail-closed validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
  - [x] HSM provider production-mode/stub-guard validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
  - [x] PKI-oriented fail-closed dependency validation (matrix coverage) (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
- [~] Tighten crypto error-path handling and secure-default enforcement (Target: Q4 2026)
  - [x] Crypto error-path tests K-ERR-01..K-ERR-04 (2026-08-07)
  - [x] Key-provider failover tests K-PROV-01..K-PROV-04 (2026-08-07)
  - [x] Production failure-injection matrix validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)

### Phase 3: Policy and Data-Protection Hardening
- [~] Expand RLS/masking/policy-enforcement regression coverage under mixed query workloads (Target: Q4 2026)
  - [x] RLS tests P-RLS-01..P-RLS-04 (2026-08-07)
  - [x] Query masking tests P-MASK-01..P-MASK-02 (2026-08-07)
  - [x] Policy benchmarks P-MRG-01..P-MRG-05 (2026-08-07)
  - [x] Real query workload testing (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
  - [x] Mixed RLS+ABAC scenarios (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
- [~] Validate deny-by-default and policy-merge semantics under conflicting rule sets (Target: Q4 2026)
  - [x] Policy-merge tests P-MRG-01..P-MRG-04 (2026-08-07)
  - [x] Deny-by-default tests P-DENY-01..P-DENY-04 (2026-08-07)
  - [x] Conflict resolution edge case validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
  - [x] Concurrent policy update atomicity (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)

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
  - [x] Production integration validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
- [~] Phase 3 policy hardening: RLS + policy-merge + deny-by-default + masking tests + benchmarks (P-RLS, P-MRG, P-DENY, P-MASK gates) (Target: Q4 2026)
  - [x] Tests: test_security_phase3_policy_hardening_focused.cpp (2026-08-07)
  - [x] Benchmarks: bench_security_phase3_policy_gates.cpp (2026-08-07)
  - [x] Production integration validation (`tests/security/test_security_wavec_production_validation_focused.cpp`) (2026-08-17)
- [~] Phase 4a audit export hardening: high-volume stress tests + reliability gates + benchmarks (EXPORT-STRESS-01..10, E-ATOMIC, E-IDEM, E-LATENCY gates) (Target: Q4 2026)
  - [x] Tests: test_audit_export_stress_focused.cpp with 10 focused stress cases (2026-08-18)
  - [x] Benchmarks: bench_audit_export_gates.cpp with export rate/latency/recovery gates (2026-08-18)
  - [x] Metrics implementation: ExportMetrics + reliability gate methods (2026-08-18)
  - [x] Gate baseline: benchmarks/wave9/audit_export_gate_manifest.json (2026-08-18)
- [x] Benchmark CMakeLists registered: benchmarks/security/CMakeLists.txt
- Nachweise: security focused tests, auth/policy regressions, crypto/key-provider tests, security benchmarks, audit export stress/gates (Phase 1-4a)
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some high-assurance runtime envelopes still require broader benchmark and regression evidence.
- Certain external dependency failure combinations need additional hardening validation.
- Policy explainability and operator-facing diagnostics continue to be refined.

## Breaking Changes
- Security public APIs in active major lines remain additive-first.
- Any behavior change requiring migration must be versioned and documented in changelog/migration notes.

## Program Execution Model — Wave Context

This module is scoped to **Wave C — Security Production Validation** in the program-level wave model.
Wave C begins only after Wave B exit criteria are met.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full Wave A → B → C → D gate model and exit criteria.

### Wave C Scope for `security`
- [x] Security: complete Vault/HSM/PKI integration validation, provider failover, real RLS/query workloads, concurrent policy updates, and policy-conflict edge cases (`tests/security/test_security_wavec_production_validation_focused.cpp`) (Target: Q4 2026, done: 2026-08-17)

### Wave C Entry Gate (prerequisite from Wave B)
- [ ] Wave B gate is closed: retrieval chain baselines stable, ACM observability gates closed, hardware baselines confirmed (Target: Q4 2026)

### Wave C Exit Criteria (this module's contribution)
- [x] Production-style security integration evidence complete (`tests/security/test_security_wavec_production_validation_focused.cpp`) (Target: Q4 2026, done: 2026-08-17)
- [x] Integrity and reliability verified under sustained load (`tests/security/test_security_wavec_production_validation_focused.cpp`) (Target: Q4 2026, done: 2026-08-17)
- [x] Policy gates consistently block boundary/license/hash/SBOM regressions (`tests/security/test_security_wavec_production_validation_focused.cpp`) (Target: Q4 2026, done: 2026-08-17)

### Dependencies on Later Waves
- Wave D operability hardening depends on stable Wave C security controls.
