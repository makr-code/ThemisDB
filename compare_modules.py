#!/usr/bin/env python3
"""Compare Phase 2 modules with GitHub gap-remediation issues"""

import json
import subprocess

# Get all gap-remediation issues
result = subprocess.run(
    ['gh', 'issue', 'list', '--search', 'label:gap-remediation', '--json', 'number,title', '--limit', '100'],
    capture_output=True, text=True
)
gh_issues = json.loads(result.stdout)

# Extract module names from GitHub issues
gh_modules = set()
for issue in gh_issues:
    title = issue['title']
    if 'Gap Remediation:' in title:
        module_name = title.split('Gap Remediation:')[1].split('(')[0].strip()
        gh_modules.add(module_name)

# Load Phase 2 modules
phase2 = json.loads(open('ai_working/phase2_batch_results.json').read())
phase2_modules = set(phase2.keys())

# Compare
print("=== MODULE COMPARISON ===\n")
print(f"Phase 2 modules: {len(phase2_modules)}")
print(f"GitHub gap-remediation issues: {len(gh_modules)}\n")

missing_from_gh = phase2_modules - gh_modules
print(f"MODULES MISSING FROM GITHUB ({len(missing_from_gh)}):")
for m in sorted(missing_from_gh):
    print(f"  - {m}")

extra_on_gh = gh_modules - phase2_modules
if extra_on_gh:
    print(f"\nMODULES EXTRA ON GITHUB ({len(extra_on_gh)}):")
    for m in sorted(extra_on_gh):
        print(f"  + {m}")
