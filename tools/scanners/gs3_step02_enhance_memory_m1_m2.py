#!/usr/bin/env python3
"""
Phase 1-4 Enhancement: M-1 & M-2 Memory Safety Detection (Enhanced)

CWE-416: Use After Free (M-1)
CWE-415: Double Free (M-2)

Enhanced Patterns:
- Use-after-free: pointer dereference after delete/free
- Double-free: pointer freed twice
- Use-after-scope: stack variable used after it goes out of scope
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional, Set
from enum import Enum


class MemorySafetyGapType(Enum):
    """Memory safety gap categories"""
    USE_AFTER_FREE = "use_after_free"
    DOUBLE_FREE = "double_free"
    USE_AFTER_SCOPE = "use_after_scope"
    MEMORY_LEAK_POTENTIAL = "memory_leak_potential"


@dataclass
class MemorySafetyGap:
    """Represents a memory safety gap"""
    file_path: str
    line_num: int
    gap_type: MemorySafetyGapType
    snippet: str
    severity: str  # CRITICAL, HIGH
    description: str
    remediation: str
    pattern_name: str
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
            'pattern': self.pattern_name,
            'enhancement': 'M-1/M-2',
            'cwe': 'CWE-416/CWE-415',
        }


class MemorySafetyScanner:
    """M-1/M-2: Memory Safety Detection"""
    
    # Pattern M-1: Use-After-Free Detection
    USE_AFTER_FREE_PATTERNS = {
        'delete_then_use': (
            re.compile(r'delete\s+\w+;.*\n.*\w+\s*->', re.MULTILINE),
            'CRITICAL',
            'Pointer used after delete'
        ),
        'free_then_use': (
            re.compile(r'free\s*\(\s*\w+\s*\);.*\n.*(?:\w+\s*->|\*\w+)', re.MULTILINE),
            'CRITICAL',
            'Pointer used after free'
        ),
        'delete_array_then_index': (
            re.compile(r'delete\s*\[\s*\]\s+\w+;.*\n.*\w+\s*\['),
            'CRITICAL',
            'Array pointer used after delete[]'
        ),
        'nullcheck_after_delete': (
            re.compile(r'delete\s+(\w+);.*\nif\s*\(\s*\1\s*(?:!=|==)\s*nullptr\s*\)', re.MULTILINE),
            'CRITICAL',
            'Use-after-free pattern: delete followed by null check'
        ),
        'use_in_catch': (
            re.compile(r'delete\s+(\w+);.*?\n.*?\}.*?catch.*?{.*?\1', re.MULTILINE | re.DOTALL),
            'HIGH',
            'Deleted pointer potentially used in catch block'
        ),
    }
    
    # Pattern M-2: Double-Free Detection
    DOUBLE_FREE_PATTERNS = {
        'double_delete': (
            re.compile(r'delete\s+(\w+);.*?\n.*?delete\s+\1\s*;', re.MULTILINE),
            'CRITICAL',
            'Pointer deleted twice (double-free)'
        ),
        'double_free_c': (
            re.compile(r'free\s*\(\s*(\w+)\s*\);.*?\n.*?free\s*\(\s*\1\s*\);', re.MULTILINE),
            'CRITICAL',
            'Pointer freed twice (double-free)'
        ),
        'delete_array_mismatch': (
            re.compile(r'(?:new\s+\w+\s*\[|delete\s*\[\s*\])', re.MULTILINE),
            'CRITICAL',
            'Potential delete[]/delete mismatch'
        ),
        'free_after_realloc': (
            re.compile(r'(\w+)\s*=\s*realloc\s*\(\s*\1.*?\n.*?free\s*\(\s*\1\s*\);', re.MULTILINE),
            'CRITICAL',
            'free() after realloc on same pointer (double-free if realloc fails)'
        ),
    }
    
    # Pattern M-1+M-2: Use-After-Scope
    USE_AFTER_SCOPE_PATTERNS = {
        'return_local_address': (
            re.compile(r'return\s+&\s*(\w+)\s*;|return\s+\w+\s*\(\s*&(\w+)\s*\)', re.MULTILINE),
            'CRITICAL',
            'Returning address of local variable (use-after-scope)'
        ),
        'store_local_pointer': (
            re.compile(r'(?:pGlobal|g_\w+|s_\w+)\s*=\s*&\s*\w+\s*;', re.IGNORECASE),
            'CRITICAL',
            'Global pointer assigned local variable address (use-after-scope)'
        ),
        'local_to_static': (
            re.compile(r'static.*\*?\s*\w+\s*=\s*&\s*\w+\s*;', re.MULTILINE),
            'CRITICAL',
            'Static pointer assigned local variable address (use-after-scope)'
        ),
        'capture_by_reference': (
            re.compile(r'\[\s*&\s*(?:this|ptr|obj|var|local)', re.MULTILINE),
            'HIGH',
            'Lambda capturing local by reference (potential use-after-scope)'
        ),
    }
    
    # Heuristic Patterns: Potential Memory Leaks Leading to UAF
    MEMORY_LEAK_HEURISTICS = {
        'exception_before_delete': (
            re.compile(r'(?:throw|exception|abort|exit)\s*\([^)]*\);.*?delete', re.MULTILINE),
            'HIGH',
            'Exception/abort before delete — memory leak and use-after-free risk'
        ),
        'early_return_no_delete': (
            re.compile(r'(?:new\s+\w+|malloc\s*\(\s*\w+\s*\)).*?if\s*\([^)]*\)\s*return', re.MULTILINE),
            'HIGH',
            'Allocated memory not freed on early return path'
        ),
    }
    
    def __init__(self):
        self.gaps: List[MemorySafetyGap] = []
    
    def scan_file(self, file_path: str) -> List[MemorySafetyGap]:
        """Scan a single file for memory safety issues"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return gaps
        
        # Scan for use-after-free (M-1)
        gaps.extend(self._scan_patterns(
            content, file_path, self.USE_AFTER_FREE_PATTERNS, 
            MemorySafetyGapType.USE_AFTER_FREE
        ))
        
        # Scan for double-free (M-2)
        gaps.extend(self._scan_patterns(
            content, file_path, self.DOUBLE_FREE_PATTERNS,
            MemorySafetyGapType.DOUBLE_FREE
        ))
        
        # Scan for use-after-scope
        gaps.extend(self._scan_patterns(
            content, file_path, self.USE_AFTER_SCOPE_PATTERNS,
            MemorySafetyGapType.USE_AFTER_SCOPE
        ))
        
        # Scan for memory leak heuristics
        gaps.extend(self._scan_patterns(
            content, file_path, self.MEMORY_LEAK_HEURISTICS,
            MemorySafetyGapType.MEMORY_LEAK_POTENTIAL
        ))
        
        return gaps
    
    def _scan_patterns(self, content: str, file_path: str,
                       patterns: Dict, gap_type: MemorySafetyGapType) -> List[MemorySafetyGap]:
        """Scan for a set of patterns and return gaps"""
        gaps = []
        lines = content.split('\n')
        
        for pattern_name, (pattern_re, severity, description) in patterns.items():
            matches = list(pattern_re.finditer(content))
            
            for match in matches:
                # Calculate line number
                line_num = content[:match.start()].count('\n') + 1
                
                # Get snippet (try to extract the matched line)
                line_start = content.rfind('\n', 0, match.start()) + 1
                line_end = content.find('\n', match.end())
                if line_end == -1:
                    line_end = len(content)
                
                snippet = content[line_start:line_end].strip()[:100]
                
                gap = MemorySafetyGap(
                    file_path=file_path,
                    line_num=line_num,
                    gap_type=gap_type,
                    snippet=snippet,
                    severity=severity,
                    description=description,
                    remediation='Use smart pointers (std::unique_ptr, std::shared_ptr) to manage memory automatically',
                    pattern_name=pattern_name
                )
                gaps.append(gap)
        
        return gaps


def main():
    """Main entry point for scanner"""
    import sys
    
    scanner = MemorySafetyScanner()
    all_gaps = []
    
    # Scan .cpp and .h files
    for ext in ['**/*.cpp', '**/*.h', '**/*.hpp']:
        for file_path in Path('.').glob(ext):
            if any(skip in str(file_path) for skip in ['test', 'build', '.git', 'external']):
                continue
            gaps = scanner.scan_file(str(file_path))
            all_gaps.extend(gaps)
    
    # Output results
    results = {
        'enhancement': 'M-1/M-2',
        'cwe': 'CWE-416/CWE-415',
        'title': 'Memory Safety Detection (Use-After-Free / Double-Free)',
        'total_gaps': len(all_gaps),
        'gaps': [gap.to_dict() for gap in all_gaps]
    }
    
    print(json.dumps(results, indent=2))
    return len(all_gaps)


if __name__ == '__main__':
    main()
