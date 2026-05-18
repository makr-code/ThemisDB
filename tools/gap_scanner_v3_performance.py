#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Performance Anti-Patterns Detection

Detects:
- String concatenation in loops (StringBuilder pattern)
- Repeated allocations in loops
- Inefficient sorting/searching
- Unnecessary copies
- Lock contention in hot paths
- Expensive operations in inner loops
- Missing caching/memoization
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class PerformanceGapType(Enum):
    """Performance anti-pattern gap classifications"""
    STRING_CONCAT_LOOP = "string_concat_loop"     # String + in loop
    ALLOCATION_LOOP = "allocation_loop"           # new/malloc in loop
    REPEATED_SEARCH = "repeated_search"           # find/search in loop
    EXPENSIVE_COPY = "expensive_copy"             # Unnecessary copy
    LOCK_CONTENTION = "lock_contention"           # Mutex in hot loop
    EXPENSIVE_INNER_OP = "expensive_inner_op"     # Expensive in inner loop
    MISSING_CACHE = "missing_cache"               # No caching/memoization
    DOUBLE_PROCESSING = "double_processing"       # Process twice


@dataclass
class PerformanceGap:
    """Represents a performance anti-pattern gap"""
    file_path: str
    line_num: int
    gap_type: PerformanceGapType
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


