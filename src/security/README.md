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
SOT_Domain: module-behavior
Canonical_References:
  - include/security/key_provider.h
  - include/security/encryption.h
  - include/security/access_control_manager.h
  - include/security/rbac.h
  - include/security/zero_trust_policy_enforcer.h
  - include/security/aql_injection_detector.h
  - include/security/security_evidence_collector.h
  - tests/security/CMakeLists.txt
  - tests/CMakeLists.txt
  - benchmarks/security/CMakeLists.txt
  - CMakePresets.json
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security Module

`src/security/` contains security-related implementations. Public declarations live in
[`include/security/`](../../include/security/); callers and deployment configuration
determine which controls are used. This overview does not certify that every control
is enabled or that the module is production-ready.

## Public interfaces

- Key lifecycle: `themis::KeyProvider`, `KeyMetadata`, and `KeyCache`
  ([`key_provider.h`](../../include/security/key_provider.h)).
- Field encryption: `themis::FieldEncryption` and `EncryptedBlob`
  ([`encryption.h`](../../include/security/encryption.h)).
- Access control: `themis::security::AccessControlManager` and
  `themis::security::RBAC` ([`access_control_manager.h`](../../include/security/access_control_manager.h),
  [`rbac.h`](../../include/security/rbac.h)).
- Request checks: `themis::security::ZeroTrustPolicyEnforcer` and
  `AQLInjectionDetector` (their respective headers).
- Evidence collection: `themis::security::SecurityEvidenceCollector`
  ([`security_evidence_collector.h`](../../include/security/security_evidence_collector.h)).

See [`API.md`](API.md) for a concise declaration-level summary and the headers for
full signatures. The implementation and module status are tracked in
[`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md).

## Build, tests, and configuration

The root `linux-release` preset sets `THEMIS_BUILD_TESTS=OFF` and
`THEMIS_BUILD_BENCHMARKS=OFF`; test targets are configured only when
`THEMIS_BUILD_TESTS=ON`. `tests/CMakeLists.txt` discovers
`tests/security/CMakeLists.txt`, whose flat `test_*.cpp` glob creates focused
targets and whose `test_security_*.cpp` glob also considers auto-focused targets.
There are 88 top-level `test_*.cpp` files matched by these flat globs. Four
additional `.cpp` files are nested: three attack-vector sources
(`tests/security/attack-vectors/{authentication,crypto,injection}/`) and one
under `tests/security/ai_safety/`. The flat globs do not recurse.

Security benchmarks require `THEMIS_BUILD_BENCHMARKS=ON` and
`benchmark::benchmark`. The security benchmark subdirectory skips
`bench_security.cpp` and `bench_compliance_security_governance.cpp`; the former
is separately registered by `benchmarks/CMakeLists.txt`.

There is no single security-module configuration object. For example,
`VaultKeyProvider::Config` is declared in `vault_key_provider.h`,
`AccessControlConfig` in `access_control_manager.h`, and `RBACConfig` in
`rbac.h`. Treat those declarations and their implementations as authoritative;
this page makes no claim that an example deployment or backend is enabled.

## References

- [`API.md`](API.md)
- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`ROADMAP.md`](ROADMAP.md)
- [`tests/security/README.md`](../../tests/security/README.md)
- [`benchmarks/security/README.md`](../../benchmarks/security/README.md)
