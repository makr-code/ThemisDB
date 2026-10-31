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

# Chimera source audit notes

**Purpose / audience:** identify what was and was not checked for developers planning an audit. This is not a completed security or release audit.

## Review scope for issue #7336

Source and wiring were inspected for documentation synchronization: public headers in `include/chimera/`, local implementations in `src/chimera/`, focused tests, benchmark sources, and the relevant CMake entries. No build, test, security scanner, or benchmark was run.

## Evidence and limits

- The local source tree contains ThemisDB, retry, batch, transaction, MongoDB, Neo4j, and Qdrant implementation files.
- The default `THEMIS_BUILD_CHIMERA` value is `OFF`.
- Vendor code paths are conditional; CMake's canonical adapter source list also references `external/chimera`. The presence of local adapter files alone does not establish that they are part of the default or enabled production target.
- `RetryExecutor::should_retry` currently uses coarse policy booleans and does not inspect a result's concrete error code. This is a behavior limitation, not a finding that a runtime test was performed.
- `batch_executor.cpp` contains no shared queue/flush implementation; adapter-specific implementations live elsewhere.

## Audit status

No comprehensive source audit, runtime validation, or production-readiness assessment is claimed here. Any security or audit conclusion requires a separately scoped review with build configuration, dependency versions, executed tests, and deployment settings recorded.

## References

`include/chimera/*.hpp`, `src/chimera/*.cpp`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
