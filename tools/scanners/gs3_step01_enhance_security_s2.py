#!/usr/bin/env python3
"""
Phase 1-4 Enhancement: S-2 Cryptographic Weaknesses Detection (Enhanced)

CWE-327: Use of a Broken or Risky Cryptographic Algorithm
Extends detection for weak crypto patterns beyond MD5/SHA1.

Enhanced Patterns:
1. Weak Hash Algorithms in Legacy Code (EVP_sha1, EVP_md5, MD2)
2. DES/3DES Cipher Usage (insecure block ciphers)
3. Fixed-Size XOR Encryption (custom weak implementations)
4. Weak Random Number Generators (rand() for crypto, std::random_device misuse)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional
from enum import Enum


class S2CryptoGapType(Enum):
    """S-2 Enhanced crypto weakness categories"""
    WEAK_HASH_ALGORITHM = "weak_hash_algorithm"
    WEAK_CIPHER_ALGORITHM = "weak_cipher_algorithm"
    XOR_ENCRYPTION = "xor_encryption"
    WEAK_RNG = "weak_rng"
    HARDCODED_IV = "hardcoded_iv"


@dataclass
class S2CryptoGap:
    """Represents an S-2 cryptographic weakness gap"""
    file_path: str
    line_num: int
    gap_type: S2CryptoGapType
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
            'enhancement': 'S-2',
            'cwe': 'CWE-327',
        }


class S2CryptoScanner:
    """S-2 Enhanced: Cryptographic Weaknesses Detection"""
    
    # Pattern 1: Weak Hash Algorithms
    WEAK_HASH_PATTERNS = {
        'evp_md5': (
            re.compile(r'EVP_md5\s*\('),
            'CRITICAL',
            'MD5 via OpenSSL EVP interface — use EVP_sha256 or EVP_sha3_256'
        ),
        'evp_sha1': (
            re.compile(r'EVP_sha1\s*\('),
            'CRITICAL',
            'SHA-1 via OpenSSL EVP interface — use EVP_sha256 or stronger'
        ),
        'md2_usage': (
            re.compile(r'(?:MD2_Init|EVP_md2|MD2)\s*\('),
            'CRITICAL',
            'MD2 hash (completely broken) — use SHA-256 or stronger'
        ),
        'sha0_usage': (
            re.compile(r'(?:SHA0|SHA_0)\s*\('),
            'CRITICAL',
            'SHA-0 (broken) — use SHA-256 or stronger'
        ),
        'ripemd_usage': (
            re.compile(r'(?:RIPEMD|ripemd)\s*\('),
            'HIGH',
            'RIPEMD hash (weakened) — use SHA-256 or stronger'
        ),
    }
    
    # Pattern 2: Weak Cipher Algorithms
    WEAK_CIPHER_PATTERNS = {
        'des_key_setup': (
            re.compile(r'DES_set_key\s*\('),
            'CRITICAL',
            'DES cipher (insecure, 56-bit key) — use AES-256'
        ),
        'evp_des_cbc': (
            re.compile(r'EVP_des_cbc\s*\('),
            'CRITICAL',
            'DES in CBC mode — use AES-256-CBC or AES-256-GCM'
        ),
        'evp_des_ecb': (
            re.compile(r'EVP_des_ecb\s*\('),
            'CRITICAL',
            'DES in ECB mode (doesn\'t hide patterns) — use AES-256-GCM'
        ),
        'evp_des3_cbc': (
            re.compile(r'EVP_des_ede3_cbc\s*\('),
            'HIGH',
            '3DES cipher (112-bit effective key) — use AES-256'
        ),
        'blowfish_cipher': (
            re.compile(r'(?:BF_|Blowfish|blowfish|EVP_bf)\s*\('),
            'HIGH',
            'Blowfish cipher (small block size, weak) — use AES-256'
        ),
        'rc4_cipher': (
            re.compile(r'(?:RC4|rc4|EVP_rc4)\s*\('),
            'CRITICAL',
            'RC4 cipher (broken stream cipher) — use ChaCha20 or AES-256-GCM'
        ),
        'rc2_cipher': (
            re.compile(r'(?:RC2|rc2|EVP_rc2)\s*\('),
            'CRITICAL',
            'RC2 cipher (weak block cipher) — use AES-256'
        ),
    }
    
    # Pattern 3: XOR-based Encryption
    XOR_ENCRYPTION_PATTERNS = {
        'xor_cipher_name': (
            re.compile(r'(?:xor_cipher|xor_encrypt|xor_crypt|XOR_CIPHER)\s*\(', re.IGNORECASE),
            'CRITICAL',
            'XOR-based encryption (cryptographically insecure) — use AES-256-GCM'
        ),
        'manual_xor_loop': (
            re.compile(r'for\s*\([^)]*\)\s*(?:{|[^{]*)\s*\w+\s*[\[\^]\s*=\s*(?:key|plaintext)', re.IGNORECASE | re.MULTILINE),
            'CRITICAL',
            'Manual XOR loop (likely weak encryption) — use crypto library'
        ),
        'xor_encrypt_string': (
            re.compile(r'(?:encryption|cipher|crypt|encrypt)\s*[=:]\s*["\'](?:xor|XOR)["\']', re.IGNORECASE),
            'CRITICAL',
            'XOR encryption explicitly selected — use standard crypto algorithm'
        ),
    }
    
    # Pattern 4: Weak Random Number Generators
    WEAK_RNG_PATTERNS = {
        'rand_crypto_usage': (
            re.compile(r'(?:rand\s*\(|srand\s*\()\s*[^)]*(?:crypto|key|nonce|IV|secret)', re.IGNORECASE | re.MULTILINE),
            'CRITICAL',
            'rand() used for cryptographic purpose — use std::random_device or OpenSSL RAND'
        ),
        'srand_unseeded': (
            re.compile(r'srand\s*\(\s*(?:time\s*\(|clock\s*\(|getpid\s*\()', re.IGNORECASE),
            'CRITICAL',
            'Weak seed for srand() — use cryptographically secure seeding'
        ),
        'random_device_direct': (
            re.compile(r'std::random_device\s*\(\)\s*\(\)', re.IGNORECASE),
            'HIGH',
            'std::random_device used directly (slow, may be predictable on some systems)'
        ),
        'time_based_seed': (
            re.compile(r'(?:srand|seed)\s*\(\s*(?:time\s*\(|clock\s*\(|timespec|std::chrono)', re.IGNORECASE),
            'HIGH',
            'Time-based seed (predictable) — use cryptographically secure random source'
        ),
        'getpid_seed': (
            re.compile(r'(?:srand|seed)\s*\(\s*(?:getpid|PID|process_id)', re.IGNORECASE),
            'HIGH',
            'Process ID used as seed (predictable on reboot) — use secure random source'
        ),
    }
    
    # Pattern 5: Hardcoded Initialization Vectors
    HARDCODED_IV_PATTERNS = {
        'iv_constant_string': (
            re.compile(r'(?:iv|IV|initialization_vector|init_vector)\s*[=:]\s*["\'][a-zA-Z0-9]{8,16}["\']', re.IGNORECASE),
            'CRITICAL',
            'Hardcoded initialization vector — use random IV for each encryption'
        ),
        'iv_zeros': (
            re.compile(r'(?:iv|IV)\s*[=:]\s*(?:nullptr|NULL|{0}|memset.*0)', re.IGNORECASE),
            'CRITICAL',
            'Initialization vector is all zeros — use random IV'
        ),
        'iv_constant_array': (
            re.compile(r'(?:unsigned\s+)?char\s+(?:iv|IV|initialization_vector)\s*\[\]\s*=\s*{', re.IGNORECASE),
            'CRITICAL',
            'Hardcoded IV in character array — use random IV'
        ),
    }
    
    def __init__(self):
        self.gaps: List[S2CryptoGap] = []
    
    def scan_file(self, file_path: str) -> List[S2CryptoGap]:
        """Scan a single file for S-2 crypto weaknesses"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return gaps
        
        # Scan for weak hash algorithms
        gaps.extend(self._scan_patterns(
            lines, file_path, self.WEAK_HASH_PATTERNS, S2CryptoGapType.WEAK_HASH_ALGORITHM
        ))
        
        # Scan for weak ciphers
        gaps.extend(self._scan_patterns(
            lines, file_path, self.WEAK_CIPHER_PATTERNS, S2CryptoGapType.WEAK_CIPHER_ALGORITHM
        ))
        
        # Scan for XOR encryption
        gaps.extend(self._scan_patterns(
            lines, file_path, self.XOR_ENCRYPTION_PATTERNS, S2CryptoGapType.XOR_ENCRYPTION
        ))
        
        # Scan for weak RNG
        gaps.extend(self._scan_patterns(
            lines, file_path, self.WEAK_RNG_PATTERNS, S2CryptoGapType.WEAK_RNG
        ))
        
        # Scan for hardcoded IVs
        gaps.extend(self._scan_patterns(
            lines, file_path, self.HARDCODED_IV_PATTERNS, S2CryptoGapType.HARDCODED_IV
        ))
        
        return gaps
    
    def _scan_patterns(self, lines: List[str], file_path: str,
                       patterns: Dict, gap_type: S2CryptoGapType) -> List[S2CryptoGap]:
        """Scan for a set of patterns and return gaps"""
        gaps = []
        
        for pattern_name, (pattern_re, severity, description) in patterns.items():
            for line_num, line in enumerate(lines, 1):
                if pattern_re.search(line):
                    snippet = line.strip()[:100]
                    gap = S2CryptoGap(
                        file_path=file_path,
                        line_num=line_num,
                        gap_type=gap_type,
                        snippet=snippet,
                        severity=severity,
                        description=description,
                        remediation='Replace with modern cryptographic algorithm (AES-256-GCM, ChaCha20, SHA-256)'
                    )
                    gaps.append(gap)
        
        return gaps


def main():
    """Main entry point for scanner"""
    import sys
    
    scanner = S2CryptoScanner()
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
        'enhancement': 'S-2',
        'cwe': 'CWE-327',
        'title': 'Cryptographic Weaknesses Detection',
        'total_gaps': len(all_gaps),
        'gaps': [gap.to_dict() for gap in all_gaps]
    }
    
    print(json.dumps(results, indent=2))
    return len(all_gaps)


if __name__ == '__main__':
    main()
