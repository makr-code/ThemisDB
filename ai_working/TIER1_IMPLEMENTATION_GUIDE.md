# Tier 1 Hardening Implementation Guide (2026-08-02)

**Prepared for:** Part 1 kickoff (Week 1-2, 2026-08-09 → 2026-08-22)  
**Current Date:** 2026-08-02 (7 days before execution starts)  
**Purpose:** Pre-execution validation and readiness checklist

---

## Part 1 Readiness Status

### ✅ Infrastructure Created (Aug 2)

- [x] `ai_working/TIER1_EXECUTION_LOG.md` — weekly progress template
- [x] `ai_working/TIER1_PRIORITY_MATRIX.md` — gap categorization framework
- [x] `src/llm/MODULE_GAPS.md`, `src/server/MODULE_GAPS.md`, `src/sharding/MODULE_GAPS.md` exist
- [x] Git commit: infrastructure baseline pushed

### ⏳ Pre-Execution Tasks (Aug 3-8)

Before Aug 9, complete:

1. **Verify Scanner Setup**
   - [ ] Confirm `tools/scanners/phase_1_4_enhancement_registry.py` can execute on current develop
   - [ ] Test each scanner module individually (S-1, S-2, S-3, M-1/M-2, C-1)
   - [ ] Document expected output format (JSON summary + categorized manifest)

2. **Validate Build Infrastructure**
   - [ ] `windows-release` preset builds successfully on current head
   - [ ] `linux-release` preset builds successfully on current head (requires Ninja + vcpkg)
   - [ ] All focused test targets discoverable (pattern: `module_<module>_test_*_focused`)
   - [ ] `release_critical` CI gate active and passing

3. **Coordinate With Stakeholders**
   - [ ] Notify LLM team: Batch 1 (Memory Safety) kickoff Aug 12
   - [ ] Notify Server team: Block 1 (Concurrency) kickoff Aug 20
   - [ ] Notify Sharding team: Block 2 (Coordination) kickoff Sep 5
   - [ ] Schedule weekly sync for progress updates

### ⏳ Part 1 Execution (Aug 9-22)

#### A. Execute phase_1_4_enhancement_registry.py (Aug 9-11)

**Command:**
```bash
cd /home/runner/work/ThemisDB/ThemisDB
python3 tools/scanners/phase_1_4_enhancement_registry.py
```

**Success Criteria:**
- Registry summary printed to stdout
- Results exported to `phase_1_4_enhancement_results.json`
- Gap counts within ±15% of expected ranges:
  - S-1: 100-160 (or 85-184 with 15% tolerance)
  - S-2: 70-120 (or 60-138)
  - S-3: 80-150 (or 68-173)
  - M-1/M-2: 50-100 (or 43-115)
  - C-1: 40-80 (or 34-92)
  - **Total: 340-610 (or 289-702)**

**Action on Failure:**
- If S-2 scanner fails: Debug regex pattern (noted error: "nothing to repeat at position 35")
- If total gaps < 289 or > 702: Escalate to team lead; adjust Tier 1 targets if necessary

#### B. Phase 1-4 Gap Triage & Prioritization (Aug 12-15)

**Process:**
1. Parse `phase_1_4_enhancement_results.json` into TIER1_PRIORITY_MATRIX.md
2. Classify each gap by:
   - Release-critical (GA blocker)
   - Performance/hardening (quality gate)
   - Documentation (reference)
3. Map to modules and assign batches
4. Update `src/<module>/MODULE_GAPS.md` with gap details

**Deliverable:**
- `ai_working/TIER1_PRIORITY_MATRIX.md` (updated with actual gap data)
- `src/llm/MODULE_GAPS.md` populated with Batch 1 gaps
- `src/server/MODULE_GAPS.md` populated with Block 1 gaps (stubs)
- `src/sharding/MODULE_GAPS.md` populated with Block 2 gaps (stubs)

#### C. Begin LLM Batch 1 — Memory Safety (Aug 12-19)

**Scope:**
- Fix 40-50 M-1/M-2 gaps (RAII, leak detection, cache cleanup)
- Target files: `src/llm/` (50-70 files with M-1/M-2 scanner hits)
- Acceptance: MEM-01..MEM-16 tests pass, 0 new ASan/TSan failures

**Implementation Strategy:**
1. Identify top 10 memory safety gaps (CRITICAL/HIGH severity)
2. For each gap:
   - Create fix (RAII wrapper, leak cleanup, cache synchronization)
   - Add focused test case (MEM-01, MEM-02, etc.)
   - Validate on windows-release + linux-release
   - Prepare PR with test coverage
3. Batch all fixes into single `feature/tier1-llm-batch1` PR

**Parallel Work:**
- This can run in parallel with triage (B) starting Aug 12
- Triage results (B) used to prioritize gap fixes

#### D. Finalize Build & CI Readiness (Aug 20-21)

**Tasks:**
- [ ] Confirm all LLM Batch 1 focused tests build on windows-release
- [ ] Confirm all LLM Batch 1 focused tests build on linux-release
- [ ] Run release_critical CI gate on current develop; verify 100% pass
- [ ] Pre-stage infrastructure for Block 1 + Block 2 parallel work (Aug 22+)

**Success Criteria:**
- All focused test targets: 100% build success
- release_critical CI gate: 0 failures
- No performance regression vs. baseline (≤ 3%)

---

## Part 2 Pre-Planning (Aug 3-15)

While Part 1 executes, prepare Part 2:

### Server Block 1 Prep (Aug 3-15)

