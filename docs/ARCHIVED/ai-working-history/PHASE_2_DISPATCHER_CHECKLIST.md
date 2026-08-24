# Phase 2 Dispatcher — CRITICAL Fixes Agent Launch Checklist

**Status:** Ready for Dispatch (2026-08-22)  
**Agent Type:** themisdb-implementer  
**Scope:** 41 CRITICAL findings (or 25 real gaps from Phase 1)  
**Timeline:** Aug 22 – Sep 5, 2026  

---

## Pre-Dispatch Verification

### Phase 1 Deliverables Verified ✅
- [x] INGESTION_PHASE_1_VERIFICATION_REPORT.json (16 KB) — JSON database
- [x] gap_verifier_report_ingestion.md (9.2 KB) — Executive summary
- [x] PHASE_1_VERIFICATION_INDEX.md — Quick reference
- [x] All 178 findings categorized (100% coverage)
- [x] False positives isolated (114 findings = 64%)
- [x] Quality gates passed (6/6)

### Coordination Framework Verified ✅
- [x] INGESTION_PHASE_2_AGENT_SPECS.md ready (12.2 KB)
- [x] INGESTION_GAP_CLOSURE_COORDINATION.md (master plan)
- [x] INGESTION_KICKOFF_SUMMARY.md (overview)
- [x] Test naming convention defined (INGESTION-<CAT>-<NUM>)
- [x] Quality gates specified (clang-format, clang-tidy, ThreadSanitizer)

### Repository Readiness ✅
- [x] src/ingestion/MODULE_GAPS.md available (source data)
- [x] src/ingestion/ROADMAP.md current
- [x] tests/ingestion/ test suite accessible
- [x] cmake --preset community-release available
- [x] release_critical CI gate operational
- [x] Build infrastructure validated

---

## Agent Dispatch Parameters

```
Agent Configuration:
  Type: themisdb-implementer
  Name: ingestion-phase2-critical-fixes
  Focus: CRITICAL severity findings (41 total, 25 real gaps priority)
  Mode: sync (or background with daily progress reports)
  
Input Package:
  - INGESTION_PHASE_1_VERIFICATION_REPORT.json (verified tier 1+2 findings)
  - gap_verifier_report_ingestion.md (analysis + false positive rationale)
  - INGESTION_PHASE_2_AGENT_SPECS.md (implementation patterns)
  - INGESTION_GAP_CLOSURE_COORDINATION.md (context)
  
Expected Output:
  - 41+ fixed source files (or 25 if prioritizing real gaps)
  - 41+ test cases (INGESTION-<CAT>-<NUM> format)
  - INGESTION_PHASE_2_COMPLETION_SUMMARY.md
  - Commits to develop with message pattern: "INGESTION Phase 2: Fix <category>-<number>"
```

---

## Critical Findings Priority

### Tier 1: CRITICAL-BLOCKING (25 findings) — FIX FIRST
**Real unimplemented gaps without defensive guards**

Files with critical gaps:
1. base_entity_assembler_step.cpp (2 gaps)
2. chunk_embed_step.cpp (1 gap)
3. chunk_text_step.cpp (1 gap)
4. decompress_step.cpp (1 gap)
5. workflow_engine.cpp (1 gap)
... + 19 other gaps distributed across module

**Fix Pattern:** Implement core functionality (not stubs)

### Tier 2: CRITICAL-HIGH (41 per coordination, 24 guarded stubs)
**Real gaps + defensive patterns that need hardening**

**Overlap:** 24 of the 41 are guarded stubs (if-guard defensive)
**Action:** Fix the 17 non-guarded CRITICAL findings first

**Fix Pattern:** Wrap in RAII, add timeout params, synchronize shared data

---

## Execution Checklist

### Pre-Fix Validation
- [ ] Read INGESTION_PHASE_2_AGENT_SPECS.md completely
- [ ] Understand 6 fix categories (timeout, data-race, resource-leak, etc.)
- [ ] Review test naming convention (INGESTION-xxx-yyy-variant)
- [ ] Verify build environment (cmake --preset community-release works)
- [ ] Run baseline test suite (all ingestion tests pass)

### Per-Fix Workflow (Repeat for each finding)
1. [ ] Understand the gap (read source file + gap description)
2. [ ] Implement fix using appropriate pattern from AGENT_SPECS.md
3. [ ] Write 1+ focused test case (INGESTION-xxx pattern)
4. [ ] Run clang-format, clang-tidy on modified file
5. [ ] Run test suite: `ctest --preset community-release -L ingestion`
6. [ ] If data-race fix: Run with ThreadSanitizer, verify 0 warnings
7. [ ] If resource-leak fix: Run with AddressSanitizer, verify 0 leaks
8. [ ] Commit with message: `INGESTION Phase 2: Fix <category>-<number> — <description>`

