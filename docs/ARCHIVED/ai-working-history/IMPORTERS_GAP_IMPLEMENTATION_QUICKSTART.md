# IMPORTERS Gap Closure Implementation — Quick Start Guide

**Project:** Importers Module Gap Closure (282 gaps, 10-week execution)  
**Start Date:** 2026-08-15  
**Status:** Phase 0 COMPLETE; Phase 1 RUNNING  
**Master Coordination:** `ai_working/IMPORTERS_GAP_CLOSURE_COORDINATION.md`

---

## Phase-by-Phase Quick Links

| Phase | Name | Status | Artifact | Agent |
|-------|------|--------|----------|-------|
| **0** | Framework Setup | ✅ COMPLETE | Master coordination + 6 specs | (Setup) |
| **1** | Triage & Validation | 🟡 RUNNING | IMPORTERS_PHASE1_GAP_TRIAGE.md | `gap-verifier` (importers-phase1-triage) |
| **2** | CRITICAL Fixes | ⏹ QUEUED | IMPORTERS_PHASE2_CRITICAL_FIXES_COMPLETE.md | `themisdb-implementer` |
| **3** | HIGH Batch A1 | ⏹ QUEUED | IMPORTERS_PHASE3_HIGH_BATCH_A1_COMPLETE.md | `themisdb-implementer` |
| **4** | HIGH Batch A2 | ⏹ QUEUED | IMPORTERS_PHASE4_HIGH_BATCH_A2_COMPLETE.md | `themisdb-implementer` |
| **5** | MEDIUM/LOW | ⏹ QUEUED | IMPORTERS_PHASE5_MEDIUM_LOW_COMPLETE.md | `task` + `themisdb-implementer` |
| **6** | Review & Certification | ⏹ QUEUED | IMPORTERS_PHASE6_FINAL_CLOSURE_CERTIFICATE.md | `themisdb-reviewer` |

---

## Gap Distribution Summary

```
Total Gaps: 282

By Severity:
  CRITICAL: 44 gaps (15.6%)  → Phase 2 (100% closure required)
  HIGH:     151 gaps (53.5%) → Phase 3-4 (≥90% closure required)
  MEDIUM:   82 gaps (29.1%) → Phase 5 (≥60% closure required)
  LOW:      5 gaps (1.8%)  → Phase 5 (≥60% closure required)

By Complexity:
  Tier-1 (Simple):    ~40 gaps   → Phase 2 (days 1-2 per batch)
  Tier-2 (Moderate):  ~120 gaps  → Phase 2-4 (days 3-5 per batch)
  Tier-3 (Complex):   ~122 gaps  → Phase 3-5 (weeks 1-2 per batch)

By File (Top 5):
  postgres_importer.cpp:  55 gaps (19.5%)
  mysql_importer.cpp:     38 gaps (13.5%)
  mongo_importer.cpp:     24 gaps (8.5%)
  flatfile_importer.cpp:  22 gaps (7.8%)
  s3_importer.cpp:        16 gaps (5.7%)
  ... + 22 other files
```

---

## Success Criteria (Phase 6 Exit Gate)

- [x] Phase 0: Framework setup complete
- [ ] Phase 1: All 282 gaps classified (>90% confidence)
- [ ] Phase 2: 100% CRITICAL closure (44/44 gaps)
- [ ] Phase 3-4: ≥90% HIGH closure (≥135/151 gaps)
- [ ] Phase 5: ≥60% MEDIUM/LOW closure (≥52/87 gaps)
- [ ] Phase 6: Code review approved, benchmarks stable, documentation synced

**Overall Target:** 60-70% total gap remediation (~170-190 gaps) by 2026-10-15

---

## Key Execution Documents

### Master Coordination
- **File:** `ai_working/IMPORTERS_GAP_CLOSURE_COORDINATION.md`
- **Content:** Phase status dashboard, gap inventory by severity/complexity, blocker table, quality gates, progress tracking protocol
- **Usage:** Central reference for phase status and next steps

### Phase Specifications (Read Before Dispatch)
1. **Phase 1 Triage:** `ai_working/IMPORTERS_PHASE1_TRIAGE_AGENT_SPEC.md`
   - Input: MODULE_GAPS.md (282 gaps)
   - Output: Triage classification matrix, batch proposals
   - Agent: `gap-verifier`

