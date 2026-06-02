# auth Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: auth
- Generated: 2026-06-02 11:55:47
- Status: Critical Findings Present
- Total Findings: 370
- Actionable Findings (Critical + High): 226
- Affected Files: 31

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 39 |
| High | 187 |
| Medium | 144 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 115 |
| performance_patterns | 62 |
| exception_safety | 38 |
| container | 34 |
| raii | 26 |
| performance | 21 |
| audit_logging | 18 |
| memory | 14 |
| security | 12 |
| concurrency | 9 |
| platform | 9 |
| legacy_duplication | 4 |
| uninitialized | 4 |
| determinism | 3 |
| type_conversion | 2 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/auth/ldap_authenticator.cpp | 36 | 4 | 13 | 19 | 0 |
| src/auth/mtls_authenticator.cpp | 31 | 3 | 18 | 10 | 0 |
| src/auth/federated_identity_manager.cpp | 24 | 1 | 13 | 10 | 0 |
| src/auth/oauth_device_flow.cpp | 24 | 4 | 13 | 7 | 0 |
| src/auth/oauth_pkce_flow.cpp | 23 | 1 | 16 | 6 | 0 |
| src/auth/webauthn_authenticator.cpp | 23 | 0 | 8 | 15 | 0 |
| src/auth/totp_secret_encryption.cpp | 22 | 2 | 15 | 5 | 0 |
| src/auth/jwks_security.cpp | 21 | 8 | 5 | 8 | 0 |
| src/auth/rate_limiter_backend.cpp | 19 | 0 | 13 | 6 | 0 |
| src/auth/saml_authenticator.cpp | 15 | 0 | 5 | 10 | 0 |
| src/auth/api_key_authenticator.cpp | 14 | 2 | 12 | 0 | 0 |
| src/auth/jwt_validator.cpp | 14 | 2 | 8 | 4 | 0 |
| src/auth/jwt_key_rotation_manager.cpp | 13 | 0 | 9 | 4 | 0 |
| src/auth/redis_token_blacklist.cpp | 12 | 0 | 12 | 0 | 0 |
| src/auth/gssapi_authenticator.cpp | 11 | 0 | 0 | 11 | 0 |
| src/auth/password_policy.cpp | 11 | 5 | 1 | 5 | 0 |
| src/auth/kerberos_security.cpp | 8 | 0 | 5 | 3 | 0 |
| src/auth/ldap_connection_pool.cpp | 7 | 2 | 2 | 3 | 0 |
| src/auth/rocksdb_token_blacklist.cpp | 6 | 0 | 3 | 3 | 0 |
| src/auth/oidc_provider.cpp | 5 | 1 | 4 | 0 | 0 |
| src/auth/session_manager.cpp | 5 | 0 | 1 | 4 | 0 |
| src/auth/auth_rate_limiter.cpp | 4 | 0 | 4 | 0 | 0 |
| src/auth/mfa_authenticator.cpp | 4 | 1 | 0 | 3 | 0 |
| src/auth/zero_trust_auth_verifier.cpp | 4 | 1 | 2 | 1 | 0 |
| src/auth/auth_error.cpp | 3 | 1 | 2 | 0 | 0 |
| src/auth/jwks_validator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/auth/principal_validator.cpp | 3 | 1 | 0 | 2 | 0 |
| src/auth/totp_replay_cache.cpp | 2 | 0 | 0 | 2 | 0 |
| src/auth/auth_audit_logger.cpp | 1 | 0 | 1 | 0 | 0 |
| src/auth/auth_metrics.cpp | 1 | 0 | 1 | 0 | 0 |
| src/auth/token_blacklist.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/auth/ldap_authenticator.cpp
Total findings: 36

- Line 281: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: LDAPAuthResult LDAPAuthenticator::authenticate(const std::string& username,
  Confidence: band=very_high; score=0.99
- Line 370: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return this->authenticate(username, password);
  Confidence: band=very_high; score=0.99
- Line 403: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pooled_conn = pool_->checkout();
- Line 568: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pooled_conn = pool_->checkout();
- Line 253: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
- Line 253: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
- Line 286: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 293: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 301: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 308: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 335: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs synchronously on the caller's thread to give fast
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 345: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 352: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 359: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 428: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ldap_init is deprecated in newer SDKs but still universally available
  Confidence: band=high; score=0.8
- Line 87: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\#";
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '*':  out += "\\2a"; break;
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '*':  out += "\\2a"; break;
- Line 131: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '(':  out += "\\28"; break;
- Line 132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case ')':  out += "\\29"; break;
- Line 133: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\5c"; break;
- Line 134: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\0': out += "\\00"; break;
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roles.push_back(mapping.role);
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roles.push_back(mapping.role);
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roles.push_back(mapping.role);
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roles.push_back(config_.default_role);
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]);
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]);
  Confidence: band=high; score=0.74
