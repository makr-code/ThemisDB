# auth Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: auth
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 289
- Actionable Findings (Critical + High): 205
- Affected Files: 31

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 57 |
| High | 148 |
| Medium | 82 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| uncaught_exception | 52 |
| string_concat_loop | 32 |
| resource_leaked_in_exception | 31 |
| manual_cleanup | 23 |
| uninitialized_access | 14 |
| null_dereference | 12 |
| no_timeout | 10 |
| crypto_weakness | 9 |
| missing_audit_log | 9 |
| blocking_no_timeout | 7 |
| sensitive_data_logging | 7 |
| size_assumption | 7 |
| copy_overhead | 6 |
| data_race | 6 |
| legacy_or_compat_path | 4 |
| exception_in_destructor | 3 |
| explicit_delete | 3 |
| generic_catch | 3 |
| missing_move_constructor_defaulted | 3 |
| no_retry_logic | 3 |
| range_temporary | 3 |
| thread_join_no_timeout | 3 |
| array_bounds_violation | 2 |
| db_connection_leak | 2 |
| explicit_lock_unlock | 2 |
| hardcoded_output | 2 |
| memory_order | 2 |
| module_doc_linkset_drift | 2 |
| pointer_without_null_check | 2 |
| primitive_no_volatile | 2 |
| shift_overflow | 2 |
| smart_ptr_misuse | 2 |
| stale_doc_section_reference | 2 |
| uninitialized_pointer | 2 |
| unordered_container_iter | 2 |
| deadlock_risk | 1 |
| delete_without_nullptr | 1 |
| hardcoded_path | 1 |
| lock_contention | 1 |
| lock_in_loop | 1 |
| manual_cleanup_in_destructor | 1 |
| missing_latency_metric | 1 |
| missing_vector_reserve | 1 |
| nested_loop_find | 1 |
| regex_in_loop | 1 |
| repeated_search | 1 |
| timestamp_sorting_unstable | 1 |
| unsafe_move_assignment | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| auth/jwks_security.cpp | 25 | 8 | 9 | 8 | 0 |
| auth/ldap_authenticator.cpp | 23 | 4 | 4 | 15 | 0 |
| auth/oauth_device_flow.cpp | 21 | 5 | 10 | 6 | 0 |
| auth/federated_identity_manager.cpp | 19 | 2 | 10 | 7 | 0 |
| auth/totp_secret_encryption.cpp | 18 | 2 | 15 | 1 | 0 |
| auth/mtls_authenticator.cpp | 17 | 5 | 4 | 8 | 0 |
| auth/oauth_pkce_flow.cpp | 17 | 2 | 9 | 6 | 0 |
| auth/saml_authenticator.cpp | 16 | 2 | 11 | 3 | 0 |
| auth/api_key_authenticator.cpp | 14 | 2 | 12 | 0 | 0 |
| auth/jwt_validator.cpp | 13 | 4 | 8 | 1 | 0 |
| auth/rate_limiter_backend.cpp | 11 | 0 | 5 | 6 | 0 |
| auth/webauthn_authenticator.cpp | 11 | 0 | 4 | 7 | 0 |
| auth/ldap_connection_pool.cpp | 10 | 3 | 4 | 3 | 0 |
| auth/rocksdb_token_blacklist.cpp | 9 | 1 | 7 | 1 | 0 |
| auth/jwt_key_rotation_manager.cpp | 8 | 0 | 8 | 0 | 0 |
| auth/kerberos_security.cpp | 8 | 0 | 5 | 3 | 0 |
| auth/oidc_provider.cpp | 7 | 2 | 5 | 0 | 0 |
| auth/password_policy.cpp | 7 | 5 | 1 | 1 | 0 |
| auth/zero_trust_auth_verifier.cpp | 6 | 3 | 2 | 1 | 0 |
| auth/auth_rate_limiter.cpp | 5 | 0 | 5 | 0 | 0 |
| auth/gssapi_authenticator.cpp | 5 | 1 | 0 | 4 | 0 |
| auth/mfa_authenticator.cpp | 5 | 4 | 0 | 1 | 0 |
| auth/redis_token_blacklist.cpp | 4 | 0 | 4 | 0 | 0 |
| auth/auth_error.cpp | 3 | 1 | 2 | 0 | 0 |
| auth/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| auth/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| auth/auth_audit_logger.cpp | 1 | 0 | 1 | 0 | 0 |
| auth/auth_metrics.cpp | 1 | 0 | 1 | 0 | 0 |
| auth/principal_validator.cpp | 1 | 1 | 0 | 0 | 0 |
| auth/session_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| auth/token_blacklist.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### auth/jwks_security.cpp
Total findings: 25

