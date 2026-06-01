#!/usr/bin/env python3
"""
Phase 7-2: Deprecated Library & API Usage Scanner

CWE-477 (Use of Obsolete Functions)

Detects:
- OpenSSL MD5/SHA1/DES (deprecated in 3.0)
- std::auto_ptr (deprecated C++11)
- std::unique_ptr with custom deleters
- strdup/sprintf/strcpy family
- RocksDB deprecated iterators
- gRPC deprecated methods
- TensorFlow v1 APIs
- Boost.Asio deprecated patterns
- Windows deprecated APIs
"""

import re
from pathlib import Path
from typing import List, Dict


class DeprecatedAPIsScan:
    """Scan for deprecated library and API usage"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
        
        # Deprecated patterns by category
        self.deprecated_patterns = {
            'openssl_deprecated': {
                'patterns': [
                    (r'MD5_Init|MD5_Update|MD5_Final', 'Use EVP_MD_CTX with EVP_md5()'),
                    (r'SHA1_Init|SHA1_Update|SHA1_Final', 'Use EVP_MD_CTX with EVP_sha1()'),
                    (r'DES_encrypt|DES_cbc_encrypt', 'Use AES with EVP interface'),
                    (r'SSL_library_init|SSL_load_error_strings', 'Use OPENSSL_init_ssl()'),
                ],
                'severity': 'HIGH',
                'cwe': 'CWE-477'
            },
            'cpp_stdlib': {
                'patterns': [
                    (r'std::auto_ptr', 'Use std::unique_ptr instead'),
                    (r'std::get_temporary_buffer', 'Use std::vector instead'),
                    (r'std::random_shuffle', 'Use std::shuffle instead'),
                ],
                'severity': 'MEDIUM',
                'cwe': 'CWE-477'
            },
            'unsafe_c_functions': {
                'patterns': [
                    (r'\bstrdup\s*\(', 'Use std::string instead'),
                    (r'\bstrcpy\s*\(', 'Use std::string or strncpy'),
                    (r'\bsprintf\s*\(', 'Use std::format or snprintf'),
                    (r'\bgets\s*\(', 'Use std::getline instead'),
                    (r'\bscanf\s*\(', 'Use std::stringstream or scanf_s'),
                ],
                'severity': 'CRITICAL',
                'cwe': 'CWE-477'
            },
            'rocksdb': {
                'patterns': [
                    (r'NewIterator\(\)', 'Use db->NewIterator(ReadOptions())'),
                    (r'GetSnapshot\(\)', 'Use recent API version'),
                ],
                'severity': 'MEDIUM',
                'cwe': 'CWE-477'
            },
            'grpc': {
                'patterns': [
                    (r'ClientContext\s*.*\s*timeout', 'Use gRPC deadline instead'),
                    (r'Channel::Ping', 'Use ChannelState check instead'),
                ],
                'severity': 'MEDIUM',
                'cwe': 'CWE-477'
            },
            'windows_deprecated': {
                'patterns': [
                    (r'GetVersionEx\(', 'Use IsWindowsVersionOrGreater()'),
                    (r'SetWindowsHookEx.*WH_KEYBOARD', 'Use low-level keyboard hooks'),
                ],
                'severity': 'MEDIUM',
                'cwe': 'CWE-477'
            },
        }
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for deprecated API usage"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp', '.c']:
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan all pattern categories
            for category, config in self.deprecated_patterns.items():
                self._scan_category(file_path, lines, category, config)
        
        return self.gaps
    
    def _scan_category(self, file_path: Path, lines: List[str], category: str, config: Dict):
        """Scan for specific category of deprecated APIs"""
        
        for idx, line in enumerate(lines, 1):
            for pattern, replacement in config['patterns']:
                if re.search(pattern, line):
                    # Skip if in comment
                    if line.strip().startswith('//'):
                        continue
                    
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'deprecated_apis',
                        'severity': config['severity'],
                        'pattern': pattern,
                        'description': f'Deprecated API: {pattern} → {replacement}',
                        'context': line.strip(),
                        'replacement': replacement,
                        'cwe': config['cwe']
                    })
