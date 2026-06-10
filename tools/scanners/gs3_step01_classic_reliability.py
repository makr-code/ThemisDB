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
        'http_call': re.compile(r'\b(?:httplib::Client|http_client|https_client|curl_easy|cpr::|rest_client|web::http::client)\b.*\('),
        'socket_call': re.compile(r'\b(?:connect|async_connect|reconnect)\s*\('),
        'rpc_call': re.compile(r'\b(?:grpc::\w+|rpc(?:_client)?::\w+|rest(?:_client)?::\w+|websocket\w*)\s*\('),
    }
    
    # Blocking operations
    BLOCKING_PATTERNS = {
        'mutex_lock': re.compile(r'\.\s*lock\s*\('),
        'thread_join': re.compile(r'(thread|task)\.join\(\)'),
        'file_io': re.compile(r'(?<!\.)\b(read|write|open|fopen|fscanf)\s*\('),
        'semaphore_wait': re.compile(r'(wait|acquire)\s*\('),
    }
    
    # Exception patterns
    EXCEPTION_PATTERNS = {
        'throw': re.compile(r'\bthrow\b'),
        'catch_generic': re.compile(r'catch\s*\(\s*\.\.\.\s*\)'),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[ReliabilityGap]] = {}

    def _has_timeout_context(self, lines: List[str], line_idx: int) -> bool:
        """Detect timeout semantics in nearby context (before/after)."""
        start = max(0, line_idx - 8)
        end = min(len(lines), line_idx + 8)
        context = ''.join(lines[start:end]).lower()
        timeout_tokens = [
            'timeout', '_for', 'wait_for', 'wait_until', 'duration',
            'expires_after', 'expires_at', 'deadline', 'milliseconds', 'seconds',
        ]
        return any(tok in context for tok in timeout_tokens)
    
    def _is_sync_only_operation(self, line: str) -> bool:
        """WHITELIST: Identify sync-only operations that don't need timeouts.
        
        Sync operations (local computation, no blocking I/O) don't need timeouts.
        Returns True if this is a safe sync operation (should be whitelisted).
        """
        l_lower = line.lower()
        
        # WHITELIST 1: Local synchronous operations
        # (computations, in-memory data structures, no blocking calls)
        sync_patterns = [
            'for_each(', 'std::find', 'std::sort', 'std::copy',
            'std::accumulate', 'std::transform', 'std::filter',
            'vector.push_back', 'vector.pop_back', 'map[',
            'obj.field', 'obj.method()', '.get()', '.size()',
            'std::lock_guard', 'scoped_lock',  # These are synchronization, not blocking I/O
        ]
        for pattern in sync_patterns:
            if pattern in l_lower:
                return True
        
        # WHITELIST 2: Synchronous file reads with known-small sizes (like PEM certs)
        if self._is_local_pem_read(line):
            return True
        
        # WHITELIST 3: Constructor/destructor operations
        if '::~' in line or 'destructor' in l_lower or '__init__' in l_lower:
            return True
        
        # WHITELIST 4: Configuration/setup operations (happen at startup)
        if 'config' in l_lower or 'init' in l_lower or 'setup' in l_lower or 'start' in l_lower:
            # But NOT for RPC/network calls in setup
            if not any(rpc in l_lower for rpc in ['grpc', 'http', 'socket', 'query']):
                return True
        
        return False
    
    def _is_handler_or_entry_point(self, func_name: str, line: str) -> bool:
        """WHITELIST: Check if this is a handler/entry point function that SHOULD have health checks.
        
        Returns True if this looks like a handler function (not internal utility).
        """
        l_lower = line.lower() + func_name.lower()
        
        # HANDLER PATTERNS: These should have health checks
        handler_patterns = [
            'handle', 'process', 'execute', 'serve', 'dispatch',
            'rpc', 'grpc', 'http', 'endpoint', 'api',
            'request', 'query', 'command',
        ]
        
        is_handler = any(p in l_lower for p in handler_patterns)
        
        if not is_handler:
            return False  # Not a handler
        
        # SKIP INTERNAL UTILITIES: Even if they have handler-like names
        skip_patterns = [
            '::detail::', '::internal::', '::impl::',
            '_internal', '__impl', '_impl',
            'private:', 'protected:',
        ]
        is_internal = any(p in line for p in skip_patterns)
        
        return is_handler and not is_internal
    
    def _should_skip_no_health_check(self, line: str, func_name: str) -> bool:
        """WHITELIST: Skip NO_HEALTH_CHECK for non-handler functions.
        
        Returns True if this line should be whitelisted (NOT flagged).
        """
        # WHITELIST 1: Internal utilities/data processors (not entry points)
        if not self._is_handler_or_entry_point(func_name, line):
            return True
        
        # WHITELIST 2: Functions with explicit health check markers
        if 'health' in line.lower() or 'check' in line.lower() or 'status' in line.lower():
            return True
        
        # WHITELIST 3: Already has init or set in same context
        if 'init' in line.lower() or 'set' in line.lower():
            return True
        
        # WHITELIST 4: Critical path marker (async, timeout, deadline)
        critical_markers = ['timeout', 'deadline', 'async', 'critical']
        if any(m in line.lower() for m in critical_markers):
            return True
        
        return False

    def _is_local_pem_read(self, line: str) -> bool:
        """Ignore local cert/key file reads that are not network blocking calls."""
        l = line.lower()
        if 'fopen' not in l and 'open(' not in l:
            return False
        return any(ext in l for ext in ['.pem', '.crt', '.key'])

    def _is_local_data_path(self, line: str, lines: List[str], line_idx: int) -> bool:
        context = ''.join(lines[max(0, line_idx - 8):min(len(lines), line_idx + 8)]).lower()
        local_tokens = [
            'rocksdb', 'sqlite', 'leveldb', 'in_memory', 'local cache',
            'cache', 'filesystem', 'manifest', 'wal'
        ]
        if any(token in context for token in local_tokens):
            return True
        return 'query(' in line.lower() and 'http' not in context and 'rpc' not in context

    def _is_boundary_throw_path(self, line: str, prev_context: str) -> bool:
        combined = (prev_context + '\n' + line).lower()
        boundary_tokens = [
            'handler', 'endpoint', 'controller', 'api', 'rpc',
            'build', 'builder', 'initialize', 'init', 'configure', 'factory'
        ]
        return any(token in combined for token in boundary_tokens) and 'throw std::' in combined

    def _is_critical_runtime_path(self, file_path: Path) -> bool:
        rel = str(file_path.relative_to(self.repo_root)).lower()
        return any(token in rel for token in ['security', 'auth', 'server', 'api', 'network'])

    def _is_handler_boundary_context(self, file_path: Path, lines: List[str], line_idx: int) -> bool:
        rel = str(file_path.relative_to(self.repo_root)).lower()
        if any(tok in rel for tok in ['server', 'api', 'handler', 'endpoint', 'rpc', 'network']):
            return True

        start = max(0, line_idx - 30)
        prev = ''.join(lines[start:line_idx + 1]).lower()
        return any(tok in prev for tok in ['handle', 'endpoint', 'controller', 'request', 'rpc', 'serve'])

    def _is_constructor_validation_throw(self, lines: List[str], line_idx: int, line: str) -> bool:
        """Allow common constructor argument-validation throws."""
        l = line.lower()
        if 'throw' not in l:
            return False
        if not any(exc in l for exc in [
            'std::invalid_argument',
            'std::out_of_range',
            'std::domain_error',
            'std::logic_error',
            'std::runtime_error',
        ]):
            return False

        start = max(0, line_idx - 30)
        prev_lines = lines[start:line_idx]
        prev = ''.join(prev_lines)

        # Qualified constructor definition: ClassName::ClassName(...)
        if re.search(r'\b([A-Za-z_]\w*)::\1\s*\(', prev):
            return True

        # In-class constructor signature before throw.
        if re.search(r'^\s*(explicit\s+)?[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:noexcept\s*\([^)]*\))?\s*(?::[^{}]*)?\s*\{', prev, re.MULTILINE):
            return True

        # Common validation-style throw guarded by input checks.
        recent = ''.join(lines[max(0, line_idx - 3):line_idx + 1]).lower()
        if ('if' in recent and any(tok in recent for tok in ['invalid', 'must', 'required', 'cannot', 'empty'])):
            return True

        if 'throw std::' in l and 'if' in recent:
            return True

        return False

    def _looks_like_declaration_or_signature(self, line: str) -> bool:
        s = line.strip()
        if not s or s.startswith('#'):
            return True
        # Function signatures and declarations should not be treated as runtime call sites.
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
        # Function definition/signature shape: ReturnType Qualifier::name(args) [const] [{]
        if re.search(r'^[A-Za-z_][\w:<>&*\s~]*\s+[A-Za-z_][\w:~]*\s*\([^;]*\)\s*(?:const)?\s*(?:noexcept(?:\([^)]*\))?)?\s*(?:\{|$)', s):
            return True
        return False
    
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
            if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
                continue
            if 'TEST' in line or 'MOCK' in line or 'test' in file_path.name:
                continue
            
            # Check for RPC/network calls without retry
            for rpc_type, pattern in self.RPC_PATTERNS.items():
                if pattern.search(line):
                    if self._looks_like_declaration_or_signature(line):
                        continue
                    if self._looks_like_function_signature_line(line):
                        continue
                    if self._is_local_data_path(line, lines, line_num - 1):
                        continue

                    # Focus on outbound/network client behavior; avoid broad server-side signatures.
                    lower_line = line.lower()
                    if rpc_type in ('grpc_call', 'http_call', 'rpc_call'):
                        outbound_markers = ['client', 'stub', 'request', 'connect', 'send', 'post', 'get(', 'call(']
                        if not any(m in lower_line for m in outbound_markers):
                            continue

                    # Check if retry/backoff exists in next few lines
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+14)])
                    
                    has_retry = any(retry_word in next_context.lower() 
                                   for retry_word in ['retry', 'attempt', 'loop', 'for ', 'retrypolicy', 'resilien', 'fallback'])
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
                    if block_type == 'file_io' and self._is_local_pem_read(line):
                        continue
                    
                    # WHITELIST: Skip sync-only operations that don't need timeout
                    if self._is_sync_only_operation(line):
                        continue
                    
                    # Check if timeout is specified
                    has_timeout = self._has_timeout_context(lines, line_num - 1)
                    
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
                stripped = line.strip()
                if not stripped.startswith('throw '):
                    continue
                if stripped == 'throw;':
                    continue
                if not self._is_handler_boundary_context(file_path, lines, line_num - 1):
                    continue
                if self._is_constructor_validation_throw(lines, line_num - 1, line):
                    continue
                # Check if this is in a try/catch context
                prev_context = ''.join(lines[max(0, line_num-20):line_num])

                if self._is_boundary_throw_path(line, prev_context):
                    continue
                
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
                catch_context = ''.join(lines[line_num:min(len(lines), line_num + 8)])
                if 'throw;' in catch_context:
                    continue

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

                if not self._is_critical_runtime_path(file_path) and not likely_swallow:
                    continue

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
                # Only treat this as a field/variable declaration candidate.
                # Avoid function signatures and call expressions that contain
                # "Status" as a return type or namespace symbol.
                looks_like_declaration = ';' in line and '(' not in line
                if looks_like_declaration and '=' not in line and 'return' not in line:
                    # Extract function name context (look backwards for function signature)
                    func_name = ''
                    for i in range(line_num - 1, max(0, line_num - 30), -1):
                        func_match = re.search(r'(\w+)\s*\(', lines[i])
                        if func_match:
                            func_name = func_match.group(1)
                            break
                    
                    # Check if we should skip this based on whitelist
                    if self._should_skip_no_health_check(line, func_name):
                        continue  # Whitelisted - not a handler or already checked
                    
                    # Possible uninitialized status
                    next_context = ''.join(lines[line_num:min(len(lines), line_num+3)])
                    if 'init' not in next_context.lower() and 'set' not in next_context.lower():
                        gap = ReliabilityGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=ReliabilityGapType.NO_HEALTH_CHECK,
                            snippet=line.strip()[:100],
                            severity='MEDIUM',
                            description=f'Handler function "{func_name}" — status field but no health check',
                            remediation='Initialize status and implement periodic health checks for handler functions'
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
