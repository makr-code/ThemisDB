# security Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: security
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 472
- Actionable Findings (Critical + High): 299
- Affected Files: 40

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 35 |
| High | 264 |
| Medium | 169 |
| Low | 4 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 106 |
| uncaught_exception | 51 |
| manual_cleanup | 47 |
| no_retry_logic | 23 |
| generic_catch | 19 |
| string_concat_loop | 19 |
| stale_doc_section_reference | 15 |
| copy_overhead | 13 |
| uninitialized_access | 13 |
| legacy_or_compat_path | 12 |
| range_temporary | 12 |
| missing_move_constructor_defaulted | 10 |
| explicit_delete | 9 |
| missing_dtor | 9 |
| null_dereference | 9 |
| hardcoded_path | 8 |
| missing_audit_log | 8 |
| unordered_container_iter | 8 |
| deadlock_risk | 7 |
| delete_without_nullptr | 7 |
| array_bounds_violation | 5 |
| delete_no_nullptr | 5 |
| no_timeout | 5 |
| size_assumption | 5 |
| uninitialized_array | 5 |
| db_connection_leak | 4 |
| module_doc_linkset_drift | 4 |
| pointer_arithmetic_unbounded | 4 |
| primitive_no_volatile | 3 |
| array_bounds | 2 |
| data_race | 2 |
| repeated_search | 2 |
| socket_leak | 2 |
| uninitialized_member_field | 2 |
| arithmetic_overflow | 1 |
| duplicate_qualified_signature | 1 |
| endianness_assumption | 1 |
| exception_in_destructor | 1 |
| expensive_inner_op | 1 |
| hardcoded_output | 1 |
| missing_latency_metric | 1 |
| missing_trace_point | 1 |
| o_n_squared | 1 |
| path_traversal | 1 |
| pointer_without_null_check | 1 |
| regex_in_loop | 1 |
| shared_state_no_sync | 1 |
| timestamp_sorting_unstable | 1 |
| unchecked_memcpy | 1 |
| unnecessary_copy | 1 |
| windows_only_api | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| security/vault_key_provider.cpp | 37 | 0 | 31 | 6 | 0 |
| security/timestamp_authority.cpp | 32 | 0 | 27 | 5 | 0 |
| security/hsm_key_provider_adapter.cpp | 31 | 0 | 21 | 10 | 0 |
| security/input_validator.cpp | 28 | 0 | 6 | 22 | 0 |
| security/pki_key_provider.cpp | 27 | 1 | 20 | 6 | 0 |
| security/post_quantum_crypto.cpp | 27 | 3 | 18 | 6 | 0 |
| security/hsm_provider_pkcs11.cpp | 26 | 1 | 17 | 8 | 0 |
| security/hsm_provider.cpp | 24 | 1 | 6 | 17 | 0 |
| security/malware_scanner.cpp | 22 | 4 | 13 | 5 | 0 |
| security/aql_injection_detector.cpp | 15 | 0 | 8 | 7 | 0 |
| security/confidential_computing.cpp | 15 | 5 | 6 | 4 | 0 |
| security/vcc_pki_client.cpp | 15 | 2 | 5 | 8 | 0 |
| security/embedded_user_registration_plugin.cpp | 14 | 0 | 10 | 4 | 0 |
| security/rbac.cpp | 14 | 0 | 3 | 11 | 0 |
| security/timestamp_authority_openssl.cpp | 14 | 0 | 9 | 5 | 0 |
| security/mock_key_provider.cpp | 13 | 0 | 13 | 0 | 0 |
| security/security_evidence_collector.cpp | 12 | 5 | 5 | 2 | 0 |
| security/cms_signing.cpp | 11 | 3 | 5 | 3 | 0 |
| security/keyprovider_signing.cpp | 10 | 0 | 3 | 7 | 0 |
| security/manifest_signer.cpp | 10 | 0 | 8 | 2 | 0 |
| security/usb_admin_authenticator.cpp | 9 | 0 | 4 | 5 | 0 |
| security/access_control_manager.cpp | 8 | 4 | 4 | 0 | 0 |
| security/ai_snapshot_cleanup.cpp | 8 | 0 | 6 | 2 | 0 |
| security/field_encryption.cpp | 6 | 1 | 2 | 3 | 0 |
| security/intent_classifier.cpp | 6 | 0 | 3 | 3 | 0 |
| security/webdav_user_registration_plugin.cpp | 6 | 0 | 1 | 5 | 0 |
| security/access_control.cpp | 5 | 4 | 1 | 0 | 0 |
| security/row_level_security.cpp | 4 | 0 | 1 | 3 | 0 |
| security/vram_secure_clear.cpp | 4 | 0 | 4 | 0 | 0 |
| security/zero_trust_policy_enforcer.cpp | 4 | 0 | 0 | 4 | 0 |
| security/arrow_user_registration_plugin.cpp | 3 | 0 | 1 | 2 | 0 |
| security/usb_volume_hardening.cpp | 3 | 1 | 1 | 1 | 0 |
| security/secret_manager.cpp | 2 | 0 | 2 | 0 | 0 |
| security/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| security/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| security/README.md | 1 | 0 | 0 | 0 | 1 |
| security/ROADMAP.md | 1 | 0 | 0 | 0 | 1 |
| security/ai_operation_guard.cpp | 1 | 0 | 0 | 1 | 0 |
| security/prompt_injection_pattern_registry.cpp | 1 | 0 | 0 | 1 | 0 |
| security/vault_signing_provider.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### security/vault_key_provider.cpp
Total findings: 37

- Line 161: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::timed_mutex> lock(mutex);

            CURL* raw_handle = curl_easy_duphandle(curl);

            if (!raw_handle) {

                throw KeyOperationException("curl_easy_duphandle failed", -1, std::string(), true);

            }

            local_curl_raw = CURL_ptr(raw_handle);

        }
