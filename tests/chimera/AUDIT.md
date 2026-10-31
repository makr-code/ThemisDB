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

# Chimera test audit notes

**Purpose / audience:** record test-source and CMake evidence boundaries. This is not a test execution report or an audit sign-off.

## Static review performed

The local test sources and CMake declarations were inspected for issue #7336. Root CMake conditionally declares ThemisDB adapter, streaming, and prepared-statement tests when `THEMIS_BUILD_CHIMERA` is enabled. That option defaults off. Additional focused registrations look up local or external sources. Its common adapter source list also names local paths absent from the current `src/chimera/` inventory; no configure was run, so target-generation behavior is unknown. The local `tests/chimera/CMakeLists.txt` has a glob-based recipe, but no `add_subdirectory(chimera)` reference was found in the inspected test/top-level CMake files.

## Evidence limitations

- No configuration, compilation, CTest discovery, test execution, sanitizer, or coverage run was performed.
- `test_chimera_highcardinality_stress.cpp` exercises in-process stubs, not actual adapter infrastructure.
- Source presence alone does not establish that the MongoDB phase-2 test is registered or that optional vendor drivers are enabled.
- Historical test LOC totals and statements that the suite is "validated" are not repeated as current results.

## Follow-up

For a test audit, configure each supported `THEMIS_BUILD_CHIMERA`/dependency combination, capture generated targets and CTest names, then run and report the exact focused suites.

References: `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `tests/chimera/*.cpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