2. **Phase 2 CRITICAL:** `ai_working/IMPORTERS_PHASE2_CRITICAL_AGENT_SPEC.md`
   - Scope: 44 gaps across 11 files (3 batches: A, B, C)
   - Focus: null_dereference, data_race, timeout, smart_ptr, resource_leak
   - Agent: `themisdb-implementer`

3. **Phase 3-4 HIGH:** `ai_working/IMPORTERS_PHASE3_4_HIGH_AGENT_SPECS.md`
   - Phase 3 (Batch A1): postgres (31) + mysql (15) + mongo (12) = 58 gaps
   - Phase 4 (Batch A2): flatfile (10) + s3 (12) + kafka (12) + oracle (8) + sqlite (9) + schema (4) = 55 gaps
   - Agent: `themisdb-implementer` (parallel execution OK)

4. **Phase 5 MEDIUM/LOW:** `ai_working/IMPORTERS_PHASE5_MEDIUM_LOW_AGENT_SPECS.md`
   - Batch M1 (28 items): Data structures (map→unordered_map, vector reserve)
   - Batch M2 (22 items): Algorithms (nested loops→hash, repeated search→cache)
   - Batch M3 (32 items): Documentation and code style
   - Agent: `task` (M1, M3) + `themisdb-implementer` (M2)

5. **Phase 6 Review:** `ai_working/IMPORTERS_PHASE6_REVIEW_AGENT_SPEC.md`
   - Scope: Code review, conformance check, CI/CD validation, final certification
   - Agent: `themisdb-reviewer`

### Weekly Status Tracker
- **Pattern:** `ai_working/IMPORTERS_GAP_WEEKLY_STATUS_W<N>.md`
- **Created:** Friday EOD each week with progress summary, blockers, next week priorities
- **Content:** Gap closure rate, test pass rate, benchmark variance, risk updates

---

## Critical Paths & Dependencies

### Phase Dependencies (Strict Ordering)
```
Phase 1 (Triage)
    ↓ (completion gate)
Phase 2 (CRITICAL)
    ↓ (completion gate)
Phase 3-4-5 (parallel)
    ↓ (any completion)
Phase 6 (Review & Certification)
```

### No Parallelization Before Phase 2 CRITICAL
- Phase 2 must complete 100% before Phase 3-5 start
- Reason: CRITICAL gaps affect shared state and connector pooling

### Full Parallelization Phase 3-5
- Phase 3 (postgres/mysql/mongo) can run simultaneously with Phase 4 (flatfile/s3/kafka/oracle/sqlite/schema)
- Phase 5 (M1/M2/M3) can start after Phase 2 CRITICAL (no dependency on Phase 3-4 completion)

---

## Blocker Mitigation Table

| Blocker | Phase | Mitigation | Status |
|---------|-------|-----------|--------|
| RocksDB missing in Community builds | 2-6 | Feature-gated THEMIS_WIKI_PHASE_B | ✅ RESOLVED |
| Connector availability in CI | 3-4 | Mock/stub connectors in tests | ⏳ TBD |
| Benchmark instability | 2-6 | ±5% regression budget + 5 repetitions | ✅ STANDARD |
| Cross-module state conflicts | 2 | Standardize timeout/mutex semantics | ⏳ P2 Batch A |
| Deferred items impact | 6 | Document reason + future target phase | ⏳ TBD |

---

## Estimated Timeline

| Week | Phase | Milestones | Status |
|------|-------|-----------|--------|
| W1 (Aug 15-22) | 1 | Phase 1 triage complete; all 282 gaps classified | 🟡 IN PROGRESS |
| W2-3 (Aug 22-Sep 5) | 2 | Phase 2 CRITICAL fixes complete (44/44 gaps) | ⏹ QUEUED |
| W4-5 (Sep 5-19) | 3-4 | Phase 3-4 parallel HIGH fixes (≥80% closure) | ⏹ QUEUED |
| W6-8 (Sep 19-Oct 3) | 5 | Phase 5 MEDIUM/LOW fixes (≥60% closure) | ⏹ QUEUED |
| W8-10 (Oct 3-15) | 6 | Code review, CI validation, final certification | ⏹ QUEUED |

---

## Build & Test Commands Quick Reference

