#!/usr/bin/env python3
"""
FINAL FIX: Move all #includes to the TOP before namespace opens.
Standard headers + custom headers, then namespace, then rest of code.
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

def fix_includes_order(file_path):
    """Move all #includes to top, before namespace opens."""
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    lines = content.split('\n')
    
    # Separate: initial comments, includes, then rest
    comments = []
    includes = []
    rest = []
    in_comments = True
    
    for line in lines:
        stripped = line.strip()
        
        # Once we leave comments, everything else is either include or code
        if in_comments:
            if stripped.startswith('#include') or (stripped.startswith('namespace') and includes):
                # We're exiting comments
                in_comments = False
                if stripped.startswith('#include'):
                    includes.append(line)
                else:
                    rest.append(line)
            elif stripped.startswith('/**') or stripped.startswith('/*') or stripped.startswith('*') or stripped.startswith('*/') or stripped.startswith('//') or not stripped:
                comments.append(line)
            elif stripped.startswith('#include'):
                includes.append(line)
            else:
                in_comments = False
                rest.append(line)
        else:
            if stripped.startswith('#include'):
                includes.append(line)
            else:
                rest.append(line)
    
    # Now check if rest already has namespace opens
    has_namespace_open = False
    ns_open_lines = []
    for i, line in enumerate(rest):
        if 'namespace themis' in line or 'namespace graph' in line:
            has_namespace_open = True
            ns_open_lines = rest[:i+2]  # Include two namespace opens
            rest = rest[i+2:]
            break
    
    # Rebuild file
    new_lines = comments + includes
    if has_namespace_open:
        new_lines += ns_open_lines
    else:
        new_lines += ['', 'namespace themis {', 'namespace graph {']
    new_lines += rest
    
    # Add namespace closes if not present
    content_str = '\n'.join(rest).strip()
    if not content_str.endswith('} // namespace graph') and not content_str.endswith('} // namespace themis'):
        new_lines.append('')
        new_lines.append('} // namespace graph')
        new_lines.append('} // namespace themis')
    
    new_content = '\n'.join(new_lines)
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_NEEDING_FIX:
        fix_includes_order(file_path)
    print("\n[SUCCESS] All 7 files reorganized with correct include order")
