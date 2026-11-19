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

# Chimera module evidence

**Purpose / audience:** compact index of repository evidence for maintainers. This replaces the earlier issue #5635 snapshot, whose LOC counts, maturity scores, build-fix narrative, readiness statements, and test totals are not current verified facts.

## Current source inventory

- Public headers: `include/chimera/` contains the base adapter compatibility header, ThemisDB adapter, retry, batch, transaction, and local MongoDB/Neo4j/Qdrant declarations.
- Local implementation files: `src/chimera/{themisdb_adapter,mongodb_adapter,neo4j_adapter,qdrant_adapter,retry_executor,batch_executor,transaction}.cpp`.
- Local test files include ThemisDB adapter, streaming, prepared statements, high-cardinality stress, and MongoDB phase-2 tests. CMake does not register every local test in the same way; see [`tests/chimera/README.md`](../../tests/chimera/README.md) and `tests/CMakeLists.txt`.
- Benchmarks: `bench_chimera_adapter.cpp` exercises a default-constructed ThemisDB adapter; `bench_chimera_dedicated_gates.cpp` measures local router/merge stubs.

## Build evidence

`CMakeLists.txt` declares `THEMIS_BUILD_CHIMERA` with default `OFF`. Its enabled adapter wiring in `cmake/ChimeraAdapters.cmake` references `external/chimera` and local `themisdb_adapter.cpp`; it does not list every local implementation file. The focused-test helper additionally names several local source paths absent from the current `src/chimera/` inventory as well as external-checkout paths. No configuration or build was run, so resulting target validity is not confirmed.

## Status limits

This inventory does not establish that each adapter compiles, that vendor libraries are enabled, that tests pass, that performance targets are met, or that a human has reviewed the module. Historical scan artifacts under `audit/` may be consulted as dated evidence but are not current implementation truth by themselves.

## Validation evidence (2026-11-19)

- `g++ -std=c++20 -Wall -Wextra -Iinclude -Iexternal/chimera/include -c src/chimera/retry_executor.cpp`: passed.
- `g++ -std=c++20 -Wall -Wextra -Iinclude -Iexternal/chimera/include -Isrc -c src/chimera/themisdb_adapter.cpp`: passed with existing unused-parameter warnings in unrelated adapter methods.
- The new GTest cases cover error classification, default-off category retry, idempotency opt-in, invalid retry policy, attempt bounds, jitter cap, malformed URI authority, unsupported options/capabilities, and legacy capability aliases. The GTest target has not yet been configured or run.
- Local MongoDB batch and Neo4j graph capabilities are now reported only when their drivers are compiled in. Qdrant does not report vector or batch capabilities because its generated gRPC operation stubs are absent; after normal input/connection checks, insert, search, collection creation, and non-empty batch insert return `NOT_IMPLEMENTED` instead of claiming success. MongoDB transaction handles and Neo4j transaction placeholders are not reported as database transaction capability.
- A diagnostic project configure with `THEMIS_BUILD_TESTS=ON` stopped before test discovery because the required `fmt` dependency was unavailable.
- No live backend, sanitizer, integration, or performance benchmark was run; this is not v2.5.0 or v3.0.0 release sign-off.
- The Developer LLM Wiki has no curated Chimera page; these module-local evidence files remain the current source-near summary.

## Canonical evidence

Headers and implementations listed above; `CMakeLists.txt`; `cmake/ChimeraAdapters.cmake`; `tests/CMakeLists.txt`; `tests/chimera/CMakeLists.txt`; `benchmarks/CMakeLists.txt`; benchmark and test sources.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
