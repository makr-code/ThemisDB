# security Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: security
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 124
- Actionable Findings (Critical + High): 21
- Affected Files: 43

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 10 |
| High | 11 |
| Medium | 103 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 131 |
| exception_safety | 121 |
| raii | 111 |
| performance_patterns | 94 |
| container | 86 |
| memory | 37 |
| performance | 20 |
| platform | 17 |
| audit_logging | 15 |
| concurrency | 14 |
| uninitialized | 14 |
| legacy_duplication | 13 |
| determinism | 9 |
| security | 9 |
| observability | 2 |
| deprecated_apis | 1 |
| input_validation | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/security/rbac.cpp | 18 | 0 | 0 | 18 | 0 |
| src/security/field_encryption.cpp | 7 | 0 | 0 | 7 | 0 |
| src/security/row_level_security.cpp | 7 | 0 | 0 | 7 | 0 |
| src/security/security_evidence_collector.cpp | 7 | 0 | 0 | 7 | 0 |
| src/security/vault_key_provider.cpp | 7 | 0 | 1 | 6 | 0 |
| src/security/access_control.cpp | 6 | 4 | 0 | 2 | 0 |
| src/security/access_control_manager.cpp | 6 | 5 | 0 | 1 | 0 |
| src/security/malware_scanner.cpp | 6 | 0 | 0 | 6 | 0 |
| src/security/arrow_user_registration_plugin.cpp | 5 | 0 | 0 | 5 | 0 |
| src/security/embedded_user_registration_plugin.cpp | 5 | 0 | 1 | 4 | 0 |
| src/security/post_quantum_crypto.cpp | 5 | 0 | 1 | 4 | 0 |
| src/security/aql_injection_detector.cpp | 4 | 0 | 1 | 3 | 0 |
| src/security/hsm_key_provider_adapter.cpp | 4 | 0 | 2 | 2 | 0 |
| src/security/input_validator.cpp | 4 | 0 | 0 | 4 | 0 |
| src/security/pki_key_provider.cpp | 4 | 0 | 1 | 3 | 0 |
| src/security/query_masking_policy.cpp | 4 | 0 | 0 | 4 | 0 |
| src/security/mock_key_provider.cpp | 3 | 0 | 1 | 2 | 0 |
| src/security/timestamp_authority.cpp | 3 | 0 | 1 | 2 | 0 |
| src/security/vault_signing_provider.cpp | 3 | 0 | 0 | 3 | 0 |
| src/security/vcc_pki_client.cpp | 3 | 0 | 1 | 2 | 0 |
| src/security/binary_manifest.cpp | 2 | 0 | 0 | 2 | 0 |
| src/security/secret_manager.cpp | 2 | 0 | 0 | 2 | 0 |
| src/security/timestamp_authority_openssl.cpp | 2 | 1 | 0 | 1 | 0 |
| src/security/ai_operation_guard.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/intent_classifier.cpp | 1 | 0 | 1 | 0 | 0 |
| src/security/manifest_signer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/usb_admin_authenticator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/user_registration_plugin.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/webdav_user_registration_plugin.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/zero_trust_policy_enforcer.cpp | 1 | 0 | 0 | 1 | 0 |
| include/security/examples/intent_classifier_example.cpp | 0 | 0 | 0 | 0 | 0 |
| include/security/input_validator.hpp | 0 | 0 | 0 | 0 | 0 |
| src/security/ai_snapshot_cleanup.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/cms_signing.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/confidential_computing.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/encrypted_field.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/fips_crypto_mode.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/hsm_provider.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/hsm_provider_pkcs11.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/keyprovider_signing.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/prompt_injection_pattern_registry.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/usb_volume_hardening.cpp | 0 | 0 | 0 | 0 | 0 |
| src/security/vram_secure_clear.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/security/rbac.cpp
Total findings: 18

- Line 37: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: perms_arr.push_back({
  Confidence: band=high; score=0.74
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.permissions.push_back(perm);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.inherits.push_back(inherit.get<std::string>());
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Role> builtin_backup;
  Confidence: band=medium; score=0.66
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["roles"].push_back(role.toJson());
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 365: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 383: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>& visited
  Confidence: band=medium; score=0.66
- Line 419: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visiting;
  Confidence: band=medium; score=0.66
- Line 420: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hierarchy[role_name]["direct_permissions"].push_back(perm.toString());
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hierarchy[role_name]["direct_permissions"].push_back(perm.toString());
  Confidence: band=high; score=0.74
- Line 499: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: u.roles.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["attributes"].begin(); it != j["attributes"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 523: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user.roles.push_back(role);
  Confidence: band=high; score=0.74
- Line 555: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: users.push_back(user_id);
  Confidence: band=high; score=0.74
- Line 598: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["users"].push_back(user.toJson());
  Confidence: band=high; score=0.74

### src/security/field_encryption.cpp
Total findings: 7

- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: ret += '=';
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: ret += '=';
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: ret += '=';
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[i]);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[i]);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[j]);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[j]);
  Confidence: band=high; score=0.74

