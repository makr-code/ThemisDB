#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v3 — Enhanced Security Gaps Detection

Detects 12 security patterns across 3 categories:

S-1: Hardcoded Secrets Detection (CWE-798)
  - API Token Hardcoding (sk_live_, pk_live_, ghp_)
  - SSH Key Embedded (RSA/OpenSSH private keys)
  - Database Credentials Hardcoded (connection strings, mysql_connect)

S-2: Cryptographic Weakness Detection (CWE-327)
  - Weak Hash Algorithms (EVP_sha1, EVP_md5)
  - DES/3DES Cipher Usage (DES_set_key, EVP_des_cbc, EVP_des_ede3_cbc)
  - Fixed-Size XOR Encryption (xor_cipher, for loop XOR)
  - Weak Random Number Generators (rand, srand, std::random_device)

S-3: Injection Attack Prevention (CWE-94)
  - Command Injection (system, popen with user data)
  - Path Traversal (fopen, std::ifstream without normalization)
  - Template Injection (printf-style with user data as format)
  - ReDoS Vulnerability (std::regex with complex patterns)
  - XXE Vulnerability (XML parsing without entity resolution)
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional, Tuple
from enum import Enum


class SecurityGapType(Enum):
    """Security gap classifications"""
    HARDCODED_API_TOKEN = "hardcoded_api_token"           # S-1-1: API token (sk_live_, pk_live_, ghp_)
    HARDCODED_SSH_KEY = "hardcoded_ssh_key"               # S-1-2: SSH private key
    HARDCODED_DB_CREDENTIALS = "hardcoded_db_credentials" # S-1-3: Database credentials
    WEAK_HASH_ALGORITHM = "weak_hash_algorithm"           # S-2-1: SHA1, MD5
    WEAK_CIPHER_DES = "weak_cipher_des"                   # S-2-2: DES/3DES
    WEAK_XOR_ENCRYPTION = "weak_xor_encryption"           # S-2-3: XOR cipher
    WEAK_RANDOM_NUMBER = "weak_random_number"             # S-2-4: rand(), srand()
    COMMAND_INJECTION = "command_injection"               # S-3-1: system(), popen()
    PATH_TRAVERSAL = "path_traversal"                     # S-3-2: fopen(), std::ifstream
    TEMPLATE_INJECTION = "template_injection"             # S-3-3: printf-style format strings
    REDOS_VULNERABILITY = "redos_vulnerability"           # S-3-4: Complex regex patterns
    XXE_VULNERABILITY = "xxe_vulnerability"               # S-3-5: XXE in XML parsing


@dataclass
class SecurityGap:
    """Represents a security gap"""
    file_path: str
    line_num: int
    gap_type: SecurityGapType
    cwe_id: str
    snippet: str
    severity: str
    description: str
    remediation: str
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'type': self.gap_type.value,
            'cwe_id': self.cwe_id,
            'severity': self.severity,
            'snippet': self.snippet,
            'description': self.description,
            'remediation': self.remediation,
        }


