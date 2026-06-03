# security Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: security
- Generated: 2026-06-02 12:40:51
- Status: Critical Findings Present
- Total Findings: 686
- Actionable Findings (Critical + High): 335
- Affected Files: 43

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 48 |
| High | 287 |
| Medium | 351 |
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
| src/security/post_quantum_crypto.cpp | 51 | 0 | 23 | 28 | 0 |
| src/security/timestamp_authority.cpp | 46 | 0 | 14 | 32 | 0 |
| src/security/field_encryption.cpp | 45 | 9 | 16 | 20 | 0 |
| src/security/vault_key_provider.cpp | 44 | 1 | 25 | 18 | 0 |
| src/security/timestamp_authority_openssl.cpp | 43 | 5 | 16 | 22 | 0 |
| src/security/hsm_key_provider_adapter.cpp | 41 | 0 | 34 | 7 | 0 |
| src/security/pki_key_provider.cpp | 39 | 1 | 13 | 25 | 0 |
| src/security/malware_scanner.cpp | 37 | 4 | 17 | 16 | 0 |
| src/security/hsm_provider_pkcs11.cpp | 32 | 4 | 19 | 9 | 0 |
| src/security/rbac.cpp | 31 | 0 | 5 | 26 | 0 |
| src/security/input_validator.cpp | 27 | 0 | 5 | 22 | 0 |
| src/security/security_evidence_collector.cpp | 21 | 3 | 6 | 12 | 0 |
| src/security/embedded_user_registration_plugin.cpp | 19 | 0 | 11 | 8 | 0 |
| src/security/hsm_provider.cpp | 17 | 0 | 10 | 7 | 0 |
| src/security/aql_injection_detector.cpp | 16 | 0 | 8 | 8 | 0 |
| src/security/confidential_computing.cpp | 15 | 5 | 3 | 7 | 0 |
| src/security/mock_key_provider.cpp | 15 | 0 | 13 | 2 | 0 |
| src/security/vcc_pki_client.cpp | 15 | 2 | 5 | 8 | 0 |
| src/security/manifest_signer.cpp | 13 | 2 | 8 | 3 | 0 |
| src/security/row_level_security.cpp | 13 | 0 | 2 | 11 | 0 |
| src/security/access_control.cpp | 10 | 4 | 3 | 3 | 0 |
| src/security/usb_admin_authenticator.cpp | 9 | 0 | 4 | 5 | 0 |
| src/security/arrow_user_registration_plugin.cpp | 8 | 0 | 1 | 7 | 0 |
| src/security/secret_manager.cpp | 8 | 0 | 3 | 5 | 0 |
| src/security/access_control_manager.cpp | 7 | 5 | 1 | 1 | 0 |
| src/security/vault_signing_provider.cpp | 7 | 0 | 0 | 7 | 0 |
| src/security/webdav_user_registration_plugin.cpp | 7 | 0 | 3 | 4 | 0 |
| src/security/query_masking_policy.cpp | 6 | 0 | 0 | 6 | 0 |
| src/security/cms_signing.cpp | 5 | 0 | 3 | 2 | 0 |
| src/security/fips_crypto_mode.cpp | 5 | 3 | 2 | 0 | 0 |
| src/security/keyprovider_signing.cpp | 5 | 0 | 3 | 2 | 0 |
| src/security/usb_volume_hardening.cpp | 5 | 0 | 1 | 4 | 0 |
| src/security/binary_manifest.cpp | 4 | 0 | 0 | 4 | 0 |
| src/security/vram_secure_clear.cpp | 4 | 0 | 4 | 0 | 0 |
| src/security/ai_snapshot_cleanup.cpp | 3 | 0 | 2 | 1 | 0 |
| src/security/zero_trust_policy_enforcer.cpp | 3 | 0 | 0 | 3 | 0 |
| include/security/input_validator.hpp | 2 | 0 | 1 | 1 | 0 |
| src/security/encrypted_field.cpp | 2 | 0 | 2 | 0 | 0 |
| src/security/intent_classifier.cpp | 2 | 0 | 1 | 1 | 0 |
| include/security/examples/intent_classifier_example.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/ai_operation_guard.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/prompt_injection_pattern_registry.cpp | 1 | 0 | 0 | 1 | 0 |
| src/security/user_registration_plugin.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/security/post_quantum_crypto.cpp
Total findings: 51

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
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 155: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("aes256gcm_encrypt: ") + where + ": " + ossl_error());
- Line 198: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("aes256gcm_decrypt: ") + where + ": " + ossl_error());
- Line 578: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return classical_provider_->getKeyMetadata(key_id, version);
- Line 592: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return classical_provider_->createKeyFromBytes(key_id, key_bytes, metadata);
- Line 781: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: ca4[k] = static_cast<uint8_t>(B64_CHARS.find(ca4[k]));
  Confidence: band=very_high; score=0.9
- Line 894: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("HybridEncryption::decryptHybrid: invalid IV/tag size");
- Line 168: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 211: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 238: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(kctx);
- Line 244: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 247: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 284: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(priv_key); EVP_PKEY_free(pub_key); EVP_PKEY_CTX_free(ctx);
- Line 291: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(priv_key); EVP_PKEY_free(pub_key); EVP_PKEY_CTX_free(ctx);
- Line 294: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(priv_key); EVP_PKEY_free(pub_key); EVP_PKEY_CTX_free(ctx);
- Line 315: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(kctx);
- Line 321: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 324: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 350: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mctx); EVP_PKEY_free(pkey);
- Line 354: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mctx);
- Line 355: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 773: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);
  Confidence: band=high; score=0.74
- Line 774: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);
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
- Line 784: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int k = 0; k < i - 1; ++k) ret.push_back(ca3[k]);
- Line 999: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_CTX_free(pctx);
- Line 1007: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 1012: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 1028: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1055: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx); EVP_PKEY_free(pkey);
- Line 1059: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 1060: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 1075: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/security/timestamp_authority.cpp
Total findings: 46

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
- Line 87: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: //          pimpl compatibility while still tracking minimal runtime state
  Confidence: band=high; score=0.8
