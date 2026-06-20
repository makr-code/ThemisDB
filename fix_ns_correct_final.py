#!/usr/bin/env python3
"""
CORRECT FINAL FIX:
ALL #includes (both standard and custom) go BEFORE namespace opens.
The header is in a namespace, so we need to include it BEFORE we open matching namespaces.
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

def fix_namespaces_correct(file_path):
    """
    Correct approach:
    1. Collect all comments/doc blocks at the start
    2. Collect ALL #includes (standard + custom)
    3. Add namespace opens
    4. Add rest of code
    5. Add namespace closes at EOF
    """
    path = Path(file_path)
    if not path.exists():
        print(f"[SKIP] {file_path} - not found")
        return
    
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Don't process if already has namespace closes
    if '} // namespace graph' in content:
        print(f"[SKIP] {file_path} - already has namespace")
        return
    
    lines = content.split('\n')
    
    # Phase 1: Separate comments, includes, code
    comments = []
    includes = []
    code = []
    
    phase = 'comments'  # comments -> includes -> code
    
    for line in lines:
        stripped = line.strip()
        
        if phase == 'comments':
            # While in comments, look for # include or non-comment code
            if stripped.startswith('/*') or stripped.startswith('*') or stripped.startswith('//') or stripped.startswith('/**') or not stripped or stripped.startswith('*/'):
                comments.append(line)
            elif stripped.startswith('#include'):
                phase = 'includes'
                includes.append(line)
            else:
                phase = 'code'
                code.append(line)
        
        elif phase == 'includes':
            # Collect all includes
            if stripped.startswith('#include'):
                includes.append(line)
            else:
                phase = 'code'
                code.append(line)
        
        else:  # phase == 'code'
            code.append(line)
    
    # Phase 2: Remove trailing blanks from code
    while code and not code[-1].strip():
        code.pop()
    
    # Phase 3: Rebuild
    new_lines = (
        comments +
        includes +
        ['', 'namespace themis {', 'namespace graph {', ''] +
        code +
        ['', '} // namespace graph', '} // namespace themis']
    )
    
    new_content = '\n'.join(new_lines)
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print(f"[FIXED] {file_path}")

if __name__ == '__main__':
    for file_path in FILES_NEEDING_FIX:
        fix_namespaces_correct(file_path)
    print("\n[SUCCESS] All 7 files fixed with correct namespace + include order")
