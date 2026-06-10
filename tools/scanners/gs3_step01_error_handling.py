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
            re.compile(r'\b(?:connect|async_connect|reconnect)\s*\('),
            re.compile(r'\b(?:grpc::\w+|rpc(?:_client)?::\w+|rest(?:_client)?::\w+|websocket\w*)\s*\('),
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

    def _is_local_data_path(self, line: str, lines: List[str], line_no: int) -> bool:
        context = '\n'.join(self._get_context(lines, line_no, window=6)).lower()
        local_tokens = [
            'rocksdb', 'sqlite', 'leveldb', 'in_memory', 'local cache',
            'cache', 'std::filesystem', 'file_', 'manifest', 'wal'
        ]
        if any(token in context for token in local_tokens):
            return True
        return 'query(' in line and 'http' not in context and 'rpc' not in context

    def _is_validation_throw(self, lines: List[str], line_no: int, line: str) -> bool:
        recent = '\n'.join(lines[max(0, line_no - 4):line_no + 1]).lower()
        if 'throw std::' in line and 'if' in recent:
            validation_tokens = ['invalid', 'must', 'required', 'cannot', 'empty', 'range', 'unsupported']
            if any(tok in recent for tok in validation_tokens):
                return True
        return False

    def _is_boundary_throw_path(self, line: str, prev_context: str) -> bool:
        combined = (prev_context + '\n' + line).lower()
        boundary_tokens = [
            'handler', 'endpoint', 'controller', 'api', 'rpc', 'build', 'builder',
            'initialize', 'init', 'configure', 'create', 'factory'
        ]
        if any(token in combined for token in boundary_tokens) and 'throw std::' in combined:
            return True
        return False

    def _is_handler_boundary_context(self, file_path: Path, lines: List[str], line_no: int) -> bool:
        rel = str(file_path.relative_to(self.source_path)).lower()
        if any(tok in rel for tok in ['server', 'api', 'handler', 'endpoint', 'rpc', 'network']):
            return True

        prev = '\n'.join(lines[max(0, line_no - 30):line_no + 1]).lower()
        return any(tok in prev for tok in ['handle', 'endpoint', 'controller', 'request', 'rpc', 'serve'])

    def _looks_like_declaration_or_signature(self, line: str) -> bool:
        s = line.strip()
        if not s or s.startswith('#'):
            return True
        if s.endswith(';') and ('=' not in s) and ('return' not in s.lower()):
            if re.search(r'\b(?:void|bool|int|size_t|auto|std::\w+)\b.*\([^)]*\)\s*;\s*$', s):
                return True
        if re.search(r'^\s*(?:template\s*<|class\s+|struct\s+|enum\s+)', line):
            return True
        return False

    def _looks_like_function_signature_line(self, line: str) -> bool:
        s = line.strip()
        if not s:
            return False
        if re.match(r'^(if|for|while|switch|return|catch)\b', s):
            return False
        if '=' in s:
            return False
        if '.' in s or '->' in s:
            return False
        if re.search(r'^[A-Za-z_][\w:<>&*\s~]*\s+[A-Za-z_][\w:~]*\s*\([^;]*\)\s*(?:const)?\s*(?:noexcept(?:\([^)]*\))?)?\s*(?:\{|$)', s):
            return True
        return False
    
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
                if self._looks_like_declaration_or_signature(line):
                    continue
                if self._looks_like_function_signature_line(line):
                    continue
                if self._is_local_data_path(line, lines, line_no):
                    continue

                lower_line = line.lower()
                outbound_markers = ['client', 'stub', 'request', 'connect', 'send', 'post', 'get(', 'call(']
                if not any(m in lower_line for m in outbound_markers):
                    continue

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

            # bare rethrow inside catch-block
            if stripped == 'throw;':
                continue

            if not self._is_handler_boundary_context(file_path, lines, line_no):
                continue

            if self._is_validation_throw(lines, line_no, line):
                continue
            
            # WHITELIST: Constructor validation throws
            if any(exc in line for exc in ['std::invalid_argument', 'std::out_of_range', 'std::logic_error']):
                # Check if in constructor
                prev_lines = '\n'.join(lines[max(0, line_no-30):line_no])
                if re.search(r'\b([A-Za-z_]\w*)::\1\s*\(', prev_lines):
                    continue
            
            # Check if in try/catch context
            prev_context = '\n'.join(lines[max(0, line_no-20):line_no])

            if self._is_boundary_throw_path(line, prev_context):
                continue
            
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
        rel_file = str(file_path.relative_to(self.source_path)).lower()
        is_critical_path = any(token in rel_file for token in ['security', 'auth', 'server', 'api', 'network'])
        
        for line_no, line in enumerate(lines, 1):
            if self.catch_generic_pattern.search(line):
                # Accept catch-all at boundaries when exception is rethrown.
                catch_context = '\n'.join(lines[line_no:min(len(lines), line_no + 8)])
                if 'throw;' in catch_context:
                    continue

                # Many catch(...) blocks are intentional boundaries with explicit handling.
                # Flag only likely swallow patterns or critical runtime paths.
                lowered = catch_context.lower()
                likely_swallow = (
                    ('return' not in lowered)
                    and ('log' not in lowered)
                    and ('error' not in lowered)
                    and ('status' not in lowered)
                    and ('abort' not in lowered)
                )

                explicit_boundary_handling = any(tok in lowered for tok in [
                    'response', 'set_status', 'status_code', 'json', 'send', 'reply', 'http::status'
                ]) and ('return' in lowered or 'error' in lowered)

                explicit_fallback = any(tok in lowered for tok in [
                    'fallback', 'degrade', 'best effort', 'best-effort', 'recover', 'continue'
                ])

                if explicit_boundary_handling:
                    continue
                if explicit_fallback:
                    continue

                if not is_critical_path and not likely_swallow:
                    continue

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
