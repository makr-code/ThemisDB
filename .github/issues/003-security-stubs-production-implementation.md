---
title: "Replace Security Stubs with Production Implementations"
labels: security, enterprise, stubs, priority-high, bsi-compliance
milestone: v1.4.0
---

## 📋 Summary

Multiple security-critical components have **STUB implementations** that return hardcoded dummy values instead of real cryptographic operations. This blocks BSI (German Federal Office for Information Security) compliance and enterprise deployments.

**Type**: Security / Stub Removal  
**Priority**: HIGH (enterprise blocker)  
**Effort**: 2-3 weeks  
**Status**: ❌ 13 Stubs Verified

## 🔍 Verification

**Security Stubs Found** (13 instances):

### 1. Timestamp Authority Stubs (`src/security/timestamp_authority.cpp`)
```cpp
// Line 53
tok.serial_number = "STUB-SERIAL";

// Line 58-59
tok.tsa_name = "STUB-TSA";
tok.tsa_serial = "STUB-TSA-SERIAL";

// Line 87
return std::string("-----BEGIN CERTIFICATE-----\nSTUB-TSA\n-----END CERTIFICATE-----\n");
```

### 2. HSM Provider Stubs (`src/security/hsm_provider.cpp`)
```cpp
// Line 60
r.cert_serial = "STUB-CERT";

// Line 99
return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");

// Line 105
oss << "HSM STUB label=" << config_.key_label;

// Line 125
return std::string("STUB-SERIAL");
```

### 3. PKCS#11 HSM Provider Stubs (`src/security/hsm_provider_pkcs11.cpp`)
```cpp
// Line 332
r.cert_serial = "STUB-CERT";

// Line 462
if(!impl_->real_ready) return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
```

### 4. Vault Signing Provider Mocks (`src/security/vault_signing_provider.cpp`)
```cpp
// Line 81, 125, 172
res.algorithm = "MOCK+SHA256";  // ← Not real signature algorithm
```

### 5. Critical Security TODOs
```cpp
// src/utils/license_info.cpp:253
// TODO: Implement actual signature verification using RSA/SHA-256

// src/security/usb_admin_authenticator.cpp:372
// TODO: CRITICAL SECURITY - Implement proper RSA signature verification

// src/security/vcc_pki_client.cpp:348
// TODO: Implement full X.509 chain validation
```

## 🎯 Problem Statement

### Current State: ❌ Not Production-Ready

1. **Timestamp Authority returns fake timestamps** → Audit logs unverifiable
2. **HSM returns stub certificates** → No real hardware security module integration
3. **Signature verification not implemented** → License tampering possible
4. **X.509 chain validation missing** → Man-in-the-middle attacks possible

### Impact

❌ **Cannot achieve BSI compliance** (Bundesamt für Sicherheit in der Informationstechnik)  
❌ **Cannot achieve SOC 2 compliance**  
❌ **Cannot deploy in enterprise environments**  
❌ **Audit logs legally questionable**  
❌ **License system can be bypassed**

From strategic analysis (`docs/gimini/Strategische Analyse*.md`):
> **Enterprise Integration Gap (Blocker)**: Ein System ohne implementierte Daten-at-Rest-Verschlüsselung, ohne integriertes KMS (Status: "Mock") und ohne zentrale Autorisierung ist für eine BSI Grundschutz-Umgebung nicht zertifizierbar.

## 🏗️ Proposed Implementation

### Phase 1: Timestamp Authority (Week 1)

Replace stubs with **RFC 3161** compliant implementation:

