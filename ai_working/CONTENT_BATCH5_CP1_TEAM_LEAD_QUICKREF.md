# Content Batch 5 CP-1 Remediation — Team Lead Quick Reference

**Date:** 2026-08-15 15:24 UTC  
**Scope:** 5-day parallel remediation (2026-08-16 to 2026-08-20)  
**Gate:** CP-1 re-review 2026-08-22 13:58 UTC

---

## For Team X (CRITICAL-1: Dangling Pointers)

### Your Assignment
Fix dangling pointers in `ContentTypeRegistry` (3 methods + 8-12 callers)

### Tasks (Sequential, with Testing)
1. **CMT-FIX-01** (Day 1): `getByMimeType()` → `std::optional<ContentType>`
2. **CMT-FIX-02** (Day 2): `getByExtension()` → `std::optional<ContentType>`
3. **CMT-FIX-03** (Day 3): `detectFromBlob()` → `std::optional<ContentType>`
4. **CMT-FIX-04** (Day 4): Update 8–12 caller sites for optional handling
5. **CMT-FIX-05** (Day 5): Add tests (CMT-FIN-36..40)

### Key File
- `src/content/content_type.cpp` (lines 128, 149, 156–180)
- `src/content/content_type.hpp` (method signatures)

### Deliverable
- Updated methods returning `std::optional<ContentType>`
- All callers handle optional return type
- 5 new tests (pointer safety, RAII, optional semantics)
- Test output: 100% PASS

### Success Criteria
✅ No dangling pointer references  
✅ `std::optional` semantics correct in all callers  
✅ Tests pass + clang-tidy 0 warnings  
✅ Code review approval (memory safety)

### Tracking
- Daily update: `CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md` (Day 1–5)
- SQL status: `SELECT * FROM content_batch5_remediation WHERE task_id LIKE 'CMT-FIX-%'`
- Escalate blockers to Content Module Lead immediately

---

## For Teams A, B, C (HIGH-1: Doxygen Headers)

### Your Assignments

| Team | Batch | Files | Target | Lead |
|------|-------|-------|--------|------|
| **A** | **BATCH-A** | 15 core processors | 2026-08-17 | TBD |
| **B** | **BATCH-B** | 14 medium processors | 2026-08-18 | TBD |
| **C** | **BATCH-C** | 15 specialized processors | 2026-08-19 | TBD |

### Task: Add/Complete Doxygen Headers
Apply template to all 44 content processor files:

```cpp
/**
 * @file <filename>
 * @brief <One-line class/component description>.
 * @version <SEMVER>
 * @note Maturity: 🟢 PRODUCTION-READY | 🟡 BETA | 🔴 ALPHA
 * @note Score: <N>/100
 * @note Gap Summary: total=<N>; TODO=<N>, Stub=<N>, Unimpl=<N>, Mock=<N>, Sim=<N>, Debt=<N>, C=<N>, H=<N>, M=<N>, L=<N>
 * @note Status: <Production Ready | Beta | Alpha>
 * @note This block is auto-generated and will be overwritten.
 */
```

### File Lists

**Team A (Batch-A: 15 core processors)**
- abuse_detector.cpp
- audio_processor.cpp
- content_manager.cpp
- (+ 12 more core/high-maturity files)

**Team B (Batch-B: 14 medium-maturity processors)**
- mime_detector.cpp
- office_processor.cpp
- geo_processor.cpp
- (+ 11 more medium-maturity files)

**Team C (Batch-C: 15 specialized/lower-priority)**
- mock_clip_processor.cpp
- content_policy.cpp
- (+ 13 more specialized/alpha files)

### Validation
**CMT-HDR-VALIDATE** (Day 5, all teams):
```bash
doxygen Doxyfile.audit
# Target: 0 warnings in content section

clang-tidy src/content/*.cpp
# Target: 0 new warnings
```

### Deliverable Per Batch
- All files in batch: Doxygen headers complete + @note metadata
- CSV export: `filename | maturity | score | gap_summary | status`
- Doxygen audit output: 0 warnings
- Batch completion report

### Success Criteria
✅ 47/47 files have Doxygen headers (100%)  
✅ 47/47 files have @note Gap Summary metadata  
✅ Doxygen audit: 0 warnings  
✅ Template compliance: 100%  

### Tracking
- Daily update: `CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md` (Day 1–5)
- SQL status: `SELECT * FROM content_batch5_remediation WHERE task_id LIKE 'CMT-HDR-BATCH-%'`
- Escalate blockers to Content Module Lead immediately

---

## For Audit Team (HIGH-2: TODO Audit)

### Your Assignment
Reconcile TODO count discrepancy: 73 (gap scan) vs. 13 (ripgrep)

### Tasks (Concurrent with Doxygen Batches)
1. **CMT-TODO-AUDIT-01** (Day 2): Verify gap scan metadata
   - Tool version, configuration, branch used
   - Identify if scan was run on correct branch/commit

2. **CMT-TODO-AUDIT-02** (Day 3): Cross-reference Batch 1–4 history
   - Review `src/content/` commits in Batches 1–4
   - Identify commits that removed TODOs
   - Estimate TODOs addressed

3. **CMT-TODO-AUDIT-03** (Day 4): Manual ripgrep TODO scan
   ```bash
   rg "TODO" src/content/*.cpp --line-number
   # Classify by type: Optimization, Feature, Vendor, Other
   ```

4. **CMT-TODO-AUDIT-04** (Day 5): Create CONTENT_DEFERRED_FEATURES.md
   - Document all found TODOs (13 + discovered)
   - Link to GitHub issues where applicable
   - Update FUTURE_ENHANCEMENTS.md cross-references

