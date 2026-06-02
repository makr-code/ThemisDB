# auth Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: auth
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 86
- Actionable Findings (Critical + High): 23
- Affected Files: 31

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 16 |
| High | 7 |
| Medium | 63 |
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
| src/auth/ldap_authenticator.cpp | 13 | 2 | 3 | 8 | 0 |
| src/auth/password_policy.cpp | 8 | 5 | 0 | 3 | 0 |
| src/auth/federated_identity_manager.cpp | 7 | 0 | 0 | 7 | 0 |
| src/auth/webauthn_authenticator.cpp | 7 | 0 | 0 | 7 | 0 |
| src/auth/oauth_device_flow.cpp | 5 | 2 | 0 | 3 | 0 |
| src/auth/jwt_key_rotation_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/auth/jwt_validator.cpp | 4 | 0 | 2 | 2 | 0 |
| src/auth/mtls_authenticator.cpp | 4 | 2 | 0 | 2 | 0 |
| src/auth/saml_authenticator.cpp | 4 | 0 | 0 | 4 | 0 |
| src/auth/session_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/auth/mfa_authenticator.cpp | 3 | 1 | 0 | 2 | 0 |
| src/auth/oauth_pkce_flow.cpp | 3 | 0 | 0 | 3 | 0 |
| src/auth/principal_validator.cpp | 3 | 1 | 0 | 2 | 0 |
| src/auth/api_key_authenticator.cpp | 2 | 2 | 0 | 0 | 0 |
| src/auth/auth_error.cpp | 2 | 1 | 1 | 0 | 0 |
| src/auth/gssapi_authenticator.cpp | 2 | 0 | 0 | 2 | 0 |
| src/auth/ldap_connection_pool.cpp | 2 | 0 | 0 | 2 | 0 |
| src/auth/rocksdb_token_blacklist.cpp | 2 | 0 | 0 | 2 | 0 |
| src/auth/totp_secret_encryption.cpp | 2 | 0 | 0 | 2 | 0 |
| src/auth/zero_trust_auth_verifier.cpp | 2 | 0 | 1 | 1 | 0 |
| src/auth/jwks_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/auth/rate_limiter_backend.cpp | 1 | 0 | 0 | 1 | 0 |
| src/auth/totp_replay_cache.cpp | 1 | 0 | 0 | 1 | 0 |
| src/auth/auth_audit_logger.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/auth_metrics.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/auth_rate_limiter.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/jwks_security.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/kerberos_security.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/oidc_provider.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/redis_token_blacklist.cpp | 0 | 0 | 0 | 0 | 0 |
| src/auth/token_blacklist.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/auth/ldap_authenticator.cpp
Total findings: 13