### Daily Checkpoints (Every 2-3 fixes)
- [ ] Verify `release_critical` CI gate still green
- [ ] Run full test suite (no regressions)
- [ ] Check benchmark baseline (no performance degradation)
- [ ] Call engine-tools-report_progress with batch progress
- [ ] Commit to develop (don't let too many uncommitted changes accumulate)

### End-of-Phase Validation
- [ ] All 41 CRITICAL findings fixed (or 25 if phased approach)
- [ ] All fixes have test cases (41+ tests minimum)
- [ ] All test cases pass on community-release preset
- [ ] clang-format 100% compliant
- [ ] clang-tidy 0 warnings on fixed files
- [ ] ThreadSanitizer 0 warnings (data-race fixes)
- [ ] AddressSanitizer 0 leaks (resource-leak fixes)
- [ ] Benchmarks stable (±5% variance max)
- [ ] release_critical CI gate green

### Deliverable Validation
- [ ] Create INGESTION_PHASE_2_COMPLETION_SUMMARY.md
- [ ] Document per-fix status (resolved / deferred / escalated)
- [ ] List all test cases added (41+)
- [ ] Report any issues found during implementation
- [ ] Merge all commits to develop
- [ ] Final engine-tools-report_progress call with completion status

---

## Risk Flags & Escalation

### If Timeout Findings Require Architecture Review
- Flag in PR comment with links to:
  - INGESTION_GAP_CLOSURE_COORDINATION.md § Risk Mitigation
  - Specific timeout issue location in source
- Defer to Phase 2 checkpoint review if too complex
- Do NOT force fix if architectural change required

### If Data-Race Detected
- Require ThreadSanitizer green before merge
- Add additional synchronization if needed
- Document lock hierarchy in PR comments

### If Resource Leak Found
- Require AddressSanitizer green before merge
- Test both success and exception paths
- Verify RAII wrapping is complete

### If CI Gate Breaks
- Revert immediately (do not force)
- Investigate root cause
- Fix and re-test before retry
- Report via engine-tools-report_progress

### If >1 Fix Per Day Becomes Blocked
- Escalate to project lead
- Document blocker with links
- May require Phase 2 rescope or contingency

---

## Quality Gate Checklist (MUST PASS)

Final submission must pass ALL of these:
- [ ] Clang-format compliant (0 formatting violations)
- [ ] Clang-tidy green (0 warnings on fixed lines)
- [ ] Test coverage ≥95% (41 fixes × 1+ test each = ≥41 tests)
- [ ] ThreadSanitizer green (data-race fixes = 0 warnings)
- [ ] AddressSanitizer green (resource-leak fixes = 0 leaks)
- [ ] All tests pass: `ctest --preset community-release -L ingestion`
- [ ] Benchmarks stable (no >5% regression in key metrics)
- [ ] release_critical CI gate green on develop
- [ ] No new compiler warnings
- [ ] All GitHub issue references valid

---

## Coordination Handoff

Upon Phase 2 completion:
1. [ ] Push all commits to develop
2. [ ] Verify release_critical CI gate is green
3. [ ] Generate INGESTION_PHASE_2_COMPLETION_SUMMARY.md
4. [ ] Call engine-tools-report_progress with Phase 2 complete status
5. [ ] Merge checklist items into main coordination doc

Next phase (Phase 3 — HIGH fixes) will receive:
- INGESTION_PHASE_2_COMPLETION_SUMMARY.md (what was fixed)
- All merged commits on develop (the actual fixes)
- Confidence that Phase 2 tests are solid foundation for Phase 3

---

## Support References

**Questions?** Review these in order:
1. INGESTION_PHASE_2_AGENT_SPECS.md (specific patterns for each category)
2. INGESTION_GAP_CLOSURE_COORDINATION.md § Risk Mitigation (escalation guidance)
3. INGESTION_KICKOFF_SUMMARY.md (timeline + context)
4. README_INGESTION_COORDINATION.txt (quick reference)

**Blocker?** Escalate via:
- PR comment referencing specific issue
- Mark as "blocked pending architecture review" with justification
- Report via engine-tools-report_progress (BLOCKER section)
- Contact project lead

---

## Success Criteria

Phase 2 is **COMPLETE** when:
1. ✅ All 41 CRITICAL findings have fixes (or 25 if prioritized)
2. ✅ All 41+ test cases pass on community-release preset
3. ✅ All quality gates passed (clang-format, clang-tidy, TSAN, ASAN)
4. ✅ All commits merged to develop
5. ✅ release_critical CI gate green
6. ✅ INGESTION_PHASE_2_COMPLETION_SUMMARY.md generated
7. ✅ No regressions in benchmarks (±5% variance max)

**Timeline:** Aug 22 – Sep 5, 2026  
**Status:** 🚀 READY FOR DISPATCH

---

**Dispatcher:** Ready to launch Phase 2 agent  
**Next Phase:** Phase 3 (HIGH fixes) dispatch — ~2026-08-29  
**GA Milestone:** v2.4.0 GA release — Week of Sept 22, 2026  

