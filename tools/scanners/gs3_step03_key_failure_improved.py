#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 - Key Failure Detection (IMPROVED)

Improvements:
1. Key/string checks only in cryptographic context to reduce generic string false positives.
2. Exclude tests/fixtures/examples/benchmarks from critical key findings.
3. Keep high-confidence detections for hardcoded PEM/base64/hex secrets and weak crypto primitives.
"""

import json
import re
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, List


class KeyFailureType(Enum):
    HARDCODED_KEY = 'hardcoded_key'
    WEAK_KEY_GENERATION = 'weak_key_generation'
    NO_KEY_ROTATION = 'no_key_rotation'
    WEAK_KEY_SIZE = 'weak_key_size'
    WEAK_KDF = 'weak_kdf'


@dataclass
class KeyFailureGap:
    file_path: str
    line_num: int
    gap_type: KeyFailureType
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


class KeyFailureScannerImproved:
    """Improved scanner for key-management failures."""

    HARDCODED_KEY_PATTERNS = {
        'pem_key': re.compile(r'-----BEGIN\s+(?:RSA\s+)?PRIVATE\s+KEY-----'),
        'base64_key': re.compile(r'(?:key|secret|private)\s*=\s*["\']([A-Za-z0-9+/]{40,}={0,2})["\']', re.IGNORECASE),
        'hex_key': re.compile(r'(?:key|secret|private)\s*=\s*["\']([0-9a-fA-F]{32,})["\']', re.IGNORECASE),
    }

    WEAK_RNG_PATTERNS = {
        'rand_function': re.compile(r'\b(rand|random|drand48|lrand48|mrand48)\s*\('),
        'time_seed': re.compile(r'(srand|srandom)\s*\(\s*time\s*\('),
    }

    WEAK_KEY_SIZE_PATTERNS = {
        'des': re.compile(r'\bDES\b|\bDES_\w+\('),
        'rsa_512_1024': re.compile(r'RSA.*(?:512|1024)|RSA_generate_key\((?:512|1024)'),
        'small_key': re.compile(r'(?:key_size|keySize|KEY_SIZE)\s*=\s*([0-9]+)', re.IGNORECASE),
    }

    WEAK_KDF_PATTERNS = {
        'simple_md5': re.compile(r'MD5_\w+|EVP_md5\(\)'),
        'simple_sha1': re.compile(r'SHA1_\w+|EVP_sha1\(\)|SHA_\w+\('),
        'low_iteration': re.compile(r'(?:iteration|count)\s*=\s*([0-9]+)(?![0-9])', re.IGNORECASE),
    }

    NO_ROTATION_PATTERNS = {
        'static_key': re.compile(r'(?:const|static)\s+.*(?:key|secret|password)\s*='),
        'global_key': re.compile(r'^(?:const|static)?\s*(?:const\s+)?(?:char|std::string)\s+\w*key\w*', re.IGNORECASE),
    }

    APPROVED_PATTERNS = {
        'env_var': re.compile(r'getenv|std::getenv|getenv_s|secure_getenv', re.IGNORECASE),
        'vault': re.compile(r'vault|hsm|kms|key_server|key_manager', re.IGNORECASE),
        'crypto_lib': re.compile(r'EVP_|openssl|boringssl|libsodium|crypto_', re.IGNORECASE),
    }

    CRYPTO_CONTEXT = [
        r'\bcrypt\b', r'\bcrypto\b', r'\bencrypt', r'\bdecrypt', r'\btls\b',
        r'\bssl\b', r'\bcert', r'\bkey\b', r'\bsecret\b', r'\btoken\b',
        r'argon2', r'pbkdf2', r'hmac', r'cipher', r'aes', r'chacha',
    ]

    TEST_PATH_MARKERS = ['tests/', 'test_', '_test.', 'benchmarks/', 'bench_', 'fixtures/', 'examples/', 'demo_']

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[KeyFailureGap]] = {}
        self._crypto_context_re = [re.compile(p, re.IGNORECASE) for p in self.CRYPTO_CONTEXT]

    @staticmethod
    def _path_norm(path: Path) -> str:
        return str(path).replace('\\', '/').lower()

    def _is_non_prod_path(self, file_path: Path) -> bool:
        p = self._path_norm(file_path)
        return any(m in p for m in self.TEST_PATH_MARKERS)

    def _is_approved_usage(self, line: str) -> bool:
        return any(p.search(line) for p in self.APPROVED_PATTERNS.values())

    def _is_crypto_context(self, line: str, context: str) -> bool:
        joined = f"{line}\\n{context}"
        return any(p.search(joined) for p in self._crypto_context_re)

    @staticmethod
    def _extract_number(match) -> int:
        try:
            if match.lastindex and match.lastindex >= 1:
                return int(match.group(1))
        except Exception:
            pass
        return 0

    def _check_hardcoded_keys(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        gaps: List[KeyFailureGap] = []
        for line_num, line in enumerate(lines, 1):
            if self._is_approved_usage(line):
                continue
            context = ''.join(lines[max(0, line_num - 3):min(len(lines), line_num + 2)])
            if not self._is_crypto_context(line, context):
                continue

            if self.HARDCODED_KEY_PATTERNS['pem_key'].search(line):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.HARDCODED_KEY,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Hardcoded PEM private key detected',
                    remediation='Move key material to secure key management (KMS/HSM/Vault)',
                    confidence=0.99,
                ))
                continue

            if self.HARDCODED_KEY_PATTERNS['base64_key'].search(line) or self.HARDCODED_KEY_PATTERNS['hex_key'].search(line):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.HARDCODED_KEY,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Potential hardcoded cryptographic key detected',
                    remediation='Load secrets from secure runtime source; avoid static literals',
                    confidence=0.94,
                ))
        return gaps

    def _check_weak_rng(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        gaps: List[KeyFailureGap] = []
        for line_num, line in enumerate(lines, 1):
            context = ''.join(lines[max(0, line_num - 2):min(len(lines), line_num + 3)])
            if not self._is_crypto_context(line, context):
                continue
            if any(p.search(line) for p in self.WEAK_RNG_PATTERNS.values()):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KEY_GENERATION,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Weak randomness source in key/security context',
                    remediation='Use cryptographically secure RNG (OS CSPRNG / crypto library RNG)',
                    confidence=0.90,
                ))
        return gaps

    def _check_key_size(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        gaps: List[KeyFailureGap] = []
        for line_num, line in enumerate(lines, 1):
            context = ''.join(lines[max(0, line_num - 2):min(len(lines), line_num + 3)])
            if not self._is_crypto_context(line, context):
                continue

            if self.WEAK_KEY_SIZE_PATTERNS['des'].search(line) or self.WEAK_KEY_SIZE_PATTERNS['rsa_512_1024'].search(line):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KEY_SIZE,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Weak cryptographic algorithm/key size detected',
                    remediation='Use modern primitives (AES-256-GCM / ChaCha20-Poly1305, RSA>=2048)',
                    confidence=0.95,
                ))
                continue

            m = self.WEAK_KEY_SIZE_PATTERNS['small_key'].search(line)
            if m:
                key_size = self._extract_number(m)
                if 0 < key_size < 128:
                    gaps.append(KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.WEAK_KEY_SIZE,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description=f'Key size too small ({key_size} bits)',
                        remediation='Use >=128-bit symmetric keys (or stronger policy baseline)',
                        confidence=0.84,
                    ))
        return gaps

    def _check_weak_kdf(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        gaps: List[KeyFailureGap] = []
        for line_num, line in enumerate(lines, 1):
            context = ''.join(lines[max(0, line_num - 2):min(len(lines), line_num + 3)])
            if not self._is_crypto_context(line, context):
                continue

            if self.WEAK_KDF_PATTERNS['simple_md5'].search(line):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KDF,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='MD5 used in key derivation/security path',
                    remediation='Use Argon2/scrypt/PBKDF2-HMAC-SHA256+ with policy-compliant params',
                    confidence=0.95,
                ))
                continue

            if self.WEAK_KDF_PATTERNS['simple_sha1'].search(line):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.WEAK_KDF,
                    snippet=line.strip()[:100],
                    severity='HIGH',
                    description='SHA-1 used in key derivation/security path',
                    remediation='Use SHA-256+ based KDF configuration',
                    confidence=0.90,
                ))
                continue

            m = self.WEAK_KDF_PATTERNS['low_iteration'].search(line)
            if m:
                iterations = self._extract_number(m)
                if 0 < iterations < 100000:
                    gaps.append(KeyFailureGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=KeyFailureType.WEAK_KDF,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description=f'Low KDF iteration count ({iterations})',
                        remediation='Increase KDF work factor to current policy baseline',
                        confidence=0.80,
                    ))
        return gaps

    def _check_no_rotation(self, file_path: Path, lines: List[str]) -> List[KeyFailureGap]:
        gaps: List[KeyFailureGap] = []
        for line_num, line in enumerate(lines, 1):
            if self._is_approved_usage(line):
                continue
            context = ''.join(lines[max(0, line_num - 2):min(len(lines), line_num + 3)])
            if not self._is_crypto_context(line, context):
                continue

            if any(p.search(line) for p in self.NO_ROTATION_PATTERNS.values()):
                gaps.append(KeyFailureGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=KeyFailureType.NO_KEY_ROTATION,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='Static key material without visible rotation strategy',
                    remediation='Use versioned keys and enforce key rotation lifecycle',
                    confidence=0.74,
                ))
        return gaps

    def scan_file(self, file_path: Path) -> List[KeyFailureGap]:
        if self._is_non_prod_path(file_path):
            return []

        try:
            lines = file_path.read_text(encoding='utf-8', errors='ignore').splitlines()
        except Exception:
            return []

        gaps: List[KeyFailureGap] = []
        gaps.extend(self._check_hardcoded_keys(file_path, lines))
        gaps.extend(self._check_weak_rng(file_path, lines))
        gaps.extend(self._check_key_size(file_path, lines))
        gaps.extend(self._check_weak_kdf(file_path, lines))
        gaps.extend(self._check_no_rotation(file_path, lines))
        return gaps

    def scan_repository(self) -> Dict[str, List[KeyFailureGap]]:
        gaps_by_file: Dict[str, List[KeyFailureGap]] = {}

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
        data: Dict[str, List[Dict]] = {}
        for file_path, gap_list in self.gaps.items():
            data[file_path] = [g.to_dict() for g in gap_list]
        return json.dumps(data, indent=2)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Key Failure Detection Scanner (Improved)')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()

    scanner = KeyFailureScannerImproved(args.repo)
    gaps = scanner.scan_repository()

    result = scanner.to_json()
    if args.output:
        Path(args.output).write_text(result, encoding='utf-8')
        print(f'Results written to {args.output}')
    else:
        print(result)

    total = sum(len(v) for v in gaps.values())
    print(f'\nTotal key management vulnerabilities found: {total}')
