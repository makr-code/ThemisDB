---
name: 🔐 Security: LoRA Signature Verification
about: Implement cryptographic signature verification for LoRA adapters
title: "[SECURITY] Implement LoRA Cryptographic Signature Verification"
labels: priority:P1, type:security, area:llm, area:security, effort:large, v1.4.0
assignees: ''
---

## ⚠️ Important for v1.4.0

**Current Status:** Format validation only, no cryptographic verification  
**Priority:** P1 (High)  
**Effort:** 1-2 weeks  
**Target Version:** v1.4.0  
**Related Audit:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 3.3 (LLM Module)

---

## 📋 Problem Description

The LoRA security validator only validates format, not cryptographic signatures:

```cpp
// src/llm/lora_security_validator.cpp
spdlog::warn("Embedded LoRA signature cryptographic verification not implemented - using format validation only");
result.error_message = "Cryptographic verification not implemented - format validated only";
```

**Security Risk:** **MEDIUM**  
- LoRA adapters can be loaded without signature verification
- No guarantee of adapter authenticity
- Potential for malicious adapter injection
- No chain of trust for adapter sources

---

## 🎯 Requirements

### Must Have (P1) - v1.4.0

- [ ] **RSA-SHA256 Signature Verification**
  - Verify embedded signatures in LoRA adapter files
  - Support RSA-2048 and RSA-4096 keys
  - Verify signature matches adapter content
  
- [ ] **Certificate Chain Validation**
  - Validate X.509 certificate chain
  - Check certificate expiration
  - Verify issuer trust
  - Support custom CA certificates
  
- [ ] **Signature Metadata**
  - Extract signer information
  - Timestamp validation
  - Version checking
  
- [ ] **Configuration**
  - Enable/disable signature verification
  - Trusted CA certificate store
  - CRL (Certificate Revocation List) checking

### Should Have (P2)

- [ ] **Multiple Signature Support**
  - Allow multiple signers per adapter
  - Require N-of-M signatures
  
- [ ] **ECDSA Support**
  - Support ECDSA signatures (P-256, P-384)
  - More efficient than RSA
  
- [ ] **Hardware Security Module**
  - Integrate with HSM for key verification
  - Use HSM for trusted root keys

### Nice to Have (P3)

- [ ] **Online Verification**
  - OCSP (Online Certificate Status Protocol)
  - Real-time revocation checking
  
- [ ] **Signature Registry**
  - Central registry of trusted signers
  - Automatic key updates

---

## 🔧 Implementation Details

### Files to Modify

- `src/llm/lora_security_validator.cpp` - Implement cryptographic verification
- `include/llm/lora_security_validator.h` - Add verification types
- `src/llm/lora_framework/lora_adapter_manager.cpp` - Integrate verification

### Signature Format

LoRA adapters should include embedded signature:

```json
{
  "lora_metadata": {
    "name": "adapter-v1",
    "version": "1.0.0",
    "signature": {
      "algorithm": "RSA-SHA256",
      "value": "base64_encoded_signature",
      "certificate": "base64_encoded_cert",
      "chain": ["base64_cert1", "base64_cert2"],
      "timestamp": "2026-01-20T12:00:00Z"
    }
  }
}
```

### OpenSSL Verification Example

```cpp
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

bool verifyLoRASignature(
    const std::vector<uint8_t>& adapter_data,
    const std::string& signature_b64,
    const std::string& cert_pem
) {
    // 1. Load certificate
    BIO* bio = BIO_new_mem_buf(cert_pem.c_str(), -1);
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    
    // 2. Extract public key
    EVP_PKEY* pubkey = X509_get_pubkey(cert);
    
    // 3. Decode signature
    std::vector<uint8_t> signature = base64_decode(signature_b64);
    
    // 4. Verify signature
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pubkey);
    EVP_DigestVerifyUpdate(ctx, adapter_data.data(), adapter_data.size());
    int result = EVP_DigestVerifyFinal(ctx, signature.data(), signature.size());
    
    // 5. Cleanup
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubkey);
    X509_free(cert);
    BIO_free(bio);
    
    return (result == 1);
}
```

### Configuration Example

```yaml
lora_security:
  signature_verification:
    enabled: true
    required: true  # Reject unsigned adapters
    
  trusted_ca_certs:
    - "/etc/themisdb/ca-certs/root-ca.pem"
    - "/etc/themisdb/ca-certs/intermediate-ca.pem"
    
  crl_checking:
    enabled: true
    crl_paths:
      - "/etc/themisdb/crl/ca.crl"
    
  ocsp:
    enabled: false  # Optional online checking
    
  allow_self_signed: false  # For development only
```

---

## ✅ Acceptance Criteria

- [ ] Cryptographic signature verification implemented
- [ ] Certificate chain validation working
- [ ] CRL checking functional
- [ ] **Zero warnings** about missing verification
- [ ] Unsigned adapters rejected (when required=true)
- [ ] All tests pass
- [ ] Documentation includes signing guide

---

## 🧪 Testing Requirements

### Unit Tests

- [ ] Test signature verification with valid signature
- [ ] Test rejection of invalid signature
- [ ] Test rejection of unsigned adapter
- [ ] Test certificate chain validation
- [ ] Test expired certificate detection
- [ ] Test CRL revocation checking
- [ ] Test with self-signed certificates (dev mode)

### Integration Tests

- [ ] Load signed adapter successfully
- [ ] Reject unsigned adapter when required
- [ ] Test with revoked certificate
- [ ] Test with expired certificate
- [ ] Test adapter loading performance (< 10ms overhead)

### Manual Testing

- [ ] Generate test certificates
- [ ] Sign test adapter
- [ ] Verify with openssl command line
- [ ] Test with real HSM (if available)

---

## 📚 References

- **OpenSSL EVP API:** https://www.openssl.org/docs/man3.0/man3/EVP_DigestVerify.html
- **X.509 Certificates:** https://www.ietf.org/rfc/rfc5280.txt
- **CRL:** https://www.ietf.org/rfc/rfc5280.txt Section 5
- **Current Implementation:** `src/llm/lora_security_validator.cpp` (lines 50-150)

---

## 📊 Success Metrics

- ✅ Cryptographic verification functional
- ✅ Verification overhead < 10ms per adapter
- ✅ 100% test coverage for verification logic
- ✅ No security warnings in logs
- ✅ Ready for v1.4.0 release

---

## 🚨 Important Notes

- **Performance:** Signature verification should not significantly impact adapter loading time
- **Backward Compatibility:** Support loading unsigned adapters in development mode
- **Key Management:** Document how to generate and distribute signing keys
- **Certificate Renewal:** Plan for certificate rotation before expiration

---

## 📅 Implementation Plan

### Week 1: Core Verification
- [ ] Day 1-2: Implement RSA-SHA256 verification
- [ ] Day 3-4: Implement certificate chain validation
- [ ] Day 5: Unit tests

### Week 2: Advanced Features
- [ ] Day 1: CRL checking
- [ ] Day 2: Configuration system
- [ ] Day 3-4: Integration tests
- [ ] Day 5: Documentation and signing guide

---

**Created:** Based on Namespace Implementation Audit (2026-01-20)  
**Audit Section:** 3.3 LLM Module - LoRA Security
