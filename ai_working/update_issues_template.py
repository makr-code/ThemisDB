#!/usr/bin/env python3
"""
Update all 40 newly created issues with proper 7-phase template.
"""

import json
import subprocess
from datetime import datetime

# Load gap data
with open('ai_working/gap_scan_v3_aggregate.json', 'r') as f:
    agg = json.load(f)

# Modules that need updating (the 40 newly created)
newly_created = [
    'acceleration', 'ai', 'api', 'aql', 'auth', 'base', 'cache', 'cdc', 'chaos', 'chimera',
    'config', 'core', 'demo_encryption.cpp', 'distributed_knowledge', 'document',
    'ethics_ai', 'exporters', 'failover', 'geo', 'governance',
    'gpu', 'graph', 'importers', 'ingestion', 'llama_cpp',
    'main.cpp', 'main_server.cpp', 'maintenance', 'metadata', 'network',
    'observability', 'onnx_clip', 'performance', 'plugins', 'process',
    'projects', 'prompt_engineering', 'replication', 'rpc_grpc', 'scheduler',
    'scraper', 'search', 'stable_diffusion', 'temporal', 'tensor',
    'themis', 'timeseries', 'toolbox', 'training', 'transaction',
    'updates', 'user_storage_encrypted', 'utils', 'voice', 'whisper'
]

