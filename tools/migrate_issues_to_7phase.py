#!/usr/bin/env python3
"""
GitHub Issue Migration Tool — Convert Existing Issues to 7-Phase Workflow

Updates all existing gap remediation issues to use the new 7-phase workflow model.
Preserves issue metadata while updating body with structured workflow.

Usage:
  python migrate_issues_to_7phase.py <aggregate.json> --github [--dry-run]
  
Options:
  --github      Actually update issues on GitHub (requires GH_TOKEN)
  --dry-run     Show what would change (no actual updates)
  --module      Update specific module only
"""

import json
import sys
import os
import subprocess
from pathlib import Path
from typing import Dict, Any, List, Optional


class GitHubIssueMigrator:
    """Migrate existing issues to 7-phase workflow"""
    
    def __init__(self, aggregate_path: str, dry_run: bool = True):
        self.aggregate_path = aggregate_path
        self.dry_run = dry_run
        
        with open(aggregate_path, 'r') as f:
            self.aggregate = json.load(f)
        
        # Try to get GitHub token from environment
        self.gh_token = os.environ.get('GH_TOKEN')
        self.repo_owner = 'makr-code'
        self.repo_name = 'ThemisDB'
    
    def list_existing_issues(self) -> List[Dict[str, Any]]:
        """List all gap-remediation issues on GitHub"""
        
        if self.dry_run:
            print("[TASKS] [DRY-RUN] Would list existing issues from GitHub...")
            return []
        
        if not self.gh_token:
            print("[ERROR] GH_TOKEN not set. Cannot query GitHub.")
            return []
        
        # Use GitHub CLI to list issues
        cmd = [
            'gh', 'issue', 'list',
            '--repo', f'{self.repo_owner}/{self.repo_name}',
            '--label', 'gap-scanner',
            '--state', 'open',
            '--json', 'number,title,body,labels'
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            issues = json.loads(result.stdout)
            return issues
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] GitHub CLI error: {e.stderr}")
            return []
    
    def parse_module_from_title(self, title: str) -> Optional[str]:
        """Extract module name from issue title"""
        # Expected format: "[CRITICAL] SECURITY Module Gap Remediation"
        # or: "[CRITICAL] MEMORY Module Gap Remediation"
        
        parts = title.split()
        for i, part in enumerate(parts):
            if 'MODULE' in part.upper() or 'REMEDIATION' in part.upper():
                # Look for module name before "Module" or "MODULE"
                if i > 1:
                    # Title format: "[CRITICAL] MODULENAME Module Gap Remediation"
                    return parts[1].lower()
        
        return None
    
    def generate_new_issue_body(self, module: str, existing_body: str) -> str:
        """Generate new 7-phase issue body for module"""
        
        if module not in self.aggregate:
            print(f"[WARN]  Module {module} not in aggregate, skipping")
            return existing_body
        
        module_data = self.aggregate[module]
        
        critical = module_data.get('severity_critical', 0)
        high = module_data.get('severity_high', 0)
        medium = module_data.get('severity_medium', 0)
        total = module_data.get('total', 0)
        
        # Build new body
        body = f"""# [CRITICAL] {module.upper()} Module Gap Remediation — 7-Phase Workflow

**Status:** Ready for Implementation (7-Phase AI-Agent Model)  
**Updated:** {self._get_date()}  
**Workflow:** See [ISSUE_WORKFLOW_TEMPLATE.md](tools/ISSUE_WORKFLOW_TEMPLATE.md)

---

## 📊 Gap Summary

| Metric | Value | % |
|--------|-------|---|
| **Total Gaps** | {total} | 100% |
| **[CRITICAL] CRITICAL** | {critical} | {int(critical/max(1,total)*100)}% |
| **🟠 HIGH** | {high} | {int(high/max(1,total)*100)}% |
| **🟡 MEDIUM** | {medium} | {int(medium/max(1,total)*100)}% |
| **ACTIONABLE (C+H)** | {critical+high} | {int((critical+high)/max(1,total)*100)}% |

---

## [WORKFLOW] 7-Phase Workflow Progress

```
Phase 0: Validation              [ ] → Ready to start
Phase 1: Audit & Discovery       [ ] → Not started
Phase 2: Planning & Tasks        [ ] → Waiting for Phase 1
Phase 3: Implementation          [ ] → Not started (65% effort)
Phase 4: Automated Review        [ ] → Automated (after Phase 3)
Phase 5: Human Code Review       [ ] → Waiting for Phase 4
Phase 6: Documentation           [ ] → Waiting for Phase 5
Phase 7: Merge & Release         [ ] → Waiting for Phase 6
```

---

## [OK] Phase 0: Pre-Start Validation

**Duration:** 5-10 min | **Effort:** < 1%

### Validation Checklist
```
[ ] Read issue description and understand scope
[ ] Verify gap inventory file exists (ai_working/gap_scan_v3_{module}.json)
[ ] Confirm build environment: cmake --build --preset windows-release
[ ] Verify Python environment: .venv activated
[ ] Test: ctest --help (confirm test runner available)
[ ] Check disk space: ≥ 10GB available
```

**Next Step:** When ready, AI agent posts: `Phase 0 validation complete. Starting Phase 1.`

---

## [TASKS] Phase 1: Code Audit & Gap Discovery

**Duration:** 1-4 hours | **Effort:** ~15%

### Discovery Tasks
```
[ ] Run: python tools/gap_scanner_v3.py . ai_working --module {module}
[ ] Generate: artifacts/gap_audit_report_{module}.json
[ ] Generate: artifacts/gaps_by_category_{module}.json
[ ] Generate: artifacts/gaps_by_file_{module}.json
[ ] Spot-check: 10 random gaps (verify line numbers + context)
[ ] Identify: High-impact files (> 50 gaps each)
[ ] Create: artifacts/gap_audit_summary_{module}.md
```

**Success:** 100% of gaps catalogued with file/line/function/severity

**Status Comment Template:**
```
[OK] Phase 1 COMPLETE: {total} gaps catalogued
- CRITICAL: {critical} gaps
- HIGH: {high} gaps  
- MEDIUM: {medium} gaps
- Next: Phase 2 (Planning)
```

---

## 📐 Phase 2: Implementation Planning

**Duration:** 1-2 hours | **Effort:** ~10%

### Planning Tasks
```
[ ] Analyze gap dependencies
[ ] Group by implementation task (category + file)
[ ] Order tasks (resolve dependencies first)
[ ] Estimate LOC changes per task
[ ] Identify risky tasks
[ ] Create: artifacts/implementation_plan_{module}.md
[ ] Create: artifacts/task_dependencies_{module}.json
[ ] Create: artifacts/implementation_checklist_{module}.md
```

**Success:** All gaps mapped to tasks, ordering validated

**Status Comment Template:**
```
[OK] Phase 2 COMPLETE: Implementation plan ready
- Total tasks: N
- Shared refactorings: M
- Estimated effort: X days
- Next: Phase 3 (Implementation)
```

---

## 💻 Phase 3: Code Implementation

**Duration:** 8-40 hours | **Effort:** ~65% (MAIN WORK)

### Implementation Loop
```
FOR EACH TASK in implementation_plan:
  [ ] Read implementation guidance for category
  [ ] Implement gap fixes
  [ ] Update API documentation (Doxygen comments)
  [ ] Add/update unit tests (target ≥ 90% coverage)
  [ ] Build: cmake --build --preset windows-release
  [ ] Test: ctest --preset windows-release --filter TaskName*
  [ ] Verify: No new warnings (-Wall -Wextra)
  [ ] Commit: git commit -m "Fix: [Category] - [Gap Description]"
  ☞ Every 5 commits: POST STATUS COMMENT to this issue
```

### Quality Gate (Per Task)
```
[OK] Compilation: PASS (0 errors)
[OK] Tests: 100% pass rate
[OK] Coverage: ≥ 90% for changed code
[OK] Performance: ≤ 5% regression
[OK] No new static analysis issues
```

### Rollback Triggers
If any of these occurs, **revert to last good commit** and comment:
```
[ERROR] Build fails (> 30 min to fix)
[ERROR] > 20% tests fail
[ERROR] > 5 new compiler warnings
[ERROR] > 10% performance regression
```

**Status Comment Template (Every 5 commits):**
```
[WORKFLOW] Phase 3 Progress: X/N tasks complete
- Commits: Y
- Tests passing: A/A [OK]
- Build status: OK
- Current file: src/module/file.cpp
- ETA: [date/time]
```

---

## 🤖 Phase 4: Automated Code Review & Testing

**Duration:** 30-60 min | **Effort:** ~5% (Automated via CI/CD)

### Automated Checks
```
[ ] Full test suite: ctest --preset windows-release
[ ] Code coverage: ≥ 90%
[ ] Static analysis: clang-tidy, cppcheck
[ ] Security scan: CWE/injection checks
[ ] Performance baseline: Benchmark comparison
```

**Success:** All automated checks PASS

---

## 👤 Phase 5: Human Code Review & Sign-Off

**Duration:** 2-4 hours | **Effort:** ~5%

### Code Review Checklist
```
ARCHITECTURE & DESIGN
[ ] Changes align with module design
[ ] No new tech debt introduced
[ ] API contract preserved

SECURITY
[ ] No new injection vectors
[ ] No hardcoded credentials
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
[ ] Comments explain "why"
[ ] Documentation complete

TESTING
[ ] Unit tests present
[ ] ≥ 90% coverage
```

**Success:** Code review approved with ≤ 5 comments

---

## 📚 Phase 6: Documentation & Knowledge Transfer

**Duration:** 1-2 hours | **Effort:** ~5%

### Documentation Tasks
```
[ ] Create/update: Gap Remediation Summary
[ ] Document: Any new APIs or breaking changes
[ ] Update: Module README (if needed)
[ ] Add: CHANGELOG entry
[ ] Create: Architecture diagrams (if new patterns)
```

**Success:** Documentation complete and reviewed

---

## 🎉 Phase 7: Merge & Release

**Duration:** 10-30 min | **Effort:** < 2%

### Final Merge Tasks
```
[ ] Rebase feature branch on latest develop
[ ] Run full test suite one final time
[ ] Merge to develop (squash if requested)
[ ] Delete feature branch
[ ] Tag release (if applicable)
```

**Success:** Merged to develop, tests passing on develop

---

## 🚨 Error Handling & Escalation

### Build Fails
**Trigger:** `cmake --build` fails or times out  
**Recovery:** (1) Review error (2) If fixable in 30 min: fix; (3) Else: rollback  
**Action:** Revert to last good commit, post comment with error details

### Tests Fail (> 20%)
**Trigger:** More than 20% of new tests fail  
**Recovery:** (1) Run individually (-VV) (2) Analyze root cause (3) Fix or rollback  
**Action:** If not fixable in 1 hour, revert and escalate

### Performance Regression (> 10%)
**Trigger:** Benchmark shows > 10% slower  
**Recovery:** (1) Profile hotspot (2) Optimize or revert  
**Action:** Must be < 5% before Phase 5 approval

---

## [OK] Issue Resolution Criteria

**DONE** when ALL:
- [OK] Phase 0: Validation complete
- [OK] Phase 1: 100% gaps catalogued with precision
- [OK] Phase 2: All gaps mapped to tasks, plan validated
- [OK] Phase 3: All CRITICAL/HIGH gaps fixed, ≥ 80% MEDIUM fixed
- [OK] Phase 4: 100% automated tests pass
- [OK] Phase 5: Human code review approved
- [OK] Phase 6: Documentation complete
- [OK] Phase 7: Merged to develop

---

## 📖 Additional Resources

- **Workflow Template:** [ISSUE_WORKFLOW_TEMPLATE.md](tools/ISSUE_WORKFLOW_TEMPLATE.md)
- **Gap Scanner:** `tools/gap_scanner_v3.py`
- **Quality Gate:** `scripts/quality-gate.ps1`
- **Build Commands:** See workspace README

---

**Labels:** gap-scanner, {module}, ready-for-ai-agent, 7phase-workflow  
**Last Updated:** {self._get_date()}
"""
        
        return body
    
    def _get_date(self) -> str:
        """Get current date in YYYY-MM-DD format"""
        from datetime import datetime
        return datetime.now().strftime('%Y-%m-%d')
    
    def update_issue_on_github(self, issue_number: int, module: str, existing_body: str) -> bool:
        """Update single issue on GitHub"""
        
        new_body = self.generate_new_issue_body(module, existing_body)
        
        if self.dry_run:
            print(f"\n📝 [DRY-RUN] Would update issue #{issue_number} ({module})")
            print("=" * 80)
            print(new_body[:500] + "\n...[truncated]")
            print("=" * 80)
            return True
        
        if not self.gh_token:
            print(f"[ERROR] GH_TOKEN not set. Cannot update issue #{issue_number}")
            return False
        
        # Use GitHub CLI to update issue
        cmd = [
            'gh', 'issue', 'edit', str(issue_number),
            '--repo', f'{self.repo_owner}/{self.repo_name}',
            '--body', new_body
        ]
        
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            print(f"[OK] Updated issue #{issue_number} ({module})")
            return True
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] Failed to update issue #{issue_number}: {e.stderr}")
            return False
    
    def migrate_all_issues(self, specific_module: Optional[str] = None) -> Dict[str, Any]:
        """Migrate all existing gap remediation issues"""
        
        print("\n" + "=" * 80)
        print("[MIGRATE] Migrating Existing Issues to 7-Phase Workflow")
        print("=" * 80)
        
        if self.dry_run:
            print("[WARN] [DRY-RUN MODE] — No actual GitHub changes will be made\n")
        
        # List existing issues
        print("\n[INFO] Fetching existing gap-remediation issues from GitHub...")
        existing_issues = self.list_existing_issues()
        
        if not existing_issues:
            print("[INFO] No gap-remediation issues found (or GitHub CLI not available)")
            print("[TIP] Set GH_TOKEN and ensure 'gh' CLI is installed\n")
            return {
                'total': 0,
                'updated': 0,
                'failed': 0,
                'skipped': 0
            }
        
        print(f"Found {len(existing_issues)} existing issues\n")
        
        stats = {
            'total': len(existing_issues),
            'updated': 0,
            'failed': 0,
            'skipped': 0
        }
        
        # Update each issue
        for issue in existing_issues:
            issue_num = issue['number']
            title = issue['title']
            
            # Extract module name
            module = self.parse_module_from_title(title)
            
            if not module:
                print(f"⏭️  Issue #{issue_num}: Could not parse module from title (skipping)")
                stats['skipped'] += 1
                continue
            
            # Filter by specific module if requested
            if specific_module and module != specific_module.lower():
                print(f"⏭️  Issue #{issue_num}: Filtered (not {specific_module})")
                stats['skipped'] += 1
                continue
            
            # Update issue
            if self.update_issue_on_github(issue_num, module, issue.get('body', '')):
                stats['updated'] += 1
            else:
                stats['failed'] += 1
        
        # Summary
        print("\n" + "=" * 80)
        print("📊 Migration Summary")
        print("=" * 80)
        print(f"Total issues found:     {stats['total']}")
        print(f"Updated:                {stats['updated']} [OK]")
        print(f"Failed:                 {stats['failed']} [ERROR]")
        print(f"Skipped:                {stats['skipped']} ⏭️")
        print(f"\nMode:                   {'DRY-RUN (no changes)' if self.dry_run else 'LIVE (changes applied)'}")
        print("=" * 80 + "\n")
        
        return stats


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Migrate existing issues to 7-phase workflow')
    parser.add_argument('aggregate', help='Path to gap_scan_v3_aggregate.json')
    parser.add_argument('--github', action='store_true', help='Actually update issues on GitHub')
    parser.add_argument('--dry-run', action='store_true', help='Show what would change (default if no --github)')
    parser.add_argument('--module', help='Update only specific module')
    
    args = parser.parse_args()
    
    # Default to dry-run if --github not specified
    if not args.github:
        args.dry_run = True
    
    try:
        migrator = GitHubIssueMigrator(args.aggregate, dry_run=args.dry_run)
        stats = migrator.migrate_all_issues(specific_module=args.module)
        
        if stats['failed'] > 0:
            sys.exit(1)
    except Exception as e:
        print(f"[ERROR] Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()