- Line 43: severity=CRITICAL; category=uninitialized_pointer
  Description: Undefined behavior: potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer declared but not initialized
- Line 186: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 303: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->last_stats.pinning_verified = (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode:
- Line 311: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::NONE) {
- Line 315: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::PUBLIC_KEY) {
- Line 355: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* fp = fopen(cert_path.c_str(), "r");
- Line 424: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* fp = fopen(cert_path.c_str(), "r");
- Line 451: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* fp = fopen(cert_path.c_str(), "r");
- Line 35: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 36: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 46: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string result(bufferPtr->data, bufferPtr->length);
- Line 46: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 284: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    

    if (res != CURLE_OK) {

        throw std::runtime_error("JWKS fetch failed: " + std::string(curl_easy_strerror(res)));

    }

    

    // Get response code
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &response_code);

    

    if (response_code != 200) {

        throw std::runtime_error("JWKS fetch returned HTTP " + std::to_string(response_code));

    }

    

    // Update stats
- Line 357: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Read certificate file

    FILE* fp = fopen(cert_path.c_str(), "r");

    if (!fp) {

        throw std::runtime_error("Failed to open certificate: " + cert_path);

    }

    

    X509* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
- Line 364: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: fclose(fp);

    

    if (!cert) {

        throw std::runtime_error("Failed to parse certificate: " + cert_path);

    }

    

    // Extract SPKI (Subject Public Key Info)
- Line 373: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (spki_len <= 0) {

        X509_free(cert);

        throw std::runtime_error("Failed to extract SPKI");

    }

    

    // Compute SHA256 hash
- Line 383: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(spki);
- Line 384: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);
- Line 417: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(spki);
- Line 418: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);
- Line 443: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);
- Line 474: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(issuer);
- Line 487: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 490: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);

### auth/ldap_authenticator.cpp
Total findings: 23

- Line 281: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: LDAPAuthResult LDAPAuthenticator::authenticate(const std::string& username,
- Line 370: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return this->authenticate(username, password);
- Line 403: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pooled_conn = pool_->checkout();
- Line 568: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: pooled_conn = pool_->checkout();
- Line 253: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
- Line 253: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
- Line 335: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate inputs synchronously on the caller's thread to give fast
- Line 428: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ldap_init is deprecated in newer SDKs but still universally available
- Line 87: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "\\#";
- Line 129: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '*':  out += "\\2a"; break;
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '*':  out += "\\2a"; break;
- Line 131: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '(':  out += "\\28"; break;
- Line 132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case ')':  out += "\\29"; break;
- Line 133: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\5c"; break;
- Line 134: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\0': out += "\\00"; break;
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back(mapping.role);
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back(config_.default_role);
- Line 524: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ldap_value_free(vals);
- Line 527: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ldap_msgfree(result);
- Line 690: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ldap_msgfree(result);
- Line 727: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LDAP Group Membership (v1' that was not found in 'src/auth/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/auth/FUTURE_ENHANCEMENTS.md § "LDAP Group Membership (v1.6.0)"
- Line 758: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) {

        try {

            return fn(username, dn, password);

        } catch (...) {

            return LDAPAuthResult::Failed("LdapBindFn threw an exception");

        }

    if (auto bind_fn = getLdapBindFn(); bind_fn) {
- Line 758: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### auth/oauth_device_flow.cpp
Total findings: 21

- Line 240: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // High-level authenticate()
- Line 243: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: JWTClaims OAuthDeviceFlow::authenticate(std::function<void(const DeviceCodeResponse &)> progress_cb) {
- Line 279: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::debug("OAuthDeviceFlow: slow_down received, new interval={}s", poll_interval);
- Line 371: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: break;

        }

        if (still_running) {

            mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);

        }

    } while (still_running && mc == CURLM_OK);
- Line 371: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4113 feat(auth): Async / Non-Blo... (2026-03-12) | #3296 [auth] Add configur
- Line 44: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: OAuthDeviceFlow::OAuthDeviceFlow(const Config &config) : config_(config) {

    if (config_.device_authorization_endpoint.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration error",

                                      "device_authorization_endpoint must not be empty"));

    }

    if (config_.token_endpoint.empty()) {
- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "device_authorization_endpoint must not be empty"));

    }

    if (config_.token_endpoint.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration error",

                                      "token_endpoint must not be empty"));

    }

    if (config_.client_id.empty()) {
- Line 52: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "token_endpoint must not be empty"));

    }

    if (config_.client_id.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration error",

                                      "client_id must not be empty"));

    }

}
- Line 196: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const std::string desc = j.value("error_description", "");

        spdlog::error("OAuthDeviceFlow: token endpoint error '{}': {}", err, desc);

        status_out = PollStatus::Error;

        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Token endpoint error",

                                      "error=" + err + " description=" + desc));

    }
