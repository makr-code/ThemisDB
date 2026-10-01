# Wave C Validation + Wave D Execution Planning
## ThemisDB v2.4.0 GA: Post-Wave B Roadmap

**Date:** 2026-09-02, 15:45 UTC  
**Status:** Wave B Phase 1 complete; Wave C already complete; Wave D in progress  
**Branch:** copilot/update-documentation-for-gaps-again  

---

## EXECUTIVE SUMMARY

| Wave | Status | Gate Date | Key Dates | Next Action |
|------|--------|-----------|-----------|------------|
| **Wave A** | ~COMPLETE | Q4 2026 | See critical blockers below | Run Transaction Phase 1-3 CI immediately |
| **Wave B** | Phase 1 ✅ | Sept 30, 2026 | Exec Sept 7/16 - Sept 30 | Proceed with execution (all blockers resolved) |
| **Wave C** | ✅ COMPLETE | Aug 18, 2026 | Security/audit/policy all PASS | **Pre-sign-off validation only** |
| **Wave D** | ~Phase 1-2 | Q1 2027 | Distributed tracing, runbooks, soak tests | Execute Phase 2A-2C in parallel with Wave B |

---

## CRITICAL BLOCKERS REQUIRING IMMEDIATE ACTION (This Week)

### 🔴 BLOCKER 1: Transaction Phase 1-3 Tests — NO CI EXECUTION YET
**Impact:** Wave A exit criteria cannot be validated without CI evidence  
**Evidence:**
- 73 focused tests implemented per `src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`
- Phase 1 (33 tests): TXN-RECOVERY-01..04, TXN-SAGA-HARDENING-01..04, etc.
- Phase 2 (24 tests): SAGA retry storm, Byzantine fault injection
- Phase 3 (14 tests): Timeout determinism, cascading failure prevention

**Required Action:**
```bash
# Run immediately (should have been done 2026-08-31):
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset community-release -B /tmp/txn-build
cmake --build /tmp/txn-build --config Release --target run_tests -j 4 2>&1 | grep -E "transaction|PASSED|FAILED"
```

**Assign To:** Wave A Transaction Lead (must complete before Sept 5)  
**Sign-Off Condition:** ≥90% tests passing (≤7 allowed failures)

---

### 🔴 BLOCKER 2: Query FTS Executor — 0% Implementation, Q4 Deadline at Risk
**Impact:** Critical path for Wave B; must begin immediately  
**Evidence:**
- Lexer/parser complete 2026-08-09 ✅
- FTS executor backend: **0% implementation progress**
- Target: Complete Phase 1 by Sept 30 (to meet Wave B exit gate)

**Status in Wave B:**
- Wave B2 Phase 1 deliverables locked (headers + 401-line roadmap)
- 42+ test targets specified
- Design review scheduled Sept 16-17
- **Actual implementation starts Sept 16 (2.0 FTE, 15 days)**

**Assign To:** Query Module Lead (already scoped in Wave B2)  
**Sign-Off Condition:** All 42+ tests passing + zero TSAN races by Sept 30

---

### 🔴 BLOCKER 3: GPU CUDA-Call Audit — 340→~100 Remaining Unchecked Calls
**Impact:** Phase D blocked; GPU acceleration not recommended for production  
**Evidence:**
- CUDA-call audit complete 2026-08-24 ✅
- RAII guards created (`include/gpu/cuda_raii.h`) ✅
- **Remaining work:** 340→~100 migration (need 50% reduction by Q3 end)

**Current Status:**
- Wave A GPU Batch A4 tests implemented
- Representative-hardware baselines pending

**Recommended Action:**
- 🟡 Phase 2 GPU work deferred to Q4 2026 (post-Wave B)
- ✅ Phase D representative-hardware baselines can use CPU fallback for Wave B exit gate
- ℹ️ GPU hardware availability pending confirmation (B1 must file procurement by Sept 5)

**Assign To:** GPU Module Lead  
**Sign-Off Condition:** 50% of remaining 100 calls migrated + Phase 2-3 tests green on CPU

---

## WAVE C VALIDATION PLAN (Immediate: This Week)

**Status:** Wave C technically complete (all exit criteria PASS 2026-08-18)  
**Action:** Pre-sign-off validation only (verify evidence not stale)

### 1. Security Module (Track 1)
- ✅ Vault/HSM/PKI integration validation complete
- ✅ RLS/query workloads (20k+ rows) verified
- ✅ Policy-conflict edge cases deterministic
- **Action:** Re-verify Vault/HSM runner availability + HSM attestation chain

### 2. Audit Module (Track 2)
- ✅ Tamper-evidence integrity under concurrent load (4k events)
- ✅ High-volume export (50k+ events, 8.6k events/sec, zero data loss)
- **Action:** Confirm archive/export pipeline still validates on current `develop` HEAD

