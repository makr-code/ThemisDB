#!/usr/bin/env python3
import json

with open('ai_working/gap_scan_test_phase5.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

# Sample first 5 findings
if isinstance(data, list):
    print("List format (5 items):")
    for i, gap in enumerate(data[:5]):
        print(f"  {i+1}. {gap.get('file')}:{gap.get('line')}")
elif isinstance(data, dict):
    if 'findings' in data:
        print("Dict with 'findings' key (5 items):")
        for i, gap in enumerate(data['findings'][:5]):
            print(f"  {i+1}. {gap.get('file')}:{gap.get('line')}")
    else:
        print(f"Dict keys: {list(data.keys())}")

# Check scope classification
print("\n--- Scope Classification Test ---")
test_paths = [
    "src/graph/explain_plan.cpp",
    "explain_plan.cpp",
    "llama.cpp/src/ggml.cpp",
    "whisper.cpp/whisper.cpp",
]

def normalize_path(path):
    return (path or '').replace('\\', '/').lower()

def classify_scope(path):
    normalized = normalize_path(path)
    if normalized.startswith('tests/'):
        return 'themis_tests'
    if normalized.startswith('benchmarks/'):
        return 'themis_benchmarks'
    if normalized.startswith(('src/', 'include/', 'tools/', 'scripts/', 'cmake/', 'docs/', 'examples/')):
        return 'themis_core'
    return 'third_party'

for path in test_paths:
    scope = classify_scope(path)
    print(f"  {path:40} → {scope}")

# Check metadata scope_breakdown
if isinstance(data, dict) and 'metadata' in data:
    if 'scope_breakdown' in data['metadata']:
        scope = data['metadata']['scope_breakdown']
        print("\nActual scope_breakdown in JSON:")
        for key in ['themis_core', 'themis_tests', 'themis_benchmarks', 'third_party']:
            count = scope.get('counts', {}).get(key, 0)
            print(f"  {key}: {count}")
