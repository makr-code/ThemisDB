#!/usr/bin/env python3
"""Phase 3 Quick Reference: Module-by-Module Checklist"""

import json
import sys

# Phase 2 module list
phase2 = json.loads(open('ai_working/phase2_batch_results.json').read())
modules = sorted(phase2.keys())

# Map modules to GitHub issues
issue_mapping = {
    'acceleration': 5257, 'ai': 5267, 'analytics': 5314, 'api': 5258, 'aql': 5259,
    'auth': 5260, 'base': 5261, 'cache': 5262, 'cdc': 5263, 'chimera': 5264,
    'config': 5265, 'content': 5315, 'core': 5266, 'chaos': 5268, 'demo_encryption.cpp': 5269,
    'distributed_knowledge': 5270, 'document': 5271, 'ethics_ai': 5272, 'exporters': 5273,
    'failover': 5274, 'geo': 5275, 'governance': 5276, 'gpu': 5277, 'graph': 5278,
    'importers': 5279, 'index': 5316, 'ingestion': 5280, 'llama_cpp': 5281, 'llm': 5317,
    'main.cpp': 5282, 'main_server.cpp': 5283, 'maintenance': 5284, 'metadata': 5285,
    'network': 5286, 'observability': 5287, 'onnx_clip': 5288, 'performance': 5289,
    'plugins': 5290, 'process': 5291, 'projects': 5292, 'prompt_engineering': 5293,
    'query': 5318, 'rag': 5319, 'replication': 5294, 'rpc_grpc': 5295, 'scheduler': 5296,
    'security': 5320, 'server': 5321, 'sharding': 5322, 'storage': 5323, 'test_integration': 5297,
    'timeseries': 5303, 'toolbox': 5304, 'training': 5305, 'transaction': 5306,
    'updates': 5307, 'user_storage_encrypted': 5308, 'utils': 5309, 'vector': 5299,
    'voice': 5310, 'whisper': 5311,
}

if len(sys.argv) < 2:
    print("PHASE 3 - QUICK REFERENCE")
    print("=" * 80)
    print()
    print("Usage:")
    print("  python quick_reference.py <module>")
    print()
    print("Examples:")
    print("  python quick_reference.py index")
    print("  python quick_reference.py analytics")
    print()
    print("All modules:")
    print(f"  {', '.join(modules[:10])} ... ({len(modules)} total)")
    print()
    sys.exit(0)

module = sys.argv[1]
if module not in modules:
    print(f"ERROR: Module '{module}' not found")
    sys.exit(1)

issue_num = issue_mapping[module]
branch = f"feature/phase3-{module}-codegen"

print()
print(f"{module.upper()} - QUICK CHECKLIST")
print("=" * 60)
print()
print(f"Issue:         #{issue_num}")
print(f"Branch:        {branch}")
print(f"PR Body File:  ai_working/pr_body_{module}.md")
print()
print("STEPS:")
print("-" * 60)

steps = [
    f"gh issue view #{issue_num}",
    f"git checkout -b {branch}",
    f"git branch -u origin/develop",
    "git add . && git commit -m 'Phase 3: ...'",
    f"gh pr create --draft --title 'Phase 3: ... - {module.upper()}' --base develop",
    "[SAVE PR_NUM from output]",
    f"gh pr comment $PR_NUM --body 'Closes #{issue_num}'",
    f"gh issue comment #{issue_num} --body 'PR: ...'",
    f"gh issue edit #{issue_num} --add-label 'phase-3'",
]

for i, step in enumerate(steps, 1):
    print(f"{i:2d}. {step}")

print()
print(f"VERIFY:")
print(f"  gh pr view $PR_NUM")
print(f"  gh issue view #{issue_num}")
print()
