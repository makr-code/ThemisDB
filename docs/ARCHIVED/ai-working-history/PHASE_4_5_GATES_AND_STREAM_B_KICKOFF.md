# Phase 4 Release Gate Checklist & Phase 5 Stream B Kickoff Guide

**Status**: TEMPLATE (Aug 26-30 Phase 4, Sept 1 Phase 5)  
**Owner**: Release Management + Stream B Agents

---

## PHASE 4: Release Gate Validation (Aug 26-30)

### Pre-Merge: GATE-P2-01..10 Checklist

#### Build Gates (GATE-P2-01 through P2-03)

**GATE-P2-01: linux-release Preset Build**
```bash
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16
```
- [ ] Build succeeds (exit code 0)
- [ ] No new compiler warnings
- [ ] Binary size stable (±5% vs baseline)
- [ ] All tensor targets built
- **Evidence**: CI log snippet: `Build completed successfully`

**GATE-P2-02: windows-release Preset Build**
```bash
cmake --preset windows-release
cmake --build --preset windows-release --parallel 16
```
- [ ] Build succeeds (exit code 0)
- [ ] No MSVC warnings
- [ ] .exe sizes stable
- **Evidence**: CI log snippet: `MSBuild succeeded`

**GATE-P2-03: community-release Preset Build**
```bash
cmake --preset community-release --debug-trace
cmake --build --preset community-release --parallel 8
```
- [ ] Build succeeds even without private dependencies
- [ ] No hard-fail on missing ethics_ai or user_storage
- **Evidence**: CI log snippet: `Community preset build OK`

---

#### Unit Test Gate (GATE-P2-04)

**GATE-P2-04: Tensor Unit Tests (72+ tests)**
```bash
ctest --preset linux-release -L "tensor" -V --timeout 300
```

| Test Category | Count | Pass Rate | Timeout |
|---------------|-------|-----------|---------|
| Concurrent (A1) | 40 | 100% | 60s each |
| Diagnostics (A2) | 11 | 100% | 30s each |
| Fingerprint graph | 8 | 100% | 45s each |
| Index manager | 6 | 100% | 30s each |
| GGML routing | 4 | 100% | 90s each |
| Storage bridge | 3 | 100% | 30s each |
| **Total** | **72+** | **100%** | — |

- [ ] All tests PASS (0 failures)
- [ ] All tests complete within timeout
- [ ] No flakes (10 repeated runs: all PASS)
- [ ] No new skips
- **Evidence**: 
  - CTest output: `100% tests passed, 0 tests failed out of 72`
  - Repeated run log (10x): `Passed 10/10 runs`

---

#### Integration Gate (GATE-P2-05)

**GATE-P2-05: Cross-Module Integration Tests (release_critical)**
```bash
ctest --preset linux-release -L "release_critical" -V --timeout 600
```

| Integration | Tests | Status |
|-------------|-------|--------|
| Tensor ↔ Query | TINT-Q-01..05 | ✅ PASS |
| Tensor ↔ Storage | TINT-S-01..04 | ✅ PASS |
| Tensor ↔ LLM | TINT-L-01..04 | ✅ PASS |
| Tensor ↔ Server | TINT-SV-01..08 | ✅ PASS |
| Tensor ↔ Sharding | TINT-SH-01..04 | ✅ PASS |
| **Total** | **25+** | **✅ PASS** |

- [ ] All integration tests PASS (0 failures)
- [ ] No ThreadSanitizer errors
- [ ] No AddressSanitizer errors
- [ ] No intermittent flakes
- **Evidence**: 
  - release_critical CI gate PASS
  - 5 independent test runs: all PASS
  - TSan output: `0 data races detected`

---

#### Stability Gate (GATE-P2-06)

**GATE-P2-06: Wave 6 Retention Tests**
```bash
ctest --preset linux-release -L "wave6" -V --timeout 1200
```

| Wave 6 Block | Tests | Expected |
|--------------|-------|----------|
| RCJ (Critical Journey) | RCJ-01..08 | ✅ PASS |
| SSS (Stress/Soak) | SSS-01..08 | ✅ PASS |
| FIR (Failure Injection) | FIR-01..08 | ✅ PASS |

- [ ] All Wave 6 tests PASS (no regressions)
- [ ] p95 latency stable (±10% vs baseline)
- [ ] Memory growth stable (<5% over 1000 ops)
- [ ] No new CRITICAL failures
- **Evidence**:
  - Wave 6 test summary: `24/24 tests passed`
  - Latency comparison: `p95 baseline 78ms, current 81ms (+3.8%, within tolerance)`

---

#### Performance Gate (GATE-P2-07)

