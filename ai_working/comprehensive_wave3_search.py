#!/usr/bin/env python3
"""Comprehensive Wave 3 gap search"""
import os
import re
import subprocess
from pathlib import Path
from collections import defaultdict

def find_cpp_files(base_path, modules):
    """Find all C++ files in target modules"""
    files = []
    src_path = Path(base_path) / "src"
    for module in modules:
        module_path = src_path / module
        if module_path.exists():
            files.extend(module_path.glob("**/*.cpp"))
    return files

def search_move_patterns(cpp_file):
    """Search for move patterns in a C++ file"""
    patterns = []
    try:
        with open(cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        for i, line in enumerate(lines):
            # Look for std::move patterns
            if 'std::move' in line and '.clear()' not in line:
                patterns.append((i + 1, line.strip()))
    except:
        pass
    
    return patterns

print("=" * 100)
print("COMPREHENSIVE WAVE 3 GAP SEARCH")
print("=" * 100)

base = '/home/runner/work/ThemisDB/ThemisDB'
modules = ['sharding', 'storage', 'training', 'gpu']

all_files = find_cpp_files(base, modules)
print(f"\nSearching {len(all_files)} C++ files across {len(modules)} modules...")

move_patterns = defaultdict(list)
total_moves = 0

for cpp_file in all_files:
    patterns = search_move_patterns(cpp_file)
    if patterns:
        rel_path = str(cpp_file.relative_to(base))
        move_patterns[rel_path] = patterns
        total_moves += len(patterns)

print(f"Found {total_moves} total move(...) patterns")
print(f"Files with patterns: {len(move_patterns)}")

# Print summary
print("\n" + "-" * 100)
print("Files with multiple move patterns (potential complexity):")
print("-" * 100)

by_count = sorted(move_patterns.items(), key=lambda x: -len(x[1]))
for file_path, patterns in by_count[:20]:
    print(f"\n{file_path}: {len(patterns)} patterns")
    for line_num, code in patterns[:5]:
        print(f"  {line_num:5d}: {code[:80]}")
    if len(patterns) > 5:
        print(f"  ... and {len(patterns) - 5} more")

print(f"\n{'='*100}")
print(f"Total files scanned: {len(all_files)}")
print(f"Files with std::move: {len(move_patterns)}")
print(f"Total move(...) calls: {total_moves}")