- Line 524: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ldap_value_free(vals);
- Line 527: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ldap_msgfree(result);
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]->bv_val,
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]->bv_val,
  Confidence: band=high; score=0.74
- Line 690: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ldap_msgfree(result);
- Line 758: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/auth/mtls_authenticator.cpp
Total findings: 31

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 170: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: MTLSClaims MTLSAuthenticator::authenticate(const std::string &cert_pem) {
  Confidence: band=very_high; score=0.99
- Line 306: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return authenticate(pem);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2969 [auth] Wire up mTLS authent... (2026-03-12) | #2777 feat(auth): Impleme
- Line 96: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS authenticator configuration
- Line 101: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS authenticator configuration
- Line 106: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS authenticator configuration
- Line 176: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate authentication failed",
- Line 184: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed
- Line 189: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 208: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_EXPIRED, "Certificate not yet valid",
- Line 213: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate authentication failed",
- Line 299: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed
- Line 305: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string pem(bptr->data, bptr->length);
- Line 315: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 344: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate parse error",
- Line 353: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate parse error",
- Line 394: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return std::string(bptr->data, bptr->length);
- Line 33: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(p);
- Line 38: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(p);
- Line 43: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(p);
- Line 53: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_CRL_free(p);
- Line 273: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(utf8);
- Line 372: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(utf8);
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(reinterpret_cast<const char *>(data), static_cast<size_t>(len));
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(oss.str());
- Line 491: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: GENERAL_NAMES_free(san_names);

### src/auth/federated_identity_manager.cpp
Total findings: 24

- Line 341: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 30: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string *>(userdata)->append(ptr, total);
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::JWT_INVALID_FORMAT, "Token exceeds maximum allowed size
- Line 64: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 137: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Realm already registered",
- Line 211: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::JWT_ISSUER_MISMATCH, "Token issuer is not trusted",
- Line 230: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Unknown realm",
- Line 407: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::JWT_ISSUER_MISMATCH, "Token issuer is not trusted",
- Line 425: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Token exchange not available",
- Line 432: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Token exchange requires a secure
- Line 506: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR,
- Line 545: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 85: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: b64 += '=';
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: b64 += '=';
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<char>((val >> bits) & 0xFF));
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issuers.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += '&';
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '&';
- Line 455: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: scope_str += ' ';
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: scope_str += ' ';
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.emplace_back("scope", scope_str);
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> granted;
  Confidence: band=medium; score=0.66

### src/auth/oauth_device_flow.cpp
Total findings: 24

- Line 240: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: // High-level authenticate()
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: JWTClaims OAuthDeviceFlow::authenticate(std::function<void(const DeviceCodeResponse &)> progress_cb) {
  Confidence: band=very_high; score=0.99
- Line 279: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::debug("OAuthDeviceFlow: slow_down received, new interval={}s", poll_interval);
- Line 371: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4113 feat(auth): Async / Non-Blo... (2026-03-12) | #3296 [auth] Add configur
- Line 32: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string *>(userdata)->append(ptr, total);
- Line 44: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration e
- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration e
- Line 52: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration e
- Line 182: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "Authorization denied",
- Line 188: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_TOKEN_EXPIRED, "Device code expired",
- Line 196: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Token endpoint error",
- Line 227: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM, "No id_token in token respo
- Line 231: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "JWKS URL not configured",
- Line 256: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(poll_interval));
- Line 294: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_TOKEN_EXPIRED, "Device authorization timed out",
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 76: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: scope_str += ' ';
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: scope_str += ' ';
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.emplace_back("scope", scope_str);
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += '&';
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '&';
- Line 430: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '=';

### src/auth/oauth_pkce_flow.cpp
Total findings: 23

- Line 289: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4113 feat(auth): Async / Non-Blo... (2026-03-12) | #3311 fix(auth): register
- Line 31: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string *>(userdata)->append(ptr, total);
- Line 43: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
- Line 47: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
- Line 51: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "PKCE token exchange failed",
- Line 155: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "PKCE token exchange failed",
- Line 221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM, "No id_token in token respo
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "JWKS URL not configured",
- Line 331: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to generate secure random
- Line 342: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to compute SHA-256 hash",
- Line 353: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to compute SHA-256 hash",
- Line 368: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b0     = data[i];
- Line 369: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b1     = (i + 1 < len) ? data[i + 1] : 0u;
- Line 370: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b2     = (i + 2 < len) ? data[i + 2] : 0u;
- Line 121: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: scope_str += ' ';
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: scope_str += ' ';
- Line 374: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < len) ? kTable[(triple >> 6) & 0x3F] : '=';
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += '&';
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '&';
- Line 418: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: body += '=';