**GATE-P2-07: Benchmark Gates PASS**
```bash
# Tensor-specific gates (A3 baseline)
./build-linux-release/benchmarks/bench_tensor_fingerprint_graph \
  --benchmark_filter="findSimilar_p95_10k" --benchmark_repetitions=100

# Failover gates (Phase 5 baseline)
./build-linux-release/benchmarks/bench_failover_phase2_phase3_gates \
  --benchmark_filter=".*" --benchmark_repetitions=50
```

| Benchmark | Gate | Baseline | Current | Status |
|-----------|------|----------|---------|--------|
| findSimilar p95 (10k) | TA-01 | ≤80ms | — | ⏳ TBD |
| findSimilar p99 (10k) | TA-02 | ≤140ms | — | ⏳ TBD |
| Store overwrite overhead | TA-03 | ≤5% | — | ⏳ TBD |
| Mixed workload throughput | TA-04 | ≥2k ops/s | — | ⏳ TBD |
| Memory per operation | TA-05 | <5% growth | — | ⏳ TBD |
| Failover transition latency | FP23-01 | ≤100µs | — | ⏳ TBD |

- [ ] All TA-01..06 gates PASS
- [ ] All FP23-01..06 gates PASS (if applicable)
- [ ] No unexplained regressions (>10% is FAIL)
- **Evidence**: Benchmark summary report with all gates PASS

---

#### Security Gate (GATE-P2-08)

**GATE-P2-08a: CodeQL Static Analysis**
```bash
codeql database create --language=cpp ./codeql-db
codeql database analyze ./codeql-db cpp-security.ql --format=sarif-latest
```
- [ ] No new CRITICAL findings (0 CRITICAL issues)
- [ ] No new HIGH findings (0 NEW HIGH issues)
- [ ] All pre-existing HIGH issues tracked
- **Evidence**: CodeQL SARIF report (0 new CRITICAL/HIGH)

**GATE-P2-08b: AddressSanitizer (ASan)**
```bash
cmake --preset linux-release -DENABLE_SANITIZERS=asan
ctest --preset linux-release -L "tensor" -V
```
- [ ] No memory leaks detected
- [ ] No heap buffer overflows
- [ ] No use-after-free errors
- **Evidence**: ASan output: `ERROR: LeakSanitizer: detected leaks: 0 bytes`

**GATE-P2-08c: UndefinedBehaviorSanitizer (UBSan)**
```bash
cmake --preset linux-release -DENABLE_SANITIZERS=ubsan
ctest --preset linux-release -L "tensor" -V
```
- [ ] No undefined behavior detected
- [ ] No signed integer overflows
- [ ] No null pointer dereferences
- **Evidence**: UBSan output: `0 undefined behaviors detected`

---

#### Documentation Gate (GATE-P2-09)

**GATE-P2-09a: API Documentation (Doxygen)**
- [ ] All new public C++ functions documented (@brief, @param, @return, @throws)
- [ ] All error codes (TENSOR-9500..9599) documented
- [ ] emitDiagnostic() usage documented in each fix
- [ ] Doxygen build succeeds without warnings
- **Evidence**: Doxygen output: `0 warnings`, API docs coverage ≥95%

**GATE-P2-09b: Diagnostic Mapping**
- [ ] TENSOR error code range fully defined (TENSOR-9500..9599)
- [ ] Each error code linked to specific issue + diagnostic message
- [ ] Silent failure paths (CRITICAL-1) documented as fixed
- [ ] Mitigation guide for MTTR reduction documented
- **Evidence**: A2_REMEDIATION_COMPLETION.md + error code registry

**GATE-P2-09c: Integration Guide**
- [ ] CROSS_MODULE_INTEGRATION_MAP.md complete with all 5 module pairs
- [ ] Dependency order documented (storage → query → LLM → server → sharding)
- [ ] Test matrix and expected behavior documented
- **Evidence**: Integration map published in docs/integration/

**GATE-P2-09d: A2 Audit Resolution Verified**
- [ ] All 11 findings marked FIXED in DIAGNOSTIC_AUDIT.md
- [ ] Before/after code snippets show diagnostic emission
- [ ] Test P2-A2-01..11 pass and validate fixes
- **Evidence**: A2_REMEDIATION_COMPLETION.md with evidence links

**GATE-P2-09e: Rollback Plan**
- [ ] INTEGRATION_ROLLBACK_PLAYBOOK.md reviewed and signed off
- [ ] Rollback decision tree clear (Level 1-4)
- [ ] Rollback drill performed (simulated Level 1-2)
- **Evidence**: Rollback checklist signed by release manager

---

#### Code Review Gate (GATE-P2-10)

**GATE-P2-10: Maintainer Sign-off**
- [ ] ≥2 maintainers reviewed PR (list names)
- [ ] All code review comments addressed
- [ ] No blocking concerns (green checkmarks)
- [ ] Architecture alignment verified
- **Evidence**: 
  - PR approval from [@maintainer1, @maintainer2]
  - Code review comments: all resolved ✅

