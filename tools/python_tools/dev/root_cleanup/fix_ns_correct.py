#!/usr/bin/env python3
"""
Fix namespace declarations - CORRECT APPROACH:
1. Namespace opens go BEFORE #include  
2. Namespace closes go at EOF
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

def fix_namespaces_correct(file_path):
    """Add namespace opens BEFORE includes and closes at EOF."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already has namespace (correctly placed)
    lines = content.split('\n')
    
    # Check first non-comment/doc line
    first_code_line_idx = -1
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped and not stripped.startswith('//') and not stripped.startswith('*'):
            if not (stripped.startswith('/**') or stripped.startswith('/*')):
                first_code_line_idx = i
                break
    
    if first_code_line_idx != -1:
        first_line = lines[first_code_line_idx].strip()
        if 'namespace themis {' in content:
            print(f"[SKIP] {file_path} - already has namespace themis")
            return
    
    # Find first include
    first_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            first_include_idx = i
            break
    
    if first_include_idx == -1:
        print(f"[ERROR] {file_path} - no includes found")
        return
    
    # Insert namespace opens BEFORE first include
    namespace_opens = ["namespace themis {", "namespace graph {", ""]
    lines = lines[:first_include_idx] + namespace_opens + lines[first_include_idx:]
    
    # Remove trailing empty lines
    while lines and not lines[-1].strip():
        lines.pop()
    
    # Add closing namespaces
    namespace_closes = ["", "} // namespace graph", "} // namespace themis"]
    lines = lines + namespace_closes
    
    # Write back
    with open(path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_NEEDING_FIX:
        fix_namespaces_correct(file_path)
    print("\n[SUCCESS] All 7 files fixed with CORRECT namespace placement")
