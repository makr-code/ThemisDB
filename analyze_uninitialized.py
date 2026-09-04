#!/usr/bin/env python3
"""
Analyze and fix -Wmaybe-uninitialized compiler warnings.
Patterns:
1. Variables conditionally assigned in branches
2. Loop-conditional initialization
3. Switch statement without default
4. Pointer/Reference conditionally assigned
"""

import re
import os
import sys
from pathlib import Path

# Common patterns for maybe-uninitialized variables
PATTERNS = [
    # Pattern 1: Conditional assignment without else
    # int var; if (...) var = value;
    {
        'name': 'conditional_assignment',
        'regex': r'(\s+)((?:int|long|double|float|bool|char|size_t|uint|auto)\s+\w+)\s*;\s*(?://.*\n)?\s*if\s*\(',
        'fix': r'\1\2 = 0;  // initialized for -Wmaybe-uninitialized\n\1if ('
    },
    # Pattern 2: Pointer conditionally assigned
    # T* ptr; if (...) ptr = &obj;
    {
        'name': 'pointer_conditional',
        'regex': r'(\s+)([\w:*]+\s*\*\s*\w+)\s*;\s*(?://.*\n)?\s*if\s*\(',
        'fix': r'\1\2 = nullptr;  // initialized for -Wmaybe-uninitialized\n\1if ('
    },
]

def scan_file(filepath):
    """Scan file for potential maybe-uninitialized patterns."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            lines = content.split('\n')
        
        issues = []
        
        # Look for uninitialized variable patterns
        for i, line in enumerate(lines):
            # Pattern 1: Variable declaration without initialization
            # followed by conditional assignment or loop
            match = re.match(r'^\s*(int|long|double|float|bool|char|size_t|uint|auto)\s+(\w+)\s*;', line)
            if match:
                dtype = match.group(1)
                varname = match.group(2)
                # Check next non-comment line
                for j in range(i + 1, min(i + 5, len(lines))):
                    next_line = lines[j].strip()
                    if not next_line or next_line.startswith('//'):
                        continue
                    if any(x in next_line for x in ['if (', 'for (', 'while (', 'do {']):
                        issues.append({
                            'line': i + 1,
                            'type': 'uninitialized_conditional',
                            'var': varname,
                            'dtype': dtype
                        })
                    break
            
            # Pattern 2: Pointer without initialization
            match = re.match(r'^\s*([\w:*]+\s*\*\s*)(\w+)\s*;', line)
            if match:
                full_type = match.group(1)
                varname = match.group(2)
                for j in range(i + 1, min(i + 5, len(lines))):
                    next_line = lines[j].strip()
                    if not next_line or next_line.startswith('//'):
                        continue
                    if any(x in next_line for x in ['if (', '=', 'for (', 'while (']):
                        if 'if (' in next_line or 'for (' in next_line or 'while (' in next_line:
                            issues.append({
                                'line': i + 1,
                                'type': 'pointer_uninitialized',
                                'var': varname,
                                'dtype': full_type.strip()
                            })
                    break
        
        return issues
    except Exception as e:
        return []

def main():
    # Find all C/C++ source files
    cpp_files = []
    for root, dirs, files in os.walk('/home/runner/work/ThemisDB/ThemisDB/src'):
        # Skip certain directories
        dirs[:] = [d for d in dirs if d not in ['.git', 'build', 'external']]
        
        for file in files:
            if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx')):
                cpp_files.append(os.path.join(root, file))
    
    print(f"Found {len(cpp_files)} C/C++ files")
    
    total_issues = 0
    files_with_issues = 0
    
    for fpath in sorted(cpp_files)[:100]:  # Sample first 100 files
        issues = scan_file(fpath)
        if issues:
            files_with_issues += 1
            total_issues += len(issues)
            print(f"\n{fpath}:")
            for issue in issues[:3]:
                print(f"  Line {issue['line']}: {issue['type']} - {issue['var']} ({issue['dtype']})")
    
    print(f"\n\nSummary (sample of 100 files):")
    print(f"  Files with potential issues: {files_with_issues}")
    print(f"  Total potential issues: {total_issues}")
    print(f"  Estimate for {len(cpp_files)} files: {total_issues * len(cpp_files) // 100}")

if __name__ == '__main__':
    main()
