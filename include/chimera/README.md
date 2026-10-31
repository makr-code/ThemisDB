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

# Chimera public headers

**Purpose / audience:** entry point for developers reading or integrating the declarations under `include/chimera/`.

## API map

- `database_adapter.hpp`: external CHIMERA header selection or minimal local compatibility fallback; local streaming and prepared-statement extension declarations.
- `themisdb_adapter.hpp`: ThemisDB adapter and its stream/statement implementation types.
- `retry_executor.hpp`: retry policy and executor.
- `batch_executor.hpp`: batch configuration/statistics and adapter interface.
- `transaction.hpp`: transaction context, state, operation, and handle types.
- `mongodb_adapter.hpp`, `neo4j_adapter.hpp`, `qdrant_adapter.hpp`: local vendor adapter declarations with conditional driver paths.

The precise base API depends on whether the external header is found. See [`CHIMERA_ADAPTER_CONTRACT.md`](CHIMERA_ADAPTER_CONTRACT.md) before relying on enum values or compatibility claims.

## Build and use boundary

The root option `THEMIS_BUILD_CHIMERA` defaults to `OFF`; the enabled CMake wiring references `external/chimera` and local `src/chimera/themisdb_adapter.cpp`. Merely including these headers does not enable or link all adapter implementations. Build and runtime requirements are documented in [`../../src/chimera/README.md`](../../src/chimera/README.md).

### Quickstart

For a Chimera-enabled integration, configure with `-DTHEMIS_BUILD_CHIMERA=ON` after initializing the external checkout and its dependencies, then verify the generated target and selected header source. This describes the configuration switch, not a confirmed successful build.

## References

[`ARCHITECTURE.md`](ARCHITECTURE.md), [`CHIMERA_ADAPTER_CONTRACT.md`](CHIMERA_ADAPTER_CONTRACT.md), [`../../src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md), `cmake/ChimeraAdapters.cmake`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
