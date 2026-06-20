#!/usr/bin/env python3
"""
Fix C++ namespace qualification for graph module method DEFINITIONS.
Only fixes actual method definitions (Type ClassName::method(...)),
NOT class definitions or nested types.
"""

import re
from pathlib import Path

# Map of (file_path, class_name_prefix) pairs
FIXES = [
    ("src/graph/graph_query_optimizer.cpp", "GraphQueryOptimizer"),
    ("src/graph/knowledge_graph_reasoner.cpp", "KnowledgeGraphReasoner"),
    ("src/graph/rotate_completion.cpp", ["RotatEModel", "KGCompletionEngine", "InferenceStore"]),
    ("src/graph/distributed_graph.cpp", ["LocalShardGraphExecutor", "DistributedGraphManager"]),
    ("src/graph/parallel_traversal.cpp", "ParallelTraversal"),
    ("src/graph/scheduled_edge_refresh.cpp", "ScheduledGraphEdgeRefreshEngine"),
]

def fix_file(filepath, class_names):
    """Fix namespace qualifications only for method definitions."""
    if isinstance(class_names, str):
        class_names = [class_names]
    
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    modified = False
    
    for i, line in enumerate(lines):
        # Skip pure comment/whitespace lines
        stripped = line.strip()
        if not stripped or stripped.startswith('//'):
            continue
        
        for class_name in class_names:
            # Check if this line has a method definition pattern
            # Pattern: `Type ClassName::method(...)` at line start (with optional leading whitespace/Result<>)
            # Must have :: and look like a method definition
            
            # Match: any return type + ClassName:: + method name + (
            if f"{class_name}::" in line and "themis::graph::" not in line:
                # Only fix if it looks like a METHOD DEFINITION (not class definition or type usage)
                # Check: does NOT start with "class" or "struct"
                if not re.search(rf"^\s*(class|struct)\s+{re.escape(class_name)}", line):
                    # This is likely a method definition
                    # Replace ClassName:: with themis::graph::ClassName::
                    new_line = line.replace(f"{class_name}::", f"themis::graph::{class_name}::")
                    if new_line != line:
                        lines[i] = new_line
                        modified = True
    
    if modified:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(lines)
        return True
    return False

def main():
    root = Path(".")
    
    for file_path, class_names in FIXES:
        full_path = root / file_path
        if not full_path.exists():
            print(f"ERROR: File not found: {full_path}")
            return 1
        
        if isinstance(class_names, str):
            class_names_display = class_names
        else:
            class_names_display = ", ".join(class_names)
        
        print(f"Fixing {file_path} ({class_names_display})...", end=" ")
        
        if fix_file(full_path, class_names):
            print("✓ FIXED")
        else:
            print("⊘ NO CHANGES")
    
    print("\nAll files processed successfully!")
    return 0

if __name__ == "__main__":
    exit(main())
