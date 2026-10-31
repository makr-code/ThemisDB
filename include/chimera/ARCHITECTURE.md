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

# Chimera public-header architecture

**Purpose / audience:** describe the declared public-facing surfaces for adapter integrators; implementation and build availability are separate concerns.

## Header map

| Header | Declared responsibility |
|---|---|
| `database_adapter.hpp` | Uses the external CHIMERA header when found; otherwise supplies a deliberately minimal compatibility shim. It also declares local streaming and prepared-statement extensions. |
| `themisdb_adapter.hpp` | `ThemisDBAdapter`, `ThemisDBResultStream`, and `ThemisDBPreparedStatement`; declarations for adapter, async, streaming, and prepared-statement operations. |
| `retry_executor.hpp` | `RetryPolicy` and `RetryExecutor`. |
| `batch_executor.hpp` | `BatchConfig`, `BatchStatistics`, and `IBatchAdapter`. |
| `transaction.hpp` | `IsolationLevel`, `ChimeraTransactionState`, `Operation`, `TransactionContext`, and `TransactionHandle`. |
| `mongodb_adapter.hpp` | `MongoDBAdapter`, with optional driver includes under compile definitions. |
| `neo4j_adapter.hpp` | `Neo4jAdapter`, with conditional driver-specific members. |
| `qdrant_adapter.hpp` | `QdrantAdapter` and a private Qdrant gRPC client boundary. |

Most local declarations use namespace `chimera`; the injected ThemisDB engine types are forward-declared in namespace `themis`. The base types, enum details, and capabilities can vary with the external header versus the fallback shim; do not describe the complete surface as a fixed ABI based only on the shim.

## Build boundary

The root `THEMIS_BUILD_CHIMERA` option defaults to `OFF`. The enabled adapter source list is defined by `cmake/ChimeraAdapters.cmake` and references `external/chimera` as well as local `src/chimera/themisdb_adapter.cpp`. Header presence does not imply the corresponding implementation is built or vendor-enabled.

## References

Header declarations in this directory; [`../../src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md); `cmake/ChimeraAdapters.cmake`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
