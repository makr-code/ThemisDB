---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the original file creation date is unknown.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: api-contract
Canonical_References:
  - include/security/key_provider.h
  - include/security/encryption.h
  - include/security/access_control_manager.h
  - include/security/rbac.h
  - include/security/vault_key_provider.h
  - include/security/security_api_contract.h
  - src/security/README.md
  - tests/security/CMakeLists.txt
  - CMakePresets.json
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security public headers

`include/security/` declares security-related C++ APIs. Namespaces differ by
header: for example, `KeyProvider`, `FieldEncryption`, and `VaultKeyProvider`
are in `themis`, while `RBAC` and `AccessControlManager` are in
`themis::security`. Use the declarations as the contract; this index does not
promise that every API is enabled or integrated in every deployment.

## Representative contracts

| Header | Declared API |
|---|---|
| [`key_provider.h`](key_provider.h) | `themis::KeyProvider`, `KeyMetadata`, `KeyStatus`, and separate `KeyCache` |
| [`encryption.h`](encryption.h) | `themis::FieldEncryption`, `EncryptedBlob`, `EncryptedField<T>` |
| [`vault_key_provider.h`](vault_key_provider.h) | `themis::VaultKeyProvider` and `VaultKeyProvider::Config` |
| [`access_control_manager.h`](access_control_manager.h) | `themis::security::AccessControlManager`, `AccessControlConfig`, and access decisions |
| [`rbac.h`](rbac.h) | `themis::security::RBAC`, roles, permissions, and `UserRoleStore` |
| [`zero_trust_policy_enforcer.h`](zero_trust_policy_enforcer.h) | Request context, network policy, and verification result declarations |
| [`security_evidence_collector.h`](security_evidence_collector.h) | `themis::security::SecurityEvidenceCollector` and evidence structures |

This is a representative index, not a backend compatibility or security
certification list. See [`src/security/API.md`](../../src/security/API.md) for
the concise declaration summary and [`src/security/README.md`](../../src/security/README.md)
for build and test configuration notes.

## Build, tests, and configuration

The `linux-release` CMake preset sets `THEMIS_BUILD_TESTS=OFF`. Security test
targets are configured when `THEMIS_BUILD_TESTS=ON`; the security test CMake
file generates focused targets from flat test sources and separately considers
`test_security_*.cpp` auto-focused targets. `VaultKeyProvider::Config`,
`AccessControlConfig`, and `RBACConfig` are the current header-level
configuration structures; there is no single header-defined security-module
configuration object.

`include/security/security_api_contract.h` documents policy constants and
normative requirements. Do not treat the presence of a constant or requirement
there as proof that all runtime paths enforce it.
