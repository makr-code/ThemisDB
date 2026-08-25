# User Storage Encryption Module - Phases 2-6 Implementation Roadmap

**Date:** 2026-08-08  
**Status:** Planning (Phases 2-6)  
**Coordinator:** Main agent  
**Phase 1 Status:** Delegated to themisdb-implementer (in progress)

---

## Overview

This document coordinates Phases 2-6 of the 6-phase user_storage_encryption module hardening plan. Phase 1 (Gap Remediation) is being executed in parallel by themisdb-implementer agent.

**Total Duration:** 17-23 weeks (4-6 months) estimated  
**Target Completion:** 2026-10-31  

---

## Phase 2: Integration Testing & Vault Hardening (2-3 weeks)

**Start Date:** Post-Phase 1 (estimated 2026-08-22)  
**Target:** 2026-09-05  

### Objectives

- Validate all 4 security tiers (gocryptfs, Vault, HSM, streng-geheim) with production-like infrastructure
- Test key rotation without service downtime
- Validate Vault token lifecycle management
- Harden gocryptfs mount lifecycle

### Key Deliverables

1. **Docker Compose Infrastructure**
   - `docker-compose.user_storage.yml` with Vault dev server + ThemisDB integration
   - 3 security tier configurations (gocryptfs, Vault HSM, streng-geheim with M-of-N)
   - SoftHSM2 for PKCS#11 testing (avoid hardware dependency in CI)