- Line 187: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_slist* raw_headers = nullptr;

        raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + config.vault_token).c_str());

        if (!raw_headers) {

            throw KeyOperationException("Failed to create HTTP headers", -1, std::string(), false);

        }

        raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");

        if (!raw_headers) {
- Line 191: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");

        if (!raw_headers) {

            throw KeyOperationException("Failed to append Content-Type header", -1, std::string(), false);

        }

        CURLSList_ptr headers(raw_headers);
- Line 204: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: long http_code = 0;

        if (res == CURLE_OK) {

            if (curl_easy_getinfo(local_curl, CURLINFO_RESPONSE_CODE, &http_code) != CURLE_OK) {

                throw KeyOperationException("curl_easy_getinfo failed", -1, std::string(), false);

            }

        }

        // local_curl_raw and headers are automatically cleaned up here via RAII
- Line 219: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: transient = true; break;

                default: transient = false; break;

            }

            throw KeyOperationException(std::string("CURL error: ") + curl_easy_strerror(res), -1, std::string(), transient);

        }



        if (http_code == 404) {
- Line 223: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        if (http_code == 404) {

            throw KeyNotFoundException("key", 0);

        } else if (http_code == 403) {

            throw KeyOperationException("Vault authentication failed (403 Forbidden)", (int)http_code, response, false);

        } else if (http_code >= 500) {
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (http_code == 404) {

            throw KeyNotFoundException("key", 0);

        } else if (http_code == 403) {

            throw KeyOperationException("Vault authentication failed (403 Forbidden)", (int)http_code, response, false);

        } else if (http_code >= 500) {

            throw KeyOperationException("Vault server error (HTTP " + std::to_string(http_code) + ")", (int)http_code, response, true);

        } else if (http_code >= 400) {
- Line 227: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else if (http_code == 403) {

            throw KeyOperationException("Vault authentication failed (403 Forbidden)", (int)http_code, response, false);

        } else if (http_code >= 500) {

            throw KeyOperationException("Vault server error (HTTP " + std::to_string(http_code) + ")", (int)http_code, response, true);

        } else if (http_code >= 400) {

            throw KeyOperationException("Vault request failed (HTTP " + std::to_string(http_code) + "): " + response, (int)http_code, response, false);

        }
- Line 229: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else if (http_code >= 500) {

            throw KeyOperationException("Vault server error (HTTP " + std::to_string(http_code) + ")", (int)http_code, response, true);

        } else if (http_code >= 400) {

            throw KeyOperationException("Vault request failed (HTTP " + std::to_string(http_code) + "): " + response, (int)http_code, response, false);

        }



        return response;
- Line 438: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const auto& version_data = data["versions"][version_key];

                    if (version_data.contains("created_time")) {

                        // Parse RFC3339 timestamp (simplified)

                        std::string created = version_data["created_time"].get<std::string>();

                        // For now, use current time (proper parsing would use strptime)

                        meta.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(

                            std::chrono::system_clock::now().time_since_epoch()
- Line 468: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(5))) {
- Line 493: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (!lock.try_lock_for(std::chrono::seconds(5))) {
- Line 518: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 520: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 526: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 621: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 632: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 661: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check if key is deprecated first (safety check)
- Line 665: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: throw KeyOperationException("Cannot delete ACTIVE key. Deprecate it first via rotation.");
- Line 665: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

        KeyMetadata meta = getKeyMetadata(key_id, version);

        if (meta.status == KeyStatus::ACTIVE) {

            throw KeyOperationException("Cannot delete ACTIVE key. Deprecate it first via rotation.");

        }

    } catch (const KeyNotFoundException&) {

        // Key already doesn't exist, that's fine
- Line 665: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: throw KeyOperationException("Cannot delete ACTIVE key. Deprecate it first via rotation.");
- Line 684: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::timed_mutex> lock(impl_->mutex);

        CURL* raw_handle = curl_easy_duphandle(impl_->curl);

        if (!raw_handle) {

            throw KeyOperationException("curl_easy_duphandle failed during deleteKey");

        }

        local_curl_raw = CURL_ptr(raw_handle);

        vault_token  = impl_->config.vault_token;
- Line 700: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_slist* raw_headers = nullptr;

    raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + vault_token).c_str());

    if (!raw_headers) {

        throw KeyOperationException("Failed to create HTTP headers for deleteKey", -1, std::string(), false);

    }

    CURLSList_ptr headers(raw_headers);

    curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers.get());
- Line 710: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
- Line 710: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // RAII wrappers automatically clean up local_curl_raw and headers



    if (res != CURLE_OK) {

        throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));

    }

    

    // Clear from cache (re-acquire mutex for cache modification)
- Line 710: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
- Line 710: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // RAII wrappers automatically clean up local_curl_raw and headers



    if (res != CURLE_OK) {

        throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));

    }

    

    // Clear from cache (re-acquire mutex for cache modification)
- Line 780: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::timed_mutex> lock(impl_->mutex);

        CURL* raw_handle = curl_easy_duphandle(impl_->curl);

        if (!raw_handle) {

            throw KeyOperationException("curl_easy_duphandle failed during createKey");

        }

        local_curl_raw = CURL_ptr(raw_handle);

        vault_token = impl_->config.vault_token;
- Line 798: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_slist* raw_headers = nullptr;

    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");

    if (!raw_headers) {

        throw KeyOperationException("Failed to create HTTP headers for createKey", -1, std::string(), false);

    }

    raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + vault_token).c_str());

    if (!raw_headers) {
- Line 802: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + vault_token).c_str());

    if (!raw_headers) {

        throw KeyOperationException("Failed to append Vault-Token header to createKey", -1, std::string(), false);

    }

    CURLSList_ptr headers(raw_headers);

    curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers.get());
- Line 812: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // RAII wrappers automatically clean up local_curl_raw and headers



    if (res != CURLE_OK) {

        throw KeyOperationException(std::string("Failed to create key: ") + curl_easy_strerror(res));

    }



    // Parse response to get version
- Line 314: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 327: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 628: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool is_transient = true;
- Line 667: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 710: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
- Line 728: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### security/timestamp_authority.cpp
Total findings: 32

- Line 87: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: //          pimpl compatibility while still tracking minimal runtime state
- Line 588: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 594: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 603: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 675: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
- Line 687: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: h = curl_slist_append(h, "Accept: application/timestamp-reply");

        headers.reset(h);



        curl_easy_setopt(curl.get(), CURLOPT_URL,           config_.url.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_POST,           1L);

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));
- Line 688: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: headers.reset(h);



        curl_easy_setopt(curl.get(), CURLOPT_URL,           config_.url.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_POST,           1L);

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());
- Line 689: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_URL,           config_.url.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_POST,           1L);

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);
- Line 690: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_URL,           config_.url.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_POST,           1L);

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);
- Line 691: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_POST,           1L);

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);

        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,        static_cast<long>(config_.timeout_seconds));
