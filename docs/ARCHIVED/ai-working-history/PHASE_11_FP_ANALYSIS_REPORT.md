# Phase 11 Security Hardening Scanner — FP Analysis & Fine-Tuning Report

## Execution Summary

**Scan Date:** 2026-06-02  
**Repository:** ThemisDB  
**Total Gaps Detected:** 23,670  
**Scanners:** 6 (Data Leak, Encryption Leak, E2E Encryption, Key Failure, Attack Vectors, Military Hardening)

## Gap Distribution

| Scanner | Gaps | Confidence Avg | Primary Gap Type |
|---------|------|-----------------|------------------|
| Data Leak | 12,395 | 0.70 | unzeroed_memory (11,683) |
| Military Hardening | 6,037 | 0.75 | missing_audit_log (4,049) |
| Attack Vectors | 2,566 | 0.68 | csrf_vulnerability (2,222) |
| Key Failure | 1,358 | 0.80 | no_key_rotation (1,189) |
| E2E Encryption | 1,199 | 0.75 | no_transit_encryption (804) |
| Encryption Leak | 115 | 0.85 | weak_hash_algorithm (36) |

## FALSE POSITIVE FINDINGS

### FP Pattern 1: `unzeroed_memory` (11,683 gaps — 49% of total)
**Issue:** All memory allocations flagged as security-critical, not just secret/sensitive buffers  
**Root Cause:** Pattern `std::vector|malloc|new` without context discrimination  
**Impact:** Massive false positive rate  
**Files Affected:** 964 gaps in `http_server.cpp` alone (buffer allocations for HTTP response bodies)

**Fine-Tuning Action:**
```python
# Current (overly broad):
SENSITIVE_PATTERNS = {
    'unzeroed': re.compile(r'new\s|malloc\(|std::vector')
}

# Refined (context-aware):
SENSITIVE_PATTERNS = {
    'unzeroed': re.compile(r'new.*secret|malloc.*key|vector.*password|calloc.*cipher')
}

# Confidence: Reduce from 0.70 to 0.50 for generic allocations
# Unless within crypto/auth/secret handling blocks
```

**Target Reduction:** 11,683 → ~2,000 (83% reduction)

---

### FP Pattern 2: `missing_audit_log` (4,049 gaps — 17% of total)
**Issue:** All functions flagged as requiring audit logging, even utility helpers  
**Root Cause:** Pattern matches any function definition in security modules  
**Impact:** Non-actionable noise; audit logging only needed for security operations  
**Files Affected:** 200+ security-related source files

**Fine-Tuning Action:**
```python
# Current (catches all functions):
AUDIT_PATTERNS = {
    'security_op': re.compile(r'def |class ')
}

# Refined (actual security operations):
AUDIT_PATTERNS = {
    'security_op': re.compile(r'authenticate|authorize|encrypt|decrypt|sign|verify|key_derive')
}

# Add confidence scoring:
# - High confidence (0.85): authenticate, authorize, decrypt
# - Medium confidence (0.70): encrypt, sign
# - Low confidence (0.50): helper functions in security modules
```

**Target Reduction:** 4,049 → ~1,000 (75% reduction)

---

### FP Pattern 3: `classified_data_unprotected` (937 gaps)
**Issue:** Variables with "secret"/"classified" naming flagged even if properly encrypted  
**Root Cause:** Naming convention pattern without storage/encryption context  
**Impact:** High false positive rate in well-protected code

**Fine-Tuning Action:**
```python
# Current (name-based only):
CLASSIFIED_PATTERNS = {
    'classified_var': re.compile(r'secret|classified|confidential')
}

# Refined (require actual plaintext storage):
CLASSIFIED_PATTERNS = {
    'plaintext_secret': re.compile(r'secret.*=.*"[^"]*";|secret.*file|secret.*db.*plaintext')
}

# Require evidence of unencrypted storage:
# - String literal assignment
# - File I/O without encryption
# - Database storage without ENC() function
```

**Target Reduction:** 937 → ~150 (84% reduction)

---

### FP Pattern 4: `csrf_vulnerability` (2,222 gaps)
**Issue:** Generic form detection flags even test/documentation forms  
**Root Cause:** Simple regex match for HTML form tags without context  
**Impact:** 2,222 low-confidence findings, mostly in test/example code

