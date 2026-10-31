---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera test follow-up candidates

**Purpose / audience:** concrete test-infrastructure candidates based on current sources and CMake; not scheduled commitments.

- Establish whether `tests/chimera/CMakeLists.txt` is intended to be included. If so, wire it explicitly and verify which globbed tests can compile; otherwise remove the assumption from module guidance.
- Make CMake registrations correspond to present source files and distinguish local tests from tests supplied by `external/chimera`.
- Add configuration-specific coverage for engine-enabled versus simulation paths and for each optional vendor driver that maintainers choose to support.
- Exercise `RetryExecutor` against concrete error results; current implementation does not classify on the result's error code.
- Keep stub-only high-cardinality tests labeled as synthetic; add integration coverage before using them to support claims about real routing.

## Validation evidence

For any future test claim, record the configure option/dependencies, exact target and CTest name, command, and result. No test or build was run for issue #7336.

References: `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `tests/chimera/*.cpp`, `include/chimera/retry_executor.hpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