- Line 692: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);

        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,        static_cast<long>(config_.timeout_seconds));

        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
- Line 693: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);

        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,        static_cast<long>(config_.timeout_seconds));

        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
- Line 694: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);

        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,        static_cast<long>(config_.timeout_seconds));

        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);



        if (!config_.ca_cert_path.empty())
- Line 695: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);

        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,        static_cast<long>(config_.timeout_seconds));

        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);



        if (!config_.ca_cert_path.empty())

            curl_easy_setopt(curl.get(), CURLOPT_CAINFO, config_.ca_cert_path.c_str());
- Line 698: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);



        if (!config_.ca_cert_path.empty())

            curl_easy_setopt(curl.get(), CURLOPT_CAINFO, config_.ca_cert_path.c_str());



        if (!config_.verify_tsa_cert) {

            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
- Line 701: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_CAINFO, config_.ca_cert_path.c_str());



        if (!config_.verify_tsa_cert) {

            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);

            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);

        }
- Line 702: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (!config_.verify_tsa_cert) {

            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);

            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);

        }



        CURLcode res = curl_easy_perform(curl.get());
- Line 705: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);

        }



        CURLcode res = curl_easy_perform(curl.get());

        long http_code = 0;

        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
- Line 707: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: CURLcode res = curl_easy_perform(curl.get());

        long http_code = 0;

        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);



        if (res == CURLE_OK && http_code == 200) {

            // Success - return response immediately
- Line 736: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
- Line 810: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1058: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1091: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1139: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1144: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1170: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1239: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 26: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 309: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'eIDAS TSA Validation".' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md §"eIDAS TSA Validation".
- Line 666: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
- Line 824: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: mem.release();
- Line 1072: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: mem.release();

### security/hsm_key_provider_adapter.cpp
Total findings: 31

- Line 139: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 152: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Mark old version as DEPRECATED
- Line 161: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 162: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 163: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 164: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 165: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 166: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 168: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto key_it = key_store_.find(key_id);

    if (key_it == key_store_.end()) {

        throw KeyNotFoundException(key_id, version);

    }

    

    if (version == 0) {
- Line 239: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 239: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Check if key is ACTIVE

    if (version_it->second.metadata.status == KeyStatus::ACTIVE) {

        throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));

    }

    

    // Mark as DELETED
- Line 239: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 295: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 296: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 297: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 298: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 301: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 304: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 308: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Mark other active versions as deprecated
- Line 146: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 239: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 288: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 399: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.hsm_errors++;

                throw KeyOperationException(

                    "WrapDEKFn bridge failed: " + std::string(e.what()));

            } catch (...) {

                stats_.hsm_errors++;

                throw KeyOperationException("WrapDEKFn bridge failed: unknown error");

            }
- Line 399: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 435: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'HSM Key Provider Production' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md § "HSM Key Provider Production"
- Line 436: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Security stub lifecycle' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: ROADMAP.md §Security stub lifecycle
- Line 449: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 474: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.hsm_errors++;

                throw KeyOperationException(

                    "UnwrapDEKFn bridge failed: " + std::string(e.what()));

            } catch (...) {

                stats_.hsm_errors++;

                throw KeyOperationException("UnwrapDEKFn bridge failed: unknown error");

            }
- Line 474: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### security/input_validator.cpp
Total findings: 28

- Line 155: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "Use only [a-zA-Z0-9._-]"};
- Line 243: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 289: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "Start identifier with [a-zA-Z_]"};
- Line 297: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "Use only [a-zA-Z0-9_]"};
- Line 398: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "\\u%04X", (unsigned)c);
- Line 398: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(buf, sizeof(buf), "\\u%04X", (unsigned)c);
- Line 83: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "Path contains traversal sequences (../, ..\\)",
- Line 147: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "Use only safe filenames without ../ or ..\\"};
- Line 336: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '<':  output += "&lt;"; break;
- Line 337: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '<':  output += "&lt;"; break;
- Line 338: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '>':  output += "&gt;"; break;
- Line 339: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '&':  output += "&amp;"; break;
- Line 340: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  output += "&quot;"; break;
- Line 341: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': output += "&#39;"; break;
- Line 355: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: output += '\\';
- Line 356: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: output += '\\';
- Line 370: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: output += "'\\''";  // End quote, escaped quote, start quote
- Line 371: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: output += "'\\''";  // End quote, escaped quote, start quote
- Line 371: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: output += "'\\''";  // End quote, escaped quote, start quote
- Line 386: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  output += "\\\""; break;
- Line 387: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  output += "\\\""; break;
- Line 388: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': output += "\\\\"; break;
- Line 389: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\b': output += "\\b"; break;
- Line 390: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\f': output += "\\f"; break;
- Line 391: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': output += "\\n"; break;
- Line 392: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': output += "\\r"; break;
- Line 393: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': output += "\\t"; break;
- Line 398: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(buf, sizeof(buf), "\\u%04X", (unsigned)c);

### security/pki_key_provider.cpp
Total findings: 27

- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto existing_opt = db_->get(ikm_db_key);
- Line 120: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Check if certificate is not yet valid

        result = X509_cmp_current_time(X509_get0_notBefore(cert.get()));

        if (result > 0) {

            throw std::runtime_error("Certificate is not yet valid: " + cert_path);

        }

        

        spdlog::info("PKIKeyProvider: Certificate validation passed");
- Line 129: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Extract public key from certificate

    PKI_EVP_PKEY_ptr pkey(X509_get_pubkey(cert.get()));

    if (!pkey) {

        throw std::runtime_error("Failed to extract public key from certificate");

    }

    

    // Serialize public key to DER format for key derivation
- Line 136: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: unsigned char* pubkey_der = nullptr;

    int pubkey_len = i2d_PUBKEY(pkey.get(), &pubkey_der);

    if (pubkey_len <= 0 || !pubkey_der) {

        throw std::runtime_error("Failed to serialize public key");

    }

    

    // Wrap DER pointer for automatic cleanup
