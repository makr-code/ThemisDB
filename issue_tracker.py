#!/usr/bin/env python3
"""Updated issue tracker with correct GitHub issue numbers"""

import json

# Phase 2 module list
phase2 = json.loads(open('ai_working/phase2_batch_results.json').read())
modules = sorted(phase2.keys())

# Map modules to GitHub issues (created + existing)
issue_mapping = {
    'acceleration': 5257,
    'ai': 5267,
    'analytics': 5314,
    'api': 5258,
    'aql': 5259,
    'auth': 5260,
    'base': 5261,
    'cache': 5262,
    'cdc': 5263,
    'chimera': 5264,
    'config': 5265,
    'content': 5315,
    'core': 5266,
    'chaos': 5268,
    'demo_encryption.cpp': 5269,
    'distributed_knowledge': 5270,
    'document': 5271,
    'ethics_ai': 5272,
    'exporters': 5273,
    'failover': 5274,
    'geo': 5275,
    'governance': 5276,
    'gpu': 5277,
    'graph': 5278,
    'importers': 5279,
    'index': 5316,
    'ingestion': 5280,
    'llama_cpp': 5281,
    'llm': 5317,
    'main.cpp': 5282,
    'main_server.cpp': 5283,
    'maintenance': 5284,
    'metadata': 5285,
    'network': 5286,
    'observability': 5287,
    'onnx_clip': 5288,
    'performance': 5289,
    'plugins': 5290,
    'process': 5291,
    'projects': 5292,
    'prompt_engineering': 5293,
    'query': 5318,
    'rag': 5319,
    'replication': 5294,
    'rpc_grpc': 5295,
    'scheduler': 5296,
    'security': 5320,
    'server': 5321,
    'sharding': 5322,
    'storage': 5323,
    'test_integration': 5297,
    'timeseries': 5303,
    'toolbox': 5304,
    'training': 5305,
    'transaction': 5306,
    'updates': 5307,
    'user_storage_encrypted': 5308,
    'utils': 5309,
    'vector': 5299,
    'voice': 5310,
    'whisper': 5311,
}

# Command line parsing
import sys

if len(sys.argv) < 2:
    print("ISSUE TRACKER - All 65 Modules")
    print("=" * 80)
    print()
    print("Usage: python issue_tracker.py <module>")
    print("       python issue_tracker.py --list")
    print("       python issue_tracker.py --check-gh")
    print()
    sys.exit(0)

if sys.argv[1] == '--list':
    print("ALL MODULES AND GITHUB ISSUES")
    print("=" * 80)
    print()
    for i, module in enumerate(modules, 1):
        issue_num = issue_mapping.get(module, '????')
        phase3_file = f'ai_working/phase3_{module}_results.json'
        try:
            phase3_data = json.loads(open(phase3_file).read())
            phase3_status = f"✓ {phase3_data.get('syntax_ok', '?')}/{phase3_data.get('tasks_generated', '?')} valid"
        except:
            phase3_status = "- not generated"
        
        print(f"{i:2d}. #{issue_num:5d} - {module:30s} - {phase3_status}")
    
    print()
    print(f"Total: {len(modules)} modules")
    sys.exit(0)

# Show issue for specific module
module = sys.argv[1]
if module not in modules:
    print(f"ERROR: Module '{module}' not found")
    sys.exit(1)

issue_num = issue_mapping.get(module, '????')

print()
print(f"MODULE: {module.upper()}")
print("=" * 80)
print(f"GitHub Issue:     #{issue_num}")
print(f"Issue Title:      Phase 3: Code Generation - {module.upper()}")
print(f"Repository:       makr-code/ThemisDB")
print()

# Show Phase 3 status if available
phase3_file = f'ai_working/phase3_{module}_results.json'
try:
    phase3_data = json.loads(open(phase3_file).read())
    print(f"PHASE 3 STATUS:   COMPLETED")
    print(f"  Model:          {phase3_data.get('model', 'unknown')}")
    print(f"  Tasks:          {phase3_data.get('tasks_generated', '?')}/{phase3_data.get('tasks_total', '?')}")
    print(f"  Valid:          {phase3_data.get('syntax_ok', '?')}/{phase3_data.get('tasks_generated', '?')}")
    print(f"  Exec Time:      {phase3_data.get('execution_time', '?'):.1f}s")
    print(f"  Results File:   {phase3_file}")
except FileNotFoundError:
    print(f"PHASE 3 STATUS:   NOT GENERATED YET")

print()
print("NEXT STEPS:")
print("-" * 80)
print(f"1. Generate code: python phase3_codegen.py {module}")
print(f"2. Create branch: git checkout -b feature/phase3-{module}-codegen")
print(f"3. Create PR: gh pr create --draft --title 'Phase 3: ... - {module.upper()}'")
print(f"4. Link issue: gh pr comment <PR> --body 'Closes #{issue_num}'")
print()