- Line 281: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: LDAPAuthResult LDAPAuthenticator::authenticate(const std::string& username,
  Confidence: band=very_high; score=0.99
- Line 370: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return this->authenticate(username, password);
  Confidence: band=very_high; score=0.99
- Line 253: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(roles.begin(), roles.end(), mapping.role) == roles.end()) {
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs synchronously on the caller's thread to give fast
  Confidence: band=very_high; score=0.9
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
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roles.push_back(mapping.role);
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roles.push_back(mapping.role);
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]);
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]);
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]->bv_val,
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.emplace_back(vals[i]->bv_val,
  Confidence: band=high; score=0.74

### src/auth/password_policy.cpp
Total findings: 8

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
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back(msg.str());
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.violations.push_back("Password contains a forbidden pattern");
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<char, int> freq;
  Confidence: band=medium; score=0.66

### src/auth/federated_identity_manager.cpp
Total findings: 7

- Line 85: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: b64 += '=';
  Confidence: band=high; score=0.74
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
- Line 455: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: scope_str += ' ';
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.emplace_back("scope", scope_str);
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> granted;
  Confidence: band=medium; score=0.66

### src/auth/webauthn_authenticator.cpp
Total findings: 7

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
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.push_back({{"type", "public-key"}, {"alg", alg_id}});
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: excl.push_back({{"type", "public-key"}, {"id", cid}});
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allow.push_back({{"type", "public-key"}, {"id", cid}});
  Confidence: band=high; score=0.74

### src/auth/oauth_device_flow.cpp
Total findings: 5

- Line 240: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: // High-level authenticate()
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: JWTClaims OAuthDeviceFlow::authenticate(std::function<void(const DeviceCodeResponse &)> progress_cb) {
  Confidence: band=very_high; score=0.99
- Line 76: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: scope_str += ' ';
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.emplace_back("scope", scope_str);
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += '&';
  Confidence: band=high; score=0.74

### src/auth/jwt_key_rotation_manager.cpp
Total findings: 4

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

### src/auth/jwt_validator.cpp
Total findings: 4

- Line 334: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY directly instead of deprecated RSA_new()
  Confidence: band=high; score=0.8
- Line 342: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: #pragma warning(disable : 4996) // OpenSSL deprecated APIs
  Confidence: band=high; score=0.8
- Line 781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.audience.push_back(v.get<std::string>());
  Confidence: band=high; score=0.74
- Line 902: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: revoked_kids_runtime_.push_back(kid);
  Confidence: band=high; score=0.74

### src/auth/mtls_authenticator.cpp
Total findings: 4

- Line 170: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: MTLSClaims MTLSAuthenticator::authenticate(const std::string &cert_pem) {
  Confidence: band=very_high; score=0.99
- Line 306: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return authenticate(pem);
  Confidence: band=very_high; score=0.99
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(reinterpret_cast<const char *>(data), static_cast<size_t>(len));
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(oss.str());
  Confidence: band=high; score=0.74

### src/auth/saml_authenticator.cpp
Total findings: 4

- Line 222: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: url += "&RelayState=" + urlEncode(relay_state);
  Confidence: band=high; score=0.74
- Line 1178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.audience.push_back(nodeText(child));
  Confidence: band=high; score=0.74
- Line 1240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.raw_attributes.emplace_back(attr_name, val);
  Confidence: band=high; score=0.74
- Line 1240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.raw_attributes.emplace_back(attr_name, val);
  Confidence: band=high; score=0.74

### src/auth/session_manager.cpp
Total findings: 4

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

### src/auth/mfa_authenticator.cpp
Total findings: 3

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

### src/auth/oauth_pkce_flow.cpp
Total findings: 3

- Line 121: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: scope_str += ' ';
  Confidence: band=high; score=0.74
- Line 374: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += (i + 1 < len) ? kTable[(triple >> 6) & 0x3F] : '=';
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += '&';
  Confidence: band=high; score=0.74

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

### src/auth/api_key_authenticator.cpp
Total findings: 2

- Line 76: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: ApiKeyClaims ApiKeyAuthenticator::authenticate(const std::string& key_id,
  Confidence: band=very_high; score=0.99
- Line 205: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return authenticate(combined.substr(0, dot), combined.substr(dot + 1));
  Confidence: band=very_high; score=0.99

### src/auth/auth_error.cpp
Total findings: 2

- Line 206: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: auth
  Context: ss << "auth-";
  Confidence: band=very_high; score=0.92
- Line 106: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (; it != end; ++it) {
  Confidence: band=very_high; score=0.9

### src/auth/gssapi_authenticator.cpp
Total findings: 2

- Line 198: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: roles_str += ", ";
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: roles.push_back(mapping.role);
  Confidence: band=high; score=0.74

### src/auth/ldap_connection_pool.cpp
Total findings: 2

- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idle_.push_back(ld);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void LDAPConnectionPool::destroyHandle(LDAP *handle) noexcept {
  Confidence: band=high; score=0.74

### src/auth/rocksdb_token_blacklist.cpp
Total findings: 2

- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descs.emplace_back(cf, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: other_cf_handles_.push_back(cf_handles[i]);
  Confidence: band=high; score=0.74

### src/auth/totp_secret_encryption.cpp
Total findings: 2

- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: secrets.push_back(new_version);
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active_secrets.push_back(secret);
  Confidence: band=high; score=0.74

### src/auth/zero_trust_auth_verifier.cpp
Total findings: 2

- Line 223: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (;;) {
  Confidence: band=very_high; score=0.9
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_eval.push_back(entry);
  Confidence: band=high; score=0.74

### src/auth/jwks_validator.cpp
Total findings: 1

- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("Duplicate key ID: " + kid);
  Confidence: band=high; score=0.74

### src/auth/rate_limiter_backend.cpp
Total findings: 1

- Line 193: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Each sorted-set member must be unique; use timestamp + per-node counter.
  Confidence: band=high; score=0.74

### src/auth/totp_replay_cache.cpp
Total findings: 1

- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user_cache.push_back({code, now});
  Confidence: band=high; score=0.74

### src/auth/auth_audit_logger.cpp
Total findings: 0


### src/auth/auth_metrics.cpp
Total findings: 0


### src/auth/auth_rate_limiter.cpp
Total findings: 0


### src/auth/jwks_security.cpp
Total findings: 0


### src/auth/kerberos_security.cpp
Total findings: 0


### src/auth/oidc_provider.cpp
Total findings: 0


### src/auth/redis_token_blacklist.cpp
Total findings: 0


### src/auth/token_blacklist.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
