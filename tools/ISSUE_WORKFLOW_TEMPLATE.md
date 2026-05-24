# 🤖 AI-Agent Issue Workflow Template (7 Phases)

**For:** Code Gap Remediation Issues  
**Audience:** AI Agents (Claude, GPT, etc.) + Human Reviewers  
**Goal:** Provide structured, atomic, verifiable work breakdown

---

## Phase 0️⃣: Pre-Start Validation & Planning
**Duration:** 5-10 min | **Effort:** < 1%  
**Owner:** AI Agent (validate), Human (approve if issues)

### Entry Criteria
- [ ] Issue description loaded and parsed
- [ ] Gap inventory file exists (audit_report.json)
- [ ] Build environment is clean (no stale artifacts)
- [ ] All dependencies installed (Python 3.13, build tools)

### Tasks
```
[ ] Read issue description and gap inventory
[ ] Verify all required files exist in repo
[ ] Check build system (cmake, ninja, compiler)
[ ] Validate Python environment (.venv active)
[ ] Confirm test runner available (ctest, pytest)
[ ] Check available disk space (≥ 10GB for build)
```

### Acceptance Criteria
✅ Issue clearly understood  
✅ Gap inventory validated (0 duplicate IDs)  
✅ Build toolchain ready  
✅ Can execute: `cmake --build --preset windows-release` successfully

### Exit Criteria & Next Steps
- ✅ **PASS**: Proceed to Phase 1 (Audit)
- ❌ **FAIL**: Post comment with diagnostics, assign to human

---

## Phase 1️⃣: Code Audit & Gap Discovery
**Duration:** 1-4 hours | **Effort:** ~15%  
**Owner:** AI Agent (scan), Human (spot-check)

### Entry Criteria
- [ ] Pre-start validation passed
- [ ] Feature branch checked out (`feature/module-gaps-fix`)

### Tasks
```
[ ] Run gap scanner for module: gap_scanner_v3.py . ai_working
[ ] Load audit_report.json generated
[ ] Extract gaps by file, line, category, severity
[ ] Group by semantic category (Security, Memory, RAII, etc.)
[ ] Create module_gap_manifest.json with detailed inventory
[ ] Identify high-impact files (gaps > 50)
[ ] Generate audit summary (gaps_audit_summary.md)
[ ] Spot-check 5-10 gaps (verify line numbers, context accuracy)
```

### Acceptance Criteria
✅ Audit report generated with 100% of gaps catalogued  
✅ Every gap has: file, line number, function context, severity, category  
✅ No duplicate gap IDs  
✅ High-impact files identified  
✅ Spot-check shows < 2% false positives

### Deliverables
```
artifacts/
├── gap_audit_report.json         # Complete gap inventory
├── gaps_by_category.json         # Grouped by category
├── gaps_by_file.json             # Grouped by file
└── gaps_audit_summary.md         # Human-readable summary
```

### Exit Criteria & Next Steps
- ✅ **PASS**: Proceed to Phase 2 (Planning)
- ⚠️ **PARTIAL**: If audit has < 5% errors, document and proceed; if > 5%, restart Phase 1
- ❌ **FAIL**: Post audit report to issue, request human review

---

## Phase 2️⃣: Implementation Planning
**Duration:** 1-2 hours | **Effort:** ~10%  
**Owner:** AI Agent (plan), Human (validate)

### Entry Criteria
- [ ] Phase 1 audit completed
- [ ] module_gap_manifest.json exists

### Tasks
```
[ ] Load gap manifest and analyze dependencies
[ ] Group gaps by implementation task (category + file)
[ ] Identify shared refactoring (e.g., missing RAII wrapper used in 5 places)
[ ] Order tasks to minimize conflicts (dependencies first)
[ ] Estimate LOC changes per task (code + tests + docs)
[ ] Create implementation_plan.md with task ordering
[ ] Identify breaking changes or API impacts
[ ] Document fallback/rollback strategy per task
[ ] Create implementation_checklist.md
```

### Acceptance Criteria
✅ All gaps have assigned implementation task  
✅ Tasks ordered to avoid conflicts  
✅ Shared refactorings identified (save 20-30% effort)  
✅ LOC estimates provided (±30% accuracy acceptable)  
✅ Fallback strategy documented for risky tasks

