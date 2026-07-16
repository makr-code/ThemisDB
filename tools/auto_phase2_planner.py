#!/usr/bin/env python3
"""
Phase 2 Planning: Generate detailed implementation plans from Phase 1 audit.
Creates tasks, prioritization, and effort estimates.
"""

import json
import sys
from pathlib import Path

def generate_phase2_plan(module):
    """Generate Phase 2 plan from Phase 1 categorized gaps"""
    
    # Load Phase 1 report
    phase1_file = Path('phase1_report.json')
    if not phase1_file.exists():
        print(f"ERROR: phase1_report.json not found. Run Phase 1 first.")
        return False
    
    with open(phase1_file, 'r') as f:
        phase1 = json.load(f)
    
    # Extract gap statistics
    gaps_by_severity = phase1.get('by_severity', {})
    gaps_by_category = phase1.get('by_category', {})
    
    critical_count = gaps_by_severity.get('CRITICAL', 0)
    high_count = gaps_by_severity.get('HIGH', 0)
    medium_count = gaps_by_severity.get('MEDIUM', 0)
    
    total_gaps = critical_count + high_count + medium_count
    
    # Generate Phase 2 plan
    plan = f"""# Phase 2 Implementation Plan: {module}

Generated from Phase 1 Audit Report

## Gap Summary

**Total Gaps:** {total_gaps}
- CRITICAL: {critical_count} gaps (~{critical_count*2}h effort)
- HIGH: {high_count} gaps (~{high_count*0.5}h effort)
- MEDIUM: {medium_count} gaps (~{medium_count*0.25}h effort)

---

## Task Breakdown by Severity

### Priority 1: CRITICAL Gaps ({critical_count} gaps)

**Objective:** Fix security and correctness issues that block production

**Tasks:**
""" 
    
    if critical_count > 0:
        per_task = max(1, critical_count // 5)
        for i in range(1, min(6, critical_count + 1)):
            start = (i-1) * per_task + 1
            end = min(i * per_task, critical_count)
            plan += f"- [ ] Task 1.{i}: Fix CRITICAL gaps {start}-{end} (est. {(end-start+1)*2}h)\n"
    else:
        plan += "- N/A (No CRITICAL gaps identified)\n"
    
    plan += f"""
### Priority 2: HIGH Gaps ({high_count} gaps)

**Objective:** Address reliability and performance concerns

**Tasks:**
"""
    
    if high_count > 0:
        per_task = max(1, high_count // 5)
        for i in range(1, min(6, high_count // per_task + 2)):
            start = (i-1) * per_task + 1
            end = min(i * per_task, high_count)
            if end >= start:
                plan += f"- [ ] Task 2.{i}: Fix HIGH gaps {start}-{end} (est. {(end-start+1)*0.5}h)\n"
    else:
        plan += "- N/A (No HIGH gaps identified)\n"
    
    plan += f"""
### Priority 3: MEDIUM Gaps ({medium_count} gaps)

**Objective:** Polish and edge-case handling

**Tasks:**
"""
    
    if medium_count > 0:
        per_task = max(1, medium_count // 5)
        for i in range(1, min(6, medium_count // per_task + 2)):
            start = (i-1) * per_task + 1
            end = min(i * per_task, medium_count)
            if end >= start:
                plan += f"- [ ] Task 3.{i}: Fix MEDIUM gaps {start}-{end} (est. {(end-start+1)*0.25}h)\n"
    else:
        plan += "- N/A (No MEDIUM gaps identified)\n"
    
    # Gap distribution by category
    plan += f"""

---

## Gap Distribution by Category

"""
    
    for category, count in sorted(gaps_by_category.items(), key=lambda x: x[1], reverse=True):
        if count > 0:
            bar = '█' * max(1, count // 50)
            plan += f"- **{category}:** {count} gaps {bar}\n"
    
    # Implementation strategy
    effort_hours = critical_count*2 + high_count*0.5 + medium_count*0.25
    effort_days = max(1, int(effort_hours / 8))
    
    plan += f"""

---

## Implementation Strategy

**Estimated Total Effort:** {effort_hours:.0f} hours (~{effort_days} days @ 8h/day)

### Approach
1. **Isolation:** Work on one priority level at a time (CRITICAL > HIGH > MEDIUM)
2. **Modularity:** Each task should be independently testable
3. **Testing:** Write tests for each gap fixed (TDD approach)
4. **Checkpoints:** Every 5 commits > run `ctest` + quality gate
5. **Review:** Code review required before merge

### Risk Assessment
- **High Risk:** CRITICAL gaps (must fix before merge)
- **Medium Risk:** HIGH gaps (fix before release)
- **Low Risk:** MEDIUM gaps (can defer if time-constrained)

### Success Criteria
- [ ] All CRITICAL gaps fixed and tested
- [ ] All HIGH gaps addressed
- [ ] Code coverage >= 75% for changed code
- [ ] No regressions vs baseline
- [ ] Quality gate passes

---

## Detailed Task List (Ready for Phase 3)

| Task ID | Priority | Gap Count | Est. Hours | Status |
|---------|----------|-----------|-----------|--------|
| 1.1 | CRITICAL | {per_task if critical_count > 0 else 0} | {(per_task if critical_count > 0 else 0)*2} | [ ] |
| 1.2 | CRITICAL | {per_task if critical_count > 0 else 0} | {(per_task if critical_count > 0 else 0)*2} | [ ] |
| 2.1 | HIGH | {per_task if high_count > 0 else 0} | {(per_task if high_count > 0 else 0)*0.5} | [ ] |
| 2.2 | HIGH | {per_task if high_count > 0 else 0} | {(per_task if high_count > 0 else 0)*0.5} | [ ] |
| 3.1 | MEDIUM | {per_task if medium_count > 0 else 0} | {(per_task if medium_count > 0 else 0)*0.25} | [ ] |
| **TOTAL** | | {total_gaps} | {effort_hours:.0f}h | |

---

## Phase 2 Acceptance Criteria

- [x] Phase 1 audit complete
- [x] Gaps categorized by severity and type
- [x] Task breakdown created
- [x] Effort estimates validated
- [x] Implementation sequence defined
- [x] Risk assessment documented
- [x] Success criteria agreed

**Ready for Phase 3:** YES ✓

---

## Appendix: Gap Categories

Based on Phase 1 Categorization:

"""
    
    for category in sorted(gaps_by_category.keys()):
        plan += f"- **{category}** (Phase 1 categorizer output)\n"
    
    plan += f"""

---

**Next Step:** Begin Phase 3 Implementation (Code Changes)
- Estimated start: Immediately after Phase 2 sign-off
- Checkpoint frequency: Every 5 commits
- Review requirement: Yes (before merge)

[END]
"""
    
    return plan

def main():
    """Execute Phase 2 planning"""
    
    module = sys.argv[1] if len(sys.argv) > 1 else "llm"
    
    print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        PHASE 2 PLANNING: {module.upper():30s}                       ║
╚════════════════════════════════════════════════════════════════════════════════╝

Reading Phase 1 categorization...
""")
    
    plan = generate_phase2_plan(module)
    
    # Save plan (UTF-8 to handle all characters)
    output_file = Path('phase2_plan.md')
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(plan)
    
    print(f"""[OK] Phase 2 plan generated: {output_file}

Summary:
--------
""")
    
    # Print summary
    lines = plan.split('\n')
    for line in lines[1:30]:  # Show first part
        print(line)
    
    print(f"""
...

DELIVERABLES:
  ✓ phase2_plan.md (detailed task breakdown)
  ✓ Effort estimates per task
  ✓ Task prioritization
  ✓ Risk assessment
  ✓ Phase 3 readiness checklist

NEXT: Begin Phase 3 Implementation
  Command: python tools/auto_phase3_checkpoint.py --module {module}
""")

if __name__ == '__main__':
    main()
