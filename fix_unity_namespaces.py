#!/usr/bin/env python3
"""
Fix namespace declarations for Unity build compatibility.
For files 2-20 in the Unity build, REMOVE all namespace opens/closes.
"""

import re
from pathlib import Path

# Map of (file_path,) tuples
# These are files 10-20 in the Unity build that need namespace cleanup
FILES_TO_CLEAN = [
    "src/graph/graph_query_optimizer.cpp",
    "src/graph/knowledge_graph_reasoner.cpp",
    "src/graph/rotate_completion.cpp",
    "src/graph/distributed_graph.cpp",
    "src/graph/parallel_traversal.cpp",
    "src/graph/scheduled_edge_refresh.cpp",
]

def remove_namespace_declarations(filepath):
    """Remove namespace opens/closes from file."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original = content
    
    # Remove patterns like:
    # namespace themis {
    # namespace graph {
    # At the end of the file, remove:
    # } // namespace graph
    # } // namespace themis
    
    # Step 1: Remove opening namespace declarations (leave includes intact)
    # Pattern: find `namespace themis {` and `namespace graph {` blocks
    
    # Find the line with `#include` directives end
    lines = content.split('\n')
    modified_lines = []
    skip_namespace_open = False
    skip_namespace_close = False
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        
        # Skip namespace opening lines
        if stripped.startswith('namespace themis {') or stripped.startswith('namespace graph {'):
            skip_namespace_open = True
            continue
        
        # Skip namespace closing lines  
        if '} // namespace' in line and ('graph' in line or 'themis' in line):
            skip_namespace_close = True
            continue
        
        # Skip empty lines that were right after namespace changes
        if skip_namespace_open and not stripped:
            skip_namespace_open = False
            continue
        
        if skip_namespace_close and not stripped:
            skip_namespace_close = False
            continue
        
        modified_lines.append(line)
    
    modified_content = '\n'.join(modified_lines)
    
    if modified_content != original:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(modified_content)
        return True
    return False

def main():
    root = Path(".")
    
    for file_path in FILES_TO_CLEAN:
        full_path = root / file_path
        if not full_path.exists():
            print(f"ERROR: File not found: {full_path}")
            return 1
        
        print(f"Cleaning {file_path}...", end=" ")
        
        if remove_namespace_declarations(full_path):
            print("✓ CLEANED")
        else:
            print("⊘ NO CHANGES")
    
    print("\nAll files cleaned successfully!")
    return 0

if __name__ == "__main__":
    exit(main())
