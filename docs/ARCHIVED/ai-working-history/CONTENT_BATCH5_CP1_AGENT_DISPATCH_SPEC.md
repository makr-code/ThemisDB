# Content Batch 5 CP-1 — Agent Dispatch Specification

**Date:** 2026-08-15 15:24 UTC  
**Status:** 🟢 **READY FOR DISPATCH** (awaiting team assignments)  
**Target Dispatch:** 2026-08-16 08:00 UTC (after team confirmation)

---

## Overview

This document specifies how to dispatch three parallel background agents to execute the CP-1 remediation workstreams.

**Parallel Execution Model:**
- Agent 1: `content-batch5-critical1-dangling-pointers` (CRITICAL-1 fixes)
- Agent 2: `content-batch5-high1-doxygen-headers` (HIGH-1 header work)
- Agent 3: `content-batch5-high2-todo-audit` (HIGH-2 audit)

**Coordination:**
- Agents run **independently** with **daily synchronization** via:
  - SQL table updates: `content_batch5_remediation`
  - Daily tracking markdown: `CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md`
  - Blocker escalation to Content Module Lead

---

## Agent 1: CRITICAL-1 Dangling Pointers

### Launch Command (Pseudo-syntax)

```
task --agent themisdb-implementer \
  --name "content-batch5-critical1-dangling-pointers" \
  --mode background \
  --context_tier long_context \
  --prompt """
  ## Task: Fix Dangling Pointers in ContentTypeRegistry

  **Assigned Team:** [Team X Name]
  **Duration:** 5 days (2026-08-16 to 2026-08-20)
  **Daily Status Updates:** ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md

  ### Deliverable
  Fix dangling pointers in src/content/content_type.cpp + all callers.
  Return type: std::optional<ContentType>

  ### Tasks (Sequential)
  1. CMT-FIX-01 (Day 1): Fix getByMimeType()
  2. CMT-FIX-02 (Day 2): Fix getByExtension()
  3. CMT-FIX-03 (Day 3): Fix detectFromBlob()
  4. CMT-FIX-04 (Day 4): Update 8-12 callers
  5. CMT-FIX-05 (Day 5): Add tests (CMT-FIN-36..40)

  ### Acceptance Criteria
  - All dangling pointer references eliminated
  - std::optional semantics correct in all callers
  - Tests CMT-FIX-05 pass 100% (pointer safety, RAII, optional)
  - clang-tidy: 0 warnings on modified files
  - Code review approval (memory safety)

  ### Key Files
  - src/content/content_type.cpp (lines 128, 149, 156-180)
  - src/content/content_type.hpp (method signatures)

  ### Daily Tracking
  Update SQL: UPDATE content_batch5_remediation SET status='completed', 
             completion_date=datetime('now') WHERE task_id='CMT-FIX-01' (after each task)

  Update markdown: ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md 
                   (EOD each day: Day 1-5)

  ### Progress Template
  - Day 1 EOD: CMT-FIX-01 complete | test output attached
  - Day 2 EOD: CMT-FIX-02 complete | test output attached
  - Day 3 EOD: CMT-FIX-03 complete | test output attached
  - Day 4 EOD: CMT-FIX-04 complete | caller updates summary
  - Day 5 EOD: CMT-FIX-05 complete | test output (100% PASS)

  ### Additional Guidance
  - Follow RAII principles (no raw new/delete)
  - Prefer std::optional over raw pointers
  - Test edge cases: empty optional, multiple return sites
  - Reference: ai_working/CONTENT_BATCH5_CP1_BLOCKER_REMEDIATION.md (CRITICAL-1 section)

  ### Blocker Escalation
  If blocked, update ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md 
  immediately with blocker description + recommend mitigation to Content Module Lead.
  """
```

### Agent Spec Details

| Parameter | Value |
|-----------|-------|
| **Agent Type** | themisdb-implementer |
| **Mode** | background |
| **Context Tier** | long_context |
| **Model Preference** | deepseek-coder-v2:16b (if Ollama available) |
| **Effort** | high |
| **Timeout** | 120 seconds per turn (daily updates) |

### Expected Output (Daily)

