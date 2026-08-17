# Wave A/B Remediation Program — Quick Reference Guide

**Generated:** 2026-08-14  
**Status:** Execution Framework Complete; A1 Build Verification In Progress  
**Next Action:** Review A1 results; unblock A2-A5 parallel execution  

---

## Executive Summary (1-Pager)

**What:** Wave A/B remediation program across 8 batches (5 modules Wave A, 3 modules Wave B) + 38 remaining modules (Batches 7-13).

**Timeline:**
- Wave A (A1-A5): 2026-08-14 → 2026-10-02 ✅ Comprehensive action plans drafted
- Wave B (B1-B3): 2026-10-02 → 2026-10-31 (blocked on Wave A exit)
- Batches 7-13: 2026-10-02 → 2027-03-31 (blocked on Wave B exit)

**Gate Model:** Strict wave-gate progression. No batch starts until predecessor exit criteria validated.

**Success Criteria:** All distributed paths fail-closed; major modules have p95/p99 baselines; chaos evidence complete; operability runbooks signed-off by 2027-03-31.

---

## File Navigation

| Document | Purpose | Read Time | Audience |
|----------|---------|-----------|----------|
| **WAVE_AB_EXECUTION_MASTER_BOARD.md** | Master control thread; all batches + gates | 20 min | Orchestrator, Project Lead |
| **WAVE_A1_DETAILED_ACTION_PLAN.md** | A1 critical path; build/run + chaos evidence | 25 min | A1 owner, Build team |
| **WAVE_A2_A5_PARALLEL_ACTION_PLANS.md** | A2-A5 detailed tasks + dependencies | 30 min | A2-A5 owners, Implementation teams |
| **BATCH_7_13_REMAINING_MODULES_INVENTORY.md** | 38-module inventory + post-Wave-B scheduling | 20 min | Orchestrator, Planning lead |
| **This File** | Quick reference + checklists | 10 min | Everyone |

---

## Role-Specific Guidance

### 🎯 Orchestrator (Wave Program Lead)

**Your Weekly Checklist:**

1. **Monday:** Review status of active batches
   - Check `WAVE_AB_EXECUTION_MASTER_BOARD.md` for gate status
   - Flag any blockers or delays
   
2. **Wednesday:** Mid-week sync with batch owners
   - Verify no unexpected issues
   - Escalate delays immediately

3. **Friday:** Close-out & roadmap adjustment
   - Update master board with completion status
   - Gate validation: prerequisites met for next batch?
   - Unblock next-in-line batch if gate PASS

**Key Metrics to Track:**
- A1: Build ✅ / Run ✅ / Chaos evidence ✅ (target: 2026-09-04)
- A2-A5: Implementation % complete (target: 2026-09-25)
- Wave A exit gate: Release-critical CI (target: 2026-10-02)
- B1-B3: Performance gates locked (target: 2026-10-31)
- Batches 7-13: Closure evidence filed (target: 2027-03-31)

### 🔨 Implementation Owner (A1-A5 / B1-B3 / Batches 7+)

**Your Workflow:**

1. **Read:** Detailed action plan for your batch
   - Understand acceptance criteria (✅ checkboxes in master board)
   - Identify your tasks + target dates
   
2. **Build:** Implement in phases
   - Phase 1: Design/API (few days)
   - Phase 2: Core implementation (1–2 weeks)
   - Phase 3: Testing + verification (1 week)
   - Phase 4: CI gate + closure (1 week)

3. **Evidence:** Document closure
   - Build logs + test results → `BATCH_X_CLOSURE_REPORT.md`
   - Chaos evidence → `BATCH_X_CHAOS_EVIDENCE.md` (if applicable)
   - Runbooks → `BATCH_X_OPERATOR_RUNBOOK.md` (if applicable)

4. **Gate:** Signal readiness to orchestrator
   - All acceptance criteria ✅
   - `release_critical` CI green
   - Evidence bundle filed
   - Ready for: ✅ Next batch unblock OR ⏸️ Waiting for dependencies

### 🧪 Testing/QA Lead

**Your Checklist:**

- [ ] Review test coverage for your assigned batch
  - Unit tests: coverage ≥ 80%
  - Integration tests: real-world scenarios
  - Chaos tests: fault-injection + recovery
  
- [ ] Verify `release_critical` CI gates
  - Run locally if possible
  - Identify flaky tests early
  - Escalate test infrastructure issues

- [ ] Validate evidence
  - Spot-check test logs (sample 10% of test output)
  - Verify CRC/hash for consistency claims
  - Confirm reproducibility (≥2 runs with same outcome)

### 🏗️ Build/Infrastructure Team

**Your Setup Checklist:**

- [ ] **CMake Presets:** Verify all Wave A/B modules compile with `community-release` preset
  - ✅ transaction, replication, voice, gpu, sharding, search, access_model, llm_wiki
  - ✅ Required: RocksDB (optional via diagnostic preset for community builds)
  - ✅ CUDA (optional; CPU fallback tests run unconditionally)