---

### Phase 4 Summary Document

Create `ai_working/PHASE_4_GATE_EVIDENCE.md`:

```markdown
# Phase 4: Release Gate Evidence Summary

**Date**: Aug 26-30, 2026  
**PR**: feature/tensor-q3-hardening → develop  
**Merge Target**: Aug 31, 2026

## Gate Status

| Gate | Status | Evidence Link |
|------|--------|---------------|
| P2-01 | ✅ PASS | CI#12345 (linux-release build) |
| P2-02 | ✅ PASS | CI#12346 (windows-release build) |
| P2-03 | ✅ PASS | CI#12347 (community-release build) |
| P2-04 | ✅ PASS | CTest run 2026-08-29 (72/72 tests) |
| P2-05 | ✅ PASS | release_critical gate (25/25 tests) |
| P2-06 | ✅ PASS | Wave 6 retention (24/24 tests) |
| P2-07 | ✅ PASS | Benchmark gates (12/12 gates PASS) |
| P2-08 | ✅ PASS | CodeQL/ASan/UBSan (0 new findings) |
| P2-09 | ✅ PASS | Doxygen/Docs/Rollback plan reviewed |
| P2-10 | ✅ PASS | Approved by @maintainer1, @maintainer2 |

**Overall**: ✅ ALL GATES PASS → Ready for Aug 31 merge

---

## Merge Authorization

This PR is approved for merge to develop on or after Aug 31, 2026.

**Signed By**:
- [ ] @maintainer1 (Architecture Review)
- [ ] @maintainer2 (Code Quality Review)
- [ ] @release-manager (Release Gate Validation)

**Date Authorized**: Aug 30, 2026  
**Merge Window**: Aug 31, 2026 08:00-16:00 UTC
```

---

## PHASE 5: Stream B Kickoff (Sept 1, 2026)

### Sept 1 Launch Activities (30-60 min)

**9:00 AM**: Merge Execution
- [ ] Final green light from release manager
- [ ] Merge feature/tensor-q3-hardening → develop
- [ ] Tag commit: v2.4.0-p2-hardening
- [ ] Notify #stream-b-launch channel

**9:15 AM**: Stream B Agent Briefing

**Agent B1: Edge Scenario Hardening (tensor-b1-edges)**
```markdown
# Block B1: Deterministic Edge Scenario Handling

**Duration**: Sept 1-21 (3 weeks)  
**Owner**: themisdb-implementer (tensor-b1-edges)

## Deliverables
- [ ] Edge case inventory (10-15 critical scenarios)
- [ ] tensor_edge_case_handler.h/cpp (fail-safe transitions)
- [ ] test_tensor_bridge_edge_cases_focused.cpp (TEDGE-01..30)

## Critical Edge Scenarios to Handle
1. Invalid adapter references (graph has stale refs)
2. Out-of-bounds index accesses (query beyond fingerprint graph)
3. Stale fingerprints in graph (concurrent update race)
4. Bridge routing failures under load (LLM adapter overload)
5. Graph export/replay edge cases (partial state export)
6. Memory pressure scenarios (OOM handling)
7. Concurrent adapter additions (race condition)
8. Fingerprint collision handling (hash conflict)
9. RocksDB partial write recovery (crash mid-flush)
10. GGML graph exhaustion (max contexts exceeded)

## Success Criteria
- All 10-15 scenarios handled deterministically
- No undefined behavior (ASan/UBSan passing)
- Error codes match documented contract
- Recovery paths validated
- Ready for Stream C integration

## Estimated Effort: 2-3 weeks
```

**Agent B2: Stress Coverage (tensor-b2-stress)**
```markdown
# Block B2: Stress Coverage for Concurrent Graph Patterns

**Duration**: Sept 1-21 (3 weeks)  
**Owner**: general-purpose (tensor-b2-stress)

## Deliverables
- [ ] Workload mixer design (90% query, 10% store/remove)
- [ ] test_tensor_stress_suite_focused.cpp (TSTRESS-01..16)
- [ ] STRESS_COVERAGE.md (documented scenarios)

## Stress Test Scenarios
1. 10k operation sequences (sustained throughput)
2. 100k operation sequences (scalability)
3. Adaptive query/store ratios (90/10, 80/20, 50/50)
4. Chaos injection (random failures, delays)
5. Resource exhaustion (memory pressure)
6. Concurrent adapter contention (all 8 adapters busy)
7. Cross-shard replication stress (TINT-SH level)
8. 48+ hour sustained run (stability)

## Performance Targets
- Throughput: ≥2,000 ops/sec
- Memory: <5% unbounded growth
- P99 latency: stable (no cliff)
- Failure rate: <0.1%

## Success Criteria
- TSTRESS-01..16 all PASS
- 48h+ sustained run completes
- No memory leaks (ASan clean)
- No race conditions (TSan clean)
- Ready for production workload validation

## Estimated Effort: 2-3 weeks
```

