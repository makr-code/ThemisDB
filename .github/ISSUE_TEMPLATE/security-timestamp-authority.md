---
name: 🔐 Security: Timestamp Authority Implementation
about: Implement RFC 3161 compliant Timestamp Authority
title: "[SECURITY] Implement RFC 3161 Timestamp Authority"
labels: priority:P0, type:security, area:security, effort:large, production-blocker
assignees: ''
---

## 🔴 Production Blocker

**Current Status:** Development-only STUB implementation  
**Priority:** P0 (Critical)  
**Effort:** 2-3 weeks  
**Target Version:** v1.3.1  
**Related Audit:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 3.2.2

---

## 📋 Problem Description

The current Timestamp Authority is a **stub for development only**:

```cpp
// src/security/timestamp_authority.cpp
// WARNING: This is a STUB implementation for development only
THEMIS_WARN("Using TimestampAuthority STUB - NOT SECURE for production!");
tok.serial_number = "STUB-SERIAL";
tok.tsa_name = "STUB-TSA";
tok.tsa_serial = "STUB-TSA-SERIAL";
```

**Security Risk:** **HIGH**  
- Timestamps are not cryptographically signed
- No proof of time authenticity
- Not legally binding for compliance requirements
- Vulnerable to timestamp manipulation

---

## 🎯 Requirements

### Must Have (P0)

- [ ] **RFC 3161 Compliance**
  - Implement Time-Stamp Protocol (TSP)
  - Generate RFC 3161 compliant timestamp tokens
  - Sign tokens with TSA certificate
  - Include accurate time from trusted source
  
- [ ] **TSA Certificate Management**
  - Load TSA certificate and private key
  - Validate certificate chain
  - Support certificate rotation
  - Handle certificate expiration
  
- [ ] **Token Operations**
  - Create timestamp request (TimeStampReq)
  - Generate timestamp response (TimeStampResp)
  - Include hash of data being timestamped
  - Sign token with TSA private key
  
- [ ] **Time Source**
  - Use NTP server for accurate time
  - Verify time source reliability
  - Handle time sync failures

### Should Have (P1)

- [ ] **External TSA Support**
  - Option to use external TSA service (Digicert, GlobalSign, etc.)
  - HTTP client for external TSA requests
  - Fallback to external TSA if internal fails
  
- [ ] **Token Verification**
  - Verify timestamp token signature
  - Check certificate chain validity
  - Validate token structure
  
- [ ] **QTSP Compliance**
  - Qualified Trust Service Provider validation
  - eIDAS compliance (for EU deployments)

### Nice to Have (P2)

- [ ] **Token Storage**
  - Store issued tokens for audit
  - Query tokens by timestamp range
  
- [ ] **Batch Operations**
  - Batch timestamp multiple items
  - Reduced overhead for bulk operations

---

## 🔧 Implementation Options

### Option 1: Internal TSA (Recommended for v1.3.1)

**Pros:**
- Full control over timestamping
- No external dependencies
- Lower latency

**Cons:**
- Need to manage TSA certificates
- Need reliable time source (NTP)
- More implementation work

**Dependencies:**
- OpenSSL (already available)
- NTP client library

### Option 2: External TSA Service

**Pros:**
- No certificate management
- Legally binding timestamps
- QTSP compliance out-of-the-box

**Cons:**
- Network dependency
- Per-timestamp cost
- Higher latency

**Providers:**
- Digicert Timestamp Authority
- GlobalSign TSA
- Sectigo TSA

### Option 3: Hybrid (Recommended for Production)

- Internal TSA for development/testing
- External TSA for production
- Configuration-driven selection

---

## 🔧 Implementation Details

### Files to Modify

- `src/security/timestamp_authority.cpp` - Replace stub with RFC 3161 implementation
- `include/security/timestamp_authority.h` - Add RFC 3161 structures
- `CMakeLists.txt` - Add dependencies if needed

### OpenSSL RFC 3161 APIs

```cpp
// OpenSSL provides RFC 3161 support:
#include <openssl/ts.h>

// Key functions:
TS_RESP *TS_RESP_create_response(TS_RESP_CTX *ctx, BIO *req_bio);
TS_TST_INFO *TS_REQ_to_TS_TST_INFO(TS_REQ *req);
int TS_RESP_verify_response(TS_VERIFY_CTX *ctx, TS_RESP *response);
```

### Configuration Example

```yaml
timestamp_authority:
  mode: "internal"  # or "external"
  
  # Internal TSA config
  internal:
    cert_path: "/etc/themisdb/tsa-cert.pem"
    key_path: "/etc/themisdb/tsa-key.pem"
    ntp_server: "time.nist.gov"
    hash_algorithm: "SHA256"
  
  # External TSA config
  external:
    url: "http://timestamp.digicert.com"
    timeout_ms: 5000
    retry_count: 3
```

---

## ✅ Acceptance Criteria

- [ ] Generate RFC 3161 compliant timestamp tokens
- [ ] Tokens are cryptographically signed
- [ ] Time source is synchronized with NTP
- [ ] **Zero stub code** remaining in `timestamp_authority.cpp`
- [ ] All tests pass (internal and external TSA modes)
- [ ] Documentation includes TSA setup guide
- [ ] Tokens can be verified by standard tools (openssl ts -verify)

---

## 🧪 Testing Requirements

### Unit Tests

- [ ] Test timestamp token generation
- [ ] Test token signature verification
- [ ] Test certificate chain validation
- [ ] Test NTP time synchronization
- [ ] Test external TSA fallback
- [ ] Test error handling (cert expired, time sync failed)

### Integration Tests

- [ ] Generate 1000 tokens and verify all
- [ ] Test with openssl command-line verification
- [ ] Test external TSA integration (if configured)
- [ ] Load test (100 tokens/sec for 1 minute)

### Manual Testing

- [ ] Verify token with `openssl ts -verify`
- [ ] Test with real external TSA service
- [ ] Verify time accuracy (< 1 second drift)

---

## 📚 References

- **RFC 3161:** Internet X.509 PKI Time-Stamp Protocol (TSP)  
  https://www.ietf.org/rfc/rfc3161.txt
  
- **OpenSSL TS Manual:**  
  https://www.openssl.org/docs/man1.1.1/man1/openssl-ts.html
  
- **eIDAS Regulation:**  
  https://ec.europa.eu/digital-single-market/en/trust-services-and-eid
  
- **Current Implementation:**  
  `src/security/timestamp_authority.cpp` (lines 1-200)

---

## 📊 Success Metrics

- ✅ No security warnings in logs
- ✅ Timestamp token generation < 50ms (p99)
- ✅ 100% RFC 3161 compliance
- ✅ Tokens verifiable by standard tools
- ✅ Production-ready for v1.3.1 release

---

## 🚨 Important Notes

- **Certificate Management:** TSA certificate must have special extensions (id-kp-timeStamping)
- **Legal Requirements:** Check local regulations for timestamp requirements
- **Time Accuracy:** NTP drift must be monitored (max 1 second allowed)
- **Audit Trail:** All issued timestamps should be logged for compliance

---

**Created:** Based on Namespace Implementation Audit (2026-01-20)  
**Audit Section:** 3.2.2 Security: Timestamp Authority
