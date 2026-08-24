# Phase 1-4 Scanner Improvements — Detection Pattern Enhancements

**Date:** 2026-05-19  
**Status:** PLANNING PHASE  
**Target:** Q3 2026 (concurrent with Phase 6 development)  
**Estimated Additional LOC:** 400–500 across 8 existing scanners  
**Expected Gap Increase:** +2,200–3,200 (7–10% more than Phase 1-4 baseline)

---

## Overview

Phase 1-4 improvements add new detection patterns to existing scanners, increasing gap detection sensitivity and addressing patterns not originally covered. These improvements are lower-complexity than Phase 6 scanners but provide significant gap increase.

---

## Security Scanner Improvements (Phase 1-4 → Enhanced)

### S-1: Hardcoded Secrets Detection (NEW PATTERN)
**CWE-798: Use of Hard-Coded Credentials**

**Current Detection (Phase 1-4):** `password` variable assignments, string literals containing "KEY"/"SECRET"

**Enhanced Patterns:**
1. API Token Hardcoding
   - Pattern: `const char* token = "sk_live_...",` or `const std::string API_KEY = "..."`
   - Severity: 🔴 CRITICAL
   - Scope: All string literals in .cpp files, const initializers
   - Expected new gaps: 40–60 per 10K LOC

2. SSH Key Embedded
   - Pattern: `BEGIN RSA PRIVATE KEY` or `BEGIN OPENSSH PRIVATE KEY` in source
   - Severity: 🔴 CRITICAL
   - Scope: String literals, char arrays
   - Expected new gaps: 20–30

3. Database Credentials Hardcoded
   - Pattern: `postgres://user:pass@host` in connection strings, `mysql_connect("localhost", "root", "password")`
   - Severity: 🔴 CRITICAL
   - Scope: Function calls with string arguments
   - Expected new gaps: 30–50

4. Certificate/Key Material
   - Pattern: `-----BEGIN CERTIFICATE-----` or `-----BEGIN PRIVATE KEY-----`
   - Severity: 🔴 CRITICAL
   - Scope: Multi-line string literals
   - Expected new gaps: 10–20

**Estimated Additional Gaps:** 100–160 (Phase 1-4 + 5 new patterns = 1,614 → 1,714)

---

### S-2: Cryptographic Weakness Detection (NEW PATTERN)
**CWE-327: Use of a Broken/Risky Cryptographic Algorithm**

**Current Detection:** Direct detection of `MD5_Init`, `SHA1_Init` calls

**Enhanced Patterns:**
1. Weak Hash Algorithms in Legacy Code
   - Pattern: `EVP_sha1()`, `EVP_md5()` in OpenSSL context
   - Severity: 🟠 HIGH
   - Scope: OpenSSL API calls
   - Expected new gaps: 15–25

2. DES/3DES Cipher Usage
   - Pattern: `DES_set_key`, `EVP_des_cbc()`, `EVP_des_ede3_cbc()`
   - Severity: 🟠 HIGH
   - Scope: Cryptographic initialization
   - Expected new gaps: 10–20

3. Fixed-Size XOR Encryption
   - Pattern: Custom `xor_cipher()` implementations or `for (i = 0; i < size; ++i) buf[i] ^= key;`
   - Severity: 🔴 CRITICAL
   - Scope: Custom crypto implementations
   - Expected new gaps: 20–35

4. Weak Random Number Generators
   - Pattern: `rand()`, `srand()` for cryptographic purposes, `std::random_device` used directly
   - Severity: 🟠 HIGH
   - Scope: RNG initialization for security context
   - Expected new gaps: 25–40

**Estimated Additional Gaps:** 70–120 (Phase 1-4 + 4 new patterns = 1,514 → 1,584)

---

### S-3: Injection Attack Prevention (NEW PATTERN)
**CWE-94: Improper Control of Generation of Code (Code Injection)**

**Current Detection:** SQL injection patterns with basic string concatenation

**Enhanced Patterns:**
1. Command Injection via Shell Execution
   - Pattern: `system(user_input)`, `popen(formatted_string, "r")` with user data
   - Severity: 🔴 CRITICAL
   - Scope: Shell execution functions
   - Expected new gaps: 25–40