- Line 227: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: JWTClaims OAuthDeviceFlow::validateIdToken(const TokenResponse &token_response) {

    if (token_response.id_token.empty()) {

        throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM, "No id_token in token response",

                                      "id_token is empty; ensure 'openid' scope was requested"));

    }

    if (config_.jwks_url.empty()) {
- Line 231: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "id_token is empty; ensure 'openid' scope was requested"));

    }

    if (config_.jwks_url.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "JWKS URL not configured",

                                      "jwks_url must be set in OAuthDeviceFlow::Config to validate id_token"));

    }
- Line 256: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(poll_interval));
- Line 279: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 397: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error(std::string("libcurl multi error: ") + curl_multi_strerror(mc));

    }

    if (easy_rc != CURLE_OK) {

        throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));

    }



    return response_body;
- Line 76: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: scope_str += ' ';
- Line 77: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: scope_str += ' ';
- Line 243: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 426: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += '&';
- Line 427: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += '&';
- Line 430: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += '=';

### auth/federated_identity_manager.cpp
Total findings: 19

- Line 341: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: break;

        }

        if (still_running) {

            mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);

        }

    } while (still_running && mc == CURLM_OK);
- Line 341: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 211: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const auto it = realms_.find(iss);

        if (it == realms_.end()) {

            spdlog::warn("FederatedIdentityManager: no realm registered for issuer '{}'", iss);

            throw AuthException(AuthError(AuthErrorCode::JWT_ISSUER_MISMATCH, "Token issuer is not trusted",

                                          "No realm registered for issuer '" + iss + "'"));

        }

        provider = it->second;
- Line 281: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: char *enc_val = curl_easy_escape(curl, params[i].second.c_str(), static_cast<int>(params[i].second.size()));

        if (!enc_val) {

            curl_easy_cleanup(curl);

            throw std::runtime_error("curl_easy_escape failed to URL-encode form value");

        }

        body += enc_val;

        curl_free(enc_val);
- Line 366: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_cleanup(curl);



    if (mc != CURLM_OK) {

        throw std::runtime_error(std::string("libcurl multi error: ") + curl_multi_strerror(mc));

    }

    if (easy_rc != CURLE_OK) {

        throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error(std::string("libcurl multi error: ") + curl_multi_strerror(mc));

    }

    if (easy_rc != CURLE_OK) {

        throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));

    }

    if (http_code < 200 || http_code >= 300) {

        throw std::runtime_error("HTTP " + std::to_string(http_code) + " from " + url);
- Line 372: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));

    }

    if (http_code < 200 || http_code >= 300) {

        throw std::runtime_error("HTTP " + std::to_string(http_code) + " from " + url);

    }



    return response_body;
- Line 407: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::warn("FederatedIdentityManager::exchangeToken: "

                         "no realm registered for issuer '{}'",

                         iss);

            throw AuthException(AuthError(AuthErrorCode::JWT_ISSUER_MISMATCH, "Token issuer is not trusted",

                                          "No realm registered for issuer '" + iss + "'"));

        }

        provider = it->second;
- Line 425: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const std::string token_endpoint = provider->discoveryDocument().token_endpoint;



    if (token_endpoint.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Token exchange not available",

                                      "Realm '" + iss + "' discovery document does not contain a token_endpoint"));

    }
- Line 432: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Reject non-HTTPS endpoints to prevent accidental secret leakage over

    // cleartext connections (RFC 8693 §2.1 mandates TLS for the token endpoint).

    if (token_endpoint.compare(0, 8, "https://") != 0) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Token exchange requires a secure connection",

                                      "token_endpoint '" + token_endpoint + "' must use HTTPS"));

    }
- Line 506: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (!j.contains("access_token") || !j["access_token"].is_string()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR,

                                      "Token exchange response missing access_token",

                                      "IdP response did not contain a string 'access_token' field"));

    }