```
## Day N (2026-08-XX) — Agent Report

### CMT-FIX-XX: [COMPLETED | IN_PROGRESS | BLOCKED]
- Code changes: [description + file paths]
- Test results: [pass/fail, count]
- Memory safety: [RAII verified, optional semantics correct]
- Blockers: [if any]
- Next: [action for following day]

### Metrics
- Completion: N% | Tests: M/N passed | Code review: [status]
```

---

## Agent 2: HIGH-1 Doxygen Headers

### Launch Command (Pseudo-syntax)

```
task --agent themisdb-implementer \
  --name "content-batch5-high1-doxygen-headers" \
  --mode background \
  --context_tier long_context \
  --prompt """
  ## Task: Add/Complete Doxygen Headers for 44 Content Processors

  **Assigned Teams:** Teams A (Batch-A), B (Batch-B), C (Batch-C)
  **Duration:** 5 days (2026-08-16 to 2026-08-20, parallel batches)
  **Daily Status Updates:** ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md

  ### Deliverable
  Add/complete Doxygen headers for all 44 content processor files.
  Apply required template to each file.

  ### Required Template
  ```cpp
  /**
   * @file <filename>
   * @brief <One-line class/component description>.
   * @version <SEMVER matching module version>
   * @note Maturity: 🟢 PRODUCTION-READY | 🟡 BETA | 🔴 ALPHA
   * @note Score: <N>/100 (implementation completeness + test coverage)
   * @note Gap Summary: total=<N>; TODO=<N>, Stub=<N>, Unimpl=<N>, Mock=<N>, Sim=<N>, Debt=<N>, C=<N>, H=<N>, M=<N>, L=<N>
   * @note Status: <Production Ready | Beta | Alpha>
   * @note This block is auto-generated and will be overwritten.
   */
  ```

  ### Tasks (Parallel Batches)
  - Batch-A (Day 1-2): 15 core processors
  - Batch-B (Day 2-3): 14 medium-maturity processors
  - Batch-C (Day 3-4): 15 specialized/lower-priority processors
  - Validate (Day 5): doxygen Doxyfile.audit + clang-tidy

  ### Acceptance Criteria
  - 47/47 files have Doxygen headers (100% compliance)
  - 47/47 files have @note Gap Summary metadata
  - Maturity distribution: PRODUCTION-READY (40), BETA (6), ALPHA (1)
  - Doxygen audit: 0 warnings in content section
  - clang-tidy: 0 new warnings
  - Template compliance verified

  ### Daily Tracking
  Update SQL: UPDATE content_batch5_remediation 
             SET subtasks_complete=N, status='in_progress|completed' 
             WHERE task_id='CMT-HDR-BATCH-A|B|C' (daily)

  Update markdown: ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md 
                   (EOD each day: Day 1-5)

  ### File Lists
  [Insert Batch A, B, C file lists from CONTENT_BATCH5_CP1_TEAM_LEAD_QUICKREF.md]

  ### Progress Template
  - Day 1 EOD: Batch-A progress (N/15 files complete) | any blockers
  - Day 2 EOD: Batch-A complete, Batch-B started (N/14 files complete)
  - Day 3 EOD: Batch-B complete, Batch-C started (N/15 files complete)
  - Day 4 EOD: Batch-C complete | validation ready
  - Day 5 EOD: doxygen audit output | clang-tidy output | 0 warnings confirmed

  ### Output Artifacts
  - Batch completion reports (per batch)
  - CSV: content_maturity_spreadsheet.csv (47 files, all fields)
  - Doxygen audit log (0 warnings)
  - Template compliance verification report

  ### Reference
  ai_working/CONTENT_BATCH5_CP1_BLOCKER_REMEDIATION.md (HIGH-1 section)
  ai_working/CONTENT_BATCH5_CP1_TEAM_LEAD_QUICKREF.md (Team A/B/C sections)
  """
```

### Agent Spec Details

| Parameter | Value |
|-----------|-------|
| **Agent Type** | themisdb-implementer |
| **Mode** | background |
| **Context Tier** | long_context |
| **Model Preference** | qwen2.5-coder:14b (tool-use for batch edits) |
| **Effort** | high |
| **Timeout** | 120 seconds per turn (daily updates) |

### Expected Output (Daily)

