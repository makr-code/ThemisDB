---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the prior generated README was dated 2026-04-17.
Prior_Generated_Date: 2026-04-17
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
  - tests/security/CMakeLists.txt
  - tests/CMakeLists.txt
  - cmake/CMakeLists.txt
  - CMakePresets.json
  - tests/security/test_security_evidence_collector.cpp
  - tests/security/test_fips_crypto_mode.cpp
  - tests/security/test_security_negative_integration.cpp
  - tests/security/test_security_wavec_production_validation_focused.cpp
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security tests

`tests/security/` has 88 top-level `test_*.cpp` files, which are the sources
matched by the flat globs in [`CMakeLists.txt`](CMakeLists.txt). Separately,
there are three nested attack-vector sources under
`attack-vectors/{authentication,crypto,injection}/` and one nested source under
`ai_safety/`. These four nested sources are not matched by the flat globs.

## CMake behavior

`tests/CMakeLists.txt` discovers module directories containing a
`CMakeLists.txt`, including this directory. The security CMake file:

- globs top-level `test_*.cpp` into `module_security_<stem>_focused` targets;
- separately globs `test_security_*.cpp` into auto-focused targets and skips
  target creation when that target name already exists;
- registers these module-focused tests with the project test helper.

The `test_security_*.cpp` files match both source globs, but the two target-name
patterns differ. The flat globs are not recursive; this page makes no claim that the nested
attack-vector or `ai_safety` sources are registered by this file. The root
`tests/CMakeLists.txt` also contains explicit focused registrations for selected
security tests, including the evidence-collector test.

Tests are configured only when `THEMIS_BUILD_TESTS=ON`; the `linux-release`
preset sets that option to `OFF`. The source inventory and CMake rules describe
test targets, not proof of product security, full coverage, or production
readiness.

## Representative test areas

The current source names include access control and RBAC, authentication and
token flows, key rotation and HSM/PKCS#11, FIPS mode, query/input validation,
security evidence, negative integration, and production-validation cases.
Examples include `test_rbac_comprehensive.cpp`,
`test_hsm_provider.cpp`, `test_input_validation_security.cpp`,
`test_security_evidence_collector.cpp`, and
`test_security_wavec_production_validation_focused.cpp`.