- Line 545: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::warn("FederatedIdentityManager::exchangeToken: "

                             "required scope '{}' not in granted scope '{}' for realm '{}'",

                             required, result.scope, iss);

                throw AuthException(AuthError(

                    AuthErrorCode::AUTH_INSUFFICIENT_PERMISSIONS, "Exchanged token is missing a required scope",

                    "Required scope '" + required + "' was not granted; returned scope: '" + result.scope + "'"));

            }
- Line 85: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: b64 += '=';
- Line 86: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: b64 += '=';
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += '&';
- Line 265: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += '&';
- Line 455: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: scope_str += ' ';
- Line 456: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: scope_str += ' ';
- Line 532: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> granted;

### auth/totp_secret_encryption.cpp
Total findings: 18

- Line 41: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 72: severity=CRITICAL; category=uninitialized_pointer
  Description: Undefined behavior: potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer declared but not initialized
- Line 47: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: OPENSSL_cleanse(config.master_key.data(), config.master_key.size() * sizeof(uint8_t));
- Line 64: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 65: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 75: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string result(bufferPtr->data, bufferPtr->length);
- Line 75: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 87: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 88: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 286: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 290: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 347: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 358: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 359: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 360: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 361: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 362: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 265: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_CIPHER_CTX_free(ctx);

### auth/mtls_authenticator.cpp
Total findings: 17

- Line 111: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 170: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: MTLSClaims MTLSAuthenticator::authenticate(const std::string &cert_pem) {
- Line 306: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return authenticate(pem);
- Line 474: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // IPv4: 4 bytes, IPv6: 16 bytes

            const unsigned char *data = ASN1_STRING_get0_data(gn->d.iPAddress);

            const int len             = ASN1_STRING_length(gn->d.iPAddress);

            if (data && len == 4) {

                std::ostringstream oss;

                oss << static_cast<int>(data[0]) << '.' << static_cast<int>(data[1]) << '.' << static_cast<int>(data[2])

                    << '.' << static_cast<int>(data[3]);

                result.push_back(oss.str());

            } else if (data && len == 16) {

                std::ostringstream oss;

                oss << std::hex;
- Line 475: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const unsigned char *data = ASN1_STRING_get0_data(gn->d.iPAddress);

            const int len             = ASN1_STRING_length(gn->d.iPAddress);

            if (data && len == 4) {

                std::ostringstream oss;

                oss << static_cast<int>(data[0]) << '.' << static_cast<int>(data[1]) << '.' << static_cast<int>(data[2])

                    << '.' << static_cast<int>(data[3]);

                result.push_back(oss.str());

            } else if (data && len == 16) {

                std::ostringstream oss;

                oss << std::hex;

                for (int b = 0; b < 16; b += 2) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2969 [auth] Wire up mTLS authent... (2026-03-12) | #2777 feat(auth): Impleme
- Line 120: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 305: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string pem(bptr->data, bptr->length);
- Line 394: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return std::string(bptr->data, bptr->length);
- Line 33: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(p);
- Line 38: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_free(p);
- Line 43: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_CTX_free(p);
- Line 53: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_CRL_free(p);
- Line 273: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(utf8);
- Line 372: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(utf8);
- Line 465: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result.emplace_back(reinterpret_cast<const char *>(data), static_cast<size_t>(len));
- Line 491: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: GENERAL_NAMES_free(san_names);

### auth/oauth_pkce_flow.cpp
Total findings: 17

- Line 289: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: break;

        }

        if (still_running) {

            mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);

        }

    } while (still_running && mc == CURLM_OK);
- Line 289: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4113 feat(auth): Async / Non-Blo... (2026-03-12) | #3311 fix(auth): register
- Line 43: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: OAuthPKCEFlow::OAuthPKCEFlow(const Config &config) : config_(config) {

    if (config_.authorization_endpoint.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",

                                      "authorization_endpoint must not be empty"));

    }

    if (config_.token_endpoint.empty()) {
- Line 47: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "authorization_endpoint must not be empty"));

    }

    if (config_.token_endpoint.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",

                                      "token_endpoint must not be empty"));

    }

    if (config_.client_id.empty()) {
- Line 51: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "token_endpoint must not be empty"));

    }

    if (config_.client_id.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",

                                      "client_id must not be empty"));

    }

    if (config_.redirect_uri.empty()) {
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "client_id must not be empty"));

    }

    if (config_.redirect_uri.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",

                                      "redirect_uri must not be empty"));

    }

}
- Line 221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: JWTClaims OAuthPKCEFlow::validateIdToken(const TokenResponse &token_response) {

    if (token_response.id_token.empty()) {

        throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM, "No id_token in token response",

                                      "id_token is empty; ensure 'openid' scope was requested"));

    }

    if (config_.jwks_url.empty()) {
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "id_token is empty; ensure 'openid' scope was requested"));

    }

    if (config_.jwks_url.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "JWKS URL not configured",

                                      "jwks_url must be set in OAuthPKCEFlow::Config to validate id_token"));

    }
