# user_storage_encryption Module: Implementation Coordination Plan

**Date:** 2026-08-08  
**Status:** Phase 2-5 Implementation (Parallel Execution Mode)  
**Target Branch:** `develop` (per BRANCHING_STRATEGY.md)

---

## Executive Summary

The user_storage_encryption module is undergoing complete Phase 2-5 implementation using coordinated sub-agents:

| Phase | Component | Agent | Status | ETA |
|-------|-----------|-------|--------|-----|
| **1** | API Contract | ✅ | Complete | — |
| **2-3** | Backend/Scheduler Hardening + Error Handling | themisdb-implementer | 🔄 Running | 5-10m |
| **4-5** | Integration Tests + Benchmarks | general-purpose | 🔄 Running | 10-15m |
| **Gap** | Verification & Severity Assessment | gap-verifier | 🔄 Running | 3-5m |
| **Review** | Code Quality Gate | themisdb-reviewer | ⏳ Pending | After P2-3 |
| **6** | Documentation & Governance | TBD | ⏳ Pending | After all phases |

---

## Phase 2-3: Backend & Error Handling Hardening

### Scope
Complete hardening of the four core user_storage_encrypted components:

1. **GoCryptFS Backend** (gocryptfs_backend.cpp/.hpp)
   - Add command timeout handling (30s for mount/unmount, 60s for key ops)
   - Implement FUSE error recovery (detect unavailable FUSE, stale mounts)
   - Verify stdin pipe buffer cleanup (explicit_bzero, RAII)
   - Enhance error codes for backend failures

2. **Key Rotation Scheduler** (key_rotation_scheduler.cpp/.hpp)
   - Add recovery diagnostics (rotation attempt/failure/success counts)
   - Implement callback failure isolation (bounded retries per level)
   - Verify state persistence (last_check_ms, interval_days)
   - Add structured diagnostic events

3. **Multi-Level Storage** (multi_level_storage.cpp/.hpp)
   - Implement bounded error propagation (level N failure ≠ level N+1 failure)
   - Verify stale mount reconciliation (fusermount -u + fallback to umount)
   - Add per-level error tracking and recovery

4. **Key Derivation Service** (key_derivation_service.cpp/.hpp)
   - Verify Argon2id determinism (m=65536, t=3, p=4)
   - Add input validation (empty salt, invalid dir, passphrase edge cases)
   - Performance budgeting (KDF ≤ 1s per container, p99)

### Deliverables
- ✅ Updated source files (4 components)
- ✅ Full Doxygen documentation
- ✅ Error code mappings
- ✅ Recovery procedures documented
- ✅ All focused unit tests passing (linux-release preset)
- ✅ Build: zero warnings

### Code Quality Standards
Per CLAUDE.md and modern C++ best practices:
- ✅ RAII for all resource management
- ✅ Const-correctness throughout
- ✅ Move semantics where appropriate
- ✅ No raw pointers in public APIs
- ✅ Exception-safe code
- ✅ Explicit error handling (no silent fallbacks)

### Build Verification
```bash
cmake --preset linux-release
cmake --build --preset linux-release --target module_user_storage_encrypted_lib --parallel 16
ctest --preset linux-release -L user_storage_encrypted -V
```

**Expected Result:** All tests PASS, zero compiler warnings

---

## Phase 4-5: Integration Tests & Benchmarks

### Phase 4 Scope: Expanded Test Coverage

**Component 4.1: Docker Compose Vault Fixture**
- Verify hashicorp/vault:1.15 is properly configured
- Health checks for Vault availability
- Test credentials setup (kv-v2 mount at /themis/)

**Component 4.2: End-to-End Integration Tests**
- E2E-01 through E2E-08 (8 lifecycle + security tests)
- File: `test_user_storage_encrypted_e2e_vault_integration_focused.cpp`
- Coverage:
  - Create/Mount/Write/Unmount/Remount lifecycle
  - All 4 security tiers (OFFEN, VERTRAUT, VERTRAULICH, STRENG_GEHEIM)
  - Zero-downtime key rotation under concurrent load
  - FUSE unavailable scenario handling
  - Invalid path & symlink traversal prevention
  - Vault timeout & recovery
  - Stale mount reconciliation
  - Multi-tenant isolation

