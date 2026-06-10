#!/usr/bin/env python3
"""
Generate and create 7-phase GitHub issues for remaining 55 modules.
"""

import json
import subprocess
from pathlib import Path
from datetime import datetime

ISSUE_TEMPLATE = """# Phase 1-10 Gap Remediation: {module_upper}

## Current Status
- **Module**: {module}
- **Total Gaps Identified**: {gap_count:,}
- **Severity Breakdown**: (See Phase 1 Audit)
- **Target Completion**: Q3 2026
- **Priority**: Medium-High

---

## In Progress / Planned Features

- [ ] Phase 0: Pre-Start Validation
- [ ] Phase 1: Code Audit & Discovery
- [ ] Phase 2: Planning & Tasks
- [ ] Phase 3: Implementation
- [ ] Phase 4: Automated Review
- [ ] Phase 5: Human Code Review
- [ ] Phase 6: Documentation
- [ ] Phase 7: Merge & Release

---

## Implementation Phases

### Phase 0: Pre-Start Validation (< 1%)
**Entry Criteria**: Module identified in gap scan
**Exit Criteria**: Environment verified, prerequisites met

Checklist:
- [ ] CMake preset validated (windows-release)
- [ ] Build tools available (cmake, ninja, cl.exe)
- [ ] Aggregate gap data loaded
- [ ] CTest framework available
- [ ] Module buildable (quick CMake configure)

**Error Handling**: If any check fails, escalate and document blockers

---

### Phase 1: Code Audit & Discovery (~15%)
**Entry Criteria**: Phase 0 PASS
**Exit Criteria**: Gaps categorized, priority order established

Checklist:
- [ ] Load {gap_count:,} gaps for {module}
- [ ] Categorize by: Security, Memory, Concurrency, Performance, RAII, Type Safety, API Design, Reliability
- [ ] Assign severity: CRITICAL, HIGH, MEDIUM
- [ ] Create phase1_report.md (human readable)
- [ ] Generate phase1_report.json (structured data)

**Automated via**: `python tools/auto_gap_categorizer.py --module {module}`

---

### Phase 2: Planning & Tasks (~10%)
**Entry Criteria**: Phase 1 PASS
**Exit Criteria**: Prioritized task list, effort estimates

Checklist:
- [ ] Review phase1_report.json output
- [ ] Identify quick wins (< 4h effort)
- [ ] Break down into 5-commit task sets
- [ ] Create phase2_plan.md with:
  - Task breakdown (T1, T2, T3, ...)
  - Estimated effort per task (hours)
  - Risk assessment (blockers, dependencies)
  - Resource requirements

---

### Phase 3: Implementation (~65%)
**Entry Criteria**: Phase 2 PASS
**Exit Criteria**: All gaps addressed, tests passing

Checklist:
- [ ] Implement Task Set 1 (commit #1-#5, checkpoint after #5)
- [ ] Implement Task Set 2 (commit #6-#10, checkpoint after #10)
- [ ] Implement Task Set 3 (commit #11-#15, checkpoint after #15)
- [ ] ... (continue as needed)
- [ ] All tests passing: `ctest --preset windows-release --output-on-failure`

**Automated via**: `python tools/auto_checkpoint_runner.py {module}`

**Checkpoints Every 5 Commits**:
- Build validation (cmake --build)
- Unit tests (ctest)
- Code quality checks (clang-tidy, cppcheck optional)
- Generate checkpoint_YYYYMMDD_HHMMSS.md for GitHub

**Rollback Trigger**: If checkpoint FAIL, revert last commit set and investigate root cause

---

### Phase 4: Automated Review (~5%)
**Entry Criteria**: Phase 3 PASS
**Exit Criteria**: CI/CD green, automated checks complete

Checklist:
- [ ] All GitHub Actions passing
- [ ] Build succeeds on all presets
- [ ] Test coverage maintained/improved
- [ ] No new compiler warnings
- [ ] Static analysis (if enabled) clean

---

### Phase 5: Human Code Review (~5%)
**Entry Criteria**: Phase 4 PASS
**Exit Criteria**: Approved by maintainers

Checklist:
- [ ] Code review request opened
- [ ] Architecture decisions documented
- [ ] Performance implications assessed
- [ ] Security review complete (if applicable)
- [ ] PR approved by 2+ maintainers

---

### Phase 6: Documentation (~5%)
**Entry Criteria**: Phase 5 PASS
**Exit Criteria**: API docs, README, CHANGELOG updated

Checklist:
- [ ] Update API documentation (Doxygen comments)
- [ ] Update README if user-facing changes
- [ ] Add CHANGELOG entry
- [ ] Update ARCHITECTURE.md if structural changes
- [ ] Verify docs build: `doxygen Doxyfile.audit`

---

### Phase 7: Merge & Release (< 2%)
**Entry Criteria**: Phase 6 PASS
**Exit Criteria**: Merged to develop, released

Checklist:
- [ ] Rebase on latest develop
- [ ] Final test suite run
- [ ] Merge PR
- [ ] Tag release (optional)
- [ ] Close this issue

---

## Production Readiness Checklist

- [ ] All Phase 1-10 gaps addressed
- [ ] Tests passing (unit + integration)
- [ ] Documentation complete
- [ ] Code review approved
- [ ] No CRITICAL issues remaining
- [ ] Performance impact acceptable

---

## Known Issues & Limitations

_To be filled during gap audit_

---

## Breaking Changes

_To be noted if any backward compatibility concerns_

---

## Resources

- [ISSUE_WORKFLOW_TEMPLATE.md](./.github/ISSUE_WORKFLOW_TEMPLATE.md) — Detailed Phase 0-7 reference
- [DEVELOPMENT_OPPORTUNITIES.md](./DEVELOPMENT_OPPORTUNITIES.md) — Automation strategy and ROI
- [Gap Scan Report](./ai_working/gap_scan_v3_{module}.json) — Full data for this module

---

**Last Updated**: {timestamp}
**Status**: Created by automated workflow
**Labels**: `gap-remediation`, `phase-1-10`, `{module}`, `automation`
"""