- Line 315: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error(std::string("libcurl multi error: ") + curl_multi_strerror(mc));

    }

    if (easy_rc != CURLE_OK) {

        throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));

    }



    return response_body;
- Line 331: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return;

    }

    if (RAND_bytes(buf, static_cast<int>(len)) != 1) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to generate secure random bytes",

                                      "OpenSSL RAND_bytes returned error"));

    }

}
- Line 121: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: scope_str += ' ';
- Line 122: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: scope_str += ' ';
- Line 374: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += (i + 1 < len) ? kTable[(triple >> 6) & 0x3F] : '=';
- Line 414: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += '&';
- Line 415: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += '&';
- Line 418: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += '=';

### auth/saml_authenticator.cpp
Total findings: 16

- Line 336: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: digest_md = EVP_sha1();
- Line 359: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: sig_md = EVP_sha1();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4787 Security hardening in auth/... (2026-04-22) | #4746 Add Q2 2026 Waveâ€‘
- Line 63: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: SAMLAuthenticator::~SAMLAuthenticator() {
- Line 182: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 196: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string encoded(buf_ptr->data, buf_ptr->length);
- Line 237: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 318: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Reject unless the operator has explicitly enabled the legacy fallback.
- Line 629: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 678: severity=HIGH; category=crypto_weakness
  Description: weak_cipher_des_usage: DES/3DES/Blowfish cipher — use AES-256
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cipher            = EVP_aes_256_cbc();
- Line 681: severity=HIGH; category=crypto_weakness
  Description: weak_cipher_des_usage: DES/3DES/Blowfish cipher — use AES-256
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cipher            = EVP_aes_128_cbc();
- Line 686: severity=HIGH; category=crypto_weakness
  Description: weak_cipher_des_usage: DES/3DES/Blowfish cipher — use AES-256
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cipher            = EVP_aes_256_cbc();
- Line 689: severity=HIGH; category=crypto_weakness
  Description: weak_cipher_des_usage: DES/3DES/Blowfish cipher — use AES-256
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: cipher            = EVP_aes_128_cbc();
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " AllowCreate=\"true\"/>"
- Line 222: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: url += "&RelayState=" + urlEncode(relay_state);
- Line 610: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(sp_pkey);

### auth/api_key_authenticator.cpp
Total findings: 14

- Line 76: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: ApiKeyClaims ApiKeyAuthenticator::authenticate(const std::string& key_id,
- Line 205: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return authenticate(combined.substr(0, dot), combined.substr(dot + 1));
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4094 fix(auth): constant-time co... (2026-03-12) | #2733 [auth] API key auth
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void ApiKeyAuthenticator::addCredential(const ApiKeyCredential& credential) {

    if (credential.key_id.empty()) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_CONFIG_INVALID,

            "API key credential error",

            "key_id must not be empty"
- Line 47: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    // A valid SHA-256 hex digest is exactly 64 lower-case hex characters.

    if (credential.secret_hash.size() != 64) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_CONFIG_INVALID,

            "API key credential error",

            "secret_hash must be a 64-character SHA-256 hex digest"
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

    // --- Input validation ---------------------------------------------------

    if (key_id.empty()) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_INVALID_CREDENTIALS,

            "Authentication failed",

            "key_id must not be empty"
- Line 88: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ));

    }

    if (key_id.size() > config_.max_key_id_length) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_INVALID_CREDENTIALS,

            "Authentication failed",

            "key_id exceeds maximum allowed length"
- Line 95: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ));

    }

    if (secret.empty()) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_INVALID_CREDENTIALS,

            "Authentication failed",

            "secret must not be empty"
- Line 102: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ));

    }

    if (secret.size() > config_.max_secret_length) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_INVALID_CREDENTIALS,

            "Authentication failed",

            "secret exceeds maximum allowed length"
