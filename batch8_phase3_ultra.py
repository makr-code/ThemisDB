#!/usr/bin/env python3
"""
Batch 8 Phase 3: Ultra-Aggressive Pattern Matcher

Looking for additional patterns:
1. Variables in complex conditional branches
2. Variables before goto/return with conditional paths
3. Variables in struct/class initializers
4. Enums and status codes
"""

import re
import os
from collections import defaultdict

class UltraAggressiveFixer:
    """Ultra-aggressive fixing for maybe-uninitialized warnings."""
    
    def __init__(self):
        self.files_modified = 0
        self.fixes_applied = 0
        self.pattern_counts = defaultdict(int)
    
    def fix_file(self, filepath):
        """Apply all remaining patterns."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                original = f.read()
            
            content = original
            
            # Pattern 1: Variables declared but not initialized in any block
            # Look across multiple lines for variables that appear in conditional paths
            lines = content.split('\n')
            
            for i in range(len(lines)):
                # Find declarations
                match = re.match(r'^(\s+)(int|uint|float|double|bool|size_t|uint32_t|uint64_t|long|short|char|auto|std::\w+)(\s+)(\w+)(\s*);$', lines[i])
                if match:
                    indent, dtype, space, varname, trailing = match.groups()
                    
                    # Look ahead for usage
                    for j in range(i + 1, min(i + 10, len(lines))):
                        next_line = lines[j]
                        
                        # If we see the variable being used without initialization, init it
                        if varname in next_line and any(op in next_line for op in ['=', '+', '-', '*', '/', 'if (', 'return ', 'printf', 'cout', '.', '->', '(']):
                            if '=' not in lines[i]:
                                # Initialize it
                                lines[i] = f"{indent}{dtype}{space}{varname}{trailing} = {{}};"
                                self.fixes_applied += 1
                                self.pattern_counts['use_without_init'] += 1
                                break
                        elif any(kw in next_line for kw in ['if (', 'while (', 'for (']):
                            # Before control flow, might be used
                            if '=' not in lines[i]:
                                lines[i] = f"{indent}{dtype}{space}{varname}{trailing} = {{}};"
                                self.fixes_applied += 1
                                self.pattern_counts['control_flow'] += 1
                            break
            
            content = '\n'.join(lines)
            
            # Pattern 2: Struct member initializations
            content = re.sub(
                r'(\n\s+)(\w+)\s+(\w+)\s*;(\s*)(?=\n\s+this->|\n\s+return|\n\s+if)',
                r'\1\2 \3 = {};\4',
                content
            )
            
            if content != original:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.files_modified += 1
                return True
            
            return False
        
        except Exception:
            return False
    
    def process_directory(self, dirpath):
        """Process all files."""
        skip_dirs = {'.git', 'build', 'external', '.cmake', 'CMakeFiles', 'vcpkg_installed', '.vscode', 'deploy', 'build_warn'}
        
        for root, dirs, files in os.walk(dirpath):
            dirs[:] = [d for d in dirs if d not in skip_dirs]
            
            for file in files:
                if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx', '.c', '.ipp', '.tpp')):
                    filepath = os.path.join(root, file)
                    self.fix_file(filepath)

def main():
    fixer = UltraAggressiveFixer()
    
    print("Running ultra-aggressive maybe-uninitialized fixer...")
    fixer.process_directory('/home/runner/work/ThemisDB/ThemisDB')
    
    print(f"\n{'='*60}")
    print(f"Batch 8 Phase 3f: Ultra-Aggressive Fixer Results")
    print(f"{'='*60}")
    print(f"Files modified:     {fixer.files_modified}")
    print(f"Total fixes:        {fixer.fixes_applied}")
    print(f"\nPattern Breakdown:")
    for pattern, count in sorted(fixer.pattern_counts.items(), key=lambda x: -x[1]):
        print(f"  {pattern:<30} {count:>5}")

if __name__ == '__main__':
    main()
