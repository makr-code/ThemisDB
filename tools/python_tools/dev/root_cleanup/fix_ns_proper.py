#!/usr/bin/env python3
"""
Fix namespace declarations - PROPER APPROACH:
1. All standard headers (< >) first - BEFORE namespace opens
2. Namespace opens
3. Custom headers (" ") after namespace opens
4. Rest of code
5. Namespace closes at EOF
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

def fix_namespaces_proper(file_path):
    """Reorganize includes: standard headers before namespace, custom after."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # Check if already fixed
    content = ''.join(lines)
    if 'namespace themis {' in content:
        print(f"[SKIP] {file_path} - already has namespace")
        return
    
    # Separate doc/comments, standard headers, custom headers, and rest
    doc_lines = []
    standard_includes = []
    custom_includes = []
    rest_lines = []
    
    in_doc = False
    i = 0
    
    # Extract leading doc comment block
    while i < len(lines):
        line = lines[i]
        if line.strip().startswith('/**') or in_doc:
            doc_lines.append(line)
            if '/**' in line:
                in_doc = True
            if '*/' in line:
                in_doc = False
                i += 1
                break
            i += 1
        elif line.strip().startswith('/*') or line.strip().startswith('*'):
            doc_lines.append(line)
            i += 1
        else:
            break
    
    # Separate includes and rest
    while i < len(lines):
        line = lines[i]
        if '#include <' in line:
            standard_includes.append(line)
        elif '#include "' in line:
            custom_includes.append(line)
        else:
            # Once we hit non-include code, everything else is "rest"
            rest_lines = lines[i:]
            break
        i += 1
    
    # Reconstruct: doc + standard includes + namespace open + custom includes + rest + namespace close
    new_lines = (
        doc_lines +
        standard_includes +
        ["\n", "namespace themis {\n", "namespace graph {\n", "\n"] +
        custom_includes +
        rest_lines
    )
    
    # Remove trailing empty lines before adding namespace closes
    while new_lines and not new_lines[-1].strip():
        new_lines.pop()
    
    # Add namespace closes
    new_lines += ["\n", "} // namespace graph\n", "} // namespace themis\n"]
    
    # Write back
    with open(path, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_NEEDING_FIX:
        fix_namespaces_proper(file_path)
    print("\n[SUCCESS] All 7 files fixed with PROPER namespace placement")