**Fine-Tuning Action:**
```python
# Current (overly broad):
CSRF_PATTERNS = {
    'no_token': re.compile(r'<form.*method.*post', re.IGNORECASE)
}

# Refined (require state-changing POST + no token):
CSRF_PATTERNS = {
    'no_token': re.compile(r'<form.*method.*post[^>]*>(?!.*csrf|.*token)', re.IGNORECASE)
}

# Context filter:
# - Skip test/example files
# - Skip read-only operations (GET/HEAD)
# - Skip forms with _token or CSRF validation present
```

**Target Reduction:** 2,222 → ~500 (77% reduction)

---

## Confidence Score Distribution

| Confidence | Count | Status |
|-----------|-------|--------|
| 0.5 (Low) | 2,780 | Review-only; high FP risk |
| 0.6 | 4,421 | Probable false positives |
| 0.7 | 14,067 | Mixed; requires filtering |
| 0.8 | 1,900 | Likely true positives |
| 0.9+ | 502 | High-confidence findings |

**Recommendation:** 
- Threshold 0.65+: Report as "MEDIUM" severity
- Threshold 0.80+: Report as "HIGH" severity
- Threshold <0.50: Suppress from reports

---

## Top Gap Sources (Files with Most Gaps)

| File | Gaps | Primary Issues | Likely FP % |
|------|------|---|---|
| `src/server/http_server.cpp` | 964 | unzeroed_memory (94%) | ~90% |
| `src/index/process_graph.cpp` | 437 | missing_audit_log | ~60% |
| `src/query/aql_parser.cpp` | 324 | CSRF, audit_log | ~70% |
| `src/llm/llama_wrapper.cpp` | 317 | missing_audit_log | ~50% |
| `src/query/cypher_parser.cpp` | 218 | audit_log, encryption | ~65% |
| `src/auth/totp_secret_encryption.cpp` | 198 | classified_data | ~30% |
| `src/index/secondary_index.cpp` | 191 | audit_log | ~70% |
| `src/security/post_quantum_crypto.cpp` | 185 | audit_log, algorithm | ~40% |

---

## Fine-Tuning Priority (by impact)

### Immediate (Week 1):
1. **Data Leak Scanner**: Reduce unzeroed_memory false positives (11,683 → 2,000)
   - Add semantic filter: only flag secret/key/password allocations
   - Raise confidence to 0.85+ for actual security-critical buffers

2. **Military Hardening Scanner**: Reduce audit_log false positives (4,049 → 1,000)
   - Filter to actual security operation methods
   - Build whitelist of known safe logging patterns

### Short-Term (Week 2):
3. **Attack Vectors Scanner**: Improve CSRF detection (2,222 → 500)
   - Add context filter for state-changing operations
   - Skip test/example code patterns

4. **Military Hardening**: Fix classified_data detection (937 → 150)
   - Require evidence of plaintext storage, not just naming

### Medium-Term (Week 3):
5. **E2E Encryption**: Improve transit encryption detection (804 gaps)
   - Add HTTPS/TLS context validation
   - Build whitelist for approved protocols

6. **Key Failure**: Validate key rotation patterns (1,189 gaps)
   - Look for actual rotation logic, not just variable naming

---

## Expected Impact After Fine-Tuning

| Phase | Total Gaps | Estimated Quality |
|-------|-----------|-------------------|
| Current | 23,670 | ~30% true positives |
| After P1 Fine-Tune | ~15,000 | ~65% true positives |
| After P1+P2 Fine-Tune | ~10,000 | ~80% true positives |
| After Full Fine-Tune | ~6,000 | ~90% true positives |

---

## Actionable Gaps (Highest Confidence)

**CRITICAL findings (confidence >= 0.85):**
- Weak hash algorithms (MD5/SHA1): 36 gaps
- Deprecated ciphers (DES/RC4): 45 gaps
- Hardcoded keys: 1 gap
- Unsafe deserialization: 129 gaps

**Recommendation:** These are likely true positives; prioritize for remediation.

---

## Notes for Implementation

1. **Confidence Scoring**: Current 0.7 average is too high for this corpus. Most gaps are in well-engineered code with FP patterns.
2. **Test Context Filtering**: Critical for attack_vectors and military_hardening scanners.
3. **Approved Patterns**: Build explicit whitelists for:
   - Crypto libraries (libsodium, OpenSSL, BoringSSL)
   - Common safe patterns (prepared statements, HTML escaping)
4. **Documentation**: Create scanner-specific suppression rules for each gap type.