**Component 4.3: Stress & Failure Injection Tests**
- STRESS-01 through STRESS-06 (6 concurrency/error injection tests)
- File: `test_user_storage_encrypted_stress_focused.cpp`
- Coverage:
  - Concurrent mount/unmount (8 threads × 10 iterations)
  - Key rotation under high load (4R+2W, 5 rotations)
  - Command timeout simulation
  - Vault unavailability simulation
  - Disk full simulation
  - Permission denied simulation

**Component 4.4: CMakeLists.txt Updates**
- Register new focused test targets with `themis_register_module_focused_test()`
- Set TIMEOUT 300 for E2E tests (longer than unit tests)
- Add labels: `user_storage_encrypted`, `integration`, `e2e`, `stress`
- Add SKIP_IF_NO_DOCKER graceful degradation

### Phase 5 Scope: Performance & Release Gates

**Component 5.1: Benchmark Suite Expansion**
- File: `bench_user_storage_encrypted_lifecycle_gates.cpp`
- 6 benchmark families (USK-P5-01 through USK-P5-06):
  - USK-P5-01: Mount latency (p95/p99, threshold ≤500ms)
  - USK-P5-02: Unmount latency (p95/p99, threshold ≤300ms)
  - USK-P5-03: Key derivation latency (p99, threshold ≤1500ms)
  - USK-P5-04: Key rotation latency (p99, threshold ≤10000ms)
  - USK-P5-05: Concurrent mount throughput (threshold ≥0.5/sec/thread)
  - USK-P5-06: Encrypted write throughput (threshold ≥50 MB/s)

**Component 5.2: Release Gate Definition & CTest Integration**
- All benchmarks use steady_clock (per benchmark hygiene rules)
- Use OS temp dir for I/O operations
- Multiple runs (5-10) for latency stats
- Report p50, p95, p99 in addition to mean/median
- CTest label: `release_critical` for gates USK-P5-01 through USK-P5-06

**Component 5.3: Baseline Stabilization**
- Run benchmarks 3-5 times on stable system
- Capture baseline values for all gates
- Document baselines in `benchmarks/user_storage_encrypted/README.md`
- Document performance assumptions (CPU, disk type, gocryptfs version)
- Set regression gates (alert if p99 latency increases >20%)

### Test Execution Workflow
```bash
# Start Docker Compose (Vault)
docker-compose -f docker-compose.user-storage.yml up -d vault vault-init
sleep 10  # Wait for Vault readiness

# Run E2E tests (TIMEOUT 300s)
ctest --preset linux-release -L user_storage_encrypted,e2e -V

# Run stress tests (TIMEOUT 300s)
ctest --preset linux-release -L user_storage_encrypted,stress -V

# Run benchmark gates
ctest --preset linux-release -L user_storage_encrypted,release_critical -V

# Cleanup
docker-compose -f docker-compose.user-storage.yml down
```

**Expected Result:** All tests PASS, benchmarks within budgets

---

## Gap Verification Phase

### Scope
Scan, verify, and classify all gaps in the four core components:
1. GoCryptFS Backend: 3 gaps identified
2. Key Rotation Scheduler: 3 gaps identified
3. Key Derivation Service: 5 gaps identified
4. Multi-Level Storage: 4 gaps identified

### Verification Tasks
1. ✅ Scan for STUB/SIMULATION/MOCK/TODO/FIXME markers
2. ✅ Classify each gap: real vs. false positive
3. ✅ Reassess severity: CRITICAL, HIGH, MEDIUM, LOW
4. ✅ Verify approval status (human-approved vs. unapproved)
5. ✅ Generate GitHub issue templates
6. ✅ Flag gaps blocking Phase 2-3 implementation

### Output
- Structured gap verification report (JSON)
- Prioritized gap list for remediation
- GitHub issue templates
- Unapproved stub/mock removal recommendations

---

## Code Review Phase (themisdb-reviewer)

### Timing
After Phase 2-3 implementation completes (themisdb-implementer)