- [ ] Identify top 60-80 C-1 (concurrency) gaps in server module
- [ ] Categorize by area: request routing, idempotency cache, auth middleware
- [ ] Sketch fixes for 3-5 highest-severity gaps
- [ ] Draft test plan for SRV-01..SRV-12
- [ ] Prepare PR template for `feature/tier1-server-block1`

### Sharding Block 2 Prep (Aug 3-15)

- [ ] Run TSAN on current develop to identify lock-ordering violations (baseline)
- [ ] Categorize 340+ cross-shard concurrency gaps
- [ ] Identify 70% reduction target: 340 → 102 gaps
- [ ] Sketch fixes for top lock-ordering violations
- [ ] Prepare PR template for `feature/tier1-sharding-block2`

---

## Contingency Plans

### If Scanner Execution Fails (Aug 9-11)

**Scenario:** Registry cannot run or produces zero gaps  
**Fallback:** Use existing MODULE_GAPS.md data + manual module audit

**Steps:**
1. Check for scanner module import errors
2. Manually run S-1/S-2/S-3/M-1/M-2/C-1 scanners individually
3. If all fail: Use existing gap inventory from FINAL_GA_READINESS_CHECKLIST.md
4. Estimate Tier 1 gaps: 82,611 × 0.25 = 20,653 fixes

### If LLM Batch 1 Falls Behind (Aug 12-19)

**Scenario:** Memory safety gaps more complex than expected  
**Fallback:** Reduce Batch 1 scope to top 20-30 gaps (not full 40-50)

**Steps:**
1. Prioritize CRITICAL + HIGH severity gaps only
2. Defer MEDIUM severity to Phase 3a (remaining modules)
3. Ensure MEM-01..MEM-08 tests deliver (reduce to 8 tests if needed)
4. Push Aug 22 sign-off to Aug 25 (3-day slip acceptable)

### If build fails on community-release (Aug 20-21)

**Scenario:** RocksDB missing or fmt package config issue  
**Fallback:** Use windows-release + linux-release presets only

**Steps:**
1. Skip community-release preset for focused test validation
2. Document RocksDB/fmt installation requirement for community builds
3. Defer community-release fix to post-GA (v2.4.1)
4. Maintain vcpkg-based linux-release as primary for Tier 1 work

---

## Success Metrics & KPIs

### Part 1 (Week 1-2) Gate

**PASS Criteria:**
- [ ] Registry execution: ±15% variance from expected gaps
- [ ] 100% gap categorization (no "unclear" status)
- [ ] LLM Batch 1: MEM-01..MEM-16 tests pass 100%
- [ ] release_critical CI: 0 regressions
- [ ] 0 new ASan/TSan failures on windows-release + linux-release

**FAIL Criteria (Escalate):**
- [ ] Scanner execution crashes or times out
- [ ] Gap count variance > ±15% (requires Tier 1 adjustment)
- [ ] LLM Batch 1 test failures > 10%
- [ ] release_critical CI regression
- [ ] New ASan/TSan failures

### Part 2 (Week 3-8) Gate

**PASS Criteria (for each block/batch):**
- [ ] Focused test targets: 100% pass on windows-release + linux-release
- [ ] release_critical CI: 0 regressions (gate must remain green)
- [ ] Gap fixes merged: PR count ≥ expected per block (e.g., ≥ 20 PRs for Server Block 1)
- [ ] Performance regression ≤ 3% on benchmarks

**FAIL Criteria:**
- [ ] Focused test failures > 5%
- [ ] release_critical CI regression
- [ ] Performance regression > 3%
- [ ] Schedule slip > 1 week per block

---

## Communication Plan

### Weekly (Mondays 9 AM UTC)

Email stakeholders with:
- Gap fixes merged (count by module)
- Blockers / escalations
- Upcoming week focus

### Biweekly (Fridays)

Sync call:
- Phase summary (gap reduction %, risk status)
- Blockers requiring leadership decision
- Upcoming 2-week forecast

### End-of-Phase (Aug 22, Oct 1, Nov 30)

Decision point:
- GO / DEFER assessment
- GA readiness snapshot
- Next phase kickoff or hold

---

## References

- **ROADMAP.md:** Root roadmap (Tier 1 target, execution batches)
- **FINAL_GA_READINESS_CHECKLIST.md:** Current GA status (82,611 gap baseline)
- **src/<module>/ROADMAP.md:** Module-specific phase status
- **src/<module>/MODULE_GAPS.md:** Gap inventory (updated incrementally)
- **.github/workflows/09-pr-gates_release-critical-tests.yml:** CI gate definition
- **benchmarks/MEASUREMENT_HYGIENE.md:** Performance gate rules

---

## Appendix: Scanner Module Details

The phase_1_4_enhancement_registry.py integrates these scanners:

| Scanner | Module | CWE | Description | Expected Gaps |
|---------|--------|-----|-------------|---------------|
| S-1 | gs3_step01_enhance_security_s1 | CWE-798 | Hardcoded Secrets | 100-160 |
| S-2 | gs3_step01_enhance_security_s2 | CWE-327 | Cryptographic Weaknesses | 70-120 |
| S-3 | gs3_step01_enhance_security_s3 | CWE-94 | Injection Attacks | 80-150 |
| M-1/M-2 | gs3_step02_enhance_memory_m1_m2 | CWE-416/415 | Memory Safety | 50-100 |
| C-1 | gs3_step01_enhance_concurrency_c1 | CWE-362 | Race Conditions | 40-80 |

Each scanner outputs gaps as a list of `{file, line, issue, severity, gap_id}` tuples.

---

**Prepared by:** Copilot  
**Prepared on:** 2026-08-02  
**Next Review:** 2026-08-09 (scanner execution results)
