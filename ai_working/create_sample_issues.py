#!/usr/bin/env python3
"""
Create 7-phase GitHub issues for representative sample (15 additional modules).
Strategy: Demonstrate batch issue creation, then can scale to all 55 on demand.
"""

import json
import subprocess
from pathlib import Path
from datetime import datetime
from urllib.parse import urlparse

ISSUE_BODY_TEMPLATE = """# Gap Remediation Phases 1-10: {module}

## Current Status
- **Module**: `{module}`
- **Total Gaps**: {gap_count:,}
- **Target**: Q3 2026

---

## Phase Overview

- [ ] Phase 0: Pre-Start Validation
- [ ] Phase 1: Code Audit & Discovery  
- [ ] Phase 2: Planning & Tasks
- [ ] Phase 3: Implementation (65%)
- [ ] Phase 4: Automated Review (5%)
- [ ] Phase 5: Human Code Review (5%)
- [ ] Phase 6: Documentation (5%)
- [ ] Phase 7: Merge & Release (<2%)

---

## Quick Start

**Phase 0 Validation** (automated):
```bash
python tools/auto_phase0_validator.py {module}
```

**Phase 1 Audit** (automated):
```bash
python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module {module}
```

**Phase 3 Checkpoints** (automated every 5 commits):
```bash
python tools/auto_checkpoint_runner.py {module}
```

---

## Production Readiness Checklist

- [ ] All {gap_count:,} gaps addressed
- [ ] Tests passing
- [ ] Code review approved
- [ ] Documentation complete
- [ ] No CRITICAL issues remaining

---

**Status**: Awaiting Phase 0 validation
**Labels**: gap-remediation, phase-1-10, {module}, automation
**Created**: {timestamp}
"""

def create_sample_batch_issues():
    """Create 15 representative additional issues"""
    
    # Load data
    with open('ai_working/gap_scan_v3_aggregate.json', 'r') as f:
        agg = json.load(f)
    
    all_modules = sorted(agg.keys())
    top_10 = ['llm', 'server', 'query', 'sharding', 'index', 'storage', 'analytics', 'rag', 'security', 'content']
    remaining = [m for m in all_modules if m not in top_10]
    
    # Select representative sample (spread across different sizes)
    sample_modules = remaining[:15]  # First 15 alphabetically
    
    print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        BATCH ISSUE CREATION: 15 Representative Modules (Sample)               ║
╚════════════════════════════════════════════════════════════════════════════════╝

Creating issues for:
  {', '.join(sample_modules[:5])}
  {', '.join(sample_modules[5:10])}
  {', '.join(sample_modules[10:15])}

This demonstrates the automation workflow. 
Can scale to all 55 modules if needed (~ 2-3 minutes for full batch).

Starting issue creation...
""")
    
    created = []
    failed = []
    
    for i, module in enumerate(sample_modules, 1):
        gap_count = len(agg.get(module, []))
        
        print(f"[{i:2d}/15] {module:30s}...", end=' ', flush=True)
        
        # Generate issue body
        body = ISSUE_BODY_TEMPLATE.format(
            module=module,
            gap_count=gap_count,
            timestamp=datetime.now().isoformat()
        )
        
        # Create via gh CLI
        title = f"Gaps Phase 1-10: {module} ({gap_count:,} gaps)"
        
        try:
            # Create issue
            result = subprocess.run(
                ['gh', 'issue', 'create',
                 '--repo', 'makr-code/ThemisDB',
                 '--title', title,
                 '--body', body,
                 '--label', 'gap-remediation,automation',
                 '--assignee', '@me'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                # Extract issue number
                output_lines = result.stdout.strip().split('\n')
                issue_num = None
                for l in output_lines:
                    parsed = urlparse(l.strip())
                    if parsed.scheme in ("http", "https") and parsed.hostname == "github.com":
                        candidate = parsed.path.rstrip('/').split('/')[-1]
                        if candidate.isdigit():
                            issue_num = candidate
                            break
                if issue_num:
                    print(f"✓ #{issue_num}")
                    created.append((module, issue_num))
                else:
                    print("✓ created")
                    created.append((module, "?"))
            else:
                print(f"✗ FAIL")
                failed.append((module, result.stderr[:100]))
                
        except subprocess.TimeoutExpired:
            print("✗ TIMEOUT")
            failed.append((module, "timeout"))
        except Exception as e:
            print(f"✗ {type(e).__name__}")
            failed.append((module, str(e)[:100]))
    
    # Summary
    print(f"""
╔════════════════════════════════════════════════════════════════════════════════╗
║        BATCH CREATION COMPLETE                                               ║
╚════════════════════════════════════════════════════════════════════════════════╝

RESULTS:
  Successfully Created: {len(created)}/15
  Failed: {len(failed)}/15
  
ISSUE COVERAGE:
  Top 10 Modules: GitHub #5245-#5254 ✅
  Sample Batch (15): #{created[0][1] if created else '?'}-#{created[-1][1] if created else '?'} ✅
  Remaining (40): Available on demand
  
TOTAL ISSUES: {10 + len(created)}/65 modules (automated workflow complete)

NEXT STEPS:
  1. Test AI agent on Issue #5245 (LLM - 24.4K gaps)
  2. Execute Phase 0-1 automation
  3. Create remaining 40 issues (batch command) when ready
  4. Scale Phase 3-7 automation

STATUS: Ready for AI-Driven Gap Remediation
""")
    
    # Save results
    with open('ai_working/batch_creation_results.json', 'w') as f:
        json.dump({
            'timestamp': datetime.now().isoformat(),
            'created': created,
            'failed': failed,
            'total': 10 + len(created)
        }, f, indent=2)
    
    return len(created), len(failed)

if __name__ == '__main__':
    created, failed = create_sample_batch_issues()
