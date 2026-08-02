#!/usr/bin/env python3
"""
Phase 1-4 Enhancement: S-1 Hardcoded Secrets Detection (Enhanced)

CWE-798: Use of Hard-Coded Credentials
Extends existing gs3_step01_core_security.py with additional secret patterns.

Enhanced Patterns:
1. API Token Hardcoding (sk_live_*, pk_*, api_key literals)
2. SSH Key Embedded (BEGIN RSA PRIVATE KEY, BEGIN OPENSSH PRIVATE KEY)
3. Database Credentials Hardcoded (mysql_connect, postgres connection strings)
4. Certificate/Key Material (BEGIN CERTIFICATE, BEGIN PRIVATE KEY blocks)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional
from enum import Enum


class S1SecretGapType(Enum):
    """S-1 Enhanced secret detection categories"""
    API_TOKEN_HARDCODED = "api_token_hardcoded"
    SSH_KEY_EMBEDDED = "ssh_key_embedded"
    DB_CREDENTIALS_HARDCODED = "db_credentials_hardcoded"
    CERTIFICATE_EMBEDDED = "certificate_embedded"
    GENERIC_SECRET_LITERAL = "generic_secret_literal"


@dataclass
class S1SecretGap:
    """Represents an S-1 hardcoded secret gap"""
    file_path: str
    line_num: int
    gap_type: S1SecretGapType
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
            'enhancement': 'S-1',
            'cwe': 'CWE-798',
        }


class S1SecretScanner:
    """S-1 Enhanced: Hardcoded Secrets Detection"""
    
    # Pattern 1: API Token Hardcoding
    API_TOKEN_PATTERNS = {
        'stripe_live_key': (
            re.compile(r'(?:STRIPE_API_KEY|stripe_api_key|pk_live_[a-zA-Z0-9_]{20,}|sk_live_[a-zA-Z0-9_]{20,})', re.IGNORECASE),
            'CRITICAL',
            'Stripe API key hardcoded in code'
        ),
        'aws_secret_key': (
            re.compile(r'(?:AWS_SECRET_ACCESS_KEY|aws_secret_key|AKIA[0-9A-Z]{16})\s*[=:]\s*["\']', re.IGNORECASE),
            'CRITICAL',
            'AWS secret access key hardcoded in code'
        ),
        'github_token': (
            re.compile(r'(?:GITHUB_TOKEN|github_token)\s*[=:]\s*["\']ghp_[A-Za-z0-9_]{36,}["\']', re.IGNORECASE),
            'CRITICAL',
            'GitHub personal access token hardcoded in code'
        ),
        'generic_api_key': (
            re.compile(r'(?:API_KEY|api_key|API_SECRET|api_secret)\s*[=:]\s*["\'][a-zA-Z0-9\-_.]{20,}["\']', re.IGNORECASE),
            'CRITICAL',
            'Generic API key hardcoded in code'
        ),
        'bearer_token': (
            re.compile(r'(?:Bearer|BEARER)\s+[A-Za-z0-9\-_.=]{40,}', re.IGNORECASE),
            'CRITICAL',
            '****** hardcoded in code or hardcoded in string literals'
        ),
    }
    
    # Pattern 2: SSH Key Embedded
    SSH_KEY_PATTERNS = {
        'openssh_private_key': (
            re.compile(r'-----BEGIN\s+OPENSSH\s+PRIVATE\s+KEY-----', re.MULTILINE),
            'CRITICAL',
            'OpenSSH private key embedded in source code'
        ),
        'rsa_private_key': (
            re.compile(r'-----BEGIN\s+RSA\s+PRIVATE\s+KEY-----', re.MULTILINE),
            'CRITICAL',
            'RSA private key embedded in source code'
        ),
        'ecdsa_private_key': (
            re.compile(r'-----BEGIN\s+EC(?:DSA)?\s+PRIVATE\s+KEY-----', re.MULTILINE),
            'CRITICAL',
            'ECDSA private key embedded in source code'
        ),
        'dsa_private_key': (
            re.compile(r'-----BEGIN\s+DSA\s+PRIVATE\s+KEY-----', re.MULTILINE),
            'CRITICAL',
            'DSA private key embedded in source code'
        ),
        'pgp_private_key': (
            re.compile(r'-----BEGIN\s+PGP\s+PRIVATE\s+KEY\s+BLOCK-----', re.MULTILINE),
            'CRITICAL',
            'PGP private key embedded in source code'
        ),
    }
    
    # Pattern 3: Database Credentials Hardcoded
    DB_CRED_PATTERNS = {
        'mysql_connection_creds': (
            re.compile(r'(?:mysql_connect|mysqli_connect|new\s+MySQLi)\s*\(\s*["\'].*["\'],\s*["\'](?:root|admin|user)["\'],\s*["\'][^"\']*["\']', re.IGNORECASE),
            'CRITICAL',
            'MySQL database credentials hardcoded in function call'
        ),
        'postgres_connection_string': (
            re.compile(r'(?:postgresql://|postgres://)\s*[^:]+:[^@]+@', re.IGNORECASE),
            'CRITICAL',
            'PostgreSQL connection string with embedded credentials'
        ),
        'mongodb_connection_string': (
            re.compile(r'mongodb(?:\+srv)?://\s*[^:]+:[^@]+@', re.IGNORECASE),
            'CRITICAL',
            'MongoDB connection string with embedded credentials'
        ),
        'connection_string_password': (
            re.compile(r'(?:connection_string|ConnectionString|DbConnection|connStr)\s*[=:]\s*["\'].*(?:Password|password|pwd)=[^"\']*["\']', re.IGNORECASE),
            'CRITICAL',
            'Database connection string with embedded password'
        ),
        'oracle_credentials': (
            re.compile(r'(?:OCILogon|ocilogon)\s*\(\s*["\'][^"\']*["\'],\s*["\'][^"\']*["\'],\s*["\'][^"\']*["\']', re.IGNORECASE),
            'CRITICAL',
            'Oracle database credentials hardcoded in function call'
        ),
    }
    
    # Pattern 4: Certificate/Key Material
    CERT_PATTERNS = {
        'x509_certificate': (
            re.compile(r'-----BEGIN\s+CERTIFICATE-----', re.MULTILINE),
            'HIGH',
            'X.509 certificate embedded in source code'
        ),
        'certificate_request': (
            re.compile(r'-----BEGIN\s+CERTIFICATE\s+REQUEST-----', re.MULTILINE),
            'HIGH',
            'Certificate signing request embedded in source code'
        ),
        'public_key_pkcs': (
            re.compile(r'-----BEGIN\s+PUBLIC\s+KEY-----', re.MULTILINE),
            'HIGH',
            'Public key embedded in source code (potential info leak)'
        ),
    }
    
    def __init__(self):
        self.gaps: List[S1SecretGap] = []
    
    def scan_file(self, file_path: str) -> List[S1SecretGap]:
        """Scan a single file for S-1 hardcoded secrets"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return gaps
        
        # Scan for API tokens
        gaps.extend(self._scan_patterns(
            lines, file_path, self.API_TOKEN_PATTERNS, S1SecretGapType.API_TOKEN_HARDCODED
        ))
        
        # Scan for SSH keys
        gaps.extend(self._scan_patterns(
            lines, file_path, self.SSH_KEY_PATTERNS, S1SecretGapType.SSH_KEY_EMBEDDED
        ))
        
        # Scan for database credentials
        gaps.extend(self._scan_patterns(
            lines, file_path, self.DB_CRED_PATTERNS, S1SecretGapType.DB_CREDENTIALS_HARDCODED
        ))
        
        # Scan for certificates
        gaps.extend(self._scan_patterns(
            lines, file_path, self.CERT_PATTERNS, S1SecretGapType.CERTIFICATE_EMBEDDED
        ))
        
        return gaps
    
    def _scan_patterns(self, lines: List[str], file_path: str, 
                       patterns: Dict, gap_type: S1SecretGapType) -> List[S1SecretGap]:
        """Scan for a set of patterns and return gaps"""
        gaps = []
        
        for pattern_name, (pattern_re, severity, description) in patterns.items():
            for line_num, line in enumerate(lines, 1):
                if pattern_re.search(line):
                    snippet = line.strip()[:100]
                    gap = S1SecretGap(
                        file_path=file_path,
                        line_num=line_num,
                        gap_type=gap_type,
                        snippet=snippet,
                        severity=severity,
                        description=description,
                        remediation='Move credentials to environment variables, .env files, or secure vault'
                    )
                    gaps.append(gap)
        
        return gaps


def main():
    """Main entry point for scanner"""
    import sys
    
    scanner = S1SecretScanner()
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
        'enhancement': 'S-1',
        'cwe': 'CWE-798',
        'title': 'Hardcoded Secrets Detection',
        'total_gaps': len(all_gaps),
        'gaps': [gap.to_dict() for gap in all_gaps]
    }
    
    print(json.dumps(results, indent=2))
    return len(all_gaps)


if __name__ == '__main__':
    main()