- [ ] **CI/CD:** Ensure `.github/workflows/09-pr-gates_release-critical-tests.yml` green
  - ✅ Wave A modules: transaction, replication, voice, gpu, sharding
  - ✅ Wave B modules: search, access_model, llm_wiki
  - ⏸️ Batches 7-13: defer to post-Wave-B entry

- [ ] **Performance Baseline:** Capture `representative hardware` p95/p99 for Wave A/B modules
  - Storage: `benchmarks/wave_baseline_*.json`
  - Include: CPU, memory, disk I/O, network config

### 🔒 Security/Compliance Lead

**Your Validation Checklist:**

- [ ] **Wave A Gate:** Chaos evidence integrity
  - Verify deterministic crash-recovery (5/5 runs identical)
  - Validate fail-closed behavior (no silent data loss)
  - Audit WAL replay idempotency

- [ ] **Batch 7 Gate:** Ethics AI + Security module completeness
  - Ethics AI: ChainVisualizer + NormEvidence + CSEP tests (≥8)
  - Security: Vault/HSM/PKI integration + RLS production workload
  - Audit: Integrity verification + export reliability

- [ ] **Release Gate:** Security sign-off
  - All security controls have production-style validation
  - Penetration test evidence bundled (`security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`)
  - No known vulnerabilities in released code

---

## Quick Decisions

### "When should my batch start?"

| Your Batch | Start Trigger | Target Start Date |
|-----------|--------------|------------------|
| A2-A5 (Replication, Voice, GPU, Sharding) | A1 exit criteria PASS | 2026-09-04 |
| B1-B3 (Search, Access Model, LLM Wiki) | Wave A exit criteria PASS | 2026-10-02 |
| B7 (Ethics AI, Security, Audit, Observability) | Wave B entry (parallel to B1-B3) | 2026-10-02 |
| B8 (Server, LLM, Tensor top-risk) | B7 exit criteria PASS | 2026-10-31 |
| B9–B13 (Remaining 38 modules) | Previous batch exit | Rolling: 2026-11 → 2027-03 |

### "My test is failing. What do I do?"

1. **Reproduce locally:** Can you reproduce on your machine? 
   - YES: Debug + fix locally; re-run test
   - NO: Likely CI/environment-specific; check CI logs for env diffs

2. **Is it expected (known issue)?**
   - YES: File issue, tag as `batch-X-known-issue`; plan remediation
   - NO: Unknown failure; triage immediately

3. **How long can I delay?**
   - If blocking gate → escalate to orchestrator ASAP
   - If internal to your batch → ≤2 days; then escalate
   - Never delay beyond target date without orchestrator approval

### "How do I document chaos evidence?"

**Template:**
```markdown
# AC-6: Coordinator Crash-Recovery

## Test Scenario
- Setup: [describe initial state]
- Inject: [describe failure]
- Trigger: [describe recovery]

## Results
| Run | Outcome | Duration | Notes |
|-----|---------|----------|-------|
| 1 | ✅ PASS | 2.5s | ... |
| 2 | ✅ PASS | 2.3s | ... |
| 3 | ✅ PASS | 2.8s | ... |
| 4 | ✅ PASS | 2.4s | ... |
| 5 | ✅ PASS | 2.6s | ... |

## Conclusion
Deterministic recovery: 5/5 runs PASS within 5s SLA ✅
```

---

## Status Dashboard (Updated Weekly)

### Wave A Progress (As of 2026-08-14)

```
A1 [████░░░░░░] 25% — Build verification in progress (target: 2026-08-21)
A2 [░░░░░░░░░░]  0% — Waiting for A1 exit (start: 2026-09-04)
A3 [░░░░░░░░░░]  0% — Waiting for A1 exit (start: 2026-09-04)
A4 [░░░░░░░░░░]  0% — Waiting for A1 exit (start: 2026-09-04)
A5 [░░░░░░░░░░]  0% — Waiting for A1 exit (start: 2026-09-04)
───────────────────────────────────
Wave A Exit: ⏳ PENDING (target: 2026-10-02)
```

### Wave B Progress (As of 2026-08-14)

```
B1 [░░░░░░░░░░]  0% — Waiting for Wave A exit (start: 2026-10-02)
B2 [░░░░░░░░░░]  0% — Waiting for Wave A exit (start: 2026-10-02)
B3 [░░░░░░░░░░]  0% — Waiting for Wave A exit (start: 2026-10-02)
───────────────────────────────────
Wave B Exit: ⏳ PENDING (target: 2026-10-31)
```

### Batch 7–13 Progress (As of 2026-08-14)

