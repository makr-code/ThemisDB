#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Memory Safety Gaps Detection

Detects:
- Raw new/delete without RAII (memory leaks)
- Pointer arithmetic without bounds checking
- Unchecked malloc/realloc
- Array out-of-bounds (static analysis)
- Delete without nullptr assignment
- Shared pointer cycles
- Use-after-free risks
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class MemoryGapType(Enum):
    """Memory safety gap classifications"""
    NEW_WITHOUT_DELETE = "new_without_delete"     # Memory leak
    POINTER_ARITHMETIC = "pointer_arithmetic"     # Unbounded ptr access
    UNCHECKED_MALLOC = "unchecked_malloc"         # malloc without null check
    ARRAY_BOUNDS = "array_bounds"                 # Static overflow
    DELETE_NO_NULLPTR = "delete_no_nullptr"       # Use-after-free risk
    SHARED_PTR_CYCLE = "shared_ptr_cycle"         # Ref cycle leak
    MANUAL_MEMORY = "manual_memory"               # Raw new/delete usage
    POINTER_OWNERSHIP = "pointer_ownership"       # Unclear ownership


@dataclass
class MemoryGap:
    """Represents a memory safety gap"""
    file_path: str
    line_num: int
    gap_type: MemoryGapType
    snippet: str
    severity: str  # CRITICAL, HIGH, MEDIUM
    description: str
    remediation: str
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
        }


