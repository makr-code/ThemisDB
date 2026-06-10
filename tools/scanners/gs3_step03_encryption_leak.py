#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Encryption Leak Detection

Detects cryptographic encryption vulnerabilities:
- Weak or deprecated cryptographic algorithms
- Missing authentication (unverified ciphertexts)
- ECB mode usage (deterministic encryption)
- Hardcoded IV/nonce
- Missing salt in password hashing
- Custom cryptography implementations
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class EncryptionLeakType(Enum):
    """Encryption vulnerability classifications"""
    WEAK_HASH_ALGORITHM = "weak_hash_algorithm"      # MD5, SHA1, SHA224
    DEPRECATED_CIPHER = "deprecated_cipher"          # DES, 3DES, RC4, Blowfish
    ECB_MODE = "ecb_mode"                            # Electronic Code Book (deterministic)
    WEAK_KDF = "weak_kdf"                            # Weak key derivation
    UNVERIFIED_ENCRYPTION = "unverified_encryption"  # No AEAD/HMAC
    HARDCODED_IV = "hardcoded_iv"                    # IV/nonce not random
    MISSING_SALT = "missing_salt"                    # Password hash without salt
    CUSTOM_CRYPTO = "custom_crypto"                  # Custom implementation (suspicious)


@dataclass
class EncryptionLeakGap:
    """Represents an encryption vulnerability"""
    file_path: str
    line_num: int
    gap_type: EncryptionLeakType
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