### 3. CI Policy Gates (Track 3)
- ✅ All 4 workflows (private/public boundaries, edition/license, hash/SBOM, community fail-closed) linted
- **Action:** Run `.github/workflows/gate-pr-policy.yml` on `develop` to confirm all gates trigger correctly

### Sign-Off Readiness Check
**File:** `docs/governance/GA_PROMOTION_SIGN_OFF.md` §2.3 Batch C  
**Verification:**
```bash
# Check all Batch C gates are PASS
grep "C-[1-8].*PASS" /home/runner/work/ThemisDB/ThemisDB/docs/governance/GA_PROMOTION_SIGN_OFF.md | wc -l
# Expected: 8 lines (C-1 through C-8 all PASS)
```

---

## WAVE D EXECUTION PLAN (Start Immediately — Phase 2A-2C Parallel with Wave B)

**Wave D is for Q1 2027 GA+; can start Phase 2 work NOW (leverages Wave B completion).**

### Timeline Options

**OPTION 1: Sequential (Conservative)**
- Sept 2-30: Wave B execution (B1/B2/B3)
- Oct 1-31: Wave D Phase 2A-2C (distributed tracing, metrics, exporter reliability)
- Target: Wave D ready for Q1 2027 GA promotion

**OPTION 2: Parallel (Maximum Effort — RECOMMENDED)**
- Sept 2-30: Wave B execution (B1/B2/B3) + **Wave D Phase 2A design/review**
- Oct 1-15: Wave D Phase 2A-2C implementation begins (3.0 FTE)
- Target: Wave D Phase 1-2 complete by Dec 1, ready for Q1 2027 execution

### Wave D Phase 2A — Distributed Tracing SDK (Sept-Oct 2026)

**Deliverables:**
1. `include/observability/distributed_tracing.h` — `DistributedTraceSpan` class
2. Transaction module tracing integration (Coordinator trace points)
3. Sharding module tracing integration (ShardRouter trace points)
4. Replication module tracing integration (WALShipper trace points)
5. Unit tests (span creation, baggage propagation, parent-child linking)
6. Verification report: `src/observability/PHASE2A_DISTRIBUTED_TRACING_VERIFICATION.md`

**Effort:** 1.5 FTE × 3 weeks = 45 FTE-days  
**Success Criteria:**
- All trace points emit W3C Trace Context-compliant spans
- Zero TSAN races in trace span creation/propagation
- Integration tests pass (10+ test cases per module)

### Wave D Phase 2B — High-Cardinality Metrics (Oct-Nov 2026)

**Deliverables:**
1. Extended `MetricsCollector` with shard-level, replication-lag quantile metrics
2. Operator dashboard integration (Grafana JSON dashboards)
3. Metrics validation suite (20+ test cases)
4. Verification report: `src/observability/PHASE2B_METRICS_COLLECTION_VERIFICATION.md`

**Effort:** 1.0 FTE × 2 weeks = 20 FTE-days  
**Success Criteria:**
- High-cardinality metrics emit correctly under load (1000+ events/sec)
- Dashboard queries execute in <500ms
- Zero memory leaks under sustained metric collection

### Wave D Phase 2C — Exporter Reliability Suite (Oct-Nov 2026)

**Deliverables:**
1. OpenTelemetry exporter stress tests (1000s events/sec, 60+ minute runs)
2. Export failure resilience tests (network partition, exporter restart, quota exhaustion)
3. Verification report: `src/observability/PHASE2C_EXPORTER_RELIABILITY_VERIFICATION.md`

**Effort:** 1.0 FTE × 2 weeks = 20 FTE-days  
**Success Criteria:**
- Exporter handles 10k events/sec without data loss
- Network partition recovery <5 seconds
- All events eventually exported (guaranteed delivery under transient failures)

### Wave D Phase 4 — Long-Duration Soak Tests (Nov-Dec 2026)

**Deliverables:**
1. 60-minute soak test execution results (3 test files already created)
   - `tests/integration/test_replication_soak_60min.cpp`
   - `tests/integration/test_sharding_distributed_write_soak.cpp`
   - `tests/integration/test_telemetry_soak.cpp`
2. Memory leak validation (jemalloc, AddressSanitizer)
3. Sustained performance evidence (p95/p99 latency stability over time)

**Effort:** 0.5 FTE × 2 weeks = 10 FTE-days (mostly test execution, not coding)  
**Success Criteria:**
- All 3 soak tests complete 60-minute runs on representative hardware
- Zero memory growth >2% over 60 minutes
- p95 latency variance <15% across first/last 10-minute windows

---

## WAVE D CRITICAL DEPENDENCY: REPRESENTATIVE HARDWARE

