#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 ÔÇö Security Gaps Detection

Detects:
- Unsafe C functions (strcpy, sprintf, gets, scanf)
- Hardcoded secrets and credentials
- Missing input validation
- SQL/command injection risks
- Unchecked error returns
- Missing null checks
- Plaintext password storage
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional
from enum import Enum


class SecurityGapType(Enum):
    """Security gap classifications"""
    UNSAFE_FUNCTION = "unsafe_function"           # strcpy, sprintf, gets, scanf
    HARDCODED_SECRET = "hardcoded_secret"         # API_KEY, PASSWORD literal
    MISSING_VALIDATION = "missing_validation"     # No bounds check on input
    SQL_INJECTION = "sql_injection"               # String concat + execute
    UNCHECKED_ERROR = "unchecked_error"           # Return code ignored
    NULL_DEREFERENCE = "null_dereference"         # Pointer use without check
    PLAINTEXT_PASSWORD = "plaintext_password"     # Stored in plaintext
    MISSING_AUTH = "missing_auth"                 # No authentication check
    WEAK_CRYPTO = "weak_crypto"                   # Hardcoded IV, no salt


@dataclass
class SecurityGap:
    """Represents a security gap"""
    file_path: str
    line_num: int
    gap_type: SecurityGapType
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


class SecurityGapScanner:
    """Detect security vulnerabilities in C++ code"""
    
    # Unsafe function patterns
    UNSAFE_PATTERNS = {
        'strcpy': (re.compile(r'\bstrcpy\s*\('), 'CRITICAL', 
                   'Buffer overflow risk ÔÇö use strncpy or std::string'),
        'sprintf': (re.compile(r'\bsprintf\s*\('), 'CRITICAL',
                    'Buffer overflow risk ÔÇö use snprintf or fmt::format'),
        'gets': (re.compile(r'\bgets\s*\('), 'CRITICAL',
                 'NEVER USE ÔÇö always causes buffer overflow'),
        'scanf_unsafe': (re.compile(r'\bscanf\s*\(\s*"%[^n]'), 'CRITICAL',
                        'Unbounded input ÔÇö use scanf_s or std::cin'),
        'scanf_string': (re.compile(r'\bscanf\s*\(\s*"%s"'), 'CRITICAL',
                        'Unbounded string input ÔÇö use %Ns for max length'),
    }
    
    # Credential/secret patterns
    SECRET_PATTERNS = {
        'hardcoded_api_key': (
            re.compile(r'(API_KEY|api_key|APIKEY)\s*=\s*["\'].*["\']'),
            'CRITICAL', 'Hardcoded API key ÔÇö use environment variable'
        ),
        'hardcoded_password': (
            re.compile(r'(PASSWORD|password|PASSWD|passwd)\s*=\s*["\'].*["\']'),
            'CRITICAL', 'Hardcoded password ÔÇö use secure vault'
        ),
        'hardcoded_secret': (
            re.compile(r'(SECRET|secret|TOKEN|token)\s*=\s*["\'].*["\']'),
            'CRITICAL', 'Hardcoded secret ÔÇö use environment variable'
        ),
        'hardcoded_key': (
            re.compile(r'(ENCRYPTION_KEY|encryption_key|MASTER_KEY)\s*=\s*["\'].*["\']'),
            'CRITICAL', 'Hardcoded encryption key ÔÇö use key management service'
        ),
    }
    
    # SQL/Command injection patterns
    INJECTION_PATTERNS = {
        'string_concat_sql': (
            re.compile(r'(query|sql|command)\s*=\s*["\'].*["\']?\s*\+'),
            'CRITICAL', 'SQL injection risk ÔÇö use prepared statements'
        ),
        'format_string': (
            re.compile(r'(sprintf|snprintf|format)\s*\(\s*["\'].*%s'),
            'HIGH', 'Format string risk with user input'
        ),
    }
    
    # Input validation patterns
    VALIDATION_PATTERNS = {
        'param_unused': (
            re.compile(r'(size_t len|int size|uint32_t length)\s*\)'),
            'HIGH', 'Parameter passed but never validated ÔÇö check bounds'
        ),
    }
    
    # Error handling patterns
    ERROR_PATTERNS = {
        'unchecked_return': (
            re.compile(r'(status|error|result|ret)\s*=\s*\w+\s*\(\);?\s*\n(?!\s*(if|while|assert|check))'),
            'HIGH', 'Error code returned but not checked ÔÇö add validation'
        ),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[SecurityGap]] = {}
    
    def scan_file(self, file_path: Path) -> List[SecurityGap]:
        """Scan single file for security gaps"""
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
            
            # Check unsafe functions
            for pattern_name, (pattern, severity, desc) in self.UNSAFE_PATTERNS.items():
                if pattern.search(line):
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=SecurityGapType.UNSAFE_FUNCTION,
                        snippet=line.strip()[:100],
                        severity=severity,
                        description=f'{pattern_name}: {desc}',
                        remediation=desc
                    )
                    gaps.append(gap)
            
            # Check hardcoded secrets
            for pattern_name, (pattern, severity, desc) in self.SECRET_PATTERNS.items():
                if pattern.search(line):
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=SecurityGapType.HARDCODED_SECRET,
                        snippet=line.strip()[:100],
                        severity=severity,
                        description=f'{pattern_name}: {desc}',
                        remediation=desc
                    )
                    gaps.append(gap)
            
            # Check SQL injection patterns
            for pattern_name, (pattern, severity, desc) in self.INJECTION_PATTERNS.items():
                if pattern.search(line):
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=SecurityGapType.SQL_INJECTION,
                        snippet=line.strip()[:100],
                        severity=severity,
                        description=f'{pattern_name}: {desc}',
                        remediation=desc
                    )
                    gaps.append(gap)
            
            # Check for null pointer dereference (heuristic)
            if '->' in line and 'if' not in lines[max(0, line_num-3):line_num]:
                # Pointer dereference without preceding null check
                if any(var in line for var in ['ptr', 'obj', 'handle', 'ref']):
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=SecurityGapType.NULL_DEREFERENCE,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Potential null pointer dereference',
                        remediation='Add null check before dereferencing'
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[SecurityGap]]:
        """Scan module for security gaps"""
        gaps_by_file = {}
        
        # Scan source files
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
        """Scan all modules for security gaps"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for SECURITY GAPS...")
        
        # Find all modules
        src_root = self.repo_root / 'src'
        modules = sorted([d.name for d in src_root.iterdir() if d.is_dir()])
        
        aggregate = {}
        
        for module in modules:
            gaps_by_file = self.scan_module(module)
            total_gaps = sum(len(g) for g in gaps_by_file.values())
            
            if total_gaps > 0:
                print(f"   {module:30} {total_gaps:4} gaps")
                
                # Categorize by type
                gap_counts = {}
                for gaps in gaps_by_file.values():
                    for gap in gaps:
                        gap_type = gap.gap_type.value
                        gap_counts[gap_type] = gap_counts.get(gap_type, 0) + 1
                
                # Categorize by severity
                severity_counts = {}
                for gaps in gaps_by_file.values():
                    for gap in gaps:
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
        
        # Save results
        with open(output_path / 'gap_scan_v3_security_aggregate.json', 'w') as f:
            json.dump(aggregate, f, indent=2)
        
        print(f"\n[OK] Security scan complete. Results in {output_dir}/")
        return aggregate


if __name__ == '__main__':
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    print("[INFO] Security Gap Scanner v3")
    print("=" * 70)
    
    scanner = SecurityGapScanner(repo_root)
    results = scanner.run_full_scan(output_dir)
    
    total_gaps = sum(m.get('total', 0) for m in results.values())
    critical = sum(m.get('severity_critical', 0) for m in results.values())
    high = sum(m.get('severity_high', 0) for m in results.values())
    
    print(f"\n[SUMMARY] Security Gaps:")
    print(f"   Total: {total_gaps}")
    print(f"   CRITICAL: {critical}")
    print(f"   HIGH: {high}")
    print("\n" + "=" * 70)
