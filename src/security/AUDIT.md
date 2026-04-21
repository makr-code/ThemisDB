> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
# Audit Report — Security Module

**Last Audit:** 2026-04-21 | **Status:** 🔴 Critical — 2 S0 findings block all logins; 1 S1 HMAC bypass

> **Note:** Previous audit claimed "Security Issues: None critical". Source code analysis found
> two guaranteed authentication deadlocks (S0) that prevent any user from logging in,
> plus a platform-conditional HMAC bypass in the cache coordinator (S0).
> Header quality scores of 97–100/100 do not reflect actual code correctness.

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified (cmake/ModularBuild.cmake) |
| Test Coverage | ✅ 7 focused test targets |
| S0 Critical / Safety Violations | 🔴 2 (authentication permanently deadlocked) |
| S1 High | 🔴 3 |
| S2 Medium | ⚠️ 4 |
| S3 Low | ℹ️ 2 |
| Successful login possible | 🔴 **No — guaranteed deadlock in `authenticate()`** |

## Source Files Audited

| Component | Files | Safety Status |
|-----------|-------|---------------|
| Access control | `access_control.cpp`, `access_control_manager.cpp`, `rbac.cpp`, `row_level_security.cpp`, `zero_trust_policy_enforcer.cpp` | 🔴 S0: `authenticate()` + `changePassword()` guaranteed deadlock |
| Cryptography | `post_quantum_crypto.cpp`, `field_encryption.cpp`, `encrypted_field.cpp`, `fips_crypto_mode.cpp`, `cms_signing.cpp` | 🔴 S1: key material written to disk; mock key provider reachable in production |
| HSM & key providers | `hsm_provider.cpp`, `hsm_provider_pkcs11.cpp`, `hsm_key_provider_adapter.cpp`, `hsm_signing.cpp`, `keyprovider_signing.cpp`, `key_cache.cpp`, `mock_key_provider.cpp` | ✅ No critical findings |
| PKI & certificates | `vcc_pki_client.cpp`, `pki_key_provider.cpp`, `timestamp_authority.cpp`, `timestamp_authority_openssl.cpp`, `tsa_api.cpp` | ✅ No critical findings |
| Vault integration | `vault_key_provider.cpp`, `vault_signing_provider.cpp` | ✅ No critical findings |
| Signing & manifests | `manifest_signer.cpp`, `binary_manifest.cpp` | ✅ No critical findings |
| Secrets & evidence | `secret_manager.cpp`, `security_evidence_collector.cpp`, `confidential_computing.cpp` | ✅ No critical findings |
| PII & query masking | `pii_redaction_policy.cpp`, `query_masking_policy.cpp` | ✅ No critical findings |
| Threat detection | `aql_injection_detector.cpp`, `behavioral_anomaly_detector.cpp`, `intent_classifier.cpp`, `malware_scanner.cpp` | ⚠️ S2: SQL injection detection trivially bypassed |
| VRAM security | `vram_secure_clear.cpp` | ✅ No critical findings |
| USB / hardware | `usb_admin_authenticator.cpp`, `usb_volume_hardening.cpp` | ✅ No critical findings |
| User registration plugins | `user_registration_plugin.cpp`, `embedded_user_registration_plugin.cpp`, `arrow_user_registration_plugin.cpp`, `webdav_user_registration_plugin.cpp` | ✅ No critical findings |

## Findings

### S0 — Critical (Guaranteed Authentication Failure / Deadlock)

#### A-1 · `access_control.cpp` · `authenticate()` — Guaranteed deadlock

`authenticate()` acquires `mutex_` at line 133 (non-recursive `std::mutex`), then calls
`getUserRoles()` (line 607) which also acquires `mutex_`, and `createSession()` (line 638)
which also acquires `mutex_`. Both are called unconditionally on every successful
authentication path. **No login of any kind (OAuth, password, token) can succeed.**

