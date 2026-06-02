#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 ÔÇö STL Container Misuse Detection

Detects:
- O(n┬▓) patterns in loops (nested iterations, repeated lookups)
- Inefficient container operations (find in vector, push_back in loop)
- Iterator invalidation issues
- Incorrect container choice for use case
- Copy-on-modify patterns
- Uninitialized container accesses
- Range iteration on temporary
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class ContainerGapType(Enum):
    """Container/STL misuse gap classifications"""
    O_N_SQUARED = "o_n_squared"                   # O(n┬▓) pattern
    INEFFICIENT_FIND = "inefficient_find"         # find() in vector
    ITERATOR_INVALIDATION = "iterator_invalidation"  # Iterator after modify
    WRONG_CONTAINER = "wrong_container"           # Wrong container type
    REPEATED_LOOKUP = "repeated_lookup"           # Same lookup in loop
    COPY_OVERHEAD = "copy_overhead"               # Unnecessary copy
    UNINITIALIZED_ACCESS = "uninitialized_access" # Access before init
    RANGE_TEMPORARY = "range_temporary"           # Range for on temp


@dataclass
class ContainerGap:
    """Represents a container/STL misuse gap"""
    file_path: str
    line_num: int
    gap_type: ContainerGapType
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


class ContainerGapScanner:
    """Detect STL container misuse in C++ code"""
    
    CONTAINER_PATTERNS = {
        'vector': re.compile(r'std::vector|vector<'),
        'map': re.compile(r'std::map|map<'),
        'set': re.compile(r'std::set|set<'),
        'unordered_map': re.compile(r'std::unordered_map|unordered_map<'),
        'list': re.compile(r'std::list|list<'),
    }
    
    OPERATIONS = {
        'find': re.compile(r'\.find\s*\('),
        'push_back': re.compile(r'\.push_back\s*\('),
        'insert': re.compile(r'\.insert\s*\('),
        'erase': re.compile(r'\.erase\s*\('),
        'at': re.compile(r'\.at\s*\('),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[ContainerGap]] = {}
    
    def scan_file(self, file_path: Path) -> List[ContainerGap]:
        """Scan single file for container misuse"""
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
            
            # Check for O(n┬▓) patterns: find() in vector within loop
            if re.search(r'for\s*\(', line):
                loop_indent = len(line) - len(line.lstrip())
                next_lines = lines[line_num:min(len(lines), line_num+20)]
                
                # Check for nested loops with find
                for idx, next_line in enumerate(next_lines):
                    next_indent = len(next_line) - len(next_line.lstrip())
                    
                    if next_indent > loop_indent and '.find(' in next_line:
                        # Check if find is on vector
                        context = ''.join(lines[max(0, line_num-5):line_num+idx+1])
                        
                        if 'std::vector' in context or 'vector<' in context:
                            gap = ContainerGap(
                                file_path=str(file_path.relative_to(self.repo_root)),
                                line_num=line_num + idx,
                                gap_type=ContainerGapType.O_N_SQUARED,
                                snippet=next_line.strip()[:100],
                                severity='HIGH',
                                description='O(n┬▓) pattern: find() on vector inside loop',
                                remediation='Use std::unordered_map or std::set for O(log n) or O(1) lookup'
                            )
                            gaps.append(gap)
            
            # Check for repeated lookups in same loop
            if '.find(' in line:
                # Extract what's being found
                match = re.search(r'\.find\s*\(\s*(\w+)', line)
                if match:
                    search_var = match.group(1)
                    
                    # Check next 10 lines for repeated find with same variable
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+10)])
                    repeated = next_context.count(f'.find({search_var}')
                    
                    if repeated > 1:
                        gap = ContainerGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ContainerGapType.REPEATED_LOOKUP,
                            snippet=line.strip()[:100],
                            severity='MEDIUM',
                            description=f'Repeated find() for same key: {search_var}',
                            remediation='Cache the result or use lower_bound/upper_bound for range operations'
                        )
                        gaps.append(gap)
            
            # Check for inefficient vector.find() (should use set/map)
            if '.find(' in line:
                context = ''.join(lines[max(0, line_num-5):line_num+1])
                
                if 'std::vector' in context or 'vector<' in context:
                    # Check if it's a containment check
                    if '!= end()' in line or '== end()' in line:
                        gap = ContainerGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ContainerGapType.INEFFICIENT_FIND,
                            snippet=line.strip()[:100],
                            severity='MEDIUM',
                            description='find() on std::vector is O(n) ÔÇö consider std::set or std::unordered_set',
                            remediation='Replace vector with set/unordered_set for frequent lookups'
                        )
                        gaps.append(gap)
            
            # Check for push_back in loop (may cause reallocation)
            if '.push_back(' in line:
                # Check if in a loop
                prev_context = ''.join(lines[max(0, line_num-20):line_num])
                
                if 'for' in prev_context or 'while' in prev_context:
                    gap = ContainerGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ContainerGapType.COPY_OVERHEAD,
                        snippet=line.strip()[:100],
                        severity='MEDIUM',
                        description='push_back in loop ÔÇö consider pre-allocating with reserve()',
                        remediation='Call vector.reserve(expected_size) before loop to avoid reallocations'
                    )
                    gaps.append(gap)
            
            # Check for iterator use after modify
            if re.search(r'auto\s+\w+\s*=\s*\w+\.find', line) or re.search(r'auto\s+\w+\s*=\s*\w+\.begin', line):
                # Extract iterator variable
                match = re.search(r'auto\s+(\w+)', line)
                if match:
                    iter_var = match.group(1)
                    
                    # Check if container is modified after iterator creation
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+10)])
                    
                    if any(op in next_context for op in ['.erase(', '.insert(', '.push_back(', '.pop_back(', '.clear()']):
                        gap = ContainerGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ContainerGapType.ITERATOR_INVALIDATION,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description=f'Iterator {iter_var} may be invalidated by container modification',
                            remediation='Re-create iterator after modification or use erase() return value'
                        )
                        gaps.append(gap)
            
            # Check for range-for on temporary/rvalue
            if 'for' in line and '(' in line and ')' in line:
                # Pattern: for (auto x : func()) or for (auto x : std::vector<>())
                if re.search(r'for\s*\([^)]*:\s*\w+\s*\(', line):
                    gap = ContainerGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ContainerGapType.RANGE_TEMPORARY,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Range-for on temporary container ÔÇö references may be invalid',
                        remediation='Store container in variable first: auto c = func(); for (auto x : c) { ... }'
                    )
                    gaps.append(gap)
            
            # Check for uninitialized container access
            if '[' in line and ']' in line and '=' not in line:
                # Pattern: arr[i] or container[key] without prior init/insert
                match = re.search(r'(\w+)\s*\[', line)
                if match:
                    container = match.group(1)
                    
                    # Check previous 5 lines for initialization
                    prev_context = ''.join(lines[max(0, line_num-5):line_num])
                    
                    if 'insert' not in prev_context and 'emplace' not in prev_context and f'reserve' not in prev_context:
                        if f'{container}[' not in prev_context:  # First access
                            gap = ContainerGap(
                                file_path=str(file_path.relative_to(self.repo_root)),
                                line_num=line_num,
                                gap_type=ContainerGapType.UNINITIALIZED_ACCESS,
                                snippet=line.strip()[:100],
                                severity='HIGH',
                                description=f'Container element access before initialization',
                                remediation='Use .at() for bounds checking or initialize element first'
                            )
                            gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[ContainerGap]]:
        """Scan module for container misuse"""
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
        """Scan all modules for container misuse"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for CONTAINER MISUSE GAPS...")
        
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
