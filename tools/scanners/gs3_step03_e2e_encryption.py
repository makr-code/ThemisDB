#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — E2E Security Encryption (P11-3)

Verifies encryption in transit (HTTPS/TLS) and at rest (file/database):
- Detects plaintext transmission over HTTP
- Identifies unencrypted storage
- Checks for encryption bypass paths
- Validates key derivation
- Verifies authentication tags (MAC/HMAC)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class E2EEncryptionType(Enum):
    """E2E encryption gap classifications"""
    NO_TRANSIT_ENCRYPTION = "no_transit_encryption"    # HTTP instead of HTTPS
    NO_REST_ENCRYPTION = "no_rest_encryption"          # Plaintext storage
    UNENCRYPTED_LOG = "unencrypted_log"                # Sensitive data in logs
    ENCRYPTION_BYPASS = "encryption_bypass"            # Fallback to plaintext
    PARTIAL_ENCRYPTION = "partial_encryption"          # Inconsistent encryption
    UNVERIFIED_ENCRYPTION = "unverified_encryption"    # No MAC/HMAC


@dataclass
class E2EEncryptionGap:
    """Represents an E2E encryption gap"""
    file_path: str
    line_num: int
    gap_type: E2EEncryptionType
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


class E2EEncryptionScanner:
    """Verify E2E encryption in transit and at rest"""
    
    # TRANSIT ENCRYPTION PATTERNS
    TRANSIT_PATTERNS = {
        'plaintext_http': re.compile(r'http://|HTTP_|socket\(|connect\(.*80\)', re.IGNORECASE),
        'no_tls': re.compile(r'send\(.*socket|write\(.*socket|TCP.*plaintext', re.IGNORECASE),
        'weak_ssl': re.compile(r'SSLv2|SSLv3|TLSv1\(\)|verify.*false', re.IGNORECASE),
    }
    
    # STORAGE ENCRYPTION PATTERNS
    REST_PATTERNS = {
        'plaintext_file': re.compile(r'std::ofstream|fopen|write\(.*file|db.*plaintext', re.IGNORECASE),
        'plaintext_db': re.compile(r'INSERT.*SELECT|UPDATE.*SET|db\.query\(', re.IGNORECASE),
    }
    
    # ENCRYPTION BYPASS PATTERNS
    BYPASS_PATTERNS = {
        'fallback_plaintext': re.compile(r'if.*encrypt.*fail|except.*crypto|fallback.*plaintext', re.IGNORECASE),
        'conditional_encrypt': re.compile(r'if.*secure.*encrypt|if.*is_admin|if.*is_trusted', re.IGNORECASE),
    }
    
    # LOGGING PATTERNS (sensitive data)
    LOGGING_PATTERNS = {
        'log_sensitive': re.compile(r'log.*password|log.*key|log.*token|log.*secret', re.IGNORECASE),
        'printf_sensitive': re.compile(r'printf.*%s.*password|cout.*secret|log.*credit', re.IGNORECASE),
    }
    
    # APPROVED PATTERNS (WHITELIST)
    APPROVED_PATTERNS = {
        'tls_explicit': re.compile(r'HTTPS|TLS_|SSL_CONTEXT_TLS|tls://'),
        'encryption_framework': re.compile(r'EVP_encrypt|crypto_secretbox|sodium_|ChaCha20-Poly1305|AES-256-GCM'),
        'verified_encrypt': re.compile(r'HMAC|MAC|signature|EVP_CTRL_GCM_GET_TAG|auth_tag'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [r'test.*encrypt', r'mock.*tls', r'fixture.*crypto']
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[E2EEncryptionGap]] = {}
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_PATTERNS]
    
    def _is_test_context(self, file_name: str, line: str) -> bool:
        """Check if test/fixture context"""
        if 'test' in file_name.lower():
            return True
        for pattern in self.test_patterns:
            if pattern.search(line):
                return True
        return False
    
    def _is_approved_usage(self, line: str) -> bool:
        """Check if using approved encryption"""
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def _check_missing_verification(self, file_path: Path, lines: List[str]) -> List[E2EEncryptionGap]:
        """Detect encryption without integrity verification (MAC/signature)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if re.search(r'encrypt\(|EVP_EncryptFinal|cipher\.update', line, re.IGNORECASE):
                if not re.search(r'HMAC|MAC|signature|verify|auth_tag|EVP_CTRL_GCM_GET_TAG', line, re.IGNORECASE):
                    gap = E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.UNVERIFIED_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Encryption without integrity verification',
                        remediation='Add HMAC or AEAD (GCM/ChaCha20-Poly1305) for authentication',
                        confidence=0.60
                    )
                    gaps.append(gap)
        return gaps
    
    def _check_encryption_bypass(self, file_path: Path, lines: List[str]) -> List[E2EEncryptionGap]:
        """Detect fallback to unencrypted channels"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if re.search(r'if.*encrypt.*fail|except.*crypto|fallback.*plaintext', line, re.IGNORECASE):
                gap = E2EEncryptionGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=E2EEncryptionType.ENCRYPTION_BYPASS,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Potential fallback to unencrypted data transmission',
                    remediation='Fail securely - never fallback to plaintext; raise exception on crypto errors',
                    confidence=0.75
                )
                gaps.append(gap)
        return gaps
    
    def _check_partial_encryption(self, file_path: Path, lines: List[str]) -> List[E2EEncryptionGap]:
        """Detect inconsistent or partial encryption coverage"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if re.search(r'encrypt.*header|encrypt.*metadata', line, re.IGNORECASE):
                if not re.search(r'encrypt.*payload|encrypt.*all|encrypt.*data', line, re.IGNORECASE):
                    gap = E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.PARTIAL_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='MEDIUM',
                        description='Partial/inconsistent encryption',
                        remediation='Encrypt all sensitive fields consistently',
                        confidence=0.50
                    )
                    gaps.append(gap)
        return gaps
    
    def _check_key_derivation(self, file_path: Path, lines: List[str]) -> List[E2EEncryptionGap]:
        """Verify keys are properly derived, not hardcoded"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if re.search(r'key\s*=\s*["\'][a-zA-Z0-9+/]{32,}["\']|key\s*=\s*0x[a-f0-9]{32,}', line, re.IGNORECASE):
                if not re.search(r'PBKDF2|scrypt|Argon2|KDF|hash', line, re.IGNORECASE):
                    gap = E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.UNVERIFIED_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Hardcoded or non-derived encryption key',
                        remediation='Derive keys from passwords using PBKDF2/scrypt/Argon2',
                        confidence=0.85
                    )
                    gaps.append(gap)
        return gaps
    
    def scan_file(self, file_path: Path) -> List[E2EEncryptionGap]:
        """Scan single file for E2E encryption gaps"""
        gaps = []
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        # Check each line
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Transit encryption
            if self.TRANSIT_PATTERNS['plaintext_http'].search(line):
                if not self._is_approved_usage(line):
                    gaps.append(E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.NO_TRANSIT_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Plaintext transmission detected',
                        remediation='Use HTTPS/TLS for all data transmission',
                        confidence=0.70
                    ))
            
            # Storage encryption
            if self.REST_PATTERNS['plaintext_file'].search(line):
                if not self._is_approved_usage(line):
                    gaps.append(E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.NO_REST_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Plaintext file storage detected',
                        remediation='Encrypt sensitive data before storing to disk',
                        confidence=0.65
                    ))
        
        # Run all additional checks
        gaps.extend(self._check_missing_verification(file_path, lines))
        gaps.extend(self._check_encryption_bypass(file_path, lines))
        gaps.extend(self._check_partial_encryption(file_path, lines))
        gaps.extend(self._check_key_derivation(file_path, lines))
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[E2EEncryptionGap]]:
        """Scan module for E2E encryption gaps"""
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
    
    def scan_repository(self) -> Dict[str, List[E2EEncryptionGap]]:
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
    
    parser = argparse.ArgumentParser(description='E2E Encryption Scanner')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    scanner = E2EEncryptionScanner(args.repo)
    gaps = scanner.scan_repository()
    result = scanner.to_json()
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(result)
        print(f"Results written to {args.output}")
    else:
        print(result)
    
    total_gaps = sum(len(v) for v in gaps.values())
    print(f"\nTotal E2E encryption gaps found: {total_gaps}")