```
## Day N (2026-08-XX) — Batch X Report

### CMT-HDR-BATCH-X: [COMPLETED | IN_PROGRESS | BLOCKED]
- Files complete: M/N
- Template compliance: X/N files verified
- Blockers: [if any]
- Sample file review: [one file + header]
- Next: [action for following batch]

### Metrics
- Completion: N% | Files: M/N | Template: X/N verified
```

---

## Agent 3: HIGH-2 TODO Audit

### Launch Command (Pseudo-syntax)

```
task --agent gap-verifier \
  --name "content-batch5-high2-todo-audit" \
  --mode background \
  --context_tier long_context \
  --prompt """
  ## Task: TODO Count Reconciliation & Audit

  **Assigned Team:** Audit Team
  **Duration:** 4 days (2026-08-17 to 2026-08-20, concurrent with Doxygen)
  **Daily Status Updates:** ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md

  ### Deliverable
  Reconcile TODO count discrepancy: 73 (gap scan) vs. 13 (ripgrep).
  Create CONTENT_DEFERRED_FEATURES.md with all TODO documentation.

  ### Tasks (Sequential)
  1. CMT-TODO-AUDIT-01 (Day 2): Verify gap scan metadata
  2. CMT-TODO-AUDIT-02 (Day 3): Cross-reference Batch 1-4 history
  3. CMT-TODO-AUDIT-03 (Day 4): Manual ripgrep TODO scan
  4. CMT-TODO-AUDIT-04 (Day 5): Create CONTENT_DEFERRED_FEATURES.md

  ### Reconciliation Hypothesis
  - Gap scan reported 73 TODOs (pre-Batch-1-4)
  - Batches 1-4 removed ~60 TODOs
  - Current ripgrep: 13 TODOs (accurate)
  - Validation: Confirm via commit history

  ### Acceptance Criteria
  - TODO discrepancy explained (73 vs. 13 reconciliation)
  - Gap scan accuracy validated or tool recalibrated
  - All 13 (or actual) TODOs classified + documented
  - CONTENT_DEFERRED_FEATURES.md complete
  - GitHub issue cross-refs verified
  - FUTURE_ENHANCEMENTS.md synced

  ### Daily Tracking
  Update SQL: UPDATE content_batch5_remediation 
             SET status='in_progress|completed' 
             WHERE task_id='CMT-TODO-AUDIT-XX' (each day)

  Update markdown: ai_working/CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md 
                   (EOD each day: Day 2-5)

  ### Progress Template
  - Day 2 EOD: CMT-TODO-AUDIT-01 complete | gap scan metadata findings
  - Day 3 EOD: CMT-TODO-AUDIT-02 complete | Batch 1-4 history analysis
  - Day 4 EOD: CMT-TODO-AUDIT-03 complete | ripgrep scan results + classification
  - Day 5 EOD: CMT-TODO-AUDIT-04 complete | CONTENT_DEFERRED_FEATURES.md + cross-refs

  ### Output Artifacts
  - Gap scan metadata audit report
  - Batch 1-4 history analysis (commits + TODO removals)
  - Ripgrep scan results (13 TODOs + CSV classification)
  - Reconciliation summary (explaining 73 vs. 13)
  - CONTENT_DEFERRED_FEATURES.md (complete documentation)

  ### Reference
  ai_working/CONTENT_BATCH5_CP1_BLOCKER_REMEDIATION.md (HIGH-2 section)
  ai_working/CONTENT_BATCH5_CP1_TEAM_LEAD_QUICKREF.md (Audit Team section)
  """
```

### Agent Spec Details

| Parameter | Value |
|-----------|-------|
| **Agent Type** | gap-verifier |
| **Mode** | background |
| **Context Tier** | long_context |
| **Model Preference** | claude-opus-4.8 (analysis + reconciliation) |
| **Effort** | high |
| **Timeout** | 120 seconds per turn (daily updates) |

### Expected Output (Daily)

```
## Day N (2026-08-XX) — Audit Report

### CMT-TODO-AUDIT-XX: [COMPLETED | IN_PROGRESS | BLOCKED]
- Findings: [brief summary of audit step findings]
- Data: [count, results, or analysis output]
- Blockers: [if any]
- Next: [action for following day]

### Metrics
- Discrepancy status: 73 vs. 13 [explained | pending]
- TODOs classified: N items
```

