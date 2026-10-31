---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera test security scope

**Purpose / audience:** describe security-relevant test boundaries, not runtime controls.

The tests in this directory exercise adapter behavior; they do not themselves provide authentication, authorization, secret storage, TLS, or production security enforcement. `test_chimera_highcardinality_stress.cpp` uses local stubs, and the ordinary ThemisDB adapter test fixture is simulation-oriented. Passing such tests would not prove the security of an engine-backed or vendor-driver deployment.

Before using test results as security evidence, identify the exact adapter/build definitions and exercise the real dependency and connection configuration. Do not commit credentials in test fixtures or logs. Report vulnerabilities according to the project-level [`../../SECURITY.md`](../../SECURITY.md).

References: `tests/chimera/*.cpp`, `tests/CMakeLists.txt`, `CMakeLists.txt`, `include/chimera/*.hpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