### src/auth/webauthn_authenticator.cpp
Total findings: 23

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2969 [auth] Wire up mTLS authent... (2026-03-12) | #2822 [auth] WebAuthn/FID
- Line 355: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b0 = data[i];
- Line 356: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b1 = (i + 1 < len) ? data[i + 1] : 0u;
- Line 357: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint32_t b2 = (i + 2 < len) ? data[i + 2] : 0u;
- Line 500: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "WebAuthn configuration error",
- Line 1036: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 360: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < len) ? kB64Table[(t >> 6) & 0x3F] : '=';
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: padded += '=';
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: padded += '=';
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: padded += '=';
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back({{"type", "public-key"}, {"alg", alg_id}});
  Confidence: band=high; score=0.74
- Line 444: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: params.push_back({{"type", "public-key"}, {"alg", alg_id}});
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: excl.push_back({{"type", "public-key"}, {"id", cid}});
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: excl.push_back({{"type", "public-key"}, {"id", cid}});
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allow.push_back({{"type", "public-key"}, {"id", cid}});
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: allow.push_back({{"type", "public-key"}, {"id", cid}});
- Line 982: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(der);
- Line 1032: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(der);

### src/auth/totp_secret_encryption.cpp
Total findings: 22

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 47: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: OPENSSL_cleanse(config.master_key.data(), config.master_key.size() * sizeof(uint8_t));
- Line 75: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string result(bufferPtr->data, bufferPtr->length);
- Line 214: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 265: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: secrets.push_back(new_version);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_secrets.push_back(secret);
  Confidence: band=high; score=0.74

### src/auth/jwks_security.cpp
Total findings: 21

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
- Line 303: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->last_stats.pinning_verified = (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode:
- Line 311: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::NONE) {
- Line 315: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::PUBLIC_KEY) {
- Line 355: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(cert_path.c_str(), "r");
- Line 424: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(cert_path.c_str(), "r");
- Line 451: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(cert_path.c_str(), "r");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
- Line 46: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string result(bufferPtr->data, bufferPtr->length);
- Line 46: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string result(bufferPtr->data, bufferPtr->length);
- Line 383: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(spki);
- Line 384: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 417: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(spki);
- Line 418: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 443: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 474: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(issuer);
- Line 487: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 490: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);

### src/auth/rate_limiter_backend.cpp
Total findings: 19

- Line 121: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RedisRateLimiterBackend::connect()
- Line 133: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!ctx_ || ctx_->err) {
- Line 136: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ctx_ ? ctx_->errstr : "allocation failure");
- Line 159: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void RedisRateLimiterBackend::disconnect()
- Line 170: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: connect();
- Line 176: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 209: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: THEMIS_WARN("RedisRateLimiterBackend::increment: command failed: {}", ctx_->errstr);
- Line 210: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 248: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const_cast<RedisRateLimiterBackend*>(this)->disconnect();
- Line 274: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 284: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RedisRateLimiterBackend::reconnect()
- Line 287: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return connect();
- Line 390: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RedisRateLimiterBackend::reconnect()
- Line 193: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Each sorted-set member must be unique; use timestamp + per-node counter.
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 350: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 367: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/auth/saml_authenticator.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4787 Security hardening in auth/... (2026-04-22) | #4746 Add Q2 2026 Waveâ€‘
- Line 196: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string encoded(buf_ptr->data, buf_ptr->length);
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " AllowCreate=\"true\"/>"
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " AllowCreate=\"true\"/>"
- Line 222: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: url += "&RelayState=" + urlEncode(relay_state);
  Confidence: band=high; score=0.74
- Line 610: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(sp_pkey);
- Line 804: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Missing StatusCode element");
- Line 810: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "SAML Status is not Success: " + status_value);
- Line 1178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.audience.push_back(nodeText(child));
  Confidence: band=high; score=0.74
- Line 1179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.audience.push_back(nodeText(child));
- Line 1240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.raw_attributes.emplace_back(attr_name, val);
  Confidence: band=high; score=0.74
- Line 1240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.raw_attributes.emplace_back(attr_name, val);
  Confidence: band=high; score=0.74