- Line 120: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: AuthAuditLogger al(audit_logger_);

                al.logApiKeyFailure(key_id, "key_id_not_found");

            }

            throw AuthException(AuthError(

                AuthErrorCode::API_KEY_INVALID,

                "Authentication failed",

                "key_id not found: " + key_id
- Line 136: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: AuthAuditLogger al(audit_logger_);

            al.logApiKeyFailure(key_id, "key_inactive");

        }

        throw AuthException(AuthError(

            AuthErrorCode::API_KEY_INACTIVE,

            "Authentication failed",

            "API key is inactive: " + key_id
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: AuthAuditLogger al(audit_logger_);

                al.logApiKeyFailure(key_id, "key_expired");

            }

            throw AuthException(AuthError(

                AuthErrorCode::API_KEY_EXPIRED,

                "Authentication failed",

                "API key has expired: " + key_id
- Line 199: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ApiKeyClaims ApiKeyAuthenticator::authenticateCombined(const std::string& combined) {

    const auto dot = combined.find('.');

    if (dot == std::string::npos || dot == 0 || dot + 1 >= combined.size()) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_INVALID_CREDENTIALS,

            "Authentication failed",

            "Combined API key must be in '<key_id>.<secret>' format"
- Line 216: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: unsigned char digest[SHA256_DIGEST_LENGTH];

    if (SHA256(reinterpret_cast<const unsigned char*>(secret.data()),

               secret.size(), digest) == nullptr) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_INTERNAL_ERROR,

            "Internal error",

            "OpenSSL SHA256 failed"

### auth/jwt_validator.cpp
Total findings: 13

- Line 138: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

        std::unique_lock<std::mutex> refresh_lock(jwks_refresh_mutex_);

        // Wait if another thread is already refreshing.

        jwks_refresh_cv_.wait(refresh_lock, [this] { return !jwks_refreshing_; });



        // Double-check: the refreshing thread may have just updated the cache.

        {
- Line 138: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: jwks_refresh_cv_.wait(refresh_lock, [this] { return !jwks_refreshing_; });
- Line 212: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: break;

                            }

                            if (still_running) {

                                curl_multi_wait(multi, nullptr, 0, 1000, nullptr);

                            }

                        } while (still_running && mc == CURLM_OK);
- Line 212: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: curl_multi_wait(multi, nullptr, 0, 1000, nullptr);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4386 [WIP] Update documentation ... (2026-03-22) | #4279 feat(auth): JWT sco
- Line 136: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> refresh_lock(jwks_refresh_mutex_);
- Line 159: severity=HIGH; category=unsafe_move_assignment
  Description: Move assignment must guarantee strong exception safety
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 245: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 334: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Use EVP_PKEY directly instead of deprecated RSA_new()
- Line 334: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 342: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: #pragma warning(disable : 4996) // OpenSSL deprecated APIs
- Line 867: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "jwt/token/" + claims.jti,

                                            {{"reason", "token_revoked"}, {"jti", claims.jti}});

        }

        throw std::runtime_error("Token has been revoked");

    }

    if (audit_logger_) {

        nlohmann::json d;
- Line 93: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: base64 += '=';

### auth/rate_limiter_backend.cpp
Total findings: 11

- Line 133: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!ctx_ || ctx_->err) {
- Line 136: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx_ ? ctx_->errstr : "allocation failure");
- Line 170: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: RedisRateLimiterBackend::RedisRateLimiterBackend(const Config& config)

    : config_(config)

{

    connect();

}



RedisRateLimiterBackend::~RedisRateLimiterBackend()
- Line 209: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_WARN("RedisRateLimiterBackend::increment: command failed: {}", ctx_->errstr);
- Line 287: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool RedisRateLimiterBackend::reconnect()

{

    std::lock_guard<std::mutex> lock(mutex_);

    return connect();

}



#else // !THEMIS_ENABLE_REDIS
- Line 193: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Each sorted-set member must be unique; use timestamp + per-node counter.
- Line 301: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Redis Rate Limiter Activation' that was not found in 'src/auth/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/auth/FUTURE_ENHANCEMENTS.md §"Redis Rate Limiter Activation"
- Line 383: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) {

        try {

            return fn();

        } catch (...) {

            return false;

        }

    }
- Line 383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 400: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) {

        try {

            return fn();

        } catch (...) {

            return false;

        }

    }
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### auth/webauthn_authenticator.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2969 [auth] Wire up mTLS authent... (2026-03-12) | #2822 [auth] WebAuthn/FID
- Line 500: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: WebAuthnAuthenticator::WebAuthnAuthenticator(const RelyingParty &rp) : rp_(rp), expected_origin_("https://" + rp.id) {

    if (rp.id.empty()) {

        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "WebAuthn configuration error",

                                      "RelyingParty.id must not be empty"));

    }

    spdlog::info("WebAuthnAuthenticator: initialized for RP '{}' (origin: {})", rp_.id, expected_origin_);
