# GA v2.4.0 Human Release Approver — Quick Reference Guide

**Prepared for:** Release Approver / Release Engineer  
**Date:** 2026-08-07  
**Action Required:** Review and Sign-Off (Section 9 of GA_PROMOTION_SIGN_OFF.md)  

---

## ✓ What You Need to Review

### 1. Benchmark Gate Results (AUTO-VALIDATED)

All Wave 9 hard gates must show **PASS**:

```
GATE-W9-01: Audit throughput ≥ 100k ops/s
GATE-W9-02: Auth p99 latency ≤ 150 µs
GATE-W9-03: Node restart rejoin ≤ 2000 µs
GATE-W9-04: RTO recovery cycle ≤ 5000 µs
GATE-W9-05: Triage completeness = 1.0
GATE-W9-06: Cross-tenant throughput ≥ 60k ops/s
```

- Validation script output: Look for "✓ All release gates PASS"
- Exit code: `0` = all gates pass, `1` = failure

### 2. Security Evidence (ALREADY VALIDATED)

**Status: ✓ COMPLETED 2026-08-01**

- **Sanitizer Evidence:** `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
  - ASan: Zero new defects ✓
  - UBSan: Zero new defects ✓
  - TSan: Zero new data races ✓

- **Pentest Evidence:** `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
  - Zero new Critical/High findings ✓
  - Residual risks documented: PTR-01, PTR-02 ✓

### 3. Module Gap Analysis (ALREADY VALIDATED)

**Status: ✓ VERIFIED 2026-08-04**

Check these module gap registers:
- `src/server/MODULE_GAPS.md` — No new CRITICAL findings
- `src/llm/MODULE_GAPS.md` — No new CRITICAL findings
- `src/sharding/MODULE_GAPS.md` — No new CRITICAL findings

### 4. Documentation & Versioning (ALREADY VALIDATED)

**Status: ✓ VERIFIED 2026-08-04**

- [x] `CHANGELOG.md` — Ready to move `[Unreleased]` → `[2.4.0]`
- [x] `VERSIONING.md` — v2.4.0 marked as "stable"
- [x] `ROADMAP.md` — All Phase 1-6 completion markers present
- [x] `FUTURE_ENHANCEMENTS.md` — Aligned with v2.4.0 scope
- [x] `docs/DOXYGEN_COVERAGE_REPORT.md` — 99.8% header coverage

---

## ⚡ Your Sign-Off Checklist

Before completing Section 9, verify:

- [ ] **Benchmark gates reviewed:** All 6 Wave 9 hard gates show PASS
- [ ] **Wave 7/8 regression gates reviewed:** Baseline maintained or improved
- [ ] **Sanitizer evidence reviewed:** ASan/UBSan/TSan all zero new defects
- [ ] **Pentest evidence reviewed:** Zero new Critical/High findings
- [ ] **Module gaps reviewed:** No new CRITICAL in server/llm/sharding
- [ ] **Deferred items accepted:** DEF-01, DEF-02, DEF-03, DEF-04
- [ ] **Documentation current:** CHANGELOG, VERSION, ROADMAP aligned

---

## ✍️ How to Sign Off

### Step 1: Open the Sign-Off Document

```bash
cd /home/runner/work/ThemisDB/ThemisDB
vim docs/governance/GA_PROMOTION_SIGN_OFF.md
# Go to line 183–206 (Section 9)
```

### Step 2: Fill in Your Information

Replace the placeholders in the signature block:

```markdown
Release Approver (name/role):  YOUR NAME, Release Engineer
Signature / Reference:          your-github-handle or your-email@company.com
Date:                           2026-08-07

Deferred items accepted (DEF-01..04): [x] Yes  [ ] No
```

### Step 3: Confirm Deferred Items

Check the box for "Yes" to accept the four deferred items:
- DEF-01: Build reproducibility → v1.9.1 patch
- DEF-02: Graph/query optimization → v2.0.0
- DEF-03: WAL/failover sharding (already completed)
- DEF-04: Gossip-port firewall → operator runbook

### Step 4: Mark Final Approval

Check the approval box:

```markdown
APPROVED:  [x] YES — proceed with develop → community merge and v2.4.0 tag
           [ ] NO  — open items: ______________________________
```

### Step 5: Commit and Push

```bash
git add docs/governance/GA_PROMOTION_SIGN_OFF.md
git commit -m "GA v2.4.0: Human release approval signed off by [YOUR NAME]"
git push origin develop
```

---

## 🚀 After You Sign Off

Once you mark the approval box:

1. **Merge Phase Begins** (Release Engineer)
   - Create merge commit: develop → community
   - Verify `release_critical` CI passes
   - Merge to `community` branch

2. **Tag Creation** (Release Engineer)
   - Create annotated tag: `v2.4.0`
   - Reference your sign-off in tag annotation
   - Push tag to origin

3. **Artefact Build & Release** (Release Engineer)
   - Build from v2.4.0 tag (NOT from develop)
   - Create GitHub Release entry
   - Attach binaries/packages

4. **Version Updates** (Release Engineer)
   - Update RELEASE_TYPE → "stable"
   - Update CHANGELOG: [Unreleased] → [2.4.0]
   - Commit and push to community

---

## ❌ If You Find Issues

If you identify missing validation or concerns:

1. **Do NOT sign off** — mark approval box as NO
2. **Document the issue** in the notes section:
   ```markdown
   Notes / conditions:
   [Describe the issue and what needs to be fixed]
   ```
3. **Open a GitHub issue** with `severity:P0` label
4. **Request remediation** from the team
5. **Re-run benchmarks** after fix
6. **Re-attempt sign-off** with corrected evidence

---

## 📋 Supporting References

**Documentation:**
- Full sign-off document: `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- Promotion checklist: `ai_working/GA_V2_4_0_PROMOTION_CHECKLIST.md`
- Execution runbook: `ai_working/GA_V2_4_0_PROMOTION_RUNBOOK.md`

**Evidence Artefacts:**
- Gate validation report: `/tmp/ga_v2_4_0_validation_report.json`
- Benchmark results: `benchmarks/results/wave{7,8,9}/*.json`
- Sanitizer evidence: `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
- Pentest evidence: `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`

**Release Strategy:**
- `RELEASE_STRATEGY.md` — Beta-to-GA gate model
- `VERSIONING.md` — Versioning and SLA
- `BRANCHING_STRATEGY.md` — Branch governance

---

## ⏱️ Timeline

- **Now:** Review evidence and this checklist (5–10 minutes)
- **Upon completion:** Sign-off in Section 9 (2–3 minutes)
- **Next:** Release Engineer proceeds with merge/tag (15–20 minutes)
- **Total time to GA:** ~30–45 minutes from sign-off

---

## 📞 Questions?

Refer to:
- `ai_working/GA_V2_4_0_PROMOTION_RUNBOOK.md` for full execution details
- `RELEASE_STRATEGY.md` §2.3–2.4 for gate model explanation
- `docs/governance/` directory for governance documentation

---

**Ready to proceed?** ✓ Yes, I've reviewed all evidence above.

**Please sign Section 9 when ready:** `docs/governance/GA_PROMOTION_SIGN_OFF.md` (lines 183–206)
