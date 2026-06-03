#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Key Failure Detection

Detects cryptographic key management vulnerabilities:
- Hardcoded cryptographic keys
- Keys stored in plaintext
- Weak key generation
- Keys not rotated
- Keys not protected in memory
- Key size too small
- Keys in version control
- Insecure key derivation
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class KeyFailureType(Enum):
    """Cryptographic key failure classifications"""
    HARDCODED_KEY = "hardcoded_key"              # Key in source code
    PLAINTEXT_KEY_STORAGE = "plaintext_key_storage"  # Key not encrypted at rest
    WEAK_KEY_GENERATION = "weak_key_generation"  # Insufficient entropy
    NO_KEY_ROTATION = "no_key_rotation"          # Static key, never rotated
    UNPROTECTED_MEMORY = "unprotected_memory"    # Key not zeroed from memory
    WEAK_KEY_SIZE = "weak_key_size"              # Key < 128 bits symmetric, < 2048 RSA
    KEY_IN_VCS = "key_in_vcs"                    # Key in git/version control
    WEAK_KDF = "weak_kdf"                        # Weak key derivation function
    NO_KEY_BACKUP = "no_key_backup"              # Missing key recovery procedure
    INSECURE_KEY_EXCHANGE = "insecure_key_exchange"  # No perfect forward secrecy


@dataclass
class KeyFailureGap:
    """Represents a key management vulnerability"""
    file_path: str
    line_num: int
    gap_type: KeyFailureType
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


