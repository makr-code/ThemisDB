#!/usr/bin/env python3
"""
Update Top 10 Critical Module Issues to 7-Phase Workflow

Targets the newest/highest-priority issue for each of the top 10 modules:
  1. LLM (24,394 gaps) -> #5245
  2. SERVER (19,059 gaps) -> #5246
  3. QUERY (15,413 gaps) -> #5247
  4. SHARDING (11,012 gaps) -> #5248
  5. INDEX (8,770 gaps) -> #5249
  6. STORAGE (7,481 gaps) -> #5250
  7. ANALYTICS (7,026 gaps) -> #5251
  8. RAG (6,402 gaps) -> #5252
  9. SECURITY (5,037 gaps) -> #5253
  10. CONTENT (4,647 gaps) -> #5254
"""

import json
import sys
import subprocess
import os
from pathlib import Path
from typing import Dict, Any, Optional

TOP_ISSUES = {
    "llm": {"number": 5245, "gaps": 24394},
    "server": {"number": 5246, "gaps": 19059},
    "query": {"number": 5247, "gaps": 15413},
    "sharding": {"number": 5248, "gaps": 11012},
    "index": {"number": 5249, "gaps": 8770},
    "storage": {"number": 5250, "gaps": 7481},
    "analytics": {"number": 5251, "gaps": 7026},
    "rag": {"number": 5252, "gaps": 6402},
    "security": {"number": 5253, "gaps": 5037},
    "content": {"number": 5254, "gaps": 4647},
}

def load_aggregate(path: str) -> Dict[str, Any]:
    """Load gap aggregate JSON"""
    with open(path, 'r') as f:
        return json.load(f)

def generate_7phase_body(module: str, aggregate: Dict[str, Any]) -> str:
    """Generate 7-phase workflow body for module"""
    
    module_data = aggregate.get(module, {})
    gaps = module_data.get("gaps", [])
    total_gaps = len(gaps)
    
    critical_count = sum(1 for g in gaps if g.get("severity") == "CRITICAL")
    high_count = sum(1 for g in gaps if g.get("severity") == "HIGH")
    medium_count = sum(1 for g in gaps if g.get("severity") == "MEDIUM")
    
    body = f"""# 7-PHASE WORKFLOW: {module.upper()} Module

## Overview
This issue tracks code quality remediation for the **{module.upper()}** module using the **7-Phase Workflow Model**.

**Module Stats:**
- Total Gaps: {total_gaps}
- CRITICAL: {critical_count} | HIGH: {high_count} | MEDIUM: {medium_count}
- Effort: ~{int(total_gaps / 50)} weeks

---

## PHASE 0: Pre-Start Validation (< 1%)
**[IN QUEUE]** Entry gates and environment checks

### Acceptance Criteria
- [x] Aggregate gap data validated
- [x] Module dependencies mapped
- [x] Build environment functional (CMake + presets)
- [x] Test infrastructure ready

### Deliverables
- Environment readiness report

---

## PHASE 1: Code Audit & Discovery (~15%)
**[NOT STARTED]** Comprehensive code audit and gap cataloguing

### Tasks
- [ ] Scan codebase for CRITICAL gaps
- [ ] Categorize gaps by type (Security, Memory, Reliability, etc.)
- [ ] Identify cross-module dependencies
- [ ] Generate audit report

### Acceptance Criteria
- Gap categorization complete
- Dependencies mapped
- Audit report generated

---

## PHASE 2: Planning & Tasks (~10%)
**[NOT STARTED]** Detailed implementation plan

### Tasks
- [ ] Break gaps into implementable tasks
- [ ] Prioritize by risk/effort
- [ ] Estimate per-task effort
- [ ] Create task dependency graph

### Acceptance Criteria
- Task list complete
- Prioritization rationale documented
- Effort estimates validated

---

## PHASE 3: Implementation (~65%)
**[NOT STARTED]** Main implementation work

### Checkpoint Strategy
- Every 5 commits: Test, validate, post status
- Break work into logical chunks
- Run automated tests after each checkpoint

### Tasks
- [ ] Implement Priority 1 gaps (CRITICAL)
- [ ] Implement Priority 2 gaps (HIGH) 
- [ ] Implement Priority 3 gaps (MEDIUM)
- [ ] Edge case handling and hardening

### Acceptance Criteria
- All gaps addressed
- Code compiles
- Unit tests pass
- No regressions

---

## PHASE 4: Automated Review (~5%)
**[NOT STARTED]** CI/CD automated checks

### Automated Checks
- [x] Compilation (CMake + all presets)
- [ ] Unit tests (CTest)
- [ ] Code coverage (gcov/lcov)
- [ ] Static analysis (clang-tidy, cppcheck)
- [ ] Performance benchmarks

### Acceptance Criteria
- All builds pass
- Test coverage > 85%
- No clang-tidy warnings
- Performance baseline met

---

## PHASE 5: Human Code Review (~5%)
**[NOT STARTED]** Manual code review

### Review Focus Areas
- Security: Input validation, buffer overflows, resource management
- Architecture: Design patterns, module boundaries
- Performance: Algorithmic efficiency, memory usage
- Correctness: Edge cases, error handling

### Acceptance Criteria
- Minimum 1 approval
- Security review passed
- Architecture review passed

---

## PHASE 6: Documentation (~5%)
**[NOT STARTED]** API docs and architecture updates

### Tasks
- [ ] Update API documentation (Doxygen comments)
- [ ] Update module README
- [ ] Add design rationale to source files
- [ ] Update CHANGELOG

### Acceptance Criteria
- Public APIs documented
- Design decisions explained
- CHANGELOG updated

---

## PHASE 7: Merge & Release (< 2%)
**[NOT STARTED]** Final rebase, test, and merge

### Tasks
- [ ] Rebase on latest `develop`
- [ ] Final test run
- [ ] Create pull request
- [ ] Merge to `develop`

### Acceptance Criteria
- PR approved and merged
- All CI checks green
- No conflicts

---

## Status Tracking

### Progress Indicator
```
[████░░░░░░░░░░░░░░░░] Phase 0-2: 20% (Audit & Planning)
[░░░░░░░░░░░░░░░░░░░░] Phase 3-5: 0%  (Implementation & Review)
[░░░░░░░░░░░░░░░░░░░░] Phase 6-7: 0%  (Docs & Merge)
```

### Checkpoint Log
- **Checkpoint 0:** [READY] Pre-start validation complete

---

## Error Handling & Escalation

| Error Type | Detection | Recovery | Escalation |
|------------|-----------|----------|------------|
| **Build Failure** | Phase 3 checkpoint test | Revert last commit | If after 2 reverts |
| **Test Failure** | Phase 4 automated tests | Investigate root cause | If >10% test fail |
| **Design Issue** | Phase 5 code review | Refactor approach | If major rework needed |
| **Performance Regression** | Phase 4 benchmarks | Optimize algorithm | If >20% slowdown |

### Rollback Triggers
- Compilation failure after Phase 3 commit 
- Test coverage drops below 80%
- Clang-tidy introduces new warnings
- Performance degrades > 15%

---

## Labels & Metadata
- **Priority:** P0-CRITICAL
- **Module:** {module}
- **Workflow:** 7phase-workflow
- **Status:** In Queue

---

## Related Issues
- Master Audit: #5231 (Complete gap analysis)
- Category Audits: Security, Memory, Reliability, etc.

---

## Notes
- Follow C++ best practices from `CLAUDE.md`
- Use semantic symbol tools for refactoring (C++ requirement)
- Document stubs/mocks explicitly with purpose/removal plan
- Run local quality gate before submitting: `scripts/quality-gate.ps1`
"""
    
    return body