### Deliverables
```
artifacts/
├── implementation_plan.md         # Detailed task breakdown
├── task_dependencies.json         # Task ordering/dependencies
├── implementation_checklist.md    # Atomic tasks with estimates
└── risk_assessment.md             # Identified risks & mitigations
```

### Exit Criteria & Next Steps
- ✅ **PASS**: Proceed to Phase 3 (Implementation)
- ⚠️ **NEEDS REVIEW**: If dependencies unclear or risks high, post plan to issue for human feedback
- ❌ **FAIL**: If > 20% of gaps can't be mapped to tasks, escalate to human

---

## Phase 3️⃣: Code Implementation
**Duration:** 8-40 hours | **Effort:** ~65%  
**Owner:** AI Agent (code), Human (spot-check every 5-10 commits)

### Entry Criteria
- [ ] Implementation plan validated
- [ ] Feature branch checked out
- [ ] `git reset --hard origin/develop` (clean state)

### Tasks (Iterative per Task)
```
FOR EACH TASK in implementation_plan:
  [ ] Checkout/create feature branch
  [ ] Read implementation guidance for category
  [ ] Implement gap fixes for all gaps in task
  [ ] Update API documentation (Doxygen)
  [ ] Add unit tests for fixed gaps
  [ ] Run: cmake --build --preset windows-release
  [ ] Run: ctest --preset windows-release --filter TaskName*
  [ ] Verify: no new compiler warnings (-Wall -Wextra)
  [ ] Verify: no new static analysis issues (clang-tidy)
  [ ] Commit with atomic message: "Fix: [Category] - [Gap Type]"
  [ ] Every 5 commits: push to feature branch, post status to issue
```

### Quality Gates (Per Task)
```
✅ Compilation: PASS (0 errors, ≤ N new warnings allowed)
✅ Tests: PASS (100% of new + related tests pass)
✅ Coverage: ≥ 90% code coverage for changed files
✅ Performance: No regression in benchmarks (±5% acceptable)
✅ Documentation: API docs updated, comments added
✅ Linting: No new style violations
```

### Incremental Checkpoints
- After 25% of gaps: Commit + test + post status
- After 50% of gaps: Commit + test + post status to issue
- After 75% of gaps: Commit + test + post status to issue
- At 100%: Full test suite + ready for Phase 4

### Acceptance Criteria
✅ All CRITICAL gaps fixed  
✅ All HIGH gaps fixed (or deferred with documented reason)  
✅ All MEDIUM gaps attempted (≥ 80% fixed acceptable)  
✅ 100% test pass rate (no regressions)  
✅ No compiler warnings on clean build  
✅ Static analysis passes  
✅ Documentation complete

### Rollback Criteria
If any of the following triggers, rollback to last known-good commit and post issue comment:
```
❌ Build fails (cannot recover in 30 min)
❌ > 20% of new tests fail
❌ New compiler warnings > 5
❌ Performance regression > 10%
❌ Unable to implement category (escalate)
```

### Deliverables
```
src/
├── [fixed files with gap remediation]
├── [new RAII wrappers if created]
└── [updated headers with documentation]

tests/
├── test_category_name_fixes.cpp   # New tests for fixed gaps
└── test_regression_suite.cpp      # Ensure no regressions

artifacts/
├── implementation_log.md           # What was fixed, where
├── code_changes_summary.md        # Delta report
└── test_results.json              # Full test output
```

### Exit Criteria & Next Steps
- ✅ **PASS**: Proceed to Phase 4 (Code Review)
- ⚠️ **PARTIAL**: If ≥ 80% of gaps fixed, proceed with documented deferrals
- ❌ **FAIL**: Rollback, post detailed error log to issue, assign to human

---

## Phase 4️⃣: Automated Code Review & Testing
**Duration:** 30-60 min | **Effort:** ~5%  
**Owner:** Automated (CI/CD pipeline)

### Entry Criteria
- [ ] All code implementation committed
- [ ] Feature branch pushed to GitHub
- [ ] No build errors locally

### Tasks (Automated)
```
[ ] Run full test suite (all modules)
[ ] Run code coverage analysis (report %)
[ ] Run static analysis (clang-tidy, cppcheck)
[ ] Run security scan (OWASP, CWE checks)
[ ] Run performance benchmarks (vs baseline)
[ ] Generate code review report
[ ] Check for new compiler warnings
```