2. Path Traversal in File Operations
   - Pattern: `fopen(user_path)`, `std::ifstream(user_file)` without normalization
   - Severity: 🔴 CRITICAL
   - Scope: File I/O with user-provided paths
   - Expected new gaps: 30–50

3. Template Injection (SSTI equivalent in C++)
   - Pattern: String formatting with unvalidated format strings in logs/output
   - Severity: 🟠 HIGH
   - Scope: Printf-style functions with user data as format
   - Expected new gaps: 15–25

4. Regular Expression Denial of Service (ReDoS)
   - Pattern: `std::regex` with complex patterns and user input without timeout
   - Severity: 🟠 HIGH
   - Scope: Regex compilation from user input
   - Expected new gaps: 10–20

5. XML External Entity (XXE) Vulnerabilities
   - Pattern: XML parsing without disabling external entity resolution
   - Severity: 🟠 HIGH
   - Scope: XML library calls (libxml2, rapidxml, etc.)
   - Expected new gaps: 12–18

**Estimated Additional Gaps:** 92–153 (Phase 1-4 + 5 new patterns = 1,514 → 1,606)

---

## Memory Safety Scanner Improvements (Phase 1-4 → Enhanced)

### M-1: Use-After-Free Detection (NEW PATTERN)
**CWE-416: Use After Free**

**Current Detection:** Variable reference after `delete` statement

**Enhanced Patterns:**
1. Iterator Invalidation After Container Modification
   - Pattern: `auto it = v.begin(); v.push_back(...); use(*it);`
   - Severity: 🔴 CRITICAL
   - Scope: Container operation sequences
   - Expected new gaps: 40–60

2. Pointer to Temporary Object
   - Pattern: `int* p = &SomeFunc().member;` returning temporary
   - Severity: 🔴 CRITICAL
   - Scope: Taking address of temporary
   - Expected new gaps: 35–50

3. Use After std::move (Overlaps with P6-5)
   - Pattern: `T t; use(t); T u = std::move(t); use(t);` (not caught by lifetime)
   - Severity: 🔴 CRITICAL
   - Scope: Moved-from object usage
   - Expected new gaps: 50–70

**Estimated Additional Gaps:** 125–180 (Phase 1-4 + 3 new patterns = 2,227 → 2,352)

---

### M-2: Double-Free Detection (NEW PATTERN)
**CWE-415: Double Free**

**Current Detection:** `delete ptr; ... delete ptr;` sequential in same scope

**Enhanced Patterns:**
1. Double-Free in Exception Paths
   - Pattern: `delete ptr;` in try block and `catch { delete ptr; }`
   - Severity: 🔴 CRITICAL
   - Scope: Exception handling paths
   - Expected new gaps: 20–35

2. Double-Free in Loop Clearing
   - Pattern: `for (auto p : collection) delete p;` then `collection.clear()` or destructor re-deletes
   - Severity: 🔴 CRITICAL
   - Scope: Resource cleanup sequences
   - Expected new gaps: 15–25

**Estimated Additional Gaps:** 35–60 (Phase 1-4 + 2 new patterns = 2,227 → 2,262)

---

## Concurrency Scanner Improvements (Phase 1-4 → Enhanced)

### C-1: Race Condition Detection Enhancement (NEW PATTERN)
**CWE-362: Concurrent Access to Critical Section**

**Current Detection:** Unsynchronized access to non-atomic variables across threads

**Enhanced Patterns:**
1. Read-Check-Write Races (TOCTOU)
   - Pattern: `if (file_exists(path)) { process_file(path); }` without lock
   - Severity: 🔴 CRITICAL (security) / 🟠 HIGH (logic)
   - Scope: File system operations, configuration reads
   - Expected new gaps: 25–40

2. Double-Checked Locking
   - Pattern: `if (!init_done) { lock_guard<mutex> l(m); if (!init_done) { ... } }`
   - Severity: 🟠 HIGH (may work but risky)
   - Scope: Initialization patterns
   - Expected new gaps: 15–25

