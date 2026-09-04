#!/usr/bin/env python3
"""
Batch 8 Phase 3: Aggressive Maybe-Uninitialized Fixer

This version applies even more aggressive pattern matching to find
and fix uninitialized variable warnings at scale.
"""

import re
import os
from collections import defaultdict

class AggressiveUninitializedFixer:
    """Aggressively fix maybe-uninitialized warnings."""
    
    def __init__(self):
        self.files_modified = 0
        self.fixes_applied = 0
        self.pattern_counts = defaultdict(int)
    
    def fix_single_line_patterns(self, content):
        """Fix patterns on single or few lines."""
        original = content
        
        # Pattern 1: Uninitialized variable in if/switch/loop statement
        # type var; followed by control structure
        content = re.sub(
            r'(\n\s+)(int|uint|float|double|bool|char|size_t|uint32_t|uint64_t|int32_t|int64_t|long|short)(\s+)(\w+)(\s*);(\s*)(?=\n\s+(if|for|while|switch|do)\s*\()',
            r'\1\2\3\4\5 = 0;\6',
            content
        )
        
        # Pattern 2: Pointer uninitialized
        # T* var; followed by control structure
        content = re.sub(
            r'(\n\s+)([\w:]+\s*\*+)(\s+)(\w+)(\s*);(\s*)(?=\n\s+(if|for|while|switch)\s*\()',
            r'\1\2\3\4\5 = nullptr;\6',
            content
        )
        
        # Pattern 3: Variables declared without init at block start
        # {
        #   type var;
        #   if (...)
        content = re.sub(
            r'(\{\s*\n\s+)(int|uint|float|double|bool|size_t|uint32_t|uint64_t)(\s+)(\w+)(\s*);',
            r'\1\2\3\4\5 = 0;',
            content
        )
        
        return content
    
    def fix_multiline_patterns(self, filepath):
        """Fix patterns that span multiple lines."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            modified = False
            
            for i in range(len(lines) - 1):
                line = lines[i]
                
                # Look for declarations without initialization
                match = re.match(r'^(\s+)(int|uint|float|double|bool|char|size_t|uint32_t|uint64_t|long)(\s+)(\w+)(\s*);(\s*)$', line)
                if match:
                    varname = match.group(4)
                    # Check if next line is control flow
                    next_line = lines[i + 1].strip() if i + 1 < len(lines) else ''
                    if any(kw in next_line for kw in ['if (', 'for (', 'while (', 'switch (', 'do {']):
                        # Initialize it
                        lines[i] = re.sub(
                            rf'(\s+)(int|uint|float|double|bool|char|size_t|uint32_t|uint64_t|long)(\s+{re.escape(varname)}\s*);(\s*)$',
                            r'\1\2\3 = 0;\4',
                            line
                        )
                        modified = True
                
                # Look for pointer declarations without initialization
                match = re.match(r'^(\s+)([\w:]+\s*\*+)(\s+)(\w+)(\s*);(\s*)$', line)
                if match:
                    varname = match.group(4)
                    next_line = lines[i + 1].strip() if i + 1 < len(lines) else ''
                    if any(kw in next_line for kw in ['if (', 'for (', 'while (', 'switch (']):
                        lines[i] = re.sub(
                            rf'(\s+)([\w:]+\s*\*+)(\s+{re.escape(varname)}\s*);(\s*)$',
                            r'\1\2\3 = nullptr;\4',
                            line
                        )
                        modified = True
            
            if modified:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.writelines(lines)
            
            return modified
        
        except Exception:
            return False
    
    def process_file(self, filepath):
        """Process a single file."""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                original = f.read()
            
            # Apply single-line patterns
            content = self.fix_single_line_patterns(original)
            
            # Count changes
            fixes = len(re.findall(r' = 0;', content)) - len(re.findall(r' = 0;', original))
            fixes += len(re.findall(r' = nullptr;', content)) - len(re.findall(r' = nullptr;', original))
            
            if fixes > 0:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.files_modified += 1
                self.fixes_applied += fixes
                self.pattern_counts['single_line_fixes'] += fixes
                return True
            
            # Try multiline patterns
            if self.fix_multiline_patterns(filepath):
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    new_content = f.read()
                fixes = len(re.findall(r' = 0;', new_content)) - len(re.findall(r' = 0;', original))
                fixes += len(re.findall(r' = nullptr;', new_content)) - len(re.findall(r' = nullptr;', original))
                if fixes > 0:
                    self.fixes_applied += fixes
                    self.pattern_counts['multiline_fixes'] += fixes
                return fixes > 0
            
            return False
        
        except Exception:
            return False
    
    def process_directory(self, dirpath):
        """Process all C/C++ files recursively."""
        skip_dirs = {'.git', 'build', 'external', '.cmake', 'CMakeFiles', 'vcpkg_installed', '.vscode', 'deploy', 'build_warn'}
        
        for root, dirs, files in os.walk(dirpath):
            dirs[:] = [d for d in dirs if d not in skip_dirs]
            
            for file in files:
                if file.endswith(('.cpp', '.cc', '.h', '.hpp', '.cxx', '.c', '.ipp', '.tpp')):
                    filepath = os.path.join(root, file)
                    self.process_file(filepath)

def main():
    fixer = AggressiveUninitializedFixer()
    
    print("Running aggressive maybe-uninitialized fixer...")
    fixer.process_directory('/home/runner/work/ThemisDB/ThemisDB')
    
    print(f"\n{'='*60}")
    print(f"Batch 8 Phase 3c: Aggressive Fixer Results")
    print(f"{'='*60}")
    print(f"Files modified:     {fixer.files_modified}")
    print(f"Total fixes:        {fixer.fixes_applied}")
    print(f"\nPattern Breakdown:")
    for pattern, count in sorted(fixer.pattern_counts.items(), key=lambda x: -x[1]):
        print(f"  {pattern:<30} {count:>5}")

if __name__ == '__main__':
    main()