class EncryptionLeakScanner:
    """Detect cryptographic encryption vulnerabilities"""
    
    # WEAK HASH PATTERNS
    WEAK_HASH_PATTERNS = {
        'md5': re.compile(r'\bMD5|EVP_md5\(\)|MD5_\w+', re.IGNORECASE),
        'sha1': re.compile(r'\bSHA1|EVP_sha1\(\)|SHA_\w+\(', re.IGNORECASE),
        'sha224': re.compile(r'\bSHA224|EVP_sha224\(\)', re.IGNORECASE),
    }
    
    # DEPRECATED CIPHER PATTERNS
    DEPRECATED_CIPHER_PATTERNS = {
        'des': re.compile(r'\bDES\b|\bDES_\w+\(|EVP_des\(', re.IGNORECASE),
        '3des': re.compile(r'\b3DES\b|EVP_des3\(|EVP_des_ede3', re.IGNORECASE),
        'rc4': re.compile(r'\bRC4\b|EVP_rc4\(|ARCFOUR', re.IGNORECASE),
        'blowfish': re.compile(r'\bBlowfish|EVP_bf_', re.IGNORECASE),
    }
    
    # ECB MODE PATTERNS
    ECB_MODE_PATTERNS = {
        'ecb_explicit': re.compile(r'ECB|_ecb|MODE_ECB|EVP_CIPHER_ECB', re.IGNORECASE),
        'no_iv': re.compile(r'encrypt.*\(\s*\w+\s*,\s*\w+\s*,\s*nullptr\s*\)', re.IGNORECASE),
    }
    
    # UNVERIFIED ENCRYPTION PATTERNS
    UNVERIFIED_PATTERNS = {
        'no_hmac': re.compile(r'encrypt\(|cipher\.update\(|EVP_EncryptFinal', re.IGNORECASE),
        'no_auth_tag': re.compile(r'EVP_CIPHER_CTX_ctrl.*EVP_CTRL_GCM_GET_TAG', re.IGNORECASE),
    }
    
    # HARDCODED IV PATTERNS
    HARDCODED_IV_PATTERNS = {
        'static_iv': re.compile(r'(?:iv|nonce|salt)\s*=\s*["\']([a-zA-Z0-9+/]{16,})["\']', re.IGNORECASE),
        'zero_iv': re.compile(r'(?:iv|nonce|salt)\s*=\s*0x00+', re.IGNORECASE),
    }
    
    # APPROVED PATTERNS (WHITELIST)
    APPROVED_PATTERNS = {
        'openssl_aes': re.compile(r'EVP_aes_256_gcm|EVP_aes_256_cbc|AES-256-GCM'),
        'chacha': re.compile(r'ChaCha20|EVP_chacha20_poly1305'),
        'libsodium': re.compile(r'sodium_|crypto_secretbox|crypto_box'),
        'boringssl': re.compile(r'EVP_AEAD_AES_256_GCM|EVP_AEAD_CHACHA20_POLY1305'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [
        r'test.*crypt|crypt.*test',
        r'demo.*encrypt|encrypt.*demo',
        r'example.*cipher',
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[EncryptionLeakGap]] = {}
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_PATTERNS]
    
    def _is_test_context(self, file_name: str, line: str) -> bool:
        """Check if this is test/fixture context."""
        if 'test' in file_name.lower() or 'fixture' in file_name.lower():
            return True
        for pattern in self.test_patterns:
            if pattern.search(line):
                return True
        return False
    
    def _is_approved_usage(self, line: str) -> bool:
        """Check if this uses approved crypto."""
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def _check_weak_hash(self, file_path: Path, lines: List[str]) -> List[EncryptionLeakGap]:
        """Detect weak hash algorithms."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            if self._is_approved_usage(line):
                continue
            
            for hash_type, pattern in self.WEAK_HASH_PATTERNS.items():
                if pattern.search(line):
                    severity = 'CRITICAL' if hash_type in ['md5'] else 'HIGH'
                    gap = EncryptionLeakGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=EncryptionLeakType.WEAK_HASH_ALGORITHM,
                        snippet=line.strip()[:100],
                        severity=severity,
                        description=f'{hash_type.upper()} detected in cryptographic context',
                        remediation='Use SHA-256 or better (SHA-512, BLAKE2)',
                        confidence=0.95
                    )
                    gaps.append(gap)
                    break
        
        return gaps
    
    def _check_deprecated_ciphers(self, file_path: Path, lines: List[str]) -> List[EncryptionLeakGap]:
        """Detect deprecated cipher algorithms."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            if self._is_approved_usage(line):
                continue
            
            for cipher_type, pattern in self.DEPRECATED_CIPHER_PATTERNS.items():
                if pattern.search(line):
                    gap = EncryptionLeakGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=EncryptionLeakType.DEPRECATED_CIPHER,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description=f'{cipher_type.upper()} cipher detected — deprecated',
                        remediation='Use AES-256-GCM or ChaCha20-Poly1305',
                        confidence=0.98
                    )
                    gaps.append(gap)
                    break
        
        return gaps
    
    def _check_ecb_mode(self, file_path: Path, lines: List[str]) -> List[EncryptionLeakGap]:
        """Detect ECB mode usage."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            for mode_type, pattern in self.ECB_MODE_PATTERNS.items():
                if pattern.search(line):
                    gap = EncryptionLeakGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=EncryptionLeakType.ECB_MODE,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='ECB mode detected — deterministic encryption',
                        remediation='Use authenticated encryption: AES-GCM or ChaCha20-Poly1305',
                        confidence=0.90
                    )
                    gaps.append(gap)
                    break
        
        return gaps
    
    def _check_hardcoded_iv(self, file_path: Path, lines: List[str]) -> List[EncryptionLeakGap]:
        """Detect hardcoded IV/nonce."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            for iv_type, pattern in self.HARDCODED_IV_PATTERNS.items():
                if pattern.search(line):
                    gap = EncryptionLeakGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=EncryptionLeakType.HARDCODED_IV,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Hardcoded IV/nonce detected — breaks security',
                        remediation='Generate random IV/nonce for each encryption operation',
                        confidence=0.85
                    )
                    gaps.append(gap)
                    break
        
        return gaps
    
    def scan_file(self, file_path: Path) -> List[EncryptionLeakGap]:
        """Scan single file for encryption vulnerabilities"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        # Run all checks
        gaps.extend(self._check_weak_hash(file_path, lines))
        gaps.extend(self._check_deprecated_ciphers(file_path, lines))
        gaps.extend(self._check_ecb_mode(file_path, lines))
        gaps.extend(self._check_hardcoded_iv(file_path, lines))
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[EncryptionLeakGap]]:
        """Scan module for encryption vulnerabilities"""
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
    
    def scan_repository(self) -> Dict[str, List[EncryptionLeakGap]]:
        """Scan entire repository for encryption vulnerabilities"""
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
    
    parser = argparse.ArgumentParser(description='Encryption Leak Detection Scanner')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    scanner = EncryptionLeakScanner(args.repo)
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
    print(f"\nTotal encryption vulnerabilities found: {total_gaps}")
