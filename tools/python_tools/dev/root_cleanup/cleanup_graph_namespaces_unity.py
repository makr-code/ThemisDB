#!/usr/bin/env python3
"""Clean up namespace declarations for Unity build compatibility in themis_graph.

In Unity mode, only File 1 (graph_query_optimizer.cpp) should OPEN namespaces.
File 11 (graph_query_rewriter.cpp) should CLOSE namespaces.
Files 2-10 must have NO namespace declarations.
"""

import re
from pathlib import Path

# Unity build file order in themis_graph
FILES_IN_UNITY_ORDER = [
    ("src/graph/graph_query_optimizer.cpp", "first"),      # File 1 - OPENS
    ("src/graph/explain_plan.cpp", "middle"),              # File 2 
    ("src/graph/ontology_manager.cpp", "middle"),          # File 3
    ("src/graph/knowledge_graph_reasoner.cpp", "middle"),  # File 4
    ("src/graph/rotate_completion.cpp", "middle"),         # File 5
    ("src/graph/path_constraints.cpp", "middle"),          # File 6
    ("src/graph/distributed_graph.cpp", "middle"),         # File 7
    ("src/graph/gpu_traversal.cpp", "middle"),             # File 8
    ("src/graph/parallel_traversal.cpp", "middle"),        # File 9
    ("src/graph/scheduled_edge_refresh.cpp", "middle"),    # File 10
    ("src/graph/graph_query_rewriter.cpp", "last"),        # File 11 - CLOSES
]

def remove_namespace_blocks(content):
    """Remove all namespace blocks from content."""
    lines = content.split('\n')
    result = []
    skip_until = -1
    
    for i, line in enumerate(lines):
        if i < skip_until:
            continue
        
        # Skip lines that open namespaces at module level (not inside functions)
        if re.match(r'^\s*namespace\s+\w+\s*\{\s*$', line):
            continue
        
        # Skip closing braces that close namespaces (} // namespace ...)
        if re.match(r'^\s*}\s*//\s*namespace', line):
            continue
        
        result.append(line)
    
    return '\n'.join(result)

def ensure_namespace_open_after_includes(content):
    """Ensure namespace opens right after last include."""
    lines = content.split('\n')
    
    # Find last include
    last_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i
    
    if last_include_idx == -1:
        return content  # No includes found, skip
    
    # Check if namespace already there
    for i in range(last_include_idx + 1, min(last_include_idx + 5, len(lines))):
        if 'namespace themis' in lines[i]:
            return content  # Already has namespace
    
    # Insert namespace opens
    insert_lines = ["", "namespace themis {", "namespace graph {"]
    lines = lines[:last_include_idx + 1] + insert_lines + lines[last_include_idx + 1:]
    
    return '\n'.join(lines)

def ensure_namespace_close_at_end(content):
    """Ensure namespace closes at end of file."""
    lines = content.split('\n')
    
    # Remove trailing empty lines
    while lines and not lines[-1].strip():
        lines.pop()
    
    # Check if already closed
    if any('namespace graph' in line and '}' in line for line in lines[-3:]):
        return content
    
    # Add closing braces
    lines.append("")
    lines.append("} // namespace graph")
    lines.append("} // namespace themis")
    
    return '\n'.join(lines)

def process_file(file_path, role):
    """Process a single file based on its role in Unity build."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print(f"[PROCESS] {file_path} (role: {role})")
    
    if role == "first":
        # File 1: Remove old namespaces, add new ones after includes
        content = remove_namespace_blocks(content)
        content = ensure_namespace_open_after_includes(content)
    elif role == "middle":
        # Files 2-10: Remove ALL namespace blocks
        content = remove_namespace_blocks(content)
    elif role == "last":
        # File 11: Keep structure, ensure closing namespaces at EOF
        content = remove_namespace_blocks(content)
        content = ensure_namespace_close_at_end(content)
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"  --> Fixed")

if __name__ == '__main__':
    for file_path, role in FILES_IN_UNITY_ORDER:
        process_file(file_path, role)
    print("\n[SUCCESS] All files cleaned up for Unity build")