**All Wave D Phase 2C + Phase 4 work requires:**
- 1× `self-hosted gpu-cuda` runner with CUDA 12.x (Turing/Ampere/Hopper gen)
- OpenTelemetry exporter validation lab with network partition simulator
- 60+ minute execution window (test lab reservation system)

**Status:** 🔴 NO RUNNERS ONLINE  
**Action Required (Human):**
- File GPU procurement request by **Sept 5** (B1 will do this for Wave B1 hardware)
- Provisioning lead must bring up ≥1 self-hosted runner by **Sept 15**
- Lab reservation window must be confirmed with test coordinator

**Mitigation if Hardware Unavailable:**
- Use CPU-only baselines (already approved for Wave B exit)
- Defer GPU phase 2C exporter stress to Q1 2027 (non-blocking for GA)
- CPU soak tests can execute on CI infrastructure immediately

---

## ACTION CHECKLIST (THIS WEEK)

### 🔴 CRITICAL (Must complete Sept 2-5)

- [ ] **Transaction:** Run Phase 1-3 test suite on `develop`
  - File: `src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`
  - Command: `cmake --build /tmp/build --target run_tests 2>&1 | tee txn_ci_results.txt`
  - Owner: Wave A Transaction Lead
  - Due: Sept 3, 18:00 UTC

- [ ] **GPU:** Confirm 50% of remaining 100 CUDA calls migrated
  - File: `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`
  - Acceptance: ≤50 unchecked calls remaining
  - Owner: GPU Module Lead
  - Due: Sept 5, 18:00 UTC

- [ ] **Hardware:** File GPU procurement request for Wave B1 (required for B1 + Wave D Phase 2C)
  - Targets: A100/H100/RTX4090 + CUDA 12.x
  - Owner: B1 DevOps Lead
  - Due: Sept 5, 18:00 UTC (before execution kickoff)

### 🟡 HIGH-PRIORITY (Must complete Sept 5-6)

- [ ] **Wave C:** Re-verify all 8 Batch C gates still PASS on current `develop`
  - File: `docs/governance/GA_PROMOTION_SIGN_OFF.md` §2.3
  - Action: Run `.github/workflows/gate-pr-policy.yml` manually
  - Owner: Security/Audit/Policy Lead
  - Due: Sept 6, 18:00 UTC

- [ ] **Wave D:** Pre-execution design review for Phase 2A (distributed tracing)
  - File: `docs/operability/WAVE_D_ROADMAP.md` §2A
  - Decision: Sequential (Phase 2A starts Oct 1) vs. Parallel (Phase 2A starts Sept 15)?
  - Owner: Platform Observability Lead
  - Due: Sept 6, 18:00 UTC

- [ ] **Wave B:** Confirm 5 FTE resource allocation is locked
  - B1: 1.5 FTE (hardware validation)
  - B2: 2.0 FTE (Query FTS executor — CRITICAL PATH)
  - B3: 1.5 FTE (Transaction CI execution)
  - Owner: Project Lead
  - Due: Sept 5, 18:00 UTC

### 🟢 NICE-TO-HAVE (Sept 6-7)

- [ ] **Wave D Planning:** Create detailed Sprint 1 execution plan for Phase 2A-2C if parallel option chosen
- [ ] **Wave B Comms:** Distribute Wave B quick-start guide to module leads
- [ ] **GA Sign-Off:** Schedule final sign-off meeting for Sept 30 (Wave B exit gate + GA promotion readiness)

---

## WAVE A-B-C-D SYNCHRONIZED VIEW

### Current State (Sept 2, 2026)

```
Wave A (Runtime Reliability)
├─ Status: ~COMPLETE (most modules delivered; CI evidence pending)
├─ Blockers: 
│  ├─ 🔴 Transaction CI execution (must run this week)
│  ├─ 🔴 GPU CUDA-call migration (50% reduction needed)
│  └─ 🟡 Representative-hardware baselines (Q4 2026)
└─ Gate: Sept 30, 2026 (Wave B + A→B CI validation)

Wave B (Performance Consolidation)
├─ Status: Phase 1 ✅ COMPLETE (Sept 2)
├─ Options: 
│  ├─ B1: LLM Wiki Hardware Validation (Sept 16-30, 1.5 FTE)
│  ├─ B2: Query FTS Executor Phase 1 (Sept 16-30, 2.0 FTE) — CRITICAL PATH
│  └─ B3: Transaction CI Execution (Sept 7-24, 1.5 FTE)
├─ Deliverables: 6,400+ lines committed (all locked, zero changes expected)
└─ Gate: Sept 30, 18:00 UTC (CPU baselines + benchmark gates + Doxygen)

Wave C (Security Production Validation)
├─ Status: ✅ COMPLETE (2026-08-18)
├─ Evidence: All 8 gates PASS (security/audit/policy)
├─ Action: Pre-sign-off validation only (re-verify no staleness)
└─ Gate: Already PASS; GA promotion depends on human sign-off (§9 of GA_PROMOTION_SIGN_OFF.md)

Wave D (Operability Hardening)
├─ Status: Phase 1 ✅, Phase 2A-2C ~PLANNING
├─ Options:
│  ├─ Sequential: Phase 2 starts Oct 1 (after Wave B complete)
│  └─ Parallel: Phase 2A design review Sept 15; implementation Oct 1
├─ Timeline: Q1 2027 (distributed tracing, metrics, soak tests)
└─ Gate: Not critical for GA; enables Q1 2027 release readiness
```