class MemoryGapScanner:
    """Detect memory safety issues in C++ code"""
    
    # Pattern definitions
    NEW_PATTERNS = {
        'new_keyword': re.compile(r'\bnew\s+\w+\s*\('),
        'delete_keyword': re.compile(r'\bdelete\s+\w+'),
    }
    
    MALLOC_PATTERNS = {
        'malloc': re.compile(r'\bmalloc\s*\('),
        'calloc': re.compile(r'\bcalloc\s*\('),
        'realloc': re.compile(r'\brealloc\s*\('),
    }
    
    SMART_PTR_PATTERNS = {
        'unique_ptr': re.compile(r'std::unique_ptr\s*<'),
        'shared_ptr': re.compile(r'std::shared_ptr\s*<'),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[MemoryGap]] = {}

    def _has_bounds_guard_context(self, lines: List[str], line_idx: int) -> bool:
        """Detect common bounds guards, including function-entry size checks."""
        start = max(0, line_idx - 120)
        context = ''.join(lines[start:line_idx])
        guard_patterns = [
            r'if\s*\([^)]*(data|buffer|array)\.size\s*\(\)\s*[<>]=?\s*[^)]*\)',
            r'if\s*\([^)]*\bsize\s*\(\)\s*[<>]=?\s*[^)]*\)',
            r'assert\s*\([^)]*(size\s*\(\)|length\s*\(\))[^)]*\)',
            r'CHECK\s*\([^)]*(size\s*\(\)|length\s*\(\))[^)]*\)',
            r'if\s*\([^)]*\bindex\b\s*<\s*[^)]*(size\s*\(\)|length\s*\(\))[^)]*\)',
        ]
        return any(re.search(p, context) for p in guard_patterns)
    
    def scan_file(self, file_path: Path) -> List[MemoryGap]:
        """Scan single file for memory safety gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            # Skip comments and test code
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue
            if 'TEST' in line or 'MOCK' in line or 'test' in file_path.name:
                continue
            
            # Check for raw 'new' without RAII
            if self.NEW_PATTERNS['new_keyword'].search(line):
                # Check if it's using smart_ptr (safe)
                prev_line = lines[max(0, line_num-2):line_num]
                prev_text = ''.join(prev_line)
                
                if not any(ptr in prev_text for ptr in ['unique_ptr', 'shared_ptr', 'make_unique', 'make_shared']):
                    gap = MemoryGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MemoryGapType.NEW_WITHOUT_DELETE,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Raw new without RAII wrapper — potential memory leak',
                        remediation='Use std::unique_ptr<T> or std::make_unique<T>()'
                    )
                    gaps.append(gap)
            
            # Check for pointer arithmetic
            if '->' in line or '[' in line and ']' in line:
                # Look for array/pointer access without bounds check
                if any(var in line for var in ['ptr', 'buffer', 'data', 'array']):
                    # Check if bounds check exists in previous lines
                    prev_context = ''.join(lines[max(0, line_num-12):line_num])
                    has_local_guard = any(check in prev_context for check in ['if', 'assert', 'CHECK', 'DCHECK', 'size()', 'length()'])
                    has_wide_guard = self._has_bounds_guard_context(lines, line_num - 1)
                    if not (has_local_guard or has_wide_guard):
                        gap = MemoryGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MemoryGapType.POINTER_ARITHMETIC,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description='Pointer/array access without bounds validation',
                            remediation='Add bounds check before dereferencing'
                        )
                        gaps.append(gap)
            
            # Check for unchecked malloc/calloc/realloc
            for malloc_type, pattern in [('malloc', self.MALLOC_PATTERNS['malloc']),
                                         ('calloc', self.MALLOC_PATTERNS['calloc']),
                                         ('realloc', self.MALLOC_PATTERNS['realloc'])]:
                if pattern.search(line):
                    # Check if result is checked in next few lines
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+3)])
                    if 'if' not in next_context or 'nullptr' not in next_context:
                        gap = MemoryGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MemoryGapType.UNCHECKED_MALLOC,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description=f'Unchecked {malloc_type} — no null check before use',
                            remediation=f'Check: if (ptr != nullptr) before dereferencing'
                        )
                        gaps.append(gap)
            
            # Check for delete without nullptr
            if self.NEW_PATTERNS['delete_keyword'].search(line):
                # Extract variable name
                match = re.search(r'delete\s+(\w+)', line)
                if match:
                    var_name = match.group(1)
                    # Check if variable is nullified after
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+2)])
                    if f'{var_name} = nullptr' not in next_context and f'{var_name} =nullptr' not in next_context:
                        gap = MemoryGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MemoryGapType.DELETE_NO_NULLPTR,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description='Delete without nullifying pointer — use-after-free risk',
                            remediation=f'After delete: {var_name} = nullptr;'
                        )
                        gaps.append(gap)
            
            # Check for shared_ptr reference cycles
            if self.SMART_PTR_PATTERNS['shared_ptr'].search(line):
                # Look for bidirectional shared_ptr in same struct
                if any(member in line for member in ['next:', 'prev:', 'parent:', 'child:']):
                    # Check if both directions use shared_ptr
                    prev_context = ''.join(lines[max(0, line_num-10):line_num])
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+10)])
                    combined = prev_context + next_context
                    
                    shared_ptr_count = combined.count('std::shared_ptr')
                    if shared_ptr_count >= 2:
                        gap = MemoryGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MemoryGapType.SHARED_PTR_CYCLE,
                            snippet=line.strip()[:100],
                            severity='MEDIUM',
                            description='Bidirectional shared_ptr — potential reference cycle leak',
                            remediation='Use weak_ptr for back-references to break cycle'
                        )
                        gaps.append(gap)
            
            # Check for static array bounds
            if '[' in line and ']' in line:
                # Try to extract array size
                array_match = re.search(r'(\w+)\s*\[\s*(\d+)\s*\]', line)
                if array_match:
                    array_name = array_match.group(1)
                    array_size = int(array_match.group(2))
                    
                    # Look for loop accessing this array
                    next_lines = ''.join(lines[line_num:min(len(lines), line_num+20)])
                    loop_match = re.search(rf'for\s*\([^)]*i\s*<\s*(\d+)', next_lines)
                    if loop_match:
                        loop_size = int(loop_match.group(1))
                        if loop_size > array_size:
                            gap = MemoryGap(
                                file_path=str(file_path.relative_to(self.repo_root)),
                                line_num=line_num,
                                gap_type=MemoryGapType.ARRAY_BOUNDS,
                                snippet=line.strip()[:100],
                                severity='CRITICAL',
                                description=f'Array bounds violation: loop {loop_size} > array {array_size}',
                                remediation='Fix loop condition or increase array size'
                            )
                            gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[MemoryGap]]:
        """Scan module for memory gaps"""
        gaps_by_file = {}
        
        src_dir = self.repo_root / 'src' / module
        include_dir = self.repo_root / 'include' / module
        
        for directory in [src_dir, include_dir]:
            if not directory.exists():
                continue
            
            cpp_files = list(directory.rglob('*.cpp'))
            hpp_files = list(directory.rglob('*.hpp'))
            for file_path in cpp_files + hpp_files:
                gaps = self.scan_file(file_path)
                if gaps:
                    gaps_by_file[str(file_path.relative_to(self.repo_root))] = gaps
        
        return gaps_by_file
    
    def run_full_scan(self, output_dir: str = 'ai_working') -> Dict[str, any]:
        """Scan all modules for memory gaps"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for MEMORY SAFETY GAPS...")
        
        src_root = self.repo_root / 'src'
        modules = sorted([d.name for d in src_root.iterdir() if d.is_dir()])
        
        aggregate = {}
        
        for module in modules:
            gaps_by_file = self.scan_module(module)
            total_gaps = sum(len(g) for g in gaps_by_file.values())
            
            if total_gaps > 0:
                print(f"   {module:30} {total_gaps:4} gaps")
                
                gap_counts = {}
                severity_counts = {}
                
                for gaps in gaps_by_file.values():
                    for gap in gaps:
                        gap_type = gap.gap_type.value
                        gap_counts[gap_type] = gap_counts.get(gap_type, 0) + 1
                        sev = gap.severity
                        severity_counts[sev] = severity_counts.get(sev, 0) + 1
                
                aggregate[module] = {
                    'total': total_gaps,
                    'severity_critical': severity_counts.get('CRITICAL', 0),
                    'severity_high': severity_counts.get('HIGH', 0),
                    'severity_medium': severity_counts.get('MEDIUM', 0),
                    'by_type': gap_counts,
                    'gaps_by_file': {
                        f: [g.to_dict() for g in gaps]
                        for f, gaps in gaps_by_file.items()
                    }
                }
        
        with open(output_path / 'gap_scan_v3_memory_aggregate.json', 'w') as f:
            json.dump(aggregate, f, indent=2)
        
        print(f"\n[OK] Memory scan complete. Results in {output_dir}/")
        return aggregate


if __name__ == '__main__':
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    print("[INFO] Memory Safety Gap Scanner v3")
    print("=" * 70)
    
    scanner = MemoryGapScanner(repo_root)
    results = scanner.run_full_scan(output_dir)
    
    total_gaps = sum(m.get('total', 0) for m in results.values())
    critical = sum(m.get('severity_critical', 0) for m in results.values())
    high = sum(m.get('severity_high', 0) for m in results.values())
    
    print(f"\n[SUMMARY] Memory Safety Gaps:")
    print(f"   Total: {total_gaps}")
    print(f"   CRITICAL: {critical}")
    print(f"   HIGH: {high}")
    print("\n" + "=" * 70)
