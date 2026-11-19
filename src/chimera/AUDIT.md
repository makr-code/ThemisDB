---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-11-19
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

## Review scope for issue #7511

Source and wiring were inspected for contract synchronization: public headers in `include/chimera/`, local implementations in `src/chimera/`, focused tests, benchmark sources, and the relevant CMake entries. The retry and ThemisDB adapter translation units compiled with GNU 13.3.0 and C++20. Project configuration was attempted with diagnostic mode but stopped before test discovery because the required fmt package was not installed. No GTest execution, live integration, security scan, or benchmark was completed.

## Evidence and limits

- `connect()` rejects missing/whitespace-only URI authorities and unsupported option maps; successful connection state remains in-process and is not network reachability evidence.
- The local source tree contains ThemisDB, retry, batch, transaction, MongoDB, Neo4j, and Qdrant implementation files.
- The default `THEMIS_BUILD_CHIMERA` value is `OFF`.
- Vendor code paths are conditional; CMake's canonical adapter source list also references `external/chimera`. The presence of local adapter files alone does not establish that they are part of the default or enabled production target.
- `RetryExecutor` classifies concrete error codes, treats a non-empty retry-code set as an exclusive allowlist, validates policy bounds, and requires explicit idempotency opt-in. The operation callable still has no enforced deadline.
- The ThemisDB capability set omits unsupported distributed/full-text/geospatial/time-series/secondary-index and ACID transaction claims. Legacy prepared-statement and connection-pool token aliases are not reported as capabilities.
- `batch_executor.cpp` contains no shared queue/flush implementation; adapter-specific implementations live elsewhere.

## Audit status

No comprehensive source audit, runtime validation, or production-readiness assessment is claimed here. Any security or audit conclusion requires a separately scoped review with build configuration, dependency versions, executed tests, and deployment settings recorded.

## References

`include/chimera/*.hpp`, `src/chimera/*.cpp`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
