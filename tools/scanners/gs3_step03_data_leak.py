#!/usr/bin/env python3
"""
Gap Scanner V3 — Phase 11 Data Leak Detection (Migrated to OOP)

Detects data leak vulnerabilities:
- Hardcoded PII (SSN, credit card, phone numbers)
- Sensitive logging (passwords, tokens, API keys)
- Unencrypted sensitive data storage
- Credentials in config files
"""

import sys
from pathlib import Path
from typing import List
import re

sys.path.insert(0, str(Path(__file__).parent.parent))
from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class DataLeakScanner(BaseGapScanner):
    """Detect data leak vulnerabilities in C++ code."""
    
    PRIORITY = ScannerPriority.SPECIALIZED
    ENABLED = True
    MAX_RUNTIME_SECONDS = 60
    
    def __init__(self):
        """Initialize Data Leak Scanner."""
        super().__init__("Data Leak Detection", "3.1")
        
        # PII patterns
        self.pii_patterns = {
            'ssn': (re.compile(r'\b\d{3}[-\.]?\d{2}[-\.]?\d{4}\b'), 'SSN detected'),
            'credit_card': (re.compile(r'\b\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}\b'), 'Credit card detected'),
            'phone': (re.compile(r'\b[\(\[]?[0-9]{3}[\)\]]?[-.\s]?[0-9]{3}[-.\s]?[0-9]{4}\b'), 'Phone number'),
        }
        
        # Hardcoded secrets
        self.secret_patterns = {
            'api_key': (re.compile(r'(?:api[_-]?key|apikey)\s*=\s*["\']([a-zA-Z0-9_-]{20,})["\']', re.IGNORECASE), 'API key'),
            'password': (re.compile(r'(?:password|passwd|pwd)\s*=\s*["\']([^"\']{4,})["\']', re.IGNORECASE), 'Password'),
            'token': (re.compile(r'(?:token|access_token)\s*=\s*["\']([a-zA-Z0-9_.-]{20,})["\']', re.IGNORECASE), 'Token'),
            'aws_key': (re.compile(r'AKIA[0-9A-Z]{16}'), 'AWS access key'),
        }
        
        # Test data whitelist
        self.test_patterns = [
            r'test.*\d{3}-\d{2}-\d{4}',
            r'1234[-\s]?4567[-\s]?8901[-\s]?2345',
            r'TEST_.*|DEMO_.*',
        ]
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan for data leak vulnerabilities."""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()
            self.files_scanned += 1
            
            lines = self._read_file_lines(file_path)
            if not lines:
                continue
            
            # Skip test files
            if 'test' in file_path.name.lower():
                continue
            
            gaps.extend(self._check_pii(file_path, lines))
            gaps.extend(self._check_secrets(file_path, lines))
            gaps.extend(self._check_sensitive_logging(file_path, lines))
        
        return self.deduplicate(gaps)
    
    def _check_pii(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect hardcoded PII."""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            if line.strip().startswith('//'):
                continue
            
            for pii_type, (pattern, desc) in self.pii_patterns.items():
                match = pattern.search(line)
                if match and not self._is_test_data(line):
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type=f"pii_{pii_type}",
                        severity="CRITICAL",
                        confidence=0.85,
                        description=f"Hardcoded {desc} detected",
                        remediation="Remove PII from source code; use secure credential management",
                        context=line.strip(),
                        scanner=self.name,
                        step="03_data_leak"
                    ))
        
        return gaps
    
    def _check_secrets(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect hardcoded secrets."""
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            if line.strip().startswith('//'):
                continue
            
            for secret_type, (pattern, desc) in self.secret_patterns.items():
                match = pattern.search(line)
                if match and not self._is_test_data(line):
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type=f"hardcoded_{secret_type}",
                        severity="CRITICAL",
                        confidence=0.80,
                        description=f"Hardcoded {desc} detected",
                        remediation="Remove secret from source; use env vars or secret manager",
                        context=line.strip(),
                        scanner=self.name,
                        step="03_data_leak"
                    ))
        
        return gaps
    
    def _check_sensitive_logging(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """Detect sensitive data in logging statements."""
        gaps = []
        
        sensitive_keywords = ['password', 'token', 'secret', 'key', 'apikey', 'credential']
        
        for line_no, line in enumerate(lines, 1):
            # Check for logging calls with sensitive data
            if any(kw in line.lower() for kw in sensitive_keywords):
                if any(log_fn in line for log_fn in ['printf', 'cout', 'LOG', 'SPDLOG', 'logger']):
                    gaps.append(Gap(
                        file=str(file_path.relative_to(self.source_path)),
                        line=line_no,
                        type="sensitive_logging",
                        severity="HIGH",
                        confidence=0.70,
                        description="Sensitive data logged unmasked",
                        remediation="Mask sensitive fields in logs (password=***, token=***...)",
                        context=line.strip(),
                        scanner=self.name,
                        step="03_data_leak"
                    ))
        
        return gaps
    
    def _is_test_data(self, line: str) -> bool:
        """Check if line contains test data patterns."""
        for pattern in self.test_patterns:
            if re.search(pattern, line, re.IGNORECASE):
                return True
        return False


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source_dir>")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    scanner = DataLeakScanner()
    
    print(f"[{scanner.name}] Starting scan...\n")
    gaps = scanner.scan(source_dir)
    
    by_severity = {}
    for gap in gaps:
        by_severity[gap.severity] = by_severity.get(gap.severity, 0) + 1
    
    print(f"\nFound {len(gaps)} data leak gaps in {scanner.files_scanned} files")
    
    if by_severity:
        print(f"\n  {', '.join(f'{sev}: {count}' for sev, count in sorted(by_severity.items()))}")