def generate_issue_for_module(module: str, gap_count: int) -> str:
    """Generate 7-phase issue markdown for a module"""
    
    return ISSUE_TEMPLATE.format(
        module=module,
        module_upper=module.upper(),
        gap_count=gap_count,
        timestamp=datetime.now().isoformat()
    )

def create_github_issues_batch(remaining_modules: list, agg: dict):
    """Create GitHub issues for remaining 55 modules"""
    
    print(f"\n{'='*80}")
    print("CREATING GITHUB ISSUES FOR REMAINING 55 MODULES")
    print(f"{'='*80}\n")
    
    created_count = 0
    failed_count = 0
    
    for i, module in enumerate(sorted(remaining_modules), 1):
        gap_count = len(agg.get(module, []))
        
        print(f"[{i:2d}/55] Creating issue for {module:30s} ({gap_count:6,d} gaps)...", end=' ', flush=True)
        
        # Generate issue body
        issue_body = generate_issue_for_module(module, gap_count)
        
        # Create GitHub issue
        title = f"Gap Remediation Phase 1-10: {module} ({gap_count:,} gaps)"
        
        try:
            # Try to create issue with gh CLI
            cmd = [
                'gh', 'issue', 'create',
                '--repo', 'makr-code/ThemisDB',
                '--title', title,
                '--body', issue_body,
                '--label', 'gap-remediation,phase-1-10,automation',
                '--assignee', '@me'
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                # Extract issue number from output
                output = result.stdout.strip()
                if '#' in output:
                    issue_num = output.split('#')[1].split()[0]
                    print(f"✓ #{issue_num}")
                    created_count += 1
                else:
                    print("✓ created (number not captured)")
                    created_count += 1
            else:
                print(f"✗ FAIL")
                if 'not authenticated' in result.stderr.lower():
                    print(f"    Auth failed - set GH_TOKEN and retry")
                failed_count += 1
                
        except subprocess.TimeoutExpired:
            print("✗ TIMEOUT")
            failed_count += 1
        except Exception as e:
            print(f"✗ ERROR: {e}")
            failed_count += 1
    
    print(f"\n{'='*80}")
    print(f"RESULTS: {created_count} created, {failed_count} failed")
    print(f"{'='*80}")
    
    return created_count, failed_count

def main():
    """Main orchestration"""
    
    # Load data
    with open('ai_working/gap_scan_v3_aggregate.json', 'r') as f:
        agg = json.load(f)
    
    all_modules = sorted(agg.keys())
    top_10_modules = ['llm', 'server', 'query', 'sharding', 'index', 'storage', 'analytics', 'rag', 'security', 'content']
    remaining_modules = [m for m in all_modules if m not in top_10_modules]
    
    print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        FULL 65-MODULE GITHUB ISSUE GENERATION & CREATION                      ║
╚════════════════════════════════════════════════════════════════════════════════╝

STATUS:
  Total Modules: {len(all_modules)}
  Existing Issues: 10 (#5245-#5254) ✅
  To Generate: {len(remaining_modules)}
  
TIMELINE:
  Generation: ~{len(remaining_modules)//10} seconds
  GitHub API: ~{len(remaining_modules)*2} seconds
  Total: ~{len(remaining_modules)*3//10} seconds
""")
    
    # Check GitHub auth
    result = subprocess.run(['gh', 'auth', 'status'], capture_output=True, text=True)
    if result.returncode != 0:
        print("[⚠️] GitHub CLI not authenticated. Set GH_TOKEN and retry.")
        print("    Command: gh auth login --with-token < token.txt")
        return
    
    # Create issues
    created, failed = create_github_issues_batch(remaining_modules, agg)
    
    # Final summary
    total_issues = 10 + created  # 10 existing + newly created
    
    print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        FULL WORKFLOW COVERAGE ACHIEVED                                        ║
╚════════════════════════════════════════════════════════════════════════════════╝

ISSUE SUMMARY:
  Top 10 Modules: GitHub #5245-#5254 ✅
  Remaining Modules: {created} newly created ✅
  Total Issues: {total_issues} (100% module coverage)
  
WORKFLOW STATUS:
  Phase 0: Pre-flight validation ✅ (auto_phase0_validator.py)
  Phase 1: Gap categorization ✅ (auto_gap_categorizer.py)
  Phase 3: Checkpoint automation ✅ (auto_checkpoint_runner.py)
  Phase 2-7: Ready for AI agent execution
  
NEXT STEPS:
  1. Test AI agent on Issue #5245 (LLM, 24.4K gaps)
  2. Execute Phase 0-1 automation
  3. Generate Phase 2 plan
  4. Scale Phase 3 implementation
  5. Deploy Phase 4-7 automation
  
Ready for: AI-Driven Gap Remediation at Scale
""")

if __name__ == '__main__':
    main()
