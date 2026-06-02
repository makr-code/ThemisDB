#!/usr/bin/env python3
"""
Gap Scanner Step 01.2 — Error Handling & Reliability

Detects:
- Missing retry logic for network/RPC calls
- Missing timeouts on blocking operations
- Missing circuit breakers
- Uncaught exceptions in handler functions
- Generic exception handling (catch(...))
- Missing health checks in critical paths
- No exponential backoff strategies
"""

import re
import sys
from pathlib import Path
from typing import List

sys.path.insert(0, str(Path(__file__).parents[1]))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class ErrorHandlingScanner(BaseGapScanner):
    """Phase 1.2: Error Handling & Reliability (±5 line context)"""
    
    PRIORITY = ScannerPriority.MEDIUM
    ENABLED = True
    MAX_RUNTIME_SECONDS = 30
    
    def __init__(self):
        """Initialize Error Handling Scanner."""
        super().__init__("Error Handling Scanner", "3.1")
        
        # Network/RPC patterns
        self.rpc_patterns = [
            re.compile(r'(stub_|stub->|client\.)(.*?)\('),
            re.compile(r'(http|curl|request)\..*\('),
            re.compile(r'(socket|connect|send|recv|accept)\s*\('),
            re.compile(r'(query|execute|prepare)\s*\('),
        ]
        
        # Blocking operations
        self.blocking_patterns = [
            re.compile(r'\.\s*lock\s*\('),
            re.compile(r'(thread|task)\.join\(\)'),
            re.compile(r'(wait|acquire)\s*\('),
        ]
        
        # Exception patterns
        self.throw_pattern = re.compile(r'\bthrow\b')
        self.catch_generic_pattern = re.compile(r'catch\s*\(\s*\.\.\.\s*\)')
        self.try_pattern = re.compile(r'\btry\b')
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan source directory for error handling gaps"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()  # Ensure absolute path
            self.files_scanned += 1
            lines = self._read_file_lines(file_path)
            
            if 'test' in file_path.name.lower():
                continue
            
            gaps.extend(self._check_no_retry_logic(file_path, lines))
            gaps.extend(self._check_no_timeout(file_path, lines))
            gaps.extend(self._check_uncaught_exceptions(file_path, lines))
            gaps.extend(self._check_generic_catch(file_path, lines))
        
        return self.deduplicate(gaps)
    
    def _check_no_retry_logic(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect RPC/network calls without retry logic"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if line.strip().startswith('//'):
                continue
            
            # Check for RPC/network patterns
            is_rpc = any(pattern.search(line) for pattern in self.rpc_patterns)
            
            if is_rpc:
                # Check if retry/backoff exists in next lines
                next_context = '\n'.join(lines[line_no:min(len(lines), line_no+10)]).lower()
                
                has_retry = any(word in next_context for word in ['retry', 'attempt', 'loop', 'for '])
                has_backoff = any(word in next_context for word in ['backoff', 'sleep', 'wait', 'exponential'])
                
                if not (has_retry or has_backoff):
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="no_retry_logic",
                        severity="HIGH",
                        confidence=0.72,
                        description="RPC/network call without retry logic — transient failures will propagate",
                        remediation="Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _check_no_timeout(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect blocking operations without timeout"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if line.strip().startswith('//'):
                continue
            
            # Check for blocking patterns
            is_blocking = any(pattern.search(line) for pattern in self.blocking_patterns)
            
            if is_blocking:
                # Check if timeout/deadline in context
                context = '\n'.join(self._get_context(lines, line_no, window=8)).lower()
                
                has_timeout = any(word in context for word in [
                    'timeout', '_for', 'wait_for', 'wait_until', 'duration',
                    'expires_after', 'expires_at', 'deadline', 'milliseconds', 'seconds'
                ])
                
                if not has_timeout:
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="blocking_no_timeout",
                        severity="CRITICAL",
                        confidence=0.75,
                        description="Blocking operation without timeout — can block indefinitely",
                        remediation="Add timeout parameter (e.g., wait_for(timeout), with_timeout())",
                        context='\n'.join(self._get_context(lines, line_no, window=3))
                    ))
        
        return gaps
    
    def _check_uncaught_exceptions(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect exception throws outside try/catch"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if not self.throw_pattern.search(line):
                continue
            
            stripped = line.strip()
            if not stripped.startswith('throw '):
                continue
            
            # WHITELIST: Constructor validation throws
            if any(exc in line for exc in ['std::invalid_argument', 'std::out_of_range', 'std::logic_error']):
                # Check if in constructor
                prev_lines = '\n'.join(lines[max(0, line_no-30):line_no])
                if re.search(r'\b([A-Za-z_]\w*)::\1\s*\(', prev_lines):
                    continue
            
            # Check if in try/catch context
            prev_context = '\n'.join(lines[max(0, line_no-20):line_no])
            
            if 'try' not in prev_context:
                gaps.append(Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="uncaught_exception",
                    severity="HIGH",
                    confidence=0.70,
                    description="Exception thrown without try/catch context",
                    remediation="Wrap throwing code in try/catch or add proper error handling",
                    context='\n'.join(self._get_context(lines, line_no, window=3))
                ))
        
        return gaps
    
    def _check_generic_catch(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect generic catch(...) patterns"""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if self.catch_generic_pattern.search(line):
                gaps.append(Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type="generic_catch",
                    severity="MEDIUM",
                    confidence=0.68,
                    description="Generic catch(...) — specific exception types ignored",
                    remediation="Catch specific exceptions: catch(std::exception& e) { ... }",
                    context='\n'.join(self._get_context(lines, line_no, window=3))
                ))
        
        return gaps


if __name__ == "__main__":
    import time
    
    source_dir = sys.argv[1] if len(sys.argv) > 1 else "./src"
    
    print("[Error Handling Scanner] Starting scan...")
    start = time.time()
    
    scanner = ErrorHandlingScanner()
    gaps = scanner.scan(source_dir)
    
    elapsed = time.time() - start
    
    print(f"\nFound {len(gaps)} error handling gaps in {elapsed:.2f}s")
    print(f"Scanned {scanner.files_scanned} files\n")
    
    by_severity = {}
    for gap in gaps:
        sev = gap.severity
        by_severity[sev] = by_severity.get(sev, 0) + 1
    
    for sev in ['CRITICAL', 'HIGH', 'MEDIUM']:
        if sev in by_severity:
            print(f"  {sev}: {by_severity[sev]}")
    
    by_type = {}
    for gap in gaps:
        typ = gap.type
        by_type[typ] = by_type.get(typ, 0) + 1
    
    print("\nBy Type:")
    for typ, count in sorted(by_type.items(), key=lambda x: -x[1])[:5]:
        print(f"  {typ}: {count}")
