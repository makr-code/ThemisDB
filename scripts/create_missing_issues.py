"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            create_missing_issues.py                           ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     106                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Create missing GitHub Issues from templates
"""

import subprocess
import json
import sys
from pathlib import Path

def run_command(cmd):
    """Execute command and return output"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return result.returncode, result.stdout.strip(), result.stderr.strip()
    except Exception as e:
        print(f"[ERROR] Command execution failed: {e}", file=sys.stderr)
        return 1, "", str(e)

def create_issue(title, labels, body):
    """Create a GitHub issue using gh CLI"""
    print(f"Creating: {title}")
    
    # Escape special characters in labels and body
    labels_str = ",".join(labels)
    
    # Use gh issue create command
    cmd = f'gh issue create --title "{title}" --label "{labels_str}" --body "{body}"'
    
    returncode, stdout, stderr = run_command(cmd)
    
    if returncode == 0:
        # Extract issue number from output
        if '#' in stdout:
            issue_num = stdout.split('#')[1].split()[0]
            print(f"✅ Created: #{issue_num}")
            return True
        else:
            print(f"✅ Created: {stdout}")
            return True
    else:
        print(f"❌ Failed: {stderr}")
        return False

def main():
    print("=== Erstelle 3 fehlende GitHub Issues ===\n")
    
    issues_to_create = [
        {
            "title": "[FEATURE] Phase 1: Named Snapshots (Semantic Tagging)",
            "labels": ["type:enhancement", "area:storage", "priority:critical", "milestone:current"],
            "body": "Implement Named Snapshots feature for ThemisDB's MVCC system.\n\nObjectives:\n- Semantic tagging of database states\n- Persistent tag storage in RocksDB\n- REST API for tag management\n- Foundation for Point-in-Time Recovery\n\nSee .github/ISSUE_TEMPLATE/git_features_phase1_named_snapshots.md for full implementation details.\n\nEstimated Duration: 3-4 weeks"
        },
        {
            "title": "[FEATURE] Phase 2: Diff API (Structured Diff)",
            "labels": ["type:enhancement", "area:storage", "area:api", "priority:high", "milestone:next"],
            "body": "Implement a structured Diff API for ThemisDB's MVCC system.\n\nObjectives:\n- Structured diffs between any two database states\n- Filtering capabilities by table and key prefix\n- Pagination support for large diff results\n- Performance optimization (<100ms for 10K changes)\n\nSee .github/ISSUE_TEMPLATE/git_features_phase2_diff_api.md for full implementation details.\n\nEstimated Duration: 3-4 weeks\nDependencies: Phase 1 must be completed first"
        },
        {
            "title": "[FEATURE] Phase 3: Point-in-Time Recovery (PITR)",
            "labels": ["type:enhancement", "area:storage", "priority:critical", "milestone:future"],
            "body": "Implement Point-in-Time Recovery for ThemisDB's MVCC system.\n\nObjectives:\n- Safe restoration to any point in time\n- Automatic backup before restore operations\n- Dry-run mode for preview before execution\n- Selective restore (specific tables only)\n- Robust error handling with automatic rollback\n\nSee .github/ISSUE_TEMPLATE/git_features_phase3_pitr.md for full implementation details.\n\nEstimated Duration: 3-4 weeks\nDependencies: Phase 1 & Phase 2 must be completed first"
        }
    ]
    
    created_count = 0
    failed_count = 0
    
    for i, issue in enumerate(issues_to_create, 1):
        print(f"\n[{i}/3] {issue['title']}")
        
        if create_issue(issue["title"], issue["labels"], issue["body"]):
            created_count += 1
        else:
            failed_count += 1
    
    print("\n" + "="*50)
    print(f"✅ Created: {created_count} | ❌ Failed: {failed_count}")
    print("="*50)
    
    return 0 if failed_count == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
