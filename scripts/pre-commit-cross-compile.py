"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pre-commit-cross-compile.py                        ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:31:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     101                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Pre-Commit Hook: Cross-Compile Code Validation

Install: ln -s ../../scripts/pre-commit-cross-compile.py .git/hooks/pre-commit
Make executable: chmod +x .git/hooks/pre-commit

This hook prevents commits that violate cross-compile requirements.
"""

import sys
import subprocess
from pathlib import Path

def main():
    # Get the repository root
    repo_root = subprocess.run(
        ['git', 'rev-parse', '--show-toplevel'],
        capture_output=True,
        text=True,
        check=True
    ).stdout.strip()

    repo_root = Path(repo_root)
    reviewer_script = repo_root / 'scripts' / 'cross-compile-reviewer.py'

    if not reviewer_script.exists():
        print("⚠️  Cross-compile reviewer not found, skipping validation")
        return 0

    # Get staged files
    try:
        result = subprocess.run(
            ['git', 'diff', '--staged', '--name-only'],
            capture_output=True,
            text=True,
            check=True
        )
        staged_files = result.stdout.strip().split('\n')
        staged_files = [f for f in staged_files if f]
    except subprocess.CalledProcessError:
        return 0

    if not staged_files:
        return 0

    # Filter for relevant files
    relevant_files = []
    for f in staged_files:
        path = Path(f)
        if path.suffix in {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.cmake', '.py'}:
            relevant_files.append(f)

    if not relevant_files:
        return 0

    # Run cross-compile reviewer
    print("🔍 Validating cross-compile compatibility...")
    
    result = subprocess.run(
        ['python3', str(reviewer_script), '--files'] + relevant_files + ['--output', 'text'],
        cwd=repo_root,
        capture_output=True,
        text=True
    )

    print(result.stdout)
    
    if result.returncode != 0:
        print("\n❌ Commit blocked: Cross-compile violations found")
        print("💡 To fix issues, see CROSS_COMPILE_REQUIREMENTS.md")
        print("\n⏭️  To bypass this check (not recommended): git commit --no-verify")
        return 1

    print("✅ Cross-compile validation passed!")
    return 0

if __name__ == '__main__':
    sys.exit(main())
