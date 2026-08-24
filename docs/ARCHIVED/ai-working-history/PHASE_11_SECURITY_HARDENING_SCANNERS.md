# Phase 11: Security Hardening & Cryptographic Vulnerability Scanners

**Date:** 2026-06-02  
**Version:** 1.0 (Planning)  
**Status:** 🟡 IN DESIGN  
**Target:** Detect cryptographic vulnerabilities, data leaks, key management failures, and attack vectors  
**Timeline:** Q3-Q4 2026 (4-6 weeks, ~2,200 LOC)  
**Expected Impact:** +4,000–7,000 gaps  

---

## Overview

Phase 11 introduces **6 specialized security scanners** targeting:
1. **Data Leak Detection** — Sensitive data exposure (PII, credentials, tokens)
2. **Encryption Leak Detection** — Encryption protocol weaknesses
3. **E2E Security Encryption** — End-to-end encryption correctness
4. **Key Failure Detection** — Cryptographic key management issues
5. **Attack Vector Detection** — System-level vulnerability patterns
6. **Military Hardening** — Classified/restricted data protection

---

## Scanner Specifications

### P11-1 · Data Leak Detection (380 LOC) — 🔴 CRITICAL

**Purpose:** Detect unencrypted sensitive data exposure (CWE-200, CWE-532)

**Detection Patterns (10-12):**
- Hardcoded PII (SSN, credit card, phone numbers)
- Sensitive logging: passwords, tokens, API keys, secrets
- Unencrypted sensitive data storage (plaintext in files/DB)
- Sensitive variables unmasked in error messages
- Memory not zeroed after use (secrets in stack)
- Credentials in config files/environment (unencrypted)
- Sensitive data in logs (console output, trace statements)
- Unencrypted inter-process communication
- Hardcoded database credentials
- API tokens/keys in source code

**Implementation Strategy:**
- Regex patterns for common PII formats (SSN, credit card, phone)
- Whitelist for legitimate test data patterns
- Context-aware scanning: logging calls, variable assignments
- Track data flow from assignment to usage

**Test Strategy:**
- Known-good test cases with PII patterns
- False positive validation on legitimate test data
- Cross-module consistency checks

**Complexity:** MEDIUM  
**Effort:** 3-4 days  
**Expected Gaps:** +600–1,000  

---

### P11-2 · Encryption Leak Detection (420 LOC) — 🔴 CRITICAL

**Purpose:** Detect cryptographic protocol weaknesses (CWE-327, CWE-325)

**Detection Patterns (12-15):**
- Weak hash algorithms (MD5, SHA1, SHA256-224)
- Deprecated ciphers (DES, 3DES, RC4, Blowfish)
- ECB mode usage (no IV/nonce, pattern leakage)
- Weak key derivation (simple iteration count < 100k, no salt)
- Unverified encryption (no authentication tag, no HMAC)
- Hardcoded IV/nonce (deterministic encryption)
- Missing salt in password hashing
- OpenSSL low-level API usage (not EVP layer)
- Custom cryptography implementation
- Weak random number generation (rand(), time-based)
- Cleartext protocol usage (HTTP, FTP, Telnet, unencrypted DB)
- Missing TLS certificate verification
- Cipher suite without AES/ChaCha20

**Implementation Strategy:**
- Pattern library for weak ciphers and hashes
- Whitelist for approved cryptographic practices (AES-256-GCM, SHA-256, etc.)
- Context analyzer for cryptographic API calls
- Configuration parser for TLS/SSL settings

**Test Strategy:**
- Real-world weak crypto examples
- Approved cipher validation
- Integration with OpenSSL/BoringSSL patterns

**Complexity:** HIGH  
**Effort:** 4-5 days  
**Expected Gaps:** +1,200–1,800  

---

### P11-3 · E2E Security Encryption Verification (400 LOC) — 🟠 HIGH

**Purpose:** Ensure end-to-end encryption is correctly implemented (CWE-327)

**Detection Patterns (10-13):**
- Missing encryption in transmission (plaintext wire protocol)
- Decryption without verification (no authentication)
- Encryption but plaintext storage (encrypted in transit, plaintext at rest)
- Key not derived from password (hardcoded or randomly generated key)
- Encryption state not checked before secure operations
- Missing encryption in sensitive modules (auth, payment, PII)
- Partial encryption (some fields encrypted, others plaintext)
- Encryption that can be bypassed (alternative code path)
- No re-encryption of old data on key rotation
- Missing forward secrecy (ephemeral keys not used)
- Encryption algorithm can be downgraded
- Missing Perfect Forward Secrecy (PFS) in key exchange

**Implementation Strategy:**
- E2E pattern analyzer: detect encryption entry/exit points
- Context-aware scanning: sensitive data types vs encryption coverage
- Multi-stage analysis: wire protocols, storage, key derivation
- Flow analysis: track encrypted vs plaintext data

