#!/usr/bin/env python3
"""Phase 3 Workflow: Display exact gh CLI commands per module"""

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
    print("Phase 3 Workflow: Display gh CLI commands")
    print("=" * 80)
    print()
    print("Usage: python workflow_gh_cli.py <module>")
    print()
    print("Example: python workflow_gh_cli.py index")
    print()
    sys.exit(0)

module = sys.argv[1]
if module not in modules:
    print(f"ERROR: Module '{module}' not found")
    sys.exit(1)

issue_num = issue_mapping[module]
branch = f"feature/phase3-{module}-codegen"
pr_body_file = f"ai_working/pr_body_{module}.md"

print("=" * 80)
print(f"COMPLETE WORKFLOW: {module.upper()}")
print("=" * 80)
print()

print("PREREQUISITES:")
print("-" * 80)
print("$ gh auth login")
print("$ gh auth status  # Verify authenticated")
print()

print("STEP 1: CHECK ISSUE STATUS")
print("-" * 80)
print(f"$ gh issue view {issue_num}")
print(f"  -> Shows current issue status, labels, assignees")
print()

print("STEP 2: REVIEW PHASE 3 RESULTS")
print("-" * 80)
print(f"$ cat ai_working/phase3_{module}_results.json | jq '.results[] | {{id:.task_id, valid:.syntax_ok}}'")
print(f"  -> Review: generated tasks and validation status")
print()

print("STEP 3: CREATE FEATURE BRANCH")
print("-" * 80)
print(f"$ git checkout -b {branch}")
print(f"$ git branch -u origin/develop")
print()

print("STEP 4: COMMIT GENERATED CODE")
print("-" * 80)
print(f"$ git add .")
print(f"$ git commit -m 'Phase 3: Ollama code generation for {module}'")
print(f"$ git commit -m 'Implements #{issue_num}: Code generation with codellama'")
print()

print("STEP 5: CREATE DRAFT PR")
print("-" * 80)
print(f"$ gh pr create \\")
print(f"    --draft \\")
print(f"    --title 'Phase 3: Code Generation - {module.upper()}' \\")
print(f"    --base develop \\")
print(f"    --body-file {pr_body_file}")
print()
print("Output will show: Created draft pull request #XXXX")
print()

print("STEP 6: LINK ISSUE IN PR BODY")
print("-" * 80)
print("[Store PR number from step 5 as $PR_NUM]")
print(f"$ gh pr comment $PR_NUM --body 'Closes #{issue_num}'")
print()

print("STEP 7: UPDATE ISSUE WITH PR LINK")
print("-" * 80)
print(f"$ gh issue comment #{issue_num} --body 'Phase 3 code generation complete. PR: $PR_URL'")
print()

print("STEP 8: ADD LABELS (optional)")
print("-" * 80)
print(f"$ gh issue edit #{issue_num} --add-label 'type:feature,high-priority'")
print()

print("STEP 9: READY FOR REVIEW")
print("-" * 80)
print(f"$ gh pr view $PR_NUM --repo makr-code/ThemisDB")
print()

print("=" * 80)
print("NEXT: Wait for Copilot review, merge when approved")
print("=" * 80)
print()

print("USEFUL COMMANDS:")
print("-" * 80)
print("# Check PR status")
print("$ gh pr checks $PR_NUM")
print()
print("# View issue with all linked PRs")
print(f"$ gh issue view #{issue_num}")
print()
print("# List all Phase 3 PRs")
print("$ gh pr list --search 'Phase 3 code generation'")
print()