**Configure (Community Release with RocksDB fallback):**
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset community-release-allow-missing-rocksdb
```

**Build (Release Profile):**
```bash
cmake --build --preset community-release-allow-missing-rocksdb --parallel 16
```

**Run All Importers Focused Tests:**
```bash
ctest -R "importers.*focused" --output-on-failure -j 4
```

**Run Release Gate Benchmarks:**
```bash
./build/community-release-allow-missing-rocksdb/benchmarks/importers/bench_importers_release_gates \
  --benchmark_min_time=5s \
  --benchmark_out_format=csv
```

**Check for New Warnings:**
```bash
cmake --build --preset community-release-allow-missing-rocksdb 2>&1 | grep -i "warning" | wc -l
# Expected: 0
```

---

## Dispatcher Quick-Start: Launching Next Phase

### When Phase 1 Completes (Expected: 2026-08-22)

1. **Read** `ai_working/IMPORTERS_PHASE1_GAP_TRIAGE.md` (output from Phase 1 agent)
2. **Verify** all 282 gaps classified with >90% confidence
3. **Extract** batch proposals for Phase 2-5
4. **Dispatch Phase 2 Batch A:**
   ```
   task(
     agent_type: "themisdb-implementer",
     description: "Phase 2 CRITICAL Batch A (postgres/mysql)",
     mode: "background",
     name: "importers-phase2-batch-a",
     prompt: "[See IMPORTERS_PHASE2_CRITICAL_AGENT_SPEC.md for detailed prompt]"
   )
   ```
5. **Create** `ai_working/IMPORTERS_GAP_WEEKLY_STATUS_W2.md` with Phase 1 completion summary

### When Phase 2 Completes (Expected: 2026-09-05)

1. **Verify** 100% CRITICAL closure (44/44 gaps)
2. **Dispatch Phase 3 + Phase 4 parallel** (both use themisdb-implementer, different files)
3. **Dispatch Phase 5 Batch M1 + M3 in parallel** (use task agents for speed)
4. **Create** `ai_working/IMPORTERS_GAP_WEEKLY_STATUS_W4.md`

### When Phases 3-5 Complete (Expected: 2026-10-03)

1. **Dispatch Phase 6** (themisdb-reviewer)
2. **Phase 6 produces:** CODE_REVIEW_FINDINGS.md, CONFORMANCE_MATRIX.md, CI_VALIDATION_REPORT.md, FINAL_CLOSURE_CERTIFICATE.md

---

## Communication & Escalation

**Daily Standup:** Commit messages reference gap categories and phase number  
**Weekly Consolidation:** Friday EOD status file  
**Blocker Escalation:** Immediately via issue or comment with 24h notification  
**Phase Completion:** Agent produces artifact; manual sign-off required before next phase

**Status Dashboard Sync:** Update `IMPORTERS_GAP_CLOSURE_COORDINATION.md` after each phase completion

---

## Success Indicators (Go/No-Go Criteria)

✅ **Green Light to Proceed:**
- Phase completion artifact exists and is complete
- Quality gates passed (compilation clean, tests pass, benchmarks stable)
- No blocking findings in code review
- Deferred items documented with justification

🟡 **Caution (Proceed with Mitigation):**
- Minor gaps remain unresolved (documented in artifact)
- Benchmark variance within 5% envelope
- Code review finding severity is LOW (not MEDIUM/CRITICAL)

🔴 **Stop & Remediate:**
- Phase completion artifact missing or incomplete
- >5% benchmark regression
- Compilation errors or new warnings
- Code review finding severity is MEDIUM or CRITICAL (blocks merge)
- >10% focused test failure rate

---

## Memory Notes for Future Sessions

- **Parallelization Rule:** Phases 3-5 can run in parallel, but Phase 2 CRITICAL must complete first (100% closure)
- **User Preference:** Larger remediation batches preferred over micro-fixes (from previous work patterns)
- **Build Preset:** Always use `community-release-allow-missing-rocksdb` for importers testing (RocksDB not required)
- **Release Gates:** IMRG-01..06 are the canonical benchmark gates; tolerance is ±5% variance
- **Module Status:** importers is contributing module in Wave A→B→C→D model; must stay `release_critical`-green throughout

---

## Next Immediate Action

**Status:** Waiting for Phase 1 (gap-verifier agent) to complete  
**Agent:** `importers-phase1-triage`  
**ETA:** 2026-08-22 (end of Week 1)  
**On Completion:** Check `ai_working/IMPORTERS_PHASE1_GAP_TRIAGE.md` and dispatch Phase 2 Batch A
