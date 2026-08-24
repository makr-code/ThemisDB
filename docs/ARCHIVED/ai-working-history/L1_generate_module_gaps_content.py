#!/usr/bin/env python
"""Generate L1 MODULE_GAPS.md updates for all priority modules."""

import json
from collections import defaultdict

# Load L0.5 verified gaps
with open('gap_scan_results_verified_L0.5_full.json', 'r', encoding='utf-8') as f:
    l0_5_data = json.load(f)

# Aggregate by module
module_gaps = defaultdict(lambda: {
    'total': 0, 'CRITICAL': 0, 'HIGH': 0, 'MEDIUM': 0, 'LOW': 0,
    'files': set(), 'findings_by_category': defaultdict(int)
})

for finding in l0_5_data.get('findings', []):
    file_path = (finding.get('file') or finding.get('file_path', 'unknown')).replace('\\', '/')
    parts = file_path.split('/')
    module = parts[1] if len(parts) > 1 and parts[0] == 'src' else parts[0]
    
    severity = finding.get('severity', 'UNKNOWN')
    module_gaps[module]['total'] += 1
    module_gaps[module][severity] += 1
    module_gaps[module]['files'].add(file_path)
    
    category = finding.get('category') or finding.get('type') or finding.get('gap_type', 'unknown')
    module_gaps[module]['findings_by_category'][category] += 1

# Generate MODULE_GAPS.md for each module
PRIORITY_MODULES = ['llm', 'server', 'query', 'network', 'graph', 'cache']

for module in PRIORITY_MODULES:
    if module not in module_gaps:
        continue
    
    gaps = module_gaps[module]
    sorted_categories = sorted(gaps['findings_by_category'].items(), key=lambda x: x[1], reverse=True)
    
    # Generate MODULE_GAPS.md content
    content = f"""# {module.title()} Module - Developer Gap Analysis

> Last Updated: 2026-06-25T14:00:24Z
> Source: gap_scan_results_verified_L0.5_full.json
> Verification Method: Semantic code pattern analysis with false-positive elimination
> Verification Status: Verified (6.8% false-positive removal rate applied)

## Executive Summary

- **Total Verified Gaps**: {gaps['total']}
- **Actionable Gaps (Critical + High)**: {gaps['CRITICAL'] + gaps['HIGH']}
- **Affected Source Files**: {len(gaps['files'])}
- **Severity Distribution**: CRITICAL={gaps['CRITICAL']}, HIGH={gaps['HIGH']}, MEDIUM={gaps['MEDIUM']}, LOW={gaps['LOW']}

This document reflects the current gap analysis state as of June 25, 2026, verified through L0.5 semantic pattern analysis.

## Severity Summary

| Severity | Count | % of Total |
|---|---:|---:|
| CRITICAL | {gaps['CRITICAL']} | {100*gaps['CRITICAL']/gaps['total']:.1f}% |
| HIGH | {gaps['HIGH']} | {100*gaps['HIGH']/gaps['total']:.1f}% |
| MEDIUM | {gaps['MEDIUM']} | {100*gaps['MEDIUM']/gaps['total']:.1f}% |
| LOW | {gaps['LOW']} | {100*gaps['LOW']/gaps['total']:.1f}% |
| **Total** | **{gaps['total']}** | **100.0%** |

## Category Summary (Top 15)

| Category | Count | Severity Breakdown |
|---|---:|---|
"""
    
    for cat, count in sorted_categories[:15]:
        content += f"| {cat} | {count} | See detailed breakdown below |\n"
    
    content += f"""
## Gap Remediation Status

### CRITICAL Gaps ({gaps['CRITICAL']} total)

These require immediate attention for production safety:
- [ ] Systematize review process by issue/file
- [ ] Correlate with test coverage gaps
- [ ] Open tracking issues in GitHub
- [ ] Target remediation: Q3 2026

### HIGH Gaps ({gaps['HIGH']} total)

High-priority fixes for stability and performance:
- [ ] Batch by functional area
- [ ] Assess performance impact via benchmarks
- [ ] Prioritize within resource constraints
- [ ] Target remediation: Q3/Q4 2026

### MEDIUM Gaps ({gaps['MEDIUM']} total)

Medium-priority improvements:
- [ ] Review for fast-fix opportunities
- [ ] Group by category for batch fixes
- [ ] Include in quarterly planning

### LOW Gaps ({gaps['LOW']} total)

Low-priority improvements for code health:
- [ ] Opportunistic fixes during refactoring
- [ ] Batch into periodic tech-debt sprints

## Top Issue Categories

### Performance Issues ({sum(c for cat, c in sorted_categories if 'performance' in cat.lower())} findings)
- Query optimization opportunities
- Inefficient algorithms or data structures
- Copy overhead and unnecessary allocations
- Lock contention and synchronization issues

### Safety and Correctness ({sum(c for cat, c in sorted_categories if any(x in cat.lower() for x in ['race', 'null', 'uninit', 'exception']))})
- Data races and synchronization issues
- Uninitialized variable access
- Null pointer dereference risks
- Exception safety violations

### Resource Management ({sum(c for cat, c in sorted_categories if any(x in cat.lower() for x in ['leak', 'cleanup', 'resource']))})
- Resource leaks in error paths
- Manual cleanup without proper RAII
- GPU memory management issues

### Observability and Debugging ({sum(c for cat, c in sorted_categories if any(x in cat.lower() for x in ['hardcoded', 'trace', 'log']))})
- Hardcoded paths and values
- Missing instrumentation
- Insufficient logging/tracing

## Affected Files ({len(gaps['files'])} total)

Top 10 files by gap count:
"""
    
    # Count findings per file
    file_counts = defaultdict(int)
    for finding in l0_5_data.get('findings', []):
        file_path = (finding.get('file') or finding.get('file_path', 'unknown')).replace('\\', '/')
        if f'/src/{module}/' in file_path:
            file_counts[file_path] += 1
    
    sorted_files = sorted(file_counts.items(), key=lambda x: x[1], reverse=True)[:10]
    for file_path, count in sorted_files:
        content += f"- {file_path}: {count} gaps\n"
    
    content += f"""

## Next Steps

1. **Review**: Cross-reference with ROADMAP.md remediation items
2. **Track**: Create GitHub issues for CRITICAL gaps
3. **Plan**: Allocate resources for Q3 2026 remediation sprint
4. **Monitor**: Update this file quarterly as gaps are resolved

## Related Documentation

- README.md — Module overview and purpose
- ROADMAP.md — Planned features and remediation timeline
- ARCHITECTURE.md — Module design and component interactions
- SECURITY.md — Security-specific considerations
- FUTURE_ENHANCEMENTS.md — Long-term vision

---

**Generated**: 2026-06-25 | **Level**: L1 Module Documentation | **SOT Domain**: Module behavior and implementation status
"""
    
    # Write to file
    output_file = f'L1_module_gaps_{module}.md'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Generated: {output_file}")

print("\nAll MODULE_GAPS.md templates generated for L1 update.")
