# security — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **security** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 3648
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 70
- **HIGH**: 255
- **MEDIUM**: 3320
- **LOW**: 3

### By Type

- arithmetic_overflow: 1
- braces_imbalance: 7
- braces_imbalance_midfile: 11
- catch_all_swallow: 4
- circular_lock_ordering: 10
- copy_overhead: 13
- critical_function_noexcept: 3
- data_race: 1
- db_connection_leak: 4
- deadlock_risk: 7
- delete_no_nullptr: 4
- delete_without_nullptr: 5
- duplicate_qualified_signature: 1
- endianness_assumption: 1
- exception_in_destructor: 5
- expensive_inner_op: 1
- generic_catch: 19
- hardcoded_path: 8
- legacy_or_compat_path: 12
- manual_cleanup: 47
- missing_audit_log: 3
- missing_dtor: 9
- missing_noexcept_on_move: 2
- missing_volatile: 8
- module_doc_linkset_drift: 4
- no_retry_logic: 23
- no_timeout: 5
- no_transit_encryption: 39
- null_dereference: 9
- o_n_squared: 1
- path_traversal: 1
- pointer_arithmetic_unbounded: 3
- range_temporary: 12
- repeated_search: 2
- resource_leaked_in_exception: 2
- scope_mismatch: 3144
- sensitive_data_logging: 1
- simulation_stub_marker: 1
- size_assumption: 5
- socket_leak: 2
- stale_doc_section_reference: 15
- string_concat_loop: 14
- todo_as_productionlogic: 88
- uncaught_exception: 51
- unchecked_memcpy: 1
- unchecked_result: 16
- uninitialized_access: 13
- uninitialized_array: 5
- uninitialized_variable: 4
- windows_only_api: 1

## Top 20 Gaps

- [braces_imbalance] access_control.cpp:1 (CRITICAL)
- [braces_imbalance] fips_crypto_mode.cpp:1 (CRITICAL)
- [braces_imbalance] pki_key_provider.cpp:1 (CRITICAL)
- [braces_imbalance] timestamp_authority_openssl.cpp:1 (CRITICAL)
- [missing_dtor] cms_signing.cpp:33 (CRITICAL)
- [missing_dtor] cms_signing.cpp:36 (CRITICAL)
- [missing_dtor] cms_signing.cpp:39 (CRITICAL)
- [missing_dtor] usb_volume_hardening.cpp:47 (CRITICAL)
- [exception_in_destructor] cms_signing.cpp:56 (CRITICAL)
- [missing_dtor] hsm_provider.cpp:67 (CRITICAL)
- [no_transit_encryption] vault_key_provider.cpp:141 (CRITICAL)
- [missing_dtor] vcc_pki_client.cpp:176 (CRITICAL)
- [missing_dtor] vcc_pki_client.cpp:178 (CRITICAL)
- [exception_in_destructor] vcc_pki_client.cpp:197 (CRITICAL)
- [scope_mismatch] encrypted_field.cpp:202 (CRITICAL)
- [scope_mismatch] encrypted_field.cpp:203 (CRITICAL)
- [scope_mismatch] encrypted_field.cpp:204 (CRITICAL)
- [scope_mismatch] encrypted_field.cpp:205 (CRITICAL)
- [no_timeout] confidential_computing.cpp:221 (CRITICAL)
- [no_transit_encryption] webdav_user_registration_plugin.cpp:228 (CRITICAL)

... and 3628 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