```cpp
std::lock_guard<std::mutex> lock(mutex_);   // L133 — acquired
...
auto roles = getUserRoles(result.user_id);   // → L607: lock(mutex_) → DEADLOCK
...
auto session_token = createSession(credentials.user_id, roles, ...);  // → L638: lock(mutex_)
```

**Fix required:** Restructure `AccessControl` to use an internal unlocked variant of
`getUserRoles_` and `createSession_` callable only while the caller already holds the lock,
or split into separate mutex-gated public and lock-free private APIs.

---

#### A-2 · `access_control.cpp` · `changePassword()` — Guaranteed deadlock

`changePassword()` acquires `mutex_` at line 317, then calls `invalidateUserSessions()`
(line 716) which also acquires `mutex_`. **No password change can complete.**

```cpp
std::lock_guard<std::mutex> lock(mutex_);   // L317 — acquired
...
invalidateUserSessions(user_id);             // → L716: lock(mutex_) → DEADLOCK
```

**Fix required:** Same approach as A-1 — create internal unlocked variants.

---

### S1 — High

#### A-3 · `access_control.cpp` · `enrollMFA()` — MFA enrollment bypass

`enrollMFA()` unconditionally overwrites any existing MFA enrollment without checking whether
the caller is the account owner or an administrator:

```cpp
enrollment.enabled = true;
mfa_enrollments_[user_id] = std::move(enrollment);  // overwrites existing MFA silently
```

Any caller with access to this function can replace a user's TOTP secret, invalidating the
user's authenticator app and gaining control of subsequent OTP verification.

**Fix required:** Require active MFA verification from the existing secret before allowing
re-enrollment. Log and rate-limit all enrollment attempts.

---

#### E-1 · `field_encryption.cpp` · `write_debug_dump()` + `decryptInternal()` — Key material on disk + stderr

`write_debug_dump()` writes the **first 8 bytes of the raw encryption key** to a JSON file
on disk when `THEMIS_DEBUG_ENC_DIR` is set. `encryptInternal()` calls `write_debug_dump()`
unconditionally on every encryption (line 554, not gated on a build flag). Additionally,
`decryptInternal()` always emits `fprintf(stderr, ...)` leaking operation metadata (line 612)
without any debug gate.

```cpp
j["key_fingerprint_prefix"] = kf.str();   // 8 raw key bytes as hex

// decryptInternal(), always:
fprintf(stderr, "decryptInternal: ciphertext_len=%zu, tag_len=%zu, iv_len=%zu, key_len=%zu\n", ...);
```

**Fix required:** Replace raw key bytes with a proper HMAC-based fingerprint. Gate
`write_debug_dump()` on a compile-time `THEMIS_DEBUG` flag (not a runtime env var).
Remove unconditional `fprintf` from `decryptInternal()`.

---

#### E-4 · `field_encryption.cpp` · `createDefault()` — `MockKeyProvider` in production

```cpp
std::shared_ptr<FieldEncryption> FieldEncryption::createDefault() {
    auto mock_provider = std::make_shared<MockKeyProvider>();
    return std::make_shared<FieldEncryption>(mock_provider);
}
```

The default factory method uses a mock key provider. Any code path that calls
`createDefault()` without providing a real `KeyProvider` silently uses static mock keys.

**Fix required:** Remove `createDefault()` or have it throw/abort with a clear diagnostic
requiring an explicit key provider. Replace with `createWithProvider(shared_ptr<KeyProvider>)`.

---

#### E-2 · `field_encryption.cpp` · `encryptEntityBatch()` — Silent per-item encryption failures

```cpp
} catch (...) {
    // ignore per-item errors here
}
```

Failed encryptions in the parallel batch path produce default-constructed `EncryptedBlob`
(empty IV, empty ciphertext) in the output vector. Callers receive a full-size output but
cannot distinguish valid from failed entries. Corrupted records are silently stored.

**Fix required:** Replace silent catch with either propagating the first error, or storing
a per-item error/status in the result, and documenting the failure contract.

