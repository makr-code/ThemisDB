#!/usr/bin/env python3
"""
Extract Wave 3 gaps from source code.
Wave 3 focuses on complex control flow move patterns.
"""
import os
import re
from pathlib import Path

# Target modules for Wave 3 (per Gap Report)
TARGET_MODULES = ['sharding', 'storage', 'training', 'gpu']

# Known gaps from Gap Report + manual review
# Format: (file_path, line_number, pattern_type, description)
KNOWN_GAPS = [
    ("src/sharding/cross_shard_transaction.cpp", 3472, "conditional_move", "Transaction retry logic with conditional move"),
    ("src/storage/wom_tree.cpp", 408, "loop_move", "Tree traversal with move semantics"),
    ("src/training/auto_labeler.cpp", 291, "conditional_branch", "Modality fallback with move in if-branch"),
    ("src/gpu/launcher.cpp", 131, "lambda_capture", "Async executor with lambda capture-by-move"),
]

def load_source(file_path):
    """Load source file with error handling"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            return f.readlines()
    except:
        return []

def get_context(lines, line_num, before=5, after=5):
    """Get code context around line"""
    start = max(0, line_num - 1 - before)
    end = min(len(lines), line_num + after)
    return lines[start:end], start + 1

def analyze_gap(file_path, line_num):
    """Analyze a single gap"""
    lines = load_source(file_path)
    if not lines:
        return None
    
    context, context_start = get_context(lines, line_num)
    
    return {
        'file': file_path,
        'line': line_num,
        'context': ''.join(context),
        'context_start': context_start,
    }

print("=" * 100)
print("WAVE 3 GAP ANALYSIS - Complex Control Flow Move Semantics")
print("=" * 100)

gaps_found = []
for i, (file_path, line_num, pattern_type, desc) in enumerate(KNOWN_GAPS, 1):
    full_path = Path('/home/runner/work/ThemisDB/ThemisDB') / file_path
    
    if full_path.exists():
        analysis = analyze_gap(str(full_path), line_num)
        if analysis:
            gaps_found.append((i, file_path, line_num, pattern_type, desc, analysis))
            print(f"\n[GAP {i}] {file_path}:{line_num}")
            print(f"  Type: {pattern_type}")
            print(f"  Desc: {desc}")
            print(f"  Status: FOUND")
    else:
        print(f"\n[GAP {i}] {file_path}:{line_num}")
        print(f"  Type: {pattern_type}")
        print(f"  Status: FILE NOT FOUND")

print(f"\n\n{'='*100}")
print(f"SUMMARY: Found {len(gaps_found)}/{len(KNOWN_GAPS)} identified gaps")
print(f"Status: Ready for detailed analysis")

# Save for next phase
import json
output = {
    'total': len(KNOWN_GAPS),
    'found': len(gaps_found),
    'gaps': [
        {
            'num': g[0],
            'file': g[1],
            'line': g[2],
            'type': g[3],
            'desc': g[4]
        } for g in gaps_found
    ]
}

with open('/home/runner/work/ThemisDB/ThemisDB/ai_working/wave3_gaps_identified.json', 'w') as f:
    json.dump(output, f, indent=2)

print(f"\nGap list saved to: ai_working/wave3_gaps_identified.json")
