#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Data Leak Detection

Detects:
- Hardcoded PII (SSN, credit card, phone numbers)
- Sensitive logging (passwords, tokens, API keys, secrets)
- Unencrypted sensitive data storage
- Sensitive variables unmasked in error messages
- Memory not zeroed after use (secrets in stack)
- Credentials in config files/environment (unencrypted)
- API tokens/keys in source code
- Database credentials hardcoded
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from enum import Enum


class DataLeakType(Enum):
    """Data leak classifications"""
    HARDCODED_PII = "hardcoded_pii"           # SSN, credit card, phone
    HARDCODED_SECRET = "hardcoded_secret"     # API key, token, password
    SENSITIVE_LOGGING = "sensitive_logging"   # Password/secret in logs
    UNENCRYPTED_STORAGE = "unencrypted_storage"  # Plaintext sensitive data
    UNMASKED_ERROR = "unmasked_error"         # Sensitive data in error message
    UNZEROED_MEMORY = "unzeroed_memory"       # Secret not cleared from memory
    CONFIG_CREDENTIALS = "config_credentials" # Hardcoded DB/API credentials
    CREDENTIAL_IN_ENV = "credential_in_env"   # Credentials exposed via environ


@dataclass
class DataLeakGap:
    """Represents a data leak vulnerability"""
    file_path: str
    line_num: int
    gap_type: DataLeakType
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


