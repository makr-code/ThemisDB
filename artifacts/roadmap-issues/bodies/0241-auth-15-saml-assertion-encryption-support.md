### Context

This issue implements the roadmap item 'SAML Assertion Encryption Support' for the auth domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.4.0.

Primary detail section: 15. SAML Assertion Encryption Support

### Goal

Deliver the scoped changes for SAML Assertion Encryption Support in src/auth/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### 15. SAML Assertion Encryption Support

**Priority:** Low  
**Target Version:** v1.4.0

`saml_authenticator.cpp` validates signed SAML assertions but currently returns `{}` (empty) in the `extractAttributes()` path (lines 428, 443) when the assertion is encrypted. Encrypted SAML assertions (`<EncryptedAssertion>`) are the default in high-security SAML deployments.

**Implementation Notes:**
- `[ ]` Implement `decryptAssertion(xmlDocPtr doc, const std::string& sp_private_key_pem)` using `xmlSecOpenSSLInit()` and `xmlSecEncCtxDecrypt()` in `saml_authenticator.cpp`
- `[ ]` Load SP private key from HSM/key-store, not from plain-text PEM config field
- `[ ]` After decryption, pass the plaintext assertion through the existing signature verification path (`saml_authenticator.cpp:326-407`)
- `[ ]` Return `{}` with an explicit error code (not silently) when decryption fails — current silent empty return hides misconfiguration

---

### Acceptance Criteria

- [ ] Implement `decryptAssertion(xmlDocPtr doc, const std::string& sp_private_key_pem)` using `xmlSecOpenSSLInit()` and `xmlSecEncCtxDecrypt()` in `saml_authenticator.cpp`
- [ ] Load SP private key from HSM/key-store, not from plain-text PEM config field
- [ ] After decryption, pass the plaintext assertion through the existing signature verification path (`saml_authenticator.cpp:326-407`)
- [ ] Return `{}` with an explicit error code (not silently) when decryption fails — current silent empty return hides misconfiguration

### Relationships

- Roadmap row: #241 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#15-saml-assertion-encryption-support
- Source key: roadmap:241:auth:v1.4.0:15-saml-assertion-encryption-support

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:241:auth:v1.4.0:15-saml-assertion-encryption-support -->
<!-- roadmap-ref: row=241;module=auth;target=v1.4.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#15-saml-assertion-encryption-support -->
