#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Military Hardening (P11-6)

Enforces FIPS 140-2, classified data protection, compartmentalization:
- FIPS 140-2 algorithm approval
- Classified data handling and protection
- Compartmentalization and security level mixing
- Side-channel attack mitigations
- Audit logging for security events
- Hardware backing (TPM/HSM) requirements
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class MilitaryHardeningType(Enum):
    """Military hardening violation classifications"""
    UNAPPROVED_ALGORITHM = "unapproved_algorithm"
    CLASSIFIED_DATA_UNPROTECTED = "classified_data_unprotected"
    COMPARTMENTALIZATION_VIOLATION = "compartmentalization_violation"
    NO_SIDE_CHANNEL_MITIGATION = "no_side_channel_mitigation"
    MISSING_AUDIT_LOG = "missing_audit_log"
    COVERT_CHANNEL_RISK = "covert_channel_risk"
    NO_HARDWARE_BACKING = "no_hardware_backing"


@dataclass
class MilitaryHardeningGap:
    """Represents a military hardening violation"""
    file_path: str
    line_num: int
    gap_type: MilitaryHardeningType
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


class MilitaryHardeningScanner:
    """Detect military hardening violations (FIPS 140-2 / CC / EAL4)"""
    
    # FIPS 140-2 UNAPPROVED ALGORITHMS
    UNAPPROVED_PATTERNS = {
        'des': re.compile(r'\bDES\b|DES_\w+|EVP_des\(', re.IGNORECASE),
        'md5': re.compile(r'\bMD5\b|EVP_md5\(\)|MD5_', re.IGNORECASE),
        'sha1': re.compile(r'\bSHA1\b|EVP_sha1\(\)|SHA1_', re.IGNORECASE),
        'rc4': re.compile(r'\bRC4\b|EVP_rc4\(|ARCFOUR', re.IGNORECASE),
        'weak_rng': re.compile(r'\brand\(\)|\bsrand\(|time.*seed|drand48', re.IGNORECASE),
    }
    
    # CLASSIFIED DATA PATTERNS
    CLASSIFIED_PATTERNS = {
        'classified_var': re.compile(r'secret|classified|confidential|top.*secret', re.IGNORECASE),
        'unencrypted': re.compile(r'std::string.*secret|plaintext.*classified', re.IGNORECASE),
    }
    
    # COMPARTMENTALIZATION PATTERNS
    COMPARTMENT_PATTERNS = {
        'mixed_levels': re.compile(r'public.*secret|secret.*public|mixed.*security', re.IGNORECASE),
        'shared_buffer': re.compile(r'static.*buffer|global.*secret|shared.*classified', re.IGNORECASE),
    }
    
    # SIDE-CHANNEL PATTERNS (TIMING ATTACKS)
    SIDE_CHANNEL_PATTERNS = {
        'timing_dependent': re.compile(r'if\s*\(\s*\w+\s*==|strcmp\(|memcmp\(', re.IGNORECASE),
        'secret_operation': re.compile(r'if.*secret|if.*password|if.*token', re.IGNORECASE),
    }
    
    # AUDIT/LOGGING PATTERNS
    AUDIT_PATTERNS = {
        'security_op': re.compile(r'key.*generation|authentication|authorization|encrypt', re.IGNORECASE),
        'no_audit': re.compile(r'(?!.*log|.*audit)', re.IGNORECASE),
    }
    
    # HARDWARE BACKING PATTERNS
    HARDWARE_PATTERNS = {
        'key_storage': re.compile(r'store.*key|save.*key|key.*file', re.IGNORECASE),
        'no_hsm': re.compile(r'(?!.*HSM|.*TPM|.*vault|.*KMS)', re.IGNORECASE),
    }
    
    # APPROVED FIPS 140-2 PATTERNS (WHITELIST)
    APPROVED_FIPS_PATTERNS = {
        'aes_256': re.compile(r'AES.256|EVP_aes_256_gcm|EVP_aes_256_cbc'),
        'sha_256': re.compile(r'SHA.256|EVP_sha256|SHA256'),
        'rsa_2048': re.compile(r'RSA.2048|RSA.4096|EVP_PKEY_RSA'),
    }
    
    # APPROVED HARDWARE
    APPROVED_HARDWARE = {
        'tpm': re.compile(r'TPM|tpm|Trusted.*Module'),
        'hsm': re.compile(r'HSM|hsm|Hardware.*Security|PKCS11'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [r'test.*military', r'fips.*test', r'mock.*hardening']
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[MilitaryHardeningGap]] = {}
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_PATTERNS]
    
    def _is_test_context(self, file_name: str, line: str) -> bool:
        if 'test' in file_name.lower():
            return True
        for pattern in self.test_patterns:
            if pattern.search(line):
                return True
        return False
    
    def _is_approved_fips(self, line: str) -> bool:
        for pattern in self.APPROVED_FIPS_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def _check_unapproved_algo(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Detect unapproved algorithms (FIPS 140-2)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self._is_approved_fips(line):
                continue
            
            for algo_type, pattern in self.UNAPPROVED_PATTERNS.items():
                if pattern.search(line):
                    severity = 'CRITICAL' if algo_type in ['des', 'md5'] else 'HIGH'
                    gaps.append(MilitaryHardeningGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MilitaryHardeningType.UNAPPROVED_ALGORITHM,
                        snippet=line.strip()[:100],
                        severity=severity,
                        description=f'{algo_type.upper()} — not FIPS 140-2 approved',
                        remediation='Use only FIPS 140-2 approved algorithms (AES-256, SHA-256/512, RSA-2048+)',
                        confidence=0.95
                    ))
                    break
        return gaps
    
    def _check_classified_data(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Detect unprotected classified data"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.CLASSIFIED_PATTERNS['classified_var'].search(line):
                if not self._is_approved_fips(line):
                    gaps.append(MilitaryHardeningGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MilitaryHardeningType.CLASSIFIED_DATA_UNPROTECTED,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Classified data without FIPS 140-2 encryption protection',
                        remediation='Protect with FIPS 140-2-approved encryption (AES-256-GCM)',
                        confidence=0.80
                    ))
        return gaps
    
    def _check_timing_attack(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Detect timing-dependent operations on secrets"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.SIDE_CHANNEL_PATTERNS['timing_dependent'].search(line):
                if self.SIDE_CHANNEL_PATTERNS['secret_operation'].search(line):
                    gaps.append(MilitaryHardeningGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MilitaryHardeningType.NO_SIDE_CHANNEL_MITIGATION,
                        snippet=line.strip()[:100],
                        severity='HIGH',
                        description='Timing-dependent operation on sensitive data',
                        remediation='Use constant-time comparison (sodium_memcmp, CRYPTO_memcmp)',
                        confidence=0.75
                    ))
        return gaps
    
    def _check_compartmentalization(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Detect compartmentalization violations (mixing security levels)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.COMPARTMENT_PATTERNS['mixed_levels'].search(line):
                gaps.append(MilitaryHardeningGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=MilitaryHardeningType.COMPARTMENTALIZATION_VIOLATION,
                    snippet=line.strip()[:100],
                    severity='HIGH',
                    description='Compartmentalization violation - mixing security levels',
                    remediation='Enforce strict compartmentalization - separate secret/public data',
                    confidence=0.70
                ))
        return gaps
    
    def _check_audit_logging(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Detect missing security event logging (refined to reduce FP)"""
        gaps = []
        
        # Only check actual entry points to security operations, not internal helpers
        entry_point_patterns = [
            r'def\s+authenticate',
            r'def\s+authorize', 
            r'def\s+login',
            r'def\s+logout',
            r'def\s+verify',
            r'def\s+decrypt',
            r'def\s+sign',
        ]
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Only flag actual security operation entry points
            if not any(re.search(pattern, line, re.IGNORECASE) for pattern in entry_point_patterns):
                continue
            
            # Check if function body has logging
            context_start = line_num
            context_end = min(len(lines), line_num + 30)  # Check next 30 lines for log calls
            context = ''.join(lines[context_start:context_end]).lower()
            
            if not ('log' in context or 'audit' in context or 'syslog' in context or 'event' in context):
                gaps.append(MilitaryHardeningGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=MilitaryHardeningType.MISSING_AUDIT_LOG,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='Security operation without audit logging',
                    remediation='Log all security events (authentication, authorization, key operations)',
                    confidence=0.65  # Reduced due to FP risk
                ))
        return gaps
    
    def _check_covert_channels(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Detect potential covert channel risks"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            # Timing-based side channels through conditional operations
            if re.search(r'if.*timing|if.*latency|if.*delay', line, re.IGNORECASE):
                gaps.append(MilitaryHardeningGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=MilitaryHardeningType.COVERT_CHANNEL_RISK,
                    snippet=line.strip()[:100],
                    severity='MEDIUM',
                    description='Potential covert channel via timing variations',
                    remediation='Ensure constant-time execution for all secret operations',
                    confidence=0.50
                ))
        return gaps
    
    def _check_hardware_backing(self, file_path: Path, lines: List[str]) -> List[MilitaryHardeningGap]:
        """Check if keys are stored in hardware (TPM/HSM)"""
        gaps = []
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            if self.HARDWARE_PATTERNS['key_storage'].search(line):
                approved = any(p.search(line) for p in self.APPROVED_HARDWARE.values())
                if not approved:
                    gaps.append(MilitaryHardeningGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MilitaryHardeningType.NO_HARDWARE_BACKING,
                        snippet=line.strip()[:100],
                        severity='MEDIUM',
                        description='Key storage without hardware backing (TPM/HSM)',
                        remediation='Store cryptographic keys in TPM or HSM for protection',
                        confidence=0.55
                    ))
        return gaps
    
    def scan_file(self, file_path: Path) -> List[MilitaryHardeningGap]:
        """Scan single file for military hardening gaps"""
        gaps = []
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        gaps.extend(self._check_unapproved_algo(file_path, lines))
        gaps.extend(self._check_classified_data(file_path, lines))
        gaps.extend(self._check_timing_attack(file_path, lines))
        gaps.extend(self._check_compartmentalization(file_path, lines))
        gaps.extend(self._check_audit_logging(file_path, lines))
        gaps.extend(self._check_covert_channels(file_path, lines))
        gaps.extend(self._check_hardware_backing(file_path, lines))
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[MilitaryHardeningGap]]:
        """Scan module for military hardening gaps"""
        gaps_by_file = {}
        for directory in [self.repo_root / 'src' / module, self.repo_root / 'include' / module]:
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
    
    def scan_repository(self) -> Dict[str, List[MilitaryHardeningGap]]:
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
    parser = argparse.ArgumentParser(description='Military Hardening Scanner')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    scanner = MilitaryHardeningScanner(args.repo)
    gaps = scanner.scan_repository()
    result = scanner.to_json()
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(result)
        print(f"Results written to {args.output}")
    else:
        print(result)
    
    total_gaps = sum(len(v) for v in gaps.values())
    print(f"\nTotal military hardening violations found: {total_gaps}")
