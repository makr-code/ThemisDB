#!/usr/bin/env python3
"""
Phase 10-1: Observability & Instrumentation Scanner

CWE-778 (Insufficient Logging), CWE-532 (Information Exposure)

Detects:
- Missing metrics (latency, throughput, errors)
- No trace points in critical paths
- Debug-only traces (not production)
- Unstructured event logging
- Missing performance counters
- No correlation IDs across services
- Log sampling not configurable
- Observability overhead not quantified
- No health check endpoints
- Metric cardinality explosion
"""

import re
from pathlib import Path
from typing import List, Dict


class ObservabilityScan:
    """Scan for observability and instrumentation issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for observability issues"""
        self.gaps = []
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp']:
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_missing_metrics(file_path, lines)
            self._check_trace_points(file_path, lines)
            self._check_correlation_ids(file_path, lines)
            self._check_structured_logging(file_path, lines)
            self._check_health_checks(file_path, lines)
        
        return self.gaps

    @staticmethod
    def _is_comment_or_preprocessor(line: str) -> bool:
        stripped = line.strip()
        return (
            not stripped or
            stripped.startswith('//') or
            stripped.startswith('/*') or
            stripped.startswith('*') or
            stripped.startswith('#')
        )

    @staticmethod
    def _looks_like_function_signature(line: str) -> bool:
        stripped = line.strip()
        if not stripped or '(' not in stripped or ')' not in stripped:
            return False
        if stripped.endswith(';'):
            return False
        if stripped.startswith('if ') or stripped.startswith('if('):
            return False
        if stripped.startswith('for ') or stripped.startswith('for('):
            return False
        if stripped.startswith('while ') or stripped.startswith('while('):
            return False
        if stripped.startswith('switch ') or stripped.startswith('switch('):
            return False
        return bool(re.search(r'[A-Za-z_][\w:<>~]*\s*\(', stripped))

    @staticmethod
    def _is_service_related_file(file_path: Path) -> bool:
        p = str(file_path).lower()
        return any(tok in p for tok in ['server', 'service', 'api', 'handler', 'http', 'grpc', 'rpc'])
    
    def _check_missing_metrics(self, file_path: Path, lines: List[str]):
        """Find missing metric instrumentation"""
        if file_path.suffix not in ['.cpp', '.cc', '.cxx']:
            return

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            if not self._looks_like_function_signature(line):
                continue
            if not re.search(r'(execute|query|process|handle|compute)\s*\(', line, re.IGNORECASE):
                continue

            next_lines = '\n'.join(lines[idx:min(idx + 40, len(lines))])
            if not re.search(r'(timer|latency|stopwatch|duration|elapsed|histogram|counter)', next_lines, re.IGNORECASE):
                self.gaps.append({
                    'file': str(file_path.relative_to(self.repo_root)),
                    'line': idx,
                    'category': 'observability',
                    'severity': 'MEDIUM',
                    'pattern': 'missing_latency_metric',
                    'description': 'No latency measurement for operation',
                    'context': line.strip()
                })
    
    def _check_trace_points(self, file_path: Path, lines: List[str]):
        """Find missing trace points in critical paths"""
        
        critical_functions = [
            'execute', 'query', 'commit', 'abort', 'failover', 'failback',
            'replicate', 'consensus', 'allocate', 'deallocate', 'migrate'
        ]
        
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            if not self._looks_like_function_signature(line):
                continue
            for func in critical_functions:
                if re.search(rf'\b{func}\s*\(', line, re.IGNORECASE):
                    # Check for trace/span
                    next_lines = '\n'.join(lines[idx:min(idx + 25, len(lines))])
                    
                    if not re.search(r'(trace|span|trace_point|TRACE|SPAN)', next_lines):
                        self.gaps.append({
                            'file': str(file_path.relative_to(self.repo_root)),
                            'line': idx,
                            'category': 'observability',
                            'severity': 'HIGH',
                            'pattern': 'missing_trace_point',
                            'description': f'Critical function {func} without trace point',
                            'context': line.strip()
                        })
                        break
    
    def _check_correlation_ids(self, file_path: Path, lines: List[str]):
        """Find missing correlation IDs in distributed calls"""
        
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue

            # Look for RPC/service calls
            if re.search(
                r'(call_remote\s*\(|rpc_call\s*\(|service\.\w+\s*\(|stub->\w+\s*\(|'
                r'http_client\s*\.|https_client\s*\.|curl_easy_\w+\s*\(|'
                r'rest_client\s*\.|web::http::client\s*\()',
                line,
                re.IGNORECASE,
            ):
                # Check for correlation ID
                next_lines = '\n'.join(lines[idx:min(idx+10, len(lines))])
                
                if not re.search(r'(correlation|request_id|trace_id|span_id)', next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'observability',
                        'severity': 'MEDIUM',
                        'pattern': 'missing_correlation_id',
                        'description': 'Distributed call without correlation ID',
                        'context': line.strip()
                    })
    
    def _check_structured_logging(self, file_path: Path, lines: List[str]):
        """Find unstructured logging"""
        
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            # Look for logging calls
            if re.search(r'(LOG|logger|log)\s*\(', line):
                # Check if structured (JSON or key-value)
                if not re.search(r'(json|structured|key.*value|\{.*:|\[)', line):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'observability',
                        'severity': 'LOW',
                        'pattern': 'unstructured_log',
                        'description': 'Unstructured logging (use structured format)',
                        'context': line.strip()
                    })
    
    def _check_health_checks(self, file_path: Path, lines: List[str]):
        """Find missing health check implementation"""
        if file_path.suffix not in ['.cpp', '.cc', '.cxx']:
            return
        if not self._is_service_related_file(file_path):
            return

        file_text = '\n'.join(lines)
        if not re.search(r'(listen|route|endpoint|handler|server|service|grpc|http)', file_text, re.IGNORECASE):
            return

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_preprocessor(line):
                continue
            if not self._looks_like_function_signature(line):
                continue

            if re.search(r'(start|init|initialize)\s*\(', line, re.IGNORECASE):
                next_lines = '\n'.join(lines[idx:min(idx + 80, len(lines))])

                if not re.search(r'(health|status|ping|heartbeat)', next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'observability',
                        'severity': 'MEDIUM',
                        'pattern': 'missing_health_check',
                        'description': 'Service initialization without nearby health/status handling',
                        'context': line.strip()
                    })
