---
name: 🔐 Security: HSM Provider Implementation
about: Implement production-ready HSM Provider with PKCS#11 integration
title: "[SECURITY] Implement HSM Provider with PKCS#11"
labels: priority:P0, type:security, area:security, effort:x-large, production-blocker
assignees: ''
---

## 🔴 Production Blocker

**Current Status:** STUB implementation with security warnings  
**Priority:** P0 (Critical)  
**Effort:** 2-3 weeks  
**Target Version:** v1.3.1  
**Related Audit:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 3.2.1

---

## 📋 Problem Description

The current HSM Provider is a **stub implementation** that logs security warnings:

```cpp
// src/security/hsm_provider.cpp
THEMIS_WARN("HSMProvider STUB initialized - NOT SECURE for production!");
THEMIS_WARN("HSMProvider STUB signing - NOT cryptographically secure!");
```

**Security Risk:** **CRITICAL**  
- Keys are not stored in Hardware Security Module
- Signing operations are not cryptographically secure
- Not suitable for production deployment

---

## 🎯 Requirements

### Must Have (P0)

- [ ] **PKCS#11 Integration**
  - Integrate with real HSM via PKCS#11 standard
  - Support multiple HSM vendors (SoftHSM, YubiHSM, AWS CloudHSM, etc.)
  - Connection pooling for HSM sessions
  
- [ ] **Key Operations**
  - Generate RSA/ECDSA keys in HSM
  - Sign data using HSM-stored private keys
  - Verify signatures
  - Key lifecycle management (create, rotate, destroy)
  
- [ ] **Configuration**
  - HSM slot selection
  - PIN/password management (secure storage)
  - Key label configuration
  - Failover to secondary HSM
  
- [ ] **Error Handling**
  - HSM communication errors
  - Authentication failures
  - Key not found handling
  - Session timeout recovery

### Should Have (P1)

- [ ] **Multi-Tenancy Support**
  - Separate key spaces per tenant
  - Tenant-specific HSM slots
  
- [ ] **Monitoring**
  - HSM operation metrics (success/failure rates)
  - Latency tracking
  - Key usage statistics
  
- [ ] **Audit Logging**
  - All HSM operations logged
  - Key access audit trail
  - Compliance reporting

### Nice to Have (P2)

- [ ] **Backup Key Provider**
  - Fallback to software keys if HSM unavailable
  - Automatic failover
  
- [ ] **Key Import/Export**
  - Import existing keys into HSM
  - Secure key export for backup

---

## 🔧 Implementation Details

### Files to Modify

- `src/security/hsm_provider.cpp` - Replace stub with real PKCS#11 calls
- `include/security/hsm_provider.h` - Add PKCS#11 types
- `CMakeLists.txt` - Add PKCS#11 library dependency

### Dependencies

Add to `vcpkg.json`:
```json
{
  "dependencies": [
    {
      "name": "pkcs11-helper",
      "version": "1.29.0"
    }
  ]
}
```

Or link against system PKCS#11:
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(PKCS11 REQUIRED libpkcs11-helper-1)
```

### Recommended Libraries

- **SoftHSM2** - For development/testing
- **pkcs11-helper** - PKCS#11 wrapper library
- **OpenSC** - Additional PKCS#11 tools

### Configuration Example

```yaml
hsm:
  enabled: true
  library: "/usr/lib/softhsm/libsofthsm2.so"  # PKCS#11 library path
  slot: 0
  pin: "${HSM_PIN}"  # From environment variable
  key_label: "themisdb-signing-key"
  key_type: "RSA-2048"
  failover:
    enabled: true
    secondary_slot: 1
```

---

## ✅ Acceptance Criteria

- [ ] HSM Provider initializes with real HSM connection
- [ ] All signing operations use HSM-stored keys
- [ ] No security warnings logged
- [ ] **Zero stub code** remaining in `hsm_provider.cpp`
- [ ] All tests pass with SoftHSM in CI
- [ ] Documentation updated with HSM setup guide
- [ ] Configuration examples for major HSM vendors

---

## 🧪 Testing Requirements

### Unit Tests

- [ ] Test HSM connection with valid credentials
- [ ] Test HSM connection failure handling
- [ ] Test signing with HSM key
- [ ] Test signature verification
- [ ] Test key rotation
- [ ] Test session timeout recovery

### Integration Tests

- [ ] Test with SoftHSM (in CI)
- [ ] Test multi-threaded HSM access
- [ ] Test failover to secondary HSM
- [ ] Load test (1000+ operations/sec)

### Manual Testing

- [ ] Test with real HSM hardware (YubiHSM or similar)
- [ ] Test with AWS CloudHSM
- [ ] Verify audit logs

---

## 📚 References

- **PKCS#11 Specification:** https://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/
- **SoftHSM2 Documentation:** https://www.opendnssec.org/softhsm/
- **pkcs11-helper:** https://github.com/OpenSC/pkcs11-helper
- **Current Implementation:** `src/security/hsm_provider.cpp` (lines 1-150)

---

## 📊 Success Metrics

- ✅ No security warnings in logs
- ✅ HSM operations < 100ms latency (p99)
- ✅ 100% test coverage for HSM operations
- ✅ Production-ready for v1.3.1 release

---

## 🚨 Important Notes

- **Security Review Required:** This change must undergo security review before production
- **Backward Compatibility:** Provide migration path from mock keys
- **Documentation:** HSM setup must be documented for operations team
- **Vendor Testing:** Test with at least 2 different HSM vendors

---

**Created:** Based on Namespace Implementation Audit (2026-01-20)  
**Audit Section:** 3.2.1 Security: HSM Provider