### Acceptance Criteria
✅ All tests pass (100%)  
✅ Code coverage ≥ 90% for changed code  
✅ No new static analysis issues  
✅ Security scan passes (no new CWEs introduced)  
✅ Performance acceptable (< 5% regression)  
✅ No new compiler warnings

### Deliverables
```
artifacts/
├── test_report.json               # Full test results
├── coverage_report.html           # Code coverage
├── static_analysis_report.json    # clang-tidy, etc.
├── security_scan_report.json      # CWE/vulnerability check
└── performance_baseline.json      # Benchmark results
```

### Exit Criteria & Next Steps
- ✅ **PASS**: Proceed to Phase 5 (Human Review)
- ⚠️ **WARNINGS**: Document warnings, proceed to Phase 5 (human decides if acceptable)
- ❌ **FAIL**: Post failure report to issue, assign back to Phase 3

---

## Phase 5️⃣: Human Code Review & Sign-Off
**Duration:** 2-4 hours | **Effort:** ~5%  
**Owner:** Human (code owner, security lead)

### Entry Criteria
- [ ] All automated tests passed
- [ ] Feature branch created as Draft PR
- [ ] Automated review report posted to PR

### Review Checklist
```
PHASE 5 CODE REVIEW CHECKLIST:

Architecture & Design
[ ] Changes align with module design
[ ] No new tech debt introduced
[ ] API contract preserved (backward compatible)

Security
[ ] No new injection vectors
[ ] No hardcoded secrets/credentials
[ ] Input validation in place
[ ] CWE issues addressed

Performance
[ ] No O(n²) patterns introduced
[ ] No memory leaks (valgrind check)
[ ] Benchmarks acceptable

Correctness & Reliability
[ ] All edge cases handled
[ ] Error paths tested
[ ] Logging/observability adequate

Code Quality
[ ] Follows coding standards
[ ] Documentation complete
[ ] Comments explain "why" not "what"

Testing
[ ] Unit tests present and passing
[ ] Integration tests adequate
[ ] Edge cases covered
```

### Acceptance Criteria
✅ Code review comments < 5 (or documented as minor)  
✅ Security review passed  
✅ No architectural concerns  
✅ Tests adequate (≥ 90% coverage)  
✅ Documentation complete

### Exit Criteria & Next Steps
- ✅ **APPROVED**: Proceed to Phase 6 (Merge & Release)
- 📝 **CHANGES REQUESTED**: Post comments, reassign to AI agent for fixes, return to Phase 3
- ❌ **REJECTED**: Post detailed feedback to issue, discuss with team

---

## Phase 6️⃣: Documentation & Knowledge Transfer
**Duration:** 1-2 hours | **Effort:** ~5%  
**Owner:** AI Agent (generate), Human (approve)

### Entry Criteria
- [ ] Code review approved
- [ ] All tests passing

### Tasks
```
[ ] Create/update gap remediation summary (what was fixed)
[ ] Document any new APIs or breaking changes
[ ] Update module README if needed
[ ] Add CHANGELOG entry
[ ] Create architecture diagram if new patterns introduced
[ ] Document any deferred/partial fixes
[ ] Create FAQ for common questions
```

### Acceptance Criteria
✅ All changes documented  
✅ README updated if needed  
✅ CHANGELOG entry added  
✅ No gaps in documentation

### Deliverables
```
docs/
├── gap_remediation_summary.md     # What was fixed
├── api_changes.md                 # If any breaking changes
└── migration_guide.md             # If needed
```

### Exit Criteria & Next Steps
- ✅ **COMPLETE**: Ready for Phase 7 (Merge)
- ❌ **INCOMPLETE**: Assign back to Phase 6 for doc completion

---

## Phase 7️⃣: Merge & Release
**Duration:** 10-30 min | **Effort:** < 2%  
**Owner:** Human (code owner)

### Entry Criteria
- [ ] All review approvals obtained
- [ ] Documentation complete
- [ ] All tests passing
- [ ] Feature branch is up-to-date with develop

### Tasks
```
[ ] Rebase feature branch on latest develop
[ ] Run full test suite one more time
[ ] Verify: all checks pass
[ ] Merge PR to develop (squash if requested)
[ ] Delete feature branch
[ ] Tag release if applicable
[ ] Post merge notification to issue
```