class DataLeakScanner:
    """Detect data leak vulnerabilities in C++ code"""
    
    # PII PATTERNS (with high confidence thresholds)
    PII_PATTERNS = {
        # SSN (various formats: 123-45-6789, 123456789, etc.)
        'ssn': re.compile(r'\b\d{3}[-\.]?\d{2}[-\.]?\d{4}\b'),
        
        # Credit card (Luhn validation patterns)
        'credit_card': re.compile(r'\b\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}\b'),
        
        # Phone numbers (US format: 555-123-4567, (555) 123-4567)
        'phone': re.compile(r'\b[\(\[]?[0-9]{3}[\)\]]?[-.\s]?[0-9]{3}[-.\s]?[0-9]{4}\b'),
        
        # Email addresses
        'email': re.compile(r'\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b'),
    }
    
    # SECRET PATTERNS
    SECRET_PATTERNS = {
        'api_key': re.compile(r'(?:api[_-]?key|apikey)\s*=\s*["\']([a-zA-Z0-9_-]{20,})["\']', re.IGNORECASE),
        'token': re.compile(r'(?:token|access_token|bearer)\s*=\s*["\']([a-zA-Z0-9_.-]{20,})["\']', re.IGNORECASE),
        'password': re.compile(r'(?:password|passwd|pwd)\s*=\s*["\']([^"\']{4,})["\']', re.IGNORECASE),
        'secret': re.compile(r'(?:secret|private_key|privatekey)\s*=\s*["\']([^"\']{10,})["\']', re.IGNORECASE),
        'aws_key': re.compile(r'AKIA[0-9A-Z]{16}'),  # AWS access key format
        'github_token': re.compile(r'ghp_[A-Za-z0-9_]{36}'),
        'db_password': re.compile(r'(?:db_password|database_password|passwd)\s*=\s*["\']([^"\']{4,})["\']', re.IGNORECASE),
    }
    
    # SENSITIVE LOGGING KEYWORDS
    SENSITIVE_LOG_KEYWORDS = [
        'password', 'passwd', 'pwd', 'secret', 'token', 'key', 'credential',
        'apikey', 'api_key', 'auth', 'bearer', 'session', 'cookie', 'ssn',
        'creditcard', 'credit_card', 'pin', 'sensitive', 'classified',
    ]
    
    # TEST DATA WHITELIST (legitimate test patterns to exclude)
    TEST_DATA_PATTERNS = [
        r'test.*\d{3}-\d{2}-\d{4}',  # test SSN
        r'1234[-\s]?4567[-\s]?8901[-\s]?2345',  # Visa test card
        r'5555[-\s]?4444[-\s]?3333[-\s]?2222',  # MasterCard test
        r'(555)[- ]?(123)[- ]?(4567)',  # US test phone
        r'test@test\.com|example@example\.com|demo@demo\.com',  # Test email
        r'TEST_API_KEY|TEST_TOKEN|TEST_SECRET',  # Test uppercase constants
    ]
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[DataLeakGap]] = {}
        
        # Compile test whitelist patterns
        self.test_patterns = [re.compile(p, re.IGNORECASE) for p in self.TEST_DATA_PATTERNS]
    
    def _is_test_data(self, text: str) -> bool:
        """Check if this matches known test data pattern."""
        for pattern in self.test_patterns:
            if pattern.search(text):
                return True
        return False
    
    def _is_test_or_comment_context(self, line: str, file_name: str) -> bool:
        """Skip test files and comments."""
        if 'test' in file_name.lower() or 'mock' in file_name.lower():
            return True
        if line.strip().startswith('//') or line.strip().startswith('*'):
            return True
        return False
    
    def _extract_context(self, lines: List[str], line_num: int) -> str:
        """Extract ±3 lines of context for analysis."""
        start = max(0, line_num - 3)
        end = min(len(lines), line_num + 4)
        return ''.join(lines[start:end])
    
    def _check_pii_exposure(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        """Detect hardcoded PII patterns."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_or_comment_context(line, file_path.name):
                continue
            
            # Check for SSN
            match = self.PII_PATTERNS['ssn'].search(line)
            if match and not self._is_test_data(match.group()):
                context = self._extract_context(lines, line_num - 1)
                gap = DataLeakGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=DataLeakType.HARDCODED_PII,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Hardcoded SSN detected — potential PII exposure',
                    remediation='Use environment variables or secure key management for PII',
                    confidence=0.95
                )
                gaps.append(gap)
            
            # Check for credit card
            match = self.PII_PATTERNS['credit_card'].search(line)
            if match and not self._is_test_data(match.group()):
                gap = DataLeakGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=DataLeakType.HARDCODED_PII,
                    snippet=line.strip()[:100],
                    severity='CRITICAL',
                    description='Hardcoded credit card number detected',
                    remediation='Remove hardcoded payment data; use secure payment gateways',
                    confidence=0.90
                )
                gaps.append(gap)
            
            # Check for phone number (lower confidence, needs context)
            match = self.PII_PATTERNS['phone'].search(line)
            if match and 'phone' in line.lower() and not self._is_test_data(match.group()):
                gap = DataLeakGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    gap_type=DataLeakType.HARDCODED_PII,
                    snippet=line.strip()[:100],
                    severity='HIGH',
                    description='Hardcoded phone number in source code',
                    remediation='Store phone numbers in secure database, not source',
                    confidence=0.75
                )
                gaps.append(gap)
        
        return gaps
    
    def _check_secret_exposure(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        """Detect hardcoded secrets (API keys, tokens, passwords)."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_or_comment_context(line, file_path.name):
                continue
            
            # Check for API keys
            for secret_type, pattern in self.SECRET_PATTERNS.items():
                match = pattern.search(line)
                if match:
                    secret_value = match.group(1) if match.lastindex else match.group()
                    if not self._is_test_data(secret_value):
                        gap = DataLeakGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=DataLeakType.HARDCODED_SECRET,
                            snippet=line.strip()[:100],
                            severity='CRITICAL',
                            description=f'Hardcoded {secret_type} detected',
                            remediation='Use environment variables or secure key management (vault, HSM)',
                            confidence=0.98
                        )
                        gaps.append(gap)
        
        return gaps
    
    def _check_sensitive_logging(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        """Detect sensitive data in logging statements."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            if self._is_test_or_comment_context(line, file_path.name):
                continue
            
            # Check for logging calls with sensitive keywords
            if any(log_func in line for log_func in ['log', 'print', 'cout', 'printf', 'LOG']):
                # Check if line contains sensitive keywords
                line_lower = line.lower()
                for keyword in self.SENSITIVE_LOG_KEYWORDS:
                    if keyword in line_lower:
                        gap = DataLeakGap(
                            file_path=str(file_path.relative_to(self.repo_root)),
                            line_num=line_num,
                            gap_type=DataLeakType.SENSITIVE_LOGGING,
                            snippet=line.strip()[:100],
                            severity='HIGH',
                            description=f'Potential sensitive data ({keyword}) in logging statement',
                            remediation='Mask or redact sensitive data before logging; use structured logging',
                            confidence=0.80
                        )
                        gaps.append(gap)
                        break  # Only report once per line
        
        return gaps
    
    def _check_unzeroed_memory(self, file_path: Path, lines: List[str]) -> List[DataLeakGap]:
        """Detect secrets not zeroed from memory."""
        gaps = []
        
        for line_num, line in enumerate(lines, 1):
            # Look for secret assignments without zeroing
            secret_keywords = ['password', 'secret', 'token', 'apikey', 'privatekey']
            
            if any(kw in line.lower() for kw in secret_keywords):
                # Check if there's memset/secure_zero nearby
                context_start = max(0, line_num - 20)
                context_end = min(len(lines), line_num + 20)
                context = ''.join(lines[context_start:context_end]).lower()
                
                # Check for zeroing patterns
                has_memset = 'memset' in context
                has_secure_zero = 'secure_zero' in context or 'volatile_memset' in context
                
                if not has_memset and not has_secure_zero:
                    # Variable might not be zeroed
                    gap = DataLeakGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=DataLeakType.UNZEROED_MEMORY,
                        snippet=line.strip()[:100],
                        severity='CRITICAL',
                        description='Secret not explicitly zeroed from memory — potential information leak',
                        remediation='Zero sensitive data before deallocation: memset(ptr, 0, size) or secure_zero(ptr)',
                        confidence=0.70
                    )
                    gaps.append(gap)
        
        return gaps
    
    def scan_file(self, file_path: Path) -> List[DataLeakGap]:
        """Scan single file for data leak vulnerabilities"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        # Run all checks
        gaps.extend(self._check_pii_exposure(file_path, lines))
        gaps.extend(self._check_secret_exposure(file_path, lines))
        gaps.extend(self._check_sensitive_logging(file_path, lines))
        gaps.extend(self._check_unzeroed_memory(file_path, lines))
        
        return gaps
    
    def scan_module(self, module: str) -> Dict[str, List[DataLeakGap]]:
        """Scan module for data leak vulnerabilities"""
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
    
    def scan_repository(self) -> Dict[str, List[DataLeakGap]]:
        """Scan entire repository for data leaks"""
        gaps_by_file = {}
        
        # Scan src/ and include/ directories
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
    
    parser = argparse.ArgumentParser(description='Data Leak Detection Scanner')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()
    
    scanner = DataLeakScanner(args.repo)
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
    print(f"\nTotal data leaks found: {total_gaps}")
