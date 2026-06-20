#!/usr/bin/env python3
"""
SIMPLEST CORRECT FIX:
Wrap entire file content (after header comments) in:
  namespace themis {
  namespace graph {
  
  [all code]
  
  } // namespace graph
  } // namespace themis
"""

import re
from pathlib import Path
import sys

if sys.platform == 'win32':
    sys.stdout.reconfigure(encoding='utf-8')

FILES_NEEDING_FIX = {
    "src/graph/graph_query_optimizer.cpp": "GraphQueryOptimizer",
    "src/graph/ontology_manager.cpp": "OntologyManager",
    "src/graph/knowledge_graph_reasoner.cpp": "KnowledgeGraphReasoner",
    "src/graph/rotate_completion.cpp": "RotateCompletion",
    "src/graph/distributed_graph.cpp": "DistributedGraph",
    "src/graph/parallel_traversal.cpp": "ParallelTraversal",
    "src/graph/scheduled_edge_refresh.cpp": "ScheduledEdgeRefresh",
}

def wrap_with_namespace(file_path):
    """Wrap entire file (after header) in themis::graph namespace."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already has namespace closing
    if '} // namespace graph' in content or '} // namespace themis' in content:
        print(f"[SKIP] {file_path} - already has namespace")
        return
    
    lines = content.split('\n')
    
    # Find where to insert namespace (after initial Doxygen/comment blocks)
    # Look for first #include or first code line
    insert_pos = 0
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('#include') or (stripped and not stripped.startswith('*') and not stripped.startswith('/') and not stripped.startswith('(')):
            insert_pos = i
            break
    
    # Reconstruct file:
    # 1. Keep all lines before insert_pos (comments, Doxygen)
    # 2. Add namespace opens
    # 3. All remaining lines
    # 4. Add namespace closes
    
    before = lines[:insert_pos]
    after = lines[insert_pos:]
    
    # Remove trailing empty lines
    while after and not after[-1].strip():
        after.pop()
    
    new_content = '\n'.join(before) + '\n'
    new_content += 'namespace themis {\n'
    new_content += 'namespace graph {\n'
    new_content += '\n'
    new_content += '\n'.join(after) + '\n'
    new_content += '\n'
    new_content += '} // namespace graph\n'
    new_content += '} // namespace themis\n'
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path, _name in FILES_NEEDING_FIX.items():
        wrap_with_namespace(file_path)
    print("\n[SUCCESS] All 7 files wrapped with themis::graph namespace")
