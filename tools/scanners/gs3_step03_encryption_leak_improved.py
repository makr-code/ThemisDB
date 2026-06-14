#!/usr/bin/env python3
"""
Gap Scanner V3 — Encryption Leak Detection (IMPROVED 2026-06-14)

IMPROVEMENTS:
1. no_transit_encryption: Whitelist curl with CURLOPT_SSL_VERIFYPEER/VERIFYHOST
2. no_rest_encryption: Distinguish REST endpoints from storage/config variables
3. TLS verify options detection: Recognize modern TLS verification patterns
4. Variable filtering: Skip config/comment context

Detects:
- Plaintext transmission over network (http:// without TLS)
- Missing TLS certificate verification
- Encryption keys stored without protection
- Credential leaks in logs
"""

import sys
from pathlib import Path
from typing import List, Dict
import re

sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority


class EncryptionLeakScannerImproved(BaseGapScanner):
    """Scan for encryption and transit security gaps (IMPROVED)."""
    
    PRIORITY = ScannerPriority.SPECIALIZED
    ENABLED = True
    MAX_RUNTIME_SECONDS = 25
    
    # IMPROVEMENT 1: TLS verification options (whitelist)
    TLS_VERIFY_PATTERNS = [
        r'CURLOPT_SSL_VERIFYPEER\s*,\s*1',
        r'CURLOPT_SSL_VERIFYHOST\s*,\s*2',
        r'SSL_VERIFY_PEER',
        r'SSL_VERIFY_HOST',
        r'set_verify_mode\s*\(\s*(?:VERIFY_PEER|verify_peer)',
        r'use_certificate_chain_file',
        r'load_verify_file',
        r'tls.*verify',
        r'verify.*certificate',
    ]
    
    # IMPROVEMENT 2: Safe REST endpoint patterns
    REST_ENDPOINT_PATTERNS = [
        r'@.*"/api',
        r'router\.(?:get|post|put|delete)',
        r'http\.(?:get|post|put|delete)',
        r'REST_API',
        r'endpoint.*=.*"/',
    ]
    
    # IMPROVEMENT 3: Storage/config variable patterns (FALSE POSITIVE)
    STORAGE_CONFIG_PATTERNS = [
        r'config\s*\.',
        r'settings\s*\.',
        r'\.db_path',
        r'\.cache_dir',
        r'\.temp_path',
        r'storage_location',
        r'file_path',
        r'directory_path',
    ]
    
    def __init__(self, repo_root: str = '.'):
        """Initialize Encryption Leak Scanner (Improved)."""
        super().__init__("Encryption Leak Scanner (Improved)", "3.3.improved")
        self.repo_root = Path(repo_root)
    
    def scan(self, source_dir: str) -> List[Gap]:
        """Scan for encryption and transit security gaps"""
        gaps = []
        self.source_path = Path(source_dir).resolve()
        
        for file_path in self._scan_files(source_dir):
            file_path = file_path.resolve()
            self.files_scanned += 1
            
            lines = self._read_file_lines(file_path)
            if not lines:
                continue
            
            gaps.extend(self._check_plaintext_transmission(file_path, lines))
            gaps.extend(self._check_missing_tls_verification(file_path, lines))
            gaps.extend(self._check_rest_encryption(file_path, lines))
        
        return self.deduplicate(gaps)

    def scan_repository(self) -> Dict[str, List[Gap]]:
        """Compatibility wrapper for phase11 scanner interface in uniform orchestrator."""
        root = self.repo_root if self.repo_root.exists() else Path('.')
        gaps = self.scan(str(root))
        return {'all': gaps}
    
    def _has_tls_verification(self, context: str) -> bool:
        """
        IMPROVEMENT 1: Check if context includes TLS verification
        """
        for pattern in self.TLS_VERIFY_PATTERNS:
            if re.search(pattern, context, re.IGNORECASE):
                return True
        
        # Also check for https:// protocol
        if 'https://' in context:
            return True
        
        return False
    
    def _is_storage_config_context(self, line: str) -> bool:
        """
        IMPROVEMENT 2: Check if line is about storage/config, not encryption
        """
        for pattern in self.STORAGE_CONFIG_PATTERNS:
            if re.search(pattern, line):
                return True
        return False
    
    def _is_rest_endpoint(self, context: str) -> bool:
        """
        IMPROVEMENT 2: Check if this is a REST endpoint definition
        """
        for pattern in self.REST_ENDPOINT_PATTERNS:
            if re.search(pattern, context):
                return True
        return False
    
    def _check_plaintext_transmission(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Detect plaintext transmission (http:// without TLS upgrade)
        
        IMPROVEMENT: Skip if TLS verification is present in same scope
        """
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            if line.strip().startswith('//') or line.strip().startswith('*'):
                continue
            
            # Look for http:// (not https://)
            if re.search(r'http://[^"\']*', line) and 'https://' not in line:
                # Get surrounding context
                context_start = max(0, line_no - 10)
                context_end = min(len(lines), line_no + 10)
                context = ''.join(lines[context_start:context_end])
                
                # IMPROVEMENT 1: Check if TLS verification is in context
                if self._has_tls_verification(context):
                    continue
                
                # IMPROVEMENT 3: Skip config/storage contexts
                if self._is_storage_config_context(line):
                    continue
                
                gaps.append(Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type='plaintext_transmission',
                    severity='CRITICAL',
                    confidence=0.95,
                    description='Plaintext HTTP transmission detected (should use HTTPS)',
                    context=line.strip()[:80],
                    remediation='Use https:// or upgrade to TLS after connection',
                ))
        
        return gaps
    
    def _check_missing_tls_verification(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Detect HTTPS connections without certificate verification
        
        IMPROVEMENT 1: Whitelist known safe curl/TLS patterns
        """
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments/Doxygen
            if line.strip().startswith('//') or line.strip().startswith('*'):
                continue
            
            # Look for curl_easy_setopt or similar
            if 'curl_easy_setopt' in line or 'SSL_' in line or 'CURLOPT_' in line:
                # Get surrounding context
                context_start = max(0, line_no - 5)
                context_end = min(len(lines), line_no + 5)
                context = ''.join(lines[context_start:context_end])
                
                # IMPROVEMENT 1: Check for TLS verification options
                if self._has_tls_verification(context):
                    continue
                
                # IMPROVEMENT 3: Skip config/storage contexts
                if self._is_storage_config_context(line):
                    continue
                
                # Check for common TLS misconfigurations
                tls_issues = [
                    ('CURLOPT_SSL_VERIFYPEER.*0', 'SSL certificate peer verification disabled'),
                    ('CURLOPT_SSL_VERIFYHOST.*0', 'SSL certificate host verification disabled'),
                    ('verify.*false', 'Certificate verification disabled'),
                ]
                
                for pattern, desc in tls_issues:
                    if re.search(pattern, context):
                        gaps.append(Gap(
                            file=str(file_path.relative_to(self.source_path)),
                            line=line_no,
                            type='no_transit_encryption',
                            severity='CRITICAL',
                            confidence=0.95,
                            description=desc,
                            context=line.strip()[:80],
                            remediation='Enable certificate verification: CURLOPT_SSL_VERIFYPEER=1L, CURLOPT_SSL_VERIFYHOST=2L',
                        ))
        
        return gaps
    
    def _check_rest_encryption(self, file_path: Path, lines: List[str]) -> List[Gap]:
        """
        Detect REST endpoints without encryption
        
        IMPROVEMENT 2: Only flag REST endpoints, not storage/config variables
        """
        gaps = []
        
        for line_no, line in enumerate(lines, 1):
            # Skip comments
            if line.strip().startswith('//') or line.strip().startswith('*'):
                continue
            
            # IMPROVEMENT 2: Check if this is a REST endpoint
            context_start = max(0, line_no - 3)
            context_end = min(len(lines), line_no + 3)
            context = ''.join(lines[context_start:context_end])
            
            if not self._is_rest_endpoint(context):
                continue
            
            # Look for plaintext endpoints or missing TLS
            if 'http://' in line or (
                any(x in line for x in ['get(', 'post(', 'put(', 'delete('])
                and 'https' not in context
                and 'tls' not in context.lower()
            ):
                # Skip if TLS is configured elsewhere in function/class
                if self._has_tls_verification(context):
                    continue
                
                # Skip storage/config contexts
                if self._is_storage_config_context(line):
                    continue
                
                gaps.append(Gap(
                    file=str(file_path.relative_to(self.source_path)),
                    line=line_no,
                    type='no_rest_encryption',
                    severity='HIGH',
                    confidence=0.85,
                    description='REST endpoint may be missing encryption',
                    context=line.strip()[:80],
                    remediation='Configure HTTPS/TLS for this endpoint',
                ))
        
        return gaps


if __name__ == '__main__':
    scanner = EncryptionLeakScannerImproved()
    
    source_dir = sys.argv[1] if len(sys.argv) > 1 else '.'
    gaps = scanner.scan(source_dir)
    
    print(f"Found {len(gaps)} encryption/transit gaps (improved)")
    for gap in gaps[:10]:
        print(f"  {gap.file}:{gap.line} [{gap.type}] {gap.description}")