- Line 962: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1013: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 360: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += (i + 1 < len) ? kB64Table[(t >> 6) & 0x3F] : '=';
- Line 389: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: padded += '=';
- Line 390: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: padded += '=';
- Line 617: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 728: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 982: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(der);
- Line 1032: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(der);

### auth/ldap_connection_pool.cpp
Total findings: 10

- Line 146: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // other callers while we wait for the LDAP round-trip.

            lock.unlock();

            const bool healthy = isHealthy(candidate);

            lock.lock();



            if (closing_) {

                // Pool shut down while we were health-checking; evict and bail.
- Line 146: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 239: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("LDAPConnectionPool: failed to disable referrals on new connection");
- Line 108: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (active_count_.load(std::memory_order_acquire) > 0) {
- Line 146: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 171: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 239: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 105: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kShutdownWaitMs = 5000;
- Line 334: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ldap_msgfree(result);
- Line 344: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void LDAPConnectionPool::destroyHandle(LDAP *handle) noexcept {

### auth/rocksdb_token_blacklist.cpp
Total findings: 9

- Line 145: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: purge_thread_.join();
- Line 105: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::vector<rocksdb::ColumnFamilyHandle *> cf_handles;

    rocksdb::Status s = rocksdb::DB::Open(rocksdb::DBOptions{opts}, config_.db_path, cf_descs, &cf_handles, &db_);

    if (!s.ok()) {

        throw std::runtime_error("RocksDBTokenBlacklist: failed to open DB at '" + config_.db_path

                                 + "': " + s.ToString());

    }
- Line 124: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete db_;
- Line 148: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: purge_thread_.join();

    }



    // Close DB: destroy all CF handles first, then delete the DB pointer.

    if (cf_) {

        db_->DestroyColumnFamilyHandle(cf_);

        cf_ = nullptr;
- Line 148: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Close DB: destroy all CF handles first, then delete the DB pointer.
- Line 158: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete db_;
- Line 259: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(cv_mutex_);
- Line 260: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lk, std::chrono::seconds(config_.purge_interval_seconds), [this] { return !running_.loa
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: existing_cfs.push_back(config_.column_family);

### auth/jwt_key_rotation_manager.cpp
Total findings: 8

- Line 42: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 63: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 64: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 65: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 66: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 67: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 68: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 79: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### auth/kerberos_security.cpp
Total findings: 8

- Line 750: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<uint8_t *>(&init_addrtype) + sizeof(uint32_t));
- Line 755: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<uint8_t *>(&init_len) + sizeof(uint32_t));
- Line 763: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<uint8_t *>(&acc_addrtype) + sizeof(uint32_t));
- Line 768: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<uint8_t *>(&acc_len) + sizeof(uint32_t));
- Line 776: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<uint8_t *>(&app_len) + sizeof(uint32_t));
- Line 87: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    length = 0;', '    for (size_t i = 0; i < nb; ++i) {', '        length = (length << 8u) | data[offset++];', '    }', '    return true;']
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: sname_str += '/';
- Line 508: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        tag.length = 0;', '        for (size_t i = 0; i < num_octets; i++) {', '            tag.length = (tag.length << 8) | data[offset++];', '        }', '    }']

### auth/oidc_provider.cpp
Total findings: 7

