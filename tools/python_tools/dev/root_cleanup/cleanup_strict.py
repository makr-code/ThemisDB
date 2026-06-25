#!/usr/bin/env python3
"""
STRICT: Remove ALL namespace declarations (including anonymous) from middle files.
Unity build requires ZERO namespace opening/closing in files 2-10.
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

def remove_all_namespace_blocks(content):
    """Remove ALL namespace opens and closes - themis, graph, anonymous."""
    lines = content.split('\n')
    result = []
    depth = 0
    skip_anonymous = False
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        
        # Skip ALL namespace declarations
        if stripped in ['namespace themis {', 'namespace graph {']:
            continue
        if stripped in ['namespace {']:
            skip_anonymous = True
            continue
        
        # Skip namespace closing comments
        if stripped in ['} // namespace themis', '} // namespace graph']:
            continue
        if stripped == '}' and skip_anonymous and i > 0 and not lines[i-1].strip().startswith('case'):
            # Try to detect end of anonymous namespace (rough heuristic)
            # But be careful not to remove braces from switch/if/for blocks!
            continue
        
        skip_anonymous = False
        result.append(line)
    
    return '\n'.join(result)

def add_themis_graph_opens_after_includes(content):
    """Add namespace opens for first file."""
    lines = content.split('\n')
    
    # Find last include
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
    
    insert_lines = ["", "namespace themis {", "namespace graph {"]
    lines = lines[:last_include_idx + 1] + insert_lines + lines[last_include_idx + 1:]
    return '\n'.join(lines)

def add_themis_graph_closes_at_end(content):
    """Add namespace closes for last file."""
    lines = content.split('\n')
    
    while lines and not lines[-1].strip():
        lines.pop()
    
    if lines[-1].strip() in ['} // namespace themis', '} // namespace graph']:
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
    
    # ALL files: remove all namespace blocks
    content = remove_all_namespace_blocks(content)
    
    # First file: add themis/graph opens
    if role == "first":
        content = add_themis_graph_opens_after_includes(content)
        print(f"[FIRST - OPENS] {file_path}")
    elif role == "middle":
        print(f"[MIDDLE - NAKED] {file_path}")
    elif role == "last":
        content = add_themis_graph_closes_at_end(content)
        print(f"[LAST - CLOSES] {file_path}")
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

if __name__ == '__main__':
    for file_path, role in FILES_IN_UNITY_ORDER:
        process_file(file_path, role)
    print("\n[SUCCESS]")