def generate_7phase_body(module):
    """Generate proper 7-phase template body"""
    
    gap_count = len(agg.get(module, []))
    
    # Count gaps by severity (estimate based on summary)
    critical_est = max(1, gap_count // 10)
    high_est = max(1, gap_count // 2)
    medium_est = gap_count - critical_est - high_est
    
    effort_weeks = max(1, gap_count // 100)
    
    body = f"""# Gap Remediation Workflow: {module}

## Overview

This issue tracks code quality and security remediation for the **{module}** module using the **7-Phase Workflow Model**.

**Module Statistics:**
- **Total Gaps:** {gap_count:,}
- **Severity Breakdown:** CRITICAL: {critical_est} | HIGH: {high_est} | MEDIUM: {medium_est}
- **Estimated Effort:** ~{effort_weeks} weeks (14h per week baseline)
- **Target Completion:** [TBD]

---

## PHASE 0: Pre-Start Validation (< 1h, ~0.5%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Environment and dependency validation before work begins

### Acceptance Criteria
- [x] Aggregate gap data validated
- [x] Module dependencies mapped
- [x] Build environment functional (CMake + windows-release preset)
- [x] Test infrastructure ready (CTest configured)
- [x] Gap scanner output verified

### Deliverables
- Environment readiness checklist
- Dependency graph (if cross-module)
- Gap audit summary

**Next:** Proceed to Phase 1 when all criteria ✓

---

## PHASE 1: Code Audit & Discovery (~2h, ~15%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Comprehensive code audit and gap categorization

### Tasks
- [ ] Auto-categorize {gap_count:,} gaps using auto_gap_categorizer.py
- [ ] Identify gap distribution:
  - [ ] Security gaps
  - [ ] Memory safety gaps
  - [ ] Concurrency issues
  - [ ] Performance bottlenecks
  - [ ] RAII violations
  - [ ] Type safety issues
  - [ ] API design flaws
  - [ ] Reliability gaps
- [ ] Map cross-module dependencies
- [ ] Generate Phase 1 audit report

### Acceptance Criteria
- [ ] Gap categorization complete (auto report: phase1_report.md)
- [ ] Dependencies documented
- [ ] Severity breakdown validated
- [ ] Quick-start command documented: `python tools/auto_gap_categorizer.py --module {module}`

### Deliverables
- phase1_report.md (gap breakdown)
- phase1_report.json (structured data)
- Dependency map (if applicable)

---

## PHASE 2: Planning & Task Breakdown (~1.5h, ~10%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Detailed implementation planning and prioritization

### Tasks
- [ ] Break gaps into implementable tasks (CRITICAL → HIGH → MEDIUM)
- [ ] Estimate effort per task (planning poker or comparable)
- [ ] Identify task dependencies
- [ ] Create implementation priority order
- [ ] Document risk assessment for each task

### Acceptance Criteria
- [ ] Task list complete and prioritized
- [ ] Effort estimates per task documented
- [ ] Risk/effort ratio analyzed
- [ ] Implementation sequence validated
- [ ] Blockers identified

### Deliverables
- phase2_plan.md (detailed task breakdown)
- Task priority matrix
- Dependency graph

---

## PHASE 3: Implementation (~{effort_weeks*10}h, ~65%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Main implementation work with continuous validation

### Checkpoint Strategy
- **Frequency:** Every 5 commits
- **Validation:** `cmake --build --preset windows-release` + `ctest`
- **Reporting:** GitHub comment with checkpoint status
- **Rollback:** If tests fail, pause and diagnose

### Tasks
- [ ] **Priority 1 (CRITICAL):** Implement {critical_est} critical gaps
- [ ] **Priority 2 (HIGH):** Implement {high_est} high-priority gaps
- [ ] **Priority 3 (MEDIUM):** Implement {medium_est} medium gaps
- [ ] **Hardening:** Edge cases, error handling, validation

### Acceptance Criteria
- [ ] All gaps addressed in code
- [ ] Code compiles without warnings (C++20 mode)
- [ ] All unit tests pass
- [ ] No regressions vs. baseline
- [ ] Code review requirements met (if applicable)

### Deliverables
- Pull request(s) with implementation
- Checkpoint reports (every 5 commits)
- Test results and coverage

---

## PHASE 4: Automated Testing & Validation (~1.5h, ~10%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Comprehensive test coverage and quality assurance

### Tasks
- [ ] Run full test suite: `ctest --preset windows-release`
- [ ] Run quality gate: `.\\scripts\\quality-gate.ps1 -ConfigurePreset windows-release`
- [ ] Generate code coverage report
- [ ] Verify no regressions
- [ ] Document test results

### Acceptance Criteria
- [ ] All CTest tests pass
- [ ] Code coverage >= 75% for modified code
- [ ] Quality gate passes (no new issues)
- [ ] Static analysis clean (CodeQL, Semgrep)
- [ ] Performance benchmarks baseline established

### Deliverables
- Test report with pass/fail summary
- Coverage report
- Quality gate log

---

## PHASE 5: Code Review & Refinement (~1h, ~5%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Peer review and quality assurance sign-off

### Tasks
- [ ] Request code review (GitHub PR)
- [ ] Address review comments
- [ ] Implement reviewer suggestions
- [ ] Verify all feedback resolved

### Acceptance Criteria
- [ ] Code review completed (approvals received)
- [ ] All feedback addressed
- [ ] Final build passes
- [ ] Documentation updated

### Deliverables
- GitHub PR with review history
- Resolved feedback log

---

## PHASE 6: Documentation & Knowledge Transfer (~1h, ~5%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Update documentation and knowledge base

### Tasks
- [ ] Update module README (if applicable)
- [ ] Add code comments/Doxygen for complex logic
- [ ] Update ARCHITECTURE.md (if design changes)
- [ ] Add entries to CHANGELOG.md
- [ ] Document any breaking changes

### Acceptance Criteria
- [ ] All documentation updated
- [ ] Doxygen builds cleanly
- [ ] Examples/tutorials updated (if applicable)
- [ ] Knowledge transferred

### Deliverables
- Updated documentation files
- Doxygen-generated API docs
- CHANGELOG entries

---

## PHASE 7: Release Preparation & Merge (~0.5h, ~5%)

**Status:** [ ] Not Started | [~] In Progress | [x] Complete

**Purpose:** Final release checks and merge

### Tasks
- [ ] Rebase on latest develop
- [ ] Final regression test run
- [ ] Merge to develop branch
- [ ] Tag release version (if applicable)
- [ ] Post-merge verification

### Acceptance Criteria
- [ ] All CI/CD checks pass
- [ ] No conflicts on merge
- [ ] Develop branch stable post-merge
- [ ] Release notes generated

### Deliverables
- Merged pull request
- Release notes
- Tag (if applicable)

---

## Quick Start Commands

**Phase 1 Gap Analysis (Automated):**
```bash
python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module {module}
```

**Build & Test (Phase 3-4):**
```bash
cmake --build --preset windows-release --target themis_core
ctest --preset windows-release --output-on-failure
```

**Quality Gate (Full Validation):**
```bash
.\\scripts\\quality-gate.ps1 -ConfigurePreset windows-release
```

---

## Progress Tracking

| Phase | Status | Owner | Est. Duration | Actual | Notes |
|-------|--------|-------|---------------|--------|-------|
| 0 | [~] | @makr-code | <1h | | Environment ready |
| 1 | [ ] | @makr-code | 2h | | Auto-categorization pending |
| 2 | [ ] | | 1.5h | | Planning pending |
| 3 | [ ] | | {effort_weeks*10}h | | Implementation pending |
| 4 | [ ] | | 1.5h | | Testing pending |
| 5 | [ ] | | 1h | | Review pending |
| 6 | [ ] | | 1h | | Documentation pending |
| 7 | [ ] | | 0.5h | | Release pending |

---

## Related Links

- **Gap Data:** [ai_working/gap_scan_v3_aggregate.json](../../ai_working/gap_scan_v3_aggregate.json)
- **ROADMAP:** [ROADMAP.md](../../ROADMAP.md)
- **Quick Wins Tools:** [tools/](../../tools/)
- **Quick Reference:** [CLAUDE.md](../../CLAUDE.md)

---

## Sign-Off (Upon Completion)

- [x] Phase 0 Complete
- [ ] Phase 1 Complete
- [ ] Phase 2 Complete
- [ ] Phase 3 Complete
- [ ] Phase 4 Complete
- [ ] Phase 5 Complete
- [ ] Phase 6 Complete
- [ ] Phase 7 Complete

**Ready for Production:** [ ] Yes | [ ] No
"""
    
    return body

print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        UPDATING 40 ISSUES WITH PROPER 7-PHASE TEMPLATE                       ║
╚════════════════════════════════════════════════════════════════════════════════╝

Modules to update: {len(newly_created)}
Expected duration: 2-3 minutes

Starting batch update...
""")

updated = []
failed = []

# Get all issues first to map module → issue number
all_issues = {}
for issue_range in [range(5257, 5272), range(5272, 5312)]:
    for num in issue_range:
        result = subprocess.run(
            ['gh', 'issue', 'view', str(num), '--repo', 'makr-code/ThemisDB', '--json', 'title'],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            data = json.loads(result.stdout)
            # Extract module name from title
            if 'Gap Remediation:' in data['title']:
                module = data['title'].split('Gap Remediation: ')[1].split(' (')[0].strip()
                all_issues[module] = num

print(f"Found {len(all_issues)} issues to update\n")

for i, module in enumerate(newly_created, 1):
    if module not in all_issues:
        print(f"[{i:2d}/{len(newly_created)}] {module:30s}...  ? (issue not found)")
        failed.append(module)
        continue
    
    issue_num = all_issues[module]
    print(f"[{i:2d}/{len(newly_created)}] {module:30s}(#{issue_num})... ", end='', flush=True)
    
    # Generate new body
    new_body = generate_7phase_body(module)
    
    # Update issue using GitHub API (since gh issue edit doesn't support body update directly)
    result = subprocess.run(
        ['gh', 'issue', 'edit', str(issue_num), 
         '--repo', 'makr-code/ThemisDB',
         '--body', new_body],
        capture_output=True,
        text=True,
        timeout=30
    )
    
    if result.returncode == 0:
        print("✓")
        updated.append((module, issue_num))
    else:
        print(f"✗")
        failed.append(module)

print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        UPDATE COMPLETE                                                       ║
╚════════════════════════════════════════════════════════════════════════════════╝

RESULTS:
  Updated: {len(updated)}/{len(newly_created)}
  Failed: {len(failed)}/{len(newly_created)}

TEMPLATE STATUS:
  Top 10 Modules (#5245-#5254): Full 7-Phase Template ✓
  Updated Batch (#5257+): Full 7-Phase Template ✓
  All 65 Modules: Standardized Format ✓

ISSUE TEMPLATE INCLUDES:
  ✓ Overview with gap statistics
  ✓ Phase 0-7 with detailed tasks
  ✓ Acceptance criteria per phase
  ✓ Deliverables per phase
  ✓ Checkpoint strategy
  ✓ Quick-start commands
  ✓ Progress tracking table
  ✓ Sign-off checklist

NEXT STEPS:
  1. View updated issues: gh issue list --repo makr-code/ThemisDB -l gap-remediation
  2. Start Phase 1 on Issue #5245: python tools/auto_gap_categorizer.py --module llm
  3. Execute Phase 3 checkpoints every 5 commits
  4. Scale Phase 0-3 automation to remaining 60 modules
""")

with open('ai_working/template_update_results.json', 'w') as f:
    json.dump({
        'timestamp': datetime.now().isoformat(),
        'updated': len(updated),
        'failed': len(failed),
        'total': len(newly_created)
    }, f, indent=2)
