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

# Chimera test roadmap notes

**Status basis:** test sources and CMake declarations inspected for issue #7336; no build or test run.

## Current inventory and caveats

The directory contains ThemisDB adapter, streaming, prepared-statement, MongoDB phase-2, and stub-based high-cardinality stress tests. Root CMake conditionally registers three local test executables under `THEMIS_BUILD_CHIMERA`; additional helper registrations may resolve against local or external source paths. The local `tests/chimera/CMakeLists.txt` is not shown as included by the inspected CMake hierarchy. Thus complete target registration is not established.

## Follow-up

- [ ] Resolve the intended inclusion and ownership of `tests/chimera/CMakeLists.txt`.
- [ ] Reconcile local and external test source lists with files in the chosen checkout.
- [ ] Establish build/test results for each supported feature and vendor-driver configuration.
- [ ] Keep stub-only stress tests separate from claims about production dispatch or release readiness.

No completion, test-pass, coverage, or production-readiness status is asserted.

References: `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `tests/chimera/*.cpp`, `CMakeLists.txt`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