- Line 185: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Generiere neues IKM (32 zufällige Bytes) und speichere hex-codiert

        ikm_raw.resize(32);

        if (RAND_bytes(ikm_raw.data(), static_cast<int>(ikm_raw.size())) != 1) {

            throw std::runtime_error("RAND_bytes für IKM fehlgeschlagen");

        }

        static const char* hex_chars = "0123456789abcdef";

        std::string hex;
- Line 236: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // If JSON parsing failed, try legacy/binary format: iv||ciphertext||tag
- Line 248: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 283: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 295: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 436: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: throw std::runtime_error("Cannot delete DEK");
- Line 436: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::scoped_lock lk(mu_);

    

    if (key_id == "dek") {

        throw std::runtime_error("Cannot delete DEK");

    }

    

    field_key_cache_.erase(key_id);
- Line 436: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: throw std::runtime_error("Cannot delete DEK");
- Line 472: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 516: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 541: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 556: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 642: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 645: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 646: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 649: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Mark old version as deprecated in metadata
- Line 651: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 33: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(BIO* p) const { if (p) BIO_free(p); }
- Line 36: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509* p) const { if (p) X509_free(p); }
- Line 39: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); }
- Line 42: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
- Line 45: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(void* p) const { if (p) OPENSSL_free(p); }
- Line 436: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: throw std::runtime_error("Cannot delete DEK");

### security/post_quantum_crypto.cpp
Total findings: 27

- Line 765: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: while (in_pos < s.size() && s[in_pos] != '=' && is_b64(s[in_pos])) {

        ca4[i++] = s[in_pos++];

        if (i == 4) {

            for (int k = 0; k < 4; ++k)

                ca4[k] = B64_DEC_TABLE[ca4[k]];

            ca3[0] = (ca4[0] << 2) | ((ca4[1] & 0x30) >> 4);

            ca3[1] = ((ca4[1] & 0x0f) << 4) | ((ca4[2] & 0x3c) >> 2);

            ca3[2] = ((ca4[2] & 0x03) << 6) | ca4[3];

            for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);

            i = 0;

        }
- Line 766: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ca4[i++] = s[in_pos++];

        if (i == 4) {

            for (int k = 0; k < 4; ++k)

                ca4[k] = B64_DEC_TABLE[ca4[k]];

            ca3[0] = (ca4[0] << 2) | ((ca4[1] & 0x30) >> 4);

            ca3[1] = ((ca4[1] & 0x0f) << 4) | ((ca4[2] & 0x3c) >> 2);

            ca3[2] = ((ca4[2] & 0x03) << 6) | ca4[3];

            for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);

            i = 0;

        }

    }
