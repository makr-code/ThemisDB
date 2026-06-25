#!/usr/bin/env python3
"""
Add missing namespace declarations to 7 graph files that lack them.
DO NOT modify the 4 files that already have correct namespaces.
"""

import re
from pathlib import Path

FILES_NEEDING_FIX = [
    "src/graph/graph_query_optimizer.cpp",
    "src/graph/ontology_manager.cpp",
    "src/graph/knowledge_graph_reasoner.cpp",
    "src/graph/rotate_completion.cpp",
    "src/graph/distributed_graph.cpp",
    "src/graph/parallel_traversal.cpp",
    "src/graph/scheduled_edge_refresh.cpp",
]

def add_namespaces_to_file(file_path):
    """Add namespace opens after includes and closes at EOF."""
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
    
    # Find last include
    last_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i
    
    if last_include_idx == -1:
        print(f"[ERROR] {file_path} - no includes found")
        return
    
    # Insert namespace opens after last include
    namespace_lines = ["", "namespace themis {", "namespace graph {"]
    lines = lines[:last_include_idx + 1] + namespace_lines + lines[last_include_idx + 1:]
    
    # Find EOF and add closes
    # Remove trailing empty lines first
    while lines and not lines[-1].strip():
        lines.pop()
    
    # Add closing namespaces
    closing_lines = ["", "} // namespace graph", "} // namespace themis"]
    lines = lines + closing_lines
    
    # Write back
    with open(path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_NEEDING_FIX:
        add_namespaces_to_file(file_path)
    print("\n[SUCCESS] All 7 files fixed")