- Line 590: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     request.data());
- Line 591: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));
- Line 158: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 211: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 219: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: TimestampToken tok; tok.success = true; tok.token_der = token_data; tok.token_b64 = std::string("hex
- Line 283: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Token marked as unsuccessful");
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Token has no timestamp");
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Token timestamp is in the future");
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Maximum age must be non-negative");
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Maximum age value too large");
- Line 372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back(
- Line 561: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_REQ_free(req);
- Line 568: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_REQ_free(req);
- Line 674: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_RESP_free(resp);
- Line 716: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (hex_serial) { tok.serial_number = hex_serial; OPENSSL_free(hex_serial); }
- Line 717: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
- Line 768: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
- Line 798: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_TST_INFO_free(tst_info);
- Line 810: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (name) { tok.tsa_name = name; OPENSSL_free(name); }
- Line 818: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (s) { tok.tsa_serial = s; OPENSSL_free(s); }
- Line 819: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
- Line 838: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_RESP_free(resp);
- Line 925: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_TST_INFO_free(tst_info);
- Line 928: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: PKCS7_free(pkcs7);
- Line 1023: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(ca);
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validation_errors_.push_back("DER decode of token failed (invalid PKCS7)");
  Confidence: band=high; score=0.74
- Line 1035: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("DER decode of token failed (invalid PKCS7)");
- Line 1057: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back(std::string("OpenSSL: ") + err_buf);
- Line 1062: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: PKCS7_free(pkcs7);
- Line 1073: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Token has no timestamp");
- Line 1137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validation_errors_.push_back(
  Confidence: band=high; score=0.74
- Line 1138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back(

### src/security/field_encryption.cpp
Total findings: 45

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
  <!-- FALSE_POSITIVE: uncategorized line-0 finding with no actionable location -->
- Line 90: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 4 > array 3
  Remediation: Fix loop condition or increase array size
  Context: uint8_t char_array_3[3];
  <!-- STALE: base64 code replaced by EVP_EncodeBlock/DecodeBlock; char_array_3 no longer exists -->
- Line 98: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 4 > array 0
  Remediation: Fix loop condition or increase array size
  Context: char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
  <!-- STALE: old base64 code removed -->
- Line 99: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 4 > array 1
  Remediation: Fix loop condition or increase array size
  Context: char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
  <!-- STALE: old base64 code removed -->
- Line 100: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 4 > array 2
  Remediation: Fix loop condition or increase array size
  Context: char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
  <!-- STALE: old base64 code removed -->
- Line 101: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 4 > array 3
  Remediation: Fix loop condition or increase array size
  Context: char_array_4[3] = char_array_3[2] & 0x3f;
  <!-- STALE: old base64 code removed -->
- Line 145: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 3 > array 0
  Remediation: Fix loop condition or increase array size
  Context: char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
  <!-- STALE: old base64 code removed -->
- Line 146: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 3 > array 1
  Remediation: Fix loop condition or increase array size
  Context: char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
  <!-- STALE: old base64 code removed -->
- Line 147: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 3 > array 2
  Remediation: Fix loop condition or increase array size
  Context: char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
  <!-- STALE: old base64 code removed -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: uncategorized line-0 finding with no actionable location -->
- Line 90: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t char_array_3[3];
  <!-- STALE: old base64 code replaced by EVP_EncodeBlock/DecodeBlock -->
- Line 91: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t char_array_4[4];
  <!-- STALE: old base64 code replaced by EVP_EncodeBlock/DecodeBlock -->
- Line 545: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to generate random IV");
  <!-- FALSE_POSITIVE: intentional propagation — callers catch EncryptionException -->
- Line 556: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Key must be 32 bytes (256 bits)");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 567: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to create cipher context");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 590: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Encryption failed");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 596: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to finalize encryption");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 604: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to get authentication tag");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 624: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Key must be 32 bytes (256 bits)");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 628: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("IV must be 12 bytes");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 632: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Tag must be 16 bytes");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 638: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to create cipher context");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 661: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Decryption failed");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 667: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to set authentication tag");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 678: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Authentication failed - data may have been tampered with");
  <!-- FALSE_POSITIVE: intentional propagation -->
- Line 74: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- STALE: file uses catch(const std::exception&) throughout; line 74 is non-code context -->
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: ret += '=';
  Confidence: band=high; score=0.74
  <!-- STALE: old base64 helper removed; toBase64() now uses ostringstream -->
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: ret += '=';
  Confidence: band=high; score=0.74
  <!-- STALE: duplicate of above -->
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: ret += '=';
  Confidence: band=high; score=0.74
  <!-- STALE: duplicate of above -->
- Line 121: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: ret += '=';
  <!-- STALE: old base64 helper removed -->
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[i]);
  Confidence: band=high; score=0.74
  <!-- STALE: old base64 code removed -->
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[i]);
  Confidence: band=high; score=0.74
  <!-- STALE: duplicate -->
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ret.push_back(char_array_3[i]);
  <!-- STALE: old base64 code removed -->
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[j]);
  Confidence: band=high; score=0.74
  <!-- STALE: old base64 code removed -->
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(char_array_3[j]);
  Confidence: band=high; score=0.74
  <!-- STALE: duplicate -->
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ret.push_back(char_array_3[j]);
  <!-- STALE: old base64 code removed -->
- Line 277: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- STALE: file uses catch(const std::exception& ex) at this location -->
- Line 286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- STALE: specific catch in current source -->
- Line 302: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- STALE: specific catch in current source -->
- Line 311: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- STALE: specific catch in current source -->
- Line 478: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- FALSE_POSITIVE: catch(...) { rethrow; } pattern for metric counting — intentional -->
- Line 521: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- FALSE_POSITIVE: catch-rethrow for metrics — intentional -->
- Line 609: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- FALSE_POSITIVE: catch-rethrow for metrics — intentional -->
- Line 685: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
  <!-- STALE: current code uses EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new()) RAII -->
- Line 689: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- STALE: file uses catch(const std::exception&) at this location -->

### src/security/vault_key_provider.cpp
Total findings: 44

- Line 441: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
  <!-- FIXED: unique_lock now uses defer_lock + try_lock_for(5s) on initial acquire -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("curl_easy_duphandle failed", -1, std::string(), true);
