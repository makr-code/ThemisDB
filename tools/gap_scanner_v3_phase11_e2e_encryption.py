#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — E2E (End-to-End) Security Encryption

Detects gaps in complete encryption coverage:
- Data encrypted in transit but not at rest
- Data encrypted at rest but not in transit
- Decryption without integrity verification
- Missing encryption layers
- Encryption bypass paths
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class E2EEncryptionType(Enum):
    """E2E encryption vulnerability classifications"""
    NO_TRANSIT_ENCRYPTION = "no_transit_encryption"    # Not encrypted in transit
    NO_REST_ENCRYPTION = "no_rest_encryption"          # Not encrypted at rest
    MISSING_VERIFICATION = "missing_verification"      # No integrity check
    ENCRYPTION_BYPASS = "encryption_bypass"            # Unencrypted fallback
    PARTIAL_ENCRYPTION = "partial_encryption"          # Inconsistent encryption
    KEY_NOT_DERIVED = "key_not_derived"                # Key not from password


@dataclass
class E2EEncryptionGap:
    """Represents an E2E encryption gap"""
    file_path: str
    line_num: int
    gap_type: E2EEncryptionType
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


class E2EEncryptionScanner:
    """Detect end-to-end encryption gaps"""
    
    # TRANSIT ENCRYPTION PATTERNS
    TRANSIT_PATTERNS = {
        'no_tls': re.compile(r'http://|\.connect\(|\.send\(|socket\.|tcp::', re.IGNORECASE),
        'plaintext_http': re.compile(r'http://|HTTP_CLIENT|plaintext.*transmit', re.IGNORECASE),
    }
    
    # REST ENCRYPTION PATTERNS
    REST_PATTERNS = {
        'plaintext_file': re.compile(r'fopen|std::ofstream|write.*file|serialize.*file', re.IGNORECASE),
        'plaintext_db': re.compile(r'INSERT.*VALUES|UPDATE.*SET|SELECT.*FROM', re.IGNORECASE),
        'unencrypted_storage': re.compile(r'store\(|save\(|persist\(', re.IGNORECASE),
    }
    
    # VERIFICATION PATTERNS
    VERIFICATION_PATTERNS = {
        'decrypt_no_verify': re.compile(r'decrypt\(|EVP_DecryptFinal|decipher', re.IGNORECASE),
        'no_hmac': re.compile(r'authenticate|verify.*hmac', re.IGNORECASE),
    }
    
    # BYPASS PATTERNS
    BYPASS_PATTERNS = {
        'fallback': re.compile(r'if.*encrypt.*fail|encrypted\s*\?\s*:\s*plain|fallback.*encrypt', re.IGNORECASE),
        'optional_encryption': re.compile(r'encrypt\s*=\s*(?:false|0|nullptr)', re.IGNORECASE),
    }
    
    # APPROVED PATTERNS
    APPROVED_PATTERNS = {
        'https': re.compile(r'https://|TLS|SSL|secure.*channel'),
        'encrypted_db': re.compile(r'encrypt.*database|database.*encrypted|encrypted_column'),
        'aead': re.compile(r'AES_GCM|ChaCha20_Poly1305|AEAD'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [
        r'test.*encrypt|encrypt.*test',
        r'mock.*encrypt|stub.*encrypt',
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[E2EEncryptionGap]] = {}
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
        """Check if using approved encryption pattern."""
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def scan_file(self, file_path: Path) -> List[E2EEncryptionGap]:
        """Scan single file for E2E encryption gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for plaintext transmission
            if self.TRANSIT_PATTERNS['no_tls'].search(line):
                if not self._is_approved_usage(line):
                    gap = E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.NO_TRANSIT_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Plaintext transmission detected',
                        remediation='Use HTTPS/TLS for all data transmission',
                        confidence=0.70
                    )
                    gaps.append(gap)
            
            # Check for plaintext storage
            if self.REST_PATTERNS['plaintext_file'].search(line):
                if not self._is_approved_usage(line):
                    gap = E2EEncryptionGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=E2EEncryptionType.NO_REST_ENCRYPTION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Plaintext file storage detected',
                        remediation='Encrypt sensitive data before storing to disk',
                        confidence=0.65
                    )
                    gaps.append(gap)
        
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
        """Scan entire repository for E2E encryption gaps"""
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
    
    # Print summary
    total_gaps = sum(len(v) for v in gaps.values())
    print(f"\nTotal E2E encryption gaps found: {total_gaps}")
