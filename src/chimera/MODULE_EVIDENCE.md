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

## Canonical evidence

Headers and implementations listed above; `CMakeLists.txt`; `cmake/ChimeraAdapters.cmake`; `tests/CMakeLists.txt`; `tests/chimera/CMakeLists.txt`; `benchmarks/CMakeLists.txt`; benchmark and test sources.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