### src/security/row_level_security.cpp
Total findings: 7

- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.applicable_roles.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(p.toJson());
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(p.toJson());
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(&policy);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: permissive_policies.push_back(p);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(row);
  Confidence: band=high; score=0.74

### src/security/security_evidence_collector.cpp
Total findings: 7

- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : key_rotations) rotations.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Fill with a monotonically-increasing timestamp-based value as last resort
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.entries.push_back(e.record);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<KeyMetadata>> by_id;
  Confidence: band=medium; score=0.66
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: by_id[meta.key_id].push_back(meta);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.empty_roles.push_back(name);
  Confidence: band=high; score=0.74
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evidence.config_audit_trail.push_back(entry.record);
  Confidence: band=high; score=0.74

### src/security/vault_key_provider.cpp
Total findings: 7

- Line 598: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check if key is deprecated first (safety check)
  Confidence: band=high; score=0.8
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(base64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(key.get<std::string>());
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74

### src/security/access_control.cpp
Total findings: 6

- Line 114: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: AccessControl::AuthenticationResult AccessControl::authenticate(const Credentials& credentials) {
  Confidence: band=very_high; score=0.99
- Line 461: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: bool AccessControl::authorize(const AuthorizationContext& context) {
  Confidence: band=very_high; score=0.99
- Line 490: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto abac_decision = policy_engine_.authorize(
  Confidence: band=very_high; score=0.99
- Line 557: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: return authorize(context);
  Confidence: band=very_high; score=0.99
- Line 907: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.record);
  Confidence: band=high; score=0.74
- Line 975: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired_sessions.push_back(token);
  Confidence: band=high; score=0.74

### src/security/access_control_manager.cpp
Total findings: 6

- Line 109: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: std::optional<SecurityContext> AccessControlManager::authenticate(
  Confidence: band=very_high; score=0.99
- Line 155: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: AccessDecision AccessControlManager::authorize(
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto abac_decision = policy_engine_.authorize(
  Confidence: band=very_high; score=0.99
- Line 247: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto context = authenticate(token, source_ip);
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: return authorize(*context, resource, action);
  Confidence: band=very_high; score=0.99
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decision.applied_permissions.push_back(perm.toString());
  Confidence: band=high; score=0.74

### src/security/malware_scanner.cpp
Total findings: 6

- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c.skip_mime_types.push_back(m.get<std::string>());
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c.enabled_scanners.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.scanner_results.push_back(unavail);
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.push_back(s);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.pattern.push_back(byte);
  Confidence: band=high; score=0.74

### src/security/arrow_user_registration_plugin.cpp
Total findings: 5

- Line 42: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: [[maybe_unused]] const std::unordered_map<std::string, std::string>& attributes)
  Confidence: band=medium; score=0.66
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back("readonly");
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: users.push_back(data);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back(role);
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back(role);
  Confidence: band=high; score=0.74

### src/security/embedded_user_registration_plugin.cpp
Total findings: 5

- Line 431: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // We always produced m=19456,t=2,p=1 but parse them for forward compat.
  Confidence: band=high; score=0.8
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(reg_data);
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: it->second.password_history.push_back(new_hash);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(s.substr(start, end == std::string::npos ? end : end - start));
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(s.substr(start, end == std::string::npos ? end : end - start));
  Confidence: band=high; score=0.74

### src/security/post_quantum_crypto.cpp
Total findings: 5

- Line 781: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: ca4[k] = static_cast<uint8_t>(B64_CHARS.find(ca4[k]));
  Confidence: band=very_high; score=0.9
- Line 773: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int k = 0; k < i - 1; ++k) ret.push_back(ca3[k]);
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int k = 0; k < i - 1; ++k) ret.push_back(ca3[k]);
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int k = 0; k < i - 1; ++k) ret.push_back(ca3[k]);
  Confidence: band=high; score=0.74

### src/security/aql_injection_detector.cpp
Total findings: 4

- Line 331: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& pattern : patterns) {
  Confidence: band=very_high; score=0.9
- Line 24: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
  Confidence: band=medium; score=0.56
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.detected_patterns.push_back(literal);
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns.push_back(match.str());
  Confidence: band=high; score=0.74

### src/security/hsm_key_provider_adapter.cpp
Total findings: 4

- Line 139: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark old version as DEPRECATED
  Confidence: band=high; score=0.8
- Line 284: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark other active versions as deprecated
  Confidence: band=high; score=0.8
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(data.metadata);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(data.metadata);
  Confidence: band=high; score=0.74

### src/security/input_validator.cpp
Total findings: 4

- Line 336: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '<':  output += "&lt;"; break;
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: output += '\\';
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: output += "'\\''";  // End quote, escaped quote, start quote
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  output += "\\\""; break;
  Confidence: band=high; score=0.74

### src/security/pki_key_provider.cpp
Total findings: 4

- Line 631: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark old version as deprecated in metadata
  Confidence: band=high; score=0.8
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hex.push_back(hex_chars[(b >> 4) & 0xF]);
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(meta);
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(group_name);
  Confidence: band=high; score=0.74

### src/security/query_masking_policy.cpp
Total findings: 4

- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: masked.push_back(maskNode(item, "", snapshot));
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: masked.push_back(maskNode(item, "", snapshot));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(elem, key, snapshot));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(elem, key, snapshot));
  Confidence: band=high; score=0.74