3. Lost Wakeup in Condition Variables
   - Pattern: Check condition before `wait()` without lock guarantee
   - Severity: 🔴 CRITICAL
   - Scope: Condition variable usage patterns
   - Expected new gaps: 20–30

**Estimated Additional Gaps:** 60–95 (Phase 1-4 + 3 new patterns = 1,834 → 1,894)

---

## Implementation Strategy

### Phase 1-4 Enhanced Scanner Updates

```python
# Example enhancement to gap_scanner_v3_security.py

class SecurityScannerEnhanced(SecurityScanner):
    
    def scan_hardcoded_secrets(self, content: str, file_path: str) -> List[Gap]:
        """Enhanced: Add API token, SSH key, database credential patterns"""
        gaps = super().scan_hardcoded_secrets(content, file_path)
        
        # NEW: API token pattern
        api_token_pattern = r'(?:sk_live|pk_live|ghp_)[A-Za-z0-9_]{20,}'
        for match in re.finditer(api_token_pattern, content):
            gaps.append(Gap(
                type=GapType.SECURITY,
                severity=Severity.CRITICAL,
                line=self._line_number(content, match.start()),
                description="Hardcoded API token detected",
                cwe="CWE-798"
            ))
        
        # NEW: SSH key pattern
        ssh_pattern = r'-----BEGIN (?:RSA )?PRIVATE KEY-----'
        if re.search(ssh_pattern, content):
            gaps.append(Gap(
                type=GapType.SECURITY,
                severity=Severity.CRITICAL,
                line=self._line_number(content, re.search(ssh_pattern, content).start()),
                description="SSH private key material in source",
                cwe="CWE-798"
            ))
        
        return gaps
```

### Rollout Plan

1. **Week 1-2:** Implement S-1, S-2, S-3 (Security enhancements, +260 gaps estimated)
2. **Week 3-4:** Implement M-1, M-2 (Memory enhancements, +160 gaps estimated)
3. **Week 5-6:** Implement C-1 (Concurrency enhancements, +95 gaps estimated)
4. **Week 7-8:** Integration testing and validation
5. **Week 9:** Full Phase 1-4 Enhanced scan + GitHub issues update

**Projected Phase 1-4 Enhanced Total:** 31,720 + 2,200–3,200 = **33,920–34,920 gaps** (7–10% increase)

---

## Success Criteria

### Pattern Validation
- ✓ Each new pattern tested on 5 sample files with known instances
- ✓ False positive rate < 5% per pattern
- ✓ Pattern produces consistent results across scanner runs

### Gap Analysis
- ✓ New gap count increases Phase 1-4 by 7–10% (2,200–3,200 additional gaps)
- ✓ Top gap-producing patterns identified
- ✓ Severity distribution verified

### Documentation
- ✓ All new patterns documented with CWE mappings
- ✓ Examples provided for each pattern
- ✓ Integration guide for scanners

---

## Timeline Integration with Phase 6

```
Phase 1-4 Enhanced: Q3 2026 Week 1-8
├── Week 1-2: Security patterns (S-1, S-2, S-3)
├── Week 3-4: Memory patterns (M-1, M-2)
├── Week 5-6: Concurrency patterns (C-1)
├── Week 7-8: Integration & validation
└── Week 9: Full scan + GitHub update

Phase 6 Parallel: Q3 2026 Week 1-8
├── Week 1-2: P6-1 (ABI Safety) + P6-4 (Build System)
├── Week 3-4: P6-2 (Const Correctness)
├── Week 5-6: P6-3 (Template Meta-Programming)
├── Week 7-8: P6-5 (Ownership & Lifetime)
└── Week 9: Integration + full Phase 1-6 scan

Combined Result: Phase 1-6 = ~165,000–185,000 gaps (40–45% increase from Phase 1-5)
```

---

## Related Documents
- [PHASE_5_IMPLEMENTATION_COMPLETE.md](PHASE_5_IMPLEMENTATION_COMPLETE.md)
- [PHASE_6_SCANNER_DESIGN.md](PHASE_6_SCANNER_DESIGN.md)
- [ROADMAP.md](../ROADMAP.md)
