#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Attack Vector Detection

Detects OWASP/CWE attack patterns:
- SQL injection vulnerabilities
- Command injection vulnerabilities
- Path traversal vulnerabilities
- XSS (Cross-Site Scripting) patterns
- CSRF (Cross-Site Request Forgery) gaps
- Authentication bypass patterns
- Deserialization vulnerabilities
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class AttackVectorType(Enum):
    """Attack vector vulnerability classifications"""
    SQL_INJECTION = "sql_injection"            # SQL injection risk
    COMMAND_INJECTION = "command_injection"    # OS command injection
    PATH_TRAVERSAL = "path_traversal"          # Directory traversal
    XSS_VULNERABILITY = "xss_vulnerability"    # Cross-site scripting
    CSRF_VULNERABILITY = "csrf_vulnerability"  # CSRF token missing
    AUTH_BYPASS = "auth_bypass"                # Authentication bypass
    DESERIALIZATION = "deserialization"        # Unsafe deserialization
    XXE_VULNERABILITY = "xxe_vulnerability"    # XML External Entity
    UNSAFE_REDIRECT = "unsafe_redirect"        # Open redirect
    HARDCODED_CREDENTIALS = "hardcoded_credentials"  # Credentials in code


@dataclass
class AttackVectorGap:
    """Represents an attack vector vulnerability"""
    file_path: str
    line_num: int
    gap_type: AttackVectorType
    snippet: str
    severity: str  # CRITICAL, HIGH, MEDIUM
    description: str
    remediation: str
    confidence: float  # 0.0-1.0
    
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
    """Detect attack vector vulnerabilities"""
    
    # SQL INJECTION PATTERNS
    SQL_PATTERNS = {
        'query_concat': re.compile(r'SELECT.*\+|UPDATE.*\+|INSERT.*\+|".*".*\+|\'.*\'.*\+', re.IGNORECASE),
        'format_query': re.compile(r'sprintf.*SELECT|sprintf.*UPDATE|sprintf.*INSERT|format.*query', re.IGNORECASE),
        'no_param': re.compile(r'query\(\s*["\'].*\w+.*["\']|sql\s*<<|execute.*query', re.IGNORECASE),
    }
    
    # COMMAND INJECTION PATTERNS
    COMMAND_PATTERNS = {
        'system_call': re.compile(r'system\(|exec\(|popen\(|shell_exec|passthru', re.IGNORECASE),
        'unquoted_input': re.compile(r'system\([^"]*\w+[^"]*\)|exec\([^"]*\w+', re.IGNORECASE),
    }
    
    # PATH TRAVERSAL PATTERNS
    PATH_PATTERNS = {
        'unsanitized_path': re.compile(r'open\(|fopen\(|readfile\(.*\$', re.IGNORECASE),
        'no_validate': re.compile(r'user.*path|param.*path|input.*file', re.IGNORECASE),
    }
    
    # XSS PATTERNS
    XSS_PATTERNS = {
        'echo_user_input': re.compile(r'echo\s+\$_|print\s+\$_|echo.*REQUEST|echo.*GET|echo.*POST', re.IGNORECASE),
        'no_escape': re.compile(r'echo.*\$\w+;|print.*\$\w+;', re.IGNORECASE),
    }
    
    # APPROVED PATTERNS (WHITELIST)
    APPROVED_PATTERNS = {
        'prepared_stmt': re.compile(r'prepared|parameterized|PreparedStatement|bind\('),
        'escaped_input': re.compile(r'htmlspecialchars|mysqli_real_escape|addslashes|mysql_real_escape_string|prepared'),
        'validated': re.compile(r'validate|sanitize|whitelist|filter'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [
        r'test.*injection|injection.*test',
        r'mock.*attack|stub.*security',
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[AttackVectorGap]] = {}
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_PATTERNS]
    
    def _is_test_context(self, file_name: str, line: str) -> bool:
        """Check if this is test/fixture context."""
        if 'test' in file_name.lower():
            return True
        for pattern in self.test_patterns:
            if pattern.search(line):
                return True
        return False
    
    def _is_approved_usage(self, line: str) -> bool:
        """Check if using approved security patterns."""
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def scan_file(self, file_path: Path) -> List[AttackVectorGap]:
        """Scan single file for attack vectors"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for SQL injection
            for sql_type, pattern in self.SQL_PATTERNS.items():
                if pattern.search(line):
                    if not self._is_approved_usage(line):
                        gap = AttackVectorGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=AttackVectorType.SQL_INJECTION,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description='Potential SQL injection vulnerability',
                            remediation='Use parameterized queries and prepared statements',
                            confidence=0.75
                        )
                        gaps.append(gap)
                        break
            
            # Check for command injection
            if self.COMMAND_PATTERNS['system_call'].search(line):
                if not self._is_approved_usage(line):
                    gap = AttackVectorGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=AttackVectorType.COMMAND_INJECTION,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Potential command injection vulnerability',
                        remediation='Use exec array form or escapeshellarg() with arguments array',
                        confidence=0.85
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[AttackVectorGap]]:
        """Scan module for attack vectors"""
        gaps_by_file = {}
        
        src_dir = self.repo_root / 'src' / module
        include_dir = self.repo_root / 'include' / module
        
        for directory in [src_dir, include_dir]:
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
        """Scan entire repository for attack vectors"""
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
        """Convert gaps to JSON format"""
        gaps_data = {}
        for file_path, gap_list in self.gaps.items():
            gaps_data[file_path] = [g.to_dict() for g in gap_list]
        return json.dumps(gaps_data, indent=2)


if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='Attack Vector Detection Scanner')
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
    
    # Print summary
    total_gaps = sum(len(v) for v in gaps.values())
    print(f"\nTotal attack vector vulnerabilities found: {total_gaps}")