- Line 767: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (i == 4) {

            for (int k = 0; k < 4; ++k)

                ca4[k] = B64_DEC_TABLE[ca4[k]];

            ca3[0] = (ca4[0] << 2) | ((ca4[1] & 0x30) >> 4);

            ca3[1] = ((ca4[1] & 0x0f) << 4) | ((ca4[2] & 0x3c) >> 2);

            ca3[2] = ((ca4[2] & 0x03) << 6) | ca4[3];

            for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);

            i = 0;

        }

    }

    if (i) {
- Line 94: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 101: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 154: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 195: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 230: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 290: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 315: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 319: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 342: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 346: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 625: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

    // Minimum size: 4 + 32 (kem_ct) + 12 (iv) + 4 + 1 (enc_dek) + 16 (tag)

    if (wrapped_key.size() < 4 + 32 + 12 + 4 + 1 + 16) {

        throw std::runtime_error("unwrapKeyWithKyber: blob too short");

    }



    const uint8_t* p = wrapped_key.data();
- Line 634: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Read KEM ciphertext

    uint32_t kem_ct_len = read_u32_le(p); p += 4;

    if (p + kem_ct_len > end)

        throw std::runtime_error("unwrapKeyWithKyber: kem_ct truncated");

    std::vector<uint8_t> kem_ct(p, p + kem_ct_len); p += kem_ct_len;



    // Read IV (12 bytes)
- Line 724: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ret.reserve((data.size() + 2) / 3 * 4);  // Pre-allocate for base64 output
- Line 998: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1046: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1086: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1090: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 738: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: ret += '=';
- Line 936: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 1031: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) [[unlikely]] {

        try {

            return fn(message, secret_key);

        } catch (...) {

            THEMIS_WARN("SphincsPlus::sign: exception from user callback (suppressed)");

            return {};

        }
- Line 1031: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1077: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) [[unlikely]] {

        try {

            return fn(message, signature, public_key);

        } catch (...) {

            THEMIS_WARN("SphincsPlus::verify: exception from user callback (suppressed)");

            return false;

        }
- Line 1077: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### security/hsm_provider_pkcs11.cpp
Total findings: 26

- Line 519: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    std::vector<uint8_t> di(sizeof(SHA256_DER_PREFIX) + digest.size());', '    std::memcpy(di.data(), SHA256_DER_PREFIX, sizeof(SHA256_DER_PREFIX));', '    std::memcpy(di.data()+sizeof(SHA256_DER_PREFIX), digest.data(), digest.size());', '    return di;', '}']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3461 [HSM] Implement PKCS#11 tok... (2026-03-12) | #3458 [HSM] PKCS#11 C++ w
- Line 93: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: lib_ = dlopen(path.c_str(), RTLD_NOW);
- Line 120: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    if(data.empty()) return "";', "    // EVP_EncodeBlock adds null terminator and pads with '='", '    size_t outLen = ((data.size() + 2) / 3) * 4;', '    std::vector<unsigned char> encoded(outLen + 1);', '    int len = EVP_EncodeBlock(encoded.data(), data.data(), (int)data.size());']
- Line 144: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 174: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 426: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Generate fallback stub KEK for consistent wrap/unwrap when real HSM is unavailable

    if (!impl_->real_ready) {

        impl_->stub_kek.resize(32);

        if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {

            THEMIS_ERROR("HSMProvider: failed to generate stub KEK - aborting initialization");

            initialized_ = false;
- Line 427: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Generate fallback stub KEK for consistent wrap/unwrap when real HSM is unavailable

    if (!impl_->real_ready) {

        impl_->stub_kek.resize(32);

        if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {

            THEMIS_ERROR("HSMProvider: failed to generate stub KEK - aborting initialization");

            initialized_ = false;

            return false;
- Line 517: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::vector<uint8_t> di(sizeof(SHA256_DER_PREFIX) + digest.size());
- Line 631: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 675: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto sess = acquireSession();
- Line 722: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 793: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 843: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 892: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 976: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 1084: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mtx);
- Line 1105: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 40: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
- Line 44: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
- Line 48: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509* p) const { if (p) X509_free(p); }
- Line 56: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(BIGNUM* p) const { if (p) BN_free(p); }
- Line 77: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 2: ABAC & HSM Direct Integration' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § "Phase 2: ABAC & HSM Direct Integration"
- Line 106: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if(lib_) dlclose(lib_);
- Line 588: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(hex);
- Line 1036: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(hex);

### security/hsm_provider.cpp
Total findings: 24

- Line 56: severity=CRITICAL; category=missing_dtor
  Description: Class HSMProvider allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct HSMProvider
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3462 [HSM] Production failsafe: ... (2026-03-12) | #3454 fix: Wire PKCS#11 H
- Line 98: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    std::vector<uint8_t> iv(12);

    if (RAND_bytes(iv.data(), 12) != 1) {

        throw std::runtime_error("AES-256-GCM encryption: RAND_bytes failed: " + ossl_error());

    }

    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());

    if (!ctx) {
- Line 223: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    // Generate stub KEK for consistent wrap/unwrap operations

    impl_->stub_kek.resize(32);

    if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {

        last_error_ = "Failed to generate stub KEK";

        THEMIS_ERROR("HSMProvider stub: {}", last_error_);
- Line 224: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Generate stub KEK for consistent wrap/unwrap operations

    impl_->stub_kek.resize(32);

    if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {

        last_error_ = "Failed to generate stub KEK";

        THEMIS_ERROR("HSMProvider stub: {}", last_error_);

        initialized_ = false;
- Line 378: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    THEMIS_WARN("HSMProvider STUB encryptData - NOT hardware-protected, for development only!");

    try {

        return stub_aes_encrypt(impl_->stub_kek, data);

    } catch (const std::exception& e) {

        last_error_ = std::string("Stub AES encrypt failed: ") + e.what();

        return {};
- Line 406: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    THEMIS_WARN("HSMProvider STUB decryptData - NOT hardware-protected, for development only!");

    try {

        return stub_aes_decrypt(impl_->stub_kek, encrypted);

    } catch (const std::exception& e) {

        last_error_ = std::string("Stub AES decrypt failed: ") + e.what();

        return {};
- Line 32: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 51: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
- Line 294: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: r.error_message = std::string("signHash callback failed: ") + e.what();

            impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);

            return r;

        } catch (...) {

            r.error_message = "signHash callback failed: unknown exception";

            impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);

            return r;
- Line 294: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 327: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) {

        try {

            ok = fn(data, signature_b64, key_label.empty() ? config_.key_label : key_label);

        } catch (...) {

            ok = false;

        }

    } else {
- Line 327: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 371: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

            last_error_ = std::string("encryptData callback failed: ") + e.what();

            return {};

        } catch (...) {

            last_error_ = "encryptData callback failed: unknown exception";

            return {};

        }
- Line 371: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 399: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

            last_error_ = std::string("decryptData callback failed: ") + e.what();

            return {};

        } catch (...) {

            last_error_ = "decryptData callback failed: unknown exception";

            return {};

        }
- Line 399: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 424: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'HSM Key Management".' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md §"HSM Key Management".
- Line 438: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: last_error_ = std::string("generateKeyPair callback failed: ") + e.what();

            THEMIS_ERROR("{}", last_error_);

            return false;

        } catch (...) {

            last_error_ = "generateKeyPair callback failed: unknown exception";

            THEMIS_ERROR("{}", last_error_);

            return false;
- Line 438: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 462: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: last_error_ = std::string("importCertificate callback failed: ") + e.what();

            THEMIS_ERROR("{}", last_error_);

            return false;

        } catch (...) {

            last_error_ = "importCertificate callback failed: unknown exception";

            THEMIS_ERROR("{}", last_error_);

            return false;
- Line 462: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 486: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: last_error_ = std::string("getCertificate callback failed: ") + e.what();

            THEMIS_ERROR("{}", last_error_);

            return std::nullopt;

        } catch (...) {

            last_error_ = "getCertificate callback failed: unknown exception";

            THEMIS_ERROR("{}", last_error_);

            return std::nullopt;
- Line 486: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### security/malware_scanner.cpp
Total findings: 22

- Line 719: severity=CRITICAL; category=socket_leak
  Description: Socket created but never closed — potential resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_handle sock = socket(AF_INET, SOCK_STREAM, 0);
- Line 725: severity=CRITICAL; category=missing_dtor
  Description: Class sockaddr_in allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct sockaddr_in
- Line 734: severity=CRITICAL; category=missing_dtor
  Description: Class sockaddr allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct sockaddr
- Line 770: severity=CRITICAL; category=socket_leak
  Description: Socket created but never closed — potential resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: socket_handle sock = socket(AF_INET, SOCK_STREAM, 0);
- Line 734: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error("Invalid address");

    }

    

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {

        throw std::runtime_error("Connection failed");

    }
- Line 738: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 742: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string cmd = "n" + command + "\n";

    int bytes_sent = send(sock, cmd.c_str(), static_cast<int>(cmd.size()), 0);

    if (bytes_sent < 0 || static_cast<size_t>(bytes_sent) != cmd.size()) {

        throw std::runtime_error("Failed to send command to ClamAV");

    }

    

    // Receive response
- Line 746: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 748: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: response.reserve(8192);  // Pre-allocate for better performance
- Line 756: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    if (bytes_received < 0) {

        throw std::runtime_error("Failed to receive response from ClamAV");

    }

    

    return response;
- Line 785: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error("Invalid address");

    }

    

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {

        throw std::runtime_error("Connection failed");

    }
- Line 794: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int cmd_len = static_cast<int>(strlen(instream_cmd));

    int bytes_sent = send(sock, instream_cmd, cmd_len, 0);

    if (bytes_sent < 0 || bytes_sent != cmd_len) {

        throw std::runtime_error("Failed to send INSTREAM command to ClamAV");

    }

    

    // Send data in chunks (ClamAV protocol)