### src/auth/api_key_authenticator.cpp
Total findings: 14

- Line 76: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: ApiKeyClaims ApiKeyAuthenticator::authenticate(const std::string& key_id,
  Confidence: band=very_high; score=0.99
- Line 205: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return authenticate(combined.substr(0, dot), combined.substr(dot + 1));
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4094 fix(auth): constant-time co... (2026-03-12) | #2733 [auth] API key auth
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 47: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 88: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 95: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 102: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 120: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 136: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 199: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 216: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(

### src/auth/jwt_validator.cpp
Total findings: 14

- Line 138: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: jwks_refresh_cv_.wait(refresh_lock, [this] { return !jwks_refreshing_; });
- Line 212: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: curl_multi_wait(multi, nullptr, 0, 1000, nullptr);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4386 [WIP] Update documentation ... (2026-03-22) | #4279 feat(auth): JWT sco
- Line 43: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: out->append(ptr, total);
- Line 136: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::mutex> refresh_lock(jwks_refresh_mutex_);
- Line 245: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 334: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY directly instead of deprecated RSA_new()
  Confidence: band=high; score=0.8
- Line 342: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: #pragma warning(disable : 4996) // OpenSSL deprecated APIs
  Confidence: band=high; score=0.8
- Line 93: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: base64 += '=';
- Line 781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.audience.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 782: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.audience.push_back(v.get<std::string>());
- Line 902: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: revoked_kids_runtime_.push_back(kid);
  Confidence: band=high; score=0.74

### src/auth/jwt_key_rotation_manager.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 50: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::length_error("JWTKeyRotationManager: max_keys limit (" + std::to_string(config_.max_keys)
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(kid);
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kid);
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kid);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kid);
  Confidence: band=high; score=0.74

### src/auth/redis_token_blacklist.cpp
Total findings: 12

- Line 33: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RedisTokenBlacklist::connect() {
- Line 44: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!ctx_ || ctx_->err) {
- Line 47: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ctx_ ? ctx_->errstr : "allocation failure");
- Line 70: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: void RedisTokenBlacklist::disconnect() {
- Line 88: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: connect();
- Line 93: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 129: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: jti, ctx_->errstr);
- Line 130: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: disconnect();
- Line 168: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const_cast<RedisTokenBlacklist*>(this)->disconnect();
- Line 183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return ctx_ != nullptr && ctx_->err == 0;
- Line 186: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RedisTokenBlacklist::reconnect() {
- Line 251: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RedisTokenBlacklist::reconnect() {

### src/auth/gssapi_authenticator.cpp
Total findings: 11

- Line 93: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SECURITY_STATUS status;
- Line 119: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: OM_uint32 major_status, minor_status;
- Line 198: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: roles_str += ", ";
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: roles_str += ", ";
- Line 273: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: OM_uint32 major_status, minor_status;
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roles.push_back(mapping.role);
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roles.push_back(mapping.role);
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roles.push_back("readonly");
- Line 396: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: OM_uint32 minor_status;
- Line 424: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: gss_buffer_desc status_string;
- Line 479: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: oss << "SSPI Error: 0x" << std::hex << major_status;

### src/auth/password_policy.cpp
Total findings: 11

- Line 40: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: msg << "Password must be at least " << config_.min_length << " characters long";
  Confidence: band=very_high; score=0.92
- Line 46: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: msg << "Password must not exceed " << config_.max_length << " characters";
  Confidence: band=very_high; score=0.92
- Line 85: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: msg << "Password must contain at least " << config_.min_unique_chars << " distinct characters";
  Confidence: band=very_high; score=0.92
- Line 98: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: msg << "Password must not contain more than " << config_.max_consecutive_identical
  Confidence: band=very_high; score=0.92
- Line 114: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: msg << "Password entropy (" << static_cast<int>(entropy) << " bits) is below the required minimum of "
  Confidence: band=very_high; score=0.92
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3296 [auth] Add configurable Sha... (2026-03-12) | #2825 feat(auth): Enforce
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back(msg.str());
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back("Password contains a forbidden pattern");
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.violations.push_back("Password contains a forbidden pattern");
- Line 153: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<char, int> freq;
  Confidence: band=medium; score=0.66

### src/auth/kerberos_security.cpp
Total findings: 8

- Line 750: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: reinterpret_cast<uint8_t *>(&init_addrtype) + sizeof(uint32_t));
- Line 755: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: reinterpret_cast<uint8_t *>(&init_len) + sizeof(uint32_t));
- Line 763: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: reinterpret_cast<uint8_t *>(&acc_addrtype) + sizeof(uint32_t));
- Line 768: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: reinterpret_cast<uint8_t *>(&acc_len) + sizeof(uint32_t));
- Line 776: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: reinterpret_cast<uint8_t *>(&app_len) + sizeof(uint32_t));
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    length = 0;', '    for (size_t i = 0; i < nb; ++i) {', '        length = (length << 8u) | data[offset++];', '    }', '    return true;']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        tag.length = 0;', '        for (size_t i = 0; i < num_octets; i++) {', '            tag.length = (tag.length << 8) | data[offset++];', '        }', '    }']
  Confidence: band=medium; score=0.65
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: sname_str += '/';

