#!/usr/bin/env python3
"""
Phase 3: Context Extractor - Extract surrounding code context for gaps.

Reads source files and extracts context around gap lines for code generation.
"""

import os
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class ContextExtractor:
    """Extract source code context around gaps."""
    
    def __init__(self, context_lines: int = 5):
        """Initialize extractor.
        
        Args:
            context_lines: Number of lines before/after gap to include
        """
        self.context_lines = context_lines
        self.file_cache = {}
    
    def get_file_lines(self, filepath: str) -> Optional[List[str]]:
        """Load file and cache it.
        
        Args:
            filepath: Path to source file (Windows or Unix style)
            
        Returns:
            List of lines, or None if file not found
        """
        # Normalize path
        filepath = str(filepath).replace("\\", os.sep)
        
        if filepath in self.file_cache:
            return self.file_cache[filepath]
        
        full_path = Path(filepath)
        if not full_path.exists():
            # Try relative to project root
            full_path = Path(".") / filepath
            if not full_path.exists():
                return None
        
        try:
            with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
                lines = f.readlines()
            self.file_cache[filepath] = lines
            return lines
        except Exception as e:
            print(f"[ERROR] Cannot read {filepath}: {e}")
            return None
    
    def extract_context(self, filepath: str, gap_line: int) -> Dict:
        """Extract context around a gap.
        
        Args:
            filepath: Source file path
            gap_line: Line number where gap is (1-indexed)
            
        Returns:
            {
              'file': filepath,
              'line': gap_line,
              'before': [list of lines before gap],
              'gap': 'the line at gap_line',
              'after': [list of lines after gap],
              'context': 'formatted context string'
            }
        """
        lines = self.get_file_lines(filepath)
        if not lines:
            return {
                'file': filepath,
                'line': gap_line,
                'error': f'Cannot read file: {filepath}'
            }
        
        # Convert to 0-indexed
        gap_idx = gap_line - 1
        
        if gap_idx < 0 or gap_idx >= len(lines):
            return {
                'file': filepath,
                'line': gap_line,
                'error': f'Line {gap_line} out of range (file has {len(lines)} lines)'
            }
        
        # Extract context
        start_idx = max(0, gap_idx - self.context_lines)
        end_idx = min(len(lines), gap_idx + self.context_lines + 1)
        
        before = lines[start_idx:gap_idx]
        gap = lines[gap_idx]
        after = lines[gap_idx + 1:end_idx]
        
        # Build context string for Ollama
        context_str = "// BEFORE:\n"
        for i, line in enumerate(before, start=start_idx + 1):
            context_str += f"{i:4d}: {line.rstrip()}\n"
        
        context_str += f"\n// GAP AT LINE {gap_line}:\n"
        context_str += f"{gap_line:4d}: {gap.rstrip()}\n"
        
        context_str += "\n// AFTER:\n"
        for i, line in enumerate(after, start=gap_idx + 2):
            context_str += f"{i:4d}: {line.rstrip()}\n"
        
        return {
            'file': filepath,
            'line': gap_line,
            'before': before,
            'gap': gap.rstrip(),
            'after': after,
            'context': context_str,
            'start_line': start_idx + 1,
            'end_line': end_idx
        }
    
    def get_function_signature(self, filepath: str, gap_line: int, 
                               context_depth: int = 20) -> Optional[str]:
        """Try to extract the enclosing function signature.
        
        Searches backwards from gap_line to find a function definition.
        """
        lines = self.get_file_lines(filepath)
        if not lines:
            return None
        
        gap_idx = gap_line - 1
        search_start = max(0, gap_idx - context_depth)
        
        # Look for function patterns
        for i in range(gap_idx, search_start, -1):
            line = lines[i].strip()
            
            # C++ function patterns
            if '(' in line and ')' in line:
                if any(keyword in line for keyword in ['void', 'int', 'bool', 'auto', 'const', 'static']):
                    # Found likely function signature
                    return lines[i].rstrip()
        
        return None


def main():
    """Demo: Extract context for a gap."""
    
    # Example gap: INDEX module, ann_index.cpp line 293
    extractor = ContextExtractor(context_lines=5)
    
    result = extractor.extract_context("src/index/ann_index.cpp", 293)
    
    if 'error' in result:
        print(f"[ERROR] {result['error']}")
        return 1
    
    print("="*70)
    print(f"CONTEXT FOR: {result['file']}:{result['line']}")
    print("="*70)
    print(result['context'])
    print()
    
    # Try to find function signature
    func_sig = extractor.get_function_signature("src/index/ann_index.cpp", 293)
    if func_sig:
        print(f"Function: {func_sig}")
    
    return 0


if __name__ == "__main__":
    exit(main())