### Review Criteria

**Modern C++ Best Practices:**
- ✅ RAII for resource management
- ✅ Const-correctness
- ✅ Move semantics where appropriate
- ✅ No raw pointers in public APIs
- ✅ Smart pointer usage (std::unique_ptr, std::shared_ptr)
- ✅ Exception-safe code

**Security:**
- ✅ No secrets in code (API keys, tokens, passwords)
- ✅ Explicit error handling (no silent fallbacks)
- ✅ Input validation for all public entry points
- ✅ Path validation (no symlink traversal)
- ✅ Timeout handling for external commands

**Documentation:**
- ✅ Doxygen comments for all public methods
- ✅ Edge-case documentation
- ✅ Failure mode documentation
- ✅ Security properties documented
- ✅ Error code mappings documented

**Tests:**
- ✅ Minimum 80% coverage for new code paths
- ✅ Focused tests with realistic scenarios
- ✅ All focused tests pass (120s timeout)
- ✅ No test regressions

### Review Workflow
1. ⏳ Wait for Phase 2-3 implementation completion
2. ⏳ Review all four source files
3. ⏳ Check error handling completeness
4. ⏳ Verify documentation
5. ⏳ Run focused tests locally
6. ⏳ Approve or request changes

### Output
- ✅ Detailed review feedback
- ✅ Approval recommendation (merge-ready)
- ✅ List of required remediation items
- ✅ Verified build output (linux-release preset)

---

## Documentation Governance

### Mandatory Updates (per DOCUMENTATION_GOVERNANCE.md)

**L0 (Source Truth):**
- ✅ `src/user_storage_encrypted/ROADMAP.md` — Phase 2-5 completion status
- ✅ `src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md` — Updated backlog

**L1 (Module Docs):**
- ✅ `src/user_storage_encrypted/ARCHITECTURE.md` — Updated implementation details
- ✅ `src/user_storage_encrypted/PERFORMANCE_EXPECTATIONS.md` — Benchmark results
- ✅ Doxygen comments in source files

**L3 (Root Docs):**
- ✅ Root `ROADMAP.md` — User storage encryption status update
- ✅ `RELEASE_STRATEGY.md` — Release gate definitions
- ✅ `CHANGELOG.md` — Phase 2-5 completion entry

### Naming Conventions (per CLAUDE.md & DOCUMENTATION_GOVERNANCE.md)
- Phase 2: "Phase 2 backend/scheduler hardening"
- Phase 3: "Phase 3 error handling & diagnostics"
- Phase 4: "Phase 4 integration & stress tests"
- Phase 5: "Phase 5 release gates"

---

## Build & Test Verification

### CMake Configuration
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
```

**Expected:** Configure succeeds, all dependencies found

### Build Verification
```bash
cmake --build --preset linux-release --target module_user_storage_encrypted_lib --parallel 16
```

**Expected:** Build succeeds, zero warnings

### Test Verification
```bash
# Unit tests (Phase 2-3)
ctest --preset linux-release -L user_storage_encrypted -V

# E2E & Stress tests (Phase 4)
ctest --preset linux-release -L user_storage_encrypted,integration -V

# Benchmark gates (Phase 5)
ctest --preset linux-release -L user_storage_encrypted,release_critical -V
```

**Expected:** All tests PASS, benchmarks within budgets

---

## Final Verification & PR Merge

### Pre-Merge Checklist
- ✅ Phase 2-3 implementation complete
- ✅ All gaps verified and addressed
- ✅ Code review approval from themisdb-reviewer
- ✅ CodeQL security checks passed
- ✅ Build succeeds (linux-release preset)
- ✅ All focused tests pass
- ✅ Documentation updated
- ✅ No breaking changes in public API
- ✅ ROADMAP.md updated

### PR Structure
1. **Draft PR (Phase 2-3 Focus)**
   - Title: "feat(user_storage_encrypted): Phase 2-3 backend hardening & error handling"
   - Target: `develop`
   - Status: Draft until reviewer approval

2. **Final PR (Phase 4-5)**
   - Title: "feat(user_storage_encrypted): Phase 4-5 integration tests & benchmarks"
   - Target: `develop`
   - Status: Ready for merge

### Commit Messages
```
feat(user_storage_encrypted): Phase 2 backend/scheduler hardening
- GoCryptFS timeout handling (30s/60s thresholds)
- FUSE error recovery + stale mount reconciliation
- Key Rotation Scheduler diagnostics + recovery isolation
- Multi-Level Storage bounded error propagation
- Key Derivation Service input validation
- All focused unit tests passing

