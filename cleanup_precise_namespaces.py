#!/usr/bin/env python3
"""
Precise cleanup: Remove only top-level themis and graph namespace declarations.
Keep anonymous namespaces and local scopes untouched.
"""

import re
from pathlib import Path

FILES_IN_UNITY_ORDER = [
    ("src/graph/graph_query_optimizer.cpp", "first"),
    ("src/graph/explain_plan.cpp", "middle"),
    ("src/graph/ontology_manager.cpp", "middle"),
    ("src/graph/knowledge_graph_reasoner.cpp", "middle"),
    ("src/graph/rotate_completion.cpp", "middle"),
    ("src/graph/path_constraints.cpp", "middle"),
    ("src/graph/distributed_graph.cpp", "middle"),
    ("src/graph/gpu_traversal.cpp", "middle"),
    ("src/graph/parallel_traversal.cpp", "middle"),
    ("src/graph/scheduled_edge_refresh.cpp", "middle"),
    ("src/graph/graph_query_rewriter.cpp", "last"),
]

def remove_top_level_namespaces(content):
    """Remove only top-level 'namespace themis' and 'namespace graph' declarations."""
    lines = content.split('\n')
    result = []
    depth = 0
    
    for line in lines:
        stripped = line.strip()
        
        # Skip top-level namespace themis / graph opens
        if stripped in ['namespace themis {', 'namespace graph {']:
            continue
        
        # Skip namespace closing comments (both themis and graph)
        if stripped in ['} // namespace themis', '} // namespace graph']:
            continue
        
        # Keep everything else
        result.append(line)
    
    return '\n'.join(result)

def ensure_first_file_has_namespace_opens(content):
    """Add namespace opens after includes for first file."""
    lines = content.split('\n')
    
    # Find last include
    last_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i
    
    if last_include_idx == -1:
        return content
    
    # Check if namespaces already there
    for i in range(last_include_idx + 1, min(last_include_idx + 5, len(lines))):
        if 'namespace themis' in lines[i]:
            return content
    
    # Insert
    insert_lines = ["", "namespace themis {", "namespace graph {"]
    lines = lines[:last_include_idx + 1] + insert_lines + lines[last_include_idx + 1:]
    
    return '\n'.join(lines)

def ensure_last_file_has_namespace_closes(content):
    """Ensure last file closes themis and graph namespaces at end."""
    lines = content.split('\n')
    
    # Remove trailing empty lines
    while lines and not lines[-1].strip():
        lines.pop()
    
    # Check if already closed
    if lines[-1].strip() in ['} // namespace themis', '} // namespace graph']:
        return content
    
    # Add closes
    lines.append("")
    lines.append("} // namespace graph")
    lines.append("} // namespace themis")
    
    return '\n'.join(lines)

def process_file(file_path, role):
    """Process file based on role."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path}")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # All files: remove top-level themis/graph namespaces
    content = remove_top_level_namespaces(content)
    
    # First file: add opens
    if role == "first":
        content = ensure_first_file_has_namespace_opens(content)
        print(f"[FIRST] {file_path}")
    # Middle files: nothing special
    elif role == "middle":
        print(f"[MIDDLE] {file_path}")
    # Last file: add closes
    elif role == "last":
        content = ensure_last_file_has_namespace_closes(content)
        print(f"[LAST] {file_path}")
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

if __name__ == '__main__':
    for file_path, role in FILES_IN_UNITY_ORDER:
        process_file(file_path, role)
    print("\n[SUCCESS] Files processed")