```cpp
// Replace STUB-TSA with real RFC 3161 timestamp
TimestampToken TimestampAuthority::getTimestamp(const std::vector<uint8_t>& data) {
    // 1. Hash the data (SHA-256)
    auto hash = SHA256(data);
    
    // 2. Create TimeStampReq (ASN.1)
    auto req = createTimestampRequest(hash);
    
    // 3. Send to TSA server (or use internal TSA)
    auto tsa_response = sendToTSA(req);
    
    // 4. Parse TimeStampResp
    TimestampToken token = parseTimestampResponse(tsa_response);
    
    // 5. Verify TSA signature
    if (!verifyTSASignature(token)) {
        throw SecurityException("Invalid TSA signature");
    }
    
    return token;
}
```

**Options**:
- **Internal TSA**: Use OpenSSL's `ts_rsp_sign` to run own TSA
- **External TSA**: Integrate with services like DigiCert, Sectigo
- **FreeTSA**: Open-source TSA service

### Phase 2: HSM Provider (Week 1-2)

Replace stubs with **PKCS#11** real implementation:

```cpp
// Use SoftHSM2 for development, CloudHSM/Luna for production
HSMProvider::HSMProvider(const HSMConfig& config) {
    if (config.use_real_hsm) {
        // Load PKCS#11 library
        impl_ = std::make_unique<PKCS11HSM>(config.pkcs11_lib_path);
        impl_->initialize(config.pin);
    } else {
        // Fallback to software crypto (with clear warning)
        spdlog::warn("⚠️  Using software crypto - NOT suitable for production");
        impl_ = std::make_unique<SoftwareCrypto>();
    }
}
```

### Phase 3: License Signature Verification (Week 2)

Implement **RSA-SHA256** signature verification:

```cpp
// src/utils/license_info.cpp
bool verifyLicenseSignature(const License& license, const std::string& public_key) {
    // 1. Extract license data (without signature)
    auto license_data = extractLicenseData(license);
    
    // 2. Load RSA public key
    EVP_PKEY* pkey = loadPublicKey(public_key);
    
    // 3. Verify signature using OpenSSL
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestVerifyUpdate(ctx, license_data.data(), license_data.size());
    
    int result = EVP_DigestVerifyFinal(ctx, 
                                       license.signature.data(), 
                                       license.signature.size());
    
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return result == 1;
}
```

### Phase 4: X.509 Chain Validation (Week 2-3)

Implement **full X.509 chain validation**:

```cpp
// src/security/vcc_pki_client.cpp
bool validateCertificateChain(const X509* cert, const std::vector<X509*>& chain) {
    // 1. Create X509_STORE with root CA
    X509_STORE* store = X509_STORE_new();
    X509_STORE_add_cert(store, root_ca_cert);
    
    // 2. Create X509_STORE_CTX for verification
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
    X509_STORE_CTX_init(ctx, store, cert, nullptr);
    
    // 3. Add intermediate certificates
    STACK_OF(X509)* chain_stack = sk_X509_new_null();
    for (auto* intermediate : chain) {
        sk_X509_push(chain_stack, intermediate);
    }
    X509_STORE_CTX_set0_untrusted(ctx, chain_stack);
    
    // 4. Verify certificate chain
    int result = X509_verify_cert(ctx);
    
    // 5. Check revocation (CRL/OCSP)
    if (result == 1) {
        result = checkRevocationStatus(cert);
    }
    
    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    sk_X509_free(chain_stack);
    
    return result == 1;
}
```

## 📝 Implementation Tasks

### Milestone 1: Timestamp Authority (Week 1)

- [ ] Implement RFC 3161 timestamp request generation
- [ ] Implement TSA server integration (FreeTSA or internal)
- [ ] Implement timestamp response parsing
- [ ] Implement TSA signature verification
- [ ] Replace all `STUB-TSA` code
- [ ] Add unit tests
- [ ] Add integration tests with real TSA

### Milestone 2: HSM Provider (Week 1-2)

- [ ] Implement PKCS#11 initialization
- [ ] Implement key generation via HSM
- [ ] Implement signing via HSM
- [ ] Implement certificate retrieval from HSM
- [ ] Replace all `STUB-CERT` code
- [ ] Test with SoftHSM2 (development)
- [ ] Test with real HSM (production)
- [ ] Add configuration option `hsm.use_real=true/false`