**Test Strategy:**
- Data flow tracing: from encryption to decryption
- E2E pattern validation
- Key derivation correctness checks

**Complexity:** HIGH  
**Effort:** 4-5 days  
**Expected Gaps:** +800–1,200  

---

### P11-4 · Key Failure Detection (440 LOC) — 🔴 CRITICAL

**Purpose:** Detect cryptographic key management vulnerabilities (CWE-321, CWE-326)

**Detection Patterns (13-16):**
- Hardcoded cryptographic keys
- Keys stored in plaintext (no key derivation, no encryption)
- Weak key generation (insufficient entropy, predictable RNG)
- Keys not rotated (no rotation schedule, one-time use failure)
- Keys not protected in memory (not zeroed, swappable to disk)
- Key size too small (< 128 bits symmetric, < 2048 RSA)
- Keys in version control (git history, accidental commits)
- Keys in configuration files (unencrypted, world-readable)
- Master key not protected (not HSM, not secure enclave)
- Key derivation weakness (weak KDF, no salt, low iteration count)
- Keys passed as plaintext in function parameters
- Multiple uses of same key/IV combination
- No key backup/recovery procedure documented
- Insecure key exchange (no forward secrecy, no perfect secrecy)

**Implementation Strategy:**
- Key constant detector: identify hardcoded secrets, keys
- Storage analyzer: where keys are stored and how protected
- Lifecycle tracker: key generation, rotation, destruction
- Entropy analyzer: RNG quality and seed sources

**Test Strategy:**
- Hardcoded key detection across codebase
- Key rotation schedule validation
- Memory protection checks
- KDF parameter validation

**Complexity:** CRITICAL  
**Effort:** 5-6 days  
**Expected Gaps:** +1,500–2,200  

---

### P11-5 · Attack Vector Detection (460 LOC) — 🟠 HIGH

**Purpose:** Identify common attack surface patterns (CWE-943, CWE-200, CWE-89)

**Detection Patterns (14-18):**
- SQL injection vectors (string concatenation in queries)
- Command injection (system calls with user input)
- Path traversal (user input in file paths without validation)
- XSS vulnerabilities (unsanitized user input in output)
- CSRF token missing or not validated
- Authentication bypass (weak/missing checks)
- Authorization bypass (privilege escalation opportunities)
- Deserialization of untrusted data
- XXE (XML External Entity) vulnerabilities
- ReDoS (Regular Expression Denial of Service)
- LDAP injection (unsanitized LDAP queries)
- SSRF (Server-Side Request Forgery)
- Race condition in security checks (TOCTOU)
- Missing input validation/bounds checking
- Insufficient entropy in security tokens
- Direct object references (IDOR)
- Function pointers from untrusted source
- Buffer overflow via user input

**Implementation Strategy:**
- Input/output tracking: user input to security-sensitive functions
- Injection pattern library: SQL, command, path traversal, XSS
- Context analyzer: distinguish safe vs unsafe usage
- Integration with existing CWE scanners (Phase 5-6)

**Test Strategy:**
- OWASP Top 10 patterns
- Real-world vulnerability examples
- Safe usage validation

**Complexity:** CRITICAL  
**Effort:** 5-7 days  
**Expected Gaps:** +2,000–3,000  

---

### P11-6 · Military Hardening Scanner (500 LOC) — 🟠 HIGH

**Purpose:** Enforce FIPS 140-2/Common Criteria requirements (Restricted/Classified data)

**Detection Patterns (16-20):**
- FIPS-approved algorithms only (no MD5, SHA1, 3DES, DES, RC4)
- FIPS-approved RNGs (no predictable/time-based RNG)
- Hardware-backed keys (HSM, Trusted Platform Module)
- Classified data in plaintext memory/storage
- Missing data classification markers
- Unencrypted classified exports/backups
- Unauthorized access attempts not logged
- Missing integrity verification (no HMAC/signature)
- Debug info accessible in production (symbols, verbose logging)
- Covert channels not mitigated (timing, power analysis)
- Side-channel attacks possible (cache timing, branch prediction)
- No trusted computing base (TCB) verification
- Unauthorized algorithm downgrade
- Missing audit trail for classified operations
- No secure compartmentalization (CWE-653)
- Insufficient isolation (multi-tenant cross-contamination)
- Direct memory access not restricted
- Cryptographic state not verifiable (black-box crypto)

**Implementation Strategy:**
- Data classification analyzer: identify classified/sensitive data
- FIPS compliance checker: approved algorithms and modes
- Access control analyzer: enforcement and audit trails
- Memory/storage protection scanner: encryption and zeroing

**Test Strategy:**
- FIPS 140-2 requirements mapping
- Common Criteria EAL4 patterns
- Classified data flow tracking
- Compartmentalization checks