---

## Dispatch Workflow (Content Module Lead)

### Step 1: Confirm Team Assignments (2026-08-15 16:00–20:00 UTC)

**Action:** Assign team leads to:
- Team X (CRITICAL-1 fixes): [Name + email]
- Team A (Doxygen Batch-A): [Name + email]
- Team B (Doxygen Batch-B): [Name + email]
- Team C (Doxygen Batch-C): [Name + email]
- Audit Team (TODO audit): [Name + email]

**Update:** `CONTENT_BATCH5_CP1_REMEDIATION_COORDINATION.md` (Team Assignments section)

### Step 2: Dispatch Agents (2026-08-16 08:00 UTC)

**Command 1:** Launch CRITICAL-1 Agent
```bash
task --agent themisdb-implementer \
     --name "content-batch5-critical1-dangling-pointers" \
     --mode background \
     --prompt "<Agent 1 prompt from above>"
```

**Command 2:** Launch HIGH-1 Agent
```bash
task --agent themisdb-implementer \
     --name "content-batch5-high1-doxygen-headers" \
     --mode background \
     --prompt "<Agent 2 prompt from above>"
```

**Command 3:** Launch HIGH-2 Agent
```bash
task --agent gap-verifier \
     --name "content-batch5-high2-todo-audit" \
     --mode background \
     --prompt "<Agent 3 prompt from above>"
```

### Step 3: Daily Monitoring (2026-08-16 to 2026-08-20)

**EOD each day:**
1. Review `CONTENT_BATCH5_CP1_REMEDIATION_DAILY_TRACKING.md` updates
2. Check SQL: `SELECT * FROM content_batch5_remediation WHERE status='blocked'`
3. Assess on-schedule status (GREEN / YELLOW / RED)
4. If RED: Contact relevant agent/team + determine mitigation

### Step 4: Evidence Bundle Assembly (2026-08-21)

**Action:** Compile `CP1_EVIDENCE_BUNDLE_2026_08_22/` with all deliverables from agents + teams

**Verify:**
- [ ] CRITICAL-1: Source code diffs + tests + memory safety validation
- [ ] HIGH-1: Batch reports + Doxygen audit + compliance CSV
- [ ] HIGH-2: Audit reports + reconciliation summary + CONTENT_DEFERRED_FEATURES.md
- [ ] Integration: PR/commit summary + CI/CD validation

### Step 5: CP-1 Re-Review Gate (2026-08-22 13:58 UTC)

**Action:** Assess against acceptance criteria (see CONTENT_BATCH5_CP1_REMEDIATION_COORDINATION.md)

**Decision:**
- ✅ **PASS** → Approve merge (Stream A → develop)
- 🔴 **FAIL** → Identify failed criterion + escalate + reassign fix team

---

## Monitoring Queries

### Daily Task Status
```sql
SELECT task_id, status, subtasks_complete || '/' || subtasks_total as progress, target_date
FROM content_batch5_remediation
WHERE target_date >= date('now')
ORDER BY target_date, task_id;
```

### Blocked Tasks
```sql
SELECT task_id, description, notes
FROM content_batch5_remediation
WHERE status = 'blocked'
ORDER BY task_id;
```

### Completed Tasks
```sql
SELECT task_id, completion_date, subtasks_complete || '/' || subtasks_total as result
FROM content_batch5_remediation
WHERE status = 'completed'
ORDER BY completion_date;
```

---

## Contingency: Dispatch Sub-Agent for Specific Blocked Task

If a single task is blocked and requires focused intervention:

```
task --agent themisdb-implementer \
     --name "content-batch5-critical1-fix-getbymimetype" \
     --mode sync \
     --prompt """
     ## Task: Fix getByMimeType() Dangling Pointer
     
     [Focus on specific method + error details from blocker]
     [Return std::optional<ContentType>]
     [Update 2-3 callers specific to this method]
     """
```

---

**Status:** 🟢 **READY FOR DISPATCH**  
**Next Step:** Confirm team assignments → Launch agents at 2026-08-16 08:00 UTC