class SecurityGapScanner:
    """Detect security vulnerabilities in C++ code with enhanced patterns"""
    
    S1_HARDCODED_SECRETS = {
        'api_token_sk_live': (
            re.compile(r'["\']?sk_live_[A-Za-z0-9]{20,}["\']?'),
            'CRITICAL',
            'CWE-798',
            'Hardcoded Stripe-like API key (sk_live_*)',
            'Move to environment variable or secure vault (AWS Secrets Manager, HashiCorp Vault)'
        ),
        'api_token_pk_live': (
            re.compile(r'["\']?pk_live_[A-Za-z0-9]{20,}["\']?'),
            'CRITICAL',
            'CWE-798',
            'Hardcoded Stripe-like Public key (pk_live_*)',
            'Move to environment variable or secure vault'
        ),
        'api_token_ghp': (
            re.compile(r'["\']?ghp_[A-Za-z0-9]{20,}["\']?'),
            'CRITICAL',
            'CWE-798',
            'Hardcoded GitHub Personal Access Token',
            'Use github secrets in CI/CD; use gh CLI auth for local work'
        ),
        'ssh_key_rsa': (
            re.compile(r'-----BEGIN RSA PRIVATE KEY-----'),
            'CRITICAL',
            'CWE-798',
            'Embedded RSA private key in source',
            'Never commit private keys; use SSH key management (ssh-agent, AWS Systems Manager)'
        ),
        'ssh_key_openssh': (
            re.compile(r'-----BEGIN OPENSSH PRIVATE KEY-----'),
            'CRITICAL',
            'CWE-798',
            'Embedded OpenSSH private key in source',
            'Never commit private keys; use SSH key management'
        ),
        'db_connection_string': (
            re.compile(r'(mysql_connect|PostgreSQLConnection|mongodb://)\s*\(\s*["\'][^"\']*:[^"\']*@'),
            'CRITICAL',
            'CWE-798',
            'Database credentials hardcoded in connection string',
            'Use connection pooling with environment variables or managed secrets'
        ),
        'db_password_literal': (
            re.compile(r'(password|passwd|pwd)\s*=\s*["\'](?!.*\$\{|.*\benv\b)[^"\']{6,}["\']'),
            'CRITICAL',
            'CWE-798',
            'Database password hardcoded as literal string',
            'Store in environment variable or secure vault'
        ),
    }
    
    S2_CRYPTO_WEAKNESS = {
        'weak_hash_sha1': (
            re.compile(r'EVP_sha1\s*\(|EVP_get_digestbyname\s*\(\s*["\']sha1["\']|SHA1_Init'),
            'CRITICAL',
            'CWE-327',
            'Weak hash algorithm SHA1 used in cryptographic context',
            'Use SHA-256 or stronger (EVP_sha256, EVP_sha512, BLAKE2)'
        ),
        'weak_hash_md5': (
            re.compile(r'EVP_md5\s*\(|EVP_get_digestbyname\s*\(\s*["\']md5["\']|MD5_Init'),
            'CRITICAL',
            'CWE-327',
            'Weak hash algorithm MD5 used in cryptographic context',
            'Use SHA-256 or stronger (EVP_sha256, EVP_sha512, BLAKE2)'
        ),
        'weak_cipher_des_set_key': (
            re.compile(r'DES_set_key|DES_ecb_encrypt|DES_cbc_encrypt'),
            'CRITICAL',
            'CWE-327',
            'Weak DES cipher used; compromised by brute force attacks',
            'Use AES-256-GCM or ChaCha20-Poly1305 (EVP_aes_256_gcm)'
        ),
        'weak_cipher_des_cbc': (
            re.compile(r'EVP_des_cbc\s*\(|EVP_des_ede_cbc\s*\(|EVP_des_ede3_cbc\s*\('),
            'CRITICAL',
            'CWE-327',
            'Weak DES or 3DES cipher mode (only 56/168-bit effective key)',
            'Use AES-256-GCM or ChaCha20-Poly1305'
        ),
        'custom_xor_cipher_func': (
            re.compile(r'(xor_cipher|xor_encrypt|simple_xor)\s*\('),
            'CRITICAL',
            'CWE-327',
            'Custom XOR cipher implementation (cryptographically broken)',
            'Use standard library: OpenSSL EVP_* or libsodium'
        ),
        'custom_xor_loop': (
            re.compile(r'for\s*\([^)]*[;][^)]*\)\s*[\w\[\]]+\s*\^=\s*key|buf\[i\]\s*\^=\s*key'),
            'CRITICAL',
            'CWE-327',
            'Primitive XOR encryption loop (cryptographically weak)',
            'Use standard library: OpenSSL EVP_* or libsodium'
        ),
        'weak_rand': (
            re.compile(r'\brand\s*\(|srand\s*\(|\bstd::rand\b'),
            'CRITICAL',
            'CWE-327',
            'Weak PRNG (rand/srand) used for cryptographic key/nonce generation',
            'Use std::random_device with std::mt19937_64 or better: OpenSSL RAND_* or libsodium randombytes'
        ),
        'weak_random_device_only': (
            re.compile(r'std::random_device\s+\w+(?!\s*,|;)(?=[;\)\]]|$)'),
            'HIGH',
            'CWE-327',
            'std::random_device alone used for crypto (may not have sufficient entropy)',
            'Seed a CSPRNG: combine with std::mt19937_64 or use OpenSSL/libsodium for crypto keys'
        ),
    }
    
    S3_INJECTION_ATTACKS = {
        'command_injection_system': (
            re.compile(r'system\s*\(\s*["\'].*(%s|sprintf|format|getenv)'),
            'CRITICAL',
            'CWE-94',
            'Command injection risk: system() called with user-influenced string',
            'Avoid system(); use execve() with argument array or safe library'
        ),
        'command_injection_popen': (
            re.compile(r'popen\s*\(\s*["\'].*(%s|sprintf|format|getenv)'),
            'CRITICAL',
            'CWE-94',
            'Command injection risk: popen() called with unsanitized user input',
            'Use posix_spawn() or std::subprocess with explicit argument vector'
        ),
        'command_injection_system_var': (
            re.compile(r'system\s*\(\s*\w+\)|popen\s*\(\s*\w+\s*,'),
            'HIGH',
            'CWE-94',
            'Command injection risk: user-controlled variable passed to system/popen',
            'Never pass user input directly; use posix_spawn or subprocess library'
        ),
        'path_traversal_fopen': (
            re.compile(r'fopen\s*\(\s*(user_\w+|input|path|filename|\w+_from_user)'),
            'HIGH',
            'CWE-22',
            'Path traversal vulnerability: fopen() with user-supplied path',
            'Normalize and validate path (realpath, check basename, use whitelist)'
        ),
        'path_traversal_ifstream': (
            re.compile(r'std::ifstream\s*\(\s*(user_\w+|input|path|filename|\w+_from_user)'),
            'HIGH',
            'CWE-22',
            'Path traversal vulnerability: std::ifstream with user-supplied path',
            'Validate path canonicalization and enforce base directory containment'
        ),
        'path_traversal_fopen_var': (
            re.compile(r'fopen\s*\(["\'].*%s|fopen\s*\(\s*\w+_path'),
            'HIGH',
            'CWE-22',
            'Path traversal risk: fopen() with potentially unsanitized path variable',
            'Use path normalization (realpath) and parent directory traversal checks'
        ),
        'format_string_printf_user': (
            re.compile(r'(printf|fprintf|sprintf|snprintf|syslog)\s*\(\s*(?![\"\'])[A-Za-z_]\w*|printf\s*\(\s*["\']%s["\'],\s*user_'),
            'HIGH',
            'CWE-134',
            'Format string vulnerability: user input as format string to printf-like function',
            'Always use fixed format string: printf("%s", user_input) not printf(user_input)'
        ),
        'format_string_syslog': (
            re.compile(r'syslog\s*\(\s*[^,]+,\s*(?![\"\'])[A-Za-z_]\w*'),
            'HIGH',
            'CWE-134',
            'Format string vulnerability in syslog call',
            'Never pass user data as format string; use syslog(pri, "%s", user_data)'
        ),
        'redos_regex_complex': (
            re.compile(r'std::regex\s*\(\s*["\'](?:.*\(\?:.*\*.*\)|.*\{.*,.*\}|.*\+\+.*\).*\+)'),
            'HIGH',
            'CWE-1333',
            'ReDoS vulnerability: complex regex pattern vulnerable to exponential backtracking',
            'Simplify regex (avoid nested quantifiers); use timeout; profile with untrusted input'
        ),
        'redos_regex_user_input': (
            re.compile(r'std::regex(?:\s*\(|\s*=)\s*(?![\"\'])[A-Za-z_]\w*|std::regex_match\s*\(\s*\w+,\s*(?![\"\'])[A-Za-z_]\w*'),
            'HIGH',
            'CWE-1333',
            'ReDoS risk: std::regex constructed from user input',
            'Validate/escape user regex; use simpler string matching; set regex timeout'
        ),
        'xxe_parsing_libxml2': (
            re.compile(r'xmlParseFile|xmlParseMemory|xmlReadFile(?!.*XML_PARSE_NOENT.*false|.*setFeature.*disallowDoctypeDecl)'),
            'CRITICAL',
            'CWE-611',
            'XXE vulnerability: XML parsing without disabling external entity resolution',
            'Set parser options: xmlParserSetFeature(parser, XML_PARSE_NOENT, 0); disable DTD: xmlParserSetFeature(parser, XML_PARSE_DTDVALID, 0)'
        ),
        'xxe_parsing_rapidxml': (
            re.compile(r'rapidxml::xml_document\s*<|parse\s*\(\s*[^,]+\s*\)|xml_document\s*\(\s*\)(?!.*rapidxml::parse_no_entity_expansion)'),
            'CRITICAL',
            'CWE-611',
            'XXE vulnerability: RapidXML parsing without entity expansion disabled',
            'Use parse flag: parse_no_entity_expansion to disable external entity processing'
        ),
    }
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: Dict[str, List[SecurityGap]] = {}
    
    def _extract_snippet(self, line: str, max_len: int = 120) -> str:
        """Extract and sanitize code snippet for display"""
        snippet = line.strip()
        if len(snippet) > max_len:
            snippet = snippet[:max_len-3] + '...'
        return snippet
    
    def _has_escaping_guard(self, lines: List[str], line_idx: int, var_name: str) -> bool:
        """Detect if user input variable has prior escaping/validation"""
        start = max(0, line_idx - 10)
        context = '\n'.join(lines[start:line_idx])
        
        validation_patterns = [
            rf'({re.escape(var_name)}\s*=\s*.*sanitize|{re.escape(var_name)}\s*=\s*.*escape|{re.escape(var_name)}\s*=\s*.*quote)',
            rf'if\s*\(\s*validate\s*\(\s*{re.escape(var_name)}|if\s*\(\s*is_safe\s*\(\s*{re.escape(var_name)}',
        ]
        return any(re.search(p, context, re.IGNORECASE) for p in validation_patterns)
    
    def _is_in_comment_or_test(self, line: str, file_path: Path) -> bool:
        """Skip comments and test/example code"""
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            return True
        if 'TEST' in line or 'MOCK' in line or 'EXAMPLE' in line:
            return True
        if 'test' in file_path.name.lower() or 'example' in file_path.name.lower():
            return True
        return False
    
    def scan_file(self, file_path: Path) -> List[SecurityGap]:
        """Scan single file for security gaps"""
        gaps = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return gaps
        
        for line_num, line in enumerate(lines, 1):
            if self._is_in_comment_or_test(line, file_path):
                continue
            
            # S-1: Hardcoded Secrets Detection
            for pattern_name, (pattern, severity, cwe_id, desc, remediation) in self.S1_HARDCODED_SECRETS.items():
                if pattern.search(line):
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=self._get_gap_type_s1(pattern_name),
                        cwe_id=cwe_id,
                        snippet=self._extract_snippet(line),
                        severity=severity,
                        description=desc,
                        remediation=remediation
                    )
                    gaps.append(gap)
            
            # S-2: Cryptographic Weakness Detection
            for pattern_name, (pattern, severity, cwe_id, desc, remediation) in self.S2_CRYPTO_WEAKNESS.items():
                if pattern.search(line):
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=self._get_gap_type_s2(pattern_name),
                        cwe_id=cwe_id,
                        snippet=self._extract_snippet(line),
                        severity=severity,
                        description=desc,
                        remediation=remediation
                    )
                    gaps.append(gap)
            
            # S-3: Injection Attack Prevention
            for pattern_name, (pattern, severity, cwe_id, desc, remediation) in self.S3_INJECTION_ATTACKS.items():
                if pattern.search(line):
                    if self._has_escaping_guard(lines, line_num - 1, 'user_'):
                        continue
                    
                    gap = SecurityGap(
                        file_path=str(file_path.relative_to(self.repo_root)),
                        line_num=line_num,
                        gap_type=self._get_gap_type_s3(pattern_name),
                        cwe_id=cwe_id,
                        snippet=self._extract_snippet(line),
                        severity=severity,
                        description=desc,
                        remediation=remediation
                    )
                    gaps.append(gap)
        
        return gaps
    
    @staticmethod
    def _get_gap_type_s1(pattern_name: str) -> SecurityGapType:
        """Map S-1 pattern names to gap types"""
        if 'ssh_key' in pattern_name:
            return SecurityGapType.HARDCODED_SSH_KEY
        elif 'db_' in pattern_name or 'database' in pattern_name:
            return SecurityGapType.HARDCODED_DB_CREDENTIALS
        else:
            return SecurityGapType.HARDCODED_API_TOKEN
    
    @staticmethod
    def _get_gap_type_s2(pattern_name: str) -> SecurityGapType:
        """Map S-2 pattern names to gap types"""
        if 'hash' in pattern_name:
            return SecurityGapType.WEAK_HASH_ALGORITHM
        elif 'des' in pattern_name or 'cipher' in pattern_name:
            return SecurityGapType.WEAK_CIPHER_DES
        elif 'xor' in pattern_name:
            return SecurityGapType.WEAK_XOR_ENCRYPTION
        else:
            return SecurityGapType.WEAK_RANDOM_NUMBER
    
    @staticmethod
    def _get_gap_type_s3(pattern_name: str) -> SecurityGapType:
        """Map S-3 pattern names to gap types"""
        if 'command_injection' in pattern_name:
            return SecurityGapType.COMMAND_INJECTION
        elif 'path_traversal' in pattern_name:
            return SecurityGapType.PATH_TRAVERSAL
        elif 'format_string' in pattern_name:
            return SecurityGapType.TEMPLATE_INJECTION
        elif 'redos' in pattern_name:
            return SecurityGapType.REDOS_VULNERABILITY
        elif 'xxe' in pattern_name:
            return SecurityGapType.XXE_VULNERABILITY
        else:
            return SecurityGapType.TEMPLATE_INJECTION
    
    def scan_module(self, module: str) -> Dict[str, List[SecurityGap]]:
        """Scan module for security gaps"""
        gaps_by_file = {}
        
        src_dir = self.repo_root / 'src' / module
        include_dir = self.repo_root / 'include' / module
        
        for directory in [src_dir, include_dir]:
            if not directory.exists():
                continue
            
            cpp_files = list(directory.rglob('*.cpp'))
            hpp_files = list(directory.rglob('*.hpp'))
            h_files = list(directory.rglob('*.h'))
            
            for file_path in cpp_files + hpp_files + h_files:
                gaps = self.scan_file(file_path)
                if gaps:
                    gaps_by_file[str(file_path.relative_to(self.repo_root))] = gaps
        
        return gaps_by_file
    
    def run_full_scan(self, output_dir: str = 'ai_working') -> Dict[str, any]:
        """Scan all modules for security gaps"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print("\n[...] Scanning for ENHANCED SECURITY GAPS...")
        print("    S-1: Hardcoded Secrets (CWE-798)")
        print("    S-2: Cryptographic Weakness (CWE-327)")
        print("    S-3: Injection Attacks (CWE-94, CWE-22, CWE-134, CWE-1333, CWE-611)")
        
        src_root = self.repo_root / 'src'
        if not src_root.exists():
            print("[WARN] No src/ directory found")
            return {}
        
        modules = sorted([d.name for d in src_root.iterdir() if d.is_dir()])
        
        aggregate = {}
        total_all_gaps = 0
        total_critical = 0
        total_high = 0
        
        for module in modules:
            gaps_by_file = self.scan_module(module)
            total_gaps = sum(len(g) for g in gaps_by_file.values())
            
            if total_gaps > 0:
                print(f"   {module:30} {total_gaps:4} gaps")
                total_all_gaps += total_gaps
                
                gap_counts = {}
                severity_counts = {}
                cwe_counts = {}
                
                for gaps in gaps_by_file.values():
                    for gap in gaps:
                        gap_type = gap.gap_type.value
                        gap_counts[gap_type] = gap_counts.get(gap_type, 0) + 1
                        
                        sev = gap.severity
                        severity_counts[sev] = severity_counts.get(sev, 0) + 1
                        if sev == 'CRITICAL':
                            total_critical += 1
                        elif sev == 'HIGH':
                            total_high += 1
                        
                        cwe = gap.cwe_id
                        cwe_counts[cwe] = cwe_counts.get(cwe, 0) + 1
                
                aggregate[module] = {
                    'total': total_gaps,
                    'severity_critical': severity_counts.get('CRITICAL', 0),
                    'severity_high': severity_counts.get('HIGH', 0),
                    'severity_medium': severity_counts.get('MEDIUM', 0),
                    'by_type': gap_counts,
                    'by_cwe': cwe_counts,
                    'gaps_by_file': {
                        f: [g.to_dict() for g in gaps]
                        for f, gaps in gaps_by_file.items()
                    }
                }
        
        with open(output_path / 'gap_scan_v3_security_enhanced.json', 'w') as f:
            json.dump(aggregate, f, indent=2)
        
        print(f"\n[OK] Enhanced security scan complete. Results: {output_dir}/gap_scan_v3_security_enhanced.json")
        
        return {
            'aggregate': aggregate,
            'totals': {
                'total_gaps': total_all_gaps,
                'critical': total_critical,
                'high': total_high,
            }
        }


if __name__ == '__main__':
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    
    print("[INFO] ThemisDB Gap Scanner v3 — Enhanced Security Patterns")
    print("=" * 70)
    
    scanner = SecurityGapScanner(repo_root)
    results = scanner.run_full_scan(output_dir)
    
    if results and 'totals' in results:
        totals = results['totals']
        print(f"\n[SUMMARY] Enhanced Security Gaps:")
        print(f"   Total: {totals['total_gaps']}")
        print(f"   CRITICAL: {totals['critical']}")
        print(f"   HIGH: {totals['high']}")
        print("\n   S-1: Hardcoded Secrets (CWE-798)")
        print("   S-2: Cryptographic Weakness (CWE-327)")
        print("   S-3: Injection Attacks (CWE-94/22/134/1333/611)")
    
    print("\n" + "=" * 70)
