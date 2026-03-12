### Context

This issue implements the roadmap item 'Secure Memory for Key Material in `jwks_security.cpp`' for the auth domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: 10. Secure Memory for Key Material in `jwks_security.cpp`

### Goal

Deliver the scoped changes for Secure Memory for Key Material in `jwks_security.cpp` in src/auth/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### 10. Secure Memory for Key Material in `jwks_security.cpp`

**Priority:** High (Security)  
**Target Version:** v1.2.0

`jwks_security.cpp:276` passes `impl_->config.client_key_password` as a plain `std::string` to `CURLOPT_KEYPASSWD`. `std::string` stores content in allocator-managed heap memory that may be swapped to disk, appear in core dumps, or be left in freed pages readable by a later allocation. Similarly, private key bytes loaded into memory in `jwt_key_rotation_manager.cpp` and `totp_secret_encryption.cpp` are held in plain `std::string` or `std::vector<uint8_t>`.

**Implementation Notes:**
- `[ ]` Introduce `SecureString` wrapper (or use `sodium_malloc` / `sodium_mlock` from libsodium) for all password and private-key fields in `JWKSSecurityConfig` (`include/auth/jwks_security.h`) and `TOTPSecretEncryption` (`include/auth/totp_secret_encryption.h`)
- `[ ]` Call `OPENSSL_cleanse()` (or `sodium_memzero()`) on key buffers in destructors of `JWKSSecurityImpl`, `JWTKeyRotationManager`, and `TOTPSecretEncryption` before freeing memory
- `[ ]` Ensure `mlockall(MCL_CURRENT)` or per-allocation `mlock()` is called for pages holding key material on Linux; document Windows equivalent (`VirtualLock`)
- `[ ]` Remove `client_key_password` from any struct that may be serialised, logged, or copied by value

---

### Acceptance Criteria

- [ ] Introduce `SecureString` wrapper (or use `sodium_malloc` / `sodium_mlock` from libsodium) for all password and private-key fields in `JWKSSecurityConfig` (`include/auth/jwks_security.h`) and `TOTPSecretEncryption` (`include/auth/totp_secret_encryption.h`)
- [ ] Call `OPENSSL_cleanse()` (or `sodium_memzero()`) on key buffers in destructors of `JWKSSecurityImpl`, `JWTKeyRotationManager`, and `TOTPSecretEncryption` before freeing memory
- [ ] Ensure `mlockall(MCL_CURRENT)` or per-allocation `mlock()` is called for pages holding key material on Linux; document Windows equivalent (`VirtualLock`)
- [ ] Remove `client_key_password` from any struct that may be serialised, logged, or copied by value

### Relationships

- Roadmap row: #7 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#10-secure-memory-for-key-material-in-jwks_securitycpp
- Source key: roadmap:7:auth:v1.2.0:10-secure-memory-for-key-material-in-jwks-securitycpp

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:7:auth:v1.2.0:10-secure-memory-for-key-material-in-jwks-securitycpp -->
<!-- roadmap-ref: row=7;module=auth;target=v1.2.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#10-secure-memory-for-key-material-in-jwks_securitycpp -->
