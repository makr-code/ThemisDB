#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — RAII & Resource Leaks Detection

Detects:
- File handles not closed (FILE*, std::ifstream)
- Database connections not released
- Socket descriptors left open
- Smart pointer misuse
- Exception-unsafe resource allocation
- Missing try-catch around resource allocation
- Resource acquired in if condition (scoping issue)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class RAIIGapType(Enum):
    """RAII & resource management gap classifications"""
    FILE_HANDLE_LEAK = "file_handle_leak"         # FILE* not closed
    SOCKET_LEAK = "socket_leak"                   # Socket not closed
    DB_CONNECTION_LEAK = "db_connection_leak"     # Connection not released
    SMART_PTR_MISUSE = "smart_ptr_misuse"         # Incorrect smart pointer use
    EXCEPTION_UNSAFE = "exception_unsafe"         # Resource not exception-safe
    MISSING_DTOR = "missing_dtor"                 # No destructor defined
    MANUAL_CLEANUP = "manual_cleanup"             # Manual cleanup instead of RAII
    RESOURCE_SCOPE = "resource_scope"             # Resource scope issue


@dataclass
class RAIIGap:
    """Represents a RAII/resource management gap"""
    file_path: str
    line_num: int
    gap_type: RAIIGapType
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


class RAIIGapScanner:
    """Detect RAII violations and resource leaks in C++ code"""
    
    PATTERNS = {
        'fopen': re.compile(r'\bfopen\s*\('),
        'fclose': re.compile(r'\bfclose\s*\('),
        'ifstream': re.compile(r'std::ifstream|ifstream'),
        'ofstream': re.compile(r'std::ofstream|ofstream'),
        'socket': re.compile(r'\bsocket\s*\(|SOCKET|AF_INET|AF_UNIX'),
        'close_fd': re.compile(r'close\s*\('),
        'getConnection': re.compile(r'getConnection|acquire|allocate'),
        'releaseConnection': re.compile(r'releaseConnection|release|free'),
        'make_unique': re.compile(r'std::make_unique'),
        'make_shared': re.compile(r'std::make_shared'),
        'new_raw': re.compile(r'\bnew\s+\w+'),
    }
    
    RESOURCE_KEYWORDS = ['file', 'stream', 'socket', 'fd', 'handle', 'connection', 'pool']
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[RAIIGap]] = {}
    
    def _is_destructor_context(self, line_num: int, lines: List[str]) -> bool:
        """Check if cleanup is inside destructor (legitimate RAII pattern)."""
        # Look backwards for ~ClassName
        for i in range(line_num - 1, max(0, line_num - 30), -1):
            if re.search(r'~\w+\s*\(', lines[i]):
                return True  # Inside destructor
            # Stop if we hit class/function boundary
            if re.search(r'^\s*(class|struct|void|int|bool)\s+\w+', lines[i]):
                break
        return False
    
    def _is_error_path(self, line_num: int, lines: List[str]) -> bool:
        """Check if cleanup is in error handling path (legitimate)."""
        # Look at context: is this in try/catch or error branch?
        next_context = ''.join(lines[line_num:min(len(lines), line_num+3)])
        prev_context = ''.join(lines[max(0, line_num-10):line_num])
        
        error_patterns = [
            r'if\s*\(!|if\s*\(.*==\s*nullptr|if\s*\(.*==\s*false',  # error check
            r'catch\s*\(',
            r'goto\s+error',
            r'return\s+(false|nullptr|ERROR)',
        ]
        return any(re.search(p, prev_context + next_context) for p in error_patterns)

    def _is_raii_wrapper_cleanup(self, line: str) -> bool:
        """Detect cleanup calls that are likely inside intentional RAII wrappers."""
        l = line.lower()
        wrapper_tokens = [
            'scoped_', 'scope_guard', 'guard', 'deleter', 'unique_ptr',
            'shared_ptr', 'raii', 'cleanup_guard'
        ]
        if any(token in l for token in wrapper_tokens):
            return True

        # Close/release on explicit wrapper/handle objects are typically legitimate.
        if re.search(r'\b(this->|m_|self\.)\w*(handle|socket|fd|conn)\b', l):
            return True

        return False
    
    def _should_skip_db_connection_leak(self, line: str, context: str, line_num: int, lines: List[str]) -> bool:
        """WHITELIST: Recognize RAII + ConnectionPool patterns.
        
        DB connections in RAII wrappers, ConnectionPool, or with explicit cleanup patterns
        should be whitelisted.
        Returns True if this should NOT be flagged.
        """
        l_lower = line.lower()
        
        # WHITELIST 1: Smart pointers wrapping connections
        if 'unique_ptr' in l_lower or 'shared_ptr' in l_lower:
            return True
        
        # WHITELIST 2: ConnectionPool patterns (expected to manage lifecycle)
        if 'connectionpool' in l_lower or 'pool_' in l_lower or 'connection_pool' in l_lower:
            return True
        
        # WHITELIST 3: Explicit close() in destructor context
        if self._is_destructor_context(line_num, lines):
            return True
        
        # WHITELIST 4: Using statement (RAII scope guard)
        if 'using' in l_lower or 'scoped_' in l_lower or 'guard' in l_lower:
            return True
        
        # WHITELIST 5: RAII wrapper class definition (it will have destructor)
        if 'class ' in line or 'struct ' in line:
            # This is class definition, destructor will handle cleanup
            return True
        
        # WHITELIST 6: In-function variable initialization with RAII
        if 'auto ' in l_lower or 'unique_ptr' in context or 'shared_ptr' in context:
            return True
        
        return False
    
    def _check_db_connection_leak(self, file_path: Path, lines: List[str]) -> List[RAIIGap]:
        """Detect database connections not released (RAII pattern).
        
        Looks for getConnection/acquire without matching releaseConnection/close.
        Returns list of gaps found.
        """
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            # Skip comments and test code
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*'):
                continue
            if 'TEST' in line or 'MOCK' in line or 'test' in file_path.name:
                continue
            
            # Look for connection acquisition
            if self.PATTERNS['getConnection'].search(line):
                # Extract variable name
                match = re.search(r'(\w+)\s*=.*(?:getConnection|acquire)', line)
                if not match:
                    continue
                
                var_name = match.group(1)
                
                # Expand context to ±20 lines for better pattern matching
                context_start = max(0, line_num - 20)
                context_end = min(len(lines), line_num + 20)
                full_context = ''.join(lines[context_start:context_end])
                
                # Check if this should be whitelisted
                if self._should_skip_db_connection_leak(line, full_context, line_num, lines):
                    continue
                
                # Check if connection is released
                has_release = any(pattern in full_context for pattern in [
                    f'release({var_name})',
                    f'close({var_name})',
                    f'{var_name}->close()',
                    f'{var_name}->release()',
                    f'releaseConnection',
                ])
                
                if not has_release:
                    gap = RAIIGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=RAIIGapType.DB_CONNECTION_LEAK,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description=f'Database connection from {var_name} never released — potential leak',
                        remediation='Use ConnectionPool, RAII wrapper, or explicit close() in destructor/finally'
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan_file(self, file_path: Path) -> List[RAIIGap]:
        """Scan single file for RAII violations"""
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
            
            # Check for fopen without fclose
            if self.PATTERNS['fopen'].search(line):
                # Extract variable name if assigned
                match = re.search(r'(\w+)\s*=\s*fopen', line)
                if match:
                    var_name = match.group(1)
                    
                    # Check if fclose is called later
                    remaining_lines = ''.join(lines[line_num:min(len(lines), line_num+50)])
                    if f'fclose({var_name})' not in remaining_lines and f'fclose(' not in remaining_lines:
                        gap = RAIIGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=RAIIGapType.FILE_HANDLE_LEAK,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description='FILE* returned by fopen without matching fclose',
                            remediation='Use std::ifstream/std::ofstream (RAII) instead of fopen/fclose'
                        )
                        gaps.append(gap)
            
            # Check for raw socket() without close()
            if re.search(r'socket\s*\(\s*AF_', line):
                match = re.search(r'(\w+)\s*=\s*socket', line)
                if match:
                    var_name = match.group(1)
                    
                    remaining_lines = ''.join(lines[line_num:min(len(lines), line_num+30)])
                    if 'close(' not in remaining_lines and 'closesocket(' not in remaining_lines:
                        gap = RAIIGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=RAIIGapType.SOCKET_LEAK,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description='Socket created but never closed — potential resource leak',
                            remediation='Wrap socket in RAII class (e.g., std::unique_ptr with custom deleter)'
                        )
                        gaps.append(gap)
            
            # Check for connection allocation without release
            if self.PATTERNS['getConnection'].search(line):
                # Look for corresponding release/close
                next_context = ''.join(lines[line_num:min(len(lines), line_num+20)])
                
                if not self.PATTERNS['releaseConnection'].search(next_context):
                    gap = RAIIGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=RAIIGapType.DB_CONNECTION_LEAK,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Resource acquired but not released — potential leak',
                        remediation='Ensure all acquire() calls are matched with release() in all code paths'
                    )
                    gaps.append(gap)
            
            # Check for raw new without std::unique_ptr
            if self.PATTERNS['new_raw'].search(line) and '(' in line:
                # Check if it's assigning to a smart ptr
                if 'unique_ptr' not in line and 'shared_ptr' not in line:
                    # Check next few lines for assignment to smart ptr
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                    
                    if 'unique_ptr' not in next_context and 'shared_ptr' not in next_context:
                        gap = RAIIGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=RAIIGapType.SMART_PTR_MISUSE,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description='Raw new without immediate wrapping in smart pointer',
                            remediation='Use auto ptr = std::make_unique<T>(...);'
                        )
                        gaps.append(gap)
            
            # Check for resource allocation in if condition
            for keyword in self.RESOURCE_KEYWORDS:
                if f'if' in line and keyword in line and '=' in line:
                    # Pattern: if (resource = acquire()) — dangerous scope
                    if re.search(rf'if\s*\(\s*\w+\s*=\s*{keyword}', line):
                        gap = RAIIGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=RAIIGapType.RESOURCE_SCOPE,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description='Resource allocated in if-condition — scope issue',
                            remediation='Allocate resource before if, then check: res = acquire(); if (res) { ... }'
                        )
                        gaps.append(gap)
            
            # Check for missing destructor in resource-managing class
            if 'class ' in line or 'struct ' in line:
                class_match = re.search(r'(class|struct)\s+(\w+)', line)
                if class_match:
                    class_name = class_match.group(2)
                    
                    # Check if class has new/malloc/socket but no destructor
                    scope_lines = lines[line_num:min(len(lines), line_num+50)]
                    scope_text = ''.join(scope_lines)
                    
                    has_new = 'new ' in scope_text
                    has_socket = 'socket(' in scope_text
                    has_file = 'fopen(' in scope_text
                    has_dtor = f'~{class_name}' in scope_text
                    
                    if (has_new or has_socket or has_file) and not has_dtor:
                        gap = RAIIGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=RAIIGapType.MISSING_DTOR,
                            snippet=f'class/struct {class_name}',
                            severity='CRITICAL',
                            description=f'Class {class_name} allocates resources but has no destructor',
                            remediation=f'Add explicit destructor: ~{class_name}() {{ /* cleanup */ }}'
                        )
                        gaps.append(gap)
            
            # Check for manual cleanup instead of RAII
            if any(cleanup in line for cleanup in ['delete ', 'free(', 'close(', 'release()']):
                # WAVE 3 FP TUNING: Skip legitimate cleanup contexts
                if self._is_destructor_context(line_num, lines):
                    continue  # Cleanup in destructor is RAII, not a gap
                
                if self._is_error_path(line_num, lines):
                    continue  # Error-path cleanup is legitimate
                
                if self._is_raii_wrapper_cleanup(line):
                    continue  # Skip RAII wrapper pattern
                
                # Only flag cleanup in "normal" success paths without safeguards
                gap = RAIIGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=RAIIGapType.MANUAL_CLEANUP,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='Manual cleanup outside exception handler — not exception-safe',
                    remediation='Use RAII or smart pointers for automatic cleanup in all exception paths'
                )
                gaps.append(gap)
    
        # Check for database connection leaks (separate scan with wider context)
        db_gaps = self._check_db_connection_leak(file_path, lines)
        gaps.extend(db_gaps)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[RAIIGap]]:
        """Scan module for RAII violations"""
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
        """Scan all modules for RAII violations"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for RAII & RESOURCE MANAGEMENT GAPS...")
        
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
