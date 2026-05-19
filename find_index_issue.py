#!/usr/bin/env python3
"""Find the correct GitHub issue for INDEX module"""

import json
import subprocess
import sys

result = subprocess.run(
    ['gh', 'issue', 'list', '--search', 'label:gap-remediation', '--json', 'number,title', '--limit', '100'],
    capture_output=True, text=True
)

try:
    issues = json.loads(result.stdout)
    for issue in issues:
        if 'index' in issue['title'].lower():
            print(f"Found: #{issue['number']} - {issue['title']}")
            sys.exit(0)
    print("No INDEX issue found in gap-remediation")
    sys.exit(1)
except json.JSONDecodeError as e:
    print(f"Error: {e}")
    print(f"Output: {result.stdout}")
    sys.exit(1)
