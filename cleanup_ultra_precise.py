#!/usr/bin/env python3
"""
Ultra-precise: Remove only 'namespace themis {' and 'namespace graph {' lines.
Keep 'namespace {' (anonymous) completely intact.
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

def remove_themis_graph_namespaces(content):
    """Remove only 'namespace themis' and 'namespace graph' (NOT anonymous)."""
    lines = content.split('\n')
    result = []
    
    for line in lines:
        stripped = line.strip()
        # Remove only these exact lines
        if stripped == 'namespace themis {':
            continue
        if stripped == 'namespace graph {':
            continue
        if stripped == '} // namespace themis':
            continue
        if stripped == '} // namespace graph':
            continue
        
        result.append(line)
    
    return '\n'.join(result)

def add_themis_graph_namespaces_after_includes(content):
    """Add namespace opens after last include."""
    lines = content.split('\n')
    
    # Find last #include
    last_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i
    
    if last_include_idx == -1:
        return content
    
    # Check if already there
    for i in range(last_include_idx + 1, min(last_include_idx + 5, len(lines))):
        if 'namespace themis' in lines[i]:
            return content
    
    # Insert
    insert_lines = ["", "namespace themis {", "namespace graph {"]
    lines = lines[:last_include_idx + 1] + insert_lines + lines[last_include_idx + 1:]
    return '\n'.join(lines)

def add_themis_graph_namespaces_at_end(content):
    """Add namespace closes at EOF."""
    lines = content.split('\n')
    
    # Remove trailing blanks
    while lines and not lines[-1].strip():
        lines.pop()
    
    # Check if already there
    if 'namespace graph' in lines[-1] or 'namespace themis' in lines[-1]:
        return content
    
    lines.append("")
    lines.append("} // namespace graph")
    lines.append("} // namespace themis")
    
    return '\n'.join(lines)

def process_file(file_path, role):
    """Process a single file."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path}")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Step 1: Remove top-level themis/graph namespaces from ALL files
    content = remove_themis_graph_namespaces(content)
    
    # Step 2: Add opens for first file, closes for last file
    if role == "first":
        content = add_themis_graph_namespaces_after_includes(content)
        print(f"[FIRST] {file_path} - namespace OPENS after includes")
    elif role == "middle":
        print(f"[MIDDLE] {file_path} - removed all top-level namespaces")
    elif role == "last":
        content = add_themis_graph_namespaces_at_end(content)
        print(f"[LAST] {file_path} - namespace CLOSES at end")
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

if __name__ == '__main__':
    for file_path, role in FILES_IN_UNITY_ORDER:
        process_file(file_path, role)
    print("\n[SUCCESS]")