- Line 242: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string("libcurl multi error: ") + curl_multi_strerror(mc));

        }

        if (still_running) {

            mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);

            if (mc != CURLM_OK) {

                curl_multi_remove_handle(multi, curl);

                curl_multi_cleanup(multi);
- Line 242: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
- Line 152: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (discovery_doc_->device_authorization_endpoint.empty()) {

        throw AuthException(AuthError(

            AuthErrorCode::AUTH_CONFIG_INVALID,

            "OIDC provider does not support device authorization",

            "device_authorization_endpoint is absent from the discovery document"
- Line 247: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_multi_remove_handle(multi, curl);

                curl_multi_cleanup(multi);

                curl_easy_cleanup(curl);

                throw std::runtime_error(

                    std::string("libcurl multi wait error: ") + curl_multi_strerror(mc));

            }

        }
- Line 275: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_cleanup(curl);



    if (easy_rc != CURLE_OK) {

        throw std::runtime_error(

            std::string("libcurl error: ") + curl_easy_strerror(easy_rc));

    }

    if (http_code != 200) {
- Line 279: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string("libcurl error: ") + curl_easy_strerror(easy_rc));

    }

    if (http_code != 200) {

        throw std::runtime_error(

            "HTTP " + std::to_string(http_code) + " from " + url);

    }
- Line 290: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: OIDCDiscoveryDocument OIDCProvider::parseDiscovery(const std::string& json_body) {

    const auto j = nlohmann::json::parse(json_body);

    if (!j.is_object()) {

        throw std::runtime_error("Discovery document is not a JSON object");

    }



    OIDCDiscoveryDocument doc;

### auth/password_policy.cpp
Total findings: 7

- Line 40: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: msg << "Password must be at least " << config_.min_length << " characters long";
- Line 46: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: msg << "Password must not exceed " << config_.max_length << " characters";
- Line 85: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: msg << "Password must contain at least " << config_.min_unique_chars << " distinct characters";
- Line 98: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: msg << "Password must not contain more than " << config_.max_consecutive_identical
- Line 114: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: msg << "Password entropy (" << static_cast<int>(entropy) << " bits) is below the required minimum of "
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3296 [auth] Add configurable Sha... (2026-03-12) | #2825 feat(auth): Enforce
- Line 153: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<char, int> freq;

### auth/zero_trust_auth_verifier.cpp
Total findings: 6

- Line 43: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitor_thread_.join();
- Line 183: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitor_thread_.join();
- Line 288: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: worker_pool_->submit([this, e = entry]() {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4141 feat(auth): Zero-Trust Asyn... (2026-03-13) | #3311 fix(auth): register
- Line 223: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (;;) {
- Line 145: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool needs_spawn = false;

### auth/auth_rate_limiter.cpp
Total findings: 5

- Line 478: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(day, sizeof(day), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
- Line 478: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(day, sizeof(day), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
- Line 589: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!cs_redis_ctx_ || cs_redis_ctx_->err) {
- Line 743: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stat_total_auth_attempts_.store(0, std::memory_order_relaxed);
- Line 744: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stat_allowed_attempts_.store(0, std::memory_order_relaxed);

### auth/gssapi_authenticator.cpp
Total findings: 5

- Line 140: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from major_status never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: major_status = gss_acquire_cred(
- Line 198: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: roles_str += ", ";
- Line 199: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: roles_str += ", ";
- Line 346: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back(mapping.role);
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back("readonly");

### auth/mfa_authenticator.cpp
Total findings: 5

- Line 118: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: secret
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: << "?secret=" << enrollment.secret_base32
- Line 288: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::vector<uint8_t> hash = hmacSHA1(secret, counter_bytes);
- Line 364: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::vector<uint8_t> MFAAuthenticator::hmacSHA1(
- Line 371: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: HMAC(EVP_sha1(),
- Line 332: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: output.push_back((buffer >> (bits_left - 8)) & 0xFF);

### auth/redis_token_blacklist.cpp
Total findings: 4

- Line 44: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!ctx_ || ctx_->err) {
- Line 47: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx_ ? ctx_->errstr : "allocation failure");
- Line 88: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: RedisTokenBlacklist::RedisTokenBlacklist(const Config& config)

    : config_(config)

{

    connect();

}



RedisTokenBlacklist::~RedisTokenBlacklist() {
- Line 129: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: jti, ctx_->errstr);

### auth/auth_error.cpp
Total findings: 3

- Line 206: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: ss << "auth-";
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4144 feat(auth): SAML Assertion ... (2026-03-13) | #2826 feat(auth): improve
- Line 106: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (; it != end; ++it) {

### auth/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### auth/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### auth/auth_audit_logger.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4141 feat(auth): Zero-Trust Asyn... (2026-03-13) | #4120 feat(auth): TOTP/MF

### auth/auth_metrics.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4133 feat(auth): Credential Stuf... (2026-03-12) | #4120 feat(auth): TOTP/MF

### auth/principal_validator.cpp
Total findings: 1

- Line 98: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto abac = abac_engine_->authorize(principal, action, resource, ctx.ip_address, ctx.user_agent);

### auth/session_manager.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4094 fix(auth): constant-time co... (2026-03-12) | #2811 [auth] Wire session

### auth/token_blacklist.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4126 feat(auth): Token Blacklist... (2026-03-12) | #3378 feat(auth): Real-ti

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