### src/auth/ldap_connection_pool.cpp
Total findings: 7

- Line 146: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 239: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::error("LDAPConnectionPool: failed to disable referrals on new connection");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 108: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (active_count_.load(std::memory_order_acquire) > 0) {
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idle_.push_back(ld);
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ldap_msgfree(result);
- Line 344: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void LDAPConnectionPool::destroyHandle(LDAP *handle) noexcept {
  Confidence: band=high; score=0.74

### src/auth/rocksdb_token_blacklist.cpp
Total findings: 6

- Line 218: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(ro, cf_));
- Line 259: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(cv_mutex_);
- Line 260: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lk, std::chrono::seconds(config_.purge_interval_seconds), [this] { return !running_.loa
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: existing_cfs.push_back(config_.column_family);
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descs.emplace_back(cf, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: other_cf_handles_.push_back(cf_handles[i]);
  Confidence: band=high; score=0.74

### src/auth/oidc_provider.cpp
Total findings: 5

- Line 242: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 25: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 46: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(
- Line 152: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AuthException(AuthError(

### src/auth/session_manager.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4094 fix(auth): constant-time co... (2026-03-12) | #2811 [auth] Wire session
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user_sessions.emplace_back(info.created_at, id);
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_erase.push_back(id);
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired.push_back(id);
  Confidence: band=high; score=0.74

### src/auth/auth_rate_limiter.cpp
Total findings: 4

- Line 478: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(day, sizeof(day), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
- Line 589: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!cs_redis_ctx_ || cs_redis_ctx_->err) {
- Line 743: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stat_total_auth_attempts_.store(0, std::memory_order_relaxed);
- Line 744: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: stat_allowed_attempts_.store(0, std::memory_order_relaxed);

### src/auth/mfa_authenticator.cpp
Total findings: 4

- Line 118: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: secret
  Context: << "?secret=" << enrollment.secret_base32
  Confidence: band=very_high; score=0.92
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(generateRecoveryCode());
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back((buffer >> (bits_left - 8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back((buffer >> (bits_left - 8)) & 0xFF);

### src/auth/zero_trust_auth_verifier.cpp
Total findings: 4

- Line 288: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: worker_pool_->submit([this, e = entry]() {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4141 feat(auth): Zero-Trust Asyn... (2026-03-13) | #3311 fix(auth): register
- Line 223: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (;;) {
  Confidence: band=very_high; score=0.9
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_eval.push_back(entry);
  Confidence: band=high; score=0.74

### src/auth/auth_error.cpp
Total findings: 3

- Line 206: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: ss << "auth-";
  Confidence: band=very_high; score=0.92
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4144 feat(auth): SAML Assertion ... (2026-03-13) | #2826 feat(auth): improve
- Line 106: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (; it != end; ++it) {
  Confidence: band=very_high; score=0.9

### src/auth/jwks_validator.cpp
Total findings: 3

- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("Duplicate key IDs found");
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("Duplicate key ID: " + kid);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("Duplicate key ID: " + kid);

### src/auth/principal_validator.cpp
Total findings: 3

- Line 98: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto abac = abac_engine_->authorize(principal, action, resource, ctx.ip_address, ctx.user_agent);
  Confidence: band=very_high; score=0.99
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: config.rules.push_back(rule);
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: config.rules.push_back(rule);
  Confidence: band=high; score=0.74

### src/auth/totp_replay_cache.cpp
Total findings: 2

- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user_cache.push_back({code, now});
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: user_cache.push_back({code, now});

### src/auth/auth_audit_logger.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4141 feat(auth): Zero-Trust Asyn... (2026-03-13) | #4120 feat(auth): TOTP/MF

### src/auth/auth_metrics.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4133 feat(auth): Credential Stuf... (2026-03-12) | #4120 feat(auth): TOTP/MF

### src/auth/token_blacklist.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4126 feat(auth): Token Blacklist... (2026-03-12) | #3378 feat(auth): Real-ti

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