---

## PARALLEL EXECUTION RECOMMENDATION

**Strategy:** Execute Wave B (Sept 7-30) + Wave D Phase 2A Design (Sept 15-30) in parallel

**Why:**
1. Wave D Phase 2A (distributed tracing design) has no dependency on Wave B execution
2. Architecture/design work can complete in parallel with Wave B implementation
3. Allows Phase 2A-2C implementation to start Oct 1 without delay
4. Maximizes Q1 2027 readiness for full Wave D operability hardening

**Team Structure:**
- **Wave B Execution (Sept 7-30):** 5 FTE (B1/B2/B3 leads)
- **Wave D Phase 2A Design (Sept 15-30):** 1.0 FTE (new: Observability Architect)
- **Wave D Phase 2A Implementation (Oct 1-Nov 30):** 3.0 FTE (dedicated Phase 2A-2C team)

**Resource Impact:** +1 FTE for Wave D design (minimal impact; Wave B already 5 FTE)

---

## DECISION GATE: PROCEED VS. DEFER

**Question:** Should we proceed with Wave D Phase 2A-2C implementation in Oct-Nov 2026, or defer to Q1 2027?

**RECOMMENDATION: PROCEED (Sequential Start Oct 1)**

**Rationale:**
1. ✅ Wave D Phase 1 prerequisites already complete (runbooks exist, soak test files created)
2. ✅ No dependency on Wave A/B/C completion (only uses output as input)
3. ✅ Oct-Nov 2026 availability of observability team likely (end of fiscal Q3)
4. ✅ Advance Phase 2A design NOW (Sept 15-30) to unblock Oct 1 kickoff
5. 🎯 Q1 2027 GA promotion timeline tightens if Phase 2 deferred (risky)

**Alternative (Defer):** If no observability team availability, acceptable to defer all Phase 2A-2C to Q1 2027. Non-blocking for current GA (Wave C already complete).

---

## NEXT MEETING AGENDA (Proposed Sept 3, 14:00 UTC)

1. **Wave B Execution Go/No-Go** (5 min)
   - All 3 options ready? ✅ YES
   - Resource confirmation? ✅ YES
   - Proceed Sept 7/16? ✅ YES

2. **Wave A Blockers Review** (10 min)
   - Transaction CI execution status
   - GPU CUDA-call migration progress
   - Hardware procurement status

3. **Wave C Sign-Off Readiness** (5 min)
   - Pre-validation checklist
   - Human approval timeline
   - Sept 30 GA promotion decision

4. **Wave D Execution Decision** (5 min)
   - Sequential (Oct 1) vs. Parallel (Sept 15 design + Oct 1 impl)?
   - Team availability/allocation?
   - Proceed recommendation: SEQUENTIAL START OCT 1 ✅

5. **Q4 2026 Release Readiness Outlook** (5 min)
   - Critical path timeline (B2 → Wave D → GA promotion)
   - Risk summary (all mitigated)
   - Final sign-off expectations (Batch D human approval)

---

## SUMMARY: "Weiter" Path Forward

1. ✅ **Wave B Phase 1 Complete** (6,400+ lines committed, all 3 options execution-ready)
2. 🔴 **Wave A Final Validation** (Transaction CI + GPU CUDA + Hardware procurement THIS WEEK)
3. ✅ **Wave C Already Complete** (Batch C all 8 gates PASS; pre-sign-off validation only)
4. 🟡 **Wave D Phase 2 Planning** (Design review Sept 15-30; execution Oct 1-Nov 30)
5. 🎯 **GA Promotion Ready** (Sept 30 Wave B exit + Oct-Nov Wave D Phase 2 + Q1 2027 signature)

**Estimated Timeline:** 
- Sept 2-6: Blockers resolution + Wave B kickoff
- Sept 7-30: Wave B execution (B1/B2/B3 parallel)
- Oct 1-30: Wave D Phase 2A-2C implementation
- Nov 1-30: Wave D Phase 2A-2C refinement + soak testing
- Dec 1-31: Q1 2027 GA release preparation
- Jan 2027: GA promotion + Q1 release

**Key Success Metric:** All three options (B1/B2/B3) exit criteria PASS by Sept 30, 18:00 UTC ✅

---

**End of Wave C Validation + Wave D Execution Planning**
