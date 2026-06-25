#!/usr/bin/env python3
"""Add namespace themis::graph to graph implementation files missing namespaces."""

import re
from pathlib import Path

FILES_TO_FIX = [
    "src/graph/ontology_manager.cpp",
    "src/graph/knowledge_graph_reasoner.cpp",
    "src/graph/rotate_completion.cpp",
    "src/graph/parallel_traversal.cpp",
    "src/graph/scheduled_edge_refresh.cpp",
]

def add_namespaces_to_file(file_path):
    """Add namespace themis { namespace graph { after includes and close at EOF."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - file not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already has namespace
    if 'namespace themis {' in content or 'namespace themis{' in content:
        print(f"[OK] {file_path} - already has namespace")
        return
    
    lines = content.split('\n')
    
    # Find last include line
    last_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i
    
    if last_include_idx == -1:
        print(f"[SKIP] {file_path} - no includes found")
        return
    
    # Insert namespace after last include
    namespace_open = ["", "namespace themis {", "namespace graph {", ""]
    lines = lines[:last_include_idx + 1] + namespace_open + lines[last_include_idx + 1:]
    
    # Add closing namespaces before EOF
    # Remove trailing empty lines first
    while lines and not lines[-1].strip():
        lines.pop()
    
    # Add closing braces
    namespace_close = ["", "} // namespace graph", "} // namespace themis"]
    lines = lines + namespace_close
    
    # Write back
    with open(path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_TO_FIX:
        add_namespaces_to_file(file_path)
    print("\n[SUCCESS] All files processed")
