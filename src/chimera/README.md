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

# Chimera module

**Purpose / audience:** developer entry point for the checked-in adapter interfaces and implementations. This page describes source presence, not a product-support promise.

## Scope and API

Public headers are in [`../../include/chimera/`](../../include/chimera/). `ThemisDBAdapter` implements CHIMERA's database, async, streaming, and prepared-statement surfaces. It also accepts optional ThemisDB query/vector/graph engine pointers; engine dispatch is conditional. Supporting headers define retry, batch, and transaction-context APIs.

Local `MongoDBAdapter`, `Neo4jAdapter`, and `QdrantAdapter` declarations and implementations also exist. Their backend paths are conditional on compile definitions and external dependencies. The main CMake adapter list references external `external/chimera` implementations in addition to the local ThemisDB adapter; do not infer every local class is enabled in a normal build.

## Build and tests

The root option `THEMIS_BUILD_CHIMERA` is `OFF` by default. Enabling the suite uses `cmake/ChimeraAdapters.cmake` and requires the external CHIMERA checkout used there; the listed sources do not include every local `src/chimera/*.cpp`. Focused test wiring is conditional and has both local and external source references, including local source paths not present in the current directory inventory; see [`../../tests/chimera/README.md`](../../tests/chimera/README.md). No configure, build, or test run is claimed here.

### Quickstart

For an intended Chimera-enabled build, configure with `-DTHEMIS_BUILD_CHIMERA=ON` only after initializing the required external checkout and dependencies; then inspect the generated targets before building. The enabled CMake source lists have mixed local/external references, so a successful configure/build for a particular checkout is not asserted by this guide.

## Limitations

The retry helper currently does not classify by concrete error code; batch execution is adapter-specific; transaction helper state does not establish distributed atomicity. The dedicated CHIMERA benchmark file contains local stubs. See [`ARCHITECTURE.md`](ARCHITECTURE.md), [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), and [`SECURITY.md`](SECURITY.md).

## Canonical references

`include/chimera/*.hpp`, `src/chimera/*.cpp`, `CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `benchmarks/CMakeLists.txt`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
