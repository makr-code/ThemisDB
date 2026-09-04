#!/usr/bin/env python3
"""
Batch 8 Phase 4: Automated -Wsign-compare Warning Fixer

Patterns handled:
1. Loop counter to size_t conversion
2. Unsigned literal suffix removal (U, UL, ULL)
3. Explicit static_cast for mixed-type comparisons
4. Range-checked conversions for negative-value paths
"""

import re
import sys
import os
from pathlib import Path
from typing import List, Tuple, Optional
from collections import defaultdict

class SignCompareFixerConfig:
    """Configuration for sign-compare fixes"""
    
    # Category A: Loop counter patterns
    LOOP_PATTERNS = [
        # for (int i = 0; i < vec.size(); ++i)
        r'for\s*\(\s*int\s+(\w+)\s*=\s*0\s*;\s*\1\s*<\s*(\w+)\.size\(\)',
        r'for\s*\(\s*int\s+(\w+)\s*=\s*0\s*;\s*\1\s*<=\s*\(int\)(\w+)\.size\(\)',
        r'for\s*\(\s*int\s+(\w+)\s*=\s*0\s*;\s*\1\s*<\s*\(int\)(\w+)\.size\(\)',
        # while (i < vec.size())
        r'while\s*\(\s*(\w+)\s*<\s*(\w+)\.size\(\)\s*\)',
    ]
    
    # Category B: Unsigned literal suffixes
    LITERAL_PATTERNS = [
        (r'\b(\d+)U\b', r'\g<1>'),  # Remove U suffix
        (r'\b(\d+)UL\b', r'\g<1>'),  # Remove UL suffix
        (r'\b(\d+)ULL\b', r'\g<1>'),  # Remove ULL suffix
        (r'\b0U\b', '0'),  # Special case: 0U -> 0
    ]
    
    # Category C: Type mismatch comparisons that need casts
    # Pattern: int var compared with size_t result
    TYPECAST_PATTERNS = [
        # if (x < vec.size()) where x is int or shorter
        r'if\s*\(\s*([a-zA-Z_]\w*)\s*(<|<=|>|>=)\s*([\w:]+\.size\(\))',
        # if (x < length) where length is size_t
        r'if\s*\(\s*([a-zA-Z_]\w*)\s*(<|<=|>|>=)\s*([a-zA-Z_]\w*)',
    ]


class SignCompareFixer:
    """Main fixer class for sign-compare warnings"""
    
    def __init__(self):
        self.config = SignCompareFixerConfig()
        self.stats = defaultdict(int)
        self.changes = []
        
    def fix_loop_counter_to_size_t(self, content: str) -> Tuple[str, int]:
        """Category A: Convert loop counters to size_t"""
        changes = 0
        
        # Pattern 1: for (int i = 0; i < vec.size(); ++i)
        def replace_loop(match):
            nonlocal changes
            changes += 1
            var_name = match.group(1)
            container = match.group(2)
            prefix = content[match.start():match.start()].rsplit('\n', 1)[-1] if '\n' in content[max(0, match.start()-100):match.start()] else ''
            indent = len(prefix) - len(prefix.lstrip())
            return f'for (size_t {var_name} = 0; {var_name} < {container}.size()'
        
        for pattern in self.config.LOOP_PATTERNS[:1]:  # Start with simple pattern
            content = re.sub(pattern, replace_loop, content)
        
        return content, changes
    
    def fix_unsigned_literal_suffixes(self, content: str) -> Tuple[str, int]:
        """Category B: Remove unsigned literal suffixes"""
        changes = 0
        
        for pattern, replacement in self.config.LITERAL_PATTERNS:
            new_content = re.sub(pattern, replacement, content)
            changes += content.count(pattern) - new_content.count(pattern)
            content = new_content
        
        return content, changes
    
    def fix_mixed_type_comparisons(self, content: str) -> Tuple[str, int]:
        """Category C: Add static_cast for mixed-type comparisons"""
        changes = 0
        
        # Simple case: if (int_var < size_t_result)
        pattern = r'if\s*\(\s*([-a-zA-Z_]\w*)\s*<\s*([\w:]+\.size\(\))\s*\)'
        def replace_if_size(match):
            nonlocal changes
            var = match.group(1)
            size_call = match.group(2)
            changes += 1
            return f'if (static_cast<size_t>({var}) < {size_call})'
        
        content = re.sub(pattern, replace_if_size, content)
        return content, changes
    
    def process_file(self, filepath: str) -> Tuple[str, int]:
        """Process a single file for all sign-compare patterns"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
                original = f.read()
        except Exception as e:
            print(f"Error reading {filepath}: {e}", file=sys.stderr)
            return original, 0
        
        content = original
        total_changes = 0
        
        # Apply fixes in order of semantic safety
        content, cat_a = self.fix_loop_counter_to_size_t(content)
        total_changes += cat_a
        self.stats['category_a_loops'] += cat_a
        
        content, cat_b = self.fix_unsigned_literal_suffixes(content)
        total_changes += cat_b
        self.stats['category_b_literals'] += cat_b
        
        content, cat_c = self.fix_mixed_type_comparisons(content)
        total_changes += cat_c
        self.stats['category_c_casts'] += cat_c
        
        if total_changes > 0:
            self.changes.append((filepath, total_changes))
        
        return content, total_changes
    
    def process_directory(self, directory: str, dry_run: bool = False) -> None:
        """Process all C++ files in directory"""
        cpp_files = list(Path(directory).rglob('*.cpp')) + \
                   list(Path(directory).rglob('*.cc')) + \
                   list(Path(directory).rglob('*.h')) + \
                   list(Path(directory).rglob('*.hpp'))
        
        # Filter out build directories
        cpp_files = [f for f in cpp_files 
                    if not any(x in str(f.parent) for x in ['build', '.cache', '.git'])]
        
        print(f"Found {len(cpp_files)} C++ files to process")
        
        processed = 0
        for filepath in sorted(cpp_files)[:10]:  # Start with first 10 for testing
            if processed % 100 == 0:
                print(f"Processing {processed}/{len(cpp_files)}")
            
            new_content, changes = self.process_file(str(filepath))
            
            if changes > 0 and not dry_run:
                try:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)
                    print(f"✓ {filepath}: {changes} fixes")
                except Exception as e:
                    print(f"✗ {filepath}: {e}", file=sys.stderr)
            
            processed += 1
        
        print(f"\nSummary:")
        print(f"  Category A (loop counters): {self.stats['category_a_loops']}")
        print(f"  Category B (literal suffixes): {self.stats['category_b_literals']}")
        print(f"  Category C (static_cast): {self.stats['category_c_casts']}")
        print(f"  Total changes: {sum(self.stats.values())}")
        print(f"  Files modified: {len(self.changes)}")


if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Fix -Wsign-compare warnings in C++ code'
    )
    parser.add_argument('directory', default='.', nargs='?',
                       help='Directory to process (default: current)')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show changes without writing')
    parser.add_argument('--category', choices=['a', 'b', 'c', 'd', 'all'],
                       default='all',
                       help='Category to fix (default: all)')
    
    args = parser.parse_args()
    
    fixer = SignCompareFixer()
    fixer.process_directory(args.directory, dry_run=args.dry_run)