### Milestone 3: License Verification (Week 2)

- [ ] Implement RSA public key loading
- [ ] Implement RSA-SHA256 signature verification
- [ ] Embed ThemisDB public key in binary
- [ ] Replace `TODO: Implement actual signature verification`
- [ ] Add unit tests with test keys
- [ ] Add integration tests with real licenses
- [ ] Add license tampering detection tests

### Milestone 4: X.509 Validation (Week 2-3)

- [ ] Implement full X.509 chain validation
- [ ] Implement CRL checking
- [ ] Implement OCSP checking
- [ ] Replace `TODO: Implement full X.509 chain validation`
- [ ] Add unit tests with test certificates
- [ ] Add integration tests with real CA chains
- [ ] Add certificate expiry handling

### Milestone 5: Configuration & Documentation (Week 3)

- [ ] Add security configuration options
- [ ] Document HSM setup (SoftHSM2, CloudHSM, Luna)
- [ ] Document TSA setup (FreeTSA, internal)
- [ ] Update security documentation
- [ ] Add security best practices guide
- [ ] Update BSI compliance documentation

## 🔗 Dependencies & Related Issues

### Related Issues
- Issue #6: Column-Level Encryption (also enterprise security)
- `docs/de/development/GAPS_STUBS_SUMMARY.md` - Documents stub priorities

### External Dependencies
- OpenSSL (already used)
- PKCS#11 library (SoftHSM2, CloudHSM, Thales Luna)
- TSA service (FreeTSA or commercial)

## 📊 Success Criteria

### Functional Requirements
- ✅ All 13 security stubs replaced with real implementations
- ✅ Timestamp Authority issues RFC 3161 compliant timestamps
- ✅ HSM Provider uses real PKCS#11 hardware
- ✅ License signatures verified with RSA-SHA256
- ✅ X.509 chains fully validated including revocation

### Security Requirements
- ✅ Audit logs have verifiable timestamps
- ✅ Private keys stored in HSM, not on disk
- ✅ License tampering detected and rejected
- ✅ Invalid/expired certificates rejected

### Compliance Requirements
- ✅ Meets BSI Grundschutz requirements
- ✅ Meets SOC 2 Type II requirements
- ✅ Ready for enterprise security audits

### Quality Gates
- ✅ All unit tests passing
- ✅ Integration tests with real HSM/TSA
- ✅ Security audit passed
- ✅ Code review approved
- ✅ Documentation complete

## 📅 Timeline Estimate

| Milestone | Duration | Deliverable |
|-----------|----------|-------------|
| Timestamp Authority | 1 week | RFC 3161 implementation |
| HSM Provider | 1 week | PKCS#11 integration |
| License Verification | 3-4 days | RSA signature verification |
| X.509 Validation | 3-4 days | Full chain validation |
| Configuration & Docs | 2-3 days | Production ready |
| **Total** | **2-3 weeks** | **Enterprise ready** |

## ✅ Definition of Done

- [ ] All 13 security stubs removed
- [ ] Timestamp Authority issues real RFC 3161 timestamps
- [ ] HSM Provider uses real PKCS#11 (with SoftHSM2 fallback)
- [ ] License signature verification working
- [ ] X.509 chain validation working (including CRL/OCSP)
- [ ] All critical security TODOs resolved
- [ ] Unit tests > 90% coverage
- [ ] Integration tests with real HSM/TSA
- [ ] Security audit passed
- [ ] BSI compliance documentation updated
- [ ] Code review approved

---

**Created**: 2026-01-11  
**Verified**: 2026-01-11 (13 stubs + 3 critical TODOs confirmed)  
**Target Version**: v1.4.0  
**Criticality**: HIGH (enterprise blocker)  
**Compliance**: BSI Grundschutz, SOC 2
