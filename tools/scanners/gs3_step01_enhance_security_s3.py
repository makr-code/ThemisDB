#!/usr/bin/env python3
"""
Phase 1-4 Enhancement: S-3 Injection Attack Prevention (Enhanced)

CWE-94: Improper Control of Generation of Code (Code Injection)
Extended patterns for command injection, path traversal, and other injection vectors.

Enhanced Patterns:
1. Command Injection via Shell Execution (system(), popen(), exec())
2. Path Traversal in File Operations (fopen(), std::ifstream)
3. LDAP Injection (ldap_search with user input)
4. XML External Entity (XXE) Injection (XML parsing without validation)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional
from enum import Enum


class S3InjectionGapType(Enum):
    """S-3 Enhanced injection attack categories"""
    COMMAND_INJECTION = "command_injection"
    PATH_TRAVERSAL = "path_traversal"
    LDAP_INJECTION = "ldap_injection"
    XXE_INJECTION = "xxe_injection"
    FORMAT_STRING = "format_string"


@dataclass
class S3InjectionGap:
    """Represents an S-3 injection attack gap"""
    file_path: str
    line_num: int
    gap_type: S3InjectionGapType
    snippet: str
    severity: str  # CRITICAL, HIGH
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
            'enhancement': 'S-3',
            'cwe': 'CWE-94',
        }


class S3InjectionScanner:
    """S-3 Enhanced: Injection Attack Prevention"""
    
    # Pattern 1: Command Injection
    COMMAND_INJECTION_PATTERNS = {
        'system_user_input': (
            re.compile(r'system\s*\(\s*(?:user_|input|arg|argv|param|data|cmd|command)', re.IGNORECASE),
            'CRITICAL',
            'User input passed to system() — high command injection risk'
        ),
        'popen_user_input': (
            re.compile(r'popen\s*\(\s*(?:user_|input|arg|argv|param|data|cmd|command)', re.IGNORECASE),
            'CRITICAL',
            'User input in popen() command — command injection vulnerability'
        ),
        'exec_family_risk': (
            re.compile(r'(?:execv|execl|execvp|execvpe)\s*\(\s*["\'][^"\']*["\'],'),
            'HIGH',
            'exec() family function — ensure arguments are properly validated'
        ),
        'backtick_execution': (
            re.compile(r'`[^`]*\$[^`]*`'),
            'CRITICAL',
            'Backtick command execution with variable interpolation'
        ),
        'popen_format_string': (
            re.compile(r'popen\s*\(\s*sprintf\s*\('),
            'CRITICAL',
            'popen() with sprintf() — command injection risk'
        ),
        'system_format_string': (
            re.compile(r'system\s*\(\s*sprintf\s*\('),
            'CRITICAL',
            'system() with sprintf() — command injection risk'
        ),
    }
    
    # Pattern 2: Path Traversal
    PATH_TRAVERSAL_PATTERNS = {
        'fopen_user_path': (
            re.compile(r'fopen\s*\(\s*(?:user_|input|arg|argv|param|path|filename)', re.IGNORECASE),
            'CRITICAL',
            'User-controlled file path in fopen() — path traversal risk'
        ),
        'ifstream_user_path': (
            re.compile(r'std::ifstream\s*\(\s*(?:user_|input|arg|argv|param|path|filename)', re.IGNORECASE),
            'CRITICAL',
            'User-controlled file path in std::ifstream — path traversal risk'
        ),
        'ofstream_user_path': (
            re.compile(r'std::ofstream\s*\(\s*(?:user_|input|arg|argv|param|path|filename)', re.IGNORECASE),
            'CRITICAL',
            'User-controlled file path in std::ofstream — path traversal risk'
        ),
        'mkdir_user_path': (
            re.compile(r'(?:mkdir|_mkdir|CreateDirectory)\s*\(\s*(?:user_|input|arg|argv|param|path|dir)', re.IGNORECASE),
            'HIGH',
            'User-controlled directory path in mkdir() — path traversal risk'
        ),
        'chdir_user_path': (
            re.compile(r'chdir\s*\(\s*(?:user_|input|arg|argv|param|path|dir)', re.IGNORECASE),
            'HIGH',
            'User-controlled directory path in chdir() — directory traversal risk'
        ),
        'access_user_path': (
            re.compile(r'(?:access|_access|GetFileAttributes)\s*\(\s*(?:user_|input|arg|argv|param|path|file)', re.IGNORECASE),
            'HIGH',
            'User-controlled file path in access() — path traversal risk'
        ),
        'parent_dir_pattern': (
            re.compile(r'(?:strstr|strpos|find)\s*\([^)]*["\'][\.][\.][/\\]'),
            'HIGH',
            'Path traversal pattern (..) detected — validate and sanitize paths'
        ),
    }
    
    # Pattern 3: LDAP Injection
    LDAP_INJECTION_PATTERNS = {
        'ldap_search_concat': (
            re.compile(r'ldap_search\s*\([^)]*\+'),
            'CRITICAL',
            'String concatenation in ldap_search() — LDAP injection risk'
        ),
        'ldap_filter_user': (
            re.compile(r'ldap_search\s*\([^)]*(?:user_|input|arg|filter_user|filter_data)', re.IGNORECASE),
            'CRITICAL',
            'User input in LDAP filter — LDAP injection vulnerability'
        ),
        'ldap_bind_concat': (
            re.compile(r'ldap_bind\s*\([^)]*\+'),
            'HIGH',
            'String concatenation in ldap_bind() — potential LDAP injection'
        ),
    }
    
    # Pattern 4: XXE (XML External Entity) Injection
    XXE_INJECTION_PATTERNS = {
        'xml_parse_user': (
            re.compile(r'(?:libxml2|libxml|XML|xmlParse|xmlRead|tinyxml|rapidxml)\s*[^)]*(?:user_|input|arg|data|file|xml)'),
            'CRITICAL',
            'XML parsing with user input — XXE injection risk if not properly configured'
        ),
        'xpath_concat': (
            re.compile(r'xpath\s*\([^)]*\+'),
            'HIGH',
            'String concatenation in XPath query — XPath injection risk'
        ),
        'dtd_enabled': (
            re.compile(r'(?:LIBXML_DTDLOAD|LIBXML_DTDVALID|allow_external|external_entity)\s*[=:]\s*(?:true|True|TRUE|1|yes)', re.IGNORECASE),
            'CRITICAL',
            'External DTD/entities enabled in XML parser — XXE risk'
        ),
    }
    
    # Pattern 5: Format String Vulnerabilities
    FORMAT_STRING_PATTERNS = {
        'sprintf_format_user': (
            re.compile(r'sprintf\s*\(\s*[^,]+,\s*(?:user_|input|arg|argv|param|format|fmt|fmt_str)', re.IGNORECASE),
            'CRITICAL',
            'User input as format string in sprintf() — format string vulnerability'
        ),
        'printf_format_user': (
            re.compile(r'printf\s*\(\s*(?:user_|input|arg|argv|param|format|fmt|fmt_str)', re.IGNORECASE),
            'CRITICAL',
            'User input as format string in printf() — information disclosure risk'
        ),
        'fprintf_format_user': (
            re.compile(r'fprintf\s*\([^,]+,\s*(?:user_|input|arg|argv|param|format|fmt|fmt_str)', re.IGNORECASE),
            'CRITICAL',
            'User input as format string in fprintf() — information disclosure risk'
        ),
        'syslog_format_user': (
            re.compile(r'(?:syslog|wlog|logger)\s*\([^,]*,\s*(?:user_|input|arg|argv|param|message|msg)', re.IGNORECASE),
            'HIGH',
            'User input as format string to logging function — format string risk'
        ),
    }
    
    def __init__(self):
        self.gaps: List[S3InjectionGap] = []
    
    def scan_file(self, file_path: str) -> List[S3InjectionGap]:
        """Scan a single file for S-3 injection attacks"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return gaps
        
        # Scan for command injection
        gaps.extend(self._scan_patterns(
            lines, file_path, self.COMMAND_INJECTION_PATTERNS, S3InjectionGapType.COMMAND_INJECTION
        ))
        
        # Scan for path traversal
        gaps.extend(self._scan_patterns(
            lines, file_path, self.PATH_TRAVERSAL_PATTERNS, S3InjectionGapType.PATH_TRAVERSAL
        ))
        
        # Scan for LDAP injection
        gaps.extend(self._scan_patterns(
            lines, file_path, self.LDAP_INJECTION_PATTERNS, S3InjectionGapType.LDAP_INJECTION
        ))
        
        # Scan for XXE injection
        gaps.extend(self._scan_patterns(
            lines, file_path, self.XXE_INJECTION_PATTERNS, S3InjectionGapType.XXE_INJECTION
        ))
        
        # Scan for format string
        gaps.extend(self._scan_patterns(
            lines, file_path, self.FORMAT_STRING_PATTERNS, S3InjectionGapType.FORMAT_STRING
        ))
        
        return gaps
    
    def _scan_patterns(self, lines: List[str], file_path: str,
                       patterns: Dict, gap_type: S3InjectionGapType) -> List[S3InjectionGap]:
        """Scan for a set of patterns and return gaps"""
        gaps = []
        
        for pattern_name, (pattern_re, severity, description) in patterns.items():
            for line_num, line in enumerate(lines, 1):
                if pattern_re.search(line):
                    snippet = line.strip()[:100]
                    gap = S3InjectionGap(
                        file_path=file_path,
                        line_num=line_num,
                        gap_type=gap_type,
                        snippet=snippet,
                        severity=severity,
                        description=description,
                        remediation='Validate and sanitize all user input, use parameterized APIs'
                    )
                    gaps.append(gap)
        
        return gaps


def main():
    """Main entry point for scanner"""
    import sys
    
    scanner = S3InjectionScanner()
    all_gaps = []
    
    # Scan .cpp and .h files
    for ext in ['**/*.cpp', '**/*.h', '**/*.hpp']:
        for file_path in Path('.').glob(ext):
            if any(skip in str(file_path) for skip in ['test', 'build', '.git', 'external']):
                continue
            gaps = scanner.scan_file(str(file_path))
            all_gaps.extend(gaps)
    
    # Output results
    results = {
        'enhancement': 'S-3',
        'cwe': 'CWE-94',
        'title': 'Injection Attack Prevention',
        'total_gaps': len(all_gaps),
        'gaps': [gap.to_dict() for gap in all_gaps]
    }
    
    print(json.dumps(results, indent=2))
    return len(all_gaps)


if __name__ == '__main__':
    main()