**Complexity:** CRITICAL  
**Effort:** 6-8 days  
**Expected Gaps:** +1,800–2,800  

---

## Implementation Timeline

| Week | Phase | Tasks | Scanner | Effort |
|------|-------|-------|---------|--------|
| 1 | Design | Finalize patterns, test cases | All (P11-1 to P11-6) | 2d |
| 2 | Core | Implement P11-1 (Data Leak) | P11-1 | 3-4d |
| 3 | Core | Implement P11-2 (Encryption Leak) | P11-2 | 4-5d |
| 4 | Core | Implement P11-3 (E2E Encryption) | P11-3 | 4-5d |
| 5 | Core | Implement P11-4 (Key Failure) | P11-4 | 5-6d |
| 6 | Core | Implement P11-5 (Attack Vectors) | P11-5 | 5-7d |
| 7 | Core | Implement P11-6 (Military Hardening) | P11-6 | 6-8d |
| 8 | Test | Integration, tuning, FP reduction | All | 3-4d |
| 9 | Deliver | Documentation, commit, validation | All | 2-3d |

**Total Effort:** 4-6 weeks (28-40 person-days)  
**Total LOC:** ~2,200  

---

## Impact Projection

### Gap Contribution
- P11-1 Data Leak: +600–1,000 gaps (10%)
- P11-2 Encryption Leak: +1,200–1,800 gaps (20%)
- P11-3 E2E Encryption: +800–1,200 gaps (14%)
- P11-4 Key Failure: +1,500–2,200 gaps (35%)
- P11-5 Attack Vectors: +2,000–3,000 gaps (30%)
- P11-6 Military Hardening: +1,800–2,800 gaps (40%)

**Total Phase 11 Impact:** +8,900–12,000 gaps (expected range)

### FP Reduction Strategy
- Whitelists for approved crypto libraries (OpenSSL EVP, BoringSSL, libsodium)
- Test data patterns for PII (common test SSNs, credit cards)
- Configuration file exemptions for non-sensitive keys
- Development/testing environment detection
- Cryptographic library version awareness

### Integration with Existing Scanners
- Builds on Phase 1-6 memory/security infrastructure
- Reuses CWE mapping and severity classification
- Leverages existing data-flow analysis (Phase 5)
- Enhances observability integration (Phase 10)

---

## Success Criteria

1. **Coverage:** Detect ≥85% of common OWASP Top 10 patterns
2. **Accuracy:** FP rate < 20% after whitelisting
3. **Performance:** Scan all 65 modules in < 5 minutes
4. **Compliance:** Map ≥90% of findings to CWE/CVSS
5. **Documentation:** Full Doxygen API documentation
6. **Integration:** Works with existing gap audit pipeline

---

## Definition of Done (Per Scanner)

For each scanner (P11-1 to P11-6):
- [ ] Design document (patterns, algorithms, test cases)
- [ ] Full implementation (~380-500 LOC per scanner)
- [ ] Unit tests (≥20 test cases per scanner)
- [ ] Whitelist/FP reduction logic
- [ ] Doxygen documentation
- [ ] Integration with gap_audit_pipeline_v3.py
- [ ] Validation on 50-gap sample
- [ ] Performance benchmarking

---

## Deliverables

1. **tools/gap_scanner_v3_phase11_data_leak.py** (380 LOC)
2. **tools/gap_scanner_v3_phase11_encryption_leak.py** (420 LOC)
3. **tools/gap_scanner_v3_phase11_e2e_encryption.py** (400 LOC)
4. **tools/gap_scanner_v3_phase11_key_failure.py** (440 LOC)
5. **tools/gap_scanner_v3_phase11_attack_vectors.py** (460 LOC)
6. **tools/gap_scanner_v3_phase11_military_hardening.py** (500 LOC)
7. **tools/gap_scanner_v3_phase11_integration.py** (Integration harness, 150 LOC)
8. **ai_working/PHASE_11_VALIDATION_REPORT.md** (Analysis and FP reduction)

---

## Risks and Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| High FP rate (>30%) | MEDIUM | HIGH | Aggressive whitelist, test data patterns, approved crypto library DB |
| Performance degradation | LOW | MEDIUM | Incremental scanning, caching, profiling |
| Incomplete crypto coverage | MEDIUM | HIGH | Reference OWASP/NIST, integrate security expert review |
| False negatives in attack vectors | MEDIUM | CRITICAL | Testing against real CVEs, OWASP Top 10 mapping |
| Integration complexity | LOW | MEDIUM | Modular design, early integration testing |

---

## Future Enhancements (Phase 12+)

- Machine learning-based pattern detection
- Custom cryptographic algorithm detection
- Hardware security module (HSM) integration verification
- Quantum-resistant cryptography migration checks
- Supply chain security (dependency auditing)
- Zero-knowledge proof correctness validation
