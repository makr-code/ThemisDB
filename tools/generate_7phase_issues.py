#!/usr/bin/env python3
"""
Enhanced GitHub Issue Template Generator — 7-Phase AI-Agent Workflow

Generates detailed, actionable GitHub issues with:
  - 7-phase structured workflow (Validation → Audit → Plan → Implement → Auto-Review → Human-Review → Merge)
  - Clear entry/exit criteria for each phase
  - Checkpoint status tracking
  - Error handling & escalation procedures
  - Atomic task breakdown
  - Automated testing hooks

Features:
  - Phase-specific acceptance criteria
  - Status reporting templates
  - Error recovery procedures
  - Documentation checklists
"""

import json
import sys
from pathlib import Path
from typing import Dict, Any


class SevenPhaseIssueGenerator:
    """Generate 7-phase workflow issues for AI agents"""
    
    def __init__(self, aggregate_path: str, module: str):
        self.aggregate_path = aggregate_path
        self.module = module
        
        with open(aggregate_path, 'r') as f:
            self.aggregate = json.load(f)
        
        if module not in self.aggregate:
            raise ValueError(f"Module {module} not found in aggregate")
        
        self.module_data = self.aggregate[module]
    
    def generate_issue_body(self) -> str:
        """Generate complete issue body with 7-phase workflow"""
        
        critical = self.module_data.get('severity_critical', 0)
        high = self.module_data.get('severity_high', 0)
        medium = self.module_data.get('severity_medium', 0)
        total = self.module_data.get('total', 0)
        
        # Build issue body
        body = self._header(critical, high, medium, total)
        body += self._workflow_status()
        body += self._phase_0_validation()
        body += self._phase_1_audit(total)
        body += self._phase_2_planning()
        body += self._phase_3_implementation(critical, high, medium)
        body += self._phase_4_automated_review()
        body += self._phase_5_human_review()
        body += self._phase_6_documentation()
        body += self._phase_7_merge()
        body += self._error_handling()
        body += self._completion_criteria()
        
        return body
    
    def _header(self, critical, high, medium, total) -> str:
        return f"""# 🔴 {self.module.upper()} Module Gap Remediation Workflow

**Status:** Ready for Implementation (7-Phase AI-Agent Workflow)  
**Assigned to:** [AI Agent]  
**Start Date:** [When work begins]  
**Target Completion:** [Phases 0-7: ~2-3 weeks depending on gap complexity]

---

## 📊 Gap Summary

| Metric | Value | % |
|--------|-------|---|
| **Total Gaps** | {total} | 100% |
| **🔴 CRITICAL** | {critical} | {int(critical/max(1,total)*100)}% |
| **🟠 HIGH** | {high} | {int(high/max(1,total)*100)}% |
| **🟡 MEDIUM** | {medium} | {int(medium/max(1,total)*100)}% |
| **ACTIONABLE (C+H)** | {critical+high} | {int((critical+high)/max(1,total)*100)}% |

---

"""
    
    def _workflow_status(self) -> str:
        return """## 🚀 7-Phase Workflow Progress

```
Phase 0: Validation              [ ] → Not started
         ↓
Phase 1: Audit & Discovery      [ ] → Not started
         ↓
Phase 2: Planning & Tasks       [ ] → Not started
         ↓
Phase 3: Implementation         [ ] → Not started (65% effort)
         ↓
Phase 4: Automated Review       [ ] → Not started
         ↓
Phase 5: Human Code Review      [ ] → Not started
         ↓
Phase 6: Documentation          [ ] → Not started
         ↓
Phase 7: Merge & Release        [ ] → Not started
```

---

## ✅ Phase 0: Pre-Start Validation & Planning

**Duration:** 5-10 min | **Effort:** < 1%

### Entry Criteria
- [ ] Issue description loaded and understood
- [ ] Gap inventory file exists
- [ ] Build environment clean (no stale artifacts)
- [ ] All dependencies installed

### Validation Tasks
```
[ ] Read full issue description
[ ] Verify gap inventory file exists
[ ] Confirm build system: cmake --build --preset windows-release
[ ] Verify Python environment: .venv activated
[ ] Check disk space (≥ 10GB)
[ ] Test: ctest --help (confirm test runner)
```

### Exit Criteria
- ✅ **PASS**: All validations successful → Proceed to Phase 1
- ❌ **FAIL**: Post diagnostics comment, request human intervention

**Next Step:** When ready, post comment `@agent-name start-phase-1`

---

## 📋 Phase 1: Code Audit & Gap Discovery

**Duration:** 1-4 hours | **Effort:** ~15%

### Entry Criteria
- [ ] Phase 0 passed
- [ ] Feature branch created: `git checkout -b fix/{module}-gaps`

### Gap Discovery Tasks
```
[ ] Execute: python tools/scanners/orchestrator.py . ai_working --module {module}
[ ] Load: ai_working/gap_scan_v3_{module}.json
[ ] Create: artifacts/gap_audit_report.json (detailed inventory)
[ ] Create: artifacts/gaps_by_category.json (grouped)
[ ] Create: artifacts/gaps_by_file.json (by file)
[ ] Create: artifacts/gap_audit_summary.md (human-readable)
[ ] Spot-check: 10 random gaps (verify accuracy)
[ ] Identify: High-impact files (gaps > 50)
```

### Acceptance Criteria
✅ 100% of gaps catalogued with file, line, function, severity  
✅ Spot-check shows < 2% false positives  
✅ High-impact files identified  
✅ Summary ready for Phase 2

### Deliverables
```
✓ artifacts/gap_audit_report.json
✓ artifacts/gaps_by_category.json  
✓ artifacts/gaps_by_file.json
✓ artifacts/gap_audit_summary.md
```

### Exit Criteria
- ✅ **PASS**: Audit complete → Comment: `Phase 1 COMPLETE: [N gaps catalogued]`
- ❌ **FAIL**: Post audit issues to comment

**Next Step:** Human reviews gap counts, then `@agent-name start-phase-2`

---

## 📐 Phase 2: Implementation Planning

**Duration:** 1-2 hours | **Effort:** ~10%

### Entry Criteria
- [ ] Phase 1 audit complete
- [ ] Gap audit report reviewed

### Planning Tasks
```
[ ] Analyze gap dependencies (shared refactorings, etc.)
[ ] Group gaps by implementation task (category + file)
[ ] Order tasks to avoid conflicts (dependencies first)
[ ] Estimate LOC changes per task
[ ] Identify risky tasks (mark for monitoring)
[ ] Create: artifacts/implementation_plan.md
[ ] Create: artifacts/task_dependencies.json
[ ] Create: artifacts/implementation_checklist.md
```

### Acceptance Criteria
✅ All gaps mapped to implementation tasks  
✅ Tasks ordered properly (dependencies resolved)  
✅ Shared refactorings identified  
✅ Effort estimates provided (±30% acceptable)  
✅ Risks documented

### Deliverables
```
✓ artifacts/implementation_plan.md
✓ artifacts/task_dependencies.json
✓ artifacts/implementation_checklist.md
✓ artifacts/risk_assessment.md
```

### Exit Criteria
- ✅ **PASS**: Plan complete → Comment: `Phase 2 COMPLETE: [N tasks identified]`
- ❌ **FAIL**: Post planning issues

**Next Step:** Human reviews plan, then `@agent-name start-phase-3`

---

## 💻 Phase 3: Code Implementation

**Duration:** 8-40 hours | **Effort:** ~65% (MAIN WORK)

### Entry Criteria
- [ ] Implementation plan validated
- [ ] Feature branch ready
- [ ] Git reset to clean state: `git reset --hard origin/develop`

### Implementation Loop (Per Task)
```
FOR EACH TASK in implementation_plan:
  [ ] Read implementation guidance
  [ ] Implement all gap fixes in task
  [ ] Update API documentation (Doxygen)
  [ ] Add/update unit tests (≥ 90% coverage)
  [ ] Build: cmake --build --preset windows-release
  [ ] Test: ctest --preset windows-release --filter TaskName*
  [ ] Verify: No new warnings (-Wall -Wextra)
  [ ] Verify: No new clang-tidy issues
  [ ] Commit: git commit -m "Fix: [Category] - [Gap Description]"
  [ ] Every 5 commits: POST STATUS COMMENT to issue
```

### Quality Gate (Per Task)
```
✅ Compilation: PASS (0 errors, ≤ 3 new warnings)
✅ Tests: 100% pass rate
✅ Coverage: ≥ 90% for changed files
✅ Performance: ≤ 5% regression
✅ No new static analysis issues
```

### Checkpoint Progress (Status Posts)
```
After every 5 commits or completion of major task:
---
✅ Phase 3 Progress: [X/N tasks complete]
- Tasks done: [list]
- Tests passing: Y/Y
- Build status: OK
- ETA: [date/time]
```

### Rollback Triggers
If any occurs, **revert to last good commit** and post comment:
```
❌ Build fails (cannot fix in 30 min)
❌ > 20% tests fail
❌ > 5 new compiler warnings
❌ > 10% performance regression
```

### Exit Criteria
- ✅ **PASS**: All gaps fixed, tests 100% pass → Comment: `Phase 3 COMPLETE`
- ⚠️ **PARTIAL**: ≥ 80% fixed → Proceed with documented deferrals
- ❌ **FAIL**: Rollback and escalate

**Next Step:** `@agent-name start-phase-4` (automated)

---

## 🤖 Phase 4: Automated Code Review & Testing

**Duration:** 30-60 min | **Effort:** ~5% (Automated)

### Entry Criteria
- [ ] Phase 3 implementation complete
- [ ] All code committed to feature branch
- [ ] No uncommitted changes

### Automated Tasks (CI Pipeline)
```
[ ] Full test suite: ctest --preset windows-release --output-on-failure
[ ] Code coverage: gcov/llvm-cov analysis
[ ] Static analysis: clang-tidy -checks=*
[ ] Security scan: cppcheck --security
[ ] Performance: Benchmark vs baseline
[ ] Generate reports
```

### Acceptance Criteria
✅ 100% tests pass  
✅ Coverage ≥ 90%  
✅ No new static analysis issues  
✅ Performance acceptable (< 5% regression)  
✅ No new compiler warnings

### Deliverables
```
✓ artifacts/test_report.json
✓ artifacts/coverage_report.html
✓ artifacts/static_analysis.json
✓ artifacts/performance_baseline.json
```

### Exit Criteria
- ✅ **PASS**: All checks pass → Auto-proceed to Phase 5
- ⚠️ **WARNINGS**: Document, proceed to Phase 5 for human decision
- ❌ **FAIL**: Post report, return to Phase 3

---

## 👤 Phase 5: Human Code Review & Sign-Off

**Duration:** 2-4 hours | **Effort:** ~5%

### Entry Criteria
- [ ] Phase 4 automated checks passed
- [ ] Feature branch pushed
- [ ] Draft PR created

### Code Review Checklist
```
ARCHITECTURE & DESIGN
[ ] Changes align with module design
[ ] No new tech debt
[ ] API contract preserved

SECURITY
[ ] No new injection vectors
[ ] No hardcoded secrets
[ ] Input validation adequate

PERFORMANCE
[ ] No O(n²) patterns introduced
[ ] No memory leaks
[ ] Benchmarks acceptable

CORRECTNESS & RELIABILITY
[ ] Edge cases handled
[ ] Error paths tested
[ ] Logging adequate

CODE QUALITY
[ ] Follows standards
[ ] Documentation complete
[ ] Comments explain "why"

TESTING
[ ] Unit tests present
[ ] Integration tests adequate
[ ] ≥ 90% coverage
```

### Exit Criteria
- ✅ **APPROVED**: All comments addressed → Proceed to Phase 6
- 📝 **CHANGES REQUESTED**: Post comments, return to Phase 3
- ❌ **REJECTED**: Post detailed feedback

---

## 📚 Phase 6: Documentation & Knowledge Transfer

**Duration:** 1-2 hours | **Effort:** ~5%

### Documentation Tasks
```
[ ] Create/update: Gap Remediation Summary
[ ] Document: Any new APIs or breaking changes
[ ] Update: Module README
[ ] Add: CHANGELOG entry
[ ] Create: Architecture diagrams (if new patterns)
[ ] Document: Any deferred/partial fixes
[ ] Create: FAQ for common questions
```

### Deliverables
```
✓ docs/gap_remediation_summary.md
✓ docs/api_changes.md (if applicable)
✓ CHANGELOG entry
✓ Updated README
```

### Exit Criteria
- ✅ **COMPLETE**: All documentation done → Ready for Phase 7

---

## 🎉 Phase 7: Merge & Release

**Duration:** 10-30 min | **Effort:** < 2%

### Final Merge Tasks
```
[ ] Rebase feature branch on latest develop
[ ] Run full test suite one final time
[ ] Verify all CI checks pass
[ ] Merge to develop (squash if requested)
[ ] Delete feature branch
[ ] Post merge notification to issue
```

### Exit Criteria
- ✅ **COMPLETE**: Merged to develop, issue closed

---

## 🚨 Error Handling & Escalation

### Build Fails
**Trigger:** `cmake --build` fails or hangs  
**Recovery:**
1. Review error message
2. If fixable in < 30 min: Fix and rebuild
3. If > 30 min: Revert to last good commit
4. Post comment: `Build failed: [error]. Rolling back to [commit].`  
**Target:** Restart Phase 3

### Tests Fail (> 20%)
**Trigger:** More than 20% of new tests fail  
**Recovery:**
1. Run failed tests individually with `-VV`
2. Analyze root cause
3. If fixable in < 1 hour: Fix and retest
4. If > 1 hour: Revert changes
5. Post comment: `Tests failed. Root cause: [reason]. Rolling back.`  
**Target:** Restart Phase 3

### Performance Regression (> 10%)
**Trigger:** Benchmark shows > 10% slower  
**Recovery:**
1. Profile hotspot with perf/valgrind
2. Optimize problematic code
3. If unrecoverable: Document and escalate  
**Target:** Must be < 5% before Phase 5

### Static Analysis Issues (New)
**Trigger:** New security or static analysis flags  
**Recovery:**
1. Review flagged code
2. If false positive: Whitelist with comment
3. If real issue: Fix code
4. Rerun analysis  
**Target:** Must pass before Phase 5

---

"""
    
    def _phase_0_validation(self) -> str:
        return ""  # Already in workflow_status
    
    def _phase_1_audit(self, total) -> str:
        return ""  # Already in workflow_status
    
    def _phase_2_planning(self) -> str:
        return ""  # Already in workflow_status
    
    def _phase_3_implementation(self, critical, high, medium) -> str:
        return ""  # Already in workflow_status
    
    def _phase_4_automated_review(self) -> str:
        return ""  # Already in workflow_status
    
    def _phase_5_human_review(self) -> str:
        return ""  # Already in workflow_status
    
    def _phase_6_documentation(self) -> str:
        return ""  # Already in workflow_status
    
    def _phase_7_merge(self) -> str:
        return ""  # Already in workflow_status
    
    def _error_handling(self) -> str:
        return ""  # Already in workflow_status
    
    def _completion_criteria(self) -> str:
        return """## ✅ Issue Resolution Criteria

**DONE** when:
- ✅ Phase 0: Validation complete
- ✅ Phase 1: 100% gaps catalogued
- ✅ Phase 2: All gaps mapped to tasks
- ✅ Phase 3: All CRITICAL/HIGH gaps fixed, ≥80% MEDIUM fixed
- ✅ Phase 4: 100% automated tests pass
- ✅ Phase 5: Human code review approved
- ✅ Phase 6: Documentation complete
- ✅ Phase 7: Merged to develop

---

**Generated:** [Current Date]  
**Workflow:** 7-Phase AI-Agent Model (See tools/ISSUE_WORKFLOW_TEMPLATE.md)  
**Labels:** gap-scanner, {self.module}, ready-for-ai-agent
"""


def generate_all_issues(aggregate_path: str, output_dir: str = "ai_working/issues_7phase"):
    """Generate 7-phase issues for all modules"""
    
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    with open(aggregate_path, 'r') as f:
        aggregate = json.load(f)
    
    print(f"\n📝 Generating 7-Phase Issues ({len(aggregate)} modules)")
    print("=" * 80)
    
    for module in sorted(aggregate.keys()):
        generator = SevenPhaseIssueGenerator(aggregate_path, module)
        body = generator.generate_issue_body()
        
        output_file = Path(output_dir) / f"{module}_7phase_issue.md"
        with open(output_file, 'w') as f:
            f.write(body)
        
        gap_count = aggregate[module].get('total', 0)
        print(f"✓ {module:30} {gap_count:4} gaps → {output_file.name}")
    
    print("\n✅ Generated 7-phase issues for all modules")
    print(f"Output directory: {output_dir}/")


def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_7phase_issues.py <aggregate.json> [output_dir]")
        sys.exit(1)
    
    aggregate_path = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "ai_working/issues_7phase"
    
    generate_all_issues(aggregate_path, output_dir)


if __name__ == '__main__':
    main()
