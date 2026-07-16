# auth — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **auth** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 2759
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 57
- **HIGH**: 225
- **MEDIUM**: 2475
- **LOW**: 2

### By Type

- blocking_no_timeout: 7
- braces_imbalance: 13
- braces_imbalance_midfile: 8
- catch_all_swallow: 4
- circular_lock_ordering: 20
- copy_overhead: 6
- crypto_weakness: 9
- data_race: 2
- db_connection_leak: 2
- deadlock_risk: 2
- delete_no_nullptr: 1
- delete_without_nullptr: 2
- exception_in_destructor: 5
- generic_catch: 5
- hardcoded_path: 1
- legacy_or_compat_path: 5
- lock_contention: 6
- manual_cleanup: 23
- memory_order: 2
- missing_audit_log: 7
- missing_noexcept_on_move: 7
- missing_volatile: 3
- module_doc_linkset_drift: 2
- no_retry_logic: 22
- no_timeout: 10
- no_transit_encryption: 13
- null_dereference: 12
- plaintext_transmission: 4
- range_temporary: 4
- repeated_search: 1
- resource_leaked_in_exception: 5
- scope_mismatch: 2213
- sensitive_data_logging: 155
- shift_overflow: 2
- size_assumption: 7
- smart_ptr_misuse: 2
- stale_doc_section_reference: 2
- string_concat_loop: 18
- todo_as_productionlogic: 62
- uncaught_exception: 54
- unchecked_result: 11
- uninitialized_access: 14
- uninitialized_variable: 6

## Top 20 Gaps

- [braces_imbalance] auth_metrics.cpp:1 (CRITICAL)
- [braces_imbalance] federated_identity_manager.cpp:1 (CRITICAL)
- [braces_imbalance] oauth_device_flow.cpp:1 (CRITICAL)
- [braces_imbalance] oauth_pkce_flow.cpp:1 (CRITICAL)
- [braces_imbalance] oidc_provider.cpp:1 (CRITICAL)
- [braces_imbalance] saml_authenticator.cpp:1 (CRITICAL)
- [braces_imbalance] session_manager.cpp:1 (CRITICAL)
- [braces_imbalance] totp_secret_encryption.cpp:1 (CRITICAL)
- [exception_in_destructor] totp_secret_encryption.cpp:52 (CRITICAL)
- [exception_in_destructor] mtls_authenticator.cpp:122 (CRITICAL)
- [exception_in_destructor] http_auth_async.cpp:144 (CRITICAL)
- [db_connection_leak] gssapi_authenticator.cpp:151 (CRITICAL)
- [blocking_no_timeout] ldap_connection_pool.cpp:157 (CRITICAL)
- [no_timeout] ldap_connection_pool.cpp:157 (CRITICAL)
- [blocking_no_timeout] jwt_validator.cpp:181 (CRITICAL)
- [no_timeout] jwt_validator.cpp:181 (CRITICAL)
- [no_transit_encryption] http_auth_async.cpp:183 (CRITICAL)
- [no_transit_encryption] http_auth_async.cpp:184 (CRITICAL)
- [no_transit_encryption] http_auth_async.cpp:188 (CRITICAL)
- [no_transit_encryption] http_auth_async.cpp:189 (CRITICAL)

... and 2739 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