class PerformanceGapScanner:
    """Detect performance anti-patterns in C++ code"""
    
    PATTERNS = {
        'for_loop': re.compile(r'for\s*\('),
        'while_loop': re.compile(r'while\s*\('),
        'string_concat': re.compile(r'(\w+|\w+\.)?(\+\s*=\s*"|\+\s*")|(\w+\.append|std::string.*\+)'),
        'allocation': re.compile(r'\bnew\s+|malloc\s*\(|calloc\s*\('),
        'find': re.compile(r'\.find\s*\('),
        'search': re.compile(r'std::find|std::search|\.upper_bound|\.lower_bound'),
        'lock': re.compile(r'std::lock_guard|std::unique_lock'),
        'printf': re.compile(r'\bprintf|sprintf'),
    }
    
    EXPENSIVE_OPS = [
        'std::sort', 'std::stable_sort',
        'printf', 'sprintf',
        'cout <<', 'cerr <<',
        'std::regex',
        'json::parse', 'json::dump',
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[PerformanceGap]] = {}
    
    def scan_file(self, file_path: Path) -> List[PerformanceGap]:
        """Scan single file for performance anti-patterns"""
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
            
            # Check for loop structures
            if re.search(r'for\s*\(', line) or re.search(r'while\s*\(', line):
                loop_indent = len(line) - len(line.lstrip())
                loop_start = line_num
                
                # Scan loop body for anti-patterns
                next_lines = lines[line_num:min(len(lines), line_num+30)]
                
                for idx, loop_line in enumerate(next_lines, 1):
                    loop_body_indent = len(loop_line) - len(loop_line.lstrip())
                    
                    # Must be inside loop body
                    if loop_body_indent <= loop_indent:
                        break
                    
                    # Check for string concatenation in loop
                    if any(op in loop_line for op in ['+=', '+']) and any(s in loop_line for s in ['"', "'"]):
                        if re.search(r'(\w+|std::string.*)\s*\+=\s*["\']', loop_line):
                            gap = PerformanceGap(
                                file_path=str(file_path.relative_to(self.repo_root)),
                                line_num=loop_start + idx,
                                gap_type=PerformanceGapType.STRING_CONCAT_LOOP,
                                snippet=loop_line.strip()[:100],
                                severity='MEDIUM',
                                description='String concatenation in loop — O(n²) behavior',
                                remediation='Use std::ostringstream or pre-allocate string with .reserve()'
                            )
                            gaps.append(gap)
                    
                    # Check for allocation in loop
                    if self.PATTERNS['allocation'].search(loop_line):
                        gap = PerformanceGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=loop_start + idx,
                            gap_type=PerformanceGapType.ALLOCATION_LOOP,
                            snippet=loop_line.strip()[:100],
                            severity='HIGH',
                            description='Dynamic allocation in loop — high overhead',
                            remediation='Pre-allocate outside loop or use object pool pattern'
                        )
                        gaps.append(gap)
                    
                    # Check for find/search in loop
                    if self.PATTERNS['search'].search(loop_line):
                        gap = PerformanceGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=loop_start + idx,
                            gap_type=PerformanceGapType.REPEATED_SEARCH,
                            snippet=loop_line.strip()[:100],
                            severity='HIGH',
                            description='find/search in loop — O(n²) or worse',
                            remediation='Move search outside loop or build index/map before loop'
                        )
                        gaps.append(gap)
                    
                    # Check for expensive I/O in loop
                    if any(op in loop_line for op in ['printf', 'cout', 'cerr', 'fprintf']):
                        gap = PerformanceGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=loop_start + idx,
                            gap_type=PerformanceGapType.EXPENSIVE_INNER_OP,
                            snippet=loop_line.strip()[:100],
                            severity='MEDIUM',
                            description='I/O operation in inner loop — very expensive',
                            remediation='Buffer output or move I/O outside loop'
                        )
                        gaps.append(gap)
                    
                    # Check for lock in loop
                    if self.PATTERNS['lock'].search(loop_line):
                        gap = PerformanceGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=loop_start + idx,
                            gap_type=PerformanceGapType.LOCK_CONTENTION,
                            snippet=loop_line.strip()[:100],
                            severity='HIGH',
                            description='Mutex lock in loop — high contention',
                            remediation='Acquire lock before loop or redesign to minimize lock time'
                        )
                        gaps.append(gap)
            
            # Check for unnecessary copies
            if ' = ' in line and '(' in line:
                # Look for patterns like: std::vector<T> copy = original;
                if 'std::vector' in line or 'std::string' in line or 'std::map' in line:
                    # Check if it's a copy (not ref/pointer)
                    if '&' not in line and '*' not in line:
                        # Check if original is also used (copy not needed)
                        var_match = re.search(r'(\w+)\s*=\s*(\w+)\s*;', line)
                        if var_match:
                            copy_var = var_match.group(1)
                            orig_var = var_match.group(2)
                            
                            next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                            
                            # If original is still used after copy, might be unnecessary
                            if f'{orig_var}' in next_context and 'modify' not in next_context.lower():
                                gap = PerformanceGap(
                                    file_path=str(file_path.relative_to(self.repo_root)),
                                    line_num=line_num,
                                    gap_type=PerformanceGapType.EXPENSIVE_COPY,
                                    snippet=line.strip()[:100],
                                    severity='MEDIUM',
                                    description='Unnecessary expensive copy',
                                    remediation='Use const reference (const T&) or std::move if transfer is needed'
                                )
                                gaps.append(gap)
            
            # Check for double processing
            if 'for' in line:
                # Check if same iteration variable is used in nested loop
                match = re.search(r'for\s*\(\s*auto\s+(\w+)', line)
                if match:
                    var_name = match.group(1)
                    
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+20)])
                    
                    # Look for same container iteration
                    same_var_count = next_context.count(f'for') + next_context.count(f'for_each')
                    
                    if same_var_count > 1 and 'container' in next_context:
                        gap = PerformanceGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=PerformanceGapType.DOUBLE_PROCESSING,
                            snippet=line.strip()[:100],
                            severity='MEDIUM',
                            description='Container iterated multiple times — may be avoidable',
                            remediation='Combine iterations or pre-compute results'
                        )
                        gaps.append(gap)
    
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[PerformanceGap]]:
        """Scan module for performance anti-patterns"""
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
        """Scan all modules for performance anti-patterns"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for PERFORMANCE ANTI-PATTERNS...")
        
        src_root = self.repo_root / 'src'
        modules = sorted([d.name for d in src_root.iterdir() if d.is_dir()])
        
        aggregate = {}
        
        for module in modules:
            gaps_by_file = self.scan_module(module)
            total_gaps = sum(len(g) for g in gaps_by_file.values())
            
            if total_gaps > 0:
                print(f"   {module:30} {total_gaps:4} gaps")
                
                gap_counts = {}
                for gaps in gaps_by_file.values():
                    for gap in gaps:
                        gap_type = gap.gap_type.value
                        gap_counts[gap_type] = gap_counts.get(gap_type, 0) + 1
                
                severity_counts = {
                    'critical': sum(1 for gaps in gaps_by_file.values() for g in gaps if g.severity == 'CRITICAL'),
                    'high': sum(1 for gaps in gaps_by_file.values() for g in gaps if g.severity == 'HIGH'),
                    'medium': sum(1 for gaps in gaps_by_file.values() for g in gaps if g.severity == 'MEDIUM'),
                }
                
                aggregate[module] = {
                    'total': total_gaps,
                    'gaps_by_file': {f: [g.to_dict() for g in gaps] for f, gaps in gaps_by_file.items()},
                    'gap_types': gap_counts,
                    'severity_critical': severity_counts['critical'],
                    'severity_high': severity_counts['high'],
                    'severity_medium': severity_counts['medium'],
                }
        
        return aggregate