- Line 807: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: uint32_t size_n = htonl(static_cast<uint32_t>(to_send));

        bytes_sent = send(sock, reinterpret_cast<char*>(&size_n), 4, 0);

        if (bytes_sent < 0 || bytes_sent != 4) {

            throw std::runtime_error("Failed to send chunk size to ClamAV");

        }

        

        // Send chunk data
- Line 813: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Send chunk data

        bytes_sent = send(sock, data.c_str() + offset, static_cast<int>(to_send), 0);

        if (bytes_sent < 0 || static_cast<size_t>(bytes_sent) != to_send) {

            throw std::runtime_error("Failed to send chunk data to ClamAV");

        }

        offset += to_send;

    }
- Line 826: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 828: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: response.reserve(8192);  // Pre-allocate for better performance
- Line 836: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    if (bytes_received < 0) {

        throw std::runtime_error("Failed to receive response from ClamAV");

    }

    

    return response;
- Line 60: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 98: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 804: severity=MEDIUM; category=endianness_assumption
  Description: Endianness conversion may have undefined behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint32_t size_n = htonl(static_cast<uint32_t>(to_send));
- Line 900: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: (cached_version_.back() == '\n' || cached_version_.back() == '\r')) {

            cached_version_.pop_back();

        }

    } catch (...) {

        cached_version_ = "unknown";

    }
- Line 900: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### security/aql_injection_detector.cpp
Total findings: 15

- Line 103: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "Parse error with suspicious tokens [{}]: {}",
- Line 331: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& pattern : patterns) {
- Line 406: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ast.return_node && ast.return_node->expression) {
- Line 407: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (scanExpressionForDangerousOps(ast.return_node->expression)) {
- Line 502: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [key, value] : obj_expr->fields) {
- Line 547: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ast.return_node && ast.return_node->expression) {
- Line 548: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: extractStringLiteralsFromExpression(ast.return_node->expression, literals);
- Line 621: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& [key, value] : obj_expr->fields) {
- Line 24: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
- Line 230: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        auto result = parseAQL(template_str);

        return result.has_value();

    } catch (...) {

        return false;

    }

}
- Line 230: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex(R"(-{2}|/\*|\*/)"),
- Line 348: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex(R"(-{2}|/\*|\*/)"),
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back(match.str());
- Line 585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: literals.push_back(std::get<std::string>(literal_expr->value));

### security/confidential_computing.cpp
Total findings: 15

- Line 210: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ScopedFd fd(::open("/dev/cpu/0/msr", O_RDONLY | O_CLOEXEC | O_NONBLOCK));
- Line 391: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ScopedFd fd(::open("/dev/tdx_guest", O_RDWR | O_CLOEXEC | O_NONBLOCK));
- Line 418: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ScopedFd fd(::open("/dev/tdx_guest", O_RDWR | O_CLOEXEC | O_NONBLOCK));
- Line 476: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ScopedFd fd(::open(dev, O_RDONLY | O_CLOEXEC | O_NONBLOCK));
- Line 503: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ScopedFd fd(::open("/dev/sev-guest", O_RDWR | O_CLOEXEC | O_NONBLOCK));
- Line 88: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: #    define TDX_REPORTDATA_LEN  64U

#    define TDX_REPORT_LEN     1024U

     struct tdx_report_req {

         uint8_t reportdata[TDX_REPORTDATA_LEN];

         uint8_t tdreport[TDX_REPORT_LEN];

     };

#    define TDX_CMD_GET_REPORT0  _IOWR('T', 1, struct tdx_report_req)
- Line 99: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: #    define SNP_REPORT_DATA_SIZE 64U

#    define SNP_REPORT_SIZE     1184U

     struct snp_report_req {

         uint8_t user_data[SNP_REPORT_DATA_SIZE];

         uint32_t vmpl;

         uint8_t rsvd[28];

     };
- Line 147: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 254: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 291: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 531: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // For legacy AMD SEV (non-SNP) attestation is done via the platform
- Line 117: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 118: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
- Line 124: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 128: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### security/vcc_pki_client.cpp
Total findings: 15

- Line 165: severity=CRITICAL; category=missing_dtor
  Description: Class VCCPKIClient allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct VCCPKIClient
- Line 167: severity=CRITICAL; category=missing_dtor
  Description: Class curl_slist allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct curl_slist
- Line 203: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
- Line 206: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 260: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (res != CURLE_OK) {

            std::string error = "CURL request failed: ";

            error += curl_easy_strerror(res);

            throw std::runtime_error(error);

        }

        

        // Check HTTP status code
- Line 270: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (http_code < 200 || http_code >= 300) {

            std::ostringstream oss;

            oss << "HTTP error " << http_code << ": " << response;

            throw std::runtime_error(oss.str());

        }

        

        return response;
- Line 316: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



X509Certificate VCCPKIClient::requestCertificate(const CertificateRequest& request) {

    nlohmann::json body = request.toJson();

    

    std::string response = httpPost("/api/v1/certificates/request", body);
- Line 28: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(BIO* p) const { if (p) BIO_free(p); }
- Line 31: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509* p) const { if (p) X509_free(p); }
- Line 34: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509_STORE* p) const { if (p) X509_STORE_free(p); }
- Line 37: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509_STORE_CTX* p) const { if (p) X509_STORE_CTX_free(p); }
- Line 40: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(BIGNUM* p) const { if (p) BN_free(p); }
- Line 43: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(char* p) const { if (p) OPENSSL_free(p); }
- Line 203: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: crl.push_back(CRLEntry::fromJson(entry_json));

### security/embedded_user_registration_plugin.cpp
Total findings: 14

- Line 222: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 231: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 237: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 340: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t end = s.find(delim, start);
- Line 367: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: uint32_t t_cost = 2, m_cost = 19456, lanes = 1, threads = 1, version = 19;
- Line 373: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 431: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // We always produced m=19456,t=2,p=1 but parse them for forward compat.
- Line 458: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 532: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // --- Legacy SHA-256 path: plain 64-char hex ---
- Line 535: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(s.substr(start, end == std::string::npos ? end : end - start));
- Line 357: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int DK_LEN   = 32;
- Line 394: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_KDF_CTX_free(ctx);
- Line 539: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);

