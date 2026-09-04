#!/usr/bin/env python3
"""
Batch 8 Phase 3: Comprehensive Maybe-Uninitialized Fixer - v2

More aggressive pattern matching to capture all uninitialized variable patterns.
"""

import re
import os
from pathlib import Path
from collections import defaultdict

class MaybeUninitializedFixerV2:
    def __init__(self):
        self.files_modified = 0
        self.fixes_applied = 0
        self.pattern_counts = defaultdict(int)
        self.files_list = []
    
    def fix_file(self, filepath):
        """Apply fixes to a single file."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                original = f.read()
            
            content = original
            fixed = False
            
            # Pattern 1: Simple variable declaration without initialization
            # followed by conditional/loop, with no else clause
            # Matches: type varname; \n if/for/while/switch (
            patterns = [
                # Basic numeric types
                (r'(\n\s+)(int|long|long long|double|float|bool|char|size_t|uint|uint32_t|uint64_t|int32_t|int64_t|unsigned int|unsigned long)(\s+)(\w+)(\s*);(\s*\n\s+)(if|for|while|switch|do)\s*\(', 
                 r'\1\2\3\4\5 = 0;\6\7 \8('),
                
                # Pointer types
                (r'(\n\s+)([\w:]+\s*\*+\s*)(\w+)(\s*);(\s*\n\s+)(if|for|while)\s*\(',
                 r'\1\2\3\4 = nullptr;\5\6 \7('),
                
                # Reference types (less common to initialize, but still valid)
                (r'(\n\s+)([\w:]+\s*&+\s*)(\w+)(\s*);',
                 None),  # Skip - references must be initialized at declaration
            ]
            
            for pattern, repl in patterns:
                if repl is None:
                    continue
                content, count = re.subn(pattern, repl, content)
                if count > 0:
                    fixed = True
                    self.fixes_applied += count
                    self.pattern_counts['declared_uninitialized'] += count
            
            # Pattern 2: More specific patterns - look for common scenarios
            # Variables used in comparisons after conditional assignment
            
            # Pattern 3: Variables in return statements after conditional paths
            # return var; where var might not be initialized
            lines = content.split('\n')
            for i in range(len(lines)):
                line = lines[i]
                # Look for return statements with simple variables
                return_match = re.match(r'^(\s+)return\s+(\w+)\s*;', line)
                if return_match:
                    varname = return_match.group(2)
                    # Look backward for declaration
                    for j in range(max(0, i - 20), i):
                        decl_match = re.search(rf'(int|long|double|float|bool|char|size_t|uint)\s+{re.escape(varname)}\s*;', lines[j])
                        if decl_match and '=' not in lines[j]:
                            # Found uninitialized variable returned
                            lines[j] = re.sub(
                                rf'(int|long|double|float|bool|char|size_t|uint)(\s+{re.escape(varname)}\s*);',
                                rf'\1\2 = 0;',
                                lines[j]
                            )
                            fixed = True
                            self.fixes_applied += 1
                            self.pattern_counts['return_uninitialized'] += 1
                            break
            
            if fixed:
                content = '\n'.join(lines)
            
            if content != original:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.files_modified += 1
                self.files_list.append(filepath)
                return True
            
            return False
        
        except Exception as e:
            return False
    
    def process_directory(self, dirpath, exclude_dirs=None):
        """Process all C/C++ files in directory recursively."""
        if exclude_dirs is None:
            exclude_dirs = {'.git', 'build', 'external', '.cmake', 'CMakeFiles', 'vcpkg_installed', '.vscode'}
        
        for root, dirs, files in os.walk(dirpath):
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            
            for file in files:
                if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx', '.c')):
                    filepath = os.path.join(root, file)
                    self.fix_file(filepath)

def main():
    fixer = MaybeUninitializedFixerV2()
    
    # Process all main directories
    dirs_to_process = [
        '/home/runner/work/ThemisDB/ThemisDB/src',
        '/home/runner/work/ThemisDB/ThemisDB/tests',
        '/home/runner/work/ThemisDB/ThemisDB/benchmarks',
        '/home/runner/work/ThemisDB/ThemisDB/examples',
    ]
    
    for dirpath in dirs_to_process:
        if os.path.exists(dirpath):
            print(f"Scanning {dirpath}...")
            fixer.process_directory(dirpath)
    
    print(f"\n=== Batch 8 Phase 3: Maybe-Uninitialized Fix Results (v2) ===")
    print(f"Files modified: {fixer.files_modified}")
    print(f"Total fixes applied: {fixer.fixes_applied}")
    print(f"\nPattern distribution:")
    for pattern, count in sorted(fixer.pattern_counts.items(), key=lambda x: -x[1]):
        print(f"  {pattern}: {count}")
    
    if fixer.files_modified > 0 and fixer.files_modified <= 50:
        print(f"\nModified files:")
        for f in sorted(fixer.files_list)[:50]:
            print(f"  {f}")

if __name__ == '__main__':
    main()
