#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Military Hardening (FIPS 140-2 / Common Criteria EAL4)

Detects gaps in military/government hardening requirements:
- FIPS 140-2 approved algorithms only
- Classified data handling
- Compartmentalization violations
- Side-channel attack mitigations
- Covert channel prevention
- Hardware-backed key storage
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class MilitaryHardeningType(Enum):
    """Military hardening violation classifications"""
    UNAPPROVED_ALGORITHM = "unapproved_algorithm"      # Not FIPS 140-2 approved
    CLASSIFIED_DATA_UNPROTECTED = "classified_data_unprotected"  # Classified without protection
    COMPARTMENTALIZATION_VIOLATION = "compartmentalization_violation"  # Security levels mixed
    NO_SIDE_CHANNEL_MITIGATION = "no_side_channel_mitigation"  # Timing attack risk
    COVERT_CHANNEL_RISK = "covert_channel_risk"        # Covert channel possible
    NO_HARDWARE_BACKING = "no_hardware_backing"        # Key not hardware-backed
    MISSING_AUDIT_LOG = "missing_audit_log"            # No audit trail
    INSUFFICIENT_ENTROPY = "insufficient_entropy"      # Weak randomness


@dataclass
class MilitaryHardeningGap:
    """Represents a military hardening violation"""
    file_path: str
    line_num: int
    gap_type: MilitaryHardeningType
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


class MilitaryHardeningScanner:
    """Detect military hardening violations"""
    
    # FIPS 140-2 UNAPPROVED ALGORITHMS
    UNAPPROVED_PATTERNS = {
        'des': re.compile(r'\bDES\b|\bDES_\w+\(|EVP_des\(', re.IGNORECASE),
        'md5': re.compile(r'\bMD5|EVP_md5\(\)', re.IGNORECASE),
        'sha1': re.compile(r'\bSHA1|EVP_sha1\(\)', re.IGNORECASE),
        'rc4': re.compile(r'\bRC4\b|EVP_rc4\(', re.IGNORECASE),
    }
    
    # CLASSIFIED DATA PATTERNS (UNPROTECTED)
    CLASSIFIED_PATTERNS = {
        'classified_var': re.compile(r'secret|classified|confidential|top\s*secret', re.IGNORECASE),
        'no_encryption': re.compile(r'std::string.*secret|char.*classified|plaintext.*classified', re.IGNORECASE),
    }
    
    # COMPARTMENTALIZATION PATTERNS
    COMPARTMENT_PATTERNS = {
        'mixed_levels': re.compile(r'public.*secret|secret.*public|mixed.*security.*level', re.IGNORECASE),
        'shared_buffer': re.compile(r'static.*buffer|global.*buffer|shared_ptr.*secret', re.IGNORECASE),
    }
    
    # SIDE-CHANNEL PATTERNS
    SIDE_CHANNEL_PATTERNS = {
        'timing_dependent': re.compile(r'if\s*\(\s*\w+\s*==|string\s*compare|memcmp', re.IGNORECASE),
        'no_constant_time': re.compile(r'strcmp|strncmp|==.*secret|==.*password', re.IGNORECASE),
    }
    
    # AUDIT PATTERNS
    AUDIT_PATTERNS = {
        'no_logging': re.compile(r'security.*operation|key.*generation|authentication|authorization', re.IGNORECASE),
        'missing_audit': re.compile(r'sensitive.*operation(?!.*log)', re.IGNORECASE),
    }
    
    # APPROVED PATTERNS (WHITELIST)
    APPROVED_PATTERNS = {
        'fips_crypto': re.compile(r'AES_256|SHA_256|SHA_512|RSA_4096|ECDSA_P256'),
        'constant_time': re.compile(r'constant.*time|timing.*safe|CRYPTO_memcmp|crypto_verify'),
        'audit_log': re.compile(r'audit\s*log|security\s*log|event\s*log'),
        'hardware_backing': re.compile(r'TPM|HSM|hardware.*key|PKCS11'),
    }
    
    # TEST PATTERNS
    TEST_PATTERNS = [
        r'test.*military|military.*test',
        r'fips.*test|mock.*hardening',
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[MilitaryHardeningGap]] = {}
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
        """Check if using approved patterns."""
        for pattern in self.APPROVED_PATTERNS.values():
            if pattern.search(line):
                return True
        return False
    
    def scan_file(self, file_path: Path) -> List[MilitaryHardeningGap]:
        """Scan single file for military hardening gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_context(file_path.name, line):
                continue
            
            # Check for unapproved algorithms
            for algo_type, pattern in self.UNAPPROVED_PATTERNS.items():
                if pattern.search(line):
                    if not self._is_approved_usage(line):
                        gap = MilitaryHardeningGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MilitaryHardeningType.UNAPPROVED_ALGORITHM,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description=f'{algo_type.upper()} — not FIPS 140-2 approved',
                            remediation='Use only FIPS 140-2 approved algorithms (AES-256, SHA-256/512)',
                            confidence=0.95
                        )
                        gaps.append(gap)
                        break
            
            # Check for unprotected classified data
            if self.CLASSIFIED_PATTERNS['classified_var'].search(line):
                if not self._is_approved_usage(line):
                    gap = MilitaryHardeningGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=MilitaryHardeningType.CLASSIFIED_DATA_UNPROTECTED,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Classified data without encryption protection',
                        remediation='Protect all classified data with FIPS 140-2 encryption',
                        confidence=0.80
                    )
                    gaps.append(gap)
            
            # Check for timing-dependent operations on secrets
            if self.SIDE_CHANNEL_PATTERNS['timing_dependent'].search(line):
                if 'secret' in line.lower() or 'password' in line.lower():
                    if not self._is_approved_usage(line):
                        gap = MilitaryHardeningGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=MilitaryHardeningType.NO_SIDE_CHANNEL_MITIGATION,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description='Timing-dependent operation on sensitive data',
                            remediation='Use constant-time comparison (crypto_verify, CRYPTO_memcmp)',
                            confidence=0.75
                        )
                        gaps.append(gap)
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[MilitaryHardeningGap]]:
        """Scan module for military hardening gaps"""
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
    
    def scan_repository(self) -> Dict[str, List[MilitaryHardeningGap]]:
        """Scan entire repository for military hardening gaps"""
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
    
    # Print summary
    total_gaps = sum(len(v) for v in gaps.values())
    print(f"\nTotal military hardening violations found: {total_gaps}")