- Line 185: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException(std::string("CURL error: ") + curl_easy_strerror(res), -1, std::string()
- Line 189: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException("key", 0);
- Line 191: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Vault authentication failed (403 Forbidden)", (int)http_code, response,
- Line 193: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Vault server error (HTTP " + std::to_string(http_code) + ")", (int)http
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Vault request failed (HTTP " + std::to_string(http_code) + "): " + resp
- Line 351: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Invalid Vault response format (missing data.data)");
- Line 356: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Invalid Vault response format (missing data)");
- Line 363: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Vault returned an empty key - refusing to use zero-length key material"
- Line 375: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Invalid Vault metadata response");
- Line 511: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["input"] = base64_encode(data);
- Line 551: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Vault transit sign returned unexpected payload", -1, response, false);
- Line 558: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 569: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 595: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Key deletion only supported in KV v2");
- Line 598: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check if key is deprecated first (safety check)
  Confidence: band=high; score=0.8
- Line 602: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: ACTIVE = nullptr;
  Context: throw KeyOperationException("Cannot delete ACTIVE key. Deprecate it first via rotation.");
- Line 623: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("curl_easy_duphandle failed during deleteKey");
- Line 641: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: key = nullptr;
  Context: throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
- Line 641: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
- Line 714: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("curl_easy_duphandle failed during createKey");
- Line 734: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException(std::string("Failed to create key: ") + curl_easy_strerror(res));
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
  <!-- FIXED: base64_decode result.reserve(encoded.size() * 3 / 4) added -->
- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
  <!-- FIXED: base64_decode result.reserve(encoded.size() * 3 / 4) added -->
- Line 52: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(char((val >> valb) & 0xFF));
  <!-- FIXED: base64_decode result.reserve added -->
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(base64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
  <!-- FIXED: base64_encode result.reserve(((data.size() + 2) / 3) * 4) added -->
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(base64_chars[(val >> valb) & 0x3F]);
  <!-- FIXED: base64_encode result.reserve added -->
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (valb > -6) result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  <!-- FIXED: base64_encode result.reserve added -->
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (result.size() % 4) result.push_back('=');
  <!-- FIXED: base64_encode result.reserve added -->
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(key.get<std::string>());
  Confidence: band=high; score=0.74
  <!-- FIXED: keys.reserve(j["data"]["keys"].size()) added in listSecrets() -->
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(key.get<std::string>());
  <!-- FIXED: keys.reserve added in listSecrets() -->
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
  <!-- FIXED: result.reserve(key_ids.size()) added in listKeys() -->
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
  <!-- FIXED: result.reserve(key_ids.size()) added in listKeys() -->
- Line 495: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- FIXED: catch(const std::exception&) in listKeys() -->
- Line 641: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
- Line 743: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
  <!-- FIXED: catch(const std::exception&) in createKey() version parse -->

### src/security/timestamp_authority_openssl.cpp
Total findings: 43

- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
  <!-- FALSE_POSITIVE: no line number; uncategorized pointer findings without location are scanner artifacts -->
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
  <!-- FALSE_POSITIVE: duplicate artifact -->
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
  <!-- FALSE_POSITIVE: duplicate artifact -->
- Line 0: severity=CRITICAL; category=uncategorized
  Context: Pointer declared but not initialized
  Confidence: band=very_high; score=0.93
  <!-- FALSE_POSITIVE: duplicate artifact -->
- Line 115: severity=CRITICAL; category=deprecated_apis; pattern=\bstrdup\s*\(
  Description: Deprecated API: \bstrdup\s*\( → Use std::string instead
  Context: old_tz_copy = strdup(old_tz);
  Confidence: band=very_high; score=0.99
  <!-- STALE: line 115 in current source is start of b64Encode(); strdup removed -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
  <!-- FALSE_POSITIVE: line-0; all BIO ptr dereferences guarded by null check (ptr && ptr->data) -->
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
  <!-- FALSE_POSITIVE: duplicate -->
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
  <!-- FALSE_POSITIVE: duplicate -->
- Line 0: severity=HIGH; category=uncategorized
  Context: Pointer dereference without null check
  Confidence: band=high; score=0.81
  <!-- FALSE_POSITIVE: duplicate -->
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5421 fix: thread-safety for Prov... (2026-06-01) | #3457 [TSA] Implement RFC
  <!-- FALSE_POSITIVE: scanner matched PR History comment in file header, not code -->
- Line 77: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string out(ptr->data, ptr->length);
  <!-- FALSE_POSITIVE: current code guards with if(ptr && ptr->data && ptr->length > 0) before deref -->
- Line 321: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if(policy){ char buf[128]; OBJ_obj2txt(buf,sizeof(buf),policy,1); token.policy_oid=buf; }
  <!-- FALSE_POSITIVE: buf[128] is a fixed-size text buffer for OBJ_obj2txt; sizeof(buf) used as limit -->
- Line 131: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: free(old_tz_copy);
  <!-- STALE: strdup/free pattern no longer present in current source -->
- Line 283: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(cert_der);
  <!-- STALE: current source uses OPENSSL_free(der) immediately after copy into vector; no exception path -->
- Line 292: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(hexStr);
  <!-- STALE: OPENSSL_free(hexStr) in BIGNUM→hex block; BN freed via BIGNUM_ptr; no throw path between alloc and free -->
- Line 293: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
  <!-- STALE: current code uses BIGNUM_ptr RAII for BN -->
- Line 304: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(name_bio);
  <!-- STALE: current code uses BIO_ptr RAII -->
- Line 308: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if(certs) sk_X509_free(certs);
  <!-- STALE: current code uses STACK_OF_X509_ptr RAII -->
- Line 319: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if(serial){ BIGNUM* bn = ASN1_INTEGER_to_BN(serial,nullptr); char* hexStr = BN_bn2hex(bn); token.ser
  <!-- STALE: current code uses BIGNUM_ptr; OPENSSL_free(hexStr) in if-block -->
- Line 339: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_TST_INFO_free(tst);
  <!-- STALE: current code uses TS_TST_INFO_ptr RAII -->
- Line 342: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_RESP_free(resp);
  <!-- STALE: current code uses TS_RESP_ptr RAII -->
- Line 410: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
  <!-- STALE: current code uses BIO_ptr RAII -->
- Line 411: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
  <!-- STALE: current code uses X509_ptr RAII -->
- Line 507: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: TS_TST_INFO_free(tst);
  <!-- STALE: current code uses TS_TST_INFO_ptr RAII -->
- Line 508: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: PKCS7_free(pkcs7);
  <!-- STALE: current code uses PKCS7_ptr RAII -->
- Line 529: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Token timestamp is in the future");
  <!-- FALSE_POSITIVE: single conditional push_back, not a loop -->
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Maximum age must be non-negative");
  <!-- FALSE_POSITIVE: single conditional push_back -->
- Line 545: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Maximum age value too large");
  <!-- FALSE_POSITIVE: single conditional push_back -->
- Line 573: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Failed to create BIO for certificate");
  <!-- FALSE_POSITIVE: single conditional push_back -->
- Line 581: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Failed to parse TSA certificate");
  <!-- FALSE_POSITIVE: single conditional push_back -->
- Line 588: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Certificate has no subject");
  <!-- FALSE_POSITIVE: single conditional push_back -->
- Line 596: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("Failed to create BIO for subject name");
  <!-- FALSE_POSITIVE: single conditional push_back -->
- Line 617: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validation_errors_.push_back("TSA not found in qualified trust service providers list");
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: single conditional push_back inside a loop-less if block -->
- Line 618: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validation_errors_.push_back("TSA not found in qualified trust service providers list");
  <!-- FALSE_POSITIVE: duplicate of above -->

### src/security/hsm_key_provider_adapter.cpp
Total findings: 41

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
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 68: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, 0);
- Line 80: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("No active version found for key: " + key_id);
- Line 103: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 108: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 113: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Key is deleted: " + key_id + " v" + std::to_string(version));
- Line 139: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark old version as DEPRECATED
  Confidence: band=high; score=0.8
- Line 158: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: key_store_[key_id][new_version] = new_data;
- Line 172: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [version, data] : versions) {
- Line 185: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 201: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 217: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 222: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: ACTIVE = nullptr;
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 222: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 284: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark other active versions as deprecated
  Confidence: band=high; score=0.8
- Line 420: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("HSM failed to wrap DEK: " + hsm_->getLastError());
- Line 429: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Failed to wrap DEK with HSM: " + std::string(e.what()));
- Line 481: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("HSM failed to unwrap DEK: " + hsm_->getLastError());
- Line 490: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Failed to unwrap DEK with HSM: " + std::string(e.what()));
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(data.metadata);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(data.metadata);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(data.metadata);
- Line 222: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 375: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 450: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/security/pki_key_provider.cpp
Total findings: 39

- Line 144: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto existing_opt = db_->get(ikm_db_key);
  <!-- FALSE_POSITIVE: deriveKEK() called only from constructor before object is shared; comment at line 159 confirms no-lock is intentional -->
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
- Line 407: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: DEK = nullptr;
  Context: throw std::runtime_error("Cannot delete DEK");
- Line 631: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark old version as deprecated in metadata
  Confidence: band=high; score=0.8
- Line 94: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 113: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 119: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(pubkey_der);
- Line 120: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 121: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hex.push_back(hex_chars[(b >> 4) & 0xF]);
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 233: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 238: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 244: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 248: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 282: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 288: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 296: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 300: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 383: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(meta);
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: throw std::runtime_error("Cannot delete DEK");
- Line 498: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 503: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 509: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 514: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 548: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 553: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 557: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 667: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(group_name);
  Confidence: band=high; score=0.74

### src/security/malware_scanner.cpp
Total findings: 37

- Line 719: severity=CRITICAL; category=socket_leak
  Description: Socket created but never closed — potential resource leak
  Remediation: Wrap socket in RAII class (e.g., std::unique_ptr with custom deleter)
  Context: socket_handle sock = socket(AF_INET, SOCK_STREAM, 0);
  <!-- FALSE_POSITIVE: sendCommand() wraps sock in SocketGuard RAII on same line; auto-closed in all paths -->
- Line 725: severity=CRITICAL; category=missing_dtor
  Description: Class sockaddr_in allocates resources but has no destructor
  Remediation: Add explicit destructor: ~sockaddr_in() { /* cleanup */ }
  Context: class/struct sockaddr_in
  <!-- FALSE_POSITIVE: sockaddr_in is a POSIX plain-old-data struct with no heap resources -->
- Line 734: severity=CRITICAL; category=missing_dtor
  Description: Class sockaddr allocates resources but has no destructor
  Remediation: Add explicit destructor: ~sockaddr() { /* cleanup */ }
  Context: class/struct sockaddr
  <!-- FALSE_POSITIVE: sockaddr is a POSIX plain-old-data struct with no heap resources -->
- Line 762: severity=CRITICAL; category=socket_leak
  Description: Socket created but never closed — potential resource leak
  Remediation: Wrap socket in RAII class (e.g., std::unique_ptr with custom deleter)
  Context: socket_handle sock = socket(AF_INET, SOCK_STREAM, 0);
  <!-- FALSE_POSITIVE: scanInstream() wraps sock in SocketGuard RAII; auto-closed in all paths -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 87: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: closesocket(sock_);
- Line 719: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket_handle sock = socket(AF_INET, SOCK_STREAM, 0);
- Line 734: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
- Line 740: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(sock, cmd.c_str(), static_cast<int>(cmd.size()), 0);
- Line 746: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: while ((bytes_received = recv(sock, buffer, static_cast<int>(sizeof(buffer) - 1), 0)) > 0) {
- Line 762: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: socket_handle sock = socket(AF_INET, SOCK_STREAM, 0);
- Line 777: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
- Line 783: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(sock, instream_cmd, static_cast<int>(strlen(instream_cmd)), 0);
- Line 793: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(sock, reinterpret_cast<char*>(&size_n), 4, 0);
- Line 796: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(sock, data.c_str() + offset, static_cast<int>(to_send), 0);
- Line 802: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: send(sock, reinterpret_cast<char*>(&zero), 4, 0);
- Line 805: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[4096];
- Line 808: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: while ((bytes_received = recv(sock, buffer, static_cast<int>(sizeof(buffer) - 1), 0)) > 0) {
- Line 809: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buffer[bytes_received] = '\0';
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r.toJson());
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c.skip_mime_types.push_back(m.get<std::string>());
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c.skip_mime_types.push_back(m.get<std::string>());
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c.enabled_scanners.push_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c.enabled_scanners.push_back(s.get<std::string>());
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.scanner_results.push_back(unavail);
  Confidence: band=high; score=0.74
- Line 418: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<ScannerStatus> status;
- Line 420: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ScannerStatus s;
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.push_back(s);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.pattern.push_back(byte);
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 792: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Document endianness assumptions or use portable serialization
  Context: uint32_t size_n = htonl(static_cast<uint32_t>(to_send));
- Line 874: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/security/hsm_provider_pkcs11.cpp
Total findings: 32

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    std::vector<uint8_t> di(sizeof(SHA256_DER_PREFIX) + digest.size());', '    std::memcpy(di.data(), SHA256_DER_PREFIX, sizeof(SHA256_DER_PREFIX));', '    std::memcpy(di.data()+sizeof(SHA256_DER_PREFIX), digest.data(), digest.size());', '    return di;', '}']
  Confidence: band=very_high; score=0.9
- Line 539: severity=CRITICAL; category=data_race <!-- FIXED: discoverCertificateSession() now writes cert_serial_cache_ under std::lock_guard<std::mutex> lk(impl_->mtx) with double-check pattern -->
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cert_serial_cache_ = hex;
- Line 661: severity=CRITICAL; category=data_race <!-- FALSE_POSITIVE: sign() already holds impl_->mtx via lock_guard at line 582 when reading cert_serial_cache_ -->
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: r.cert_serial = impl_->cert_serial_cache_.empty()?"REAL-CERT":impl_->cert_serial_cache_;
- Line 1023: severity=CRITICAL; category=data_race <!-- FIXED: discoverCertificateSession() now writes cert_serial_cache_ under impl_->mtx -->
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cert_serial_cache_ = serial_hex;
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    if(data.empty()) return "";', "    // EVP_EncodeBlock adds null terminator and pads with '='", '    size_t outLen = ((data.size() + 2) / 3) * 4;', '    std::vector<unsigned char> encoded(outLen + 1);', '    int len = EVP_EncodeBlock(encoded.data(), data.data(), (int)data.size());']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3461 [HSM] Implement PKCS#11 tok... (2026-03-12) | #3458 [HSM] PKCS#11 C++ w
- Line 113: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static std::vector<uint8_t> pkcs11_stub_aes_encrypt(const std::vector<uint8_t>& key, const std::vect
- Line 142: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static std::vector<uint8_t> pkcs11_stub_aes_decrypt(const std::vector<uint8_t>& key, const std::vect
- Line 402: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->stub_kek.resize(32);
- Line 403: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {
- Line 481: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::vector<uint8_t> di(sizeof(SHA256_DER_PREFIX) + digest.size());
- Line 582: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 626: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto sess = acquireSession();
- Line 669: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 740: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 790: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 839: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 923: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 1031: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 79: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if(lib_) dlclose(lib_);
  <!-- FALSE_POSITIVE: dlclose in Loader destructor body — natural cleanup location, no exception path -->
- Line 469: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
  <!-- STALE: current sha256() uses EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new()) RAII at line 491 -->
- Line 540: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(hex);
  <!-- FALSE_POSITIVE: OPENSSL_free(hex) inside if(hex) block after BN_bn2hex; no throw path between alloc and free -->
- Line 542: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
  <!-- STALE: current code uses BIGNUM_ptr bn(ASN1_INTEGER_to_BN(...)) RAII -->
- Line 545: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(x);
  <!-- STALE: current code uses X509_ptr x(d2i_X509(...)) RAII -->
- Line 983: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(hex);
  <!-- FALSE_POSITIVE: OPENSSL_free(hex) inside if(hex) block after BN_bn2hex; no throw path -->
- Line 985: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
  <!-- STALE: current code uses BIGNUM_ptr bn(ASN1_INTEGER_to_BN(...)) RAII -->
- Line 1011: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(der);
  <!-- FIXED: der now managed by unique_ptr<unsigned char, decltype(&OPENSSL_free)> for full exception safety -->
- Line 1012: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(x509);
  <!-- STALE: current code uses X509_ptr x509(PEM_read_bio_X509(...)) RAII -->

### src/security/rbac.cpp
Total findings: 31

- Line 140: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& role : getBuiltinRoles()) {
- Line 211: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& br : getBuiltinRoles()) {
- Line 241: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["roles"] = nlohmann::json::array();
- Line 555: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(user.roles.begin(), user.roles.end(), role) != user.roles.end()) {
- Line 596: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["users"] = nlohmann::json::array();
- Line 37: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: perms_arr.push_back({
  Confidence: band=high; score=0.74
- Line 38: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: perms_arr.push_back({
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.permissions.push_back(perm);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.inherits.push_back(inherit.get<std::string>());
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.inherits.push_back(inherit.get<std::string>());
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: builtin.push_back({
- Line 191: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 209: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Role> builtin_backup;
  Confidence: band=medium; score=0.66
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["roles"].push_back(role.toJson());
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["roles"].push_back(role.toJson());
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
- Line 475: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hierarchy[role_name]["direct_permissions"].push_back(perm.toString());
- Line 499: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: u.roles.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 500: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: u.roles.push_back(r.get<std::string>());
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
- Line 599: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["users"].push_back(user.toJson());

### src/security/input_validator.cpp
Total findings: 27

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 155: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "Use only [a-zA-Z0-9._-]"};
- Line 289: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "Start identifier with [a-zA-Z_]"};
- Line 297: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "Use only [a-zA-Z0-9_]"};
- Line 398: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "\\u%04X", (unsigned)c);
- Line 83: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Path contains traversal sequences (../, ..\\)",
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Use only safe filenames without ../ or ..\\"};
- Line 336: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '<':  output += "&lt;"; break;
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  output += "&lt;"; break;
- Line 338: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  output += "&gt;"; break;
- Line 339: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  output += "&amp;"; break;
- Line 340: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  output += "&quot;"; break;
- Line 341: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': output += "&#39;"; break;
- Line 355: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: output += '\\';
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: output += '\\';
- Line 370: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: output += "'\\''";  // End quote, escaped quote, start quote
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: output += "'\\''";  // End quote, escaped quote, start quote
- Line 371: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: output += "'\\''";  // End quote, escaped quote, start quote
- Line 386: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  output += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  output += "\\\""; break;
- Line 388: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': output += "\\\\"; break;
- Line 389: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': output += "\\b"; break;
- Line 390: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': output += "\\f"; break;
- Line 391: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': output += "\\n"; break;
- Line 392: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': output += "\\r"; break;
- Line 393: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': output += "\\t"; break;
- Line 398: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(buf, sizeof(buf), "\\u%04X", (unsigned)c);

### src/security/security_evidence_collector.cpp
Total findings: 21

- Line 158: severity=CRITICAL; category=array_bounds <!-- FALSE_POSITIVE: raw is uint8_t[16], so indices 6 and 8 are in-bounds for UUID version/variant bit setting -->
  Description: Array bounds violation: loop 16 > array 6
  Remediation: Fix loop condition or increase array size
  Context: raw[6] = (raw[6] & 0x0F) | 0x40;
- Line 159: severity=CRITICAL; category=array_bounds <!-- FALSE_POSITIVE: raw is uint8_t[16], index 8 is in-bounds for UUID variant bit setting -->
  Description: Array bounds violation: loop 16 > array 8
  Remediation: Fix loop condition or increase array size
  Context: raw[8] = (raw[8] & 0x3F) | 0x80;
- Line 239: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: snap.total_roles = static_cast<uint64_t>(rbac_->listRoles().size());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 38: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["data_access_events"]   = data_access_events;
- Line 508: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(evidence_store_path)) {
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& r : key_rotation_log) rotations.push_back(r.toJson());
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& r : key_rotations) rotations.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& r : key_rotations) rotations.push_back(r.toJson());
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
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: evidence.config_audit_trail.push_back(entry.record);
- Line 401: severity=MEDIUM; category=uncaught_exception <!-- FIXED: catch(...) narrowed to catch(const std::exception&) with debug logging -->
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 523: severity=MEDIUM; category=uncaught_exception <!-- FIXED: per-file parse handler now catches const std::exception& and logs file/error -->
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/security/embedded_user_registration_plugin.cpp
Total findings: 19

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
- Line 117: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: users_[user_id] = user_data;
- Line 167: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [user_id, user_data] : users_) {
- Line 340: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t end = s.find(delim, start);
- Line 340: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t end = s.find(delim, start);
- Line 431: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // We always produced m=19456,t=2,p=1 but parse them for forward compat.
  Confidence: band=high; score=0.8
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: user_data.roles.push_back("readonly");
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
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(s.substr(start, end == std::string::npos ? end : end - start));
- Line 394: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_KDF_CTX_free(ctx);
- Line 539: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);

### src/security/hsm_provider.cpp
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3462 [HSM] Production failsafe: ... (2026-03-12) | #3454 fix: Wire PKCS#11 H
- Line 72: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static std::vector<uint8_t> stub_aes_encrypt(const std::vector<uint8_t>& key, const std::vector<uint
- Line 101: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static std::vector<uint8_t> stub_aes_decrypt(const std::vector<uint8_t>& key, const std::vector<uint
- Line 186: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: impl_->stub_kek.resize(32);
- Line 187: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {
- Line 339: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = stub_aes_encrypt(impl_->stub_kek, data);
- Line 339: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto result = stub_aes_encrypt(impl_->stub_kek, data);
- Line 363: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = stub_aes_decrypt(impl_->stub_kek, encrypted);
- Line 257: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 290: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 333: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 357: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 393: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 417: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 441: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/security/aql_injection_detector.cpp
Total findings: 16

- Line 103: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "Parse error with suspicious tokens [{}]: {}",
- Line 331: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& pattern : patterns) {
  Confidence: band=very_high; score=0.9
- Line 406: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ast.return_node && ast.return_node->expression) {
- Line 407: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (scanExpressionForDangerousOps(ast.return_node->expression)) {
- Line 502: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, value] : obj_expr->fields) {
- Line 547: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ast.return_node && ast.return_node->expression) {
- Line 548: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: extractStringLiteralsFromExpression(ast.return_node->expression, literals);
- Line 621: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& [key, value] : obj_expr->fields) {
- Line 24: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
  Confidence: band=medium; score=0.56
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.detected_patterns.push_back(literal);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex(R"(-{2}|/\*|\*/)"),
- Line 348: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex(R"(-{2}|/\*|\*/)"),
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns.push_back(match.str());
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back(match.str());
- Line 585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: literals.push_back(std::get<std::string>(literal_expr->value));

### src/security/confidential_computing.cpp
Total findings: 15

- Line 191: severity=CRITICAL; category=no_timeout <!-- FIXED: O_NONBLOCK added to prevent indefinite blocking on device open -->
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open("/dev/cpu/0/msr", O_RDONLY | O_CLOEXEC);
- Line 387: severity=CRITICAL; category=no_timeout <!-- FIXED: O_NONBLOCK added to prevent indefinite blocking on device open -->
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open("/dev/tdx_guest", O_RDWR | O_CLOEXEC);
- Line 415: severity=CRITICAL; category=no_timeout <!-- FIXED: O_NONBLOCK added to prevent indefinite blocking on device open -->
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open("/dev/tdx_guest", O_RDWR | O_CLOEXEC);
- Line 474: severity=CRITICAL; category=no_timeout <!-- FIXED: O_NONBLOCK added to prevent indefinite blocking on device open -->
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(dev, O_RDONLY | O_CLOEXEC);
- Line 502: severity=CRITICAL; category=no_timeout <!-- FIXED: O_NONBLOCK added to prevent indefinite blocking on device open -->
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open("/dev/sev-guest", O_RDWR | O_CLOEXEC);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 199: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
  <!-- FIXED: replaced with ScopedFd RAII guard; fd closed automatically on all paths -->
- Line 264: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 311: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 390: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
  <!-- FIXED: replaced with ScopedFd RAII guard; fd closed automatically on all paths -->
- Line 429: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
  <!-- FIXED: replaced with ScopedFd RAII guard; fd closed automatically on all paths -->
- Line 477: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
  <!-- FIXED: replaced with ScopedFd RAII guard; fd closed automatically on all paths -->
- Line 525: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
  <!-- FIXED: replaced with ScopedFd RAII guard; fd closed automatically on all paths -->

### src/security/mock_key_provider.cpp
Total findings: 15

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
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyOperationException("Key already exists: " + key_id + " v" + std::to_string(version));
- Line 136: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Mark old ACTIVE keys as DEPRECATED
  Confidence: band=high; score=0.8
- Line 205: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw KeyNotFoundException(key_id, version);
- Line 211: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: ACTIVE = nullptr;
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.metadata);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.metadata);
  Confidence: band=high; score=0.74

### src/security/vcc_pki_client.cpp
Total findings: 15

- Line 133: severity=CRITICAL; category=missing_dtor <!-- FALSE_POSITIVE: VCCPKIClient has explicit destructor declared in header and defaulted in source -->
  Description: Class VCCPKIClient allocates resources but has no destructor
  Remediation: Add explicit destructor: ~VCCPKIClient() { /* cleanup */ }
  Context: class/struct VCCPKIClient
- Line 135: severity=CRITICAL; category=missing_dtor <!-- FALSE_POSITIVE: curl_slist is a C struct; lifecycle is managed via curl_slist_free_all in Impl::~Impl() -->
  Description: Class curl_slist allocates resources but has no destructor
  Remediation: Add explicit destructor: ~curl_slist() { /* cleanup */ }
  Context: class/struct curl_slist
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 171: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(url, "GET", "", timeout_ms_);
- Line 280: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(url, "POST", body_str, timeout_ms_);
- Line 284: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: nlohmann::json body = request.toJson();
- Line 171: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: crl.push_back(CRLEntry::fromJson(entry_json));
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: crl.push_back(CRLEntry::fromJson(entry_json));
- Line 330: severity=MEDIUM; category=uncaught_exception <!-- FIXED: healthCheck() now catches const std::exception& instead of catch(...) -->
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 391: severity=MEDIUM; category=manual_cleanup <!-- STALE: parseCertificate() now uses BIGNUM_ptr and OPENSSL_string_ptr RAII wrappers -->
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(x509);
- Line 396: severity=MEDIUM; category=manual_cleanup <!-- STALE: parseCertificate() now uses BIGNUM_ptr and OPENSSL_string_ptr RAII wrappers -->
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(x509);
- Line 402: severity=MEDIUM; category=manual_cleanup <!-- STALE: parseCertificate() now uses BIGNUM_ptr and OPENSSL_string_ptr RAII wrappers -->
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(x509);
- Line 479: severity=MEDIUM; category=manual_cleanup <!-- FIXED: X509_STORE_CTX is now managed by X509_STORE_CTX_ptr RAII -->
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(x509_cert);

### src/security/manifest_signer.cpp
Total findings: 13

- Line 218: severity=CRITICAL; category=data_race <!-- FIXED: signManifest() now acquires std::lock_guard<std::mutex> lock(mtx_) -->
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: SigningResult result = signing_service_->sign(data, config_.key_id);
- Line 243: severity=CRITICAL; category=data_race <!-- FIXED: verifySignature() now acquires std::lock_guard<std::mutex> lock(mtx_) -->
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool valid = signing_service_->verify(data, signature, signed_manifest.signer_id);
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3342 [plugins] Remote plugin loa... (2026-03-12) | #2533 [plugins] Add dedic
- Line 39: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string result(buffer_ptr->data, buffer_ptr->length);
- Line 175: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(root_path)) {
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missing_files.push_back(file_entry.path);
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.missing_files.push_back(file_entry.path);
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.modified_files.push_back(file_entry.path + " (size mismatch)");

### src/security/row_level_security.cpp
Total findings: 13

- Line 290: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(ctx.roles.begin(), ctx.roles.end(), required_role)
- Line 290: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(ctx.roles.begin(), ctx.roles.end(), required_role)
- Line 97: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.applicable_roles.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: p.applicable_roles.push_back(r.get<std::string>());
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
- Line 263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(p.toJson());
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(&policy);
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(&policy);
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: permissive_policies.push_back(p);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(row);
  Confidence: band=high; score=0.74

### src/security/access_control.cpp
Total findings: 10

- Line 114: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: AccessControl::AuthenticationResult AccessControl::authenticate(const Credentials& credentials) {
  Confidence: band=very_high; score=0.99
  <!-- FALSE_POSITIVE: authenticate() calls logSecurityEvent() 6+ times covering all outcomes (line 73,102,126,145,161,175) -->
- Line 461: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: bool AccessControl::authorize(const AuthorizationContext& context) {
  Confidence: band=very_high; score=0.99
  <!-- FALSE_POSITIVE: authorize() calls logSecurityEvent() extensively (lines 210,223,240,283,343,372,398,446,471,497,515,526) -->
- Line 490: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: auto abac_decision = policy_engine_.authorize(
  Confidence: band=very_high; score=0.99
  <!-- FALSE_POSITIVE: scanner matched inner call site; outer authorize() already audited -->
- Line 557: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: return authorize(context);
  Confidence: band=very_high; score=0.99
  <!-- FALSE_POSITIVE: tail-call delegates to authorize() which is fully audited -->
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
  <!-- FALSE_POSITIVE: line-0 scanner artifact; no actionable location -->
- Line 987: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: unsigned char buffer[32];
  <!-- FALSE_POSITIVE: buffer[32] accessed via loop bounded by sizeof(buffer)=32; no OOB possible -->
- Line 992: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
  <!-- FALSE_POSITIVE: loop index i bounded by sizeof(buffer); scanner does not infer sizeof() -->
- Line 907: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.record);
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: result is nlohmann::json::array(), not std::vector; .reserve() not applicable -->
- Line 908: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry.record);
  <!-- FALSE_POSITIVE: nlohmann::json::array() push_back; not a std::vector -->
- Line 975: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired_sessions.push_back(token);
  Confidence: band=high; score=0.74
  <!-- FIXED: expired_sessions.reserve(sessions_.size()) added before loop in cleanupExpiredSessions() -->

### src/security/usb_admin_authenticator.cpp
Total findings: 9

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #401 Replace Security Stubs with... (2026-03-11) | #1100 [WIP] Fix missing an
  <!-- FALSE_POSITIVE: scanner matched PR History comment in file header, not code -->
- Line 588: severity=HIGH; category=windows_only_api
  Description: Windows-only API RegOpenKeyEx without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: LONG rc = RegOpenKeyExA(
  <!-- FALSE_POSITIVE: USB license check is Windows-only by design; enclosing block gated by _WIN32 ifdef -->
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: license.admin_scopes.push_back(scope.get<std::string>());
  Confidence: band=high; score=0.74
  <!-- REAL: admin_scopes pushed in for(auto& scope : scopes_json) loop; reserve(scopes_json.size()) missing -->
- Line 442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: license.admin_scopes.push_back(scope.get<std::string>());
  <!-- REAL: duplicate; same loop as above -->
- Line 476: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n"
  <!-- FALSE_POSITIVE: '/' inside a Base64-encoded PEM certificate literal; not a filesystem path -->
- Line 554: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
  <!-- FIXED: ctx now managed by EVP_MD_CTX_ptr RAII; EVP_MD_CTX_free() call removed -->
- Line 555: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(public_key);
  <!-- FIXED: public_key now managed by EVP_PKEY_ptr RAII; EVP_PKEY_free() call removed -->

### src/security/arrow_user_registration_plugin.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 42: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: [[maybe_unused]] const std::unordered_map<std::string, std::string>& attributes)
  Confidence: band=medium; score=0.66
  <!-- FALSE_POSITIVE: scanner matched the parameter type, not iteration code; attributes unused in this function -->
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back("readonly");
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: single conditional push_back, not in a loop -->
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data.roles.push_back("readonly");
  <!-- FALSE_POSITIVE: single conditional push_back, not in a loop -->
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: users.push_back(data);
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: single push_back inside a for-loop that processes individual parsed objects; not a bulk-fill loop -->
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back(role);
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: outer loop is over characters of a comma-separated string; push_back fires at comma boundaries, not every iteration; pre-scan to count commas would be required to reserve -->
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back(role);
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: duplicate -->
- Line 246: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
  <!-- STALE: current arrow_user_registration_plugin.cpp uses EVP_MD_CTX_ptr mdctx RAII at line ~250 -->

### src/security/secret_manager.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
  <!-- FALSE_POSITIVE: line-0 scanner artifact -->
- Line 69: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::length_error(
  <!-- FALSE_POSITIVE: intentional validation throw in addSecret(); callers are expected to catch -->
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entry.versions.push_back(std::move(ver));
  Confidence: band=high; score=0.74
  <!-- FALSE_POSITIVE: single push_back in a non-loop context (entry construction) -->
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
  <!-- STALE: current code has names.reserve(secrets_.size()) before the loop -->
- Line 300: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: case SecretStatus::ACTIVE:   ++s.active_versions;   break;
- Line 301: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: case SecretStatus::RETIRING: ++s.retiring_versions; break;
- Line 302: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: case SecretStatus::REVOKED:  ++s.revoked_versions;  break;

### src/security/access_control_manager.cpp
Total findings: 7

- Line 109: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log <!-- FIXED: authenticate() now emits structured JSON audit entries via THEMIS_INFO for all outcomes -->
  Description: Security function "authenticate" without audit log
  Context: std::optional<SecurityContext> AccessControlManager::authenticate(
  Confidence: band=very_high; score=0.99
- Line 155: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log <!-- FALSE_POSITIVE: auditAccessDecision() helper covers authorize() outcomes (called at lines 178, 204, 232) -->
  Description: Security function "authorize" without audit log
  Context: AccessDecision AccessControlManager::authorize(
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log <!-- FALSE_POSITIVE: inner policy_engine_.authorize() result is captured and audited by outer authorize() -->
  Description: Security function "authorize" without audit log
  Context: auto abac_decision = policy_engine_.authorize(
  Confidence: band=very_high; score=0.99
- Line 247: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log <!-- FIXED: authenticate() now emits audit log for all outcomes including caller-side invocations -->
  Description: Security function "authenticate" without audit log
  Context: auto context = authenticate(token, source_ip);
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log <!-- FALSE_POSITIVE: authorize() at line 155 already calls auditAccessDecision() -->
  Description: Security function "authorize" without audit log
  Context: return authorize(*context, resource, action);
  Confidence: band=very_high; score=0.99
- Line 461: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("AUDIT [ACCESS_CONTROL]: {}", audit_entry.dump());
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decision.applied_permissions.push_back(perm.toString());
  Confidence: band=high; score=0.74

### src/security/vault_signing_provider.cpp
Total findings: 7

- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret.push_back(b64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ret.push_back(b64_chars[(val >> valb) & 0x3F]);
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (valb > -6) ret.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (ret.size() % 4) ret.push_back('=');
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));

### src/security/webdav_user_registration_plugin.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 104: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.attributes[key] = value;
- Line 605: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: body->append(ptr, size * nmemb);
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.roles.push_back("readonly");
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data.roles.push_back("readonly");
- Line 583: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: roles.push_back("admin");
- Line 637: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);

### src/security/query_masking_policy.cpp
Total findings: 6

- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: masked.push_back(maskNode(item, "", snapshot));
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: masked.push_back(maskNode(item, "", snapshot));
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: masked.push_back(maskNode(item, "", snapshot));
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(elem, key, snapshot));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(elem, key, snapshot));
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(maskNode(elem, key, snapshot));

### src/security/cms_signing.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 55: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: res.signature.assign(reinterpret_cast<uint8_t*>(bptr->data), reinterpret_cast<uint8_t*>(bptr->data)
- Line 58: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(out);
- Line 59: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: CMS_ContentInfo_free(cms);

### src/security/fips_crypto_mode.cpp
Total findings: 5

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 112: severity=CRITICAL; category=new_without_delete <!-- FIXED: impl_ converted to std::unique_ptr<Impl>; constructor uses std::make_unique<Impl>() -->
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: FipsCryptoMode::FipsCryptoMode() : impl_(new Impl()) {}
- Line 112: severity=CRITICAL; category=smart_ptr_misuse <!-- FIXED: impl_ converted to std::unique_ptr<Impl>; constructor uses std::make_unique<Impl>() -->
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: FipsCryptoMode::FipsCryptoMode() : impl_(new Impl()) {}
- Line 115: severity=HIGH; category=delete_no_nullptr <!-- FIXED: unique_ptr destructor replaces manual delete -->
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: impl_ = nullptr;
  Context: delete impl_;
- Line 207: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw FipsPolicyViolation(

### src/security/keyprovider_signing.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 51: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 74: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/security/usb_volume_hardening.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 85: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 100: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 103: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/security/binary_manifest.cpp
Total findings: 4

- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["files"].push_back(file.to_json());
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["files"].push_back(file.to_json());
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.files_.push_back(BinaryFileEntry::from_json(file_json));
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.files_.push_back(BinaryFileEntry::from_json(file_json));

### src/security/vram_secure_clear.cpp
Total findings: 4

- Line 46: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
- Line 118: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
- Line 191: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
- Line 193: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vptr[i] = pattern;

### src/security/ai_snapshot_cleanup.cpp
Total findings: 3

- Line 47: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.snapshot_dir, ec)) {
- Line 61: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& sub : fs::recursive_directory_iterator(entry.path(), ec)) {
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back({entry.path().string(), size, ts});

### src/security/zero_trust_policy_enforcer.cpp
Total findings: 3

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 360: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### include/security/input_validator.hpp
Total findings: 2

- Line 117: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   - Filename contains only safe characters [a-zA-Z0-9._-]
- Line 29: severity=MEDIUM; category=unportable_pragma
  Description: Non-standard pragma may not be supported on all platforms
  Remediation: Use standard C++ mechanisms: static_assert, or move to portable config
  Context: #pragma once

### src/security/encrypted_field.cpp
Total findings: 2

- Line 147: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Invalid vector serialization: too short");
- Line 157: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException(

### src/security/intent_classifier.cpp
Total findings: 2

- Line 92: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // for AI Safety callers; standard callers keep 0.85 for backwards compat).
  Confidence: band=high; score=0.8
- Line 96: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: {"REMOVE @",         0.40},  // Parametrised single-key delete (bind var)

### include/security/examples/intent_classifier_example.cpp
Total findings: 1

- Line 193: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/optimization_layers/IMPL-B7-intent-classifier.md\n";

### src/security/ai_operation_guard.cpp
Total findings: 1

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::toupper(c)));
  Confidence: band=high; score=0.74

### src/security/prompt_injection_pattern_registry.cpp
Total findings: 1

- Line 87: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(\[\s*system\s*\]|\[INST\]|\[\/INST\]|<\|system\|>|<\|user\|>|<\|assistant\|>)",

### src/security/user_registration_plugin.cpp
Total findings: 1

- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(plugin);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