### security/rbac.cpp
Total findings: 14

- Line 140: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& role : getBuiltinRoles()) {
- Line 211: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& br : getBuiltinRoles()) {
- Line 555: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(user.roles.begin(), user.roles.end(), role) != user.roles.end()) {
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: builtin.push_back({
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: builtin.push_back({
- Line 191: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            nlohmann::json j = nlohmann::json::parse(content);

            return loadFromJson(j);

        } catch (...) {

            return loadFromYaml(content);

        }
- Line 191: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 209: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Role> builtin_backup;
- Line 341: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;
- Line 365: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;
- Line 383: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string>& visited
- Line 419: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visiting;
- Line 420: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited;
- Line 505: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = j["attributes"].begin(); it != j["attributes"].end(); ++it) {

### security/timestamp_authority_openssl.cpp
Total findings: 14

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5421 fix: thread-safety for Prov... (2026-06-01) | #3457 [TSA] Implement RFC
- Line 123: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 143: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: out.assign(ptr->data, ptr->length);
- Line 150: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 227: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 253: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 259: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 263: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 545: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 26: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'OpenSSL TSA Activation' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md §"OpenSSL TSA Activation"
- Line 71: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(STACK_OF(X509)* p) const { if (p) sk_X509_pop_free(p, X509_free); }
- Line 448: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(hexStr);
- Line 660: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: TS_TST_INFO_free(tst);
- Line 661: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: PKCS7_free(pkcs7);

### security/mock_key_provider.cpp
Total findings: 13

- Line 136: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Mark old ACTIVE keys as DEPRECATED
- Line 143: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 144: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 145: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 146: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 147: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 148: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 149: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 150: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 211: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 211: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto& entry = keys_[key_id][version];

    

    if (entry.metadata.status == KeyStatus::ACTIVE) {

        throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));

    }

    

    entry.metadata.status = KeyStatus::DELETED;
- Line 211: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
- Line 241: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### security/security_evidence_collector.cpp
Total findings: 12

- Line 158: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 6
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: raw[6] = (raw[6] & 0x0F) | 0x40;
- Line 158: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 6
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::memcpy(raw, &ns, sizeof(ns));

        std::memset(raw + sizeof(ns), 0, sizeof(raw) - sizeof(ns));

    }



    // Set version = 4 and variant = 10xx

    raw[6] = (raw[6] & 0x0F) | 0x40;

    raw[8] = (raw[8] & 0x3F) | 0x80;



    // Format as xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx

    std::ostringstream oss;

    oss << std::hex << std::setfill('0');
- Line 159: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 8
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: raw[8] = (raw[8] & 0x3F) | 0x80;
- Line 159: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 8
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::memset(raw + sizeof(ns), 0, sizeof(raw) - sizeof(ns));

    }



    // Set version = 4 and variant = 10xx

    raw[6] = (raw[6] & 0x0F) | 0x40;

    raw[8] = (raw[8] & 0x3F) | 0x80;



    // Format as xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx

    std::ostringstream oss;

    oss << std::hex << std::setfill('0');

    for (int i = 0; i < 16; ++i) {
- Line 239: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: snap.total_roles = static_cast<uint64_t>(rbac_->listRoles().size());
- Line 266: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 280: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 290: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 291: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 508: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(evidence_store_path)) {
- Line 150: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Fill with a monotonically-increasing timestamp-based value as last resort
- Line 267: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<KeyMetadata>> by_id;

### security/cms_signing.cpp
Total findings: 11

- Line 22: severity=CRITICAL; category=missing_dtor
  Description: Class CMS_BIO_Deleter allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct CMS_BIO_Deleter
- Line 25: severity=CRITICAL; category=missing_dtor
  Description: Class CMS_ContentInfo_Deleter allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct CMS_ContentInfo_Deleter
- Line 28: severity=CRITICAL; category=missing_dtor
  Description: Class CMS_X509_STORE_Deleter allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct CMS_X509_STORE_Deleter
- Line 51: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 71: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: res.signature.assign(reinterpret_cast<uint8_t*>(bptr->data), reinterpret_cast<uint8_t*>(bptr->data)
- Line 80: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 86: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 89: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 23: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(BIO* p) const { if (p) BIO_free(p); }
- Line 26: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(CMS_ContentInfo* p) const { if (p) CMS_ContentInfo_free(p); }
- Line 29: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509_STORE* p) const { if (p) X509_STORE_free(p); }

### security/keyprovider_signing.cpp
Total findings: 10

- Line 54: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 65: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 83: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 27: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(BIO* p) const { if (p) BIO_free(p); }
- Line 30: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); }
- Line 33: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(X509* p) const { if (p) X509_free(p); }
- Line 69: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: X509_ptr x(PEM_read_bio_X509(cbio.get(), nullptr, nullptr, nullptr));

                if (x) cert_ptr = x.release(); // transfer ownership to CMSSigningService below

            }

        } catch (...) {

            // missing cert is acceptable

        }
- Line 69: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 91: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return cms.verify(data, signature, key_id);

                }

            }

        } catch (...) {

            // fallthrough

        }

        // No cert available -> verification not possible here
- Line 91: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### security/manifest_signer.cpp
Total findings: 10

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3342 [plugins] Remote plugin loa... (2026-03-12) | #2533 [plugins] Add dedic
- Line 39: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 40: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 50: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string result(buffer_ptr->data, buffer_ptr->length);
- Line 50: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 60: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 61: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 182: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(root_path)) {
- Line 41: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: BIO_push(b64.get(), bio.release());
- Line 62: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: BIO_push(b64.get(), bio.release());

### security/usb_admin_authenticator.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #401 Replace Security Stubs with... (2026-03-11) | #1100 [WIP] Fix missing an
- Line 477: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 478: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 603: severity=HIGH; category=windows_only_api
  Description: Windows-only API RegOpenKeyEx without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: LONG rc = RegOpenKeyExA(
- Line 43: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); }
- Line 46: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
- Line 74: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'USBAdminAuthenticator Impl.' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md §USBAdminAuthenticator Impl.
- Line 479: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: BIO* result = BIO_push(b64.release(), bmem.release());
- Line 497: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n"