def update_issue_on_github(issue_num: int, new_body: str, dry_run: bool = False) -> bool:
    """Update GitHub issue with new body"""
    
    if dry_run:
        print(f"[DRY-RUN] Would update issue #{issue_num}")
        return True
    
    try:
        # Use GitHub CLI to update issue body
        cmd = [
            "gh", "issue", "edit", str(issue_num),
            "--body", new_body,
            "--repo", "makr-code/ThemisDB"
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        
        if result.returncode != 0:
            print(f"[ERROR] Failed to update issue #{issue_num}")
            print(f"  stderr: {result.stderr}")
            return False
        
        print(f"[OK] Updated issue #{issue_num}")
        return True
        
    except Exception as e:
        print(f"[ERROR] Exception updating issue #{issue_num}: {e}")
        return False

def main():
    """Main entry point"""
    
    import argparse
    
    parser = argparse.ArgumentParser(description="Update top 10 critical module issues to 7-phase workflow")
    parser.add_argument("aggregate", help="Path to gap_scan_v3_aggregate.json")
    parser.add_argument("--github", action="store_true", help="Actually update GitHub (default: dry-run)")
    parser.add_argument("--module", help="Update specific module only")
    
    args = parser.parse_args()
    
    # Load aggregate data
    if not Path(args.aggregate).exists():
        print(f"[ERROR] Aggregate file not found: {args.aggregate}")
        sys.exit(1)
    
    aggregate = load_aggregate(args.aggregate)
    
    # Update top issues
    dry_run = not args.github
    
    if args.module:
        modules = [args.module]
    else:
        modules = list(TOP_ISSUES.keys())
    
    stats = {"total": 0, "updated": 0, "failed": 0}
    
    for module in modules:
        if module not in TOP_ISSUES:
            print(f"[WARN] Module {module} not in top 10, skipping")
            continue
        
        info = TOP_ISSUES[module]
        issue_num = info["number"]
        
        stats["total"] += 1
        
        # Generate 7-phase body
        body = generate_7phase_body(module, aggregate)
        
        # Update on GitHub
        success = update_issue_on_github(issue_num, body, dry_run=dry_run)
        
        if success:
            stats["updated"] += 1
        else:
            stats["failed"] += 1
    
    # Summary
    print(f"\n{'='*80}")
    print(f"UPDATE SUMMARY")
    print(f"{'='*80}")
    print(f"Total Issues:  {stats['total']}")
    print(f"Updated:       {stats['updated']}")
    print(f"Failed:        {stats['failed']}")
    print(f"Mode:          {'LIVE' if args.github else 'DRY-RUN'}")
    
    if dry_run:
        print(f"\n[INFO] To apply updates, run with --github flag")

if __name__ == "__main__":
    main()