feat(user_storage_encrypted): Phase 3 error handling & diagnostics
- Fail-safe backend behavior for all failure modes
- Unified diagnostic system (JSON serialization)
- Path validation + symlink traversal prevention
- Explicit error codes for all failure scenarios

test(user_storage_encrypted): Phase 4 integration & stress tests
- E2E lifecycle tests (E2E-01 through E2E-08)
- Stress tests with failure injection (STRESS-01 through STRESS-06)
- Docker Compose Vault fixture
- CMakeLists.txt updated with new focused tests

perf(user_storage_encrypted): Phase 5 release gates
- Benchmark expansion (USK-P5-01 through USK-P5-06)
- Mount/unmount/derivation/rotation latency gates
- p95/p99 envelope stabilization
- CTest release_critical label integration
```

---

## Handoff Workflow Summary

```
┌─────────────────────────────────────────────────────────────┐
│ 1. PHASE 2-3 IMPLEMENTATION (themisdb-implementer)          │
│    ✅ Backend hardening, error handling, diagnostics        │
│    ✅ All focused unit tests passing                        │
│    └─→ NOTIFY: Ready for Code Review                       │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. CODE REVIEW (themisdb-reviewer)                          │
│    ✅ C++ best practices verification                       │
│    ✅ Security checks                                       │
│    ✅ Documentation review                                  │
│    └─→ APPROVE or REQUEST CHANGES                          │
└─────────────────────────────────────────────────────────────┘
                            ↓ (if approved)
┌─────────────────────────────────────────────────────────────┐
│ 3. PHASE 4-5 IMPLEMENTATION (general-purpose)               │
│    ✅ Integration tests + benchmarks                        │
│    ✅ Docker Compose Vault fixture                          │
│    ✅ All E2E/stress tests passing                          │
│    └─→ NOTIFY: Tests & Benchmarks Ready                    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. FINAL VERIFICATION & PR MERGE                            │
│    ✅ Build succeeds (linux-release preset)                │
│    ✅ All tests pass                                        │
│    ✅ Documentation complete                                │
│    ✅ CodeQL checks passed                                  │
│    └─→ MERGE TO develop                                    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 5. DOCUMENTATION & ROADMAP UPDATE                           │
│    ✅ Update root ROADMAP.md (production-ready status)     │
│    ✅ Wave-1 private plugin integration                     │
│    ✅ Release notes preparation                             │
│    └─→ COMPLETE: Phase 2-5 Delivery                        │
└─────────────────────────────────────────────────────────────┘
```

---

## Parallel Execution Benefits

By running agents in parallel:
- **Gap Verification** starts immediately (3-5m)
- **Phase 2-3 Implementation** proceeds independently (5-10m)
- **Phase 4-5 Tests & Benchmarks** executes in parallel (10-15m)
- **Total Timeline:** ~15 minutes (vs. 30+ minutes sequential)
- **Code Review** can begin after Phase 2-3 completes

---

## Next Steps After Phase 5 Completion

1. ✅ Launch code review (themisdb-reviewer)
2. ✅ Final build & test verification
3. ✅ Update root ROADMAP.md (user_storage_encrypted → production-ready)
4. ✅ Prepare release notes for next minor release
5. ✅ Plan Phase 6 (Long-term): Windows Support, Audit Log, Post-Quantum Crypto (Q4 2026+)

---

**Status:** Implementation started 2026-08-08 09:05:29 UTC  
**Target Completion:** 2026-08-08 09:20:00 UTC (parallel execution)  
**PR Merge Target:** `develop` branch  
**Reviewer:** themisdb-reviewer (after Phase 2-3)