```
B7  [░░░░░░░░░░]  0% — Waiting for Wave B entry (start: 2026-10-02)
B8  [░░░░░░░░░░]  0% — Waiting for B7 exit (start: 2026-10-31)
B9  [░░░░░░░░░░]  0% — Waiting for B8 exit (start: 2026-11-15)
...
B13 [░░░░░░░░░░]  0% — Waiting for B12 exit (start: 2027-03-01)
───────────────────────────────────
Program Completion: ⏳ PENDING (target: 2027-03-31)
```

---

## Critical Dates (Calendar)

| Date | Milestone | Status |
|------|-----------|--------|
| 2026-08-14 | Wave A/B framework complete | ✅ TODAY |
| 2026-08-21 | A1 build/run verify (Phase 2+3) | 📌 Next target |
| 2026-09-04 | A1 chaos evidence complete; A2-A5 kickoff | 📌 In 3 weeks |
| 2026-10-02 | Wave A exit criteria PASS; Wave B + B7 start | 📌 In 7 weeks |
| 2026-10-31 | Wave B exit; B7-B8 checkpoints | 📌 In 11 weeks |
| 2026-11-15 | B8 exit; B9 kickoff | 📌 In 13 weeks |
| 2026-12-15 | B9 exit; B10 mid-point | 📌 In 17 weeks |
| 2026-12-31 | B10 exit; B11 kickoff | 📌 In 19 weeks |
| 2027-01-31 | B11 exit; B12 kickoff | 📌 In 22 weeks |
| 2027-02-28 | B12 exit; B13 kickoff | 📌 In 27 weeks |
| 2027-03-31 | Program completion | 📌 In 32 weeks |

---

## Commonly Asked Questions

### Q: Can we parallelize across waves (e.g., B1 before A5 finishes)?

**A:** No. Wave gates are strict. Wave B entry requires **all** Wave A exit criteria PASS (deterministic chaos evidence + p95/p99 baselines + `release_critical` CI green). This ensures Wave B has stable runtime foundations to optimize performance.

### Q: What if a test fails?

**A:** Depends on gate status:
- **Before exit gate:** Escalate to batch owner; debug + fix; re-run
- **At exit gate:** Escalate to orchestrator + codebase lead; triage; plan remediation; delay documented + approved
- **Post-exit gate (discovered in downstream batch):** File issue; prioritize for next opportunity; document in closure report as known issue

### Q: How much effort is each batch?

**A:** Rough estimates (person-hours):
- A1: 200h (build + run + chaos)
- A2-A5 (each): 140–180h
- B1-B3 (each): 160–200h
- Batch 7 (4 modules): 650h
- Batches 8-13 (per batch): 700–900h
- Total: ~8,000 person-hours (~10–12 FTE × 8 months)

### Q: Can we compress the timeline?

**A:** Limited options:
- Wave A/B: Strict gates; no compression
- Batches 7-13: Parallel modules within batch (already planned); can't start batch early
- Best strategy: Ensure A1 completes on-time; every week gained at A1 → gained at Wave B → gained at Batch 7+

### Q: What if we miss a date?

**A:** Escalate immediately:
1. Notify orchestrator
2. Root-cause analysis (blocker? resource? underestimate?)
3. Propose revised timeline + mitigation
4. Update master board
5. Document delay + decision in closure report

---

## Key Contacts & Escalation Path

| Role | Responsibility | Contact |
|------|---------------|---------| 
| **Wave Orchestrator** | Master board; gate validation; blocker escalation | TBD |
| **A1 Owner** | Transaction batch; critical path | TBD |
| **A2-A5 Owners** | Replication, Voice, GPU, Sharding batches | TBD (one per module) |
| **B1-B3 Owners** | Search, Access Model, LLM Wiki batches | TBD (one per module) |
| **Build/CI Team** | CMake presets; workflow execution; CI gates | TBD |
| **Performance Team** | Baseline capture; bottleneck analysis; optimization | TBD |
| **Security Lead** | Chaos/recovery validation; security sign-off | TBD |
| **Release Lead** | GA sign-off; documentation; deployment readiness | TBD |

**Escalation Path for Blockers:**
```
Batch Owner → Orchestrator → Release Lead → Project Lead
```

---

## Success Metrics

**Daily:**
- A1 build/run verification: no new blockers

**Weekly:**
- Active batch: ≥90% progress toward weekly target
- Closed issues: ≤2 unexpected; all documented
- `release_critical` CI: green (no new failures)

**On Batch Exit:**
- All acceptance criteria ✅
- Evidence bundle complete + signed off
- Zero open blockers or known issues
- `release_critical` CI: PASS

**Program Completion (2027-03-31):**
- All 43 modules have closure evidence
- All distributed paths fail-closed
- All p95/p99 baselines captured
- All chaos evidence deterministic
- GA promotion sign-off complete

---

**Last Updated:** 2026-08-14  
**Next Update:** Upon A1 build verification completion (target: 2026-08-21)