### src/security/mock_key_provider.cpp
Total findings: 3

- Line 136: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark old ACTIVE keys as DEPRECATED
  Confidence: band=high; score=0.8
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.metadata);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.metadata);
  Confidence: band=high; score=0.74

### src/security/timestamp_authority.cpp
Total findings: 3

- Line 87: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //          pimpl compatibility while still tracking minimal runtime state
  Confidence: band=high; score=0.8
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validation_errors_.push_back("DER decode of token failed (invalid PKCS7)");
  Confidence: band=high; score=0.74
- Line 1137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validation_errors_.push_back(
  Confidence: band=high; score=0.74

### src/security/vault_signing_provider.cpp
Total findings: 3

- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(b64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74

### src/security/vcc_pki_client.cpp
Total findings: 3

- Line 171: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
  Confidence: band=very_high; score=0.9
- Line 171: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: crl.push_back(CRLEntry::fromJson(entry_json));
  Confidence: band=high; score=0.74

### src/security/binary_manifest.cpp
Total findings: 2

- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["files"].push_back(file.to_json());
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.files_.push_back(BinaryFileEntry::from_json(file_json));
  Confidence: band=high; score=0.74

### src/security/secret_manager.cpp
Total findings: 2

- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.versions.push_back(std::move(ver));
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/security/timestamp_authority_openssl.cpp
Total findings: 2

- Line 115: severity=CRITICAL; category=deprecated_apis; pattern=\bstrdup\s*\(
  Description: Deprecated API: \bstrdup\s*\( → Use std::string instead
  Context: old_tz_copy = strdup(old_tz);
  Confidence: band=very_high; score=0.99
- Line 617: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validation_errors_.push_back("TSA not found in qualified trust service providers list");
  Confidence: band=high; score=0.74

### src/security/ai_operation_guard.cpp
Total findings: 1

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::toupper(c)));
  Confidence: band=high; score=0.74

### src/security/intent_classifier.cpp
Total findings: 1

- Line 92: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // for AI Safety callers; standard callers keep 0.85 for backwards compat).
  Confidence: band=high; score=0.8

### src/security/manifest_signer.cpp
Total findings: 1

- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missing_files.push_back(file_entry.path);
  Confidence: band=high; score=0.74

### src/security/usb_admin_authenticator.cpp
Total findings: 1

- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: license.admin_scopes.push_back(scope.get<std::string>());
  Confidence: band=high; score=0.74

### src/security/user_registration_plugin.cpp
Total findings: 1

- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(plugin);
  Confidence: band=high; score=0.74

### src/security/webdav_user_registration_plugin.cpp
Total findings: 1

- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back("readonly");
  Confidence: band=high; score=0.74

### src/security/zero_trust_policy_enforcer.cpp
Total findings: 1

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74

### include/security/examples/intent_classifier_example.cpp
Total findings: 0


### include/security/input_validator.hpp
Total findings: 0


### src/security/ai_snapshot_cleanup.cpp
Total findings: 0


### src/security/cms_signing.cpp
Total findings: 0


### src/security/confidential_computing.cpp
Total findings: 0


### src/security/encrypted_field.cpp
Total findings: 0


### src/security/fips_crypto_mode.cpp
Total findings: 0


### src/security/hsm_provider.cpp
Total findings: 0


### src/security/hsm_provider_pkcs11.cpp
Total findings: 0


### src/security/keyprovider_signing.cpp
Total findings: 0


### src/security/prompt_injection_pattern_registry.cpp
Total findings: 0


### src/security/usb_volume_hardening.cpp
Total findings: 0


### src/security/vram_secure_clear.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
