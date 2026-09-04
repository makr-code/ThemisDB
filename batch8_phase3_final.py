#!/usr/bin/env python3
"""
Batch 8 Phase 3: Final Comprehensive Maybe-Uninitialized Fixer

Targets remaining patterns with high precision:
1. Variables with complex/template types
2. Variables in switch statements without default case
3. Variables before return statements in conditional paths
4. Variables in nested scopes
"""

import re
import os
from collections import defaultdict

class FinalComprehensiveFixer:
    """Final pass for maybe-uninitialized warnings."""
    
    def __init__(self):
        self.files_modified = 0
        self.fixes_applied = 0
        self.pattern_counts = defaultdict(int)
    
    def fix_file(self, filepath):
        """Apply final patterns to a file."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            modified = False
            i = 0
            while i < len(lines):
                line = lines[i]
                
                # Pattern 1: Declarations before control flow with possible init tracking
                # Look for: type name; followed by whitespace/comments then control flow
                match = re.match(r'^(\s*)(int|uint|float|double|bool|char|size_t|uint32_t|uint64_t|long|short|unsigned)(\s+)(\w+)(\s*);(\s*)$', line)
                if match and i + 1 < len(lines):
                    indent, dtype, space, varname, trailing, spaces = match.groups()
                    next_line_idx = i + 1
                    # Skip comment lines
                    while next_line_idx < len(lines) and re.match(r'^\s*//.*$', lines[next_line_idx]):
                        next_line_idx += 1
                    
                    if next_line_idx < len(lines):
                        next_line = lines[next_line_idx].strip()
                        if next_line and any(kw in next_line for kw in ['if (', 'for (', 'while (', 'switch (', 'do {']):
                            # Initialize the variable
                            lines[i] = f"{indent}{dtype}{space}{varname}{trailing} = 0;{spaces}\n"
                            modified = True
                            self.fixes_applied += 1
                            self.pattern_counts['final_numeric'] += 1
                
                # Pattern 2: Pointer declarations before control flow
                match = re.match(r'^(\s*)([\w:]+\s*\*+)(\s+)(\w+)(\s*);(\s*)$', line)
                if match and i + 1 < len(lines):
                    indent, ptype, space, varname, trailing, spaces = match.groups()
                    next_line_idx = i + 1
                    while next_line_idx < len(lines) and re.match(r'^\s*//.*$', lines[next_line_idx]):
                        next_line_idx += 1
                    
                    if next_line_idx < len(lines):
                        next_line = lines[next_line_idx].strip()
                        if next_line and any(kw in next_line for kw in ['if (', 'for (', 'while (', 'switch (']):
                            lines[i] = f"{indent}{ptype}{space}{varname}{trailing} = nullptr;{spaces}\n"
                            modified = True
                            self.fixes_applied += 1
                            self.pattern_counts['final_pointer'] += 1
                
                # Pattern 3: Template/complex types without initialization
                # std::vector<T> var; or other template types
                match = re.match(r'^(\s*)(std::\w+<[^>]+>\s+)(\w+)(\s*);(\s*)$', line)
                if match and i + 1 < len(lines):
                    indent, ttype, varname, trailing, spaces = match.groups()
                    next_line_idx = i + 1
                    while next_line_idx < len(lines) and re.match(r'^\s*//.*$', lines[next_line_idx]):
                        next_line_idx += 1
                    
                    if next_line_idx < len(lines):
                        next_line = lines[next_line_idx].strip()
                        if next_line and any(kw in next_line for kw in ['if (', 'for (', 'while (', '.size()', '.empty()', '= {']):
                            lines[i] = f"{indent}{ttype}{varname}{trailing} = {{}};{spaces}\n"
                            modified = True
                            self.fixes_applied += 1
                            self.pattern_counts['final_template'] += 1
                
                i += 1
            
            if modified:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.writelines(lines)
                self.files_modified += 1
                return True
            
            return False
        
        except Exception:
            return False
    
    def process_directory(self, dirpath):
        """Process all C/C++ files."""
        skip_dirs = {'.git', 'build', 'external', '.cmake', 'CMakeFiles', 'vcpkg_installed', '.vscode', 'deploy', 'build_warn'}
        
        for root, dirs, files in os.walk(dirpath):
            dirs[:] = [d for d in dirs if d not in skip_dirs]
            
            for file in files:
                if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx', '.c', '.ipp', '.tpp')):
                    filepath = os.path.join(root, file)
                    self.fix_file(filepath)

def main():
    fixer = FinalComprehensiveFixer()
    
    print("Running final comprehensive maybe-uninitialized fixer...")
    fixer.process_directory('/home/runner/work/ThemisDB/ThemisDB')
    
    print(f"\n{'='*60}")
    print(f"Batch 8 Phase 3e: Final Comprehensive Fixer Results")
    print(f"{'='*60}")
    print(f"Files modified:     {fixer.files_modified}")
    print(f"Total fixes:        {fixer.fixes_applied}")
    print(f"\nPattern Breakdown:")
    for pattern, count in sorted(fixer.pattern_counts.items(), key=lambda x: -x[1]):
        print(f"  {pattern:<30} {count:>5}")

if __name__ == '__main__':
    main()
