#!/usr/bin/env python3
"""Generate GitHub issues from impact-classified findings"""

import json
from pathlib import Path
from collections import defaultdict

def create_github_issue(subsystem, impact, severity, findings_count, example_findings):
    """Generate GitHub issue template"""
    
    priority_map = {
        ('CRITICAL', 'CRITICAL'): '🔴 P0 - CRITICAL',
        ('CRITICAL', 'HIGH'): '🟠 P0 - CRITICAL/HIGH',
        ('HIGH', 'CRITICAL'): '🟡 P1 - HIGH/CRITICAL',
        ('HIGH', 'HIGH'): '🟡 P1 - HIGH/HIGH',
    }
    
    priority = priority_map.get((severity, impact), '⚪ P2 - Medium/Low')
    
    issue_title = f"{priority} [{subsystem.upper()}] Fix {findings_count} {severity} findings with {impact} impact"
    
    issue_body = f"""## Problem
**Module**: `{subsystem}`  
**Severity**: {severity}  
**Impact**: {impact}  
**Findings Count**: {findings_count}  

This issue groups findings from AI-Vibe scanners related to:
- Production logic errors (TODO markers in control flow)
- Stub/mock code leaks from testing
- LLM prompt injection vulnerabilities
- Error handling inconsistencies
- API documentation gaps

## Impact Analysis
- **Severity**: How bad is the finding? (How much does it break?)
- **Impact**: Where does it occur? (Which critical modules are affected?)

## Example Findings
"""
    
    for i, finding in enumerate(example_findings[:5], 1):
        issue_body += f"""
### {i}. {finding['type']}
- **File**: `{finding['file']}:{finding['line']}`
- **Severity**: {finding['severity']}
- **Impact**: {finding['impact_level']} ({finding['subsystem']})
- **Description**: {finding['description']}
- **Remediation**: {finding['remediation']}
"""
    
    issue_body += f"""

## Remediation
See [IMPACT_REMEDIATION_ROADMAP.md](./IMPACT_REMEDIATION_ROADMAP.md) for detailed guidance.

### Acceptance Criteria
- [ ] All findings of this type reviewed and classified as real/false positive
- [ ] Root cause identified for each real finding
- [ ] Implementation plan created
- [ ] Unit tests added for fixed code
- [ ] No regressions introduced

### Labels
- `ai-vibe-{severity.lower()}`
- `impact-{impact.lower()}`
- `module-{subsystem}`
- `findings-{findings_count}`

### Linked Findings
See attached CSV file for all {findings_count} findings in this category.
"""
    
    return {
        'title': issue_title,
        'body': issue_body,
        'labels': [
            f'ai-vibe-{severity.lower()}',
            f'impact-{impact.lower()}',
            f'module-{subsystem}',
            'impact-based-classification',
        ],
        'priority': priority,
    }

def generate_from_scan(scan_filepath):
    """Load scan and generate issues"""
    if not Path(scan_filepath).exists():
        print(f"❌ Scan file not found: {scan_filepath}")
        return
    
    with open(scan_filepath) as f:
        data = json.load(f)
    
    gaps = data.get('gaps', [])
    
    # Group by (subsystem, impact, severity)
    grouped = defaultdict(list)
    for g in gaps:
        key = (g.get('subsystem', 'UNKNOWN'), g.get('impact_level', 'UNKNOWN'), g.get('severity', 'MEDIUM'))
        grouped[key].append(g)
    
    print(f"\n{'='*80}")
    print("GITHUB ISSUES TEMPLATES")
    print(f"{'='*80}\n")
    
    issues = []
    for (subsystem, impact, severity), findings in sorted(grouped.items(), 
                                                           key=lambda x: -len(x[1])):
        if len(findings) > 0:
            issue = create_github_issue(subsystem, impact, severity, len(findings), findings)
            issues.append(issue)
            
            print(f"Issue: {issue['title']}")
            print(f"Labels: {', '.join(issue['labels'])}")
            print(f"Finding Count: {len(findings)}")
            print(f"Body Preview: {issue['body'][:200]}...")
            print()
    
    # Save as JSON for bulk creation
    with open('ai_working/github_issues_templates.json', 'w') as f:
        json.dump(issues, f, indent=2)
    
    print(f"\n✅ Generated {len(issues)} issue templates")
    print(f"Saved to: ai_working/github_issues_templates.json")
    
    return issues

if __name__ == '__main__':
    # Try latest scan
    for scan_file in ['ai_working/scan_src_with_impact.json', 
                      'ai_working/scan_graph_impact_fixed.json']:
        if Path(scan_file).exists():
            print(f"\n📋 Generating GitHub issues from: {scan_file}")
            issues = generate_from_scan(scan_file)
            break
    else:
        print("❌ No scan results found")
