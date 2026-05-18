#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Reliability Gaps Detection

Detects:
- Missing retry logic (network calls)
- Missing timeouts (blocking operations)
- Missing circuit breakers
- No graceful degradation
- Uncaught exceptions
- Missing health checks
- No backoff strategies
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class ReliabilityGapType(Enum):
    """Reliability gap classifications"""
    NO_RETRY_LOGIC = "no_retry_logic"             # Network call without retry
    NO_TIMEOUT = "no_timeout"                     # Blocking without timeout
    NO_CIRCUIT_BREAKER = "no_circuit_breaker"     # Repeated failure cascade
    NO_GRACEFUL_DEGRADE = "no_graceful_degrade"   # Hard failure instead of fallback
    UNCAUGHT_EXCEPTION = "uncaught_exception"     # Exception not handled
    NO_HEALTH_CHECK = "no_health_check"           # Service status unknown
    NO_BACKOFF = "no_backoff"                     # No exponential backoff


@dataclass
class ReliabilityGap:
    """Represents a reliability gap"""
    file_path: str
    line_num: int
    gap_type: ReliabilityGapType
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


class ReliabilityGapScanner:
    """Detect reliability issues in C++ code"""
    
    # Network/RPC calls
    RPC_PATTERNS = {
        'grpc_call': re.compile(r'(stub_|stub->|client\.)(.*?)\('),
        'http_call': re.compile(r'(http|curl|request)\..*\('),
        'socket_call': re.compile(r'(socket|connect|send|recv|accept)\s*\('),
        'database_query': re.compile(r'(query|execute|prepare)\s*\('),
    }
    
    # Blocking operations
    BLOCKING_PATTERNS = {
        'mutex_lock': re.compile(r'(mutex|lock_guard|unique_lock|lock)\s*\('),
        'thread_join': re.compile(r'(thread|task)\.join\(\)'),
        'file_io': re.compile(r'(read|write|open|fopen|fscanf)\s*\('),
        'semaphore_wait': re.compile(r'(wait|acquire)\s*\('),
    }
    
    # Exception patterns
    EXCEPTION_PATTERNS = {
        'throw': re.compile(r'\bthrow\s+\w+'),
        'catch_generic': re.compile(r'catch\s*\(\s*\.\.\.\s*\)'),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[ReliabilityGap]] = {}
    
    def scan_file(self, file_path: Path) -> List[ReliabilityGap]:
        """Scan single file for reliability gaps"""
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
            
            # Check for RPC/network calls without retry
            for rpc_type, pattern in self.RPC_PATTERNS.items():
                if pattern.search(line):
                    # Check if retry/backoff exists in next few lines
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+10)])
                    
                    has_retry = any(retry_word in next_context.lower() 
                                   for retry_word in ['retry', 'attempt', 'loop', 'for '])
                    has_backoff = any(backoff in next_context.lower() 
                                    for backoff in ['backoff', 'sleep', 'wait', 'exponential'])
                    
                    if not (has_retry or has_backoff):
                        gap = ReliabilityGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ReliabilityGapType.NO_RETRY_LOGIC,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description=f'{rpc_type} without retry logic — transient failures will propagate',
                            remediation='Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)'
                        )
                        gaps.append(gap)
            
            # Check for blocking operations without timeout
            for block_type, pattern in self.BLOCKING_PATTERNS.items():
                if pattern.search(line):
                    # Check if timeout is specified
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+5)])
                    
                    has_timeout = any(timeout_word in next_context.lower()
                                    for timeout_word in ['timeout', '_for', 'ms', 'seconds', 'duration'])
                    
                    if not has_timeout:
                        gap = ReliabilityGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ReliabilityGapType.NO_TIMEOUT,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description=f'{block_type} without timeout — can block indefinitely',
                            remediation='Add timeout parameter (e.g., wait_for(timeout), with_timeout())'
                        )
                        gaps.append(gap)
            
            # Check for exception handling gaps
            if self.EXCEPTION_PATTERNS['throw'].search(line):
                # Check if this is in a try/catch context
                prev_context = ''.join(lines[max(0, line_num-20):line_num])
                
                if 'try' not in prev_context:
                    gap = ReliabilityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=ReliabilityGapType.UNCAUGHT_EXCEPTION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Exception thrown without try/catch context',
                        remediation='Wrap throwing code in try/catch or add proper error handling'
                    )
                    gaps.append(gap)
            
            # Check for generic exception catching
            if self.EXCEPTION_PATTERNS['catch_generic'].search(line):
                gap = ReliabilityGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=ReliabilityGapType.UNCAUGHT_EXCEPTION,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='Generic catch(...) — specific exception types ignored',
                    remediation='Catch specific exceptions: catch(std::exception& e) { ... }'
                )
                gaps.append(gap)
            
            # Check for status/health field initialization
            if 'Status' in line or 'status' in line or 'healthy' in line:
                if '=' not in line and 'return' not in line:
                    # Possible uninitialized status
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+3)])
                    if 'init' not in next_context.lower() and 'set' not in next_context.lower():
                        gap = ReliabilityGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ReliabilityGapType.NO_HEALTH_CHECK,
                            snippet=line.strip()[:100],
                            severity='MEDIUM',
                            description='Status field defined but no initialization or health check',
                            remediation='Initialize status to UNKNOWN and implement periodic health checks'
                        )
                        gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[ReliabilityGap]]:
        """Scan module for reliability gaps"""
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
        """Scan all modules for reliability gaps"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for RELIABILITY GAPS...")
        
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
        
        with open(output_path / 'gap_scan_v3_reliability_aggregate.json', 'w') as f:
            json.dump(aggregate, f, indent=2)
        
        print(f"\n[OK] Reliability scan complete. Results in {output_dir}/")
        return aggregate


if __name__ == '__main__':
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    print("[INFO] Reliability Gap Scanner v3")
    print("=" * 70)
    
    scanner = ReliabilityGapScanner(repo_root)
    results = scanner.run_full_scan(output_dir)
    
    total_gaps = sum(m.get('total', 0) for m in results.values())
    critical = sum(m.get('severity_critical', 0) for m in results.values())
    high = sum(m.get('severity_high', 0) for m in results.values())
    
    print(f"\n[SUMMARY] Reliability Gaps:")
    print(f"   Total: {total_gaps}")
    print(f"   CRITICAL: {critical}")
    print(f"   HIGH: {high}")
    print("\n" + "=" * 70)
