#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Attack Vector Detection (P11-5)

Detects OWASP Top 10 + CWE attack patterns:
- SQL injection (CWE-89)
- Command injection (CWE-78)
- Path traversal (CWE-22)
- XSS vulnerabilities (CWE-79)
- Authentication bypass (CWE-287)
- CSRF/Deserialization (CWE-352/502)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class AttackVectorType(Enum):
    """Attack vector classifications (OWASP Top 10)"""
    SQL_INJECTION = "sql_injection"
    COMMAND_INJECTION = "command_injection"
    PATH_TRAVERSAL = "path_traversal"
    XSS_VULNERABILITY = "xss_vulnerability"
    CSRF_VULNERABILITY = "csrf_vulnerability"
    AUTH_BYPASS = "auth_bypass"
    DESERIALIZATION = "deserialization"


@dataclass
class AttackVectorGap:
    """Represents an attack vector vulnerability"""
    file_path: str
    line_num: int
    gap_type: AttackVectorType
    snippet: str
    severity: str
    description: str
    remediation: str
    confidence: float
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
            'confidence': self.confidence,
        }


class AttackVectorScanner:
    """Detect OWASP Top 10 attack vectors"""
    
    # SQL INJECTION PATTERNS (CWE-89)
    SQL_PATTERNS = {
        'query_concat': re.compile(r'SELECT.*\+|UPDATE.*\+|INSERT.*\+', re.IGNORECASE),
        'format_query': re.compile(r'sprintf.*SELECT|sprintf.*UPDATE|query\(.*format', re.IGNORECASE),
        'string_format': re.compile(r'query\s*=\s*["\'].*%[sd]|sql.*\+.*var', re.IGNORECASE),
    }
    
    # COMMAND INJECTION PATTERNS (CWE-78)
    COMMAND_PATTERNS = {
        'system_call': re.compile(r'\bsystem\(|\bexec\(|\bpopen\(|shell_exec', re.IGNORECASE),
        'unquoted': re.compile(r'system\([^"]*\$|exec\([^"]*\$', re.IGNORECASE),
    }
    
    # PATH TRAVERSAL PATTERNS (CWE-22)
    PATH_PATTERNS = {
        'unsanitized': re.compile(r'open\(.*\$|fopen\(.*\$|readfile\(.*\$', re.IGNORECASE),
        'no_validate': re.compile(r'user.*path|param.*path|user.*file', re.IGNORECASE),
    }
    
    # XSS PATTERNS (CWE-79)
    XSS_PATTERNS = {
        'echo_user_input': re.compile(r'echo\s+\$_|print\s+\$_|cout.*REQUEST', re.IGNORECASE),
        'unescaped_output': re.compile(r'echo.*\$\w+\s*;|printf.*%s.*\$', re.IGNORECASE),
    }
    
    # CSRF PATTERNS (CWE-352)
    CSRF_PATTERNS = {
        'no_token': re.compile(r'form.*method.*post(?!.*token)|POST.*(?!.*csrf)', re.IGNORECASE),
        'weak_validation': re.compile(r'if.*token.*==|if.*referer.*check', re.IGNORECASE),
    }
    
    # AUTH BYPASS PATTERNS (CWE-287)
    AUTH_PATTERNS = {
        'weak_compare': re.compile(r'if.*password.*==|if.*token.*==|if.*auth.*==', re.IGNORECASE),
        'no_validation': re.compile(r'authenticate\(\)(?!.*check)|login\(\)(?!.*valid)', re.IGNORECASE),
    }
    
    # DESERIALIZATION PATTERNS (CWE-502)
    DESERIAL_PATTERNS = {
        'unsafe_unserialize': re.compile(r'unserialize\(|pickle\.load|JSON\.parse.*untrusted', re.IGNORECASE),
        'untrusted_data': re.compile(r'unserialize\(\$_|pickle\.loads\(\$|deserialize.*user', re.IGNORECASE),
    }
    
    # APPROVED PATTERNS (WHITELIST)
    APPROVED_PATTERNS = {
        'prepared_stmt': re.compile(r'prepared|parameterized|PreparedStatement|bind'),
        'escaped_input': re.compile(r'htmlspecialchars|escape|sanitize|whitelist'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [r'test.*attack', r'mock.*inject', r'fixture.*security']
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[AttackVectorGap]] = {}
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_PATTERNS]
    
    def _is_test_context(self, file_name: str, line: str) -> bool:
        if 'test' in file_name.lower():
            return True
        for pattern in self.test_patterns:
            if pattern.search(line):
                return True
        return False
    
    def _is_approved_usage(self, line: str) -> bool:
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def _check_sql_injection(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect SQL injection (CWE-89)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            for sql_type, pattern in self.SQL_PATTERNS.items():
                if pattern.search(line) and not self._is_approved_usage(line):
                    gaps.append(AttackVectorGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=AttackVectorType.SQL_INJECTION,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Potential SQL injection (CWE-89)',
                        remediation='Use parameterized queries or prepared statements',
                        confidence=0.75
                    ))
                    break
        return gaps
    
    def _check_command_injection(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect command injection (CWE-78)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.COMMAND_PATTERNS['system_call'].search(line) and not self._is_approved_usage(line):
                gaps.append(AttackVectorGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=AttackVectorType.COMMAND_INJECTION,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Potential command injection (CWE-78)',
                    remediation='Use array form of exec() or escapeshellarg()',
                    confidence=0.85
                ))
        return gaps
    
    def _check_path_traversal(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect path traversal (CWE-22)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.PATH_PATTERNS['unsanitized'].search(line) and not self._is_approved_usage(line):
                gaps.append(AttackVectorGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=AttackVectorType.PATH_TRAVERSAL,
                    snippet=line.strip()[:100],
                    severity='HIGH',
                    description='Potential path traversal (CWE-22)',
                    remediation='Validate and sanitize file paths; restrict to safe directories',
                    confidence=0.70
                ))
        return gaps
    
    def _check_xss_vulnerability(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect XSS vulnerabilities (CWE-79)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if (self.XSS_PATTERNS['echo_user_input'].search(line) or 
                self.XSS_PATTERNS['unescaped_output'].search(line)):
                if not self._is_approved_usage(line):
                    gaps.append(AttackVectorGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=AttackVectorType.XSS_VULNERABILITY,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Potential XSS vulnerability (CWE-79)',
                        remediation='Always HTML-escape user input before output',
                        confidence=0.65
                    ))
        return gaps
    
    def _check_csrf_vulnerability(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect CSRF vulnerabilities (CWE-352) — refined for high confidence"""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Only flag actual POST/PUT/DELETE endpoints with form handling
            if not any(x in line.lower() for x in ['post', 'put', 'delete']):
                continue
            
            # Look for form submission or parameter parsing without token
            if not any(x in line.lower() for x in ['form', 'body', 'parse', 'param']):
                continue
            
            # Check if CSRF token is present in same line or next 5 lines
            context_end = min(len(lines), line_num + 5)
            context = ''.join(lines[line_num-1:context_end]).lower()
            
            has_csrf = any(x in context for x in ['csrf', 'token', '_token', 'xsrf-token', 'validation'])
            
            if not has_csrf and self.CSRF_PATTERNS['no_token'].search(line):
                gaps.append(AttackVectorGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=AttackVectorType.CSRF_VULNERABILITY,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='State-changing operation without CSRF token validation (CWE-352)',
                    remediation='Validate CSRF tokens for all POST/PUT/DELETE operations',
                    confidence=0.70  # Increased confidence with better context
                ))
        return gaps
    
    def _check_auth_bypass(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect authentication bypass (CWE-287)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.AUTH_PATTERNS['weak_compare'].search(line) and not self._is_approved_usage(line):
                gaps.append(AttackVectorGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=AttackVectorType.AUTH_BYPASS,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Weak authentication logic (CWE-287)',
                    remediation='Use cryptographic comparison (hash_equals or similar)',
                    confidence=0.80
                ))
        return gaps
    
    def _check_deserialization(self, file_path: Path, lines: List[str]) -> List[AttackVectorGap]:
        """Detect deserialization vulnerabilities (CWE-502)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.DESERIAL_PATTERNS['unsafe_unserialize'].search(line):
                if self.DESERIAL_PATTERNS['untrusted_data'].search(line):
                    gaps.append(AttackVectorGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=AttackVectorType.DESERIALIZATION,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Unsafe deserialization of untrusted data (CWE-502)',
                        remediation='Use safe serialization (JSON); validate before deserializing',
                        confidence=0.90
                    ))
        return gaps
    
    def scan_file(self, file_path: Path) -> List[AttackVectorGap]:
        """Scan single file for attack vectors"""
        gaps = []
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        gaps.extend(self._check_sql_injection(file_path, lines))
        gaps.extend(self._check_command_injection(file_path, lines))
        gaps.extend(self._check_path_traversal(file_path, lines))
        gaps.extend(self._check_xss_vulnerability(file_path, lines))
        gaps.extend(self._check_csrf_vulnerability(file_path, lines))
        gaps.extend(self._check_auth_bypass(file_path, lines))
        gaps.extend(self._check_deserialization(file_path, lines))
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[AttackVectorGap]]:
        """Scan module for attack vectors"""
        gaps_by_file = {}
        for directory in [self.repo_root / 'src' / module, self.repo_root / 'include' / module]:
            if not directory.exists():
                continue
            for file_path in directory.rglob('*.cpp'):
                gaps = self.scan_file(file_path)
                if gaps:
                    gaps_by_file[str(file_path)] = gaps
            for file_path in directory.rglob('*.h'):
                gaps = self.scan_file(file_path)
                if gaps:
                    gaps_by_file[str(file_path)] = gaps
        return gaps_by_file
    
    def scan_repository(self) -> Dict[str, List[AttackVectorGap]]:
        """Scan entire repository"""
        gaps_by_file = {}
        for src_file in (self.repo_root / 'src').rglob('*.cpp'):
            gaps = self.scan_file(src_file)
            if gaps:
                gaps_by_file[str(src_file)] = gaps
        for hdr_file in (self.repo_root / 'include').rglob('*.h'):
            gaps = self.scan_file(hdr_file)
            if gaps:
                gaps_by_file[str(hdr_file)] = gaps
        self.gaps = gaps_by_file
        return gaps_by_file
    
    def to_json(self) -> str:
        """Convert gaps to JSON"""
        gaps_data = {}
        for file_path, gap_list in self.gaps.items():
            gaps_data[file_path] = [g.to_dict() for g in gap_list]
        return json.dumps(gaps_data, indent=2)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Attack Vector Scanner')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    scanner = AttackVectorScanner(args.repo)
    gaps = scanner.scan_repository()
    result = scanner.to_json()
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(result)
        print(f"Results written to {args.output}")
    else:
        print(result)
    
    total_gaps = sum(len(v) for v in gaps.values())
    print(f"\nTotal attack vector vulnerabilities found: {total_gaps}")
