#!/usr/bin/env python3
"""
Batch 8 Phase 3: Systematic Maybe-Uninitialized Fixer

This fixer systematically initializes variables that are:
1. Declared without initialization
2. Conditionally assigned before use
3. Potentially used uninitialized

Strategy: Aggressive pattern matching across all C/C++ files
to fix -Wmaybe-uninitialized warnings comprehensively.
"""

import re
import os
from collections import defaultdict
from pathlib import Path

class SystematicUninitializedFixer:
    """Systematically fix maybe-uninitialized warnings."""
    
    def __init__(self):
        self.files_modified = 0
        self.fixes_applied = 0
        self.pattern_counts = defaultdict(int)
        self.modified_files = []
    
    def apply_fixes(self, filepath):
        """Apply multiple patterns of fixes to a file."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                original = f.read()
            
            content = original
            fixes_in_file = 0
            
            # Pattern 1: Simple numeric type without initialization before control flow
            # Matches: int var; \n if/for/while/switch/do (
            pattern1 = r'(\n\s+)(int|uint|char|bool|float|double|long|short|size_t|uint32_t|uint64_t|int32_t|int64_t|unsigned|signed)(\s+)(\w+)(\s*);(\s*(?:\n\s*//.*)?)\n(\s+)(if|for|while|do|switch)\s*\('
            
            def replace1(m):
                return f"{m.group(1)}{m.group(2)}{m.group(3)}{m.group(4)}{m.group(5)} = 0;{m.group(6)}\n{m.group(7)}{m.group(8)} ("
            
            new_content, count = re.subn(pattern1, replace1, content)
            if count > 0:
                content = new_content
                fixes_in_file += count
                self.pattern_counts['numeric_default_init'] += count
            
            # Pattern 2: Pointer type without initialization
            # Matches: T* var; \n if/for/while/switch (
            pattern2 = r'(\n\s+)([\w:]+\s*\*+\s*)(\w+)(\s*);(\s*(?:\n\s*//.*)?)\n(\s+)(if|for|while|switch)\s*\('
            
            def replace2(m):
                return f"{m.group(1)}{m.group(2)}{m.group(3)}{m.group(4)} = nullptr;{m.group(5)}\n{m.group(6)}{m.group(7)} ("
            
            new_content, count = re.subn(pattern2, replace2, content)
            if count > 0:
                content = new_content
                fixes_in_file += count
                self.pattern_counts['pointer_init'] += count
            
            # Pattern 3: Variable return pattern
            # Look for return var; where var might be uninitialized
            lines = content.split('\n')
            for i, line in enumerate(lines):
                if re.search(r'\breturn\s+\w+\s*;', line):
                    match = re.search(r'\breturn\s+(\w+)\s*;', line)
                    if match:
                        varname = match.group(1)
                        # Search backward for declaration
                        for j in range(max(0, i - 25), i):
                            decl = re.search(rf'\b(int|uint|float|double|bool|size_t|uint32_t|uint64_t)\s+{re.escape(varname)}\s*;', lines[j])
                            if decl and '=' not in lines[j]:
                                # Found uninitialized declaration
                                lines[j] = re.sub(
                                    rf'\b(int|uint|float|double|bool|size_t|uint32_t|uint64_t)(\s+{re.escape(varname)}\s*);',
                                    rf'\1\2 = 0;',
                                    lines[j]
                                )
                                fixes_in_file += 1
                                self.pattern_counts['return_init'] += 1
                                break
            
            if fixes_in_file > 0:
                content = '\n'.join(lines)
            
            # Write if changed
            if content != original:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.files_modified += 1
                self.fixes_applied += fixes_in_file
                self.modified_files.append((filepath, fixes_in_file))
                return True
            
            return False
        
        except Exception as e:
            return False
    
    def process_all_files(self, root_dir):
        """Process all C/C++ files in root directory."""
        skip_dirs = {'.git', 'build', 'external', '.cmake', 'CMakeFiles', 'vcpkg_installed', '.vscode', 'deploy'}
        
        file_count = 0
        for root, dirs, files in os.walk(root_dir):
            dirs[:] = [d for d in dirs if d not in skip_dirs]
            
            for file in files:
                if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx', '.c', '.ipp', '.tpp')):
                    filepath = os.path.join(root, file)
                    if self.apply_fixes(filepath):
                        file_count += 1
        
        return file_count

def main():
    fixer = SystematicUninitializedFixer()
    
    root = '/home/runner/work/ThemisDB/ThemisDB'
    print(f"Scanning {root}...")
    print("Processing: src/ tests/ benchmarks/ examples/...")
    
    # Process main directories
    for subdir in ['src', 'tests', 'benchmarks', 'examples']:
        dirpath = os.path.join(root, subdir)
        if os.path.exists(dirpath):
            print(f"\n  Processing {subdir}/...")
            fixer.process_all_files(dirpath)
    
    print(f"\n{'='*60}")
    print(f"Batch 8 Phase 3: Maybe-Uninitialized Fixer Results")
    print(f"{'='*60}")
    print(f"Files modified:     {fixer.files_modified}")
    print(f"Total fixes:        {fixer.fixes_applied}")
    print(f"\nPattern Breakdown:")
    for pattern, count in sorted(fixer.pattern_counts.items(), key=lambda x: -x[1]):
        print(f"  {pattern:<30} {count:>5}")
    
    if fixer.modified_files and fixer.files_modified <= 100:
        print(f"\nModified Files (sample):")
        for fpath, count in sorted(fixer.modified_files)[:50]:
            rel_path = fpath.replace(root, '').lstrip('/')
            print(f"  {rel_path:<60} +{count} fixes")

if __name__ == '__main__':
    main()
