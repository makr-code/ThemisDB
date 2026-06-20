#!/usr/bin/env python3
"""
FINAL CORRECT FIX:
Follow the pattern from explain_plan.cpp:
1. #include "graph/*.h" (CUSTOM header FIRST)
2. One blank line
3. #include <...> (Standard headers)
4. namespace themis { namespace graph { namespace { (THEN namespace)
5. Code
6. Close namespaces at EOF
"""

import re
from pathlib import Path
import sys

if sys.platform == 'win32':
    sys.stdout.reconfigure(encoding='utf-8')

FILES_NEEDING_FIX = [
    "src/graph/graph_query_optimizer.cpp",
    "src/graph/ontology_manager.cpp",
    "src/graph/knowledge_graph_reasoner.cpp",
    "src/graph/rotate_completion.cpp",
    "src/graph/distributed_graph.cpp",
    "src/graph/parallel_traversal.cpp",
    "src/graph/scheduled_edge_refresh.cpp",
]

def fix_namespaces_final(file_path):
    """Apply FINAL correct namespace pattern from explain_plan.cpp."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already has namespace
    if 'namespace themis {' in content:
        print(f"[SKIP] {file_path} - already has namespace")
        return
    
    lines = content.split('\n')
    
    # Find custom includes (graph/*.h) and standard includes (< >)
    custom_includes = []
    standard_includes = []
    other_lines = []
    
    i = 0
    while i < len(lines):
        line = lines[i]
        if '#include "graph/' in line:
            custom_includes.append(line)
        elif '#include <' in line:
            standard_includes.append(line)
        else:
            # Everything else (comments, blank lines, code)
            other_lines = lines[i:]
            break
        i += 1
    
    # Reconstruct with proper pattern
    new_content = '\n'.join(
        custom_includes +
        [''] +  # blank line
        standard_includes +
        ['', 'namespace themis {', 'namespace graph {', ''] +
        other_lines
    )
    
    # Find where to add namespace closes (before final newline/eof)
    new_lines = new_content.split('\n')
    while new_lines and not new_lines[-1].strip():
        new_lines.pop()
    
    new_lines += ['', '} // namespace graph', '} // namespace themis']
    
    # Write back
    with open(path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(new_lines))
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_NEEDING_FIX:
        fix_namespaces_final(file_path)
    print("\n[SUCCESS] All 7 files fixed with FINAL correct namespace placement")