### Reconciliation Hypothesis
- Gap scan reported 73 TODOs (older, pre-Batch-1-4)
- Batches 1–4 removed ~60 TODOs
- Current ripgrep: 13 TODOs (accurate count)
- **Validation:** Confirm via commit history analysis

### Deliverable
- Gap scan metadata audit report
- Batch 1–4 history analysis (commits + TODO removals)
- Ripgrep scan results + classification CSV
- Reconciliation summary (explaining 73 vs. 13)
- CONTENT_DEFERRED_FEATURES.md (complete documentation)

### Success Criteria
✅ TODO discrepancy explained  
✅ Gap scan accuracy validated  
✅ All 13 (or actual) TODOs classified + documented  
✅ Cross-refs to GitHub issues verified  
✅ FUTURE_ENHANCEMENTS.md synced  

### Tracking
- Daily update: `CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md` (Day 2–5)
- SQL status: `SELECT * FROM content_batch5_remediation WHERE task_id LIKE 'CMT-TODO-AUDIT-%'`
- Escalate blockers to Content Module Lead immediately

---

## Daily Standup Template

**EOD each day (2026-08-16 to 2026-08-20):**

```markdown
## Day N (2026-08-XX) — <Team Name>

### Tasks Completed Today
- [x] <Task ID>: <brief description + result>

### Tasks In Progress
- [ ] <Task ID>: <brief description + estimated completion>

### Blockers or Issues
- <Issue description> → Mitigation: <action>

### Tomorrow's Plan
- <Task ID>: <action> (target: 2026-08-XX)

### Metrics
- Completion: N/M tasks done
- Test Pass Rate: X/Y (if applicable)
- Code Review: [PENDING | IN_PROGRESS | APPROVED]
```

**Where to post:**
- Update `CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md` (public)
- Update SQL: `UPDATE content_batch5_remediation SET status='in_progress|completed', subtasks_complete=N WHERE task_id='CMT-...'`

---

## SQL Query Reference

**View all remediation tasks:**
```sql
SELECT task_id, description, team_assigned, status, subtasks_complete || '/' || subtasks_total as progress
FROM content_batch5_remediation
ORDER BY start_date, task_id;
```

**View today's tasks:**
```sql
SELECT task_id, description, status, subtasks_complete || '/' || subtasks_total as progress
FROM content_batch5_remediation
WHERE start_date <= date('now') AND target_date >= date('now')
ORDER BY target_date;
```

**Mark task complete:**
```sql
UPDATE content_batch5_remediation 
SET status='completed', completion_date=datetime('now'), subtasks_complete=subtasks_total
WHERE task_id='CMT-FIX-01';
```

---

## Evidence Bundle Checklist (2026-08-21 EOD)

**All teams contribute to evidence bundle by 2026-08-21 EOD.**

### Team X (CRITICAL-1)
- [ ] Source code diffs (before/after pointers)
- [ ] Test output logs (CMT-FIX-05)
- [ ] Memory safety validation (AddressSanitizer if available)
- [ ] Caller updates summary (8–12 sites)

### Teams A, B, C (HIGH-1)
- [ ] Batch completion reports (Batch A, B, C)
- [ ] File lists (all 44 files + status per file)
- [ ] Doxygen audit output (0 warnings)
- [ ] Template compliance verification

### Audit Team (HIGH-2)
- [ ] Gap scan metadata audit report
- [ ] Batch 1–4 history analysis
- [ ] Ripgrep scan results + CSV
- [ ] CONTENT_DEFERRED_FEATURES.md

**Bundle Location:** `/ai_working/CP1_EVIDENCE_BUNDLE_2026_08_22/`

---

## Critical Dates & Deadlines

| Date | Milestone | Owner |
|------|-----------|-------|
| 2026-08-16 | Day 1 kickoff (all teams) | All |
| 2026-08-17 | CMT-FIX-01 + Batch-A complete | Team X, Team A |
| 2026-08-18 | CMT-FIX-02 + Batch-B complete | Team X, Team B |
| 2026-08-19 | CMT-FIX-03/04 + Batch-C complete | Team X, Team C |
| 2026-08-20 | All tasks complete + validation | All |
| **2026-08-21 EOD** | **Evidence bundle ready** | All |
| **2026-08-22 13:58 UTC** | **CP-1 Re-Review Gate** | Content Module Lead |

---

## Escalation Path

**Immediate Blocker?** Contact Content Module Lead within 2 hours.

**Typical Blockers:**
- Build/compile errors on modified files
- External dependency changes
- Merge conflicts on shared files
- Test failures in unrelated code (regression investigation)

**Content Module Lead Actions:**
- Assess blocker severity
- Determine if task can continue in parallel
- Reallocate team capacity if needed
- Authorize mitigation (e.g., skip non-critical validation)
- Update timeline assessment (GREEN / YELLOW / RED)

---

## Key References

- **Blocker Details:** `/ai_working/CONTENT_BATCH5_CP1_BLOCKER_REMEDIATION.md`
- **Coordination Master:** `/ai_working/CONTENT_BATCH5_CP1_REMEDIATION_COORDINATION.md`
- **Daily Tracking:** `/ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md`
- **Gap Remediation:** `/ai_working/CONTENT_BATCH5_CP1_BLOCKER_REMEDIATION.md` (Section: Blocker Details & Remediation Plans)

---

**Status:** 🟢 **READY FOR KICKOFF**  
**Next Step:** Confirm team assignments + capacity with Content Module Lead by 2026-08-16 08:00 UTC