### security/access_control_manager.cpp
Total findings: 8

- Line 188: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: AccessDecision AccessControlManager::authorize(
- Line 217: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto abac_decision = policy_engine_.authorize(
- Line 280: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto context = authenticate(token, source_ip);
- Line 309: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return authorize(*context, resource, action);
- Line 127: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("AUDIT [AUTHENTICATION]: {}", audit_entry.dump());
- Line 145: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("AUDIT [AUTHENTICATION]: {}", audit_entry.dump());
- Line 176: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("AUDIT [AUTHENTICATION]: {}", audit_entry.dump());
- Line 494: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("AUDIT [ACCESS_CONTROL]: {}", audit_entry.dump());

### security/ai_snapshot_cleanup.cpp
Total findings: 8

- Line 47: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.snapshot_dir, ec)) {
- Line 61: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& sub : fs::recursive_directory_iterator(entry.path(), ec)) {
- Line 102: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto snaps = listSnapshots();

    int deleted = 0;



    // Phase 1: delete all snapshots that exceed the retention age.

    for (auto it = snaps.begin(); it != snaps.end(); ) {

        if (isExpired(*it, config_.retention_days)) {

            spdlog::info("AI Safety ASL-11: deleting expired snapshot '{}' (age > {} days)",
- Line 102: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Phase 1: delete all snapshots that exceed the retention age.
- Line 118: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }



    // Phase 2: delete oldest snapshots while total size exceeds the cap.

    const std::uint64_t max_bytes =

        config_.max_total_gb * std::uint64_t{1024} * std::uint64_t{1024} * std::uint64_t{1024};
- Line 118: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Phase 2: delete oldest snapshots while total size exceeds the cap.
- Line 12: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3 (ASL-11)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 3 (ASL-11)
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back({entry.path().string(), size, ts});

### security/field_encryption.cpp
Total findings: 6

- Line 299: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 306: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: //             (e.g. test harnesses, demo_encryption.cpp, some legacy startup paths).
- Line 662: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 38: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
- Line 100: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::string encoded(4 * ((data.size() + 2) / 3), '\0');
- Line 314: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Field Encryption Key Provider' that was not found in 'src/security/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/FUTURE_ENHANCEMENTS.md §Field Encryption Key Provider.

### security/intent_classifier.cpp
Total findings: 6

- Line 92: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // for AI Safety callers; standard callers keep 0.85 for backwards compat).
- Line 96: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: {"REMOVE @",         0.40},  // Parametrised single-key delete (bind var)
- Line 127: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: return hasFilter ? 0.25 : 0.90;  // Unfiltered full-collection delete
- Line 14: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 4: Zero-Trust & Post-Quantum Cryptography' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § "Phase 4: Zero-Trust & Post-Quantum Cryptography"
- Line 89: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 5 (ASL-4)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 5 (ASL-4)
- Line 96: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: {"REMOVE @",         0.40},  // Parametrised single-key delete (bind var)

### security/webdav_user_registration_plugin.cpp
Total findings: 6

- Line 653: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 29: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
- Line 603: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back("admin");
- Line 605: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back("operator");
- Line 607: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back("analyst");
- Line 609: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: roles.push_back("readonly");

### security/access_control.cpp
Total findings: 5

- Line 114: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: AccessControl::AuthenticationResult AccessControl::authenticate(const Credentials& credentials) {
- Line 461: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: bool AccessControl::authorize(const AuthorizationContext& context) {
- Line 490: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto abac_decision = policy_engine_.authorize(
- Line 557: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return authorize(context);
- Line 988: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization

### security/row_level_security.cpp
Total findings: 4

- Line 290: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(ctx.roles.begin(), ctx.roles.end(), required_role)
- Line 97: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Static: parse the stored value string.

        try {

            rhs_json = nlohmann::json::parse(value);

        } catch (...) {

            // Treat value as plain string if not valid JSON.

            rhs_json = value;

        }
- Line 97: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: matches.push_back(&policy);

### security/vram_secure_clear.cpp
Total findings: 4

- Line 46: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
- Line 118: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
- Line 191: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
- Line 193: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (int pass = 0; pass < config.num_passes; ++pass) {

        uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];

        for (size_t i = 0; i < size_bytes; ++i) {

            vptr[i] = pattern;

        }

    }

### security/zero_trust_policy_enforcer.cpp
Total findings: 4

- Line 312: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

        THEMIS_WARN("ZeroTrust: invalid CIDR prefix '{}' in '{}': {}", prefix_str, cidr, e.what());

        return false;

    } catch (...) {

        THEMIS_WARN("ZeroTrust: invalid CIDR prefix '{}' in '{}'", prefix_str, cidr);

        return false;

    }
- Line 312: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 360: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int prefix_len = 0;

    try {

        prefix_len = std::stoi(prefix_str);

    } catch (...) {

        THEMIS_WARN("ZeroTrust: invalid IPv6 CIDR prefix '{}' in '{}'", prefix_str, cidr);

        return false;

    }
- Line 360: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### security/arrow_user_registration_plugin.cpp
Total findings: 3

- Line 254: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 25: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
- Line 54: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: [[maybe_unused]] const std::unordered_map<std::string, std::string>& attributes)

### security/usb_volume_hardening.cpp
Total findings: 3

- Line 36: severity=CRITICAL; category=missing_dtor
  Description: Class USBVolume_EVP_MD_CTX_Deleter allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct USBVolume_EVP_MD_CTX_Deleter
- Line 82: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 37: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }

### security/secret_manager.cpp
Total findings: 2

- Line 164: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### security/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ARCHITECTURE.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### security/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### security/README.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'README.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### security/ROADMAP.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ROADMAP.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, README.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### security/ai_operation_guard.cpp
Total findings: 1

- Line 14: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 2 (ASL-4)' that was not found in 'src/security/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/security/ROADMAP.md § Phase 2 (ASL-4)

### security/prompt_injection_pattern_registry.cpp
Total findings: 1

- Line 87: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(\[\s*system\s*\]|\[INST\]|\[\/INST\]|<\|system\|>|<\|user\|>|<\|assistant\|>)",

### security/vault_signing_provider.cpp
Total findings: 1

- Line 59: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