---

### S2 — Medium

| ID | File | Function | Description |
|----|------|----------|-------------|
| A-4 | `access_control.cpp` | `detectSQLInjection()` | Case-sensitive exact-match strings — bypassed by `union select`, Unicode lookalikes, inline comments; instills false security confidence |
| A-5 | `access_control.cpp` | `recordFailedLogin()` | Rate-limit lockout stored only in memory; any process restart clears all lockout state — brute-force protection resets on crash/restart |
| E-3 | `field_encryption.cpp` | `needsReEncryption()` | Uses exception as side-channel to detect key versions; transient KMS unavailability silently suppresses re-encryption |
| RB-2 | `rbac.cpp` | Constructor | Cyclic role hierarchy detected and logged, but system continues with corrupt data; all `checkPermission()` calls emit "Cyclic dependency" warnings at runtime |

### S1 — Additional

| ID | File | Function | Description |
|----|------|----------|-------------|
| RB-1 | `rbac.cpp` | `checkPermission()` | License server outage denies ALL permissions system-wide; no fail-open grace period |

### S3 — Low

| ID | File | Function | Description |
|----|------|----------|-------------|
| A-6 | `access_control.cpp` | `getStatistics()` | Duplicate `"active_sessions"` key in JSON output; second silently shadows first |
| RB-3 | `rbac.cpp` | `loadFromJson()` | Mutex acquired inside constructor before object is shared — misleading but harmless |

---

## Findings Summary Table

| ID | Severity | File | Function | Description |
|----|----------|------|----------|-------------|
| A-1 | **S0** | `access_control.cpp` | `authenticate()` | Non-recursive `mutex_` re-acquired via `getUserRoles()` + `createSession()` → guaranteed deadlock; no login possible |
| A-2 | **S0** | `access_control.cpp` | `changePassword()` | Non-recursive `mutex_` re-acquired via `invalidateUserSessions()` → guaranteed deadlock |
| A-3 | **S1** | `access_control.cpp` | `enrollMFA()` | No auth check before overwriting existing MFA enrollment → MFA bypass |
| E-1 | **S1** | `field_encryption.cpp` | `write_debug_dump()` / `decryptInternal()` | 8 raw key bytes written to disk; unconditional `fprintf(stderr)` in decrypt path |
| E-2 | **S1** | `field_encryption.cpp` | `encryptEntityBatch()` | Silent `catch(...)` produces empty `EncryptedBlob` on failure; callers cannot detect |
| E-4 | **S1** | `field_encryption.cpp` | `createDefault()` | `MockKeyProvider` used by default factory — production may silently use mock keys |
| RB-1 | **S1** | `rbac.cpp` | `checkPermission()` | License server outage denies all access with no grace period |
| A-4 | **S2** | `access_control.cpp` | `detectSQLInjection()` | Trivially bypassed case-sensitive exact-match detection |
| A-5 | **S2** | `access_control.cpp` | `recordFailedLogin()` | Brute-force lockout in memory only; reset on process restart |
| E-3 | **S2** | `field_encryption.cpp` | `needsReEncryption()` | Exception side-channel for key version detection; KMS errors suppress re-encryption |
| RB-2 | **S2** | `rbac.cpp` | Constructor | Cyclic role hierarchy detected but not rejected; corrupt data used at runtime |
| A-6 | **S3** | `access_control.cpp` | `getStatistics()` | Duplicate JSON key `"active_sessions"` |
| RB-3 | **S3** | `rbac.cpp` | `loadFromJson()` | Mutex in constructor — misleading but harmless |

---

## Resolved (from 2026-04-19 audit)
- Post-quantum crypto registered in cmake/CMakeLists.txt (March 2026)
- ModularBuild.cmake THEMIS_SECURITY_SOURCES updated with 8 files (March 2026)
- 7 focused test targets added in tests/CMakeLists.txt

## Open (carried forward)
- PKIClient fallback stub verification pending (#issue)
