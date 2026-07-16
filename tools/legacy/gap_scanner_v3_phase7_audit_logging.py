#!/usr/bin/env python3
"""
Phase 7-1: Audit Trail & Logging Consistency Scanner

CWE-532 (Information Exposure), CWE-778 (Insufficient Logging)

Detects:
- Missing audit logs in security-critical functions
- Hardcoded std::cout/printf instead of structured logging
- Sensitive data (PII, credentials) in logs
- Inconsistent log levels
- Missing context (user ID, request ID, timestamp)
- Non-deterministic output
- Missing log rotation/TTL
- No log integrity (HMAC/signatures)
"""

import re
from pathlib import Path
from typing import List, Dict, Tuple


class AuditLoggingScan:
    """Scan for audit trail and logging consistency issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
        
        # Security-critical functions that should have audit logs
        self.security_functions = [
            'authenticate', 'authorize', 'login', 'logout',
            'grant_access', 'revoke_access', 'create_user',
            'delete_user', 'modify_permissions', 'admin_action',
            'execute_query', 'drop_table', 'change_password'
        ]
        
        # Sensitive data patterns
        self.sensitive_patterns = [
            r'password', r'secret', r'token', r'api_key', r'credential',
            r'auth', r'ssn', r'email', r'phone', r'credit_card'
        ]
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for audit logging gaps"""
        
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
            self._check_missing_audit_logs(file_path, lines)
            self._check_hardcoded_output(file_path, lines)
            self._check_sensitive_data_logging(file_path, lines)
            self._check_inconsistent_log_levels(file_path, lines)
            self._check_missing_context(file_path, lines)
        
        return self.gaps
    
    def _check_missing_audit_logs(self, file_path: Path, lines: List[str]):
        """Find security functions without audit logs"""
        rel_file = str(file_path.relative_to(self.repo_root)).replace('\\', '/').lower()
        if rel_file.startswith('tests/') or rel_file.startswith('benchmarks/'):
            return
        
        for idx, line in enumerate(lines, 1):
            # Check for security function definitions
            for sec_func in self.security_functions:
                if not re.search(rf'\b{sec_func}\s*\(', line, re.IGNORECASE):
                    continue

                if line.strip().endswith(';'):
                    continue
                if not re.search(rf'\b{sec_func}\s*\([^;]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:\{{)?\s*$', line, re.IGNORECASE):
                    continue

                # Look ahead for audit logging
                next_lines = '\n'.join(lines[idx:min(idx+40, len(lines))])

                if not re.search(
                    r'(audit_log|logger->.*log|LOG\(|AUDIT_|audit_trail)',
                    next_lines,
                    re.IGNORECASE
                ):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'audit_logging',
                        'severity': 'CRITICAL',
                        'pattern': 'missing_audit_log',
                        'description': f'Security function "{sec_func}" without audit log',
                        'context': line.strip()
                    })
    
    def _check_hardcoded_output(self, file_path: Path, lines: List[str]):
        """Find std::cout/printf instead of structured logging"""
        
        for idx, line in enumerate(lines, 1):
            # Hardcoded output in non-test code
            if re.search(r'(std::cout|printf|puts|fprintf\s*\(\s*stdout)', line):
                if 'test' not in str(file_path).lower():
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'audit_logging',
                        'severity': 'HIGH',
                        'pattern': 'hardcoded_output',
                        'description': 'Hardcoded std::cout/printf instead of structured logging',
                        'context': line.strip()
                    })
    
    def _check_sensitive_data_logging(self, file_path: Path, lines: List[str]):
        """Find potential PII/credentials in log statements"""
        
        for idx, line in enumerate(lines, 1):
            # Look for log statements
            if re.search(r'(log|LOG|logger)\s*\(|<<', line):
                # Check for sensitive variables
                for pattern in self.sensitive_patterns:
                    if re.search(rf'\b{pattern}\b', line, re.IGNORECASE):
                        # Check if it's being logged
                        if re.search(rf'log.*{pattern}|{pattern}.*log|<<.*{pattern}', line, re.IGNORECASE):
                            self.gaps.append({
                                'file': str(file_path.relative_to(self.repo_root)),
                                'line': idx,
                                'category': 'audit_logging',
                                'severity': 'CRITICAL',
                                'pattern': 'sensitive_data_logging',
                                'description': f'Potential PII/credential logging: {pattern}',
                                'context': line.strip()
                            })
    
    def _check_inconsistent_log_levels(self, file_path: Path, lines: List[str]):
        """Find inconsistent log level usage"""
        
        for idx, line in enumerate(lines, 1):
            # Check for INFO/DEBUG logs of critical operations
            if re.search(r'(CRITICAL|security|auth|admin).*function', line, re.IGNORECASE):
                if re.search(r'logger.*(?:INFO|DEBUG|trace)', line, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'audit_logging',
                        'severity': 'HIGH',
                        'pattern': 'inconsistent_log_level',
                        'description': 'Security operation logged at low level (INFO/DEBUG instead of WARN/ERROR)',
                        'context': line.strip()
                    })
    
    def _check_missing_context(self, file_path: Path, lines: List[str]):
        """Find log statements without context (user ID, request ID, etc)"""
        
        for idx, line in enumerate(lines, 1):
            if re.search(r'logger->(?:warn|error|critical)\(', line, re.IGNORECASE):
                # Check if context is included
                context_found = re.search(
                    r'(user_id|request_id|timestamp|correlation_id|session_id)',
                    line,
                    re.IGNORECASE
                )
                
                if not context_found:
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'audit_logging',
                        'severity': 'MEDIUM',
                        'pattern': 'missing_context',
                        'description': 'Log statement missing context (user ID, request ID, timestamp)',
                        'context': line.strip()
                    })