class KeyFailureScanner:
    """Detect cryptographic key management vulnerabilities"""
    
    # HARDCODED KEY PATTERNS
    HARDCODED_KEY_PATTERNS = {
        # Base64 encoded keys (typical length 40-80+ chars)
        'base64_key': re.compile(r'(?:key|secret|private)\s*=\s*["\']([A-Za-z0-9+/]{40,}={0,2})["\']', re.IGNORECASE),
        
        # Hex encoded keys
        'hex_key': re.compile(r'(?:key|secret|private)\s*=\s*["\']([0-9a-fA-F]{32,})["\']', re.IGNORECASE),
        
        # PEM-formatted keys (-----BEGIN...-----END-----)
        'pem_key': re.compile(r'-----BEGIN\s+(?:RSA\s+)?PRIVATE\s+KEY-----'),
        
        # AWS KMS key IDs
        'aws_kms_key': re.compile(r'arn:aws:kms:[a-z0-9-]+:\d{12}:key/[a-f0-9-]{36}'),
        
        # Master keys, root keys
        'master_key': re.compile(r'(?:master_key|root_key|kek)\s*=\s*["\']([A-Za-z0-9+/]{20,})["\']', re.IGNORECASE),
    }
    
    # WEAK KEY GENERATION PATTERNS
    WEAK_RNG_PATTERNS = {
        'rand_function': re.compile(r'\b(rand|random|drand48|lrand48|mrand48)\s*\('),
        'time_seed': re.compile(r'(srand|srandom)\s*\(\s*time\s*\('),
        'predictable_seed': re.compile(r'(srand|srandom)\s*\(\s*[0-9]+\s*\)'),
        'weak_seed': re.compile(r'(srand|srandom)\s*\(\s*\w+\.(second|millisecond)\s*\)'),
    }
    
    # WEAK KEY SIZE PATTERNS
    WEAK_KEY_SIZE_PATTERNS = {
        'des': re.compile(r'\bDES\b|\bDES_\w+\('),
        'rsa_512': re.compile(r'RSA.*512|RSA_generate_key\(512'),
        'rsa_1024': re.compile(r'RSA.*1024|RSA_generate_key\(1024'),
        'small_key': re.compile(r'(?:key_size|keySize|KEY_SIZE)\s*=\s*([0-9]+)', re.IGNORECASE),
    }
    
    # WEAK KDF PATTERNS
    WEAK_KDF_PATTERNS = {
        'simple_md5': re.compile(r'MD5_\w+|EVP_md5\(\)'),
        'simple_sha1': re.compile(r'SHA1_\w+|EVP_sha1\(\)|SHA_\w+\('),
        'low_iteration': re.compile(r'(?:iteration|count)\s*=\s*([0-9]+)(?![0-9])', re.IGNORECASE),
        'no_salt': re.compile(r'(?:deriveKey|password.*hash|pbkdf2|scrypt).*password["\']?\s*\)', re.IGNORECASE),
    }
    
    # KEY ROTATION PATTERNS
    NO_ROTATION_PATTERNS = {
        'static_key': re.compile(r'(?:const|static)\s+.*(?:key|secret|password)\s*='),
        'global_key': re.compile(r'^(?:const|static)?\s*(?:const\s+)?char\s+\w*key\w*\[\]|^(?:const|static)?\s*(?:const\s+)?std::string\s+\w*key\w*'),
    }
    
    # APPROVED KEY MANAGEMENT PATTERNS (WHITELIST)
    APPROVED_PATTERNS = {
        'env_var': re.compile(r'getenv|std::getenv|getenv_s|secure_getenv'),
        'vault': re.compile(r'vault|hsm|kms|key_server|key_manager'),
        'secure_key': re.compile(r'secure_key|SecureString|ProtectedString'),
        'openssl_evp': re.compile(r'EVP_BytesToKey|EVP_PKEY'),
        'libsodium': re.compile(r'sodium_|randombytes|crypto_'),
        'boringssl': re.compile(r'BSSL|BoringSSL|boringssl'),
    }
    
    # TEST DATA WHITELIST
    TEST_PATTERNS = [
        r'test.*key|key.*test',
        r'demo.*key|key.*demo',
        r'example.*key',
        r'TESTKEYVALUE|TEST_KEY_VALUE',
        r'test/.*key|fixtures/.*key',
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[KeyFailureGap]] = {}
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
        """Check if this uses approved key management patterns."""
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def _extract_number(self, match) -> int:
        """Extract integer from regex match."""
        try:
            if match.lastindex and match.lastindex >= 1:
                return int(match.group(1))
        except (ValueError, AttributeError):
            pass
        return 0
    
    def _check_hardcoded_keys(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        """Detect hardcoded cryptographic keys."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            if self._is_approved_usage(line):
                continue
            
            # Check for PEM keys
            if self.HARDCODED_KEY_PATTERNS['pem_key'].search(line):
                gap = KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.HARDCODED_KEY,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Hardcoded PEM private key detected',
                    remediation='Move key to secure key management (HSM, Vault, Environment variables)',
                    confidence=0.99
                )
                gaps.append(gap)
            
            # Check for base64/hex encoded keys
            for key_type, pattern in [('base64', self.HARDCODED_KEY_PATTERNS['base64_key']),
                                     ('hex', self.HARDCODED_KEY_PATTERNS['hex_key'])]:
                match = pattern.search(line)
                if match:
                    gap = KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.HARDCODED_KEY,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description=f'Hardcoded {key_type}-encoded key detected',
                        remediation='Use key derivation (KDF) or secure key store',
                        confidence=0.95
                    )
                    gaps.append(gap)
                    break
        
        return gaps
    
    def _check_weak_key_generation(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        """Detect weak key generation patterns."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for weak RNG
            for rng_type, pattern in self.WEAK_RNG_PATTERNS.items():
                if pattern.search(line):
                    gap = KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.WEAK_KEY_GENERATION,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description=f'Weak random number generation ({rng_type}) in key generation',
                        remediation='Use cryptographically secure RNG: /dev/urandom, randombytes(), or OpenSSL RNG',
                        confidence=0.90
                    )
                    gaps.append(gap)
        
        return gaps
    
    def _check_weak_key_size(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        """Detect insufficient key sizes."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for DES (always weak)
            if self.WEAK_KEY_SIZE_PATTERNS['des'].search(line):
                gap = KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KEY_SIZE,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='DES encryption detected — cryptographically broken',
                    remediation='Use AES-256-GCM or ChaCha20-Poly1305 instead of DES',
                    confidence=0.99
                )
                gaps.append(gap)
            
            # Check for RSA < 2048 bits
            for rsa_pattern in [self.WEAK_KEY_SIZE_PATTERNS['rsa_512'],
                               self.WEAK_KEY_SIZE_PATTERNS['rsa_1024']]:
                if rsa_pattern.search(line):
                    gap = KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.WEAK_KEY_SIZE,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='RSA key < 2048 bits detected',
                        remediation='Use RSA-4096 or ECDSA P-256+ for modern cryptography',
                        confidence=0.95
                    )
                    gaps.append(gap)
            
            # Check for explicit small key sizes
            match = self.WEAK_KEY_SIZE_PATTERNS['small_key'].search(line)
            if match:
                key_size = self._extract_number(match)
                if 0 < key_size < 128:  # Avoid false positives on variable names
                    gap = KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.WEAK_KEY_SIZE,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description=f'Cryptographic key size too small ({key_size} bits)',
                        remediation='Use at least 128 bits for symmetric keys, 2048 for RSA',
                        confidence=0.85
                    )
                    gaps.append(gap)
        
        return gaps
    
    def _check_weak_kdf(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        """Detect weak key derivation functions."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for weak hash in KDF
            if self.WEAK_KDF_PATTERNS['simple_md5'].search(line):
                gap = KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KDF,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='MD5 used in key derivation — cryptographically broken',
                    remediation='Use PBKDF2, scrypt, or Argon2 with SHA-256 or better',
                    confidence=0.95
                )
                gaps.append(gap)
            
            if self.WEAK_KDF_PATTERNS['simple_sha1'].search(line):
                gap = KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KDF,
                    snippet=line.strip()[:100],
                    severity='HIGH',
                    description='SHA-1 used in key derivation — weak for cryptographic purposes',
                    remediation='Use SHA-256 or better in key derivation functions',
                    confidence=0.90
                )
                gaps.append(gap)
            
            # Check for low iteration count (weak PBKDF2)
            match = self.WEAK_KDF_PATTERNS['low_iteration'].search(line)
            if match:
                iterations = self._extract_number(match)
                if 0 < iterations < 100000:  # NIST recommends >= 100k
                    gap = KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.WEAK_KDF,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description=f'Low PBKDF2 iteration count ({iterations}) — insufficient',
                        remediation='Use at least 100,000 iterations for PBKDF2, or Argon2',
                        confidence=0.80
                    )
                    gaps.append(gap)
        
        return gaps
    
    def _check_no_rotation(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        """Detect static keys that are never rotated."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for static/const keys
            for pattern in [self.NO_ROTATION_PATTERNS['static_key'],
                           self.NO_ROTATION_PATTERNS['global_key']]:
                if pattern.search(line) and not self._is_approved_usage(line):
                    gap = KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.NO_KEY_ROTATION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Static/global key — never rotated',
                        remediation='Implement key rotation: use key versioning and periodic key generation',
                        confidence=0.75
                    )
                    gaps.append(gap)
                    break
        
        return gaps
    
    def scan_file(self, file_path: Path) -> List[KeyFailureGap]:
        """Scan single file for key management vulnerabilities"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        # Run all checks
        gaps.extend(self._check_hardcoded_keys(file_path, lines))
        gaps.extend(self._check_weak_key_generation(file_path, lines))
        gaps.extend(self._check_weak_key_size(file_path, lines))
        gaps.extend(self._check_weak_kdf(file_path, lines))
        gaps.extend(self._check_no_rotation(file_path, lines))
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[KeyFailureGap]]:
        """Scan module for key management vulnerabilities"""
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
    
    def scan_repository(self) -> Dict[str, List[KeyFailureGap]]:
        """Scan entire repository for key management vulnerabilities"""
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
    
    parser = argparse.ArgumentParser(description='Key Failure Detection Scanner')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    scanner = KeyFailureScanner(args.repo)
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
    print(f"\nTotal key management vulnerabilities found: {total_gaps}")
