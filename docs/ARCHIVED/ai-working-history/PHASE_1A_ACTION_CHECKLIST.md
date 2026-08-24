# Phase 1A Action Checklist — GA Promotion Sign-Off Closure (Immediate Actions)

**Document:** Day-1 implementation checklist for Phase 1A execution  
**Date:** 2026-08-05  
**Owner:** Release Approver + Release Engineer  
**Status:** 🔴 **READY TO START**

---

## Quick Start: Next 24 Hours

### Day 1 (2026-08-05)

**Release Approver:**
- [ ] Read PHASE_1A_GA_SIGN_OFF_DETAILED_PLAN.md (full plan: 30 min)
- [ ] Schedule Security Lead review meeting for sanitizer/pentest bundles (15 min)
- [ ] Check `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9 — confirm what sign-off is required
- [ ] Send authorization to Release Engineer to start verification suite

**Release Engineer:**
- [ ] Verify current `develop` branch is at clean state (`git status` clean, no uncommitted work)
- [ ] Run the full CI verification suite (this takes ~45 min to 1 hour):
  ```bash
  # Build on current develop HEAD
  cd /home/runner/work/ThemisDB/ThemisDB
  git log --oneline -5
  
  # Verify prerequisites installed
  # Configure community-release preset
  cmake --preset community-release -DCMAKE_BUILD_TYPE=Release
  
  # Build
  cmake --build --preset community-release --parallel 16 --target all
  
  # Run release_critical tests
  ctest --preset community-release -L release_critical -VV --output-on-failure
  ```
- [ ] Archive build log to `artifacts/INITIAL_BUILD_2026-08-05.log`
- [ ] Start Wave 7 benchmarks in background
- [ ] Update PHASE_1A_GA_SIGN_OFF_DETAILED_PLAN.md Section "Day 1 Verification Results" with findings

---

## Core Verification Tasks (Days 1–3)

### Step 1: CI Suite Verification

**Owner:** Release Engineer  
**Timeline:** Days 1–2  
**Effort:** 4–6 hours

**Checklist:**
- [ ] Clone `develop` branch to fresh build directory (if not already done)
- [ ] Configure `community-release` preset
  ```bash
  cmake --preset community-release \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_TESTING=ON \
    -DWITH_BENCHMARKS=ON
  ```
- [ ] Build full project
  ```bash
  cmake --build --preset community-release --parallel 16 --target all
  ```
- [ ] Verify build completes with zero errors (warnings OK)
- [ ] Run release_critical test suite
  ```bash
  ctest --preset community-release -L release_critical -VV --output-on-failure
  ```
- [ ] Capture all test output to `artifacts/RELEASE_CRITICAL_TEST_RUN_2026-08-XX.txt`
- [ ] Count total tests passed/failed
- [ ] If ANY FAILURES: escalate immediately to phase lead for triage (do NOT continue)

**Definition of Done:**
- ✅ Build succeeds (zero compiler errors)
- ✅ All release_critical tests PASS (zero failures)
- ✅ Log files archived

---

### Step 2: Wave 7/8/9 Gate Verification

**Owner:** Release Engineer (can run in parallel with Step 1)  
**Timeline:** Days 2–3  
**Effort:** 3–4 hours

**Checklist:**
- [ ] Navigate to benchmarks/wave7/
  ```bash
  cd benchmarks/wave7/
  ls -la release_gate_manifest_w7.json
  ```
- [ ] Review existing gate manifest to understand structure
- [ ] Run Wave 7 benchmark suite
  ```bash
  ctest --preset community-release -L wave7 -VV
  ```
- [ ] Capture results to `artifacts/WAVE7_GATE_RESULTS_2026-08-XX.json`
- [ ] Extract GATE-W7-01..06 status from results
- [ ] Verify all GATE-W7-* entries report "PASS" (zero failures)
- [ ] Repeat for Wave 8 (`-L wave8`)
- [ ] Repeat for Wave 9 (`-L wave9`)

**Gate Targets (for validation):**
| Gate | Metric | Target | Actual | Status |
|------|--------|--------|--------|--------|
| GATE-W7-01 | Read Latency p99 | ≤200µs | ? | ? |
| GATE-W7-02 | Write Throughput | ≥80k ops/s | ? | ? |
| GATE-W7-03 | Pool Efficiency | ≥90% | ? | ? |
| GATE-W7-04 | Network I/O @ 1% p99 | <10 Gbps | ? | ? |
| GATE-W7-05 | Memory Footprint | <8 GB | ? | ? |
| GATE-W7-06 | Storage Compaction p99 | <500 IOPS | ? | ? |
| GATE-W8-01 | Soak Duration | 72h+ | ? | ? |
| GATE-W8-02 | Error Recovery RTO | ≤5s | ? | ? |
| GATE-W8-03 | Cascade Rejoin | ≤2s | ? | ? |
| GATE-W8-04 | Fault Injection Coverage | 92% scenarios | ? | ? |
| GATE-W9-01 | SLA Uptime | 99.99% | ? | ? |
| GATE-W9-02 | Auth Overhead p99 | ≤150µs | ? | ? |
| GATE-W9-03 | Network Recovery | ≤2s | ? | ? |
| GATE-W9-04 | RTO per Shard | ≤5s | ? | ? |
| GATE-W9-05 | Chaos Coverage | 40+ scenarios | ? | ? |
| GATE-W9-06 | SLA Proof Evidence | 168h plan | ✅ | PASS |

**Definition of Done:**
- ✅ Wave 7 gates: All GATE-W7-01..06 PASS
- ✅ Wave 8 gates: All GATE-W8-01..04 PASS
- ✅ Wave 9 gates: All GATE-W9-01..06 PASS
- ✅ Results files archived

---

### Step 3: Security Review Coordination

**Owner:** Release Engineer (on behalf of Release Approver)  
**Timeline:** Day 2  
**Effort:** 2–3 hours (coordination + response time)

**Checklist:**
- [ ] Identify Security Lead contact email/Slack
- [ ] Send security review request with subject:  
  **Subject:** "Phase 1A GA Sign-Off: Security Lead Sign-Off Required for v2.4.0 GA (by 2026-08-10)"
- [ ] Include in message:
  ```
  Hi [Security Lead],
  
  ThemisDB v2.4.0 is entering final GA promotion phase (Phase 1A).
  
  We need your review and sign-off on the following:
  
  1. Sanitizer Evidence Bundle
     File: docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md
     Review: Confirm zero new ASan/UBSan/TSan defects in Phase 4-6 scope
  
  2. Penetration Test Evidence Bundle
     File: security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md
     Review: Confirm no new Critical/High findings (PTR-01/PTR-02 residual accepted)
  
  3. STRIDE Threat Model
     File: security/STRIDE_THREAT_MODEL.md
     Review: Confirm threat model is current and risk acceptance documented
  
  Please review by 2026-08-08 and provide sign-off email.
  
  Best regards,
  [Release Engineer]
  ```
- [ ] Send follow-up message to Release Approver with Security Lead contact confirmation
- [ ] Log Security Lead response when received
- [ ] Check for any open CRITICAL findings from CodeQL or security scans
  ```bash
  cd /home/runner/work/ThemisDB/ThemisDB
  # Check if security scan reports exist
  find . -name "*security*" -type f | grep -E "(report|scan|findings)" | head -10
  ```
- [ ] Confirm zero new CRITICAL findings

**Definition of Done:**
- ✅ Security Lead review request sent
- ✅ Review deadline set (2026-08-08)
- ✅ Sign-off response captured (email/comment)
- ✅ CodeQL/security scans show zero new CRITICALs

---

### Step 4: Module Gap Register Audit

**Owner:** Release Engineer  
**Timeline:** Days 2–3  
**Effort:** 2–3 hours

**Checklist:**
- [ ] Review existing module gap reports:
  ```bash
  ls -la src/*/MODULE_GAPS.md
  # Should exist for: server, llm, sharding (at minimum)
  ```
- [ ] Check `src/server/MODULE_GAPS.md` for CRITICAL findings
  ```bash
  grep -i "CRITICAL" src/server/MODULE_GAPS.md | wc -l
  # Should return 0 or baseline count from last audit
  ```
- [ ] Check `src/llm/MODULE_GAPS.md` for CRITICAL findings
- [ ] Check `src/sharding/MODULE_GAPS.md` for CRITICAL findings
- [ ] If any new CRITICALs found: escalate to module owners
- [ ] Archive audit snapshot to `artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt`
  ```bash
  echo "=== Module Gap Audit (2026-08-XX) ===" > artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  echo "" >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  echo "Server Critical Count:" >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  grep -i "CRITICAL" src/server/MODULE_GAPS.md | wc -l >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  echo "" >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  echo "LLM Critical Count:" >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  grep -i "CRITICAL" src/llm/MODULE_GAPS.md | wc -l >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  echo "" >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  echo "Sharding Critical Count:" >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  grep -i "CRITICAL" src/sharding/MODULE_GAPS.md | wc -l >> artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt
  ```

**Definition of Done:**
- ✅ Zero new CRITICAL findings in server, llm, sharding
- ✅ Gap audit archived

---

### Step 5: Documentation Verification

**Owner:** Release Engineer  
**Timeline:** Days 2–3  
**Effort:** 1–2 hours

**Checklist:**
- [ ] Review CHANGELOG.md
  ```bash
  head -20 CHANGELOG.md
  # Verify [Unreleased] section OR [2.4.0] section exists
  ```
- [ ] Confirm CHANGELOG.md has entry for v2.4.0 GA with all phase summaries
  - [ ] Phase 1 summary included
  - [ ] Phase 2 summary included
  - [ ] Phase 3 summary included
  - [ ] Phase 4 summary included
  - [ ] Phase 5 summary included
  - [ ] Phase 6 summary included (if applicable)

- [ ] Review VERSIONING.md
  ```bash
  grep -A 5 "2.4.0" VERSIONING.md
  # Verify version table shows v2.4.0 with status = "GA stable"
  ```

- [ ] Review ROADMAP.md
  ```bash
  grep -E "\[x\]" ROADMAP.md | head -20
  # Verify all Phase 0-6 items are marked complete
  ```

- [ ] Check research/implementation_influence/by_module.md exists and is current
  ```bash
  ls -la research/implementation_influence/by_module.md
  stat research/implementation_influence/by_module.md | grep -i modify
  ```

- [ ] Verify docs/DOXYGEN_COVERAGE_REPORT.md (if exists)
  ```bash
  # Check header coverage is >99%
  grep -i "coverage" docs/DOXYGEN_COVERAGE_REPORT.md 2>/dev/null || echo "Report not yet generated"
  ```

**Definition of Done:**
- ✅ CHANGELOG.md entry present for v2.4.0
- ✅ VERSIONING.md confirms GA status
- ✅ ROADMAP.md shows Phase 0–6 completion
- ✅ research/implementation_influence/by_module.md is current

---

## Phase 1A Approval Gate (Days 4–7)

### Step 6: Human Sign-Off Coordination

**Owner:** Release Approver  
**Timeline:** Days 4–7  
**Effort:** 2–3 hours

**Checklist:**
- [ ] Collect all verification artifacts from Release Engineer:
  - [ ] CI suite results (release_critical PASS)
  - [ ] Wave 7/8/9 gate results (all PASS)
  - [ ] Module gap audit (zero new CRITICALs)
  - [ ] Documentation verification (CHANGELOG/VERSIONING/ROADMAP current)

- [ ] Review `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9
  - [ ] Understand what sign-off is required (likely section contains a sign-off template or checklist)
  - [ ] Confirm all acceptance criteria in Section 9 are met
  - [ ] If any criteria not met: escalate and re-run verification

- [ ] Schedule final sign-off review meeting with stakeholders:
  - [ ] Release Approver
  - [ ] Security Lead (present sanitizer/pentest review results)
  - [ ] Architecture Lead (confirm no architectural regressions)
  - [ ] QA Lead (confirm test coverage)

- [ ] In meeting, review:
  - [ ] All CI tests PASS
  - [ ] All Wave 7/8/9 gates PASS
  - [ ] Security bundles reviewed and accepted
  - [ ] Module gaps cleared
  - [ ] Documentation current and complete

- [ ] **Release Approver Signs Off:** Add sign-off entry to `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9:
  ```markdown
  ## Section 9: Final Human Sign-Off
  
  **Date:** 2026-08-XX  
  **Approver:** [Name]  
  **Title:** [Release Approver / Release Manager]  
  
  ✅ **APPROVED FOR RELEASE v2.4.0 GA**
  
  I hereby confirm that:
  - All CI verification tests PASS (release_critical suite: 0 failures)
  - Wave 7 gates: All 6 gates PASS
  - Wave 8 gates: All 4 gates PASS
  - Wave 9 gates: All 6 gates PASS
  - Security bundles reviewed and accepted by Security Lead
  - Module gaps cleared (server/llm/sharding: zero new CRITICALs)
  - Documentation current (CHANGELOG, VERSIONING, ROADMAP)
  - No architectural regressions detected
  - Ready for production release
  
  **Signature:** [Digital Sign-Off / Approval Comment]
  ```

**Definition of Done:**
- ✅ Human sign-off recorded in docs/governance/GA_PROMOTION_SIGN_OFF.md
- ✅ All stakeholders confirmed ready

---

## Phase 1A Completion (Days 7–8)

### Step 7: Release Tag Creation

**Owner:** Release Engineer (authorized by Release Approver)  
**Timeline:** Days 7–8  
**Effort:** 1 hour

**Checklist:**
- [ ] Verify all Phase 1A approval gates complete
- [ ] Create annotated tag on current `develop` HEAD:
  ```bash
  git tag -a v2.4.0 -m "ThemisDB v2.4.0 GA Release
  
  Phase 1A Closure Complete:
  - All CI verification tests PASS
  - Wave 7/8/9 gates all PASS
  - Security review complete
  - Module gaps cleared
  - Ready for production release
  
  See docs/governance/GA_PROMOTION_SIGN_OFF.md Section 9 for sign-off details."
  
  # Verify tag created
  git tag -l v2.4.0
  ```

- [ ] Push tag to origin:
  ```bash
  git push origin v2.4.0
  ```

- [ ] Verify tag is reachable on GitHub
  ```bash
  # On GitHub, navigate to Releases or run:
  gh release view v2.4.0
  ```

**Definition of Done:**
- ✅ Tag v2.4.0 created locally
- ✅ Tag pushed to origin
- ✅ Tag reachable and verified on GitHub

---

### Step 8: Post-Verification Release Build

**Owner:** Release Engineer  
**Timeline:** Days 8  
**Effort:** 2 hours

**Checklist:**
- [ ] Checkout the v2.4.0 tag
  ```bash
  git checkout v2.4.0
  ```

- [ ] Verify release build succeeds from tag (not develop HEAD)
  ```bash
  cmake --preset community-release -DCMAKE_BUILD_TYPE=Release
  cmake --build --preset community-release --parallel 16
  ```

- [ ] Build succeeds with zero errors
- [ ] Archive build log to `artifacts/RELEASE_BUILD_v2.4.0.log`

**Definition of Done:**
- ✅ Release build from v2.4.0 tag succeeds
- ✅ Binary/package artefact produced

---

## Summary: What Success Looks Like

✅ **Phase 1A Complete when:**
1. CI suite passes (release_critical: 0 failures)
2. Wave 7/8/9 gates all PASS
3. Security Lead reviews bundles and approves
4. Module gaps cleared (zero new CRITICALs)
5. Documentation current
6. Human sign-off obtained and recorded
7. Tag v2.4.0 created and pushed
8. Release build from tag succeeds

✅ **Outcome:** v2.4.0 GA is production-ready for promotion through release lanes (Minimal → Community → Enterprise → Hyperscaler → Military)

---

## Escalation & Support

**If Tests FAIL:**
- [ ] Do NOT continue to Step 5+
- [ ] Escalate to relevant module owner
- [ ] Document failure reason and remediation
- [ ] Rerun verification after fix
- [ ] Retry full CI suite (reset to Step 1)

**If Security Review BLOCKS:**
- [ ] Do NOT create release tag
- [ ] Escalate to CTO or Release Authority
- [ ] Address security concerns before retry

**If Time Runs Out (past 2026-08-11):**
- [ ] Escalate delay to Release Authority
- [ ] Reschedule Phase 1A completion date
- [ ] Communicate to Phases 2–5 team leads (expect Phase 1A delay)

---

**Document Status:** 🔴 **READY FOR IMMEDIATE START**  
**Owner:** Release Approver + Release Engineer  
**Assigned By:** AI Delivery Agent  
**Start Date:** 2026-08-05  
**Target Completion:** 2026-08-11