**Agent B3: Performance Validation (general-purpose)**
```markdown
# Block B3: Performance Validation & Documentation

**Duration**: Sept 1-Oct 31 (ongoing, ~2-3 weeks active)  
**Owner**: general-purpose (tensor-b3-performance)

## Deliverables
- [ ] P95/P99 latency baselines (locked)
- [ ] Throughput validation under concurrent load
- [ ] STREAM_B_DELIVERY_SUMMARY.md
- [ ] Stream B Release Checklist

## Validation Scenarios
1. Single-user latency (p50, p95, p99)
2. Concurrent users (8, 16, 32, 64 parallel queries)
3. Mixed workloads (query-heavy vs store-heavy)
4. Long-running stability (72h+)
5. Peak load testing (2x expected capacity)

## Success Criteria
- P95 latency: ≤100ms (vs 80ms baseline, acceptable +20%)
- P99 latency: ≤160ms (vs 140ms baseline)
- Throughput: ≥2,000 ops/sec maintained
- No tail latency cliff (p99 vs p95 <2x ratio)
- Ready for production release (v2.4.0 final)

## Estimated Effort: 2-3 weeks active
```

**10:00 AM**: Roadmap Update
```bash
# Update ROADMAP.md
git checkout develop
# Edit ROADMAP.md:
# - Mark "Phase 2: A2 Remediation" as COMPLETE (✅)
# - Mark "Phase 3: Integration Testing" as COMPLETE (✅)
# - Mark "Phase 4: PR Preparation" as COMPLETE (✅)
# - Mark "Phase 5: Merge & Stream B Launch" as COMPLETE (✅)
# - Mark "Phase 6: Stream B Q4 Hardening" as IN PROGRESS (~)
# - Update Timeline visualization (Stream A → Stream B)

git add ROADMAP.md
git commit -m "docs(roadmap): Update Phase 2-5 closure, Stream B Q4 active"
git push origin develop
```

**10:30 AM**: Documentation Sync
- [ ] STREAM_A_COMPLETION_SUMMARY.md published
- [ ] STREAM_B_Q4_2026_PLANNING.md finalized (with B1/B2/B3 detail)
- [ ] TENSOR_IMPLEMENTATION_MASTER_PLAN.md updated

**11:00 AM**: Stream B Standup
- [ ] B1 agent ready with edge scenario inventory
- [ ] B2 agent ready with stress test design
- [ ] B3 agent ready with performance measurement plan
- [ ] Dependencies clear (B1/B2 parallel, B3 ongoing)
- [ ] Sept 8 status check scheduled

---

### Stream B Success Criteria (Oct 31 Completion)

**Block B1 Success (Edge Scenarios)**
- ✅ All 10-15 edge scenarios identified and tested
- ✅ Deterministic error codes and recovery paths
- ✅ Test coverage: TEDGE-01..30 (100% pass)
- ✅ No undefined behavior (ASan/UBSan clean)

**Block B2 Success (Stress Coverage)**
- ✅ Workload mixer operational (90/10 ratio, adaptive)
- ✅ Test coverage: TSTRESS-01..16 (100% pass)
- ✅ 48h+ sustained run completes
- ✅ Throughput ≥2,000 ops/sec, p99 stable

**Block B3 Success (Performance Validation)**
- ✅ P95/P99 baselines locked and documented
- ✅ No tail latency cliff detected
- ✅ Production readiness validated
- ✅ v2.4.0 final release gate PASS

**Stream B Merge (Oct 31, 2026)**
- [ ] feature/tensor-q4-determinism → develop
- [ ] ROADMAP.md Phase 6 marked COMPLETE
- [ ] Q4 2026 Tensor hardening CLOSED
- [ ] v2.4.0 GA release approved

---

## Stream B Launch Checklist

| Item | Owner | Status | ETA |
|------|-------|--------|-----|
| Phase 2-5 merge complete | Release Mgr | ✅ DONE (Aug 31) | — |
| ROADMAP.md updated | Coordinator | — | Sept 1, 10:30 AM |
| B1 agent briefed + running | Coordinator | — | Sept 1, 9:30 AM |
| B2 agent briefed + running | Coordinator | — | Sept 1, 9:30 AM |
| B3 agent briefed + running | Coordinator | — | Sept 1, 9:30 AM |
| Edge scenario inventory created | B1 agent | — | Sept 8 |
| Stress test suite created | B2 agent | — | Sept 8 |
| Performance baseline locked | B3 agent | — | Sept 15 |
| B1/B2/B3 integration complete | All agents | — | Oct 1 |
| Stream B delivery PR ready | B1+B2+B3 | — | Oct 30 |
| Stream B merge to develop | Release Mgr | — | Oct 31 |

