#!/usr/bin/env python3
"""Find the correct GitHub issue for each module"""

import json
import subprocess

# Get all gap-remediation issues
result = subprocess.run(
    ['gh', 'issue', 'list', '--search', 'label:gap-remediation', '--json', 'number,title', '--limit', '100'],
    capture_output=True, text=True
)
gh_issues = json.loads(result.stdout)

# Extract module names from issue titles
issue_to_module = {}
for issue in gh_issues:
    # Title format: "Gap Remediation: <module_name> (N gaps)"
    title = issue['title']
    if 'Gap Remediation:' in title:
        module_name = title.split('Gap Remediation:')[1].split('(')[0].strip()
        issue_to_module[issue['number']] = module_name
        if module_name.lower() == 'index':
            print(f"FOUND INDEX: #{issue['number']} - {title}")

# If not found, show all issues sorted by issue number
if not any(m.lower() == 'index' for m in issue_to_module.values()):
    print("\nINDEX not found. All gap-remediation issues:")
    for issue in sorted(gh_issues, key=lambda x: x['number']):
        module = issue_to_module.get(issue['number'], 'UNKNOWN')
        print(f"  #{issue['number']:4d} - {module:30s} - {issue['title']}")