2. **Integration Test Suite** (50-100 test cases)
   - File: `tests/user_storage_encrypted/test_user_storage_encrypted_integration_focused.cpp`
   - Test matrix: encrypt → store → retrieve → decrypt per security tier
   - Concurrent access patterns: 10-100 concurrent clients
   - Key rotation under active load (readers/writers don't stop during rotation)
   - Zero-downtime verification: no data loss, no client errors during rotation

3. **Vault Token Management**
   - Implement token renewal at 75% TTL (TTL ≤ 1 hour)
   - Test token expiration handling (no silent-fail)
   - Add observability hooks: log token renewal events with correlation IDs
   - Document Vault unavailability behavior (read-only vs. full lockout decision)

4. **Gocryptfs Mount Lifecycle Hardening**
   - Startup reconciliation: detect & unmount stale mounts
   - Mount-state detection via `/proc/mounts` (Linux) + platform abstraction
   - Graceful fallback for stale/orphaned mounts
   - Unmount failure recovery (retry with escalating force)

5. **Key Rotation Reliability**
   - Verify rotation completeness under concurrent load (no dropped rotations)
   - IRotationStore crash recovery (resume interrupted rotations)
   - Rotation audit log entries created atomically with key updates
   - Performance target: ≤ 10s per tenant key rotation (online, no downtime)

### Documentation

- `docs/user_storage_encrypted/VAULT_SETUP_GUIDE.md` - Vault configuration for all tiers
- `docs/user_storage_encrypted/KEY_ROTATION_RUNBOOK.md` - Operational procedures
- Updated `docs/user_storage_encrypted/TROUBLESHOOTING.md` - Common issues & recovery

### Acceptance Criteria

- [ ] Docker Compose integration passes locally
- [ ] 50+ integration tests execute in CI/CD, all pass
- [ ] Key rotation under 100 concurrent clients: zero failures, ≤ 10s duration
- [ ] Vault token renewal evidence logged
- [ ] Stale mount reconciliation tested and working
- [ ] Performance benchmarks (USEG-2: p99 mount lifecycle latency) baseline established

---

## Phase 3: Windows/macOS Support & Audit Logging (3-4 weeks)

**Start Date:** Post-Phase 2 (estimated 2026-09-05)  
**Target:** 2026-09-26  

### Objectives

- Expand platform support beyond Linux
- Implement compliance-required audit trails (GDPR, regulatory)
- Enable forensics & security event tracking

### Key Deliverables

1. **Abstract FilesystemBackend Interface**
   - Base class: `include/user_storage_encrypted/filesystem_backend.hpp`
   - Linux impl: gocryptfs backend (existing, wrapped in abstraction)
   - Windows impl: VeraCrypt CLI wrapper OR native NTFS EFS (evaluation needed)
   - macOS impl: gocryptfs on macFUSE (third-party FUSE layer)
   - Factory: `createFilesystemBackend()` platform-specific selection

2. **Windows Support Implementation** (2 weeks)
   - Evaluate VeraCrypt (cli + library) vs. native NTFS EFS
   - Windows CI preset: `windows-release` in CMakePresets.json
   - Test execution in GitHub Actions (Windows runner)
   - Path handling: Windows drive letters, UNC paths, file permissions

3. **macOS Support Implementation** (1 week)
   - Validate gocryptfs on macFUSE compatibility
   - Platform-specific unmount: `fusermount -u` (Linux) vs. `umount` (macOS)
   - macOS CI preset: `macos-release` (GitHub Actions macOS runner)
   - File permissions: POSIX modes on APFS

4. **Audit Logging System** (2-3 weeks)
   - Interface: `include/user_storage_encrypted/audit_logger.hpp`
   - Implementation: append-only timeline storage in ThemisDB
   - Collection: `user_storage_encrypted_audit` with schema:
     ```
     {
       timestamp: DateTime,
       user: String,
       operation: Enum(TIER_TRANSITION, KEY_ROTATION, ACCESS, ENCRYPT, DECRYPT, DELETE),
       security_level: Enum(PUBLIC, CONFIDENTIAL, SECRET, STRENG_GEHEIM),
       status: Enum(SUCCESS, FAILURE, TIMEOUT),
       reason: String,
       details: Json,
       hash_chain: String (previous_event_hash)
     }
     ```
   - Tamper detection: hash chain linking (BLAKE3)
   - Query API: fetch by time range, user, operation type
   - Export formats: JSON, CSV (for compliance auditors)

5. **GDPR Erasure Compliance**
   - Cryptographic deletion: delete tenant's Data Encryption Key (DEK) → ciphertext irrecoverable
   - Compliance test: verify unreadable within ≤ 30s
   - Audit trail: record GDPR_ERASURE events
   - Retention policy: configurable (default: 1 year)

### Documentation

- `docs/user_storage_encrypted/PLATFORM_SUPPORT_MATRIX.md` - Linux/macOS/Windows status
- `docs/user_storage_encrypted/AUDIT_LOGGING_GUIDE.md` - Query API, event types, retention
- `docs/user_storage_encrypted/GDPR_COMPLIANCE.md` - Right-to-erasure procedures
- CMakeLists.txt: platform conditionals for backends

### Acceptance Criteria

- [ ] Abstract FilesystemBackend interface in place
- [ ] Linux gocryptfs backend refactored to use interface
- [ ] Windows preset configured; build succeeds; tests pass (if VeraCrypt available)
- [ ] macOS preset configured; build succeeds; tests pass
- [ ] IAuditLogger implementation complete with 50+ test cases
- [ ] GDPR erasure tested & verified (≤ 30s cryptographic deletion)
- [ ] Audit log export (JSON/CSV) working
- [ ] Performance gate: audit append ≤ 10ms p99

---

## Phase 4: Post-Quantum Crypto & Advanced Access Control (4-6 weeks)

**Start Date:** Post-Phase 3 (estimated 2026-09-26)  
**Target:** 2026-11-07  

### Objectives

- Prepare for quantum-resistant cryptography
- Implement fine-grained authorization (ABAC)
- Add hardware security key support

### Key Deliverables

1. **Post-Quantum Cryptography Tier** (2 weeks)
   - NIST PQC standard ML-KEM-1024 (key encapsulation mechanism)
   - Hybrid encryption: AES-256-GCM (classical) + ML-KEM (PQ)
   - Integration: liboqs library (verified NIST reference)
   - New tier: streng-geheim-pqc (or upgrade streng-geheim)
   - Performance targets:
     - Key generation ≤ 100ms
     - Encapsulation (key encrypt) ≤ 50ms
     - Decapsulation (key decrypt) ≤ 50ms
   - Test: PQC test vectors, interoperability, hybrid correctness

2. **Threshold Cryptography (Shamir Secret Sharing)** (2 weeks)
   - M-of-N key reconstruction for streng-geheim tier
   - Escrow mechanism: split DEK into N shards, require M to reconstruct
   - Vault integration: store shards as separate secrets, each with key-holder role
   - Recovery flow:
     1. Operator requests DEK reconstruction
     2. Collect M confirmations from authorized key holders (multi-factor auth)
     3. Reconstruct DEK in memory
     4. Audit log all shard access attempts
   - Implementation: libsodium secretbox or custom Shamir library
   - Test: M-of-N correctness, shard recombination, security properties

3. **Attribute-Based Encryption (ABE) & ABAC Policy Engine** (1-2 weeks)
   - Open Policy Agent (OPA) integration
   - ABAC policy model: evaluate (principal, tier, operation, context) → allow/deny
   - Attributes: user role, department, security clearance, time-of-day, geolocation
   - Declarative policies in Rego language (OPA DSL)
   - Break-glass access: emergency override with mandatory audit alert + 24h justification
   - Performance: policy evaluation ≤ 1ms per decision
   - Test: policy engine correctness, break-glass auditing

4. **FIDO2 / WebAuthn Hardware Security Key Support** (1 week)
   - Hardware security key integration for streng-geheim authentication
   - Bind FIDO2 public key to HSM-backed private key
   - 2FA flow: FIDO2 challenge-response + Vault token grant
   - CI testing: SoftHSM + mock FIDO2 device (no physical key required)
   - Test: registration, authentication, key binding

### Documentation

- `docs/user_storage_encrypted/POST_QUANTUM_CRYPTO.md` - ML-KEM design, hybrid scheme
- `docs/user_storage_encrypted/THRESHOLD_CRYPTO.md` - M-of-N reconstruction, shard management
- `docs/user_storage_encrypted/ABAC_POLICIES.md` - OPA policy examples, evaluation order
- `docs/user_storage_encrypted/FIDO2_INTEGRATION.md` - Registration, authentication flow

### Acceptance Criteria

- [ ] ML-KEM integration complete; test vectors pass
- [ ] M-of-N threshold reconstruction working; shard recombination tested
- [ ] OPA policy engine integrated; ABAC decision log active
- [ ] FIDO2 flow tested (SoftHSM + mock device)
- [ ] Performance benchmarks: PQC ops (≤100/50/50ms), policy eval (≤1ms), FIDO2 (<500ms)
- [ ] 30+ test cases for Phase 4 features
- [ ] Updated ROADMAP.md with Phase 4 closure & PQC design rationale

---

## Phase 5: Performance Optimization & Formal Verification (3-4 weeks)

**Start Date:** Post-Phase 4 (estimated 2026-11-07)  
**Target:** 2026-12-05  

### Objectives

- Achieve production SLA targets
- Obtain formal security audit and compliance evidence
- Establish operational readiness

### Key Deliverables

1. **Performance Benchmarking & Optimization** (1.5 weeks)
   - AES-256-GCM throughput: ≥ 1 GB/s with AES-NI (single-core)
   - HSM PKCS#11 latency: ≤ 5ms per sign/verify (SoftHSM), ≤ 1ms hardware HSM
   - Vault secret fetch: ≤ 10ms p99 (with local agent cache)
   - ABAC policy evaluation: ≤ 1ms per decision
   - Key rotation: ≤ 10s per tenant (online, no downtime)
   - Mount lifecycle: p99 latency < 2s
   - Benchmark file: `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_phase5_sla_gates.cpp`
   - Release-profile gating: USEG gates enforced

2. **Formal Security Audit & Penetration Test** (2 weeks)
   - Engage independent C2-level security firm
   - Scope: cryptographic operations, HSM integration, key derivation, Vault integration, audit log integrity
   - Threat model: insider, supply-chain, side-channel, fault-injection
   - Report: high/critical findings with remediation plan
   - Remediation: all findings fixed before GA (blocking)

3. **Compliance Validation** (1 week)
   - BSI IT-Grundschutz compliance mapping (German encryption tier standards)
   - ISO 27001 control coverage (access control, encryption, audit logging, key management)
   - SOC 2 Type II evidence: segregation of duties, audit trail integrity, change management
   - Automated compliance report generation

4. **Operational Readiness** (1 week)
   - Runbooks: Vault recovery, HSM failover, key escrow, emergency access procedures
   - Monitoring dashboards: key rotation health, Vault token renewal, audit log volume
   - Alert thresholds: rotation lag > 1h, Vault unavailability > 5min, audit lag > 100ms
   - Runbook testing: simulate failures, validate recovery procedures

### Documentation

- `docs/user_storage_encrypted/PERFORMANCE_SLA_REPORT.md` - Benchmark results, SLA compliance
- `security/pentest/USER_STORAGE_ENCRYPTED_AUDIT_REPORT.md` - Security audit findings & remediation
- `docs/compliance/USER_STORAGE_ENCRYPTED_BSI_ISO_SOC2_EVIDENCE.md` - Compliance bundle
- `docs/operations/USER_STORAGE_ENCRYPTED_RUNBOOKS.md` - Procedures (Vault recovery, HSM failover, key escrow)
- `docs/operations/USER_STORAGE_ENCRYPTED_MONITORING.md` - Dashboard setup, alert thresholds

### Acceptance Criteria

- [ ] Performance benchmarks meet all SLA targets
- [ ] Security audit report delivered; all findings remediated
- [ ] Compliance evidence bundle complete (BSI/ISO/SOC2)
- [ ] Operational runbooks validated by ops team
- [ ] Monitoring dashboards active and alerting
- [ ] Updated ROADMAP.md with Phase 5 closure (production-ready gate)

---

## Phase 6: Documentation & Long-Term Support (2-3 weeks)

**Start Date:** Post-Phase 5 (estimated 2026-12-05)  
**Target:** 2026-12-19  

### Objectives

- Finalize comprehensive API and operator documentation
- Establish maintenance practices and release strategy

### Key Deliverables

1. **API Documentation (Doxygen)** (1 week)
   - Complete API reference for all 13 public header files
   - Usage examples:
     - Gocryptfs initialization
     - Key rotation workflow
     - Tier transitions (public → confidential → secret → streng-geheim)
     - GDPR erasure workflow
   - Security guidance: PKCS#11 assumptions, HSM failover, Vault token management
   - Performance tuning: AES-NI detection, buffer pool sizing, rotation intervals
   - Generated HTML via Doxygen (doxygen Doxyfile)

2. **Architecture & Design Documentation** (0.5 weeks)
   - Layer diagram: backend, key management, orchestration planes
   - Sequence diagrams: mount lifecycle, key rotation, tier transition, GDPR erasure, M-of-N threshold
   - Failure modes & recovery: stale mount reconciliation, Vault unavailability, HSM errors, network partition
   - Design rationale: why gocryptfs vs. alternatives, why Vault, why M-of-N threshold, why PQC

3. **Operator Guide** (1 week)
   - Deployment: Docker Compose, Kubernetes Helm chart, bare-metal installation
   - Configuration reference: all environment variables, config file format, security levels
   - Vault setup: policy templates, secret engine config, token lifetime settings, HSM config
   - Troubleshooting: common errors, logs, debugging commands, runbooks
   - Upgrade procedures: version compatibility, schema migrations, data backup/restore

4. **Maintenance Plan & Release Strategy** (0.5 weeks)
   - Release cadence: quarterly security updates, annual major release
   - Deprecation policy: ≥ 2 major releases notice before removal
   - Backport scope: security fixes to current + N-1 releases
   - Community contribution guidelines: reporting vulnerabilities (responsible disclosure), feature requests
   - Support SLA: response time, resolution time for critical issues

### Documentation

- `docs/user_storage_encrypted/API_REFERENCE.md` (generated from Doxygen)
- `docs/user_storage_encrypted/ARCHITECTURE_DIAGRAMS.md` - Layer, sequence, error recovery diagrams
- `docs/user_storage_encrypted/OPERATOR_PLAYBOOK.md` - 20+ pages
- `docs/user_storage_encrypted/MAINTENANCE_POLICY.md` - Release cadence, deprecation, backports

### Acceptance Criteria

- [ ] Complete Doxygen API reference generated and reviewed
- [ ] Architecture diagrams completed (layer, sequence, error recovery)
- [ ] Operator playbook includes all deployment scenarios and troubleshooting
- [ ] Maintenance & support SLA documented and approved
- [ ] GitHub release notes template created
- [ ] CI automation for documentation validation
- [ ] Updated ROADMAP.md with Phase 6 closure (v1.0.0 stable release)

---

## Cross-Phase Quality Gates

### Per-Phase Code Quality Standards

| Gate | Requirement | Verification |
|------|-------------|--------------|
| Line Coverage | 90%+ in user_storage_encrypted module | `gcov` report |
| Security Alerts | Zero CodeQL/Clang-Tidy security category | Tool scan output |
| RAII Compliance | All resources managed via RAII (no explicit delete) | Code review |
| Exception Safety | Strong/basic guarantee documented per function | Doxygen @exception tags |
| Move Semantics | Appropriate use of rvalue refs & unique_ptr | Code review |
| Const Correctness | Function signatures mark const where applicable | Code review |

### Per-Phase Testing Standards

| Gate | Requirement | Verification |
|------|-------------|--------------|
| CI Integration | Phase tests run in GitHub Actions | Workflow logs |
| Fixture Realism | Integration tests use Docker/SoftHSM/local Vault | Test code review |
| Benchmark Registration | Performance tests registered with release-profile gating | CMakeLists.txt review |
| Flakiness | Test flakiness < 1% (retry failures tracked) | CI metrics |

### Per-Phase Documentation Standards

| Gate | Requirement | Verification |
|------|-------------|--------------|
| Roadmap Closure | ROADMAP.md Phase section marked complete with date | Manual review |
| Requirements Update | PRODUCTION_REQUIREMENTS.md updated with phase work | File diff |
| API Docs | Doxygen comments for all public changes | Header file review |
| Architecture | Design decisions recorded in ARCHITECTURE.md or design docs | File update |

### Per-Phase Security Standards

| Gate | Requirement | Verification |
|------|-------------|--------------|
| Threat Model | Threat model reviewed by security team (Phase 1) | Review sign-off |
| Audit Findings | Formal audit findings remediated (Phase 5) | Security report |
| Crypto Vetting | All crypto operations use vetted libraries | Dependency audit |
| Secret Handling | Secrets never logged or exposed in exceptions | Code review + static analysis |

---

## Master Success Criteria (2026-10-31 Target)

| Criterion | Target | Verification |
|-----------|--------|--------------|
| **Code Quality** | All 49 Critical/High gaps resolved; 90%+ coverage; zero security alerts | Gap tracker, gcov, CodeQL report |
| **Testing** | All 4 tiers validated; zero-downtime rotation verified; 100+ concurrent clients stress tested | Integration test results |
| **Compliance** | Formal security audit complete; BSI/ISO/SOC2 evidence bundle ready | Audit report + evidence docs |
| **Performance** | Encryption ≥ 1 GB/s; rotation ≤ 10s/tenant; policy eval ≤ 1ms | Benchmark report |
| **Operations** | Runbooks validated; monitoring dashboards active; SLA targets met | Ops team sign-off |
| **Documentation** | API reference, architecture diagrams, operator playbook, maintenance plan | Doc audit checklist |
| **Release** | v1.0.0 stable candidate; all acceptance gates PASS; production deployments approved | Release checklist |

---

## Risk Register & Mitigation

| Risk | Impact | Likelihood | Mitigation | Owner |
|------|--------|-----------|-----------|-------|
| Vault unavailability blocks all encryption operations | High | Medium | Implement graceful degradation; read-only mode during downtime (Phase 2) | Phase 2 Lead |
| gocryptfs mount exhaustion on long-running services | Medium | Medium | Automated stale mount reconciliation (Phase 2); monitoring dashboard (Phase 5) | Phase 2 Lead |
| Crypto library vulnerability (liboqs, libargon2) | High | Low | Pin dependency versions; monitor CVE feeds; automated security updates (Phase 6) | Phase 6 Lead |
| Performance regression from PQC tier | Medium | Medium | Benchmark early (Phase 4); consider async key gen; profile with perf tools | Phase 4 Lead |
| Compliance audit findings delay GA | Medium | Medium | Begin audit prep Phase 3; engage auditor early; treat findings as Phase 5 work | Phase 5 Lead |
| Windows NTFS/VeraCrypt parity issues | Low | High | Windows CI integration Phase 3; SoC requirements review with ops | Phase 3 Lead |
| M-of-N threshold complexity increases deployment burden | Low | Medium | Provide Ansible playbooks + Helm charts (Phase 3/6); clear docs | Phase 3/6 Lead |
| GDPR erasure cryptographic proof difficult to test | Low | Low | Use deterministic test vectors; measure ciphertext unreadability (Phase 3) | Phase 3 Lead |

---

## Phase Coordination & Dependencies

```
Phase 1 (Gap Remediation)  ←── Foundation for all phases
├─→ Phase 2 (Integration & Vault)  ←── Depends on Phase 1 code stability
│   └─→ Phase 3 (Platform & Audit)  ←── Depends on Phase 2 integration tests
│       └─→ Phase 4 (PQC & ABAC)  ←── Depends on Phase 3 audit infrastructure
│           └─→ Phase 5 (Perf & Audit)  ←── Depends on Phase 4 feature completeness
│               └─→ Phase 6 (Documentation)  ←── Depends on Phase 5 production readiness
└─→ Phase 5 also depends on Phase 1 + 2 for formal audit baseline
```

**Critical Path:** Phase 1 → 2 → 3 → 4 → 5 → 6 (sequential, no parallelization due to dependencies)

---

## Communication & Escalation

- **Phase Leads:** Each phase has a designated lead (TBD)
- **Weekly Sync:** Mondays 10:00 UTC (coordinator + phase leads)
- **Issues:** GitHub issues labeled `user_storage_encrypted_phase_N` (N=1-6)
- **Blockers:** Escalate to security + architecture team if impact > 1 week delay
- **PRs:** All phase work merged via separate PRs with review checklist per phase quality gates

---

## Success Celebration Criteria 🎉

When Phase 6 completes successfully (2026-12-19 target):

✅ user_storage_encryption v1.0.0 stable release available  
✅ Production deployments approved by security + ops teams  
✅ All 4 security tiers validated and certified  
✅ Formal security audit passed  
✅ BSI/ISO/SOC2 compliance evidence complete  
✅ 90%+ code coverage + zero critical issues  
✅ SLA targets met (≥1 GB/s encryption, ≤10s key rotation)  
✅ Complete operator documentation & runbooks published  
✅ Open-source release (or private release with appropriate gating)  

---

**Status:** Phases 2-6 ready for phase leads to assign/prepare  
**Next Step:** Phase 1 completion expected by 2026-08-22; Phase 2 kickoff follows
