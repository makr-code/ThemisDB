#!/usr/bin/env python3
"""Create GitHub issues for Phase 3 Code Generation for missing modules"""

import json
import subprocess
from datetime import datetime

# Get Phase 2 modules
phase2 = json.loads(open('ai_working/phase2_batch_results.json').read())
phase2_modules = sorted(phase2.keys())

# Get existing GitHub issues
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

# Find missing modules
missing_modules = sorted(list(set(phase2_modules) - gh_modules))

print(f"MISSING MODULES ({len(missing_modules)}):")
for m in missing_modules:
    print(f"  - {m}")

print("\n" + "=" * 80)
print("CREATING GITHUB ISSUES FOR PHASE 3")
print("=" * 80 + "\n")

# Create issues for each missing module
created_issues = {}
for module in missing_modules:
    effort_hours = phase2[module]['effort_estimate']
    gap_count = phase2[module]['total_gaps']
    
    # PR body with Phase 3 information
    pr_body = f"""## Phase 3: Code Generation - {module.upper()}

### Overview
This issue tracks Phase 3 (Code Generation) for the **{module}** module using Ollama local LLM.

### Module Statistics
- **Module**: {module}
- **Total Gaps**: {gap_count}
- **Estimated Effort**: ~{effort_hours} hours
- **Phase 3 Status**: Code generation via Ollama (local model: codellama:latest)

### Workflow
1. Generate code using local Ollama
2. Create feature branch: `feature/phase3-{module}-codegen`
3. Create draft PR linking this issue
4. Review and merge when approved

### Acceptance Criteria
- [ ] Code generated via Ollama codellama:latest
- [ ] Generated code passes syntax validation
- [ ] PR created and linked to this issue
- [ ] Code review completed
- [ ] Ready for Phase 4 (Build Verification)

### Next Steps
1. Execute: `python phase3_codegen.py {module}`
2. Review results: `ai_working/phase3_{module}_results.json`
3. Create PR with generated code
4. Link PR to this issue

---
*Created: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}*
*Phase: 3 - Code Generation*
"""
    
    # Create the issue with appropriate labels
    title = f"Phase 3: Code Generation - {module.upper()}"
    
    # Use gh cli to create issue with existing labels
    result = subprocess.run(
        ['gh', 'issue', 'create',
         '--title', title,
         '--label', 'type:feature,high-priority,gap-remediation',
         '--body', pr_body],
        capture_output=True, text=True
    )
    
    if result.returncode == 0:
        # Extract issue number from output
        output = result.stdout.strip()
        if '#' in output:
            issue_num = output.split('#')[1].split()[0]
            created_issues[module] = int(issue_num)
            print(f"✓ Created #{issue_num} - {module}")
        else:
            print(f"? Created issue but couldn't parse number: {output}")
    else:
        print(f"✗ Failed to create issue for {module}")
        print(f"  Error: {result.stderr}")

print("\n" + "=" * 80)
print("CREATED ISSUES SUMMARY")
print("=" * 80 + "\n")

for module, issue_num in sorted(created_issues.items()):
    print(f"#{issue_num:5d} - {module}")

if created_issues:
    print(f"\nTotal created: {len(created_issues)}")
    print("\nNow ready to execute Phase 3 workflows!")