### Acceptance Criteria
✅ Merged to develop  
✅ CI pipeline passes on develop  
✅ Release tagged if applicable  
✅ All team members notified

### Exit Criteria
- ✅ **COMPLETE**: Issue closed, gap remediation complete

---

## 🚨 Error Handling & Escalation

### Build Fails
```
Trigger: cmake --build fails or timeout
Recovery: (1) Review compiler error, fix if < 15 min
          (2) If > 15 min, rollback and escalate
Target: Restart Phase 3 from last good commit
```

### Tests Fail
```
Trigger: > 20% of tests fail
Recovery: (1) Run failed tests individually
          (2) Analyze failure root cause
          (3) If fixable in 30 min, fix
          (4) Else rollback and escalate
Target: Restart Phase 3 from last good commit
```

### Static Analysis Issues
```
Trigger: New security/static analysis issues
Recovery: (1) Review flagged code
          (2) If false positive, whitelist
          (3) If real issue, fix
Target: Must pass before Phase 5
```

### Performance Regression
```
Trigger: > 10% performance drop
Recovery: (1) Profile hotspot
          (2) Optimize or revert problematic change
          (3) If unrecoverable, document and escalate
Target: Must be < 5% before Phase 5
```

---

## 📊 Status Reporting

### AI Agent Status (posted to issue every 2-4 hours or per milestone)
```
✅ Phase X: COMPLETE
   - Tasks done: N/M
   - Issues: [list any blockers]
   - Next: Phase X+1
```

### Human Review Checkpoint (posted by code owner)
```
📝 Code Review: IN PROGRESS
   - Reviewed: N files
   - Issues found: N
   - Questions: [list]
   - ETA: [date/time]
```

---

## ✅ Issue Resolution Criteria

Issue is **DONE** when ALL of:
- ✅ Phase 1: Audit complete (100% gaps catalogued)
- ✅ Phase 2: Plan complete (all gaps mapped to tasks)
- ✅ Phase 3: Implementation complete (all gaps fixed or deferred)
- ✅ Phase 4: Automated tests pass (100%)
- ✅ Phase 5: Human code review approved
- ✅ Phase 6: Documentation complete
- ✅ Phase 7: Merged to develop

---

## 📝 Example: Issue Body Using This Template

```markdown
# 🔴 SECURITY Module Gap Remediation (278 gaps)

## Workflow
This issue follows the **7-Phase AI-Agent Workflow**:
- Phase 0: Validation ✅ (complete)
- Phase 1: Audit 🔄 (in progress - 50% complete)
- Phase 2: Planning ⏳ (waiting for Phase 1)
- Phase 3: Implementation ⏳
- Phase 4: Automated Review ⏳
- Phase 5: Human Review ⏳
- Phase 6: Documentation ⏳
- Phase 7: Merge ⏳

## Phase 1: Gap Audit
📊 **Inventory:** 278 gaps catalogued
- CRITICAL: 12 gaps (4%)
- HIGH: 186 gaps (67%)
- MEDIUM: 80 gaps (29%)

**Next:** Proceed to Phase 2

---
[Continue with category details, high-impact files, etc.]
```

---

## 🎯 Why 7 Phases Instead of 4?

| Phase | Why Important | Prevents |
|-------|--------------|----------|
| 0 | Validate preconditions | Wasted effort on broken setup |
| 1 | Audit + discovery | Missed gaps, false positives |
| 2 | Plan + dependencies | Conflicts, rework, inefficiency |
| 3 | Implement iteratively | Giant monolithic changes, build breaks |
| 4 | Automated checks | Passing bad code to humans |
| 5 | Human review | Missed design/security issues |
| 6 | Documentation | Knowledge loss, future confusion |
| 7 | Merge | Lost work, integration issues |

---

## 🔗 For AI Agents: Quick Command Reference

```bash
# Phase 0: Validate
cmake --build --preset windows-release --target help

# Phase 1: Audit
python tools/gap_scanner_v3.py . ai_working --module security

# Phase 3: Build incrementally
cmake --build --preset windows-release --parallel 4

# Phase 3: Test specific category
ctest --preset windows-release --filter "Security*" -VV

# Phase 4: Full validation
./scripts/quality-gate.ps1 -BuildPreset windows-release
```

