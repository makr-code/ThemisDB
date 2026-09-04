#!/usr/bin/env python3
"""
Batch 8 Phase 3: Fix -Wmaybe-uninitialized compiler warnings

Patterns:
1. Variables conditionally assigned in branches
2. Loop-conditional initialization
3. Switch statement without default
4. Pointer/Reference conditionally assigned
"""

import re
import os
from pathlib import Path
from collections import defaultdict

class MaybeUninitializedFixer:
    def __init__(self):
        self.files_modified = 0
        self.fixes_applied = 0
        self.pattern_counts = defaultdict(int)
    
    def fix_file(self, filepath):
        """Apply fixes to a single file."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                original = f.read()
            
            content = original
            initial_fixes = self.fixes_applied
            
            # Apply Pattern 1: Variables conditionally assigned (int, double, etc.)
            # int var; if (...) var = ...
            pattern1 = r'(\n\s+)(int|long|long long|double|float|bool|char|size_t|uint|uint32_t|uint64_t|int32_t|int64_t|auto)(\s+)(\w+)(\s*);(\s*)(?=\n\s+(if|for|while|do)\s*\()'
            repl1 = r'\1\2\3\4\5 = 0;\6'
            content, count = re.subn(pattern1, repl1, content)
            if count > 0:
                self.pattern_counts['conditional_int'] += count
                self.fixes_applied += count
            
            # Apply Pattern 2: Pointer/reference conditionally assigned
            # T* var; if (...) var = ...
            pattern2 = r'(\n\s+)([\w:]+\s*\*\s*)(\w+)(\s*);(\s*)(?=\n\s+(if|for|while|do)\s*\()'
            repl2 = r'\1\2\3\4 = nullptr;\5'
            content, count = re.subn(pattern2, repl2, content)
            if count > 0:
                self.pattern_counts['conditional_pointer'] += count
                self.fixes_applied += count
            
            # Apply Pattern 3: Variables in loop context
            # int var; while/for (...) var = ...
            pattern3 = r'(\n\s+)(int|long|double|float|bool|size_t)(\s+)(\w+)(\s*);(\s*)(?=\n\s+(while|for)\s*\()'
            repl3 = r'\1\2\3\4\5 = 0;\6'
            content, count = re.subn(pattern3, repl3, content)
            if count > 0:
                self.pattern_counts['loop_conditional'] += count
                self.fixes_applied += count
            
            # Apply Pattern 4: Variables before switch
            # int result; switch (...) ...
            pattern4 = r'(\n\s+)(int|long|double|float|bool|size_t)(\s+)(\w+)(\s*);(\s*)(?=\n\s+switch\s*\()'
            repl4 = r'\1\2\3\4\5 = 0;\6'
            content, count = re.subn(pattern4, repl4, content)
            if count > 0:
                self.pattern_counts['switch_conditional'] += count
                self.fixes_applied += count
            
            if content != original:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.files_modified += 1
                return True
            
            return False
        
        except Exception as e:
            return False
    
    def process_directory(self, dirpath):
        """Process all C/C++ files in directory recursively."""
        skip_dirs = {'.git', 'build', 'external', '.cmake', 'CMakeFiles'}
        
        for root, dirs, files in os.walk(dirpath):
            dirs[:] = [d for d in dirs if d not in skip_dirs]
            
            for file in files:
                if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx', '.c')):
                    filepath = os.path.join(root, file)
                    self.fix_file(filepath)

def main():
    fixer = MaybeUninitializedFixer()
    
    # Process source directory
    srcdir = '/home/runner/work/ThemisDB/ThemisDB/src'
    print(f"Scanning {srcdir}...")
    fixer.process_directory(srcdir)
    
    print(f"\n=== Batch 8 Phase 3: Maybe-Uninitialized Fix Results ===")
    print(f"Files modified: {fixer.files_modified}")
    print(f"Total fixes applied: {fixer.fixes_applied}")
    print(f"\nPattern distribution:")
    for pattern, count in sorted(fixer.pattern_counts.items(), key=lambda x: -x[1]):
        print(f"  {pattern}: {count}")

if __name__ == '__main__':
    main()
