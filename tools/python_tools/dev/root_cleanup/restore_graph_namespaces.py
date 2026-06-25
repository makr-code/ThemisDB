#!/usr/bin/env python3
"""
Restore namespace declarations for ThemisDB graph module files.
Files need: namespace themis { namespace graph { ... } }
"""

import re
from pathlib import Path

# Files that need namespace restoration (files 10-21 in module, including ontology_manager)
FILES_TO_FIX = [
    ("src/graph/graph_query_optimizer.cpp", "GraphQueryOptimizer"),
    ("src/graph/knowledge_graph_reasoner.cpp", "KnowledgeGraphReasoner"),
    ("src/graph/rotate_completion.cpp", "RotatEModel"),
    ("src/graph/distributed_graph.cpp", "DistributedGraphManager"),
    ("src/graph/parallel_traversal.cpp", "ParallelTraversal"),
    ("src/graph/scheduled_edge_refresh.cpp", "ScheduledGraphEdgeRefreshEngine"),
    ("src/graph/ontology_manager.cpp", "OntologyManager"),
]

def restore_namespaces(filepath):
    """Add proper namespace declarations if missing."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    lines = content.split('\n')
    
    # Find where includes end and code begins
    # Look for last #include or pattern where includes usually end
    last_include_idx = -1
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('#include') or stripped.startswith('#pragma'):
            last_include_idx = i
        elif stripped and not stripped.startswith('//') and not stripped.startswith('/*'):
            # First non-comment, non-empty, non-include line
            if last_include_idx >= 0:
                break
    
    # Check if namespaces already exist
    if any('namespace themis' in line for line in lines):
        return False  # Already has namespaces
    
    # Find insertion point (after all includes and blank lines)
    insert_idx = last_include_idx + 1
    while insert_idx < len(lines) and (not lines[insert_idx].strip() or lines[insert_idx].strip().startswith('//')):
        insert_idx += 1
    
    # Insert namespace opening
    namespace_open = [
        "",
        "namespace themis {",
        "namespace graph {",
    ]
    
    for ns_line in reversed(namespace_open):
        lines.insert(insert_idx, ns_line)
    
    # Add closing namespaces at end
    # Remove trailing blank lines first
    while lines and not lines[-1].strip():
        lines.pop()
    
    namespace_close = [
        "",
        "} // namespace graph",
        "} // namespace themis",
    ]
    
    lines.extend(namespace_close)
    
    modified_content = '\n'.join(lines)
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(modified_content)
    
    return True

def main():
    root = Path(".")
    
    for file_path, class_name in FILES_TO_FIX:
        full_path = root / file_path
        if not full_path.exists():
            print(f"ERROR: {file_path} not found")
            return 1
        
        print(f"Restoring namespaces in {file_path}...", end=" ")
        
        if restore_namespaces(full_path):
            print("✓ RESTORED")
        else:
            print("⊘ ALREADY HAS NAMESPACES")
    
    print("\nNamespace restoration complete!")
    return 0

if __name__ == "__main__":
    exit(main())
